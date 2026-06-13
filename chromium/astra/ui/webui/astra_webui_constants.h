// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// URL scheme and host constants for Astra WebUI pages.
//
// Astra uses the "astra://" URL scheme for its built-in WebUI pages,
// analogous to how Chromium uses "chrome://".  Each page has a host name
// (e.g. "newtab", "settings") that maps to a WebUIController.
//
// Chromium owns:
//   - WebUI URL scheme registration (content/public/common/url_constants.h)
//   - chrome:// and chrome-untrusted:// schemes
//
// Astra owns:
//   - The "astra://" scheme and all its host names
//   - Which WebUIController serves each host
//
// TODO(astra): Register the "astra" scheme as a WebUI scheme in Chromium's
// URL scheme registry.  This is needed so that content::WebUI and
// content::URLDataSource recognize astra:// URLs as WebUI navigations.
// Patch point: content/public/common/url_constants.cc — add kAstraUIScheme
// to the list of WebUI schemes, or use AddWebUIConfig with a custom scheme.
// Chromium owner: content/browser/webui/web_ui_impl.cc
// =========================================================================

#ifndef ASTRA_UI_WEBUI_ASTRA_WEBUI_CONSTANTS_H_
#define ASTRA_UI_WEBUI_ASTRA_WEBUI_CONSTANTS_H_

namespace astra {

// The URL scheme for Astra WebUI pages.
// Example: astra://newtab
inline constexpr char kAstraUIScheme[] = "astra";

// ---------------------------------------------------------------------------
// Host names
// ---------------------------------------------------------------------------

// Host for the new tab page (astra://newtab).
// This is the WebUI-based alternative to the Views-based new tab bubble.
// Both exist — the WebUI NTP is shown in a regular tab (the "new tab"
// navigation), while the Views bubble appears as an overlay.
inline constexpr char kAstraNewTabHost[] = "newtab";

// Host for Astra settings (astra://settings).
// TODO(astra): Implement astra://settings WebUI page.
// Currently settings use a Views-based bubble (see ui/views/settings/).
// The WebUI settings page will be the long-term canonical settings surface.
// Patch point: astra/ui/webui/astra_settings_ui.h (to be created)
inline constexpr char kAstraSettingsHost[] = "settings";

}  // namespace astra

#endif  // ASTRA_UI_WEBUI_ASTRA_WEBUI_CONSTANTS_H_
