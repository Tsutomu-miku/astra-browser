// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_command_delegate.h"

#include <algorithm>

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_devtools_helper.h"
#include "astra/browser/astra_favorite_service.h"
#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_incognito_handler.h"
#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_recent_tabs_helper.h"
#include "astra/browser/astra_screenshot_service.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/browser/astra_workspace_window_manager.h"
#include "astra/browser/astra_window_features.h"

namespace astra {

namespace {

// =========================================================================
// Command metadata table
// =========================================================================
//
// Static metadata for every Astra command: name, description, category.
// This table is the single source of truth for command metadata.
// When adding a new command ID to AstraCommandId, add a corresponding entry
// here.
//
// The table is ordered by command ID for readability, but we use a flat_map
// for O(log n) lookup.

struct CommandMetadataEntry {
  const char* name;
  const char* description;
  AstraCommandCategory category;
};

const base::flat_map<int, CommandMetadataEntry>& GetCommandMetadataMap() {
  static const base::NoDestructor<base::flat_map<int, CommandMetadataEntry>>
      map([]() {
        base::flat_map<int, CommandMetadataEntry> m;

        // -- Sidebar (View category) ---------------------------------------
        m[kAstraCommandToggleSidebar] = {
            "Toggle Sidebar",
            "Show or hide the Astra sidebar",
            AstraCommandCategory::kView,
        };
        m[kAstraCommandToggleSidebarPin] = {
            "Toggle Sidebar Pin",
            "Pin the sidebar open or let it auto-hide",
            AstraCommandCategory::kView,
        };

        // -- Workspaces (Workspace category) -------------------------------
        m[kAstraCommandNewWorkspace] = {
            "New Workspace",
            "Create a new workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandNextWorkspace] = {
            "Next Workspace",
            "Switch to the next workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandPreviousWorkspace] = {
            "Previous Workspace",
            "Switch to the previous workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandRenameWorkspace] = {
            "Rename Workspace",
            "Rename the current workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandDeleteWorkspace] = {
            "Delete Workspace",
            "Delete the current workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandMoveTabToNextWorkspace] = {
            "Move Tab to Next Workspace",
            "Move the active tab to the next workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandMoveTabToPreviousWorkspace] = {
            "Move Tab to Previous Workspace",
            "Move the active tab to the previous workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandShowAllWorkspaces] = {
            "Show All Workspaces",
            "Open the workspace overview",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandNewWindowInWorkspace] = {
            "New Window in Workspace",
            "Open a new browser window in the current workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandMoveWindowToNextWorkspace] = {
            "Move Window to Next Workspace",
            "Move the current window to the next workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandMoveWindowToPreviousWorkspace] = {
            "Move Window to Previous Workspace",
            "Move the current window to the previous workspace",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandSwitchWorkspaceMenu] = {
            "Switch Workspace Menu",
            "Open the workspace switcher menu",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandExportWorkspaces] = {
            "Export Workspaces",
            "Export workspaces to a file",
            AstraCommandCategory::kWorkspace,
        };
        m[kAstraCommandImportWorkspaces] = {
            "Import Workspaces",
            "Import workspaces from a file",
            AstraCommandCategory::kWorkspace,
        };

        // -- Tab features (Tab category) -----------------------------------
        m[kAstraCommandToggleTabFavorite] = {
            "Toggle Tab Favorite",
            "Mark or unmark the active tab as a favorite",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandReopenClosedTab] = {
            "Reopen Closed Tab",
            "Reopen the most recently closed tab",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandRestoreAllClosedTabs] = {
            "Restore All Closed Tabs",
            "Restore all recently closed tabs",
            AstraCommandCategory::kTab,
        };

        // -- Favorite folders (Tab category) --------------------------------
        m[kAstraCommandCreateFavoriteFolder] = {
            "Create Favorite Folder",
            "Create a new favorite folder",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandRenameFavoriteFolder] = {
            "Rename Favorite Folder",
            "Rename a favorite folder",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandDeleteFavoriteFolder] = {
            "Delete Favorite Folder",
            "Delete a favorite folder",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandMoveFavoriteToFolder] = {
            "Move Favorite to Folder",
            "Move the active favorite tab to a folder",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandToggleFavoriteFolderExpanded] = {
            "Toggle Favorite Folder Expanded",
            "Expand or collapse a favorite folder",
            AstraCommandCategory::kTab,
        };

        // -- Split view (View category) ------------------------------------
        m[kAstraCommandToggleSplitView] = {
            "Toggle Split View",
            "Enable or disable split view for the active tab",
            AstraCommandCategory::kView,
        };
        m[kAstraCommandSplitViewVertical] = {
            "Split View Vertical",
            "Arrange split view vertically",
            AstraCommandCategory::kView,
        };
        m[kAstraCommandSplitViewHorizontal] = {
            "Split View Horizontal",
            "Arrange split view horizontally",
            AstraCommandCategory::kView,
        };
        m[kAstraCommandSwapSplitViews] = {
            "Swap Split Views",
            "Swap the two panes in split view",
            AstraCommandCategory::kView,
        };

        // -- Glance (View category) ----------------------------------------
        m[kAstraCommandOpenGlance] = {
            "Open Glance",
            "Open glance / peek mode",
            AstraCommandCategory::kView,
        };

        // -- Command palette (Navigation category) --------------------------
        m[kAstraCommandOpenCommandPalette] = {
            "Open Command Palette",
            "Open the command palette for quick actions",
            AstraCommandCategory::kNavigation,
        };

        // -- Tab search (Navigation category) -------------------------------
        m[kAstraCommandOpenTabSearch] = {
            "Open Tab Search",
            "Open the tab search bubble",
            AstraCommandCategory::kNavigation,
        };

        // -- Settings (Tools category) -------------------------------------
        m[kAstraCommandOpenSettings] = {
            "Open Settings",
            "Open Astra settings",
            AstraCommandCategory::kTools,
        };
        m[kAstraCommandOpenSearchSettings] = {
            "Open Search Settings",
            "Open search engine settings",
            AstraCommandCategory::kTools,
        };

        // -- Extensions panel (Tools category) ------------------------------
        m[kAstraCommandToggleExtensionsPanel] = {
            "Toggle Extensions Panel",
            "Toggle the extensions panel in the sidebar",
            AstraCommandCategory::kTools,
        };

        // -- Omnibox (Navigation category) ----------------------------------
        m[kAstraCommandFocusOmniboxCommandMode] = {
            "Focus Omnibox Command Mode",
            "Focus the address bar in command mode",
            AstraCommandCategory::kNavigation,
        };

        // -- Focus mode (View category) ------------------------------------
        m[kAstraCommandToggleFocusMode] = {
            "Toggle Focus Mode",
            "Turn focus mode on or off",
            AstraCommandCategory::kView,
        };

        // -- Screenshot (Tools category) -----------------------------------
        m[kAstraCommandScreenshotVisible] = {
            "Screenshot Visible Area",
            "Capture the visible area of the active tab",
            AstraCommandCategory::kTools,
        };
        m[kAstraCommandScreenshotFullPage] = {
            "Screenshot Full Page",
            "Capture the full page of the active tab",
            AstraCommandCategory::kTools,
        };
        m[kAstraCommandScreenshotRegion] = {
            "Screenshot Region",
            "Capture a selected region of the active tab",
            AstraCommandCategory::kTools,
        };

        // -- DevTools (Tab category) ---------------------------------------
        m[kAstraCommandToggleDevTools] = {
            "Toggle DevTools",
            "Toggle DevTools for the active tab",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandDevToolsDockBottom] = {
            "DevTools Dock Bottom",
            "Dock DevTools to the bottom",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandDevToolsDockRight] = {
            "DevTools Dock Right",
            "Dock DevTools to the right side",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandDevToolsDockLeft] = {
            "DevTools Dock Left",
            "Dock DevTools to the left side",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandDevToolsUndock] = {
            "DevTools Undock",
            "Undock DevTools into a separate window",
            AstraCommandCategory::kTab,
        };
        m[kAstraCommandOpenAstraDevToolsPanel] = {
            "Open Astra DevTools Panel",
            "Open the Astra-specific DevTools panel",
            AstraCommandCategory::kTab,
        };

        // -- New tab page (View category) ----------------------------------
        m[kAstraCommandOpenNewTabPage] = {
            "Open New Tab Page",
            "Open the Astra new tab page",
            AstraCommandCategory::kView,
        };

        return m;
      }());
  return *map;
}

}  // namespace

// =========================================================================
// Command identity
// =========================================================================

bool AstraCommandDelegate::IsAstraCommand(int command_id) {
  return command_id >= kAstraCommandFirst && command_id < kAstraCommandLast;
}

bool AstraCommandDelegate::SupportsCommand(int command_id) {
  // Every command in the Astra range with metadata is supported.
  if (!IsAstraCommand(command_id)) {
    return false;
  }
  const auto& map = GetCommandMetadataMap();
  return map.find(command_id) != map.end();
}

// =========================================================================
// Execution
// =========================================================================

bool AstraCommandDelegate::ExecuteCommand(Browser* browser, int command_id) {
  // Guard: valid browser and valid Astra command.
  if (!browser || !IsAstraCommand(command_id)) {
    return false;
  }

  bool handled = false;

  switch (command_id) {
    // -- Sidebar (UI observer notifications) -----------------------------

    case kAstraCommandToggleSidebar:
      for (auto& observer : GetObservers()) {
        observer.OnToggleSidebar();
      }
      handled = true;
      break;

    case kAstraCommandToggleSidebarPin:
      for (auto& observer : GetObservers()) {
        observer.OnToggleSidebarPin();
      }
      handled = true;
      break;

    // -- Workspaces (AstraWorkspaceService) -----------------------------

    case kAstraCommandNewWorkspace: {
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      // TODO(astra): Generate a stable unique id instead of a counter.
      // Consider using base::Token or base::UnguessableToken.
      static int workspace_counter = 0;
      std::string new_id =
          "workspace-" + base::NumberToString(++workspace_counter);
      AstraWorkspace workspace;
      workspace.id = new_id;
      workspace.name = "Workspace " + base::NumberToString(workspace_counter);
      workspace.accent_color = "#5AD8A6";
      service->AddWorkspace(std::move(workspace));
      service->ActivateWorkspace(new_id);
      handled = true;
      break;
    }

    case kAstraCommandNextWorkspace: {
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      // In incognito mode, delegate navigation to the UI layer (sidebar)
      // so that the shared service's active workspace (which is persisted
      // on the original profile) is not modified.
      // See AstraIncognitoHandler::DoesWorkspaceActivationAffectService.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        for (auto& observer : GetObservers()) {
          observer.OnWorkspaceNavigateRequested(+1);
        }
        handled = true;
        break;
      }
      std::string next_id = service->GetNextWorkspaceId();
      if (next_id == service->active_workspace_id()) {
        return false;  // No change (only one workspace).
      }
      service->ActivateWorkspace(next_id);
      handled = true;
      break;
    }

