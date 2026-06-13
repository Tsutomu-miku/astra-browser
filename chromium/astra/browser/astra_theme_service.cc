// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_theme_service.h"

#include <algorithm>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/common/astra_color_utils.h"

namespace astra {

namespace {

// -- Pref keys --------------------------------------------------------------
//
// Theme service pref keys.  Defined here (rather than in astra_prefs.h)
// because they are internal to the theme service and don't need to be
// shared with other components.
//
// Format follows Chromium pref conventions: dot-separated path under the
// "astra" namespace.

// Theme preset (int).
// Values: 0 = kSystem, 1 = kLight, 2 = kDark, 3 = kAuto.
// Controls how light/dark mode is selected.
//
// TODO(astra): Consider syncing this pref across devices if we add
//   theme sync support.  Chromium owner: sync service.
//   Patch point: components/sync_preferences/
inline constexpr char kPrefThemePreset[] = "astra.theme.preset";

// Accent color override (string, hex color).
// Empty string means no override (use workspace accent).
// When set, contains a hex color string like "#FF5B8FF9" or "#5B8FF9".
//
// This is the user's manual accent color choice that overrides the
// workspace-derived accent.
inline constexpr char kPrefAccentOverride[] = "astra.theme.accent_override";

// Active theme scheme (string).
// Empty string means no named scheme is active.
// When set, contains the machine-readable name of the active scheme.
inline constexpr char kPrefThemeScheme[] = "astra.theme.scheme";

// Whether to use workspace accent for theming (bool).
// Default: true — accent color follows the active workspace.
inline constexpr char kPrefUseWorkspaceAccent[] =
    "astra.theme.use_workspace_accent";

// Custom accent color (int, stored as ARGB integer).
// Used when use_workspace_accent is false.
// Default: Google Blue 500.
inline constexpr char kPrefCustomAccentColor[] =
    "astra.theme.custom_accent_color";

// Whether high contrast mode is enabled (bool).
// Default: false.
inline constexpr char kPrefUseHighContrast[] =
    "astra.theme.use_high_contrast";

// Whether to show accent color on tab strips (bool).
// Default: true.
inline constexpr char kPrefShowAccentOnTabs[] =
    "astra.theme.show_accent_on_tabs";

// Whether to show accent color on sidebar (bool).
// Default: true.
inline constexpr char kPrefShowAccentOnSidebar[] =
    "astra.theme.show_accent_on_sidebar";

// Accent color intensity (double, 0.5 - 2.0).
// Default: 1.0 — normal intensity.
inline constexpr char kPrefAccentIntensity[] =
    "astra.theme.accent_intensity";

// Whether auto theme scheduling is enabled (bool).
// Default: false.
inline constexpr char kPrefUseAutoThemeSchedule[] =
    "astra.theme.use_auto_theme_schedule";

// Auto theme light start time (string, HH:MM 24h format).
// Default: "07:00".
inline constexpr char kPrefAutoThemeLightStart[] =
    "astra.theme.auto_theme_light_start";

// Auto theme dark start time (string, HH:MM 24h format).
// Default: "19:00".
inline constexpr char kPrefAutoThemeDarkStart[] =
    "astra.theme.auto_theme_dark_start";

// -- Accent color palette ---------------------------------------------------

// Preset accent color palette — 10 curated colors.
//
// These are the colors shown in the theme settings accent picker.
// Colors are chosen to be accessible, distinct, and aesthetically
// consistent with Astra's design language.
//
// Order: blue, purple, pink, red, orange, yellow, green, teal, cyan, indigo.
constexpr SkColor kAccentPalette[] = {
    SkColorSetRGB(0x5B, 0x8F, 0xF9),  // Blue    — Google Blue 500
    SkColorSetRGB(0x93, 0x67, 0xEB),  // Purple  — Deep Purple 500
    SkColorSetRGB(0xEC, 0x48, 0x99),  // Pink    — Pink 500
    SkColorSetRGB(0xEF, 0x44, 0x37),  // Red     — Red 500
    SkColorSetRGB(0xFB, 0x8C, 0x00),  // Orange  — Orange 500
    SkColorSetRGB(0xFF, 0xC1, 0x07),  // Yellow  — Amber 500
    SkColorSetRGB(0x34, 0xA8, 0x53),  // Green   — Green 500
    SkColorSetRGB(0x00, 0x96, 0x88),  // Teal    — Teal 500
    SkColorSetRGB(0x03, 0xA9, 0xF4),  // Cyan    — Cyan 500
    SkColorSetRGB(0x3F, 0x51, 0xB5),  // Indigo  — Indigo 500
};

// Default accent color used when no workspace accent is available.
// Google Blue 500 — a neutral, brand-aligned default.
constexpr SkColor kDefaultAccentColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);

// Default custom accent color.
constexpr SkColor kDefaultCustomAccentColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);

// -- Theme schemes -----------------------------------------------------------

// Curated theme schemes — named color palettes with accent, secondary,
// and preferred dark/light mode.
//
// These are higher-level than the accent color palette — each scheme
// represents a complete aesthetic direction.
//
// Schemes:
//   - Ocean:    Deep blue palette, prefers dark mode
//   - Forest:   Green palette, prefers dark mode
//   - Sunset:   Warm orange palette, prefers light mode
//   - Midnight: Deep purple palette, prefers dark mode
//   - Minimal:  Neutral gray palette, prefers light mode
//   - Lavender: Purple-pink palette, prefers light mode
//   - Coral:    Warm coral palette, prefers light mode
//   - Slate:    Cool gray palette, prefers dark mode
constexpr size_t kThemeSchemeCount = 8;

