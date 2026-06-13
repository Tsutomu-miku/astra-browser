// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra Color Mixer — registers Astra-specific colors with the Chromium
// ColorProvider system.
//
// The color mixer is the bridge between Astra's product-specific color
// tokens and Chromium's ColorProvider framework. It is responsible for:
//
//   1. Registering all Astra ColorIds with their default light/dark values.
//   2. Computing accent-color-derived colors (workspace accent variants).
//   3. Responding to theme changes (dark mode toggle, accent color change).
//
// Integration with Chromium:
//
//   AddAstraColorMixer() must be called during ColorProvider construction
//   so that Astra color IDs are registered before they are queried.
//
//   TODO(astra): Integrate into Chromium's color mixer pipeline.
//   The most natural patch point is:
//     - chrome/browser/ui/color/native_chrome_color_mixer.cc
//       (AddNativeChromeColorMixer function)
//     - or chrome/browser/ui/color/chrome_color_mixers.cc
//       (AddChromeColorMixers function)
//
//   Inside the patch, call AddAstraColorMixer() guarded by
//   BUILDFLAG(IS_ASTRA_BRANDED):
//
//     #if BUILDFLAG(IS_ASTRA_BRANDED)
//     #include "astra/ui/color/astra_color_mixer.h"
//     #endif
//
//     // Inside the mixer setup:
//     #if BUILDFLAG(IS_ASTRA_BRANDED)
//     astra::AddAstraColorMixer(provider, dark_mode, accent_color);
//     #endif
//
//   Chromium owner: NativeTheme / ColorProvider
//   (ui/color/color_provider.h)
//   (chrome/browser/ui/color/chrome_color_mixers.h)
//
// Accent color:
//
//   The accent color is a workspace-specific or user-set color that
//   serves as the visual identity of the current workspace. It is used
//   to derive several accent color variants (hover, active, subtle, text).
//
//   The accent color is provided by AstraThemeService, which reads it
//   from the active workspace's metadata (AstraWorkspaceService).
//
//   TODO(astra): Implement AstraThemeService that listens to workspace
//   changes and triggers ColorProvider rebuilds.
//   Astra owner: AstraThemeService (astra/browser/astra_theme_service.h)
//   Patch point: ThemeService / ColorProviderKey in
//   chrome/browser/themes/theme_service.cc

#ifndef ASTRA_UI_COLOR_ASTRA_COLOR_MIXER_H_
#define ASTRA_UI_COLOR_ASTRA_COLOR_MIXER_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"

namespace astra {

// Adds Astra-specific colors to the given ColorProvider.
//
// This function registers all Astra ColorIds with their computed values.
// It should be called once per ColorProvider during construction, after
// Chromium's built-in color mixers have been added so Astra colors can
// reference Chromium colors as fallbacks.
//
// Parameters:
//   provider     - The ColorProvider to register Astra colors with.
//   dark_mode    - Whether the color scheme is dark mode.
//   accent_color - The workspace accent color (e.g., from AstraWorkspaceService).
//                  Used to derive accent color variants.
//
// Usage:
//
//   // During ColorProvider initialization:
//   astra::AddAstraColorMixer(color_provider, is_dark_mode, workspace_accent);
//
//   // In views, query colors via the standard ColorProvider API:
//   SkColor bg = GetColorProvider()->GetColor(kAstraColorSidebarBackground);
//
// TODO(astra): Consider using ColorProviderKey instead of explicit dark_mode
//   and accent_color parameters, to match Chromium's pattern of keying
//   mixers by a typed key object. This would make Astra colors part of
//   the ColorProvider caching system.
//   Chromium pattern: ui/color/color_provider_key.h
void AddAstraColorMixer(ui::ColorProvider* provider,
                        bool dark_mode,
                        SkColor accent_color);

}  // namespace astra

#endif  // ASTRA_UI_COLOR_ASTRA_COLOR_MIXER_H_
