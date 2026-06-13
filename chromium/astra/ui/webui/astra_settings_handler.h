// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Message handler for the Astra settings page.
//
// Bridges JavaScript on the astra://settings page to browser-side services
// and preferences.  The settings JS sends messages like "getAllSettings",
// "setPreference", etc., and this handler dispatches them to the appropriate
// Astra services or Chromium PrefService.
//
// Chromium pattern: content::WebUIMessageHandler
//   - RegisterMessageCallback("messageName", base::BindRepeating(...))
//   - AllowJavascript() / CallJavascriptFunction() for JS-side responses
//   - OnJavascriptDisallowed() for cleanup
//
// Chromium subsystems reused:
//   - content::WebUIMessageHandler — base class
//   - PrefService — preference reading/writing
//   - AstraWorkspaceService — workspace metadata and management
//   - AstraFocusModeService — focus mode settings and state
//
// Truth model:
//   - This handler is a pure bridge — it stores no state.
//   - All preference data comes from PrefService (via AstraPrefService).
//   - Workspace data comes from AstraWorkspaceService.
//   - UI state is owned by the JS side of the settings page.
//
// TODO(astra): Add observer-based push updates when preferences change.
//   Currently the JS side fetches on load; we should subscribe to
//   PrefChangeRegistrar and CallJavascriptFunction() on changes.
// Chromium owner: chrome/browser/ui/webui/settings/settings_page_ui_handler.cc
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_SETTINGS_HANDLER_H_
#define ASTRA_UI_WEBUI_ASTRA_SETTINGS_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class Value;
class Value::List;
}  // namespace base

namespace astra {

// WebUIMessageHandler for the Astra settings page.
//
// Registered messages (JS -> C++):
//   - "getAllSettings" — returns all Astra settings as a nested object
//   - "getGeneralSettings" — returns general settings (startup, search)
//   - "setGeneralSettings" — updates general settings
//   - "getSidebarSettings" — returns sidebar settings (position, width, auto-hide)
//   - "setSidebarSettings" — updates sidebar settings
//   - "getWorkspaceSettings" — returns workspace settings (default, accent colors)
//   - "setWorkspaceSettings" — updates workspace settings
//   - "getFocusModeSettings" — returns focus mode settings (duration, blocklist)
//   - "setFocusModeSettings" — updates focus mode settings
//   - "getAppearanceSettings" — returns appearance settings (theme, accent)
//   - "setAppearanceSettings" — updates appearance settings
//   - "getWorkspaces" — returns list of all workspaces (for workspace management)
//   - "createWorkspace" — creates a new workspace
//   - "deleteWorkspace" — deletes a workspace by id
//   - "renameWorkspace" — renames a workspace
//   - "setDefaultWorkspace" — sets the default workspace
//   - "getRecentlyClosed" — returns recently closed tabs/windows
//
// TODO(astra): Add more settings categories as features grow
// (extensions, privacy, keyboard shortcuts, etc.).
class AstraSettingsHandler : public content::WebUIMessageHandler {
 public:
  AstraSettingsHandler();
  ~AstraSettingsHandler() override;

  AstraSettingsHandler(const AstraSettingsHandler&) = delete;
  AstraSettingsHandler& operator=(const AstraSettingsHandler&) = delete;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;
  void OnJavascriptDisallowed() override;

 private:
  // -- Message handlers ---------------------------------------------------

  // Handles "getAllSettings" message.
  // Args: none
  // Returns: object with all settings sections {general, sidebar, workspaces,
  //          focusMode, appearance}
  void HandleGetAllSettings(const base::Value::List& args);

  // Handles "getGeneralSettings" message.
  // Args: none
  // Returns: {startupBehavior, defaultSearchEngine}
  void HandleGetGeneralSettings(const base::Value::List& args);

  // Handles "setGeneralSettings" message.
  // Args: [{startupBehavior?, defaultSearchEngine?}]
  // Returns: boolean success
  void HandleSetGeneralSettings(const base::Value::List& args);

  // Handles "getSidebarSettings" message.
  // Args: none
  // Returns: {position, width, autoHide, enabled}
  void HandleGetSidebarSettings(const base::Value::List& args);