const AstraThemeScheme kThemeSchemes[kThemeSchemeCount] = {
    {
        "ocean",
        "Ocean",
        SkColorSetRGB(0x00, 0x77, 0xB6),  // Deep blue
        SkColorSetRGB(0x00, 0xB4, 0xD8),  // Bright cyan-blue
        true,  // prefers dark
        "Deep blue tones inspired by the ocean depths.",
    },
    {
        "forest",
        "Forest",
        SkColorSetRGB(0x2D, 0x6A, 0x4F),  // Forest green
        SkColorSetRGB(0x40, 0x91, 0x6C),  // Emerald green
        true,  // prefers dark
        "Earthy greens evoking a forest canopy.",
    },
    {
        "sunset",
        "Sunset",
        SkColorSetRGB(0xE8, 0x5D, 0x04),  // Sunset orange
        SkColorSetRGB(0xF4, 0x8C, 0x06),  // Golden amber
        false,  // prefers light
        "Warm oranges and golds of a setting sun.",
    },
    {
        "midnight",
        "Midnight",
        SkColorSetRGB(0x72, 0x09, 0xB7),  // Deep purple
        SkColorSetRGB(0x3A, 0x0C, 0xA3),  // Indigo violet
        true,  // prefers dark
        "Rich purples for late-night focus.",
    },
    {
        "minimal",
        "Minimal",
        SkColorSetRGB(0x6C, 0x75, 0x7D),  // Neutral gray
        SkColorSetRGB(0xAD, 0xB5, 0xBD),  // Light gray
        false,  // prefers light
        "Clean, understated grays for a minimal look.",
    },
    {
        "lavender",
        "Lavender",
        SkColorSetRGB(0x9D, 0x4E, 0xDD),  // Lavender
        SkColorSetRGB(0xC7, 0x7D, 0xFF),  // Light purple
        false,  // prefers light
        "Soft purples for a calm, creative feel.",
    },
    {
        "coral",
        "Coral",
        SkColorSetRGB(0xFF, 0x6B, 0x6B),  // Coral red
        SkColorSetRGB(0xFF, 0x8F, 0x8F),  // Light coral
        false,  // prefers light
        "Warm coral tones for an energetic vibe.",
    },
    {
        "slate",
        "Slate",
        SkColorSetRGB(0x49, 0x50, 0x57),  // Slate gray
        SkColorSetRGB(0x6C, 0x75, 0x7D),  // Medium gray
        true,  // prefers dark
        "Cool slate tones for a professional look.",
    },
};

// Converts a hex color string to SkColor.
//
// Accepts formats:
//   "#RRGGBB"  (7 chars, with # prefix)
//   "RRGGBB"   (6 chars, no prefix)
//   "#RGB"     (4 chars, short form)
//   "RGB"      (3 chars, short form)
//
// Returns the default color if parsing fails.
//
// TODO(astra): Consider using gfx::HexStringToSkColor from ui/gfx/color_utils.h
//   once the full Chromium checkout is available.  For now, implement a
//   simple parser to avoid extra dependencies.
//   Chromium owner: gfx::ColorUtils (ui/gfx/color_utils.h)
SkColor HexToSkColor(const std::string& hex, SkColor fallback) {
  if (hex.empty())
    return fallback;

  base::StringPiece s = hex;

  // Strip leading '#' if present.
  if (s.starts_with('#'))
    s = s.substr(1);

  // Handle short form (#RGB → #RRGGBB).
  std::string expanded;
  if (s.size() == 3) {
    expanded.reserve(6);
    for (char c : s) {
      expanded.push_back(c);
      expanded.push_back(c);
    }
    s = expanded;
  }

  if (s.size() != 6)
    return fallback;

  uint32_t value = 0;
  if (!base::HexStringToUInt(s, &value))
    return fallback;

  return SkColorSetRGB(
      static_cast<uint8_t>((value >> 16) & 0xFF),
      static_cast<uint8_t>((value >> 8) & 0xFF),
      static_cast<uint8_t>(value & 0xFF));
}

// Converts an SkColor to a hex color string (#RRGGBB format).
//
// Alpha is ignored — only the RGB channels are encoded.
std::string SkColorToHex(SkColor color) {
  std::string result = "#";
  const char kHex[] = "0123456789ABCDEF";
  uint8_t r = SkColorGetR(color);
  uint8_t g = SkColorGetG(color);
  uint8_t b = SkColorGetB(color);
  result += kHex[r >> 4];
  result += kHex[r & 0x0F];
  result += kHex[g >> 4];
  result += kHex[g & 0x0F];
  result += kHex[b >> 4];
  result += kHex[b & 0x0F];
  return result;
}

// Converts an integer to an AstraThemePreset, with bounds checking.
// Returns kSystem for out-of-range values.
AstraThemePreset IntToThemePreset(int value) {
  switch (value) {
    case static_cast<int>(AstraThemePreset::kSystem):
      return AstraThemePreset::kSystem;
    case static_cast<int>(AstraThemePreset::kLight):
      return AstraThemePreset::kLight;
    case static_cast<int>(AstraThemePreset::kDark):
      return AstraThemePreset::kDark;
    case static_cast<int>(AstraThemePreset::kAuto):
      return AstraThemePreset::kAuto;
    default:
      return AstraThemePreset::kSystem;
  }
}

// -- Surface / text / border color constants -------------------------------

// Surface colors for light and dark mode.
constexpr SkColor kSurfaceLight = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kSurfaceDark = SkColorSetRGB(0x1E, 0x1E, 0x1E);

// Primary text colors for light and dark mode.
constexpr SkColor kTextPrimaryLight = SkColorSetRGB(0x20, 0x21, 0x24);
constexpr SkColor kTextPrimaryDark = SkColorSetRGB(0xE8, 0xEA, 0xED);

// Secondary text colors for light and dark mode.
constexpr SkColor kTextSecondaryLight = SkColorSetRGB(0x5F, 0x63, 0x6B);
constexpr SkColor kTextSecondaryDark = SkColorSetRGB(0x9A, 0xA0, 0xA6);

// Border colors for light and dark mode.
constexpr SkColor kBorderLight = SkColorSetRGB(0xDA, 0xDC, 0xE0);
constexpr SkColor kBorderDark = SkColorSetRGB(0x3C, 0x40, 0x43);

// -- WCAG contrast helpers -------------------------------------------------

