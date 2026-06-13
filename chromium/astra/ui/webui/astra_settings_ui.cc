// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/webui/astra_settings_ui.h"

#include "astra/app/resources/astra_resources.h"
#include "astra/build/buildflags.h"
#include "astra/ui/webui/astra_settings_handler.h"
#include "astra/ui/webui/astra_webui_constants.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "net/base/mime_util.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/web_ui_util.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// AstraSettingsDataSource — serves settings resources from ResourceBundle
// ---------------------------------------------------------------------------
//
// A content::URLDataSource implementation that serves static settings page
// resources (HTML, CSS, JS) from the compiled resource pack
// (astra_resources.pak).
//
// This mirrors the pattern used by AstraNewTabDataSource but serves the
// settings page assets from astra/app/resources/settings/.
//
// TODO(astra): Consider extracting a shared AstraURLDataSource base class
// or helper if we end up with many WebUI pages with similar resource serving.
// For now each page has its own datasource for clarity.
// Chromium owner: chrome/browser/ui/webui/chrome_url_data_manager.h
class AstraSettingsDataSource : public content::URLDataSource {
 public:
  explicit AstraSettingsDataSource(Profile* profile);
  ~AstraSettingsDataSource() override = default;

  // content::URLDataSource:
  std::string GetSource() override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;
  bool ShouldReplaceExistingSource() override;
  bool ShouldAddContentSecurityPolicy() override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) override;
  bool ShouldServeMimeTypeAsContentTypeHeader() override;

 private:
  raw_ptr<Profile> profile_;
};

AstraSettingsDataSource::AstraSettingsDataSource(Profile* profile)
    : profile_(profile) {}

std::string AstraSettingsDataSource::GetSource() {
  return kAstraSettingsHost;
}

void AstraSettingsDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  // Extract the path from the URL.
  // astra://settings/ -> "" (default to settings.html)
  // astra://settings/settings.css -> "settings.css"
  std::string path = content::URLDataSource::URLToRequestPath(url);
  if (path.empty()) {
    path = "settings.html";
  }

  // Map path to resource ID.
  int resource_id = -1;
  if (path == "settings.html") {
    resource_id = IDR_ASTRA_SETTINGS_HTML;
  } else if (path == "settings.css") {
    resource_id = IDR_ASTRA_SETTINGS_CSS;
  } else if (path == "settings.js") {
    resource_id = IDR_ASTRA_SETTINGS_JS;
  }
  // TODO(astra): Add more resource paths as settings features grow.
  // Consider using a map or generated resource mapping if we end up
  // with many files (e.g., icon assets, sub-page modules).

  scoped_refptr<base::RefCountedMemory> bytes;
  if (resource_id != -1) {
    bytes = ui::ResourceBundle::GetSharedInstance()
                .LoadDataResourceBytes(resource_id);
  }

  // If we didn't find the resource, return an empty response.
  // TODO(astra): Return a 404-style error page for missing resources.
  if (!bytes || bytes->size() == 0) {
    std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  std::move(callback).Run(bytes);
}

std::string AstraSettingsDataSource::GetMimeType(const GURL& url) {
  std::string path = content::URLDataSource::URLToRequestPath(url);
  if (path.empty()) {
    path = "settings.html";
  }

  if (base::EndsWith(path, ".html", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/html";
  }
  if (base::EndsWith(path, ".css", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  }
  if (base::EndsWith(path, ".js", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  }
  return "text/html";
}

bool AstraSettingsDataSource::AllowCaching() {
  // Settings resources are bundled with the browser binary and don't change
  // between launches, so caching is safe and improves load time.
  return true;
}

bool AstraSettingsDataSource::ShouldReplaceExistingSource() {
  // Allow replacing the datasource if one is already registered.
  return true;
}

bool AstraSettingsDataSource::ShouldAddContentSecurityPolicy() {
  return false;
}

std::string AstraSettingsDataSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  // TODO(astra): Define a proper CSP for the settings page.
  // For now, use a permissive policy suitable for development.
  // Production builds should restrict script-src to 'self', img-src to
  // data: and https: for favicons, etc.
  // Chromium reference: chrome/browser/ui/webui/settings/settings_ui.cc
  // has a comprehensive CSP we can adapt.
  switch (directive) {
    case network::mojom::CSPDirectiveName::ScriptSrc:
      return "script-src 'self' 'unsafe-inline';";
    case network::mojom::CSPDirectiveName::StyleSrc:
      return "style-src 'self' 'unsafe-inline';";
    case network::mojom::CSPDirectiveName::ImgSrc:
      return "img-src 'self' data: chrome://theme/ https://*;";
    case network::mojom::CSPDirectiveName::FontSrc:
      return "font-src 'self' data:;";
    case network::mojom::CSPDirectiveName::ConnectSrc:
      return "connect-src 'self';";
    default:
      return content::URLDataSource::GetContentSecurityPolicy(directive);
  }
}

bool AstraSettingsDataSource::ShouldServeMimeTypeAsContentTypeHeader() {
  return true;
}

}  // namespace

AstraSettingsUI::AstraSettingsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  SetupDataSource();
  AddMessageHandlers();
}

AstraSettingsUI::~AstraSettingsUI() = default;

void AstraSettingsUI::SetupDataSource() {
  Profile* profile = Profile::FromWebUI(web_ui());
  content::URLDataSource::Add(
      profile,
      std::make_unique<AstraSettingsDataSource>(profile));
}

void AstraSettingsUI::AddMessageHandlers() {
  // Add the settings message handler that bridges JS to browser services.
  web_ui()->AddMessageHandler(
      std::make_unique<AstraSettingsHandler>());
}

}  // namespace astra
