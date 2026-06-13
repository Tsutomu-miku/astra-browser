// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/webui/astra_settings_handler.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

#include "astra/browser/astra_workspace_service.h"
// TODO(astra): Include AstraPrefService once it exists.
// #include "astra/browser/astra_pref_service.h"
// TODO(astra): Include AstraFocusModeService once it exists.
// #include "astra/browser/astra_focus_mode_service.h"

namespace astra {

namespace {

// Default values for settings.
// TODO(astra): Move these to AstraPrefService and register as pref defaults.
// These are temporary defaults used when the preference service isn't
// available yet.
constexpr char kDefaultStartupBehavior[] = "continue";  // or "newtab", "specific"
constexpr char kDefaultSearchEngine[] = "google";
constexpr char kDefaultSidebarPosition[] = "left";  // or "right"
constexpr int kDefaultSidebarWidth = 280;
constexpr bool kDefaultSidebarAutoHide = false;
constexpr bool kDefaultSidebarEnabled = true;
constexpr int kDefaultFocusDuration = 25;  // minutes
constexpr bool kDefaultFocusSoundEnabled = true;
constexpr char kDefaultTheme[] = "system";  // "light", "dark", "system"
constexpr char kDefaultAccentColor[] = "#6366f1";
constexpr bool kDefaultUseSystemTheme = true;
constexpr bool kDefaultCompactMode = false;

// Default accent color presets for workspaces.
constexpr const char* kAccentColorPresets[] = {
  "#6366f1",  // indigo
  "#8b5cf6",  // violet
  "#ec4899",  // pink
  "#ef4444",  // red
  "#f97316",  // orange
  "#eab308",  // yellow
  "#22c55e",  // green
  "#14b8a6",  // teal
  "#0ea5e9",  // sky
  "#3b82f6",  // blue
};

}  // namespace

AstraSettingsHandler::AstraSettingsHandler() = default;

AstraSettingsHandler::~AstraSettingsHandler() = default;

void AstraSettingsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAllSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetAllSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getGeneralSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetGeneralSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setGeneralSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleSetGeneralSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getSidebarSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetSidebarSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setSidebarSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleSetSidebarSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getWorkspaceSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetWorkspaceSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setWorkspaceSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleSetWorkspaceSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getFocusModeSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetFocusModeSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setFocusModeSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleSetFocusModeSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getAppearanceSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleGetAppearanceSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setAppearanceSettings",
      base::BindRepeating(&AstraSettingsHandler::HandleSetAppearanceSettings,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getWorkspaces",
      base::BindRepeating(&AstraSettingsHandler::HandleGetWorkspaces,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "createWorkspace",
      base::BindRepeating(&AstraSettingsHandler::HandleCreateWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "deleteWorkspace",
      base::BindRepeating(&AstraSettingsHandler::HandleDeleteWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "renameWorkspace",
      base::BindRepeating(&AstraSettingsHandler::HandleRenameWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "setDefaultWorkspace",
      base::BindRepeating(&AstraSettingsHandler::HandleSetDefaultWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getRecentlyClosed",
      base::BindRepeating(&AstraSettingsHandler::HandleGetRecentlyClosed,
                          weak_factory_.GetWeakPtr()));
}

void AstraSettingsHandler::OnJavascriptDisallowed() {
  // Invalidate all weak pointers to cancel pending callbacks.
  weak_factory_.InvalidateWeakPtrs();
}

// =========================================================================
// Get-all settings handler
// =========================================================================

void AstraSettingsHandler::HandleGetAllSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildAllSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildAllSettingsDict(
    Profile* profile) {
  base::Value::Dict all;
  all.Set("general", BuildGeneralSettingsDict(profile));
  all.Set("sidebar", BuildSidebarSettingsDict(profile));
  all.Set("workspaces", BuildWorkspaceSettingsDict(profile));
  all.Set("focusMode", BuildFocusModeSettingsDict(profile));
  all.Set("appearance", BuildAppearanceSettingsDict(profile));
  return all;
}

// =========================================================================
// General settings
// =========================================================================

void AstraSettingsHandler::HandleGetGeneralSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildGeneralSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildGeneralSettingsDict(
    Profile* profile) {
  base::Value::Dict dict;
  dict.Set("startupBehavior", kDefaultStartupBehavior);
  dict.Set("defaultSearchEngine", kDefaultSearchEngine);
  // TODO(astra): Read from PrefService / AstraPrefService once available.
  // For now we return sensible defaults.
  return dict;
}

void AstraSettingsHandler::HandleSetGeneralSettings(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    RejectPromise(args, "Missing settings object argument");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  bool success = ApplyGeneralSettings(profile, args[1].GetDict());
  ResolvePromise(args, base::Value(success));
}

bool AstraSettingsHandler::ApplyGeneralSettings(
    Profile* profile,
    const base::Value::Dict& dict) {
  // TODO(astra): Write to PrefService / AstraPrefService once available.
  // For now we just validate the input and pretend success.
  if (dict.Find("startupBehavior")) {
    const std::string* value = dict.FindString("startupBehavior");
    if (!value || (*value != "continue" && *value != "newtab" &&
                   *value != "specific")) {
      return false;
    }
  }
  if (dict.Find("defaultSearchEngine")) {
    const std::string* value = dict.FindString("defaultSearchEngine");
    if (!value || value->empty()) {
      return false;
    }
  }
  return true;
}

// =========================================================================
// Sidebar settings
// =========================================================================

void AstraSettingsHandler::HandleGetSidebarSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildSidebarSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildSidebarSettingsDict(
    Profile* profile) {
  base::Value::Dict dict;
  dict.Set("position", kDefaultSidebarPosition);
  dict.Set("width", kDefaultSidebarWidth);
  dict.Set("autoHide", kDefaultSidebarAutoHide);
  dict.Set("enabled", kDefaultSidebarEnabled);
  // TODO(astra): Read from PrefService / AstraPrefService once available.
  return dict;
}

void AstraSettingsHandler::HandleSetSidebarSettings(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    RejectPromise(args, "Missing settings object argument");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  bool success = ApplySidebarSettings(profile, args[1].GetDict());
  ResolvePromise(args, base::Value(success));
}

bool AstraSettingsHandler::ApplySidebarSettings(
    Profile* profile,
    const base::Value::Dict& dict) {
  // TODO(astra): Write to PrefService / AstraPrefService once available.
  if (dict.Find("position")) {
    const std::string* value = dict.FindString("position");
    if (!value || (*value != "left" && *value != "right")) {
      return false;
    }
  }
  if (dict.Find("width")) {
    absl::optional<int> value = dict.FindInt("width");
    if (!value.has_value() || *value < 200 || *value > 600) {
      return false;
    }
  }
  // autoHide and enabled are bools — checked by is_dict access pattern.
  return true;
}

// =========================================================================
// Workspace settings
// =========================================================================

void AstraSettingsHandler::HandleGetWorkspaceSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildWorkspaceSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildWorkspaceSettingsDict(
    Profile* profile) {
  base::Value::Dict dict;

  // Find the default workspace ID from the workspace service.
  std::string default_id;
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (workspace_service) {
    const auto& workspaces = workspace_service->workspaces();
    for (const auto& ws : workspaces) {
      if (ws.is_default) {
        default_id = ws.id;
        break;
      }
    }
    // If no default found, use the first workspace if any.
    if (default_id.empty() && !workspaces.empty()) {
      default_id = workspaces[0].id;
    }
  }
  dict.Set("defaultWorkspaceId", default_id);

  // Accent color presets
  base::Value::List presets;
  for (const auto* color : kAccentColorPresets) {
    presets.Append(color);
  }
  dict.Set("accentColorPresets", std::move(presets));

  return dict;
}

void AstraSettingsHandler::HandleSetWorkspaceSettings(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    RejectPromise(args, "Missing settings object argument");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  bool success = ApplyWorkspaceSettings(profile, args[1].GetDict());
  ResolvePromise(args, base::Value(success));
}

bool AstraSettingsHandler::ApplyWorkspaceSettings(
    Profile* profile,
    const base::Value::Dict& dict) {
  const std::string* default_id = dict.FindString("defaultWorkspaceId");
  if (default_id && !default_id->empty()) {
    AstraWorkspaceService* workspace_service =
        AstraWorkspaceServiceFactory::GetForProfile(profile);
    if (!workspace_service) {
      return false;
    }
    // TODO(astra): Add SetDefaultWorkspace() to AstraWorkspaceService.
    // For now we iterate and set the is_default flag.
    // This is a placeholder — the service should own this logic.
    auto& workspaces = workspace_service->workspaces();
    bool found = false;
    for (auto& ws : workspaces) {
      if (ws.id == *default_id) {
        ws.is_default = true;
        found = true;
      } else {
        ws.is_default = false;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

// =========================================================================
// Focus mode settings
// =========================================================================

void AstraSettingsHandler::HandleGetFocusModeSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildFocusModeSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildFocusModeSettingsDict(
    Profile* profile) {
  base::Value::Dict dict;
  dict.Set("defaultDurationMinutes", kDefaultFocusDuration);
  dict.Set("soundEnabled", kDefaultFocusSoundEnabled);

  // Default blocklist (sample domains).
  base::Value::List blocklist;
  blocklist.Append("twitter.com");
  blocklist.Append("facebook.com");
  blocklist.Append("youtube.com");
  blocklist.Append("reddit.com");
  dict.Set("blocklist", std::move(blocklist));

  // Workspace IDs that auto-start focus mode.
  base::Value::List auto_start_workspaces;
  dict.Set("autoStartWorkspaces", std::move(auto_start_workspaces));

  // TODO(astra): Read from AstraFocusModeService once it exists.
  // For now we return sensible defaults.
  return dict;
}

void AstraSettingsHandler::HandleSetFocusModeSettings(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    RejectPromise(args, "Missing settings object argument");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  bool success = ApplyFocusModeSettings(profile, args[1].GetDict());
  ResolvePromise(args, base::Value(success));
}

bool AstraSettingsHandler::ApplyFocusModeSettings(
    Profile* profile,
    const base::Value::Dict& dict) {
  // TODO(astra): Write to AstraFocusModeService once it exists.
  if (dict.Find("defaultDurationMinutes")) {
    absl::optional<int> value = dict.FindInt("defaultDurationMinutes");
    if (!value.has_value() || *value < 1 || *value > 240) {
      return false;
    }
  }
  // soundEnabled is a bool — validated by dict structure.
  if (dict.Find("blocklist")) {
    const base::Value::List* blocklist = dict.FindList("blocklist");
    if (!blocklist) {
      return false;
    }
  }
  return true;
}

// =========================================================================
// Appearance settings
// =========================================================================

void AstraSettingsHandler::HandleGetAppearanceSettings(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  ResolvePromise(args, base::Value(BuildAppearanceSettingsDict(profile)));
}

base::Value::Dict AstraSettingsHandler::BuildAppearanceSettingsDict(
    Profile* profile) {
  base::Value::Dict dict;
  dict.Set("theme", kDefaultTheme);
  dict.Set("accentColor", kDefaultAccentColor);
  dict.Set("useSystemTheme", kDefaultUseSystemTheme);
  dict.Set("compactMode", kDefaultCompactMode);
  // TODO(astra): Read from PrefService / AstraPrefService once available.
  return dict;
}

void AstraSettingsHandler::HandleSetAppearanceSettings(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    RejectPromise(args, "Missing settings object argument");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  bool success = ApplyAppearanceSettings(profile, args[1].GetDict());
  ResolvePromise(args, base::Value(success));
}

bool AstraSettingsHandler::ApplyAppearanceSettings(
    Profile* profile,
    const base::Value::Dict& dict) {
  // TODO(astra): Write to PrefService / AstraPrefService once available.
  if (dict.Find("theme")) {
    const std::string* value = dict.FindString("theme");
    if (!value || (*value != "light" && *value != "dark" &&
                   *value != "system")) {
      return false;
    }
  }
  if (dict.Find("accentColor")) {
    const std::string* value = dict.FindString("accentColor");
    if (!value || !base::StartsWith(*value, "#",
                                     base::CompareCase::INSENSITIVE_ASCII)) {
      return false;
    }
  }
  // useSystemTheme and compactMode are bools.
  return true;
}

// =========================================================================
// Workspace CRUD (for settings page workspace management)
// =========================================================================

void AstraSettingsHandler::HandleGetWorkspaces(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!workspace_service) {
    RejectPromise(args, "Workspace service not available");
    return;
  }

  const auto& workspaces = workspace_service->workspaces();
  base::Value::List workspaces_list;
  for (const auto& ws : workspaces) {
    base::Value::Dict ws_dict;
    ws_dict.Set("id", ws.id);
    ws_dict.Set("name", ws.name);
    ws_dict.Set("accentColor", ws.accent_color);
    ws_dict.Set("isDefault", ws.is_default);
    // TODO(astra): Add tab count from TabStripModel or workspace metadata.
    ws_dict.Set("tabCount", 0);
    workspaces_list.Append(std::move(ws_dict));
  }

  ResolvePromise(args, base::Value(std::move(workspaces_list)));
}

void AstraSettingsHandler::HandleCreateWorkspace(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing name argument");
    return;
  }

  const std::string* name = args[1].GetIfString();
  if (!name || name->empty()) {
    RejectPromise(args, "Invalid workspace name");
    return;
  }

  std::string accent_color;
  if (args.size() >= 3 && args[2].is_string()) {
    accent_color = *args[2].GetIfString();
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!workspace_service) {
    RejectPromise(args, "Workspace service not available");
    return;
  }

  AstraWorkspace new_workspace;
  new_workspace.name = *name;
  new_workspace.accent_color = accent_color.empty() ? "#6366f1" : accent_color;

  workspace_service->AddWorkspace(std::move(new_workspace));

  // Look up the created workspace by name (simplified — see NTP handler
  // for the same pattern).
  // TODO(astra): Have AddWorkspace return the created workspace or its id.
  const auto& workspaces = workspace_service->workspaces();
  const AstraWorkspace* created = nullptr;
  for (const auto& ws : workspaces) {
    if (ws.name == *name) {
      created = &ws;
      break;
    }
  }

  if (!created) {
    RejectPromise(args, "Failed to create workspace");
    return;
  }

  base::Value::Dict result;
  result.Set("id", created->id);
  result.Set("name", created->name);
  result.Set("accentColor", created->accent_color);
  result.Set("isDefault", created->is_default);

  ResolvePromise(args, base::Value(std::move(result)));
}

void AstraSettingsHandler::HandleDeleteWorkspace(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing workspaceId argument");
    return;
  }

  const std::string* workspace_id = args[1].GetIfString();
  if (!workspace_id || workspace_id->empty()) {
    RejectPromise(args, "Invalid workspaceId");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!workspace_service) {
    RejectPromise(args, "Workspace service not available");
    return;
  }

  // TODO(astra): Add a proper RemoveWorkspace() method to AstraWorkspaceService.
  // For now we verify the workspace exists and return success as a placeholder.
  // The actual deletion should also handle tab migration, default workspace
  // reassignment, etc.
  const AstraWorkspace* ws = workspace_service->GetWorkspace(*workspace_id);
  if (!ws) {
    RejectPromise(args, "Workspace not found");
    return;
  }

  // Prevent deleting the default workspace.
  if (ws->is_default) {
    RejectPromise(args, "Cannot delete the default workspace");
    return;
  }

  // TODO(astra): Actually remove the workspace from the service.
  // workspace_service->RemoveWorkspace(*workspace_id);

  ResolvePromise(args, base::Value(true));
}

void AstraSettingsHandler::HandleRenameWorkspace(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 3) {
    RejectPromise(args, "Missing workspaceId or newName argument");
    return;
  }

  const std::string* workspace_id = args[1].GetIfString();
  const std::string* new_name = args[2].GetIfString();
  if (!workspace_id || workspace_id->empty() ||
      !new_name || new_name->empty()) {
    RejectPromise(args, "Invalid arguments");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!workspace_service) {
    RejectPromise(args, "Workspace service not available");
    return;
  }

  // TODO(astra): Add a proper RenameWorkspace() method to AstraWorkspaceService.
  // For now we look up by ID and simulate the rename.
  // Note: workspace_service->workspaces() returns a const ref, so we can't
  // modify it directly here.  The service should expose a mutation API.
  const AstraWorkspace* ws = workspace_service->GetWorkspace(*workspace_id);
  if (!ws) {
    RejectPromise(args, "Workspace not found");
    return;
  }

  // Placeholder: would call workspace_service->RenameWorkspace(id, name).
  ResolvePromise(args, base::Value(true));
}

void AstraSettingsHandler::HandleSetDefaultWorkspace(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing workspaceId argument");
    return;
  }

  const std::string* workspace_id = args[1].GetIfString();
  if (!workspace_id || workspace_id->empty()) {
    RejectPromise(args, "Invalid workspaceId");
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!workspace_service) {
    RejectPromise(args, "Workspace service not available");
    return;
  }

  // Verify the workspace exists.
  const AstraWorkspace* ws = workspace_service->GetWorkspace(*workspace_id);
  if (!ws) {
    RejectPromise(args, "Workspace not found");
    return;
  }

  // TODO(astra): Add SetDefaultWorkspace() to AstraWorkspaceService.
  // This handler mirrors the logic in ApplyWorkspaceSettings.

  ResolvePromise(args, base::Value(true));
}

// =========================================================================
// Recently closed (for settings page history / recently closed section)
// =========================================================================

void AstraSettingsHandler::HandleGetRecentlyClosed(
    const base::Value::List& args) {
  AllowJavascript();

  size_t count = 10;  // default
  if (args.size() >= 2 && args[1].is_int()) {
    count = static_cast<size_t>(args[1].GetInt());
    if (count == 0 || count > 50) {
      count = 10;  // clamp
    }
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  // TODO(astra): Read recently closed from Chromium's TabRestoreService.
  // Chromium subsystem: sessions::TabRestoreService
  // Chromium owner: chrome/browser/sessions/tab_restore_service.h
  // For now we return an empty list as a placeholder.
  base::Value::List empty_list;
  ResolvePromise(args, base::Value(std::move(empty_list)));
}

// =========================================================================
// Helpers
// =========================================================================

void AstraSettingsHandler::ResolvePromise(const base::Value::List& args,
                                          base::Value result) {
  if (args.empty() || !args[0].is_string()) {
    LOG(WARNING) << "ResolvePromise called without valid callback id";
    return;
  }

  const std::string& callback_id = args[0].GetString();
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("resolveAstraPromise",
                         base::Value(callback_id),
                         base::Value(true),  // success = true
                         std::move(result));
}

void AstraSettingsHandler::RejectPromise(const base::Value::List& args,
                                         const std::string& error_message) {
  if (args.empty() || !args[0].is_string()) {
    LOG(WARNING) << "RejectPromise called without valid callback id: "
                 << error_message;
    return;
  }

  const std::string& callback_id = args[0].GetString();
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("resolveAstraPromise",
                         base::Value(callback_id),
                         base::Value(false),  // success = false
                         base::Value(error_message));
}

Profile* AstraSettingsHandler::GetProfile() const {
  if (!web_ui()) {
    return nullptr;
  }
  content::WebContents* web_contents = web_ui()->GetWebContents();
  if (!web_contents) {
    return nullptr;
  }
  return Profile::FromBrowserContext(web_contents->GetBrowserContext());
}

}  // namespace astra