    case kAstraCommandPreviousWorkspace: {
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      // In incognito mode, delegate navigation to the UI layer.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        for (auto& observer : GetObservers()) {
          observer.OnWorkspaceNavigateRequested(-1);
        }
        handled = true;
        break;
      }
      std::string prev_id = service->GetPreviousWorkspaceId();
      if (prev_id == service->active_workspace_id()) {
        return false;  // No change (only one workspace).
      }
      service->ActivateWorkspace(prev_id);
      handled = true;
      break;
    }

    case kAstraCommandRenameWorkspace: {
      // TODO(astra): Rename currently targets the active workspace.
      // In a full implementation, the rename command would probably open a
      // dialog or use inline editing in the sidebar, with the specific
      // workspace id coming from the UI context.  For now, we rename the
      // active workspace as a proof of concept.
      //
      // The proper flow:
      //   1. User triggers rename (from sidebar context menu or keyboard).
      //   2. UI shows an inline rename field or dialog.
      //   3. On commit, UI calls service->RenameWorkspace(id, new_name).
      //
      // The command is still useful for keyboard accelerators that target
      // the active workspace, but the real rename UX lives in the UI.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      // For the command-only path, append " (renamed)" to prove the
      // rename path works.  Real implementation gets the name from UI.
      const std::string& active_id = service->active_workspace_id();
      const AstraWorkspace* ws = service->GetWorkspace(active_id);
      if (!ws) {
        return false;
      }
      std::string new_name = ws->name + " (renamed)";
      handled = service->RenameWorkspace(active_id, new_name);
      break;
    }

    case kAstraCommandDeleteWorkspace: {
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      // Delete the active workspace (falls back to default).
      // The service itself rejects deletion of the last/default workspace.
      handled = service->DeleteWorkspace(service->active_workspace_id());
      break;
    }

    case kAstraCommandMoveTabToNextWorkspace: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!web_contents || !service) {
        return false;
      }
      if (service->workspace_count() <= 1) {
        return false;  // Nowhere to move.
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      std::string current_ws = features->workspace_id();
      // Find the index of the current workspace and advance.
      size_t idx = service->GetWorkspaceIndex(current_ws);
      size_t next_idx = (idx + 1) % service->workspace_count();
      const auto& workspaces = service->workspaces();
      features->set_workspace_id(workspaces[next_idx].id);
      // TODO(astra): Notify workspace service / sidebar that tab metadata
      // has changed so the UI can refresh.  For now, the UI will pick up
      // the change on its next repaint / model read.
      handled = true;
      break;
    }

    case kAstraCommandMoveTabToPreviousWorkspace: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!web_contents || !service) {
        return false;
      }
      if (service->workspace_count() <= 1) {
        return false;  // Nowhere to move.
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      std::string current_ws = features->workspace_id();
      size_t idx = service->GetWorkspaceIndex(current_ws);
      size_t count = service->workspace_count();
      size_t prev_idx = (idx + count - 1) % count;
      const auto& workspaces = service->workspaces();
      features->set_workspace_id(workspaces[prev_idx].id);
      // TODO(astra): Notify sidebar of tab workspace change.
      handled = true;
      break;
    }

    case kAstraCommandShowAllWorkspaces:
      for (auto& observer : GetObservers()) {
        observer.OnShowAllWorkspaces();
      }
      handled = true;
      break;

    // -- Multi-window workspaces ----------------------------------------

    case kAstraCommandNewWindowInWorkspace: {
      // Create a new browser window in the current active workspace.
      //
      // Chromium owner: Browser::Create() — we delegate window creation
      // to Chromium and just assign the workspace.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service || !browser->profile()) {
        return false;
      }
      std::string workspace_id = service->active_workspace_id();
      // Incognito windows stay in default workspace (read-only).
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        workspace_id = "default";
      }
      AstraWorkspaceWindowManager::GetInstance()->CreateNewWindowInWorkspace(
          browser->profile(), workspace_id);
      handled = true;
      break;
    }

    case kAstraCommandMoveWindowToNextWorkspace: {
      // Move the current browser window to the next workspace.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service || !browser) {
        return false;
      }
      if (service->workspace_count() <= 1) {
        return false;  // Nowhere to move.
      }
      std::string current_ws =
          AstraWorkspaceWindowManager::GetInstance()->GetWorkspaceForWindow(
              browser);
      size_t idx = service->GetWorkspaceIndex(current_ws);
      size_t next_idx = (idx + 1) % service->workspace_count();
      const auto& workspaces = service->workspaces();
      AstraWorkspaceWindowManager::GetInstance()->MoveWindowToWorkspace(
          browser, workspaces[next_idx].id);
      handled = true;
      break;
    }

    case kAstraCommandMoveWindowToPreviousWorkspace: {
      // Move the current browser window to the previous workspace.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service || !browser) {
        return false;
      }
      if (service->workspace_count() <= 1) {
        return false;  // Nowhere to move.
      }
      std::string current_ws =
          AstraWorkspaceWindowManager::GetInstance()->GetWorkspaceForWindow(
              browser);
      size_t idx = service->GetWorkspaceIndex(current_ws);
      size_t count = service->workspace_count();
      size_t prev_idx = (idx + count - 1) % count;
      const auto& workspaces = service->workspaces();
      AstraWorkspaceWindowManager::GetInstance()->MoveWindowToWorkspace(
          browser, workspaces[prev_idx].id);
      handled = true;
      break;
    }

    // -- Tab features (AstraTabFeatures on active WebContents) ----------

    case kAstraCommandToggleTabFavorite: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      features->ToggleFavorite();
      // TODO(astra): Notify sidebar / favorites UI of the change.
      // Consider adding an AstraTabFeaturesObserver or broadcasting via
      // a TabStripModelObserver patch.
      for (auto& observer : GetObservers()) {
        observer.OnFavoriteFoldersChanged();
      }
      handled = true;
      break;
    }

    // -- Recently closed tabs (AstraRecentTabsHelper) --------------------

    case kAstraCommandReopenClosedTab: {
      // Reopen (restore) the most recently closed tab.
      //
      // The actual restore logic is owned by Chromium's TabRestoreService.
      // AstraRecentTabsHelper wraps the service for sidebar integration.
      //
      // Chromium owner: sessions::TabRestoreService
      //   (chrome/browser/sessions/tab_restore_service.h)
      // Chromium command: IDC_RESTORE_TAB
      //   (chrome/browser/ui/commands/command_ids.h)
      //
      // This Astra command provides the same functionality but routes
      // through the Astra command delegate for sidebar integration and
      // observer notifications (e.g. highlighting the recently closed
      // section when a tab is restored).
      content::WebContents* restored =
          AstraRecentTabsHelper::RestoreMostRecentTab(browser->profile());
      if (restored) {
        for (auto& observer : GetObservers()) {
          observer.OnRecentlyClosedTabRestored();
        }
        handled = true;
      }
      break;
    }

    case kAstraCommandRestoreAllClosedTabs: {
      // Restore all recently closed tabs.
      //
      // This is an Astra-specific command — Chromium does not have a
      // built-in "restore all" command for recently closed tabs.
      //
      // Chromium owner: sessions::TabRestoreService
      //   (chrome/browser/sessions/tab_restore_service.h)
      size_t restored =
          AstraRecentTabsHelper::RestoreAll(browser->profile());
      if (restored > 0) {
        for (auto& observer : GetObservers()) {
          observer.OnAllRecentlyClosedTabsRestored();
        }
        handled = true;
      }
      break;
    }

    // -- Favorites / favorite folders (AstraFavoriteService) --------------

    case kAstraCommandCreateFavoriteFolder: {
      AstraFavoriteService* service = GetFavoriteService(browser);
      if (!service) {
        return false;
      }
      // TODO(astra): Folder name should come from UI (dialog or inline
      // rename).  For now we create a folder with a default name as a
      // proof of concept.  The proper flow:
      //   1. User triggers "new folder" from sidebar or command palette.
      //   2. UI shows input for folder name.
      //   3. On commit, UI calls service->AddFolder(name, parent_id).
      static int folder_counter = 0;
      std::string name = "Folder " + base::NumberToString(++folder_counter);
      std::string folder_id = service->AddFolder(name);
      if (!folder_id.empty()) {
        for (auto& observer : GetObservers()) {
          observer.OnFavoriteFoldersChanged();
        }
        handled = true;
      }
      break;
    }

    case kAstraCommandRenameFavoriteFolder: {
      // TODO(astra): Rename currently targets a folder by id from UI
      // context.  As a command-only stand-in, we rename the first
      // non-root folder to prove the path works.  Real implementation
      // gets the folder id and new name from UI context.
      AstraFavoriteService* service = GetFavoriteService(browser);
      if (!service) {
        return false;
      }
      // Find the first non-root folder to rename.
      // In practice, the UI provides the folder id.
      const auto& folders = service->folders();
      std::string target_id;
      for (const auto& folder : folders) {
        if (!folder.is_root) {
          target_id = folder.id;
          break;
        }
      }
      if (target_id.empty()) {
        return false;  // No folder to rename.
      }
      const AstraFavoriteFolder* folder = service->GetFolder(target_id);
      if (!folder) {
        return false;
      }
      std::string new_name = folder->name + " (renamed)";
      bool result = service->RenameFolder(target_id, new_name);
      if (result) {
        for (auto& observer : GetObservers()) {
          observer.OnFavoriteFoldersChanged();
        }
      }
      handled = result;
      break;
    }

    case kAstraCommandDeleteFavoriteFolder: {
      // TODO(astra): Deletion should target a specific folder from UI
      // context.  As a command stand-in, we delete the first non-root
      // folder.  Real implementation gets the folder id from UI context.
      AstraFavoriteService* service = GetFavoriteService(browser);
      if (!service) {
        return false;
      }
      const auto& folders = service->folders();
      std::string target_id;
      for (const auto& folder : folders) {
        if (!folder.is_root) {
          target_id = folder.id;
          break;
        }
      }
      if (target_id.empty()) {
        return false;  // No folder to delete.
      }
      bool result = service->DeleteFolder(target_id);
      if (result) {
        for (auto& observer : GetObservers()) {
          observer.OnFavoriteFoldersChanged();
        }
      }
      handled = result;
      break;
    }

    case kAstraCommandMoveFavoriteToFolder: {
      // Move the active tab's favorite to the next folder.
      // TODO(astra): Real implementation gets the target folder from UI
      // context (drag-and-drop or context menu).  As a command stand-in,
      // we cycle through folders.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      AstraFavoriteService* service = GetFavoriteService(browser);
      if (!web_contents || !service) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      const auto& folders = service->folders();
      if (folders.size() <= 1) {
        return false;  // Only root — nowhere to move.
      }
      // Find the current folder index and advance to the next folder.
      size_t current_idx = 0;
      for (size_t i = 0; i < folders.size(); ++i) {
        if (folders[i].id == features->favorite_folder_id()) {
          current_idx = i;
          break;
        }
      }
      size_t next_idx = (current_idx + 1) % folders.size();
      const std::string& next_folder_id = folders[next_idx].id;
      bool result = service->MoveFavoriteToFolder(web_contents, next_folder_id);
      if (result) {
        for (auto& observer : GetObservers()) {
          observer.OnFavoriteFoldersChanged();
        }
      }
      handled = result;
      break;
    }

    case kAstraCommandToggleFavoriteFolderExpanded: {
      // TODO(astra): Toggle expanded state for a specific folder from UI
      // context.  As a stand-in, toggle the first non-root folder.
      AstraFavoriteService* service = GetFavoriteService(browser);
      if (!service) {
        return false;
      }
      const auto& folders = service->folders();
      std::string target_id;
      for (const auto& folder : folders) {
        if (!folder.is_root) {
          target_id = folder.id;
          break;
        }
      }
      if (target_id.empty()) {
        return false;
      }
      handled = service->ToggleFolderExpanded(target_id);
      break;
    }

    // -- Split view (metadata + UI notification) ------------------------

    case kAstraCommandToggleSplitView: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      // Toggle split view state for the active tab.
      // TODO(astra): In a full split view implementation, toggling would
      // find a partner tab or open a second view.  For now we just flip
      // the metadata flag and let the UI handle presentation.
      features->set_is_in_split_view(!features->is_in_split_view());
      if (!features->is_in_split_view()) {
        // When exiting split view, clear partner info.
        features->set_split_view_partner_id(std::string());
      }
      for (auto& observer : GetObservers()) {
        observer.OnSplitViewStateChanged();
      }
      handled = true;
      break;
    }

    case kAstraCommandSplitViewVertical: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      features->set_split_view_orientation(SplitViewOrientation::kVertical);
      // Also ensure split view is active.
      if (!features->is_in_split_view()) {
        features->set_is_in_split_view(true);
      }
      for (auto& observer : GetObservers()) {
        observer.OnSplitViewStateChanged();
      }
      handled = true;
      break;
    }

    case kAstraCommandSplitViewHorizontal: {
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      features->set_split_view_orientation(SplitViewOrientation::kHorizontal);
      // Also ensure split view is active.
      if (!features->is_in_split_view()) {
        features->set_is_in_split_view(true);
      }
      for (auto& observer : GetObservers()) {
        observer.OnSplitViewStateChanged();
      }
      handled = true;
      break;
    }

    case kAstraCommandSwapSplitViews: {
      // TODO(astra): Swap needs a split view pair manager that tracks
      // which two tabs/views are in split view together.  Per-tab
      // metadata alone isn't enough for swap because we need to know
      // the partner tab's identity and swap their positions.
      //
      // Chromium subsystem to reuse: TabStripModel (for tab ordering)
      // + WebContents pair (for the two split panes).
      //
      // For now, flip the split ratio as a stand-in for "swap sides"
      // and notify the UI.  The actual swap semantics should be
      // implemented in a split view controller.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      if (!features->is_in_split_view()) {
        return false;  // Nothing to swap if not in split view.
      }
      // Invert the split ratio as a stand-in for "swap sides".
      // Real implementation would swap the two WebContents positions.
      float new_ratio = 1.0f - features->split_view_ratio();
      features->set_split_view_ratio(new_ratio);
      for (auto& observer : GetObservers()) {
        observer.OnSplitViewStateChanged();
      }
      handled = true;
      break;
    }

    // -- Glance (UI notification) ---------------------------------------

    case kAstraCommandOpenGlance:
      // TODO(astra): Glance may need to know which tab / URL to show.
      // For now, the UI determines context (active tab's link hover,
      // bookmark, etc.) and opens the glance overlay.
      for (auto& observer : GetObservers()) {
        observer.OnOpenGlance();
      }
      handled = true;
      break;

    // -- Command palette (UI notification) ------------------------------

    case kAstraCommandOpenCommandPalette:
      // TODO(astra): The command palette should list both Chrome
      // standard commands and Astra-specific commands.  It queries
      // Chrome's command controller for standard commands and this
      // delegate for Astra commands, then merges the list.
      //
      // The command palette UI itself lives in astra/ui/views and
      // implements AstraCommandDelegate::Observer to receive this
      // signal.
      for (auto& observer : GetObservers()) {
        observer.OnOpenCommandPalette();
      }
      handled = true;
      break;

    // -- Tab search (UI notification) -----------------------------------

    case kAstraCommandOpenTabSearch:
      // Opens the tab search bubble (quick tab switcher).
      //
      // Tab data is owned by Chromium's TabStripModel.  The tab search
      // UI is a projection layer that reads from TabStripModel and
      // never stores tab state.
      //
      // Chromium owner: TabSearchBubbleHost / TabSearchButton
      // (chrome/browser/ui/views/tab_search/)
      // Patch point: Tab strip tab search button, or keyboard shortcut.
      for (auto& observer : GetObservers()) {
        observer.OnOpenTabSearch();
      }
      handled = true;
      break;

    // -- Settings (UI notification) --------------------------------------

    case kAstraCommandOpenSettings:
      // Opens the Astra settings bubble / page.
      //
      // Settings state lives in PrefService (truth source). The settings
      // page is a Views surface that reads/writes prefs directly.
      // This command just shows the settings UI.
      //
      // Chromium owner: Settings UI (chrome/browser/ui/views/settings/)
      // or WebUI settings (chrome/browser/resources/settings/).
      // Patch point: Chrome settings page sidebar or toolbar settings
      // button, which could be patched to open the Astra settings.
      for (auto& observer : GetObservers()) {
        observer.OnOpenSettings();
      }
      handled = true;
      break;

    case kAstraCommandOpenSearchSettings:
      // Opens Chrome's search engine settings page.
      //
      // Search engine state is fully owned by Chromium's TemplateURLService.
      // This command provides quick access to Chrome's search engine
      // management UI from the Astra command palette and sidebar.
      //
      // Chromium owner: TemplateURLService
      //   (components/search_engines/template_url_service.h)
      // Chromium WebUI: chrome://settings/searchEngines
      //   (chrome/browser/resources/settings/search_engines_page/)
      //
      // The UI layer handles opening the settings page (either Chrome's
      // WebUI or Astra's settings with a search engine section).
      for (auto& observer : GetObservers()) {
        observer.OnOpenSearchSettings();
      }
      handled = true;
      break;

    case kAstraCommandSwitchWorkspaceMenu:
      // Opens the workspace switcher menu in the profile menu area.
      //
      // This is a UI-level command: the actual workspace switching
      // logic lives in AstraWorkspaceService, and the menu UI lives in
      // astra/ui/views/profiles/.  The command delegate just forwards
      // the "show menu" signal to the UI observer.
      //
      // Chromium owner: ProfileMenuView / AvatarToolbarButton
      // (chrome/browser/ui/views/profiles/,
      //  chrome/browser/ui/views/toolbar/)
      // Patch point: Avatar toolbar button click, or an additional
      // button in the toolbar area next to the avatar.
      for (auto& observer : GetObservers()) {
        observer.OnSwitchWorkspaceMenu();
      }
      handled = true;
      break;

    // -- Extensions panel (UI observer notification) ----------------------

    case kAstraCommandToggleExtensionsPanel:
      // Toggles the extensions panel in the sidebar.
      //
      // This is a UI-level command: the actual extension state lives
      // in Chromium's ExtensionRegistry, and the extensions UI lives in
      // astra/ui/views/sidebar/.  The command delegate just forwards
      // the "toggle" signal to the UI observer.
      //
      // Chromium owner: ExtensionsToolbarButton / ExtensionsToolbarContainer
      //   (chrome/browser/ui/views/toolbar/extensions_toolbar_button.h)
      //   (chrome/browser/ui/views/toolbar/extensions_toolbar_container.h)
      // Patch point: The extensions toolbar button click could be patched
      //   to toggle the sidebar extensions panel instead of the toolbar menu.
      for (auto& observer : GetObservers()) {
        observer.OnToggleExtensionsPanel();
      }
      handled = true;
      break;

    // -- Workspace import / export (UI observer notifications) ------------

    case kAstraCommandExportWorkspaces:
      // Opens the workspace export dialog.
      //
      // This is a UI-level command: the actual export logic lives in
      // AstraWorkspaceImportExport, and the dialog UI lives in
      // astra/ui/views/workspace/.  The command delegate just forwards
      // the "show export dialog" signal to the UI observer.
      //
      // Chromium owner: SelectFileDialog (ui/shell_dialogs/select_file_dialog.h)
      // Patch point: File save dialog for JSON export.
      for (auto& observer : GetObservers()) {
        observer.OnExportWorkspaces();
      }
      handled = true;
      break;

    case kAstraCommandImportWorkspaces:
      // Opens the workspace import dialog.
      //
      // This is a UI-level command: the actual import logic lives in
      // AstraWorkspaceImportExport, and the dialog UI lives in
      // astra/ui/views/workspace/.  The command delegate just forwards
      // the "show import dialog" signal to the UI observer.
      //
      // Chromium owner: SelectFileDialog (ui/shell_dialogs/select_file_dialog.h)
      // Patch point: File open dialog for JSON import.
      for (auto& observer : GetObservers()) {
        observer.OnImportWorkspaces();
      }
      handled = true;
      break;

    // -- Omnibox (UI notification) ----------------------------------------

    case kAstraCommandFocusOmniboxCommandMode:
      // Focuses the omnibox and pre-fills it with the "> " command prefix.
      //
      // This is a UI-level command because the browser layer cannot
      // directly manipulate omnibox text — that lives in the views layer.
      // The UI layer (which has access to LocationBarView / OmniboxView)
      // handles focusing the omnibox and inserting the prefix text.
      //
      // Chromium owner: LocationBarView / OmniboxView
      // (chrome/browser/ui/views/location_bar/location_bar_view.h)
      // Patch point: The UI layer calls omnibox_view_->SetUserText(u"> ")
      // and requests focus on the omnibox.
      for (auto& observer : GetObservers()) {
        observer.OnFocusOmniboxCommandMode();
      }
      handled = true;
      break;

    // -- Focus mode -------------------------------------------------------

    case kAstraCommandToggleFocusMode: {
      // Toggles focus mode on/off.
      //
      // Focus mode state is managed by AstraFocusModeService (profile-
      // scoped keyed service). The service is the truth source for whether
      // focus mode is active, the remaining time, and the blocklist.
      //
      // This command:
      //   1. Toggles focus mode state in the service.
      //   2. Notifies UI observers so they can update presentation.
      //
      // Architecture:
      //   - Truth source: AstraFocusModeService.
      //   - UI projection: AstraFocusModeController (per-BrowserView).
      //   - Indicator: AstraFocusModeIndicator (floating widget).
      //
      // Chromium subsystems reused:
      //   - PrefService (for default duration + blocklist persistence).
      //   - ProfileKeyedServiceFactory pattern.
      //   - Fullscreen / ImmersiveModeController (future, for toolbar hide).
      //   - HostContentSettingsMap / NavigationThrottle (future, for blocking).
      AstraFocusModeService* service = GetFocusModeService(browser);
      if (!service) {
        return false;
      }
      service->ToggleFocusMode();
      for (auto& observer : GetObservers()) {
        observer.OnToggleFocusMode();
      }
      handled = true;
      break;
    }

    // -- Screenshot / screen capture (UI + service) -------------------

    case kAstraCommandScreenshotVisible:
      // Capture the visible area (viewport) of the active tab.
      //
      // This is a UI-level command: the actual capture logic lives in
      // AstraScreenshotService, and the capture bubble UI lives in
      // astra/ui/views/screenshot/. The command delegate just forwards
      // the "capture visible" signal to the UI observer, which handles
      // showing the capture overlay and the result bubble.
      //
      // Chromium owner: content::WebContents::GetContentBitmap or
      // (content/public/browser/web_contents.h)
      // Chromium owner: ScreenshotManager (content/browser/screenshot/)
      // Patch point: None needed for basic visible area capture — uses
      //   public WebContents API.
      for (auto& observer : GetObservers()) {
        observer.OnScreenshotVisible();
      }
      handled = true;
      break;

    case kAstraCommandScreenshotFullPage:
      // Capture the full page (entire scrollable document) of the active tab.
      //
      // Full page capture is more complex than visible area because it
      // requires scrolling or using the compositor's full-page snapshot.
      //
      // Chromium owner: ScreenshotManager / ShareManager
      // (chrome/browser/screenshot/, chrome/browser/share/)
      // Patch point: May need to expose full-page capture API or add
      //   an Astra hook in ScreenshotManager.
      for (auto& observer : GetObservers()) {
        observer.OnScreenshotFullPage();
      }
      handled = true;
      break;

    case kAstraCommandScreenshotRegion:
      // Capture a user-selected region of the active tab.
      //
      // The command shows the region selection overlay, waits for the
      // user to draw a rectangle, then captures that region.
      //
      // Chromium owner: ScreenshotManager region capture or
      //   DevTools element picker pattern.
      // Patch point: Region selection UI is Astra-specific; the
      //   actual capture uses Chromium APIs.
      for (auto& observer : GetObservers()) {
        observer.OnScreenshotRegion();
      }
      handled = true;
      break;

    // -- DevTools (AstraDevToolsHelper) -----------------------------------

    case kAstraCommandToggleDevTools:
      // Toggles DevTools for the active tab.
      //
      // DevTools state and window management is fully owned by Chromium.
      // AstraDevToolsHelper wraps Chromium's DevToolsWindow API.
      //
      // Chromium owner: DevToolsWindow::ToggleDevToolsWindow
      //   (chrome/browser/devtools/devtools_window.h)
      AstraDevToolsHelper::ToggleDevToolsForBrowser(browser);
      handled = true;
      break;

    case kAstraCommandDevToolsDockBottom: {
      // Docks DevTools to the bottom of the browser window.
      //
      // Chromium owner: DevToolsWindow::SetDockSide
      //   (chrome/browser/devtools/devtools_window.h)
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraDevToolsHelper::SetDevToolsDockState(web_contents,
          AstraDevToolsDockState::kBottom);
      handled = true;
      break;
    }

    case kAstraCommandDevToolsDockRight: {
      // Docks DevTools to the right side of the browser window.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraDevToolsHelper::SetDevToolsDockState(web_contents,
          AstraDevToolsDockState::kRight);
      handled = true;
      break;
    }

    case kAstraCommandDevToolsDockLeft: {
      // Docks DevTools to the left side of the browser window.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraDevToolsHelper::SetDevToolsDockState(web_contents,
          AstraDevToolsDockState::kLeft);
      handled = true;
      break;
    }

    case kAstraCommandDevToolsUndock: {
      // Undocks DevTools into a separate window.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraDevToolsHelper::SetDevToolsDockState(web_contents,
          AstraDevToolsDockState::kUndocked);
      handled = true;
      break;
    }

    case kAstraCommandOpenAstraDevToolsPanel: {
      // Opens the Astra-specific DevTools panel (workspace/favorite tools).
      //
      // This is an optional advanced feature that adds Astra-specific
      // functionality to the DevTools window.
      //
      // TODO(astra): Implement Astra DevTools panel / toolbar.
      //   Options: DevTools extension, WebUI panel, or Views toolbar.
      //   See AstraDevToolsHelper::OpenAstraDevToolsPanel for details.
      //
      // Chromium owner: DevToolsWindow
      //   (chrome/browser/devtools/devtools_window.h)
      // Patch point: DevToolsWindow::SetDockSide or Create
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      AstraDevToolsHelper::OpenAstraDevToolsPanel(web_contents);
      handled = true;
      break;
    }

    // -- New tab page (UI notification) ------------------------------------

    case kAstraCommandOpenNewTabPage:
      // Opens the Astra-branded new tab page.
      //
      // This is a UI-level command: the actual NTP content lives in
      // astra/ui/views/newtab/. The command delegate just forwards
      // the "show NTP" signal to the UI observer.
      //
      // Chromium owner: NewTabPageUI
      //   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
      // Patch point: chrome/browser/new_tab_page/new_tab_page_url_handler.cc
      //   — to redirect chrome://newtab to the Astra NTP.
      //
      // TODO(astra): Proper integration with Chrome's new tab creation
      //   path so that new tabs show the Astra NTP instead of Chrome's.
      for (auto& observer : GetObservers()) {
        observer.OnOpenNewTabPage();
      }
      handled = true;
      break;

    default:
      return false;
  }

  // If the command was handled, record it in recent commands history and
  // notify general command-execution observers.
  if (handled) {
    RecordRecentCommand(browser->profile(), command_id);
    NotifyCommandExecuted(command_id);
  }

  return handled;
}