  // Handles "setSidebarSettings" message.
  // Args: [{position?, width?, autoHide?, enabled?}]
  // Returns: boolean success
  void HandleSetSidebarSettings(const base::Value::List& args);

  // Handles "getWorkspaceSettings" message.
  // Args: none
  // Returns: {defaultWorkspaceId, accentColorPresets: [...]}
  void HandleGetWorkspaceSettings(const base::Value::List& args);

  // Handles "setWorkspaceSettings" message.
  // Args: [{defaultWorkspaceId?}]
  // Returns: boolean success
  void HandleSetWorkspaceSettings(const base::Value::List& args);

  // Handles "getFocusModeSettings" message.
  // Args: none
  // Returns: {defaultDurationMinutes, blocklist: [...], soundEnabled,
  //          autoStartWorkspaces: [...]}
  void HandleGetFocusModeSettings(const base::Value::List& args);

  // Handles "setFocusModeSettings" message.
  // Args: [{defaultDurationMinutes?, blocklist?, soundEnabled?}]
  // Returns: boolean success
  void HandleSetFocusModeSettings(const base::Value::List& args);

  // Handles "getAppearanceSettings" message.
  // Args: none
  // Returns: {theme, accentColor, useSystemTheme, compactMode}
  void HandleGetAppearanceSettings(const base::Value::List& args);

  // Handles "setAppearanceSettings" message.
  // Args: [{theme?, accentColor?, useSystemTheme?, compactMode?}]
  // Returns: boolean success
  void HandleSetAppearanceSettings(const base::Value::List& args);

  // Handles "getWorkspaces" message.
  // Args: none
  // Returns: array of workspace objects {id, name, accentColor, isDefault, tabCount}
  void HandleGetWorkspaces(const base::Value::List& args);

  // Handles "createWorkspace" message.
  // Args: [name: string, accentColor: string (optional)]
  // Returns: workspace object {id, name, accentColor, ...}
  void HandleCreateWorkspace(const base::Value::List& args);

  // Handles "deleteWorkspace" message.
  // Args: [workspaceId: string]
  // Returns: boolean success
  void HandleDeleteWorkspace(const base::Value::List& args);

  // Handles "renameWorkspace" message.
  // Args: [workspaceId: string, newName: string]
  // Returns: boolean success
  void HandleRenameWorkspace(const base::Value::List& args);

  // Handles "setDefaultWorkspace" message.
  // Args: [workspaceId: string]
  // Returns: boolean success
  void HandleSetDefaultWorkspace(const base::Value::List& args);

  // Handles "getRecentlyClosed" message.
  // Args: [count: number (optional, default 10)]
  // Returns: array of recently closed items {title, url, closedTime, type}
  void HandleGetRecentlyClosed(const base::Value::List& args);

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

  // Builds the full settings object (used by getAllSettings).
  base::Value::Dict BuildAllSettingsDict(Profile* profile);

  // Builds individual settings section objects.
  // These read from PrefService / Astra services and return Value dicts.
  base::Value::Dict BuildGeneralSettingsDict(Profile* profile);
  base::Value::Dict BuildSidebarSettingsDict(Profile* profile);
  base::Value::Dict BuildWorkspaceSettingsDict(Profile* profile);
  base::Value::Dict BuildFocusModeSettingsDict(Profile* profile);
  base::Value::Dict BuildAppearanceSettingsDict(Profile* profile);

  // Applies a partial settings dict to the appropriate service/prefs.
  // Returns true on success.
  // TODO(astra): These currently return mock/default values.
  // Wire up to actual PrefService / Astra services once they exist.
  bool ApplyGeneralSettings(Profile* profile, const base::Value::Dict& dict);
  bool ApplySidebarSettings(Profile* profile, const base::Value::Dict& dict);
  bool ApplyWorkspaceSettings(Profile* profile, const base::Value::Dict& dict);
  bool ApplyFocusModeSettings(Profile* profile, const base::Value::Dict& dict);
  bool ApplyAppearanceSettings(Profile* profile, const base::Value::Dict& dict);

  base::WeakPtrFactory<AstraSettingsHandler> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_SETTINGS_HANDLER_H_
