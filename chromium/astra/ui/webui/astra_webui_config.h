// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra WebUI configuration — registers Astra-specific WebUI hosts.
//
// This file follows Chromium's WebUIConfig pattern (see
// chrome/browser/ui/webui/chrome_web_ui_configs.cc).  Each WebUI host
// (astra://newtab, astra://settings, etc.) registers a WebUIConfig that
// tells the content layer how to create the WebUIController and how to
// serve its resources.
//
// Chromium owns:
//   - WebUIConfig base class (ui/webui/webui_config.h)
//   - URLDataSource system (content/public/browser/url_data_source.h)
//   - WebUIController base class (content/public/browser/web_ui_controller.h)
//
// Astra owns:
//   - Registration of astra:// scheme hosts
//   - Which pages exist and their controllers
//   - Resource serving for each page
//
// TODO(astra): Wire this into Chromium's WebUI config registration.
// Patch point: chrome/browser/ui/webui/chrome_web_ui_configs.cc — add a
// call to RegisterAstraWebUIConfigs() inside the ChromeWebUIConfigs
// constructor, guarded by BUILDFLAG(IS_ASTRA_BRANDED).
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_WEBUI_CONFIG_H_
#define ASTRA_UI_WEBUI_ASTRA_WEBUI_CONFIG_H_

namespace astra {

// Registers all Astra WebUI configs with the content layer.
//
// This should be called once during browser startup, typically from
// ChromeWebUIConfigs or equivalent WebUI configuration code.
//
// After registration, navigating to astra://newtab (and other Astra
// WebUI hosts) will create the appropriate WebUIController and serve
// the page's HTML/CSS/JS resources.
//
// TODO(astra): Add settings and other Astra WebUI hosts as they are
// implemented.  Currently only astra://newtab is registered.
// Patch point: chrome/browser/ui/webui/chrome_web_ui_configs.cc
void RegisterAstraWebUIConfigs();

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_WEBUI_CONFIG_H_
