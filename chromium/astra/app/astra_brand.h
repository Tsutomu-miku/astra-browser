// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra product brand constants.
//
// These string constants define Astra's product identity — the name that
// appears in window titles, about dialogs, system menus, crash reports,
// and other Chromium UI surfaces.
//
// Chromium owns: the locations where product names are displayed
// (chrome/common/chrome_constants.cc, chrome/app/theme, etc.).
// Astra owns: the actual string values for the Astra product.
//
// Branding is PRODUCT IDENTITY only — it does not change browser behavior.
// All browser logic (tabs, navigation, profiles, extensions, etc.) is
// owned and implemented by Chromium.
//
// TODO(astra): Wire these constants into Chromium's chrome_constants.cc and
// related branding files so Chromium displays Astra product names instead
// of "Chromium".  See patches/0005-astra-branding.md for patch details.
// Patch point: //chrome/common/chrome_constants.cc
// Patch point: //chrome/browser/about_flags.cc (about:flags branding)

#ifndef ASTRA_APP_ASTRA_BRAND_H_
#define ASTRA_APP_ASTRA_BRAND_H_

#include <string>

#include "astra/app/astra_version.h"

namespace astra {

// ---------------------------------------------------------------------------
// Product names
// ---------------------------------------------------------------------------

// Full product name used in window titles, about dialog, and installer.
// Example: "Astra Browser"
inline constexpr char kAstraProductName[] = "Astra Browser";

// Short product name used in compact contexts (menus, tray, notifications).
// Example: "Astra"
inline constexpr char kAstraShortProductName[] = "Astra";

// Long / legal product name used in legal text and license files.
// Example: "Astra Browser"
inline constexpr char kAstraLongProductName[] = "Astra Browser";

// Company / publisher name.
// TODO(astra): Set final company name when product organization is finalized.
inline constexpr char kAstraCompanyName[] = "Astra";

// ---------------------------------------------------------------------------
// App identifiers
// ---------------------------------------------------------------------------

// macOS bundle ID / Windows App User Model ID / Linux desktop file ID.
// Used for OS-level app identification (notifications, window server, etc.).
// TODO(astra): Finalize bundle ID and coordinate with build signing config.
// Patch point: //chrome/common/chrome_constants.cc (kBrowserProcessExecutableName)
inline constexpr char kAstraAppId[] = "app.astra.browser";

// macOS bundle ID prefix (used for helper apps and extensions).
// Helper apps use IDs like "app.astra.browser.helper".
inline constexpr char kAstraBundleIdPrefix[] = "app.astra.browser";

// Base name for executables and helper processes.
// Chromium uses this for helper app naming (e.g., "Astra Browser Helper").
// TODO(astra): Wire into chrome's kHelperAppBundleIdBase pattern.
// Patch point: //chrome/common/chrome_constants.cc
inline constexpr char kAstraExecutableName[] = "Astra Browser";

// Base name for the browser process executable on disk.
// On Windows: AstraBrowser.exe
// On macOS: Astra Browser.app/Contents/MacOS/Astra Browser
// On Linux: astra-browser
inline constexpr char kAstraBrowserProcessExecutableName[] = "AstraBrowser";

// ---------------------------------------------------------------------------
// Windows-specific identifiers
// ---------------------------------------------------------------------------

// Windows ProgId for file associations and URL protocol handlers.
// Used for registering as the default browser and associating file types.
// Format: <Company>.<Product>.<Version> (following Windows conventions).
// TODO(astra): Finalize ProgId and coordinate with installer.
// Patch point: //chrome/install_static/install_modes.cc
inline constexpr wchar_t kAstraProgId[] = L"Astra.Browser.HTM";

// Windows ProgId for the default browser registration.
// Used by Windows to identify the default browser ProgId.
inline constexpr wchar_t kAstraBrowserProgId[] = L"Astra.Browser.HTM";

// Windows CLSID for the browser COM class.
// Used for COM activation, jump lists, and shell integration.
// TODO(astra): Generate stable GUID for release builds.
// Patch point: //chrome/install_static/install_modes.cc
inline constexpr wchar_t kAstraBrowserClsid[] =
    L"{A1A2A3A4-B5B6-C7C8-D9D0-E1E2E3E4E5E6}";

// Windows App User Model ID (AppUserModelID).
// Used for taskbar grouping and notification identification.
// On Windows 7+, this controls how windows are grouped on the taskbar.
// Patch point: //chrome/browser/shell_integration_win.cc
inline constexpr wchar_t kAstraAppUserModelId[] = L"Astra.Browser";

// ---------------------------------------------------------------------------
// Update channel
// ---------------------------------------------------------------------------

// Update channel identifiers.
// The update channel controls which update track a build follows.
// In Chromium, channels are: stable, beta, dev, canary.
// Astra follows the same channel model.
//
// TODO(astra): Wire update channel into brand_config.h and Omaha update
// configuration for official builds.
// Patch point: //chrome/install_static/brand.h (Chromium brand config)

// Stable channel — general availability, most stable.
inline constexpr char kAstraUpdateChannelStable[] = "stable";

// Beta channel — pre-release, features nearing stable.
inline constexpr char kAstraUpdateChannelBeta[] = "beta";

// Dev channel — developer preview, latest features.
inline constexpr char kAstraUpdateChannelDev[] = "dev";

// Canary channel — nightly builds, bleeding edge.
inline constexpr char kAstraUpdateChannelCanary[] = "canary";

// Default update channel for non-official builds.
// Developer and local builds default to "unknown" / dev.
inline constexpr char kAstraDefaultUpdateChannel[] = "dev";

// Returns the current update channel as a string.
//
// For official builds, this is determined at build time by the branding
// configuration.  For developer builds, returns the default channel.
//
// TODO(astra): Implement build-time channel determination using brand_config
// or GN args.  Currently returns the default dev channel.
// Patch point: //chrome/install_static/brand.h (Chromium channel pattern)
std::string GetAstraUpdateChannel();

// ---------------------------------------------------------------------------
// URLs
// ---------------------------------------------------------------------------

// Product homepage URL.
// Shown in about dialog, welcome page, and update UI.
// TODO(astra): Set final product URL.
inline constexpr char kAstraProductURL[] = "https://astra.app/";

// Feedback / report issue URL.
// Used by the "Report an issue" menu item and crash report UI.
// TODO(astra): Set final feedback URL.
inline constexpr char kAstraFeedbackURL[] = "https://astra.app/feedback";

// Support / help URL.
// Used by the "Help" menu item.
// TODO(astra): Set final support URL / knowledge base.
inline constexpr char kAstraSupportURL[] = "https://astra.app/support";

// Version / changelog URL.
// Shown in the about dialog next to the version number.
// TODO(astra): Set final version / changelog URL.
inline constexpr char kAstraVersionURL[] = "https://astra.app/version";

// ---------------------------------------------------------------------------
// Version info (display purposes)
// ---------------------------------------------------------------------------
//
// Note: actual Astra product version comes from astra_version.h.
// Chromium engine version comes from chrome/version (chrome_version_string).
// The two are independent — Astra has a product version on top of the
// Chromium engine version.

// Returns the product name as a std::string.
//
// Use this in contexts where a std::string is more convenient than a
// constexpr char array.  The returned string is always kAstraProductName.
std::string GetAstraProductName();

// Returns the short product name as a std::string.
std::string GetAstraShortProductName();

// Returns the Astra product version string.
//
// This is a convenience wrapper around kAstraVersionString that returns
// a std::string.  For compile-time use, prefer the constexpr constant.
std::string GetAstraVersionString();

// Returns a combined version string for display, e.g.
// "Astra Browser 0.1.0 (Chromium 131.0.6778.85)"
//
// Use this in about dialogs, --version output, crash reports, etc.
//
// TODO(astra): Use this function in the about dialog and --version output
// via Chromium branding patches.
// Patch point: //chrome/browser/ui/about_flags.cc (about:flags)
// Patch point: //chrome/app/chrome_main_delegate.cc (--version)
// Chromium component: chrome/common/chrome_constants.cc branding strings
std::string GetAstraFullVersionString();

// Returns the product string used in the User-Agent header.
//
// The User-Agent includes an Astra product token to identify Astra
// browsers to web servers.  Format: "Astra/X.Y.Z"
//
// This is appended to Chromium's standard User-Agent string so that
// websites can detect Astra-specific capabilities if needed.
//
// TODO(astra): Wire into chrome/common/user_agent.cc or
// content/public/common/user_agent.h.
// Patch point: //content/public/common/user_agent.h
// Chromium component: content/common/user_agent.cc
std::string GetAstraUserAgentProduct();

// Returns the company name as a std::string.
std::string GetAstraCompanyName();

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_BRAND_H_