// =========================================================================
// Enabled state
// =========================================================================

bool AstraCommandDelegate::IsCommandEnabled(Browser* browser,
                                            int command_id) {
  if (!browser || !IsAstraCommand(command_id)) {
    return false;
  }

  // UI observer-based commands are always enabled if any observer is
  // registered.  If no UI has attached yet, they're still valid commands
  // but nobody is listening — we report them as enabled because the UI may
  // attach later (e.g. command palette should show them as available).
  switch (command_id) {
    case kAstraCommandToggleSidebar:
    case kAstraCommandToggleSidebarPin:
    case kAstraCommandOpenCommandPalette:
    case kAstraCommandOpenTabSearch:
    case kAstraCommandOpenSettings:
    case kAstraCommandOpenSearchSettings:
    case kAstraCommandOpenGlance:
    case kAstraCommandShowAllWorkspaces:
    case kAstraCommandSwitchWorkspaceMenu:
    case kAstraCommandToggleExtensionsPanel:
    case kAstraCommandExportWorkspaces:
    case kAstraCommandImportWorkspaces:
    case kAstraCommandFocusOmniboxCommandMode:
    case kAstraCommandToggleFocusMode:
    case kAstraCommandScreenshotVisible:
    case kAstraCommandScreenshotFullPage:
    case kAstraCommandScreenshotRegion:
    case kAstraCommandOpenNewTabPage:
      // These are UI presentation commands; enabled as long as there's a
      // valid browser window.
      return true;

    case kAstraCommandNewWindowInWorkspace:
      // New window in workspace is always enabled if there's a valid profile.
      // Incognito windows are allowed to create new incognito windows.
      return browser->profile() != nullptr;

    case kAstraCommandMoveWindowToNextWorkspace:
    case kAstraCommandMoveWindowToPreviousWorkspace: {
      // Moving windows between workspaces requires at least 2 workspaces.
      // Disabled in incognito (window workspace is read-only).
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      return service && service->workspace_count() > 1;
    }

    case kAstraCommandNewWorkspace:
      // Workspace creation is a persistent mutation — disabled in incognito.
      // See AstraIncognitoHandler::AreWorkspaceMutationsAllowed.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      return true;

    case kAstraCommandNextWorkspace:
    case kAstraCommandPreviousWorkspace: {
      // Workspace navigation is allowed in incognito — it is local to the
      // sidebar/window and does not modify persisted state.
      // Note: in incognito mode, the active workspace is tracked locally by
      // the sidebar, not by the shared service.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      // Need at least 2 workspaces to navigate between them.
      return service && service->workspace_count() > 1;
    }

    case kAstraCommandMoveTabToNextWorkspace:
    case kAstraCommandMoveTabToPreviousWorkspace: {
      // Moving tabs between workspaces is allowed in incognito — it only
      // changes per-tab metadata (workspace_id on AstraTabFeatures) which
      // is ephemeral and not persisted.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      // Need at least 2 workspaces to move between them.
      return web_contents && service && service->workspace_count() > 1;
    }

    case kAstraCommandRenameWorkspace: {
      // Workspace rename is a persistent mutation — disabled in incognito.
      // See AstraIncognitoHandler::AreWorkspaceMutationsAllowed.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      return service && !service->active_workspace_id().empty();
    }

    case kAstraCommandDeleteWorkspace: {
      // Workspace deletion is a persistent mutation — disabled in incognito.
      // See AstraIncognitoHandler::AreWorkspaceMutationsAllowed.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      // Can't delete the last workspace.  The service also rejects
      // deletion of the default workspace, but we check count here for
      // UI enable/disable purposes.
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      return service && service->workspace_count() > 1;
    }

    case kAstraCommandToggleTabFavorite: {
      // Favorite toggling is disabled in incognito — favorites are read-only.
      // See AstraIncognitoHandler::AreFavoritesMutable for rationale.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      // Need an active tab to toggle favorite state.
      return GetActiveWebContents(browser) != nullptr;
    }

    case kAstraCommandReopenClosedTab:
    case kAstraCommandRestoreAllClosedTabs:
      // Reopen/restore commands are enabled if there are recently closed tabs.
      //
      // Recently closed tabs are profile-scoped and not available in incognito.
      // Chromium's TabRestoreService does not track closed tabs for OTR profiles.
      //
      // Chromium owner: sessions::TabRestoreService
      //   (chrome/browser/sessions/tab_restore_service.h)
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      return AstraRecentTabsHelper::HasRecentlyClosedTabs(browser->profile());

    case kAstraCommandCreateFavoriteFolder: {
      // Folder creation is a persistent mutation — disabled in incognito.
      // See AstraIncognitoHandler::AreFavoriteFoldersMutable.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      // Can always create a folder.
      AstraFavoriteService* service = GetFavoriteService(browser);
      return service != nullptr;
    }

    case kAstraCommandRenameFavoriteFolder:
    case kAstraCommandDeleteFavoriteFolder:
    case kAstraCommandToggleFavoriteFolderExpanded: {
      // Folder mutations (rename/delete/expand) are persistent and shared
      // with the original profile — disabled in incognito.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      // Need at least one non-root folder to rename/delete/toggle.
      AstraFavoriteService* service = GetFavoriteService(browser);
      return service && service->folder_count() > 1;
    }

    case kAstraCommandMoveFavoriteToFolder: {
      // Moving favorites between folders is a mutation — disabled in incognito.
      if (AstraIncognitoHandler::IsIncognitoProfile(browser->profile())) {
        return false;
      }
      // Need an active tab and at least two folders to move between.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      AstraFavoriteService* service = GetFavoriteService(browser);
      return web_contents && service && service->folder_count() > 1;
    }

    case kAstraCommandToggleSplitView:
    case kAstraCommandSplitViewVertical:
    case kAstraCommandSplitViewHorizontal:
    case kAstraCommandSwapSplitViews: {
      // Split view commands need at least one tab (the active one).
      // Swap additionally needs a partner, but we don't track that at
      // this layer yet — see the note in ExecuteCommand for
      // kAstraCommandSwapSplitViews.
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      if (command_id == kAstraCommandSwapSplitViews) {
        // Swap additionally requires being in split view already.
        AstraTabFeatures* features =
            AstraTabFeatures::FromWebContents(web_contents);
        return features && features->is_in_split_view();
      }
      return true;
    }

    // -- DevTools --------------------------------------------------------

    case kAstraCommandToggleDevTools:
    case kAstraCommandDevToolsDockBottom:
    case kAstraCommandDevToolsDockRight:
    case kAstraCommandDevToolsDockLeft:
    case kAstraCommandDevToolsUndock:
    case kAstraCommandOpenAstraDevToolsPanel: {
      // DevTools commands need an active tab (the tab being inspected).
      //
      // DevTools state and window management is fully owned by Chromium's
      // DevToolsWindow.  Astra only projects the state through
      // AstraDevToolsHelper.
      //
      // Chromium owner: DevToolsWindow
      //   (chrome/browser/devtools/devtools_window.h)
      content::WebContents* web_contents = GetActiveWebContents(browser);
      if (!web_contents) {
        return false;
      }
      // Dock state commands additionally require DevTools to be open.
      if (command_id == kAstraCommandDevToolsDockBottom ||
          command_id == kAstraCommandDevToolsDockRight ||
          command_id == kAstraCommandDevToolsDockLeft ||
          command_id == kAstraCommandDevToolsUndock) {
        return AstraDevToolsHelper::IsDevToolsOpenForTab(web_contents);
      }
      // Toggle and open-panel commands are always enabled with an active tab.
      return true;
    }

    default:
      return false;
  }
}

