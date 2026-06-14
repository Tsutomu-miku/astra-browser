// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_command_service.h"

#include <algorithm>
#include <unordered_map>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Pref keys
// ---------------------------------------------------------------------------

// List of recent command IDs (most recent first).
inline constexpr char kPrefRecentCommands[] = "astra.commands.recent";

// Dictionary of custom keybindings: command_id -> keybinding string.
inline constexpr char kPrefCustomKeybindings[] = "astra.commands.keybindings";

// ---------------------------------------------------------------------------
// Built-in command constants
// ---------------------------------------------------------------------------

// Workspace commands
constexpr char kCommandNewWorkspace[] = "workspace.new";
constexpr char kCommandSwitchWorkspace[] = "workspace.switch";
constexpr char kCommandRenameWorkspace[] = "workspace.rename";
constexpr char kCommandDeleteWorkspace[] = "workspace.delete";

// Tab commands
constexpr char kCommandNewTab[] = "tab.new";
constexpr char kCommandCloseTab[] = "tab.close";
constexpr char kCommandDuplicateTab[] = "tab.duplicate";
constexpr char kCommandPinTab[] = "tab.pin";
constexpr char kCommandMuteTab[] = "tab.mute";
constexpr char kCommandReloadTab[] = "tab.reload";

// Navigation commands
constexpr char kCommandBack[] = "navigation.back";
constexpr char kCommandForward[] = "navigation.forward";
constexpr char kCommandRefresh[] = "navigation.refresh";
constexpr char kCommandHome[] = "navigation.home";

// Search commands
constexpr char kCommandSearchGoogle[] = "search.google";
constexpr char kCommandSearchBookmarks[] = "search.bookmarks";
constexpr char kCommandSearchHistory[] = "search.history";

// System commands
constexpr char kCommandToggleSidebar[] = "system.toggle_sidebar";
constexpr char kCommandToggleFullscreen[] = "system.toggle_fullscreen";
constexpr char kCommandOpenSettings[] = "system.open_settings";
constexpr char kCommandExit[] = "system.exit";

// Categories
constexpr char kCategoryWorkspace[] = "workspace";
constexpr char kCategoryTab[] = "tab";
constexpr char kCategoryNavigation[] = "navigation";
constexpr char kCategorySearch[] = "search";
constexpr char kCategorySystem[] = "system";

// Returns a case-insensitive substring match.
bool CaseInsensitiveContains(const std::string& text,
                             const std::string& query) {
  if (query.empty()) {
    return true;
  }
  std::string text_lower = base::ToLowerASCII(text);
  std::string query_lower = base::ToLowerASCII(query);
  return text_lower.find(query_lower) != std::string::npos;
}

// Compares two commands for sorting by rank descending, then name ascending.
bool CompareCommandRank(const AstraCommandInfo& a, const AstraCommandInfo& b) {
  if (a.rank != b.rank) {
    return a.rank > b.rank;  // Higher rank first.
  }
  return a.display_name < b.display_name;
}

}  // namespace

// ===========================================================================
// AstraCommandService
// ===========================================================================

AstraCommandService::AstraCommandService(Profile* profile)
    : profile_(profile) {
  RegisterBuiltInCommands();
  LoadFromPrefs();
}

AstraCommandService::~AstraCommandService() = default;

