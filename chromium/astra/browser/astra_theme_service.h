// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra Theme Service — profile-scoped service that manages Astra's theme
// and accent color state.
//
// This service bridges Chromium's ThemeService with Astra's workspace
// accent colors and the AstraColorMixer.  It is the single source of
// truth for the current accent color and dark mode state used by
// Astra UI surfaces.
//
// Truth source for:
//   - Active workspace accent color (SkColor).
//   - Which workspace's accent is currently active.
//   - Dark / light mode state (projection from Chromium).
//
// Chromium subsystems reused:
//   - ThemeService (chrome/browser/themes/) for system theme state.
//   - NativeTheme (ui/native_theme/) for dark/light mode detection.
//   - ProfileKeyedServiceFactory for service lifetime.
//   - SkColor / SkBlendMode for color operations.
//
// Astra metadata owned:
//   - Active accent color (per workspace, or user override).
//   - Theme observer notification.
//
// Patch points:
//   - Color mixer registration (patch 0012) connects to this service.
//   - BrowserView theme change notification calls OnThemeChanged().
//   - Workspace switch triggers accent color update.
//
// TODO(astra): Wire this service into the Chromium ColorProvider
//   refresh pipeline so that accent color changes trigger a
//   ColorProvider rebuild and all Astra UI surfaces update automatically.
//   Chromium owner: ColorProvider (ui/color/color_provider.h)
//   Patch point: chrome/browser/themes/theme_service.cc — OnThemeChanged
//   notification should trigger AstraColorMixer re-registration.

#ifndef ASTRA_BROWSER_ASTRA_THEME_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_THEME_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/skia/include/core/SkColor.h"

class PrefRegistrySimple;
class PrefService;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraThemeService;
class AstraWorkspaceService;

// ---------------------------------------------------------------------------
// Theme preset enum
// ---------------------------------------------------------------------------

// Preset theme modes available in Astra.
//
// These complement Chromium's native theme system with Astra-specific
// behavior.  The preset controls how the light/dark mode is chosen:
//   - kSystem: follow the system/OS theme setting (delegates to Chromium)
//   - kLight:  force light mode
//   - kDark:   force dark mode
//   - kAuto:   auto-switch based on time of day (future enhancement)
//
// Chromium analog: ThemeService::UsingSystemTheme(), browser_theme_utils.
//   Astra adds presets as a higher-level user-facing concept.
enum class AstraThemePreset {
  kSystem = 0,
  kLight = 1,
  kDark = 2,
  kAuto = 3,
};

// ---------------------------------------------------------------------------
// Factory for AstraThemeService
// ---------------------------------------------------------------------------

// ProfileKeyedServiceFactory for AstraThemeService.
//
// Creates and manages AstraThemeService instances, one per profile.
//
// Incognito behavior: kRedirectedToOriginal — theme state is shared
// between regular and incognito profiles because theme is a user-level
// preference, not a browsing-context preference.
//
// System profile: kNone — theme service is not needed for system profiles.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
class AstraThemeServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraThemeService instance for |profile|.
  // Returns nullptr for system profiles or other contexts where the
  // service is not available.
  static AstraThemeService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraThemeServiceFactory* GetInstance();

  // Registers theme-related profile prefs on the given registry.
  //
  // Registers presets, accent override, and other theme-specific prefs.
  //
  // Chromium component: PrefRegistry / PrefService.
  // Patch point: called from factory registration or from a browser prefs
  // registration hook (e.g. chrome/browser/prefs/browser_prefs.cc).
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraThemeServiceFactory>;

  AstraThemeServiceFactory();
  ~AstraThemeServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

// ---------------------------------------------------------------------------
// Observer interface
// ---------------------------------------------------------------------------