// =========================================================================
// Command metadata
// =========================================================================

bool AstraCommandDelegate::GetCommandInfo(int command_id,
                                          AstraCommandInfo* out_info) {
  if (!out_info) {
    return false;
  }
  const auto& map = GetCommandMetadataMap();
  auto it = map.find(command_id);
  if (it == map.end()) {
    return false;
  }
  const auto& entry = it->second;
  out_info->command_id = command_id;
  out_info->name = entry.name;
  out_info->description = entry.description;
  out_info->category = entry.category;
  return true;
}

std::string AstraCommandDelegate::GetCommandName(int command_id) {
  const auto& map = GetCommandMetadataMap();
  auto it = map.find(command_id);
  if (it == map.end()) {
    return std::string();
  }
  return it->second.name;
}

std::string AstraCommandDelegate::GetCommandDescription(int command_id) {
  const auto& map = GetCommandMetadataMap();
  auto it = map.find(command_id);
  if (it == map.end()) {
    return std::string();
  }
  return it->second.description;
}

AstraCommandCategory AstraCommandDelegate::GetCommandCategory(int command_id) {
  const auto& map = GetCommandMetadataMap();
  auto it = map.find(command_id);
  DCHECK(it != map.end()) << "Unknown command ID: " << command_id;
  if (it == map.end()) {
    return AstraCommandCategory::kTools;
  }
  return it->second.category;
}

