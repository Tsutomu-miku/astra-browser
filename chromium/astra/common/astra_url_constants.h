// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// URL scheme and path constants used across Astra.
//
// Astra uses a custom "astra" URL scheme for its WebUI surfaces (new tab page,
// settings, workspace overview, etc.).  These constants centralize the scheme
// and URL definitions so that app, browser, and ui layers can reference them
// without duplicating string literals.
//
// Chromium subsystems reused:
//   - url/gurl.h for GURL type and URL parsing
//   - chrome/common/webui_url_constants.h for the WebUI URL pattern
//
// TODO(astra): Register the astra:// scheme as a WebUI scheme via a Chromium
//   patch to chrome/common/url_constants.cc and chrome/browser/ui/webui/
//   chrome_web_ui_configs.cc.  Chromium owner: WebUI team.
//   Patch point: chrome/common/webui_url_constants.{h,cc}.

#ifndef ASTRA_COMMON_ASTRA_URL_CONSTANTS_H_
#define ASTRA_COMMON_ASTRA_URL_CONSTANTS_H_

#include <string>

#include "url/gurl.h"

namespace astra {

// =========================================================================
// URL scheme
// =========================================================================

// The custom URL scheme for Astra WebUI pages.
// Pages under this scheme are served by Astra's WebUI data sources.
inline constexpr char kAstraUIScheme[] = "astra";

// =========================================================================
// Astra WebUI URLs
// =========================================================================

// New Tab Page — shown when a new tab is opened.
inline constexpr char kAstraNewTabURL[] = "astra://newtab";

// Settings page — Astra-specific settings and preferences.
inline constexpr char kAstraSettingsURL[] = "astra://settings";

// History page — Astra's browsing history view.
inline constexpr char kAstraHistoryURL[] = "astra://history";

// Bookmarks page — Astra's bookmark manager view.
inline constexpr char kAstraBookmarksURL[] = "astra://bookmarks";

// Downloads page — Astra's download manager view.
inline constexpr char kAstraDownloadsURL[] = "astra://downloads";

// Workspace overview — visual overview of all workspaces.
inline constexpr char kAstraWorkspaceOverviewURL[] =
    "astra://workspace-overview";

// Command palette — quick command and tab search interface.
inline constexpr char kAstraCommandPaletteURL[] = "astra://command-palette";

// Focus mode — distraction-free browsing mode page.
inline constexpr char kAstraFocusModeURL[] = "astra://focus-mode";

// Notes page — Astra's built-in notes interface.
inline constexpr char kAstraNotesURL[] = "astra://notes";

// =========================================================================
// Helper functions
// =========================================================================

// Returns true if |url| uses the "astra" URL scheme.
// This is a lightweight scheme check — it does not verify that the host
// corresponds to a known Astra WebUI page.
inline bool IsAstraURL(const GURL& url) {
  return url.SchemeIs(kAstraUIScheme);
}

// Returns true if |url| is a valid Astra WebUI URL.
// A valid Astra WebUI URL must:
//   - Use the "astra" scheme.
//   - Have a non-empty host.
//   - Correspond to a known Astra WebUI page.
//
// This is useful for security checks (e.g., "is this URL allowed to be
// loaded in a trusted WebUI context?") and for command routing.
bool IsAstraWebUI(const GURL& url);

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_URL_CONSTANTS_H_