void AstraCommandService::Shutdown() {
  // KeyedService shutdown: clear all observer references and drop profile
  // pointer before the profile goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraCommandService::AddObserver(AstraCommandServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraCommandService::RemoveObserver(AstraCommandServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Command query -----------------------------------------------------------

std::vector<AstraCommandInfo> AstraCommandService::GetAllCommands() const {
  std::vector<AstraCommandInfo> result = commands_;
  base::ranges::sort(result, CompareCommandRank);
  return result;
}

const AstraCommandInfo* AstraCommandService::GetCommand(
    const std::string& command_id) const {
  auto it = base::ranges::find(commands_, command_id,
                               &AstraCommandInfo::command_id);
  return it == commands_.end() ? nullptr : &(*it);
}

std::vector<AstraCommandInfo> AstraCommandService::GetCommandsByCategory(
    const std::string& category) const {
  std::vector<AstraCommandInfo> result;
  for (const auto& cmd : commands_) {
    if (cmd.category == category && cmd.is_enabled) {
      result.push_back(cmd);
    }
  }
  base::ranges::sort(result, CompareCommandRank);
  return result;
}

std::vector<AstraCommandInfo> AstraCommandService::SearchCommands(
    const std::string& query) const {
  std::vector<AstraCommandInfo> result;

  std::string trimmed_query = query;
  base::TrimWhitespaceASCII(trimmed_query, base::TRIM_ALL, &trimmed_query);

  if (trimmed_query.empty()) {
    // Empty query: return all enabled commands.
    for (const auto& cmd : commands_) {
      if (cmd.is_enabled) {
        result.push_back(cmd);
      }
    }
  } else {
    for (const auto& cmd : commands_) {
      if (!cmd.is_enabled) {
        continue;
      }
      // Match against display name or description.
      if (CaseInsensitiveContains(cmd.display_name, trimmed_query) ||
          CaseInsensitiveContains(cmd.description, trimmed_query)) {
        result.push_back(cmd);
      }
    }
  }

  base::ranges::sort(result, CompareCommandRank);
  return result;
}

// -- Command execution -------------------------------------------------------

bool AstraCommandService::ExecuteCommand(const std::string& command_id) {
  AstraCommandInfo* cmd = FindCommand(command_id);
  if (!cmd || !cmd->is_enabled) {
    return false;
  }

  // TODO(astra): Real command execution requires dispatching to Chromium's
  // command system.  Each command category maps to different Chromium
  // subsystems:
  //
  //   - Tab commands:  TabStripModel / Browser (new tab, close tab, etc.)
  //     Chromium owner: chrome/browser/ui/tabs/tab_strip_model.h
  //     Patch point: BrowserCommandController or TabStripModel observer.
  //
  //   - Navigation commands:  content::WebContents (back, forward, reload)
  //     Chromium owner: content/public/browser/web_contents.h
  //     Patch point: NavigationController.
  //
  //   - Workspace commands:  AstraWorkspaceService
  //     Astra owner: astra/browser/astra_workspace_service.h
  //
  //   - Search commands:  Omnibox / SearchEngineHelper
  //     Chromium owner: chrome/browser/ui/omnibox/omnibox_view.h
  //
  //   - System commands:  Browser window controls
  //     Chromium owner: chrome/browser/ui/browser_window.h
  //
  // For now, this is a stub that records the execution and notifies
  // observers.  The actual command behavior is handled by the UI layer
  // or by Chromium's command system directly.

  AddToRecentCommands(command_id);

  for (auto& observer : observers_) {
    observer.OnCommandExecuted(command_id, cmd->display_name);
  }

  return true;
}

// -- Recent commands ---------------------------------------------------------

std::vector<AstraCommandInfo> AstraCommandService::GetRecentCommands() const {
  std::vector<AstraCommandInfo> result;
  for (const auto& id : recent_command_ids_) {
    const AstraCommandInfo* cmd = GetCommand(id);
    if (cmd && cmd->is_enabled) {
      result.push_back(*cmd);
    }
  }
  return result;
}

// -- Suggested commands ------------------------------------------------------

std::vector<AstraCommandInfo> AstraCommandService::GetSuggestedCommands()
    const {
  std::vector<AstraCommandInfo> result;

  // Start with recent commands (most relevant due to recency).
  std::vector<AstraCommandInfo> recent = GetRecentCommands();

  // Add high-rank commands that haven't been used recently.
  std::vector<AstraCommandInfo> all_enabled;
  for (const auto& cmd : commands_) {
    if (cmd.is_enabled) {
      all_enabled.push_back(cmd);
    }
  }
  base::ranges::sort(all_enabled, CompareCommandRank);

  // Build suggestion list: recent first, then top-ranked commands.
  // Cap at 8 suggestions.
  constexpr size_t kMaxSuggestions = 8;

  for (const auto& cmd : recent) {
    if (result.size() >= kMaxSuggestions) {
      break;
    }
    result.push_back(cmd);
  }

  for (const auto& cmd : all_enabled) {
    if (result.size() >= kMaxSuggestions) {
      break;
    }
    // Skip if already in result from recent commands.
    bool already_included = false;
    for (const auto& included : result) {
      if (included.command_id == cmd.command_id) {
        already_included = true;
        break;
      }
    }
    if (!already_included) {
      result.push_back(cmd);
    }
  }

  return result;
}

// -- Command registration ----------------------------------------------------

bool AstraCommandService::RegisterCommand(
    const AstraCommandInfo& command_info) {
  if (command_info.command_id.empty()) {
    return false;
  }

  // Disallow duplicate IDs.
  if (FindCommand(command_info.command_id)) {
    return false;
  }

  commands_.push_back(command_info);

  for (auto& observer : observers_) {
    observer.OnCommandRegistered(command_info.command_id);
  }

  return true;
}

bool AstraCommandService::UnregisterCommand(const std::string& command_id) {
  auto it = base::ranges::find(commands_, command_id,
                               &AstraCommandInfo::command_id);
  if (it == commands_.end()) {
    return false;
  }

  commands_.erase(it);

  // Also remove from recent list.
  auto recent_it =
      std::find(recent_command_ids_.begin(), recent_command_ids_.end(),
                command_id);
  if (recent_it != recent_command_ids_.end()) {
    recent_command_ids_.erase(recent_it);
    SaveRecentToPrefs();
  }

  // Also remove custom keybinding.
  auto kb_it = custom_keybindings_.find(command_id);
  if (kb_it != custom_keybindings_.end()) {
    custom_keybindings_.erase(kb_it);
    SaveKeybindingsToPrefs();
  }

  for (auto& observer : observers_) {
    observer.OnCommandUnregistered(command_id);
  }

  return true;
}

bool AstraCommandService::SetCommandEnabled(const std::string& command_id,
                                            bool enabled) {
  AstraCommandInfo* cmd = FindCommand(command_id);
  if (!cmd) {
    return false;
  }

  if (cmd->is_enabled == enabled) {
    return false;
  }

  cmd->is_enabled = enabled;
  return true;
}

// -- Custom keybindings ------------------------------------------------------

std::string AstraCommandService::GetCustomKeybinding(
    const std::string& command_id) const {
  auto it = custom_keybindings_.find(command_id);
  if (it != custom_keybindings_.end()) {
    return it->second;
  }
  return std::string();
}

void AstraCommandService::SetCustomKeybinding(const std::string& command_id,
                                              const std::string& keybinding) {
  if (keybinding.empty()) {
    custom_keybindings_.erase(command_id);
  } else {
    custom_keybindings_[command_id] = keybinding;
  }
  SaveKeybindingsToPrefs();
}

// -- Incognito compatibility -------------------------------------------------

bool AstraCommandService::IsIncognito() const {
  if (!profile_) {
    return false;
  }
  // TODO(astra): Use profile_->IsIncognitoProfile() once we're in a real
  // Chromium build.  For the overlay repo stub, check the profile name.
  // Chromium owner: Profile::IsIncognitoProfile().
  return false;
}

// -- Private helpers ---------------------------------------------------------

AstraCommandInfo* AstraCommandService::FindCommand(
    const std::string& command_id) {
  auto it = base::ranges::find(commands_, command_id,
                               &AstraCommandInfo::command_id);
  return it == commands_.end() ? nullptr : &(*it);
}

void AstraCommandService::RegisterBuiltInCommands() {
  // Workspace commands
  RegisterCommand({kCommandNewWorkspace, "New Workspace",
                   "Create a new workspace", kCategoryWorkspace,
                   /*icon_id=*/"workspace-new",
                   /*keybinding=*/"Ctrl+Shift+W",
                   /*is_enabled=*/true, /*rank=*/90});
  RegisterCommand({kCommandSwitchWorkspace, "Switch Workspace",
                   "Switch to a different workspace", kCategoryWorkspace,
                   /*icon_id=*/"workspace-switch",
                   /*keybinding=*/"Ctrl+`",
                   /*is_enabled=*/true, /*rank=*/85});
  RegisterCommand({kCommandRenameWorkspace, "Rename Workspace",
                   "Rename the current workspace", kCategoryWorkspace,
                   /*icon_id=*/"workspace-rename",
                   /*keybinding=*/"",
                   /*is_enabled=*/true, /*rank=*/70});
  RegisterCommand({kCommandDeleteWorkspace, "Delete Workspace",
                   "Delete the current workspace", kCategoryWorkspace,
                   /*icon_id=*/"workspace-delete",
                   /*keybinding=*/"",
                   /*is_enabled=*/true, /*rank=*/60});

  // Tab commands
  RegisterCommand({kCommandNewTab, "New Tab",
                   "Open a new tab", kCategoryTab,
                   /*icon_id=*/"tab-new",
                   /*keybinding=*/"Ctrl+T",
                   /*is_enabled=*/true, /*rank=*/100});
  RegisterCommand({kCommandCloseTab, "Close Tab",
                   "Close the current tab", kCategoryTab,
                   /*icon_id=*/"tab-close",
                   /*keybinding=*/"Ctrl+W",
                   /*is_enabled=*/true, /*rank=*/95});
  RegisterCommand({kCommandDuplicateTab, "Duplicate Tab",
                   "Duplicate the current tab", kCategoryTab,
                   /*icon_id=*/"tab-duplicate",
                   /*keybinding=*/"Ctrl+Shift+D",
                   /*is_enabled=*/true, /*rank=*/75});
  RegisterCommand({kCommandPinTab, "Pin Tab",
                   "Pin or unpin the current tab", kCategoryTab,
                   /*icon_id=*/"tab-pin",
                   /*keybinding=*/"Alt+P",
                   /*is_enabled=*/true, /*rank=*/65});
  RegisterCommand({kCommandMuteTab, "Mute Tab",
                   "Mute or unmute the current tab", kCategoryTab,
                   /*icon_id=*/"tab-mute",
                   /*keybinding=*/"Ctrl+M",
                   /*is_enabled=*/true, /*rank=*/68});
  RegisterCommand({kCommandReloadTab, "Reload Tab",
                   "Reload the current tab", kCategoryTab,
                   /*icon_id=*/"tab-reload",
                   /*keybinding=*/"Ctrl+R",
                   /*is_enabled=*/true, /*rank=*/80});

  // Navigation commands
  RegisterCommand({kCommandBack, "Back",
                   "Go back to the previous page", kCategoryNavigation,
                   /*icon_id=*/"nav-back",
                   /*keybinding=*/"Alt+Left",
                   /*is_enabled=*/true, /*rank=*/85});
  RegisterCommand({kCommandForward, "Forward",
                   "Go forward to the next page", kCategoryNavigation,
                   /*icon_id=*/"nav-forward",
                   /*keybinding=*/"Alt+Right",
                   /*is_enabled=*/true, /*rank=*/80});
  RegisterCommand({kCommandRefresh, "Refresh",
                   "Reload the current page", kCategoryNavigation,
                   /*icon_id=*/"nav-refresh",
                   /*keybinding=*/"F5",
                   /*is_enabled=*/true, /*rank=*/88});
  RegisterCommand({kCommandHome, "Home",
                   "Go to the home page", kCategoryNavigation,
                   /*icon_id=*/"nav-home",
                   /*keybinding=*/"Alt+Home",
                   /*is_enabled=*/true, /*rank=*/60});

  // Search commands
  RegisterCommand({kCommandSearchGoogle, "Search Google",
                   "Search Google with the selected text", kCategorySearch,
                   /*icon_id=*/"search-google",
                   /*keybinding=*/"Ctrl+K",
                   /*is_enabled=*/true, /*rank=*/90});
  RegisterCommand({kCommandSearchBookmarks, "Search Bookmarks",
                   "Search through your bookmarks", kCategorySearch,
                   /*icon_id=*/"search-bookmarks",
                   /*keybinding=*/"Ctrl+Shift+O",
                   /*is_enabled=*/true, /*rank=*/75});
  RegisterCommand({kCommandSearchHistory, "Search History",
                   "Search through your browsing history", kCategorySearch,
                   /*icon_id=*/"search-history",
                   /*keybinding=*/"Ctrl+H",
                   /*is_enabled=*/true, /*rank=*/70});

  // System commands
  RegisterCommand({kCommandToggleSidebar, "Toggle Sidebar",
                   "Show or hide the Astra sidebar", kCategorySystem,
                   /*icon_id=*/"sidebar-toggle",
                   /*keybinding=*/"Ctrl+B",
                   /*is_enabled=*/true, /*rank=*/85});
  RegisterCommand({kCommandToggleFullscreen, "Toggle Fullscreen",
                   "Enter or exit fullscreen mode", kCategorySystem,
                   /*icon_id=*/"fullscreen",
                   /*keybinding=*/"F11",
                   /*is_enabled=*/true, /*rank=*/75});
  RegisterCommand({kCommandOpenSettings, "Open Settings",
                   "Open browser settings", kCategorySystem,
                   /*icon_id=*/"settings",
                   /*keybinding=*/"Ctrl+,",
                   /*is_enabled=*/true, /*rank=*/70});
  RegisterCommand({kCommandExit, "Exit Browser",
                   "Quit the browser", kCategorySystem,
                   /*icon_id=*/"exit",
                   /*keybinding=*/"Ctrl+Shift+Q",
                   /*is_enabled=*/true, /*rank=*/50});
}

void AstraCommandService::AddToRecentCommands(const std::string& command_id) {
  // Remove if already present (to move it to the front).
  auto it =
      std::find(recent_command_ids_.begin(), recent_command_ids_.end(),
                command_id);
  if (it != recent_command_ids_.end()) {
    recent_command_ids_.erase(it);
  }

  // Add to the front.
  recent_command_ids_.insert(recent_command_ids_.begin(), command_id);

  // Enforce size limit.
  if (recent_command_ids_.size() > max_recent_commands_) {
    recent_command_ids_.pop_back();
  }

  SaveRecentToPrefs();
}

void AstraCommandService::LoadFromPrefs() {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }

  // Load recent commands.
  // TODO(astra): Use PrefService::GetList once we have the full Chromium
  // build with registered prefs.  For the overlay repo, we skip loading.
  // Chromium component: PrefService + PrefRegistry.
  //
  // The pref format for astra.commands.recent is a list of strings
  // (command IDs in most-recent-first order).

  // Load custom keybindings.
  // TODO(astra): Use PrefService::GetDict for the keybindings dictionary.
  // Chromium component: PrefService / base::Value::Dict.
  //
  // The pref format for astra.commands.keybindings is a dictionary
  // mapping command_id strings to keybinding strings.
}