std::vector<int> AstraCommandDelegate::GetAllCommandIds() {
  const auto& map = GetCommandMetadataMap();
  std::vector<int> ids;
  ids.reserve(map.size());
  for (const auto& entry : map) {
    ids.push_back(entry.first);
  }
  return ids;
}

std::vector<int> AstraCommandDelegate::GetCommandsByCategory(
    AstraCommandCategory category) {
  const auto& map = GetCommandMetadataMap();
  std::vector<int> ids;
  for (const auto& entry : map) {
    if (entry.second.category == category) {
      ids.push_back(entry.first);
    }
  }
  return ids;
}

// =========================================================================
// Recent commands
// =========================================================================

std::vector<int> AstraCommandDelegate::GetRecentCommands(Profile* profile) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return {};
  }

  const auto& list = prefs->GetList(prefs::kPrefCommandRecentList);
  std::vector<int> result;
  result.reserve(list.size());
  for (const auto& val : list) {
    if (val.is_int()) {
      int id = val.GetInt();
      if (IsAstraCommand(id)) {
        result.push_back(id);
      }
    }
  }
  return result;
}

void AstraCommandDelegate::ClearRecentCommands(Profile* profile) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return;
  }

  const auto& old_list = prefs->GetList(prefs::kPrefCommandRecentList);
  if (old_list.empty()) {
    return;  // No-op if already empty.
  }

  prefs->SetList(prefs::kPrefCommandRecentList, base::Value::List());
  NotifyRecentCommandsChanged();
}