// Computes the WCAG contrast ratio between two luminance values.
//
// Formula: (L1 + 0.05) / (L2 + 0.05), where L1 is the lighter luminance.
float ContrastRatioFromLuminance(float l1, float l2) {
  float lighter = std::max(l1, l2);
  float darker = std::min(l1, l2);
  return (lighter + 0.05f) / (darker + 0.05f);
}

// WCAG AA minimum contrast for normal text.
constexpr float kWcagAaContrastRatio = 4.5f;

// Clamps a double value to the given range.
double ClampDouble(double value, double min_val, double max_val) {
  return std::clamp(value, min_val, max_val);
}

// Clamps an int value to the given range.
int ClampInt(int value, int min_val, int max_val) {
  return std::clamp(value, min_val, max_val);
}

// Converts an SkColor to an integer (for pref storage as int).
// Stores as 0xAARRGGBB format.
int SkColorToInt(SkColor color) {
  return static_cast<int>(color);
}

// Converts an integer back to an SkColor.
SkColor IntToSkColor(int value) {
  return static_cast<SkColor>(value);
}

}  // namespace

// ===========================================================================
// AstraThemeServiceFactory
// ===========================================================================

AstraThemeServiceFactory::AstraThemeServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraThemeService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Theme state is user-level, not per-context.
              // Dark mode, accent color, and theme selection apply
              // to all windows of the same profile.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to redirect to).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {
  // TODO(astra): Declare dependency on AstraWorkspaceServiceFactory
  //   once both factories are properly registered with the
  //   BrowserContextDependencyManager.
  //   Chromium pattern: DependsOn(AstraWorkspaceServiceFactory::GetInstance())
  // DependsOn(AstraWorkspaceServiceFactory::GetInstance());
}

AstraThemeServiceFactory::~AstraThemeServiceFactory() = default;

// static
AstraThemeServiceFactory* AstraThemeServiceFactory::GetInstance() {
  static base::NoDestructor<AstraThemeServiceFactory> instance;
  return instance.get();
}

// static
AstraThemeService* AstraThemeServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraThemeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
void AstraThemeServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Theme preset (system/light/dark/auto).
  // Default: kSystem — follow the OS/system theme.
  registry->RegisterIntegerPref(
      kPrefThemePreset,
      static_cast<int>(AstraThemePreset::kSystem));

  // Accent color override (hex string, empty = no override).
  // Default: empty — use workspace accent color.
  registry->RegisterStringPref(kPrefAccentOverride, std::string());

  // Active theme scheme (string, empty = no named scheme).
  registry->RegisterStringPref(kPrefThemeScheme, std::string());

  // Whether to use workspace accent for theming.
  // Default: true — accent follows active workspace.
  registry->RegisterBooleanPref(kPrefUseWorkspaceAccent, true);

  // Custom accent color (stored as integer, ARGB format).
  registry->RegisterIntegerPref(
      kPrefCustomAccentColor,
      SkColorToInt(kDefaultCustomAccentColor));

  // High contrast mode.
  registry->RegisterBooleanPref(kPrefUseHighContrast, false);

  // Show accent on tab strips.
  registry->RegisterBooleanPref(kPrefShowAccentOnTabs, true);

  // Show accent on sidebar.
  registry->RegisterBooleanPref(kPrefShowAccentOnSidebar, true);

  // Accent intensity (0.5 - 2.0).
  registry->RegisterDoublePref(kPrefAccentIntensity, 1.0);

  // Auto theme scheduling.
  registry->RegisterBooleanPref(kPrefUseAutoThemeSchedule, false);

  // Auto theme light start time.
  registry->RegisterStringPref(kPrefAutoThemeLightStart, "07:00");

  // Auto theme dark start time.
  registry->RegisterStringPref(kPrefAutoThemeDarkStart, "19:00");
}

std::unique_ptr<KeyedService>
AstraThemeServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraThemeService>(profile);
}

// ===========================================================================
// ThemeServiceObserverAdapter — thin wrapper around ThemeServiceObserver
// ===========================================================================

// Thin observer wrapper that forwards ThemeService change notifications
// to AstraThemeService.
//
// Using an inner class keeps the ThemeServiceObserver interface out of
// AstraThemeService's public API and follows Chromium's pattern of
// wrapping external observer interfaces in private adapter classes.
//
// Chromium pattern: many services use private observer adapter classes
// to listen to external services without exposing those observer methods
// in their own public interface.
class AstraThemeService::ThemeServiceObserverAdapter
    : public ThemeServiceObserver {
 public:
  explicit ThemeServiceObserverAdapter(AstraThemeService* owner,
                                       ThemeService* theme_service)
      : owner_(owner), theme_service_(theme_service) {
    DCHECK(owner_);
    if (theme_service_) {
      theme_service_->AddObserver(this);
    }
  }

  ~ThemeServiceObserverAdapter() override {
    if (theme_service_) {
      theme_service_->RemoveObserver(this);
    }
  }

  ThemeServiceObserverAdapter(const ThemeServiceObserverAdapter&) = delete;
  ThemeServiceObserverAdapter& operator=(const ThemeServiceObserverAdapter&) =
      delete;

  // ThemeServiceObserver:
  void OnThemeChanged() override {
    owner_->OnThemeChanged();
  }

 private:
  raw_ptr<AstraThemeService> owner_;
  raw_ptr<ThemeService> theme_service_;
};

// ===========================================================================
// WorkspaceObserverAdapter — observes AstraWorkspaceService
// ===========================================================================