void AstraCommandService::SaveRecentToPrefs() const {
  if (!profile_) {
    return;
  }
  // TODO(astra): Persist recent commands to PrefService.
  // Chromium component: PrefService + PrefRegistry.
  // Patch point: astra_prefs.h + PrefService.
  //
  // Should be called after every change to recent_command_ids_.
  // PrefService handles deferred disk writes internally.
}

void AstraCommandService::SaveKeybindingsToPrefs() const {
  if (!profile_) {
    return;
  }
  // TODO(astra): Persist custom keybindings to PrefService.
  // Chromium component: PrefService + PrefRegistry.
  // Patch point: astra_prefs.h + PrefService.
  //
  // Store as a base::Value::Dict mapping command_id -> keybinding string.
}

// ===========================================================================
// AstraCommandServiceFactory
// ===========================================================================

// static
AstraCommandService* AstraCommandServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraCommandService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraCommandServiceFactory* AstraCommandServiceFactory::GetInstance() {
  static base::NoDestructor<AstraCommandServiceFactory> instance;
  return instance.get();
}

// static
void AstraCommandServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Register recent commands pref: list of command ID strings.
  registry->RegisterListPref(kPrefRecentCommands);

  // Register custom keybindings pref: dictionary of command_id -> keybinding.
  registry->RegisterDictionaryPref(kPrefCustomKeybindings);
}

AstraCommandServiceFactory::AstraCommandServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraCommandService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .WithSystem(ProfileSelection::kNone)
              .Build()) {
  // TODO(astra): Declare dependencies on other ProfileKeyedServices.
  // For example, this service might depend on AstraWorkspaceService if
  // command execution involves workspace operations.
}

AstraCommandServiceFactory::~AstraCommandServiceFactory() = default;

KeyedService*
AstraCommandServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return new AstraCommandService(profile);
}

}  // namespace astra
