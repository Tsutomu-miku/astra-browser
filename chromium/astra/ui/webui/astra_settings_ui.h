// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra Settings WebUI controller.
//
// Handles the astra://settings WebUI page.  This is the primary settings
// surface for Astra-specific preferences that complement Chromium's built-in
// settings (chrome://settings).
//
// Chromium subsystems reused:
//   - content::WebUIController — base class for WebUI page controllers
//   - content::URLDataSource — serves HTML/CSS/JS resources from .pak files
//   - content::WebUIMessageHandler — bridge between C++ and JS
//   - PrefService — preference storage (via AstraPrefService / Chromium prefs)
//
// Settings sections:
//   - General: startup behavior, default search engine
//   - Sidebar: position, width, auto-hide
//   - Workspaces: accent colors, default workspace
//   - Focus mode: default duration, blocklist
//   - Appearance: theme, accent color
//
// Truth model:
//   - All preference data is stored in PrefService (Chromium-owned).
//   - Workspace data comes from AstraWorkspaceService.
//   - This controller is a pure presentation layer — it does not store state.
//
// TODO(astra): Wire this into the settings entry points (menu, keyboard
//   shortcut, etc.).  Currently only accessible via direct URL navigation.
// Patch point: chrome/browser/ui/browser_command_controller.cc — add a
//   command for Astra settings that navigates to astra://settings.
// Chromium owner: chrome/browser/ui/webui/settings/settings_ui.cc
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_SETTINGS_UI_H_
#define ASTRA_UI_WEBUI_ASTRA_SETTINGS_UI_H_

#include <memory>

#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}  // namespace content

namespace astra {

// WebUIController for astra://settings.
//
// Responsibilities:
//   - Registers the URLDataSource that serves settings HTML/CSS/JS resources
//   - Creates WebUIMessageHandler(s) for JS <-> C++ communication
//   - Bridges to browser services (AstraPrefService, AstraWorkspaceService, etc.)
//
// The actual page content lives in astra/app/resources/settings/ and is
// compiled into the astra_resources.pak file by GRIT.
class AstraSettingsUI : public content::WebUIController {
 public:
  explicit AstraSettingsUI(content::WebUI* web_ui);
  ~AstraSettingsUI() override;

  AstraSettingsUI(const AstraSettingsUI&) = delete;
  AstraSettingsUI& operator=(const AstraSettingsUI&) = delete;

  // content::WebUIController:
  // The default implementation is sufficient for most cases.

 private:
  // Registers the URLDataSource that serves settings static resources.
  // Resources are loaded from the astra_resources.pak file via
  // content::URLDataSource.
  //
  // TODO(astra): Use ChromeURLDataSource / ChromeWebUIProvider patterns
  // once we integrate more deeply with chrome/browser/ui/webui.
  // For now we use a basic content::URLDataSource, mirroring the NTP pattern.
  void SetupDataSource();

  // Adds message handlers for JS <-> C++ communication.
  // The settings page's JS sends messages like "getAllSettings",
  // "setPreference", etc., which are handled by AstraSettingsHandler.
  void AddMessageHandlers();
};

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_SETTINGS_UI_H_
