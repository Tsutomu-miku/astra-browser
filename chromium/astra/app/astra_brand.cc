// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_brand.h"

#include <string>

#include "astra/app/astra_version.h"
#include "base/strings/stringprintf.h"
#include "chrome/version.h"

namespace astra {

// The constants are defined inline in the header (constexpr) so they can
// be used in compile-time contexts.  This .cc file exists to provide
// out-of-line definitions for cases where a linker needs them (e.g., when
// taking the address of a constexpr variable) and to host non-constexpr
// brand helper functions.
//
// In C++17 and later, inline constexpr variables don't need separate
// definitions.  We include this .cc file for compatibility with Chromium's
// build pattern (every .h has a corresponding .cc).

// ---------------------------------------------------------------------------
// Simple string accessors
// ---------------------------------------------------------------------------
//
// These functions return brand strings as std::string for convenient use
// in non-constexpr contexts.  Prefer the constexpr constants when the
// value is needed at compile time.

std::string GetAstraProductName() {
  return kAstraProductName;
}

std::string GetAstraShortProductName() {
  return kAstraShortProductName;
}

std::string GetAstraVersionString() {
  return kAstraVersionString;
}

std::string GetAstraCompanyName() {
  return kAstraCompanyName;
}

// ---------------------------------------------------------------------------
// Update channel
// ---------------------------------------------------------------------------

std::string GetAstraUpdateChannel() {
  // TODO(astra): Read update channel from build-time brand configuration.
  // In official Chromium builds, the channel comes from brand_config.h and
  // is determined by the branding (chrome, chromium, etc.).
  // For Astra, we should have a similar mechanism based on the build type.
  //
  // For now, return the default dev channel for all non-official builds.
  //
  // Patch point: //chrome/install_static/brand.h
  // Chromium component: install_static::GetChromeChannelName()
  return kAstraDefaultUpdateChannel;
}

// ---------------------------------------------------------------------------
// Combined version string
// ---------------------------------------------------------------------------

std::string GetAstraFullVersionString() {
  // Build a combined version string with both the Astra product version
  // and the Chromium engine version.
  //
  // Format: "Astra Browser X.Y.Z (Chromium A.B.C.D)"
  //
  // This follows Chromium's version display convention but adds the
  // Astra product version on top.
  //
  // Chromium owns: chrome_version_string() — the Blink/V8/network engine
  // version, sourced from chrome/VERSION.
  // Astra owns: kAstraVersionString — the product-level version.
  //
  // TODO(astra): Replace string concatenation with a more efficient
  // construction if this is called frequently.  For now, simple
  // concatenation is fine since this is typically called once per
  // about dialog or --version invocation.
  // Chromium component: base::StringPrintf or base::StrCat
  return base::StringPrintf("%s %s (%s)",
                            kAstraProductName,
                            kAstraVersionString,
                            chrome_version_string());
}

// ---------------------------------------------------------------------------
// User-Agent product token
// ---------------------------------------------------------------------------

std::string GetAstraUserAgentProduct() {
  // Build a User-Agent product token in RFC 7231 format: "Product/version".
  //
  // This token is appended to Chromium's User-Agent string so that
  // web servers can identify Astra browser requests.  Format:
  //
  //   Astra/0.1.0
  //
  // The full User-Agent would look like:
  //
  //   Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36
  //   (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Astra/0.1.0
  //
  // TODO(astra): Consider whether to include the update channel in the
  // User-Agent (e.g., "AstraBeta/0.1.0") for beta/channel builds.
  // Chromium does this with "Chrome", "Chrome Beta", etc. on some platforms.
  //
  // TODO(astra): Wire this into Chromium's User-Agent construction.
  // Patch point: //content/public/common/user_agent.h
  // Patch point: //chrome/common/user_agent.cc
  return base::StringPrintf("%s/%s", kAstraShortProductName, kAstraVersionString);
}

}  // namespace astra
