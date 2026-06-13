// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/accessibility/astra_accessibility_strings.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {
namespace accessibility {

// =========================================================================
// Format string helpers (stub implementations)
// =========================================================================
//
// These are stub implementations that concatenate strings manually.
// In the full Chromium build, these will be replaced with calls to
// l10n_util::GetStringFUTF16() which uses format strings from GRD files
// and supports proper pluralization and localization.
//
// TODO(astra): Replace all stub format helpers with proper l10n calls.
//   Chromium component: ui/base/l10n/l10n_util.h
//   Chromium pattern: GetStringFUTF16(IDS_NAME, param1, param2)
//   For plural forms: GetPluralStringFUTF16(IDS_NAME, count)
// =========================================================================

std::u16string GetResultsCountString(int count) {
  // TODO(astra): Use proper pluralization via l10n_util::GetPluralStringFUTF16.
  //   The current stub doesn't handle "1 result" vs "N results".
  // Chromium pattern: ui/base/l10n/l10n_util.h
  std::u16string count_str = base::NumberToString16(count);
  return count_str + u" " + std::u16string(kCommandPaletteResultsCount);
}

std::u16string GetSpaceSwitchedString(const std::u16string& space_name) {
  // TODO(astra): Replace with l10n_util::GetStringFUTF16.
  //   Format: "Switched to {space name} space"
  return u"Switched to " + space_name + u" space";
}

std::u16string GetTabMovedToString(const std::u16string& space_name) {
  // TODO(astra): Replace with l10n_util::GetStringFUTF16.
  //   Format: "Tab moved to {space name}"
  return u"Tab moved to " + space_name;
}

std::u16string GetSplitResizeString(int percent) {
  // TODO(astra): Replace with l10n_util::GetStringFUTF16Int.
  //   Format: "Split at {percent} percent"
  std::u16string percent_str = base::NumberToString16(percent);
  return u"Split at " + percent_str + u" percent";
}

}  // namespace accessibility
}  // namespace astra