int AstraCommandDelegate::GetMaxRecentCommands(Profile* profile) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return prefs::kDefaultCommandRecentMax;
  }
  int max = prefs->GetInteger(prefs::kPrefCommandRecentMax);
  // Clamp to a reasonable minimum of 1.
  return std::max(1, max);
}

void AstraCommandDelegate::SetMaxRecentCommands(Profile* profile, int max) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return;
  }

  int old_max = prefs->GetInteger(prefs::kPrefCommandRecentMax);
  int clamped_max = std::max(1, max);

  if (old_max == clamped_max) {
    return;  // No change.
  }

  prefs->SetInteger(prefs::kPrefCommandRecentMax, clamped_max);

  // If the new max is smaller, truncate the list.
  if (clamped_max < old_max) {
    const auto& list = prefs->GetList(prefs::kPrefCommandRecentList);
    if (static_cast<int>(list.size()) > clamped_max) {
      base::Value::List truncated;
      truncated.reserve(clamped_max);
      for (int i = 0; i < clamped_max; ++i) {
        truncated.Append(list[i].Clone());
      }
      prefs->SetList(prefs::kPrefCommandRecentList, std::move(truncated));
    }
  }

  NotifyRecentCommandsChanged();
}

// =========================================================================
// Command aliases
// =========================================================================