// Observer for AstraWorkspaceService that updates the theme service
// when workspace state changes.
//
// Reacts to:
//   - Active workspace change → update accent color
//   - Workspace accent color change → update accent if it's the active ws
//   - Workspace removal → fall back to default workspace if needed
//
// TODO(astra): Add OnWorkspaceAccentColorChanged to AstraWorkspaceService
//   Observer so we can react to accent color changes on the active
//   workspace without re-checking on every workspace mutation.
//   Currently, we only react to OnActiveWorkspaceChanged and rely on
//   callers to manually trigger a refresh if they change a workspace's
//   accent color while it's active.
class AstraThemeService::WorkspaceObserverAdapter
    : public AstraWorkspaceServiceObserver {
 public:
  explicit WorkspaceObserverAdapter(AstraThemeService* owner,
                                    AstraWorkspaceService* workspace_service)
      : owner_(owner), workspace_service_(workspace_service) {
    DCHECK(owner_);
    if (workspace_service_) {
      workspace_service_->AddObserver(this);
    }
  }

  ~WorkspaceObserverAdapter() override {
    if (workspace_service_) {
      workspace_service_->RemoveObserver(this);
    }
  }

  WorkspaceObserverAdapter(const WorkspaceObserverAdapter&) = delete;
  WorkspaceObserverAdapter& operator=(const WorkspaceObserverAdapter&) = delete;

  // AstraWorkspaceServiceObserver:
  void OnWorkspaceAdded(const AstraWorkspace& /*workspace*/) override {
    // No action needed — adding a workspace doesn't change the active one.
  }

  void OnWorkspaceRemoved(const std::string& workspace_id) override {
    // If the removed workspace was the active theme workspace, fall back
    // to the default workspace's accent.
    //
    // Note: AstraWorkspaceService handles reassigning the active workspace
    // to default when the active one is removed, and fires
    // OnActiveWorkspaceChanged.  So we shouldn't need to handle this here.
    // We keep this method for completeness and in case we want to add
    // special behavior in the future.
    if (owner_->GetActiveWorkspaceId() == workspace_id) {
      owner_->InitializeAccentFromWorkspace();
    }
  }

  void OnWorkspaceRenamed(const std::string& /*workspace_id*/,
                          const std::string& /*new_name*/) override {
    // No action needed — renaming doesn't affect accent color.
  }

  void OnActiveWorkspaceChanged(const std::string& /*old_id*/,
                                const std::string& new_id) override {
    // When the active workspace changes, update the theme accent color
    // to match the new workspace's accent.
    //
    // This is the primary integration point: workspace switching drives
    // accent color changes in the theme system.
    owner_->SetActiveWorkspace(new_id);
  }

  void OnWorkspacesReordered() override {
    // No action needed — reordering doesn't affect accent color.
  }

 private:
  raw_ptr<AstraThemeService> owner_;
  raw_ptr<AstraWorkspaceService> workspace_service_;
};

// ===========================================================================
// Construction / destruction
// ===========================================================================

AstraThemeService::AstraThemeService(Profile* profile)
    : profile_(profile),
      accent_color_(kDefaultAccentColor),
      has_accent_override_(false),
      theme_preset_(AstraThemePreset::kSystem),
      use_workspace_accent_(true),
      custom_accent_color_(kDefaultCustomAccentColor),
      use_high_contrast_(false),
      show_accent_on_tabs_(true),
      show_accent_on_sidebar_(true),
      accent_intensity_(1.0),
      use_auto_theme_schedule_(false),
      auto_theme_light_start_("07:00"),
      auto_theme_dark_start_("19:00") {
  DCHECK(profile_);

  PrefService* prefs = profile->GetPrefs();

  // Load theme preset from prefs.
  if (prefs) {
    theme_preset_ = IntToThemePreset(prefs->GetInteger(kPrefThemePreset));
  }

  // Get the workspace service — we need it to look up workspace accent
  // colors and to observe workspace changes.
  //
  // The workspace service is expected to exist for any profile that has
  // a theme service.  If it doesn't (e.g. system profile), we just use
  // the default accent color and don't observe anything.
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);

  if (workspace_service) {
    workspace_observer_ = std::make_unique<WorkspaceObserverAdapter>(
        this, workspace_service);
  }

  // Get the Chromium ThemeService to observe theme changes.
  //
  // TODO(astra): Use ThemeServiceFactory::GetForProfile to get the
  //   ThemeService instance for this profile.  The exact include path
  //   and factory method may vary by Chromium version.
  //   Chromium owner: ThemeServiceFactory (chrome/browser/themes/
  //     theme_service_factory.h)
  ThemeService* theme_service = nullptr;
  // theme_service = ThemeServiceFactory::GetForProfile(profile);

  if (theme_service) {
    theme_observer_ = std::make_unique<ThemeServiceObserverAdapter>(
        this, theme_service);
  }

  // Load all presentation settings from prefs.
  LoadSettingsFromPrefs();

  // Initialize accent color:
  //   1. If there's a persisted accent override, use it.
  //   2. Otherwise, derive from the active workspace.
  if (prefs && !prefs->GetString(kPrefAccentOverride).empty()) {
    SkColor override_color = HexToSkColor(
        prefs->GetString(kPrefAccentOverride), kDefaultAccentColor);
    has_accent_override_ = true;
    accent_color_ = override_color;
    active_workspace_id_.clear();
  } else {
    InitializeAccentFromWorkspace();
  }
}

AstraThemeService::~AstraThemeService() = default;

void AstraThemeService::Shutdown() {
  // KeyedService shutdown: clear observer references and drop pointers
  // before the profile goes away.

  // Destroy observer adapters first so they remove themselves from
  // the observed services while those services are still valid.
  theme_observer_.reset();
  workspace_observer_.reset();

  observers_.Clear();
  profile_ = nullptr;
}

// ===========================================================================
// Static factory access
// ===========================================================================

// static
AstraThemeService* AstraThemeService::GetForProfile(Profile* profile) {
  return AstraThemeServiceFactory::GetForProfile(profile);
}

// ===========================================================================
// Observers
// ===========================================================================

void AstraThemeService::AddObserver(AstraThemeServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraThemeService::RemoveObserver(AstraThemeServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// ===========================================================================
// Accent color
// ===========================================================================

SkColor AstraThemeService::GetAccentColor() const {
  return accent_color_;
}

bool AstraThemeService::HasAccentOverride() const {
  return has_accent_override_;
}

void AstraThemeService::ClearAccentOverride() {
  if (!has_accent_override_)
    return;

  // Revert to workspace-derived accent color.
  has_accent_override_ = false;

  // Clear the persisted override.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefAccentOverride, std::string());
    }
  }

  // Re-initialize from workspace.
  InitializeAccentFromWorkspace();
}

