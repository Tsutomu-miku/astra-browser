// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/webui/astra_webui_config.h"

#include "astra/build/buildflags.h"
#include "astra/ui/webui/astra_new_tab_ui.h"
#include "astra/ui/webui/astra_settings_ui.h"
#include "astra/ui/webui/astra_webui_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_config.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ASTRA_BRANDED)

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// AstraNewTabUIConfig — WebUIConfig for astra://newtab
// ---------------------------------------------------------------------------
//
// Tells the content layer that navigating to astra://newtab should create
// an AstraNewTabUI controller and serve new tab page resources.
//
// Chromium pattern: chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.cc
// uses a similar WebUIConfig subclass registration approach.
// TODO(astra): Switch to the newer WebUIConfig::AddWebUIConfig() pattern
// once we confirm the Chromium version supports it.  Some branches still
// use the older WebUIControllerFactory registration path.
// Patch point: ui/webui/webui_config.h

class AstraNewTabUIConfig : public ui::WebUIConfig {
 public:
  AstraNewTabUIConfig();
  ~AstraNewTabUIConfig() override = default;

  // ui::WebUIConfig:
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;

  // Returns true if this config handles URLs with the given host.
  // WebUIConfig already handles this via host() — this is overridden
  // for any additional host-level logic.
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

AstraNewTabUIConfig::AstraNewTabUIConfig()
    : ui::WebUIConfig(kAstraUIScheme, kAstraNewTabHost) {}

std::unique_ptr<content::WebUIController>
AstraNewTabUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                           const GURL& url) {
  return std::make_unique<AstraNewTabUI>(web_ui);
}

bool AstraNewTabUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  // The new tab page is always enabled in Astra-branded builds.
  return true;
}

// ---------------------------------------------------------------------------
// AstraSettingsUIConfig — WebUIConfig for astra://settings
// ---------------------------------------------------------------------------
//
// Tells the content layer that navigating to astra://settings should create
// an AstraSettingsUI controller and serve settings page resources.
//
// TODO(astra): Add enterprise policy controls for settings page visibility.
// Some settings may need to be locked by admin policy.
// Patch point: components/policy/ — add policy-controlled settings visibility.

class AstraSettingsUIConfig : public ui::WebUIConfig {
 public:
  AstraSettingsUIConfig();
  ~AstraSettingsUIConfig() override = default;

  // ui::WebUIConfig:
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

AstraSettingsUIConfig::AstraSettingsUIConfig()
    : ui::WebUIConfig(kAstraUIScheme, kAstraSettingsHost) {}

std::unique_ptr<content::WebUIController>
AstraSettingsUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                             const GURL& url) {
  return std::make_unique<AstraSettingsUI>(web_ui);
}

bool AstraSettingsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  // The settings page is always enabled in Astra-branded builds.
  // TODO(astra): Respect guest / incognito mode restrictions.
  return true;
}

}  // namespace

void RegisterAstraWebUIConfigs() {
  // Register the astra://newtab WebUI.
  ui::WebUIConfig::AddWebUIConfig(
      std::make_unique<AstraNewTabUIConfig>());

  // Register the astra://settings WebUI.
  ui::WebUIConfig::AddWebUIConfig(
      std::make_unique<AstraSettingsUIConfig>());

  // TODO(astra): Add astra://history, astra://downloads, and other
  // Astra WebUI hosts as they are implemented.
}

}  // namespace astra

#endif  // BUILDFLAG(IS_ASTRA_BRANDED)