std::vector<std::string> AstraCommandDelegate::GetCommandAliases(
    Profile* profile,
    int command_id) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return {};
  }

  const auto& dict = prefs->GetDict(prefs::kPrefCommandAliases);
  std::vector<std::string> aliases;

  for (auto [alias, value] : dict) {
    if (value.is_int() && value.GetInt() == command_id) {
      aliases.push_back(alias);
    }
  }
  return aliases;
}

bool AstraCommandDelegate::AddCommandAlias(Profile* profile,
                                           int command_id,
                                           const std::string& alias) {
  if (!IsAstraCommand(command_id) || alias.empty()) {
    return false;
  }

  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return false;
  }

  const base::Value::Dict& dict = prefs->GetDict(prefs::kPrefCommandAliases);

  // Check if the alias is already used by another command.
  const base::Value* existing = dict.Find(alias);
  if (existing && existing->is_int() && existing->GetInt() != command_id) {
    return false;  // Alias already assigned to a different command.
  }

  // If already assigned to this command, no-op.
  if (existing && existing->is_int() && existing->GetInt() == command_id) {
    return true;
  }

  // Add the alias.
  // We need to make a mutable copy since PrefService::GetDict returns const.
  base::Value::Dict mutable_dict = dict.Clone();
  mutable_dict.Set(alias, command_id);
  prefs->SetDict(prefs::kPrefCommandAliases, std::move(mutable_dict));

  NotifyCommandAliasesChanged();
  return true;
}

