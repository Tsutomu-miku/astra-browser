// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra URL constants implementation.
//
// Lightweight implementation for the IsAstraWebUI helper function.
// No state, no services — just a list of valid Astra WebUI hosts.

#include "astra/common/astra_url_constants.h"

#include <string>

namespace astra {

namespace {

// Valid host names for Astra WebUI pages.
// Keep this list in sync with the URL constants above.
const char* const kValidAstraWebUIHosts[] = {
    "newtab",
    "settings",
    "history",
    "bookmarks",
    "downloads",
    "workspace-overview",
    "command-palette",
    "focus-mode",
    "notes",
};

constexpr size_t kValidAstraWebUIHostCount =
    std::size(kValidAstraWebUIHosts);

}  // namespace

bool IsAstraWebUI(const GURL& url) {
  if (!IsAstraURL(url))
    return false;

  const std::string& host = url.host();
  if (host.empty())
    return false;

  for (size_t i = 0; i < kValidAstraWebUIHostCount; ++i) {
    if (host == kValidAstraWebUIHosts[i])
      return true;
  }

  return false;
}

}  // namespace astra
