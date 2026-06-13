// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/webui/astra_new_tab_handler.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

#include "astra/browser/astra_new_tab_page_service.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

AstraNewTabHandler::AstraNewTabHandler() = default;

AstraNewTabHandler::~AstraNewTabHandler() = default;

void AstraNewTabHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getWorkspaces",
      base::BindRepeating(&AstraNewTabHandler::HandleGetWorkspaces,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "openWorkspace",
      base::BindRepeating(&AstraNewTabHandler::HandleOpenWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "createWorkspace",
      base::BindRepeating(&AstraNewTabHandler::HandleCreateWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getTopSites",
      base::BindRepeating(&AstraNewTabHandler::HandleGetTopSites,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getRecentlyVisited",
      base::BindRepeating(&AstraNewTabHandler::HandleGetRecentlyVisited,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getRecentlyClosed",
      base::BindRepeating(&AstraNewTabHandler::HandleGetRecentlyClosed,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "reopenRecentlyClosed",
      base::BindRepeating(&AstraNewTabHandler::HandleReopenRecentlyClosed,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "deleteWorkspace",
      base::BindRepeating(&AstraNewTabHandler::HandleDeleteWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "renameWorkspace",
      base::BindRepeating(&AstraNewTabHandler::HandleRenameWorkspace,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "addShortcut",
      base::BindRepeating(&AstraNewTabHandler::HandleAddShortcut,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "removeShortcut",
      base::BindRepeating(&AstraNewTabHandler::HandleRemoveShortcut,
                          weak_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "getPageInfo",
      base::BindRepeating(&AstraNewTabHandler::HandleGetPageInfo,
                          weak_factory_.GetWeakPtr()));
}

void AstraNewTabHandler::OnJavascriptDisallowed() {
  // Invalidate all weak pointers to cancel pending callbacks.
  weak_factory_.InvalidateWeakPtrs();
}

void AstraNewTabHandler::HandleGetWorkspaces(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraNewTabPageService* ntp_service =
      AstraNewTabPageServiceFactory::GetForProfile(profile);
  if (!ntp_service) {
    RejectPromise(args, "New tab page service not available");
    return;
  }

  auto summaries = ntp_service->GetWorkspaceSummaries();

  base::Value::List workspaces_list;
  for (const auto& summary : summaries) {
    base::Value::Dict workspace_dict;
    workspace_dict.Set("id", summary.id);
    workspace_dict.Set("name", summary.name);
    workspace_dict.Set("accentColor", summary.accent_color);
    workspace_dict.Set("tabCount", summary.tab_count);
    workspace_dict.Set("isActive", summary.is_active);
    workspaces_list.Append(std::move(workspace_dict));
  }

  ResolvePromise(args, base::Value(std::move(workspaces_list)));
}

void AstraNewTabHandler::HandleOpenWorkspace(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing workspaceId argument");
    return;
  }

  // args[0] is the callback id, args[1] is the workspace id.
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

  const AstraWorkspace* workspace =
      workspace_service->GetWorkspace(*workspace_id);
  if (!workspace) {
    RejectPromise(args, "Workspace not found: " + *workspace_id);
    return;
  }

  // Switch the active workspace.
  // TODO(astra): Also navigate to the workspace's window or create a new
  // window for the workspace.  Currently we just change the active
  // workspace in the service — the UI updates follow from observers.
  // For the NTP use case, "open workspace" probably means switch the
  // current window to that workspace's tab set, or open the workspace
  // in a new window.  Needs UX definition.
  // Chromium owner: BrowserList + TabStripModel (for window switching).
  workspace_service->ActivateWorkspace(*workspace_id);

  ResolvePromise(args, base::Value(true));
}

void AstraNewTabHandler::HandleCreateWorkspace(
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

  // Create the workspace.
  // TODO(astra): Generate a proper id (e.g. base::Uuid::GenerateRandomV4()).
  // For now we use the name as a simple id placeholder.
  AstraWorkspace new_workspace;
  new_workspace.name = *name;
  new_workspace.accent_color = accent_color.empty() ? "#6366f1" : accent_color;
  // id will be set by the service if not provided.

  workspace_service->AddWorkspace(std::move(new_workspace));

  // Get the newly created workspace to return its data.
  // Since AddWorkspace appends at the end, we look it up by name.
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
  // Use Time.ToJsTime() or similar for creation time.
  // result.Set("createdTime", ...);

  ResolvePromise(args, base::Value(std::move(result)));
}

void AstraNewTabHandler::HandleGetTopSites(
    const base::Value::List& args) {
  AllowJavascript();

  size_t count = 8;  // default
  if (args.size() >= 2 && args[1].is_int()) {
    count = static_cast<size_t>(args[1].GetInt());
    if (count == 0 || count > 20) {
      count = 8;  // clamp
    }
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraNewTabPageService* ntp_service =
      AstraNewTabPageServiceFactory::GetForProfile(profile);
  if (!ntp_service) {
    RejectPromise(args, "New tab page service not available");
    return;
  }

  auto shortcuts = ntp_service->GetTopSites(count);

  base::Value::List shortcuts_list;
  for (const auto& shortcut : shortcuts) {
    base::Value::Dict shortcut_dict;
    shortcut_dict.Set("title", base::UTF16ToUTF8(shortcut.title));
    shortcut_dict.Set("url", shortcut.url.spec());
    shortcut_dict.Set("faviconUrl", shortcut.favicon_url.spec());
    shortcut_dict.Set("isMostVisited", shortcut.is_most_visited);
    shortcuts_list.Append(std::move(shortcut_dict));
  }

  ResolvePromise(args, base::Value(std::move(shortcuts_list)));
}

void AstraNewTabHandler::HandleGetRecentlyVisited(
    const base::Value::List& args) {
  AllowJavascript();

  size_t count = 5;  // default
  if (args.size() >= 2 && args[1].is_int()) {
    count = static_cast<size_t>(args[1].GetInt());
    if (count == 0 || count > 20) {
      count = 5;  // clamp
    }
  }

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  AstraNewTabPageService* ntp_service =
      AstraNewTabPageServiceFactory::GetForProfile(profile);
  if (!ntp_service) {
    RejectPromise(args, "New tab page service not available");
    return;
  }

  auto visits = ntp_service->GetRecentlyVisited(count);

  base::Value::List visits_list;
  for (const auto& visit : visits) {
    base::Value::Dict visit_dict;
    visit_dict.Set("title", base::UTF16ToUTF8(visit.title));
    visit_dict.Set("url", visit.url.spec());
    visit_dict.Set("visitTime",
                   base::checked_cast<double>(visit.visit_time.ToJsTime()));
    visits_list.Append(std::move(visit_dict));
  }

  ResolvePromise(args, base::Value(std::move(visits_list)));
}

// =========================================================================
// Recently closed handlers
// =========================================================================

void AstraNewTabHandler::HandleGetRecentlyClosed(
    const base::Value::List& args) {
  AllowJavascript();

  size_t count = 8;  // default
  if (args.size() >= 2 && args[1].is_int()) {
    count = static_cast<size_t>(args[1].GetInt());
    if (count == 0 || count > 25) {
      count = 8;  // clamp
    }
  }

  // TODO(astra): Read recently closed from Chromium's TabRestoreService.
  // Chromium subsystem: sessions::TabRestoreService
  // Chromium owner: chrome/browser/sessions/tab_restore_service.h
  // The TabRestoreService tracks recently closed tabs and windows and
  // provides methods to query and restore them.
  //
  // For now we return an empty list as a placeholder.
  // The JS side handles empty state gracefully.
  base::Value::List empty_list;
  ResolvePromise(args, base::Value(std::move(empty_list)));
}

void AstraNewTabHandler::HandleReopenRecentlyClosed(
    const base::Value::List& args) {
  AllowJavascript();

  // Optional item id — if empty, reopen the most recent.
  std::string item_id;
  if (args.size() >= 2 && args[2].is_string()) {
    item_id = *args[2].GetIfString();
  }

  // TODO(astra): Integrate with TabRestoreService::RestoreMostRecentEntry()
  // or RestoreEntryById().
  // Chromium owner: chrome/browser/sessions/tab_restore_service.cc
  //
  // This would need to:
  //   1. Get the TabRestoreService for the profile
  //   2. Restore the entry (creates a new tab/window)
  //   3. Return success status
  //
  // For now we return false as a placeholder.
  ResolvePromise(args, base::Value(false));
}

// =========================================================================
// Workspace CRUD (enhanced for NTP)
// =========================================================================

void AstraNewTabHandler::HandleDeleteWorkspace(
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
    RejectPromise(args, "Workspace not found: " + *workspace_id);
    return;
  }

  // Prevent deleting the default workspace.
  if (ws->is_default) {
    RejectPromise(args, "Cannot delete the default workspace");
    return;
  }

  // TODO(astra): Add a proper RemoveWorkspace() method to AstraWorkspaceService.
  // The service should handle:
  //   - Removing the workspace from its list
  //   - Migrating tabs if necessary
  //   - Notifying observers
  // For now we just verify and return success as a placeholder.
  ResolvePromise(args, base::Value(true));
}

void AstraNewTabHandler::HandleRenameWorkspace(
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

  // Verify the workspace exists.
  const AstraWorkspace* ws = workspace_service->GetWorkspace(*workspace_id);
  if (!ws) {
    RejectPromise(args, "Workspace not found");
    return;
  }

  // TODO(astra): Add RenameWorkspace() to AstraWorkspaceService.
  // For now we return success as a placeholder.
  ResolvePromise(args, base::Value(true));
}

// =========================================================================
// Shortcut CRUD
// =========================================================================

void AstraNewTabHandler::HandleAddShortcut(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing url argument");
    return;
  }

  const std::string* url_str = args[1].GetIfString();
  if (!url_str || url_str->empty()) {
    RejectPromise(args, "Invalid URL");
    return;
  }

  std::string title;
  if (args.size() >= 3 && args[2].is_string()) {
    title = *args[2].GetIfString();
  }

  // TODO(astra): Integrate with Chromium's shortcut system.
  // Chromium has two main shortcut systems:
  //   - TopSites (most-visited sites, from history)
  //   - Custom shortcuts (user-added, stored in pref service)
  // For custom shortcuts, see:
  //   chrome/browser/ui/webui/ntp/ntp_custom_background_handler.cc
  //   components/ntp_tiles/
  //
  // For now we return a constructed shortcut as a placeholder.
  base::Value::Dict result;
  result.Set("title", title.empty() ? *url_str : title);
  result.Set("url", *url_str);
  result.Set("faviconUrl", "");
  result.Set("isMostVisited", false);
  result.Set("isCustom", true);

  ResolvePromise(args, base::Value(std::move(result)));
}

void AstraNewTabHandler::HandleRemoveShortcut(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2) {
    RejectPromise(args, "Missing url argument");
    return;
  }

  const std::string* url_str = args[1].GetIfString();
  if (!url_str || url_str->empty()) {
    RejectPromise(args, "Invalid URL");
    return;
  }

  // TODO(astra): Integrate with Chromium's shortcut system.
  // For TopSites, blocking a site is done via:
  //   history::TopSites::AddBlacklistedURL()
  // For custom shortcuts, remove from the pref list.
  //
  // For now we return success as a placeholder.
  ResolvePromise(args, base::Value(true));
}

// =========================================================================
// Page info
// =========================================================================

void AstraNewTabHandler::HandleGetPageInfo(
    const base::Value::List& args) {
  AllowJavascript();

  Profile* profile = GetProfile();
  if (!profile) {
    RejectPromise(args, "No profile available");
    return;
  }

  // TODO(astra): Add more page info as NTP features grow.
  // Possible additions:
  //   - User profile name / avatar
  //   - Theme / accent color
  //   - Feature flags
  //   - Number of open tabs across all windows
  //
  // For now we return basic info.
  base::Value::Dict info;
  info.Set("greeting", "");  // Greeting is computed client-side by time of day.
  info.Set("hasProfile", profile != nullptr);
  info.Set("isOffTheRecord", profile->IsOffTheRecord());

  // TODO(astra): Get the profile name from ProfileAttributesStorage.
  // Chromium owner: chrome/browser/profiles/profile_attributes_storage.h
  info.Set("profileName", "");

  ResolvePromise(args, base::Value(std::move(info)));
}

void AstraNewTabHandler::ResolvePromise(const base::Value::List& args,
                                        base::Value result) {
  if (args.empty() || !args[0].is_string()) {
    LOG(WARNING) << "ResolvePromise called without valid callback id";
    return;
  }

  const std::string& callback_id = args[0].GetString();
  // The JS side uses chrome.send with a callback id, and we resolve it
  // by calling the global resolve function.
  // TODO(astra): Use a proper promise-based message passing pattern.
  // Chromium's WebUI uses various patterns:
  //   - cr.sendWithPromise() / cr.webUIResponseCallback
  //   - Direct chrome.send() with callback registration
  // We use the simple pattern: call a global "resolveAstraPromise" function.
  // Replace with cr.sendWithPromise() once we integrate cr.js.
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("resolveAstraPromise",
                         base::Value(callback_id),
                         base::Value(true),  // success = true
                         std::move(result));
}

void AstraNewTabHandler::RejectPromise(const base::Value::List& args,
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

Profile* AstraNewTabHandler::GetProfile() const {
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