// Observer interface for AstraThemeService.
//
// UI layers (sidebar, workspace switcher, command palette) should observe
// this service to update their presentation when the accent color or
// theme mode changes.  UI must never be the source of truth for theme
// state — this service is.
class AstraThemeServiceObserver : public base::CheckedObserver {
 public:
  // Called when the active accent color changes.
  // Observers should refresh any accent-colored UI elements.
  //
  // The ColorProvider will also be rebuilt as a result of the change,
  // so views using kColorAstraWorkspaceAccent* IDs will update
  // automatically through the standard OnThemeChanged() pipeline.
  virtual void OnAccentColorChanged(SkColor new_accent_color) {}

  // Called when the overall theme changes (dark/light mode toggle,
  // custom theme install, etc.).
  // Observers should do a full theme refresh if they cannot rely on
  // the ColorProvider's automatic update mechanism.
  virtual void OnThemeChanged() {}

  // Called when the theme preset changes (e.g. from system to dark).
  // Observers that care about the preset (not just the effective mode)
  // should override this.
  virtual void OnThemePresetChanged(AstraThemePreset preset) {}

  // Called when the active theme scheme changes.
  // Observers that care about named color schemes should override this.
  virtual void OnThemeSchemeChanged(const std::string& scheme_name) {}

  // Called when high contrast mode is toggled.
  // Observers that adjust their presentation for high contrast
  // should override this.
  virtual void OnHighContrastChanged(bool enabled) {}

  // Called when any theme setting changes (catch-all notification).
  // Observers can use this as a single notification for any theme-related
  // change when they don't need granularity.
  virtual void OnThemeSettingsChanged() {}

 protected:
  ~AstraThemeServiceObserver() override = default;
};

// ---------------------------------------------------------------------------
// Theme scheme descriptor
// ---------------------------------------------------------------------------

// Describes a named color scheme preset.
//
// Theme schemes are named color palettes that users can apply to customize
// the browser's look and feel.  Each scheme defines an accent color,
// a preferred light/dark mode, and an optional secondary color.
//
// Astra owns these schemes — they are a higher-level concept than
// Chromium's ThemeService and don't directly map to Chromium themes.
struct AstraThemeScheme {
  // Machine-readable name (e.g. "ocean", "forest").
  std::string name;

  // Human-readable display name (e.g. "Ocean", "Sunset").
  std::string display_name;

  // Primary accent color for the scheme.
  SkColor accent_color;

  // Optional secondary color (e.g. for gradients or secondary accents).
  SkColor secondary_color;

  // Preferred light/dark mode for this scheme.
  // True = dark, false = light.
  bool prefers_dark;

  // Brief description of the scheme.
  std::string description;
};

// ---------------------------------------------------------------------------
// Theme settings bundle
// ---------------------------------------------------------------------------

// Bundles all theme-related settings for bulk get/set operations.
//
// This struct provides a convenient way to read or apply all theme
// settings at once (e.g. for import/export or settings panels).
//
// All individual settings are also accessible via dedicated getters
// and setters on AstraThemeService.
struct AstraThemeSettings {
  // Whether to use the active workspace's accent color for theming.
  // When false, the custom accent color is used instead.
  bool use_workspace_accent = true;

  // Custom accent color (used when use_workspace_accent is false).
  SkColor custom_accent_color = SK_ColorBLUE;

  // Current theme preset (system/light/dark/auto).
  AstraThemePreset theme_preset = AstraThemePreset::kSystem;

  // Active named color scheme (empty string means no scheme active).
  std::string theme_scheme;

  // Whether high contrast mode is enabled.
  bool use_high_contrast = false;

  // Whether to show accent color on tab strips.
  bool show_accent_on_tabs = true;

  // Whether to show accent color on the sidebar.
  bool show_accent_on_sidebar = true;

  // Accent color intensity multiplier (0.5 - 2.0).
  // 1.0 = normal intensity.
  double accent_intensity = 1.0;

  // Whether auto light/dark scheduling is enabled.
  bool use_auto_theme_schedule = false;

  // Time when light mode starts in auto schedule (HH:MM 24h format).
  std::string auto_theme_light_start = "07:00";

  // Time when dark mode starts in auto schedule (HH:MM 24h format).
  std::string auto_theme_dark_start = "19:00";
};

// ---------------------------------------------------------------------------
// AstraThemeService
// ---------------------------------------------------------------------------

