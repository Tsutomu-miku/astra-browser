// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra New Tab Page WebUI controller.
//
// Handles the astra://newtab WebUI page.  This is the WebUI-based
// alternative to the Views-based new tab bubble (astra/ui/views/newtab/).
// Both surfaces exist — the WebUI NTP renders inside a regular tab
// (navigated to astra://newtab), while the Views bubble is an overlay.
//
// Chromium subsystems reused:
//   - content::WebUIController — base class for WebUI page controllers
//   - content::URLDataSource — serves HTML/CSS/JS resources from .pak files
//   - content::WebUIMessageHandler — bridge between C++ and JS
//   - TopSites / HistoryService — real shortcut data (via AstraNewTabPageService)
//
// Truth model:
//   - Shortcut data comes from Chromium's TopSites (via AstraNewTabPageService)
//   - Workspace data comes from AstraWorkspaceService (via AstraNewTabPageService)
//   - This controller is a pure presentation layer — it does not store state
//
// TODO(astra): Wire this controller as the default new tab page URL.
// Currently Chromium uses chrome://newtab by default.
// Patch point: chrome/browser/ui/browser_navigator.cc or
//   chrome/common/webui_url_constants.cc — override kChromeUINewTabURL
//   to use astra://newtab in Astra-branded builds.
// Chromium owner: chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.cc
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_NEW_TAB_UI_H_
#define ASTRA_UI_WEBUI_ASTRA_NEW_TAB_UI_H_

#include <memory>

#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}  // namespace content

namespace astra {

// WebUIController for astra://newtab.
//
// Responsibilities:
//   - Registers the URLDataSource that serves NTP HTML/CSS/JS resources
//   - Creates WebUIMessageHandler(s) for JS <-> C++ communication
//   - Bridges to browser services (AstraNewTabPageService, etc.)
//
// The actual page content lives in astra/app/resources/newtab/ and is
// compiled into the astra_resources.pak file by GRIT.
class AstraNewTabUI : public content::WebUIController {
 public:
  explicit AstraNewTabUI(content::WebUI* web_ui);
  ~AstraNewTabUI() override;

  AstraNewTabUI(const AstraNewTabUI&) = delete;
  AstraNewTabUI& operator=(const AstraNewTabUI&) = delete;

  // content::WebUIController:
  // The default implementation is sufficient for most cases.
  // Override if we need custom handling for specific WebUI features.

 private:
  // Registers the URLDataSource that serves NTP static resources.
  // Resources are loaded from the astra_resources.pak file via
  // ChromeURLDataSource or a custom content::URLDataSource.
  //
  // TODO(astra): Use ChromeURLDataSource / ChromeWebUIProvider patterns
  // once we integrate more deeply with chrome/browser/ui/webui.
  // For now we use a basic content::URLDataSource.
  void SetupDataSource();

  // Adds message handlers for JS <-> C++ communication.
  // The NTP page's JS sends messages like "getWorkspaces", "openWorkspace",
  // etc., which are handled by AstraNewTabHandler.
  void AddMessageHandlers();
};

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_NEW_TAB_UI_H_
