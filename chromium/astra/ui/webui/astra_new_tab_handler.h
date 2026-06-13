// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Message handler for the Astra new tab page.
//
// Bridges JavaScript on the astra://newtab page to browser-side services.
// The NTP JS sends messages like "getWorkspaces", "openWorkspace", etc.,
// and this handler dispatches them to the appropriate Astra services.
//
// Chromium pattern: content::WebUIMessageHandler
//   - RegisterMessageCallback("messageName", base::BindRepeating(...))
//   - AllowJavascript() / CallJavascriptFunction() for JS-side responses
//   - OnJavascriptDisallowed() for cleanup
//
// Chromium subsystems reused:
//   - content::WebUIMessageHandler — base class
//   - AstraWorkspaceService — workspace metadata
//   - AstraNewTabPageService — NTP data aggregation
//
// Truth model:
//   - This handler is a pure bridge — it stores no state.
//   - All data comes from AstraWorkspaceService / AstraNewTabPageService.
//   - UI state is owned by the JS side of the NTP page.
//
// TODO(astra): Add more message handlers as NTP features grow
// (favorites, notes, shortcuts, custom background, etc.).
// Chromium owner: chrome/browser/ui/webui/new_tab_page/new_tab_page_handler.cc
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_NEW_TAB_HANDLER_H_
#define ASTRA_UI_WEBUI_ASTRA_NEW_TAB_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class Value;
class Value::List;
}  // namespace base

namespace astra {

// WebUIMessageHandler for the Astra new tab page.
//
// Registered messages (JS -> C++):
//   - "getWorkspaces" — returns list of workspace summaries
//   - "openWorkspace" — opens a workspace by id
//   - "createWorkspace" — creates a new workspace
//   - "deleteWorkspace" — deletes a workspace by id
//   - "renameWorkspace" — renames a workspace
//   - "getTopSites" — returns most-visited site shortcuts
//   - "addShortcut" — adds a custom shortcut
//   - "removeShortcut" — removes a shortcut by URL
//   - "getRecentlyVisited" — returns recently visited pages
//   - "getRecentlyClosed" — returns recently closed tabs/windows
//   - "reopenRecentlyClosed" — reopens a recently closed item
//   - "getPageInfo" — returns general page info (greeting, etc.)
//
// TODO(astra): Add observer-based push updates when workspace data changes.
// Currently the JS side polls on load; we should subscribe to
// AstraWorkspaceServiceObserver and CallJavascriptFunction() on changes.
class AstraNewTabHandler : public content::WebUIMessageHandler {
 public:
  AstraNewTabHandler();
  ~AstraNewTabHandler() override;

  AstraNewTabHandler(const AstraNewTabHandler&) = delete;
  AstraNewTabHandler& operator=(const AstraNewTabHandler&) = delete;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;
  void OnJavascriptDisallowed() override;

 private:
  // -- Message handlers ---------------------------------------------------

  // Handles "getWorkspaces" message.
  // Args: none
  // Returns: array of workspace objects {id, name, accentColor, tabCount, isActive}
  void HandleGetWorkspaces(const base::Value::List& args);

  // Handles "openWorkspace" message.
  // Args: [workspaceId: string]
  // Returns: boolean success
  void HandleOpenWorkspace(const base::Value::List& args);

  // Handles "createWorkspace" message.
  // Args: [name: string, accentColor: string (optional)]
  // Returns: workspace object {id, name, accentColor, ...}
  void HandleCreateWorkspace(const base::Value::List& args);

  // Handles "getTopSites" message.
  // Args: [count: number (optional, default 8)]
  // Returns: array of shortcut objects {title, url, faviconUrl, isMostVisited}
  void HandleGetTopSites(const base::Value::List& args);

  // Handles "getRecentlyVisited" message.
  // Args: [count: number (optional, default 5)]
  // Returns: array of visit objects {title, url, visitTime}
  void HandleGetRecentlyVisited(const base::Value::List& args);

  // Handles "getRecentlyClosed" message.
  // Args: [count: number (optional, default 8)]
  // Returns: array of recently closed items {id, title, url, closedTime, type}
  void HandleGetRecentlyClosed(const base::Value::List& args);

  // Handles "reopenRecentlyClosed" message.
  // Args: [itemId: string] — or empty for the most recent
  // Returns: boolean success
  void HandleReopenRecentlyClosed(const base::Value::List& args);

  // Handles "deleteWorkspace" message.
  // Args: [workspaceId: string]
  // Returns: boolean success
  void HandleDeleteWorkspace(const base::Value::List& args);

  // Handles "renameWorkspace" message.
  // Args: [workspaceId: string, newName: string]
  // Returns: boolean success
  void HandleRenameWorkspace(const base::Value::List& args);

  // Handles "addShortcut" message.
  // Args: [url: string, title: string (optional)]
  // Returns: shortcut object {title, url, faviconUrl}
  void HandleAddShortcut(const base::Value::List& args);

  // Handles "removeShortcut" message.
  // Args: [url: string]
  // Returns: boolean success
  void HandleRemoveShortcut(const base::Value::List& args);

  // Handles "getPageInfo" message.
  // Args: none
  // Returns: {greeting, userName, ...}
  void HandleGetPageInfo(const base::Value::List& args);

  // -- Helpers ------------------------------------------------------------

  // Helper to resolve a Promise on the JS side with a successful result.
  // The first element of |args| is the callback id (string).
  void ResolvePromise(const base::Value::List& args,
                      base::Value result);

  // Helper to reject a Promise on the JS side with an error message.
  void RejectPromise(const base::Value::List& args,
                     const std::string& error_message);

  // Gets the profile from the associated WebUI's WebContents.
  // Returns null if the WebUI is not attached to a profile.
  // TODO(astra): Consider caching the profile pointer.
  // It's stable for the lifetime of the WebUI, but we need to be careful
  // about profile destruction ordering.
  Profile* GetProfile() const;

  base::WeakPtrFactory<AstraNewTabHandler> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_NEW_TAB_HANDLER_H_