void AstraThemeService::SetAccentColor(SkColor color) {
  if (color == accent_color_)
    return;

  has_accent_override_ = true;
  active_workspace_id_.clear();

  // Persist the override to profile prefs.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefAccentOverride, SkColorToHex(color));
    }
  }

  UpdateAccentColor(color);
}

bool AstraThemeService::SetAccentColorFromWorkspace(
    const std::string& workspace_id) {
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!workspace_service)
    return false;

  const AstraWorkspace* ws = workspace_service->GetWorkspace(workspace_id);
  if (!ws)
    return false;

  SkColor color = HexToSkColor(ws->accent_color, kDefaultAccentColor);

  has_accent_override_ = false;
  active_workspace_id_ = workspace_id;

  // Clear any persisted accent override.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefAccentOverride, std::string());
    }
  }

  UpdateAccentColor(color);
  return true;
}

// ===========================================================================
// Accent color palette
// ===========================================================================

// static
std::vector<SkColor> AstraThemeService::GetAccentColorPalette() {
  return std::vector<SkColor>(std::begin(kAccentPalette),
                              std::end(kAccentPalette));
}

// ===========================================================================
// Active workspace
// ===========================================================================

const std::string& AstraThemeService::GetActiveWorkspaceId() const {
  return active_workspace_id_;
}

void AstraThemeService::SetActiveWorkspace(const std::string& workspace_id) {
  if (workspace_id == active_workspace_id_ && !has_accent_override_)
    return;

  // Look up the workspace's accent color and apply it.
  // If the workspace doesn't exist, keep the current color.
  //
  // TODO(astra): Should we fall back to the default workspace color
  //   if the specified workspace doesn't exist?  For now, we silently
  //   ignore invalid workspace IDs.
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!workspace_service)
    return;

  const AstraWorkspace* ws = workspace_service->GetWorkspace(workspace_id);
  if (!ws)
    return;

  SkColor color = HexToSkColor(ws->accent_color, kDefaultAccentColor);

  has_accent_override_ = false;
  active_workspace_id_ = workspace_id;

  UpdateAccentColor(color);
}

// ===========================================================================
// Theme mode
// ===========================================================================

bool AstraThemeService::IsDarkMode() const {
  // Delegate to the utility function, which queries NativeTheme.
  //
  // TODO(astra): Also check ThemeService for user theme overrides.
  //   Some custom themes force light or dark mode regardless of the
  //   system setting.  ThemeService::UsingSystemTheme() and related
  //   methods can disambiguate this.
  //   Chromium owner: ThemeService (chrome/browser/themes/theme_service.h)
  return astra::IsDarkModeActive();
}

// ===========================================================================
// Theme preset
// ===========================================================================

void AstraThemeService::set_theme_preset(AstraThemePreset preset) {
  if (preset == theme_preset_)
    return;

  theme_preset_ = preset;

  // Persist to profile prefs.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetInteger(kPrefThemePreset, static_cast<int>(preset));
    }
  }

  NotifyThemePresetChanged();
}

// ===========================================================================
// Color utilities
// ===========================================================================

bool AstraThemeService::IsAccentColorDark() const {
  // Use relative luminance to determine if the accent is dark.
  // WCAG threshold of 0.179 is a common cutoff for "dark" colors.
  // Colors below this luminance are considered dark.
  return RelativeLuminance(accent_color_) < 0.179f;
}

SkColor AstraThemeService::GetLightenedAccent(float amount) const {
  // Clamp amount to valid range.
  amount = std::clamp(amount, 0.0f, 1.0f);

  // Blend toward white by the given amount.
  return BlendColors(accent_color_, SK_ColorWHITE, amount);
}

SkColor AstraThemeService::GetDarkenedAccent(float amount) const {
  // Clamp amount to valid range.
  amount = std::clamp(amount, 0.0f, 1.0f);

  // Blend toward black by the given amount.
  return BlendColors(accent_color_, SK_ColorBLACK, amount);
}

// ===========================================================================
// Contrast helpers
// ===========================================================================

SkColor AstraThemeService::GetAccentOnLight() const {
  // On a light background, ensure the accent has enough contrast.
  // If the accent is too light, return a darkened version.
  //
  // We use a luminance threshold: if the accent luminance is above
  // 0.5 (medium-bright), darken it to improve contrast on light bg.
  if (RelativeLuminance(accent_color_) > 0.5f) {
    return GetDarkenedAccent(0.3f);
  }
  return accent_color_;
}

SkColor AstraThemeService::GetAccentOnDark() const {
  // On a dark background, ensure the accent has enough contrast.
  // If the accent is too dark, return a lightened version.
  //
  // We use a luminance threshold: if the accent luminance is below
  // 0.3 (dark), lighten it to improve contrast on dark bg.
  if (RelativeLuminance(accent_color_) < 0.3f) {
    return GetLightenedAccent(0.3f);
  }
  return accent_color_;
}

// ===========================================================================
// Theme schemes
// ===========================================================================

// static
std::vector<AstraThemeScheme> AstraThemeService::GetThemeSchemes() {
  return std::vector<AstraThemeScheme>(std::begin(kThemeSchemes),
                                       std::end(kThemeSchemes));
}

bool AstraThemeService::ApplyThemeScheme(const std::string& scheme_name) {
  AstraThemeScheme scheme;
  if (!FindThemeScheme(scheme_name, &scheme))
    return false;

  // Check if this scheme is already active.
  if (active_scheme_ == scheme_name &&
      accent_color_ == scheme.accent_color) {
    return true;  // No-op: already applied.
  }

  // Apply the scheme's accent color.
  // This also sets has_accent_override_ = true and clears workspace ID.
  has_accent_override_ = true;
  active_workspace_id_.clear();
  use_workspace_accent_ = false;

  active_scheme_ = scheme_name;

  // Persist the scheme name.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefThemeScheme, scheme_name);
      prefs->SetString(kPrefAccentOverride, SkColorToHex(scheme.accent_color));
      prefs->SetBoolean(kPrefUseWorkspaceAccent, false);
    }
  }

  UpdateAccentColor(scheme.accent_color);
  NotifyThemeSchemeChanged();
  NotifyThemeSettingsChanged();

  return true;
}