// Profile-scoped keyed service that manages Astra's theme and accent
// color state.
//
// This service bridges three systems:
//   1. Chromium's ThemeService — provides dark/light mode and system theme.
//   2. AstraWorkspaceService — provides per-workspace accent colors.
//   3. AstraColorMixer — registers Astra colors with the ColorProvider.
//
// The active accent color is determined by the active workspace.
// When the active workspace changes, the accent color updates and
// observers are notified.  The accent color can also be set directly
// (e.g., for a user-defined override) via SetAccentColor().
//
// Usage:
//   AstraThemeService* theme = AstraThemeService::GetForProfile(profile);
//   SkColor accent = theme->GetAccentColor();
//   bool dark = theme->IsDarkMode();
//
//   theme->AddObserver(&my_observer);
//   // my_observer.OnAccentColorChanged() is called when accent changes.
class AstraThemeService : public KeyedService {
 public:
  // Returns the AstraThemeService for the given profile.
  // Returns nullptr for system profiles or other contexts where the
  // service is not available.
  static AstraThemeService* GetForProfile(Profile* profile);

  explicit AstraThemeService(Profile* profile);
  AstraThemeService(const AstraThemeService&) = delete;
  AstraThemeService& operator=(const AstraThemeService&) = delete;
  ~AstraThemeService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraThemeServiceObserver* observer);
  void RemoveObserver(AstraThemeServiceObserver* observer);

  // -- Accent color --------------------------------------------------------

  // Returns the current accent color as an SkColor.
  //
  // The accent color is derived from the active workspace's accent
  // color setting.  If no workspace accent is available, falls back
  // to a default blue accent.
  SkColor GetAccentColor() const;

  // Returns true if the accent color was set directly via SetAccentColor().
  //
  // When false, the accent color is derived from the active workspace.
  // When true, the accent color is a user override that persists across
  // workspace changes.
  //
  // TODO(astra): Decide whether accent overrides should persist across
  //   workspace changes.  Currently they do (stored in prefs), but the
  //   original behavior was to reset on workspace change.  Need to align
  //   with product requirements.
  bool HasAccentOverride() const;

  // Clears any active accent override, reverting to the workspace-derived
  // accent color.
  //
  // If there is no active override, this is a no-op.
  // Notifies observers if the accent color changes as a result.
  void ClearAccentOverride();

  // Sets the accent color directly, overriding the workspace's accent.
  //
  // This sets a per-profile accent override that takes precedence over
  // the workspace's accent color.  The override is persisted in prefs
  // and survives profile restarts.
  //
  // Notifies observers and triggers a theme refresh.
  void SetAccentColor(SkColor color);

  // Sets the accent color from the given workspace's accent color.
  //
  // Looks up the workspace's accent color string and applies it.
  // Clears any active accent override.
  //
  // Returns true if the workspace existed and its color was applied.
  // Returns false if the workspace was not found (accent unchanged).
  bool SetAccentColorFromWorkspace(const std::string& workspace_id);

  // -- Accent color palette --------------------------------------------------

  // Returns the preset accent color palette.
  //
  // A static set of 10 curated accent colors that users can choose from.
  // These are the same colors shown in the theme settings picker.
  //
  // Returns:
  //   A vector of SkColor values representing the preset palette.
  //
  // TODO(astra): Consider adding a "custom" option that lets users pick
  //   any color via a color picker.  The current preset palette is the
  //   MVP; custom colors are tracked separately.
  static std::vector<SkColor> GetAccentColorPalette();

  // -- Active workspace ----------------------------------------------------

  // Returns the ID of the workspace whose accent color is currently
  // active.  This may differ from AstraWorkspaceService's active
  // workspace if SetAccentColor() has been called with a direct color.
  const std::string& GetActiveWorkspaceId() const;

  // Sets the active workspace and updates the accent color to match.
  //
  // This changes which workspace's accent color is used for theming.
  // It does NOT change AstraWorkspaceService's active workspace —
  // that is a separate concern.
  //
  // TODO(astra): Decide whether the theme service's active workspace
  //   should always track AstraWorkspaceService's active workspace,
  //   or if they can be independent (e.g. for theming preview).
  //   Currently, the theme service observes workspace changes and
  //   mirrors the active workspace.
  void SetActiveWorkspace(const std::string& workspace_id);

  // -- Theme mode ----------------------------------------------------------

  // Returns true if dark mode is currently active.
  //
  // Checks both the system-level dark mode setting (via NativeTheme)
  // and any Chromium theme that may force a specific mode.
  //
  // This is a projection — Chromium's ThemeService and NativeTheme
  // are the source of truth.  AstraThemeService exposes this state
  // in a convenient form for Astra UI surfaces.
  bool IsDarkMode() const;

  // -- Theme preset --------------------------------------------------------

  // Returns the current theme preset.
  //
  // The preset controls how light/dark mode is selected:
  //   - kSystem: follow OS/system theme (default)
  //   - kLight:  force light mode
  //   - kDark:   force dark mode
  //   - kAuto:   auto-switch based on time of day
  //
  // Persisted in profile prefs.
  AstraThemePreset theme_preset() const { return theme_preset_; }

  // Sets the theme preset.
  //
  // Changes how light/dark mode is determined.  Notifies observers
  // and persists the setting to profile prefs.
  //
  // TODO(astra): When preset is kLight or kDark, we should override
  //   Chromium's ThemeService to force the corresponding mode.
  //   Currently, the preset is stored but may not fully take effect
  //   until we patch ThemeService to respect Astra's preset.
  //   Chromium owner: ThemeService (chrome/browser/themes/theme_service.h)
  //   Patch point: ThemeService::GetDisplayProperty or equivalent.
  void set_theme_preset(AstraThemePreset preset);

  // -- Theme schemes -------------------------------------------------------

  // Returns all available theme schemes.
  //
  // Theme schemes are named color palettes (e.g. "Ocean", "Forest",
  // "Sunset") that bundle an accent color, a secondary color, and a
  // preferred light/dark mode.
  //
  // Returns:
  //   A vector of all theme scheme descriptors.
  static std::vector<AstraThemeScheme> GetThemeSchemes();

  // Applies a named theme scheme by name.
  //
  // Applies the scheme's accent color, secondary color, and preferred
  // dark/light mode preference.  Notifies observers and persists the
  // scheme name to prefs.
  //
  // Args:
  //   scheme_name - Machine-readable name of the scheme to apply.
  //
  // Returns:
  //   True if the scheme was found and applied, false if not found.
  bool ApplyThemeScheme(const std::string& scheme_name);

  // Returns the name of the currently active theme scheme.
  //
  // Returns an empty string if no named scheme is active (i.e. the
  // accent color is from a workspace or direct user override).
  const std::string& active_theme_scheme() const { return active_scheme_; }

  // Finds a theme scheme by name.
  //
  // Args:
  //   scheme_name - Machine-readable name of the scheme to find.
  //   out_scheme  - Output parameter to receive the scheme if found.
  //
  // Returns:
  //   True if the scheme was found, false otherwise.
  static bool FindThemeScheme(const std::string& scheme_name,
                              AstraThemeScheme* out_scheme);

  // -- Color utilities -----------------------------------------------------

  // Returns the accent color with a custom alpha value.
  //
  // Args:
  //   alpha - Alpha value (0-255).  Values outside the range are clamped.
  //
  // Returns:
  //   The current accent color with the specified alpha.
  SkColor GetAccentColorWithAlpha(int alpha) const;

  // Returns the surface color based on the current dark/light mode.
  //
  // Surface colors are used for backgrounds of cards, sheets, panels,
  // and other elevated surfaces.
  //
  // Returns:
  //   The surface color appropriate for the current theme mode.
  SkColor GetSurfaceColor() const;

  // Returns the primary text color based on the current dark/light mode.
  //
  // Primary text is used for body text, titles, and other high-priority
  // text elements.
  //
  // Returns:
  //   The primary text color appropriate for the current theme mode.
  SkColor GetTextPrimaryColor() const;

  // Returns the secondary text color based on the current dark/light mode.
  //
  // Secondary text is used for captions, hints, and other lower-priority
  // text elements.
  //
  // Returns:
  //   The secondary text color appropriate for the current theme mode.
  SkColor GetTextSecondaryColor() const;

  // Returns the border color based on the current dark/light mode.
  //
  // Border colors are used for dividers, borders, and outline strokes.
  //
  // Returns:
  //   The border color appropriate for the current theme mode.
  SkColor GetBorderColor() const;

  // Returns the hover state color based on the accent color.
  //
  // The hover color is a slightly lighter or darker version of the
  // accent color, tuned for the current dark/light mode.
  //
  // Returns:
  //   The accent color adjusted for hover state.
  SkColor GetHoverColor() const;

  // Blends two colors together.
  //
  // Args:
  //   a - First color (base).
  //   b - Second color (foreground).
  //   t - Blend factor (0.0 = fully a, 1.0 = fully b).
  //
  // Returns:
  //   The blended color.
  static SkColor BlendColors(SkColor a, SkColor b, float t);

  // Computes the WCAG contrast ratio between two colors.
  //
  // The contrast ratio ranges from 1.0 (same color) to 21.0 (pure white
  // on pure black).
  //
  // WCAG AA requires:
  //   - 4.5:1 for normal text
  //   - 3.0:1 for large text
  //
  // Args:
  //   foreground - The foreground text color.
  //   background - The background color.
  //
  // Returns:
  //   The contrast ratio (>= 1.0).
  static float GetContrastRatio(SkColor foreground, SkColor background);

  // Checks if text of the given color is readable on the given background.
  //
  // Uses WCAG AA standard: 4.5:1 contrast for normal text.
  //
  // Args:
  //   foreground - The foreground text color.
  //   background - The background color.
  //
  // Returns:
  //   True if the contrast meets WCAG AA for normal text.
  static bool IsReadable(SkColor foreground, SkColor background);

  // Adjusts a foreground color to be readable against a background.
  //
  // If the foreground color already has sufficient contrast, returns it
  // unchanged.  Otherwise, lightens or darkens it to achieve WCAG AA
  // contrast.
  //
  // Args:
  //   foreground - The foreground text color to adjust.
  //   background - The background color.
  //
  // Returns:
  //   A color with at least 4.5:1 contrast against the background.
  static SkColor MakeReadable(SkColor foreground, SkColor background);

  // -- Presentation settings ----------------------------------------------

  // Whether to use the workspace accent color for theming.
  //
  // When true, the accent color follows the active workspace.
  // When false, a custom accent color is used instead.
  //
  // Persisted in profile prefs.
  bool use_workspace_accent() const { return use_workspace_accent_; }

  // Sets whether to use the workspace accent color.
  //
  // Notifies observers if the effective accent color changes.
  void set_use_workspace_accent(bool value);

  // Returns the custom accent color.
  //
  // This is the user-configured accent override that's used when
  // use_workspace_accent is false.
  SkColor custom_accent_color() const { return custom_accent_color_; }

  // Sets the custom accent color.
  //
  // Does not automatically enable the custom accent — call
  // set_use_workspace_accent(false) to use it.
  void set_custom_accent_color(SkColor color);

  // Whether high contrast mode is enabled.
  //
  // When enabled, UI surfaces should use higher-contrast colors,
  // thicker borders, and bolder text for accessibility.
  //
  // Persisted in profile prefs.
  bool use_high_contrast() const { return use_high_contrast_; }

  // Sets high contrast mode.
  //
  // Notifies observers and persists to prefs.
  void set_use_high_contrast(bool value);

  // Whether to show accent color on tab strips.
  //
  // When true, the active tab and tab strip show accent color highlights.
  //
  // Persisted in profile prefs.
  bool show_accent_on_tabs() const { return show_accent_on_tabs_; }

  // Sets whether to show accent color on tab strips.
  void set_show_accent_on_tabs(bool value);

  // Whether to show accent color on the sidebar.
  //
  // When true, the sidebar uses accent color for highlights and
  // active item indicators.
  //
  // Persisted in profile prefs.
  bool show_accent_on_sidebar() const { return show_accent_on_sidebar_; }

  // Sets whether to show accent color on the sidebar.
  void set_show_accent_on_sidebar(bool value);

  // Accent color intensity multiplier.
  //
  // Controls how prominent the accent color appears.
  // Range: 0.5 (subtle) to 2.0 (vibrant).
  //
  // Persisted in profile prefs.
  double accent_intensity() const { return accent_intensity_; }

  // Sets the accent intensity.
  //
  // Values are clamped to the 0.5 - 2.0 range.
  void set_accent_intensity(double value);

  // Whether auto light/dark theme scheduling is enabled.
  //
  // When enabled, the theme automatically switches between light and
  // dark modes at configured times of day.
  //
  // Persisted in profile prefs.
  bool use_auto_theme_schedule() const { return use_auto_theme_schedule_; }

  // Sets whether auto theme scheduling is enabled.
  void set_use_auto_theme_schedule(bool value);

  // Time when light mode starts in auto schedule (HH:MM 24h format).
  const std::string& auto_theme_light_start() const {
    return auto_theme_light_start_;
  }

  // Sets the light mode start time for auto schedule.
  void set_auto_theme_light_start(const std::string& time_str);

  // Time when dark mode starts in auto schedule (HH:MM 24h format).
  const std::string& auto_theme_dark_start() const {
    return auto_theme_dark_start_;
  }

  // Sets the dark mode start time for auto schedule.
  void set_auto_theme_dark_start(const std::string& time_str);

  // -- Bulk operations -----------------------------------------------------

  // Resets all theme settings to their default values.
  //
  // Resets accent, preset, schemes, presentation settings, etc.
  // Notifies observers of the changes.
  void ResetToDefaults();

  // Applies all settings from a ThemeSettings bundle.
  //
  // This is a bulk apply — all settings are updated in one call.
  // Observers receive individual notifications for each changed setting,
  // plus a final OnThemeSettingsChanged().
  //
  // Args:
  //   settings - The settings bundle to apply.
  void ApplyThemeSettings(const AstraThemeSettings& settings);

  // Returns the current theme settings as a bundle.
  //
  // Returns:
  //   A ThemeSettings struct with all current values.
  AstraThemeSettings GetThemeSettings() const;

  // -- Color utilities -----------------------------------------------------

  // Returns true if the current accent color is dark (low luminance).
  //
  // Uses relative luminance to determine if the accent reads as dark.
  // Useful for deciding whether to use light or dark text/icons on
  // accent-colored surfaces.
  //
  // This is a utility helper for UI layers — it doesn't change state.
  bool IsAccentColorDark() const;

  // Returns a lightened version of the current accent color.
  //
  // Args:
  //   amount - Lightening amount (0.0 = no change, 1.0 = fully white).
  //
  // Returns:
  //   The lightened accent color.
  //
  // This is a utility helper for UI layers — it doesn't change state.
  SkColor GetLightenedAccent(float amount) const;

  // Returns a darkened version of the current accent color.
  //
  // Args:
  //   amount - Darkening amount (0.0 = no change, 1.0 = fully black).
  //
  // Returns:
  //   The darkened accent color.
  //
  // This is a utility helper for UI layers — it doesn't change state.
  SkColor GetDarkenedAccent(float amount) const;

  // -- Contrast helpers ----------------------------------------------------

  // Returns the accent color optimized for use on light backgrounds.
  //
  // If the current accent is too light to have good contrast on a
  // light background, returns a darkened version.  Otherwise returns
  // the accent color as-is.
  //
  // This is a utility helper for UI layers — it doesn't change state.
  SkColor GetAccentOnLight() const;

  // Returns the accent color optimized for use on dark backgrounds.
  //
  // If the current accent is too dark to have good contrast on a
  // dark background, returns a lightened version.  Otherwise returns
  // the accent color as-is.
  //
  // This is a utility helper for UI layers — it doesn't change state.
  SkColor GetAccentOnDark() const;

  // -- Theme change notification ------------------------------------------

  // Called when Chromium's theme changes (dark/light toggle, custom
  // theme install, etc.).
  //
  // This is invoked by the theme observer wrapper when the underlying
  // Chromium ThemeService fires its OnThemeChanged() notification.
  //
  // Observers are notified and the ColorProvider is refreshed.
  //
  // Patch point: This method is called from a Chromium patch in
  //   chrome/browser/themes/theme_service.cc or through the
  //   ThemeServiceObserver interface.
  void OnThemeChanged();

 private:
  // Thin observer wrapper for Chromium's ThemeService.
  //
  // Implements ThemeServiceObserver and forwards theme change
  // notifications to AstraThemeService::OnThemeChanged().
  //
  // Using an inner class avoids polluting AstraThemeService's
  // public interface with Chromium observer methods and keeps
  // the dependency on ThemeService internal.
  class ThemeServiceObserverAdapter;

  // Observer for AstraWorkspaceService changes.
  //
  // Updates the accent color when the active workspace changes
  // or when a workspace's accent color is modified.
  class WorkspaceObserverAdapter;

  // Initializes accent color from the current active workspace.
  // Called from the constructor after observer setup.
  void InitializeAccentFromWorkspace();

  // Updates the accent color and notifies observers if it changed.
  void UpdateAccentColor(SkColor new_color);

  // Notifies all observers of the current accent color and theme state.
  void NotifyAccentColorChanged();
  void NotifyThemeChanged();
  void NotifyThemePresetChanged();
  void NotifyThemeSchemeChanged();
  void NotifyHighContrastChanged();
  void NotifyThemeSettingsChanged();

  // Loads all theme settings from prefs.
  // Called from the constructor to initialize state from persisted state.
  void LoadSettingsFromPrefs();

  // Saves all theme settings to prefs.
  // Called when any setting changes.
  void SaveSettingsToPrefs() const;

  raw_ptr<Profile> profile_;

  // Current accent color.
  SkColor accent_color_;

  // ID of the workspace whose accent color is currently active.
  // Empty if using a direct color override (SetAccentColor).
  std::string active_workspace_id_;

  // Whether the accent color was set directly (true) or derived
  // from a workspace (false).  Direct-set accents are persisted
  // in prefs and survive profile restarts.
  bool has_accent_override_ = false;

  // Current theme preset (system/light/dark/auto).
  // Persisted in profile prefs.
  AstraThemePreset theme_preset_ = AstraThemePreset::kSystem;

  // Name of the currently active theme scheme.
  // Empty means no named scheme is active.
  std::string active_scheme_;

  // -- Presentation settings -----------------------------------------------

  // Whether to use workspace accent for theming.
  bool use_workspace_accent_ = true;

  // Custom accent color (used when use_workspace_accent_ is false).
  SkColor custom_accent_color_ = SK_ColorBLUE;

  // Whether high contrast mode is enabled.
  bool use_high_contrast_ = false;

  // Whether to show accent color on tab strips.
  bool show_accent_on_tabs_ = true;

  // Whether to show accent color on the sidebar.
  bool show_accent_on_sidebar_ = true;

  // Accent intensity multiplier (0.5 - 2.0).
  double accent_intensity_ = 1.0;

  // Whether auto light/dark scheduling is enabled.
  bool use_auto_theme_schedule_ = false;

  // Light mode start time for auto schedule (HH:MM).
  std::string auto_theme_light_start_ = "07:00";

  // Dark mode start time for auto schedule (HH:MM).
  std::string auto_theme_dark_start_ = "19:00";

  // Observer wrapper for Chromium's ThemeService.
  std::unique_ptr<ThemeServiceObserverAdapter> theme_observer_;

  // Observer wrapper for AstraWorkspaceService.
  std::unique_ptr<WorkspaceObserverAdapter> workspace_observer_;

  // Observers of this service (Astra UI surfaces).
  base::ObserverList<AstraThemeServiceObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_THEME_SERVICE_H_