void AstraCommandDelegate::RemoveCommandAlias(Profile* profile,
                                              const std::string& alias) {
  if (alias.empty()) {
    return;
  }

  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return;
  }

  const base::Value::Dict& dict = prefs->GetDict(prefs::kPrefCommandAliases);
  if (!dict.contains(alias)) {
    return;  // No-op if alias doesn't exist.
  }

  base::Value::Dict mutable_dict = dict.Clone();
  mutable_dict.Remove(alias);
  prefs->SetDict(prefs::kPrefCommandAliases, std::move(mutable_dict));

  NotifyCommandAliasesChanged();
}

int AstraCommandDelegate::GetCommandByAlias(Profile* profile,
                                            const std::string& alias) {
  if (alias.empty()) {
    return -1;
  }

  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return -1;
  }

  const base::Value::Dict& dict = prefs->GetDict(prefs::kPrefCommandAliases);
  const base::Value* value = dict.Find(alias);
  if (!value || !value->is_int()) {
    return -1;
  }

  int command_id = value->GetInt();
  if (!IsAstraCommand(command_id)) {
    return -1;
  }

  return command_id;
}

std::vector<std::string> AstraCommandDelegate::GetAllAliases(Profile* profile) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return {};
  }

  const base::Value::Dict& dict = prefs->GetDict(prefs::kPrefCommandAliases);
  std::vector<std::string> aliases;
  aliases.reserve(dict.size());
  for (auto [alias, value] : dict) {
    if (value.is_int() && IsAstraCommand(value.GetInt())) {
      aliases.push_back(alias);
    }
  }
  return aliases;
}

// =========================================================================
// Observers
// =========================================================================

void AstraCommandDelegate::AddObserver(Observer* observer) {
  GetObservers().AddObserver(observer);
}

void AstraCommandDelegate::RemoveObserver(Observer* observer) {
  GetObservers().RemoveObserver(observer);
}

// =========================================================================
// Internal helpers
// =========================================================================

content::WebContents* AstraCommandDelegate::GetActiveWebContents(
    Browser* browser) {
  if (!browser) {
    return nullptr;
  }
  // TabStripModel owns all WebContents.  We read the active one; we never
  // create, destroy, or reparent WebContents from the command delegate.
  return browser->tab_strip_model()->GetActiveWebContents();
}

AstraWorkspaceService* AstraCommandDelegate::GetWorkspaceService(
    Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  // Obtain the profile-scoped service from the factory.
  // This follows Chromium's ProfileKeyedServiceFactory pattern.
  return AstraWorkspaceServiceFactory::GetForProfile(browser->profile());
}

AstraFavoriteService* AstraCommandDelegate::GetFavoriteService(
    Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  // Obtain the profile-scoped service from the factory.
  // This follows Chromium's ProfileKeyedServiceFactory pattern.
  return AstraFavoriteServiceFactory::GetForProfile(browser->profile());
}

AstraFocusModeService* AstraCommandDelegate::GetFocusModeService(
    Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  // Obtain the profile-scoped service from the factory.
  // This follows Chromium's ProfileKeyedServiceFactory pattern.
  return AstraFocusModeServiceFactory::GetForProfile(browser->profile());
}

AstraScreenshotService* AstraCommandDelegate::GetScreenshotService(
    Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  // Obtain the profile-scoped service from the factory.
  // This follows Chromium's ProfileKeyedServiceFactory pattern.
  return AstraScreenshotServiceFactory::GetForProfile(browser->profile());
}

base::ObserverList<AstraCommandDelegate::Observer>&
AstraCommandDelegate::GetObservers() {
  static base::NoDestructor<base::ObserverList<Observer>> observers;
  return *observers;
}

void AstraCommandDelegate::RecordRecentCommand(Profile* profile,
                                               int command_id) {
  PrefService* prefs = GetPrefsForProfile(profile);
  if (!prefs) {
    return;
  }

  int max_count = GetMaxRecentCommands(profile);

  const auto& old_list = prefs->GetList(prefs::kPrefCommandRecentList);
  base::Value::List new_list;

  // The new command goes to the front (most recent).
  new_list.Append(command_id);

  // Copy existing entries, skipping the command we just added (to move it
  // to the front) and respecting the max count.
  int added = 1;  // We already added the new command.
  for (const auto& val : old_list) {
    if (!val.is_int()) {
      continue;
    }
    int id = val.GetInt();
    if (id == command_id) {
      continue;  // Skip — already at front.
    }
    if (added >= max_count) {
      break;
    }
    new_list.Append(val.Clone());
    ++added;
  }

  prefs->SetList(prefs::kPrefCommandRecentList, std::move(new_list));
  NotifyRecentCommandsChanged();
}

PrefService* AstraCommandDelegate::GetPrefsForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  // Incognito profiles have their own OTR prefs.  We allow recent commands
  // and aliases in incognito — they are local to the incognito session and
  // are not persisted to disk.
  //
  // TODO(astra): Consider whether recent commands should be shared between
  //   regular and incognito profiles, or kept separate.  Currently each
  //   profile (including OTR) has its own list.
  return profile->GetPrefs();
}

void AstraCommandDelegate::NotifyRecentCommandsChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnRecentCommandsChanged();
  }
}

void AstraCommandDelegate::NotifyCommandAliasesChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnCommandAliasesChanged();
  }
}

void AstraCommandDelegate::NotifyCommandExecuted(int command_id) {
  for (auto& observer : GetObservers()) {
    observer.OnCommandExecuted(command_id);
  }
}

}  // namespace astra