// static
bool AstraThemeService::FindThemeScheme(const std::string& scheme_name,
                                        AstraThemeScheme* out_scheme) {
  if (scheme_name.empty())
    return false;

  for (const auto& scheme : kThemeSchemes) {
    if (scheme.name == scheme_name) {
      if (out_scheme) {
        *out_scheme = scheme;
      }
      return true;
    }
  }
  return false;
}

// ===========================================================================
// Additional color utilities
// ===========================================================================

SkColor AstraThemeService::GetAccentColorWithAlpha(int alpha) const {
  // Clamp alpha to valid range.
  alpha = ClampInt(alpha, 0, 255);
  return SkColorSetA(accent_color_, static_cast<U8CPU>(alpha));
}

SkColor AstraThemeService::GetSurfaceColor() const {
  return IsDarkMode() ? kSurfaceDark : kSurfaceLight;
}

SkColor AstraThemeService::GetTextPrimaryColor() const {
  return IsDarkMode() ? kTextPrimaryDark : kTextPrimaryLight;
}

SkColor AstraThemeService::GetTextSecondaryColor() const {
  return IsDarkMode() ? kTextSecondaryDark : kTextSecondaryLight;
}

SkColor AstraThemeService::GetBorderColor() const {
  return IsDarkMode() ? kBorderDark : kBorderLight;
}

SkColor AstraThemeService::GetHoverColor() const {
  // Hover color: lighten in dark mode, darken in light mode.
  // This ensures the hover state is visible against the surface.
  if (IsDarkMode()) {
    return GetLightenedAccent(0.15f);
  }
  return GetDarkenedAccent(0.1f);
}

// static
SkColor AstraThemeService::BlendColors(SkColor a, SkColor b, float t) {
  return astra::BlendColors(a, b, t);
}

// static
float AstraThemeService::GetContrastRatio(SkColor foreground,
                                          SkColor background) {
  float fg_lum = RelativeLuminance(foreground);
  float bg_lum = RelativeLuminance(background);
  return ContrastRatioFromLuminance(fg_lum, bg_lum);
}

// static
bool AstraThemeService::IsReadable(SkColor foreground, SkColor background) {
  return GetContrastRatio(foreground, background) >= kWcagAaContrastRatio;
}

// static
SkColor AstraThemeService::MakeReadable(SkColor foreground,
                                        SkColor background) {
  // If already readable, return as-is.
  if (IsReadable(foreground, background))
    return foreground;

  float fg_lum = RelativeLuminance(foreground);
  float bg_lum = RelativeLuminance(background);

  // Decide whether to lighten or darken the foreground.
  // If the background is dark, we lighten the foreground toward white.
  // If the background is light, we darken the foreground toward black.
  if (bg_lum < 0.5f) {
    // Dark background: lighten foreground.
    // Binary search approach: find the lighten amount that gives AA contrast.
    for (float amount = 0.1f; amount <= 1.0f; amount += 0.1f) {
      SkColor candidate = astra::BlendColors(foreground, SK_ColorWHITE, amount);
      if (IsReadable(candidate, background))
        return candidate;
    }
    return SK_ColorWHITE;  // Fallback: pure white.
  } else {
    // Light background: darken foreground.
    for (float amount = 0.1f; amount <= 1.0f; amount += 0.1f) {
      SkColor candidate = astra::BlendColors(foreground, SK_ColorBLACK, amount);
      if (IsReadable(candidate, background))
        return candidate;
    }
    return SK_ColorBLACK;  // Fallback: pure black.
  }
}

// ===========================================================================
// Presentation settings
// ===========================================================================

void AstraThemeService::set_use_workspace_accent(bool value) {
  if (value == use_workspace_accent_)
    return;

  use_workspace_accent_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefUseWorkspaceAccent, value);
    }
  }

  // If enabling workspace accent, revert to workspace accent.
  // If disabling, switch to custom accent color.
  if (value) {
    InitializeAccentFromWorkspace();
    has_accent_override_ = false;
  } else {
    UpdateAccentColor(custom_accent_color_);
    has_accent_override_ = true;
    active_workspace_id_.clear();
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_custom_accent_color(SkColor color) {
  if (color == custom_accent_color_)
    return;

  custom_accent_color_ = color;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetInteger(kPrefCustomAccentColor, SkColorToInt(color));
    }
  }

  // If we're using custom accent (not workspace), update the active color.
  if (!use_workspace_accent_) {
    UpdateAccentColor(color);
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_use_high_contrast(bool value) {
  if (value == use_high_contrast_)
    return;

  use_high_contrast_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefUseHighContrast, value);
    }
  }

  NotifyHighContrastChanged();
  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_show_accent_on_tabs(bool value) {
  if (value == show_accent_on_tabs_)
    return;

  show_accent_on_tabs_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefShowAccentOnTabs, value);
    }
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_show_accent_on_sidebar(bool value) {
  if (value == show_accent_on_sidebar_)
    return;

  show_accent_on_sidebar_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefShowAccentOnSidebar, value);
    }
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_accent_intensity(double value) {
  // Clamp to valid range.
  value = ClampDouble(value, 0.5, 2.0);

  if (value == accent_intensity_)
    return;

  accent_intensity_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetDouble(kPrefAccentIntensity, value);
    }
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_use_auto_theme_schedule(bool value) {
  if (value == use_auto_theme_schedule_)
    return;

  use_auto_theme_schedule_ = value;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefUseAutoThemeSchedule, value);
    }
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_auto_theme_light_start(const std::string& time_str) {
  if (time_str == auto_theme_light_start_)
    return;

  auto_theme_light_start_ = time_str;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefAutoThemeLightStart, time_str);
    }
  }

  NotifyThemeSettingsChanged();
}

