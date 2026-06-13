# ADR-0022: Theme / Color System

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra needs a theming system that integrates with Chromium's native theming while adding Astra-specific color tokens and accent color customization. The sidebar, workspace indicators, split view, focus mode indicator, and other Astra UI elements all need consistent theming.

Key questions:
- Does Astra define its own color system, or extend Chromium's?
- How does dark mode work (follows system, follows Chrome theme)?
- How are accent colors (workspace colors, brand colors) managed?
- How does this interact with Chrome's theme system (browser themes from the Web Store)?

## Decision

Astra **extends Chromium's `ColorProvider` system** with Astra-specific color IDs. All Astra UI uses the same `ColorProvider` mechanism as the rest of Chrome, ensuring consistency with platform theme, dark mode, and Chrome browser themes.

**Chromium subsystems reused:**
- `ui::ColorProvider` -- the central theme/color system in Chromium.
- `ui::NativeTheme` -- platform-level theme (light/dark mode, system colors).
- `ThemeService` / `BrowserThemePack` -- Chrome browser theme support.
- `views::Widget` / `views::View` -- Views framework with native color provider integration.
- `SkColor` / `SkColorSet` -- color types and sets.

**Astra color IDs:**
- Astra-specific color IDs are added in the Astra color ID range (similar to how Chrome adds color IDs on top of UI).
- Color IDs follow the naming pattern `kColorAstra*`.
- Examples: `kColorAstraSidebarBackground`, `kColorAstraSidebarTabSelected`, `kColorAstraSidebarTabHover`, `kColorAstraWorkspaceAccentPrimary`, `kColorAstraFocusModeIndicator`.
- Color IDs are registered with the `ColorProvider` via a color mixer.

**Color mixer pattern:**
- Astra provides a `AddAstraColorMixer(ColorProvider* provider, const ColorProviderKey& key)` function.
- This mixer adds Astra color definitions on top of the existing Chrome color set.
- Colors can reference other colors (e.g., `kColorAstraSidebarBackground` references `kColorToolbarBackground`).
- Light and dark mode variants are defined using `ColorProviderKey`'s color mode.
- The mixer is registered at startup (in `AstraBrowserMainExtraParts` or equivalent).

**Accent colors:**
- Workspace accent colors are user-configurable colors stored in `AstraWorkspaceService`.
- Accent colors are *not* part of the static color provider -- they are dynamic per-workspace colors.
- Astra UI elements that need accent colors read them from the workspace service at render time.
- Derived colors (hover state, pressed state) are computed from the base accent color using color utilities (`SkColor` manipulation).
- A helper utility (`AstraThemeHelpers`) provides derived color computation.

**Dark mode:**
- Follows Chromium's dark mode, which follows the system setting.
- Astra color definitions include both light and dark variants in the color mixer.
- No separate dark mode toggle in Astra -- it inherits from Chrome / system.
- The `ColorProvider` automatically handles dark mode switching; Views automatically repaint when the color provider changes.

**Chrome theme compatibility:**
- Astra colors are defined on top of Chrome's color set.
- If a user installs a Chrome theme (from the Web Store), Astra colors that reference Chrome color IDs automatically adapt.
- Astra-specific accent colors (workspace colors) are independent of Chrome themes.
- The Astra color mixer runs after Chrome's color mixer, so it can override or build on Chrome colors.

**Theme service (AstraThemeService):**
- A lightweight profile-scoped service that manages Astra-specific theme preferences.
- Stores accent color preferences, sidebar density, and other theme-related settings.
- Does NOT replace `ThemeService` -- it is supplementary.
- Persists preferences via `PrefService`.

## Consequences

Positive:

- Consistent with the rest of Chrome's UI. Astra views look native and blend in.
- Automatic dark mode support via `ColorProvider` -- no manual dark mode logic in views.
- Compatible with Chrome Web Store themes -- Astra colors that reference Chrome colors adapt automatically.
- Follows platform conventions (system theme, high contrast, etc.) for free.
- Standard Chromium pattern -- easy for developers familiar with Chromium to understand.
- Color definitions are centralized in the color mixer, not scattered across view files.

Negative:

- Adding Astra-specific color IDs requires integrating with the color provider system, which has some complexity (color mixer registration, color ID ranges).
- Dynamic accent colors (per-workspace) cannot use the static color provider system and must be handled separately.
- If Chromium's color provider API changes, the Astra color mixer may need updates during rebases.
- Chrome themes that radically change colors may make Astra accent colors look bad in some combinations.

Neutral:

- Astra does not provide its own theme store (like Chrome Web Store themes). Astra theming is about color tokens and accent customization, not full browser themes.
- Accent color customization is at the workspace level, not a global "Astra theme."

## Alternatives Considered

### Custom Astra theme system
Build a complete theme/color system from scratch for Astra UI.

- Rejected: Would not integrate with Chrome's native theming, dark mode, or Chrome Web Store themes. Astra UI would feel disconnected from the rest of the browser. Also duplicates significant infrastructure.

### CSS / WebUI-based theming
Build Astra UI as WebUI and use CSS variables for theming.

- Rejected: The primary browser chrome (sidebar, split view, etc.) should use Views, not WebUI, for performance and integration. WebUI is for secondary pages (settings, history), not the primary browser interface.

### Material You / Material Design 3 theming
Adopt Material You / M3 theming with dynamic color.

- Considered as a future direction. Chromium is gradually adopting Material 3, and Astra can follow along. For now, we align with Chromium's current color provider system and will evolve as Chromium does.

## References

- **Chromium subsystems reused:** `ui::ColorProvider`, `ui::NativeTheme`, `ThemeService`, `ColorProviderKey`, `SkColor`, `views::View::GetColorProvider()`
- **Astra components:** `AstraThemeService` (planned), Astra color mixer, `AstraThemeHelpers`
- **Patch points:** Color mixer registration (browser main extra parts, see patch 0001), potential color ID definition file additions
