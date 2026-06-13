# 0012 — Color mixer integration for Astra ColorIds

**Patch ID:** 0012
**File:** `chrome/browser/ui/color/chrome_color_mixers.cc`
(alternative: `chrome/browser/ui/color/native_chrome_color_mixer.cc`)
**Size estimate:** ~10 lines
**Status:** planned
**Astra component:** `astra/ui/color/astra_color_mixer.h`, `astra/ui/color/astra_color_ids.h`

## Context

Chromium's ColorProvider system uses a mixer pattern where each subsystem adds its
color IDs and computed values to a shared `ui::ColorProvider`. The mixers are
chained together in `AddChromeColorMixers()`, which is called whenever a
ColorProvider is constructed (e.g., per browser window, per theme change).

Astra defines product-specific color IDs (`kColorAstraSidebarBackground`,
`kColorAstraWorkspaceAccent`, etc.) in `astra/ui/color/astra_color_ids.h` and
implements a color mixer in `astra/ui/color/astra_color_mixer.cc` that registers
all Astra colors with their light/dark and accent-color-derived values.

These Astra colors must be registered with the ColorProvider before Views code
can call `GetColor(kColorAstraSidebarBackground)` on it. A small patch adds the
Astra mixer to Chromium's mixer pipeline.

This cannot be done from `//astra` alone because the list of mixers is
hardcoded in Chromium's color mixer setup function.

## Change

### Before

```cpp
// In chrome/browser/ui/color/chrome_color_mixers.cc
void AddChromeColorMixers(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key) {
  // ... various Chrome mixers ...
  AddNativeChromeColorMixer(provider, key);
  AddMaterialUiColorMixer(provider, key);
  // ... more mixers ...
}
```

### After

Include at the top of the file, guarded by build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/ui/color/astra_color_mixer.h"
#endif
```

Inside the mixer setup function, after Chrome's built-in mixers:

```cpp
void AddChromeColorMixers(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key) {
  // ... various Chrome mixers ...
  AddNativeChromeColorMixer(provider, key);
  AddMaterialUiColorMixer(provider, key);
  // ... more mixers ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra product colors (sidebar, workspace accents, command palette, etc.).
  // Added after Chrome mixers so Astra colors can reference Chrome colors
  // as fallbacks and can override if needed.
  //
  // The accent color comes from the active workspace (AstraWorkspaceService).
  // TODO(astra): Pass accent color through ColorProviderKey or ThemeService.
  //   Currently we use a default accent color; the real accent should come
  //   from the workspace service and trigger a ColorProvider rebuild on change.
  astra::AddAstraColorMixer(provider,
                            /*dark_mode=*/key.color_mode ==
                                ui::ColorProviderKey::ColorMode::kDark,
                            /*accent_color=*/SK_ColorBLUE);
#endif
}
```

**Alternative insertion point:** If `chrome_color_mixers.cc` is not the right
file, `native_chrome_color_mixer.cc` (which handles platform-native theming)
is another candidate. The Astra mixer should run after all Chrome mixers so it
can reference Chrome color tokens.

## Rationale

**Why patch the color mixer pipeline?**
- It is the standard way to add product-specific colors in Chromium.
- All Views code uses `GetColorProvider()->GetColor(kColorId)` consistently.
- Astra colors participate in theme changes, dark mode, and high contrast
  automatically — no custom theme handling needed.
- Color IDs follow Chromium's `kColor*` naming convention and numbering scheme.

**Why not use hardcoded colors?**
- Hardcoded colors don't respond to theme changes or dark mode.
- They don't participate in the ColorProvider caching system.
- They are harder to customize and don't integrate with Chrome's theme system.

**What `//astra` code does this delegate to?**
- `astra/ui/color/astra_color_mixer.h` — `AddAstraColorMixer()` function.
- `astra/ui/color/astra_color_ids.h` — all Astra ColorId constants.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `build/config/chrome_build.gni` (patch 0004)
- All Astra color code and includes are behind this flag.

## Accent color challenge

The Astra color mixer takes an `accent_color` parameter, which comes from
the active workspace (AstraWorkspaceService). Chromium's ColorProvider system
is keyed by a `ColorProviderKey` struct that includes color mode, contrast
mode, user color, etc.

**Options for passing accent color:**

1. **Extend ColorProviderKey** — Add a workspace accent color field. This is
   the cleanest approach but requires a patch to `ui/color/color_provider_key.h`.

2. **Use ThemeService** — Register the accent color through the theme service
   and read it from `ColorProviderKey::user_color`. This leverages existing
   theme infrastructure but may not match Astra's semantics exactly.

3. **Dynamic lookup at render time** — Instead of static ColorProvider colors,
   look up the accent color at paint time from the workspace service. This
   avoids ColorProvider rebuilds but is less performant and loses caching.

**Current recommendation:** Start with option 2 (use user_color / ThemeService)
for initial integration. If workspace switching is frequent and we need
ColorProvider-level caching, move to option 1 (extend ColorProviderKey).

TODO(astra): Finalize accent color integration strategy. Owner: AstraThemeService
or direct ColorProviderKey extension.

## Alternatives Considered

1. **Hardcode Astra colors in views** — Each view computes its own colors.
   Rejected: doesn't respond to theme changes, duplicative, unmaintainable.

2. **Use Chrome color IDs only** — No Astra-specific colors, just reuse Chrome
   tokens. Rejected: Astra has distinct visual identity (sidebar tint, workspace
   accents, command palette styling) that needs dedicated color tokens.

3. **Build a separate theme system** — Astra manages its own theme/color state.
   Rejected: duplicates Chromium's ColorProvider system, high maintenance cost.

4. **Patch NativeTheme instead** — Override colors at the NativeTheme level.
   Rejected: NativeTheme is more about native OS look-and-feel, not product-
   specific colors. ColorProvider is the right layer for product colors.

## Risks & Rebase Concerns

- **Low-to-medium risk.** Chromium's color mixer architecture is relatively
  stable, but the exact function signatures and file organization do change
  between milestones.

- **Accent color integration complexity.** Getting the accent color into the
  ColorProvider pipeline may require additional patches to ColorProviderKey
  or ThemeService. This is the trickiest part of this patch.

- **Graceful degradation:** If the patch fails to apply, Astra views won't
  have access to their color tokens and will fail at compile time (if they
  reference kColorAstra* constants) or runtime (if colors are missing from
  the provider). This is a hard break, not graceful, but the patch is
  small and the break is obvious.

## Related

- ADR: `docs/adr/0022-color-system.md`
- Related patches: 0002 (browser view — uses Astra colors), 0005 (branding)
- Astra source:
  - `astra/ui/color/astra_color_mixer.h`
  - `astra/ui/color/astra_color_ids.h`
  - `astra/ui/color/astra_color_mixer.cc`
