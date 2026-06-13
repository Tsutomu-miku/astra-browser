# ADR-0030: Color System — AstraColorMixer Approach

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

ADR-0022 established that Astra extends Chromium's `ColorProvider` system
with Astra-specific color tokens rather than building a custom theme system.
This ADR documents the specific implementation approach: the `AstraColorMixer`
pattern, how color IDs are managed, how accent colors work, and how the
mixer integrates with Chromium's color pipeline.

Key implementation questions:

1. How are Astra color IDs numbered to avoid collisions with Chromium?
2. What is the API shape of the color mixer function?
3. How do accent colors (per-workspace) integrate with the static
   ColorProvider system?
4. Where does the mixer get registered in Chromium's color pipeline?
5. How are light mode and dark mode variants handled?

## Decision

Astra uses a **single-function color mixer** pattern:
`AddAstraColorMixer(ColorProvider* provider, bool dark_mode, SkColor accent_color)`.
This function registers all Astra color IDs on a `ui::ColorProvider` in one
call, with light/dark variants and accent-color-derived values.

### Color ID Numbering

Astra color IDs start at `kAstraColorsStart = ui::kUiColorsEnd`, extending
Chromium's UI color ID range without overlap. The end marker
`kAstraColorsEnd` reserves a contiguous block.

Naming convention follows Chromium's `kColor*` prefix pattern with "Astra"
in the middle: `kColorAstraSidebarBackground`,
`kColorAstraWorkspaceAccent`, etc.

Color IDs are grouped by feature area with reserved ranges:
- `0-19`: Sidebar colors (background, items, text, sections)
- `20-39`: Workspace / accent colors (primary, hover, active, subtle, text, border)
- `40-59`: Split view colors (divider, drag indicator, background)
- `60-79`: Command palette colors (background, selected, text, border)
- `80-99`: Focus mode colors (indicator, dim overlay)
- `100-127`: Glance / peek colors (border, background, highlight)

This range-based grouping leaves room for growth in each category without
renumbering existing IDs.

### Color Mixer Function

The mixer is a single function, not a class:

```cpp
void AddAstraColorMixer(ui::ColorProvider* provider,
                        bool dark_mode,
                        SkColor accent_color);
```

It calls `provider->SetColor()` for each Astra color ID, choosing light
or dark values based on `dark_mode` and computing accent color variants
from `accent_color`.

Internally, the mixer is organized by feature area with helper functions:
`AddSidebarColors()`, `AddWorkspaceAccentColors()`, `AddSplitViewColors()`,
`AddCommandPaletteColors()`, `AddFocusModeColors()`,
`AddGlancePeekColors()`.

### Light / Dark Mode

Each color that has both light and dark variants is defined as two
compile-time constants (`kSidebarBackgroundLight`, `kSidebarBackgroundDark`)
and selected at mixer call time based on the `dark_mode` parameter.

This follows Chromium's pattern where `ColorProviderKey` includes a color
mode and mixers produce different values for each mode. Astra currently
uses a boolean parameter rather than `ColorProviderKey` for simplicity,
with a `TODO(astra)` to migrate to `ColorProviderKey` for proper caching.

### Accent Color System

Workspaces have an accent color that serves as visual identity. The accent
color is dynamic (changes per workspace), so it is not a static ColorProvider
color in the traditional sense.

The approach is a hybrid:

1. **Accent palette derivation.** The mixer computes a full accent palette
   from a single base color:
   - `kColorAstraWorkspaceAccent` — base accent color
   - `kColorAstraWorkspaceAccentHover` — hover state (brighter in dark mode, darker in light)
   - `kColorAstraWorkspaceAccentActive` — pressed state (darker)
   - `kColorAstraWorkspaceAccentSubtle` — low-alpha background tint
   - `kColorAstraWorkspaceAccentText` — contrast-safe text color (white or black)
   - `kColorAstraWorkspaceAccentBorder` — medium-alpha border

2. **Per-workspace dynamic.** When the active workspace changes, the
   ColorProvider is rebuilt with the new accent color. All Astra views
   repaint automatically via the standard `OnThemeChanged()` mechanism.

3. **Source of truth.** The accent color comes from
   `AstraWorkspaceService` (via `AstraThemeService`), which reads it from
   workspace metadata. The color mixer does not store accent color state.

Helper utilities in the mixer (`LightenColor`, `DarkenColor`,
`RelativeLuminance`, `GetContrastTextColor`, `MakeSubtleAccent`,
`MakeAccentBorder`) compute derived colors programmatically, ensuring
consistent contrast ratios across any base accent color.

### Integration with Chromium's Color Pipeline

The Astra color mixer is injected into Chromium's color mixer pipeline via
a patch point. The natural insertion point is after Chrome's color mixers
so Astra colors can reference Chrome colors as fallbacks.