void AstraThemeService::set_auto_theme_dark_start(const std::string& time_str) {
  if (time_str == auto_theme_dark_start_)
    return;

  auto_theme_dark_start_ = time_str;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetString(kPrefAutoThemeDarkStart, time_str);
    }
  }

  NotifyThemeSettingsChanged();
}

// ===========================================================================
// Bulk operations
// ===========================================================================

void AstraThemeService::ResetToDefaults() {
  // Reset all settings to defaults.
  use_workspace_accent_ = true;
  custom_accent_color_ = kDefaultCustomAccentColor;
  theme_preset_ = AstraThemePreset::kSystem;
  active_scheme_.clear();
  use_high_contrast_ = false;
  show_accent_on_tabs_ = true;
  show_accent_on_sidebar_ = true;
  accent_intensity_ = 1.0;
  use_auto_theme_schedule_ = false;
  auto_theme_light_start_ = "07:00";
  auto_theme_dark_start_ = "19:00";

  // Reset accent-related state.
  has_accent_override_ = false;

  // Reset prefs.
  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetBoolean(kPrefUseWorkspaceAccent, true);
      prefs->SetInteger(kPrefCustomAccentColor,
                        SkColorToInt(kDefaultCustomAccentColor));
      prefs->SetInteger(kPrefThemePreset,
                        static_cast<int>(AstraThemePreset::kSystem));
      prefs->SetString(kPrefThemeScheme, std::string());
      prefs->SetString(kPrefAccentOverride, std::string());
      prefs->SetBoolean(kPrefUseHighContrast, false);
      prefs->SetBoolean(kPrefShowAccentOnTabs, true);
      prefs->SetBoolean(kPrefShowAccentOnSidebar, true);
      prefs->SetDouble(kPrefAccentIntensity, 1.0);
      prefs->SetBoolean(kPrefUseAutoThemeSchedule, false);
      prefs->SetString(kPrefAutoThemeLightStart, "07:00");
      prefs->SetString(kPrefAutoThemeDarkStart, "19:00");
    }
  }

  // Reset accent to workspace default.
  InitializeAccentFromWorkspace();

  // Notify observers of the changes.
  NotifyAccentColorChanged();
  NotifyThemePresetChanged();
  NotifyThemeSchemeChanged();
  NotifyHighContrastChanged();
  NotifyThemeSettingsChanged();
}

void AstraThemeService::ApplyThemeSettings(const AstraThemeSettings& settings) {
  // Apply all settings, tracking which ones changed.
  bool accent_changed = false;
  bool preset_changed = false;
  bool scheme_changed = false;
  bool high_contrast_changed = false;
  bool any_changed = false;

  // Theme preset.
  if (settings.theme_preset != theme_preset_) {
    theme_preset_ = settings.theme_preset;
    preset_changed = true;
    any_changed = true;
  }

  // High contrast.
  if (settings.use_high_contrast != use_high_contrast_) {
    use_high_contrast_ = settings.use_high_contrast;
    high_contrast_changed = true;
    any_changed = true;
  }

  // Use workspace accent.
  if (settings.use_workspace_accent != use_workspace_accent_) {
    use_workspace_accent_ = settings.use_workspace_accent;
    any_changed = true;
  }

  // Custom accent color.
  if (settings.custom_accent_color != custom_accent_color_) {
    custom_accent_color_ = settings.custom_accent_color;
    any_changed = true;
  }

  // Theme scheme.
  if (settings.theme_scheme != active_scheme_) {
    active_scheme_ = settings.theme_scheme;
    scheme_changed = true;
    any_changed = true;
  }

  // Show accent on tabs.
  if (settings.show_accent_on_tabs != show_accent_on_tabs_) {
    show_accent_on_tabs_ = settings.show_accent_on_tabs;
    any_changed = true;
  }

  // Show accent on sidebar.
  if (settings.show_accent_on_sidebar != show_accent_on_sidebar_) {
    show_accent_on_sidebar_ = settings.show_accent_on_sidebar;
    any_changed = true;
  }

  // Accent intensity (clamped).
  double clamped_intensity = ClampDouble(settings.accent_intensity, 0.5, 2.0);
  if (clamped_intensity != accent_intensity_) {
    accent_intensity_ = clamped_intensity;
    any_changed = true;
  }

  // Auto theme schedule.
  if (settings.use_auto_theme_schedule != use_auto_theme_schedule_) {
    use_auto_theme_schedule_ = settings.use_auto_theme_schedule;
    any_changed = true;
  }

  if (settings.auto_theme_light_start != auto_theme_light_start_) {
    auto_theme_light_start_ = settings.auto_theme_light_start;
    any_changed = true;
  }

  if (settings.auto_theme_dark_start != auto_theme_dark_start_) {
    auto_theme_dark_start_ = settings.auto_theme_dark_start;
    any_changed = true;
  }

  // Update accent color based on the source setting.
  if (!use_workspace_accent_) {
    if (custom_accent_color_ != accent_color_) {
      UpdateAccentColor(custom_accent_color_);
      has_accent_override_ = true;
      active_workspace_id_.clear();
      accent_changed = true;
    }
  } else if (!accent_changed && has_accent_override_) {
    // Switching back to workspace accent.
    InitializeAccentFromWorkspace();
    has_accent_override_ = false;
    accent_changed = true;
  }

  // Save all to prefs.
  SaveSettingsToPrefs();

  // Notify observers.
  if (accent_changed)
    NotifyAccentColorChanged();
  if (preset_changed)
    NotifyThemePresetChanged();
  if (scheme_changed)
    NotifyThemeSchemeChanged();
  if (high_contrast_changed)
    NotifyHighContrastChanged();
  if (any_changed)
    NotifyThemeSettingsChanged();
}

