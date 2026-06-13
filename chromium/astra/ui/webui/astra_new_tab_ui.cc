// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/webui/astra_new_tab_ui.h"

#include "astra/app/resources/astra_resources.h"
#include "astra/build/buildflags.h"
#include "astra/ui/webui/astra_new_tab_handler.h"
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
// AstraNewTabDataSource — serves NTP resources from ResourceBundle
// ---------------------------------------------------------------------------
//
// A content::URLDataSource implementation that serves static NTP resources
// (HTML, CSS, JS) from the compiled resource pack (astra_resources.pak).
//
// Chromium pattern: many chrome:// pages use ChromeURLDataSource which
// automatically maps paths to resource IDs.  We implement our own here
// because Astra resources live in a separate .pak file.
//
// TODO(astra): Consider using chrome/browser/ui/webui/chrome_url_data_manager
// or a similar shared helper if/when we have more WebUI pages.  For now
// a simple datasource per page is fine.
// Chromium owner: chrome/browser/ui/webui/chrome_url_data_manager.h
class AstraNewTabDataSource : public content::URLDataSource {
 public:
  explicit AstraNewTabDataSource(Profile* profile);
  ~AstraNewTabDataSource() override = default;

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

AstraNewTabDataSource::AstraNewTabDataSource(Profile* profile)
    : profile_(profile) {}

std::string AstraNewTabDataSource::GetSource() {
  return kAstraNewTabHost;
}

void AstraNewTabDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  // Extract the path from the URL.
  // astra://newtab/ -> "" (default to newtab.html)
  // astra://newtab/newtab.css -> "newtab.css"
  std::string path = content::URLDataSource::URLToRequestPath(url);
  if (path.empty()) {
    path = "newtab.html";
  }

  // Map path to resource ID.
  int resource_id = -1;
  if (path == "newtab.html") {
    resource_id = IDR_ASTRA_NEW_TAB_HTML;
  } else if (path == "newtab.css") {
    resource_id = IDR_ASTRA_NEW_TAB_CSS;
  } else if (path == "newtab.js") {
    resource_id = IDR_ASTRA_NEW_TAB_JS;
  }
  // TODO(astra): Add more resource paths as NTP features grow.
  // Consider using a map or a generated resource mapping if we end up
  // with many files.  For now a simple if/else chain is clearer.

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

std::string AstraNewTabDataSource::GetMimeType(const GURL& url) {
  std::string path = content::URLDataSource::URLToRequestPath(url);
  if (path.empty()) {
    path = "newtab.html";
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
  // Default fallback — Chromium's URLDataSource uses text/html by default.
  return "text/html";
}

bool AstraNewTabDataSource::AllowCaching() {
  // NTP resources are bundled with the browser binary and don't change
  // between launches, so caching is safe and improves load time.
  return true;
}

bool AstraNewTabDataSource::ShouldReplaceExistingSource() {
  // Allow replacing the datasource if one is already registered (e.g.
  // during tests or if registration runs twice).
  return true;
}

bool AstraNewTabDataSource::ShouldAddContentSecurityPolicy() {
  // We add our own CSP in GetContentSecurityPolicy, so we don't need the
  // default one from URLDataSource.
  return false;
}

std::string AstraNewTabDataSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  // TODO(astra): Define a proper CSP for the NTP.
  // For now, use a permissive policy suitable for development.
  // Production builds should restrict script-src to 'self', img-src to
  // data: and https: for favicons, etc.
  // Chromium reference: chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.cc
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

bool AstraNewTabDataSource::ShouldServeMimeTypeAsContentTypeHeader() {
  return true;
}

}  // namespace

AstraNewTabUI::AstraNewTabUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  SetupDataSource();
  AddMessageHandlers();
}

AstraNewTabUI::~AstraNewTabUI() = default;

void AstraNewTabUI::SetupDataSource() {
  Profile* profile = Profile::FromWebUI(web_ui());
  content::URLDataSource::Add(
      profile,
      std::make_unique<AstraNewTabDataSource>(profile));
}

void AstraNewTabUI::AddMessageHandlers() {
  // Add the NTP message handler that bridges JS to browser services.
  web_ui()->AddMessageHandler(
      std::make_unique<AstraNewTabHandler>());
}

}  // namespace astra