**Patch point:** `chrome/browser/ui/color/chrome_color_mixers.cc` or
`chrome/browser/ui/color/native_chrome_color_mixer.cc` — call
`AddAstraColorMixer()` inside `#if BUILDFLAG(IS_ASTRA_BRANDED)`.

**Build dependency:** `chrome/browser/ui/color/BUILD.gn` adds a dependency
on `//astra/ui/color` when the Astra build flag is set.

### Querying Colors in Views

Astra views use the standard Chromium pattern:

```cpp
#include "astra/ui/color/astra_color_ids.h"

SkColor bg = GetColorProvider()->GetColor(kColorAstraSidebarBackground);
```

Views automatically receive `OnThemeChanged()` when the ColorProvider
changes (e.g., dark mode toggle, workspace switch), so repainting is
handled by the framework.

## Consequences

Positive:

- **Follows Chromium patterns.** The mixer pattern, color ID naming, and
  `GetColorProvider()` querying are all standard Chromium conventions.
  Developers familiar with Chromium can understand the system immediately.
- **Single point of registration.** All Astra colors are registered in one
  function, making it easy to audit and review the full color set.
- **Automatic theme adaptation.** Colors that reference Chrome color IDs
  adapt automatically when Chrome themes change (e.g., Web Store themes).
- **Accent colors work everywhere.** Since accent colors go through the
  ColorProvider, any view can use them without knowing about workspaces.
- **WCAG contrast safety.** The accent text color is computed from the
  base accent's luminance, ensuring readable text on any accent background.
- **Light/dark mode is free.** Views don't need manual dark mode logic —
  the ColorProvider handles it.

Negative:

- **ColorProvider rebuild on workspace switch.** Changing the active
  workspace triggers a ColorProvider rebuild, which repaints all views.
  This is acceptable for infrequent workspace switches but would be
  expensive if it happened frequently.
- **Accent color is global (per window).** All Astra surfaces in a window
  see the same accent color. Per-workspace-surface accents would need a
  different approach.
- **Color ID range risk.** Using `kUiColorsEnd` as the base assumes
  Chrome colors are also above UI colors. A `TODO(astra)` notes this
  should use `kChromeColorsEnd` for proper layering.
- **Mixer is monolithic.** All colors go through one function. As the
  color set grows, this file may become large. The internal helper
  functions by feature area mitigate this.

Neutral:

- The current implementation uses direct `SetColor()` calls rather than
  Chromium's `AddMixer()` pattern with mixer priorities. This is fine
  for additive-only colors but may need refactoring if Astra ever needs
  to override Chrome colors.
- Color values are currently placeholders based on Chromium's Material
  palette. Final values will come from the design team.

## Alternatives Considered

### Custom Astra theme system (CSS variables, etc.)

Build a complete theme system from scratch with CSS-like variables.

- Rejected: Duplicates Chromium's ColorProvider. Views would not get
  automatic repainting, and dark mode would need custom logic. Inconsistent
  with the rest of the browser UI.

### WebUI / CSS-based theming

Build Astra UI as WebUI and use CSS custom properties for theming.

- Rejected: Primary browser chrome (sidebar, split view) should use Views
  for performance and integration. WebUI is for secondary pages (settings,
  history), not the primary browser interface. See ADR-0011 for the
  sidebar discussion.

### Accent colors as runtime values (not ColorProvider colors)

Skip the ColorProvider for accent colors and have each view read the
accent color directly from `AstraWorkspaceService` at render time.

- Considered and partially used as a fallback pattern. The ColorProvider
  approach is preferred because it integrates with the standard theme
  change notification pipeline. Views that need per-tab or per-item
  accent colors (e.g., workspace list with different colors per item)
  read directly from the service.

### Material You / Material 3 dynamic color

Adopt Material 3 theming with dynamic color extraction.

- Rejected for now: Chromium is still in the process of adopting Material 3.
  Astra follows Chromium's lead on theming frameworks to minimize patch
  surface. This can be revisited when Chromium fully adopts M3.

## References

- **Chromium subsystems reused:** `ui::ColorProvider`, `ui::ColorId`,
  `SkColor`, `views::View::GetColorProvider()`, `ColorProviderKey`
- **Astra components:** `astra/ui/color/astra_color_ids.h`,
  `astra/ui/color/astra_color_mixer.h`, `astra/ui/color/astra_color_mixer.cc`
- **Build target:** `//astra/ui/color` (`chromium/astra/ui/color/BUILD.gn`)
- **Patch point:** `chrome/browser/ui/color/chrome_color_mixers.cc` or
  `native_chrome_color_mixer.cc` (color mixer registration)
- **Related ADRs:** ADR-0022 (Theme / Color System — high-level decision),
  ADR-0010 (Workspace as Metadata Projection — accent color source)