AstraThemeSettings AstraThemeService::GetThemeSettings() const {
  AstraThemeSettings settings;
  settings.use_workspace_accent = use_workspace_accent_;
  settings.custom_accent_color = custom_accent_color_;
  settings.theme_preset = theme_preset_;
  settings.theme_scheme = active_scheme_;
  settings.use_high_contrast = use_high_contrast_;
  settings.show_accent_on_tabs = show_accent_on_tabs_;
  settings.show_accent_on_sidebar = show_accent_on_sidebar_;
  settings.accent_intensity = accent_intensity_;
  settings.use_auto_theme_schedule = use_auto_theme_schedule_;
  settings.auto_theme_light_start = auto_theme_light_start_;
  settings.auto_theme_dark_start = auto_theme_dark_start_;
  return settings;
}

// ===========================================================================
// Theme change notification
// ===========================================================================

void AstraThemeService::OnThemeChanged() {
  // The Chromium theme has changed — notify our observers.
  //
  // This is called by ThemeServiceObserverAdapter when the underlying
  // ThemeService fires its OnThemeChanged() notification.
  //
  // Accent color may or may not have changed — Chromium themes can
  // include custom accent colors.  For now, we assume the accent color
  // is driven by workspaces, not by Chromium themes.
  //
  // TODO(astra): Check if the new Chromium theme provides an accent
  //   color and decide whether it should override the workspace accent.
  //   Chromium themes can define a color for the toolbar, frame, etc.
  //   We may want to extract an accent from those or let the workspace
  //   accent take precedence.
  //   Chromium owner: ThemeService / ThemeService::GetColor()
  NotifyThemeChanged();
}

// ===========================================================================
// Internal helpers
// ===========================================================================

void AstraThemeService::InitializeAccentFromWorkspace() {
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!workspace_service) {
    accent_color_ = kDefaultAccentColor;
    active_workspace_id_.clear();
    return;
  }

  // Use the active workspace from the workspace service as our starting
  // point.  If there's no active workspace (shouldn't happen), fall back
  // to the first workspace or the default.
  const AstraWorkspace& active_ws = workspace_service->active_workspace();

  accent_color_ = HexToSkColor(active_ws.accent_color, kDefaultAccentColor);
  active_workspace_id_ = active_ws.id;
  has_accent_override_ = false;
}

void AstraThemeService::UpdateAccentColor(SkColor new_color) {
  if (new_color == accent_color_)
    return;

  accent_color_ = new_color;
  NotifyAccentColorChanged();

  // TODO(astra): Trigger a ColorProvider rebuild so that all Astra UI
  //   surfaces using kColorAstraWorkspaceAccent* IDs update automatically.
  //   This requires patching Chromium's ColorProvider refresh pipeline
  //   or calling ThemeService::NotifyThemeChanged() to force a refresh.
  //   Chromium owner: ColorProvider / ThemeService.
  //   Patch point: chrome/browser/themes/theme_service.cc —
  //     ThemeService::OnThemeChanged or equivalent refresh trigger.
}

void AstraThemeService::NotifyAccentColorChanged() {
  for (auto& observer : observers_) {
    observer.OnAccentColorChanged(accent_color_);
  }
}

void AstraThemeService::NotifyThemeChanged() {
  for (auto& observer : observers_) {
    observer.OnThemeChanged();
  }
}

void AstraThemeService::NotifyThemePresetChanged() {
  for (auto& observer : observers_) {
    observer.OnThemePresetChanged(theme_preset_);
  }
}

// ===========================================================================
// Additional notification methods
// ===========================================================================

void AstraThemeService::NotifyThemeSchemeChanged() {
  for (auto& observer : observers_) {
    observer.OnThemeSchemeChanged(active_scheme_);
  }
}

void AstraThemeService::NotifyHighContrastChanged() {
  for (auto& observer : observers_) {
    observer.OnHighContrastChanged(use_high_contrast_);
  }
}

void AstraThemeService::NotifyThemeSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnThemeSettingsChanged();
  }
}

// ===========================================================================
// Pref persistence helpers
// ===========================================================================

void AstraThemeService::LoadSettingsFromPrefs() {
  if (!profile_)
    return;

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs)
    return;

  use_workspace_accent_ = prefs->GetBoolean(kPrefUseWorkspaceAccent);
  custom_accent_color_ =
      IntToSkColor(prefs->GetInteger(kPrefCustomAccentColor));
  active_scheme_ = prefs->GetString(kPrefThemeScheme);
  use_high_contrast_ = prefs->GetBoolean(kPrefUseHighContrast);
  show_accent_on_tabs_ = prefs->GetBoolean(kPrefShowAccentOnTabs);
  show_accent_on_sidebar_ = prefs->GetBoolean(kPrefShowAccentOnSidebar);
  accent_intensity_ = ClampDouble(prefs->GetDouble(kPrefAccentIntensity),
                                  0.5, 2.0);
  use_auto_theme_schedule_ = prefs->GetBoolean(kPrefUseAutoThemeSchedule);
  auto_theme_light_start_ = prefs->GetString(kPrefAutoThemeLightStart);
  auto_theme_dark_start_ = prefs->GetString(kPrefAutoThemeDarkStart);
}

void AstraThemeService::SaveSettingsToPrefs() const {
  if (!profile_)
    return;

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs)
    return;

  prefs->SetBoolean(kPrefUseWorkspaceAccent, use_workspace_accent_);
  prefs->SetInteger(kPrefCustomAccentColor, SkColorToInt(custom_accent_color_));
  prefs->SetInteger(kPrefThemePreset, static_cast<int>(theme_preset_));
  prefs->SetString(kPrefThemeScheme, active_scheme_);
  prefs->SetBoolean(kPrefUseHighContrast, use_high_contrast_);
  prefs->SetBoolean(kPrefShowAccentOnTabs, show_accent_on_tabs_);
  prefs->SetBoolean(kPrefShowAccentOnSidebar, show_accent_on_sidebar_);
  prefs->SetDouble(kPrefAccentIntensity, accent_intensity_);
  prefs->SetBoolean(kPrefUseAutoThemeSchedule, use_auto_theme_schedule_);
  prefs->SetString(kPrefAutoThemeLightStart, auto_theme_light_start_);
  prefs->SetString(kPrefAutoThemeDarkStart, auto_theme_dark_start_);
}

}  // namespace astra
