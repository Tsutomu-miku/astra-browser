// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra workspace type helpers.
//
// Lightweight implementations for color lookup and ID validation functions.
// No state, no services — just curated color palette mappings and
// format validation.

#include "astra/common/astra_workspace_types.h"

#include <cctype>
#include <string>

#include "third_party/skia/include/core/SkColor.h"

namespace astra {

// =========================================================================
// Legacy accent color lookup
// =========================================================================

SkColor GetAstraAccentColor(AstraWorkspaceAccentColor color) {
  switch (color) {
    case AstraWorkspaceAccentColor::kBlue:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Google Blue 500
    case AstraWorkspaceAccentColor::kGreen:
      return SkColorSetRGB(0x18, 0x80, 0x38);  // Google Green 600
    case AstraWorkspaceAccentColor::kPurple:
      return SkColorSetRGB(0x84, 0x33, 0xFF);  // Google Purple 500
    case AstraWorkspaceAccentColor::kOrange:
      return SkColorSetRGB(0xFA, 0x90, 0x3E);  // Google Orange 500
    case AstraWorkspaceAccentColor::kPink:
      return SkColorSetRGB(0xF0, 0x42, 0x7E);  // Google Pink 500
    case AstraWorkspaceAccentColor::kRed:
      return SkColorSetRGB(0xD9, 0x30, 0x25);  // Google Red 600
    case AstraWorkspaceAccentColor::kTeal:
      return SkColorSetRGB(0x00, 0x7B, 0x83);  // Google Teal 600
    case AstraWorkspaceAccentColor::kYellow:
      return SkColorSetRGB(0xF9, 0xAB, 0x00);  // Google Yellow 600
    case AstraWorkspaceAccentColor::kGrey:
      return SkColorSetRGB(0x5F, 0x63, 0x68);  // Google Grey 600
    case AstraWorkspaceAccentColor::kCustom:
      // Fallback — callers should use the custom_color field instead.
      return SkColorSetRGB(0x1A, 0x73, 0xE8);
  }
  // Fallthrough safety.
  return SkColorSetRGB(0x1A, 0x73, 0xE8);
}

// =========================================================================
// Workspace 12-color palette lookup
// =========================================================================

SkColor AccentColorForWorkspaceColor(AstraWorkspaceColor color) {
  switch (color) {
    case AstraWorkspaceColor::kGray:
      return SkColorSetRGB(0x5F, 0x63, 0x68);  // Google Grey 600
    case AstraWorkspaceColor::kBlue:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Google Blue 500
    case AstraWorkspaceColor::kRed:
      return SkColorSetRGB(0xD9, 0x30, 0x25);  // Google Red 600
    case AstraWorkspaceColor::kGreen:
      return SkColorSetRGB(0x18, 0x80, 0x38);  // Google Green 600
    case AstraWorkspaceColor::kYellow:
      return SkColorSetRGB(0xF9, 0xAB, 0x00);  // Google Yellow 600
    case AstraWorkspaceColor::kPurple:
      return SkColorSetRGB(0x84, 0x33, 0xFF);  // Google Purple 500
    case AstraWorkspaceColor::kPink:
      return SkColorSetRGB(0xF0, 0x42, 0x7E);  // Google Pink 500
    case AstraWorkspaceColor::kCyan:
      return SkColorSetRGB(0x00, 0x7B, 0x83);  // Google Teal 600 (cyan-ish)
    case AstraWorkspaceColor::kOrange:
      return SkColorSetRGB(0xFA, 0x90, 0x3E);  // Google Orange 500
    case AstraWorkspaceColor::kTeal:
      return SkColorSetRGB(0x00, 0x69, 0x5C);  // Google Darker Teal
    case AstraWorkspaceColor::kIndigo:
      return SkColorSetRGB(0x3F, 0x51, 0xB5);  // Indigo 600
    case AstraWorkspaceColor::kBrown:
      return SkColorSetRGB(0x6D, 0x4C, 0x41);  // Brown 700
  }
  // Fallthrough safety — return blue as default.
  return SkColorSetRGB(0x1A, 0x73, 0xE8);
}

// =========================================================================
// Workspace ID validation
// =========================================================================

bool IsValidWorkspaceId(const std::string& id) {
  if (id.empty())
    return false;

  if (id.size() > kAstraMaxWorkspaceIdLength)
    return false;

  for (char c : id) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
      return false;
    }
  }

  return true;
}

}  // namespace astra
