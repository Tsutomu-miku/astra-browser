#include "astra/browser/astra_accessibility_service.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "base/observer_list.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "ui/native_theme/native_theme.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Minimum and maximum allowed font scale values.
constexpr double kMinFontScale = 0.5;
constexpr double kMaxFontScale = 3.0;

// Common font scale presets.
constexpr double kFontScalePresets[] = {
    0.75, 1.0, 1.25, 1.5, 1.75, 2.0,
};

// Slow keys delay bounds.
constexpr int kMinSlowKeysDelayMs = 10;
constexpr int kMaxSlowKeysDelayMs = 5000;

// Large cursor size bounds.
constexpr int kMinLargeCursorSize = 1;
constexpr int kMaxLargeCursorSize = 5;

// Magnifier scale bounds.
constexpr double kMinMagnifierScale = 1.0;
constexpr double kMaxMagnifierScale = 10.0;

// Minimum font size bounds.
constexpr int kMinMinimumFontSize = 0;
constexpr int kMaxMinimumFontSize = 72;

// Font weight adjustment bounds.
constexpr int kMinFontWeightAdjustment = -100;
constexpr int kMaxFontWeightAdjustment = 300;

// Letter spacing bounds.
constexpr double kMinLetterSpacing = 0.5;
constexpr double kMaxLetterSpacing = 3.0;

// Line height bounds.
constexpr double kMinLineHeight = 0.8;
constexpr double kMaxLineHeight = 3.0;

// Color temperature bounds.
constexpr int kMinColorTemperature = 0;
constexpr int kMaxColorTemperature = 100;

// Scroll speed bounds.
constexpr double kMinScrollSpeed = 0.25;
constexpr double kMaxScrollSpeed = 5.0;

// Total number of countable boolean accessibility features.
// Used by GetEnabledFeaturesCount and GetEnabledFeatureList.
// Features counted: high_contrast, reduced_motion, caret_browsing,
// sticky_keys, slow_keys, mouse_keys, large_cursor, magnifier,
// select_to_speak, dictation, virtual_keyboard, night_light,
// color_inversion, auto_scroll
constexpr int kTotalBooleanFeatures = 14;

// Clamp a value between min and max.
template <typename T>
T Clamp(T value, T min_val, T max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraAccessibilityService::AstraAccessibilityService(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);

  // Set up pref change observer.
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile_->GetPrefs());

  // Observe all accessibility prefs.
  const char* kObservedPrefs[] = {
    prefs::kPrefHighContrastMode,
    prefs::kPrefReducedMotion,
    prefs::kPrefAccessibilityFontScale,
    prefs::kPrefCaretBrowsingEnabled,
    prefs::kPrefStickyKeysEnabled,
    prefs::kPrefSlowKeysEnabled,
    prefs::kPrefSlowKeysDelayMs,
    prefs::kPrefMouseKeysEnabled,
    prefs::kPrefLargeCursorEnabled,
    prefs::kPrefLargeCursorSize,
    prefs::kPrefMagnifierEnabled,
    prefs::kPrefMagnifierScale,
    prefs::kPrefMagnifierType,
    prefs::kPrefSelectToSpeakEnabled,
    prefs::kPrefDictationEnabled,
    prefs::kPrefVirtualKeyboardEnabled,
    prefs::kPrefMinimumFontSize,
    prefs::kPrefFontWeightAdjustment,
    prefs::kPrefLetterSpacing,
    prefs::kPrefLineHeight,
    prefs::kPrefContrastLevel,
    prefs::kPrefNightLightEnabled,
    prefs::kPrefColorTemperature,
    prefs::kPrefColorInversionEnabled,
    prefs::kPrefAnimationReductionLevel,
    prefs::kPrefAutoScrollEnabled,
    prefs::kPrefScrollSpeed,
  };

  for (const char* pref : kObservedPrefs) {
    pref_change_registrar_->Add(
        pref,
        base::BindRepeating(&AstraAccessibilityService::OnPrefChanged,
                            base::Unretained(this)));
  }
}

AstraAccessibilityService::~AstraAccessibilityService() = default;

void AstraAccessibilityService::Shutdown() {
  // Clean up pref observation.
  pref_change_registrar_.reset();
  profile_ = nullptr;
}

// =========================================================================
// Observers
// =========================================================================

void AstraAccessibilityService::AddObserver(
    AstraAccessibilityObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraAccessibilityService::RemoveObserver(
    AstraAccessibilityObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Settings queries
// =========================================================================

bool AstraAccessibilityService::IsHighContrastEnabled() const {
  if (!profile_) {
    return false;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return false;
  }

  // Check user pref first.
  bool user_enabled = prefs->GetBoolean(prefs::kPrefHighContrastMode);
  if (user_enabled) {
    return true;
  }

  // Also check system-level high contrast.
  // TODO(astra): Use NativeTheme::UsesHighContrastColors() for system-level
  //   detection.  The system setting is the ground truth for high contrast.
  // Chromium owner: ui/native_theme/native_theme.h
  //   NativeTheme::UsesHighContrastColors()
  if (ui::NativeTheme::GetInstanceForNativeUi()->UsesHighContrastColors()) {
    return true;
  }

  return false;
}

bool AstraAccessibilityService::IsReducedMotionEnabled() const {
  if (!profile_) {
    return false;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return false;
  }

  // Check user pref first.
  bool user_enabled = prefs->GetBoolean(prefs::kPrefReducedMotion);
  if (user_enabled) {
    return true;
  }

  // Also check system-level prefers-reduced-motion.
  // TODO(astra): Use NativeTheme::prefers_reduced_transitions() for system-level
  //   detection.  This reflects the OS-level "reduce motion" setting.
  // Chromium owner: ui/native_theme/native_theme.h
  //   NativeTheme::prefers_reduced_transitions()
  if (ui::NativeTheme::GetInstanceForNativeUi()
          ->prefers_reduced_transitions()) {
    return true;
  }

  return false;
}

double AstraAccessibilityService::GetFontScale() const {
  if (!profile_) {
    return 1.0;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return 1.0;
  }

  return prefs->GetDouble(prefs::kPrefAccessibilityFontScale);
}

bool AstraAccessibilityService::IsScreenReaderEnabled() const {
  // TODO(astra): Integrate with Chromium's AccessibilityManager to detect
  //   screen readers.  Chromium has both user-set and auto-detected
  //   screen reader state.
  //
  // Chromium owner: AccessibilityManager
  //   (chrome/browser/accessibility/accessibility_manager.h)
  //   - IsSpokenFeedbackEnabled()
  //   - IsSelectToSpeakEnabled()
  //   - IsSwitchAccessEnabled()
  //
  // Patch point: AccessibilityManager::GetInstance() or observe via
  //   AccessibilityManager::AddObserver() / OnAccessibilityStatusEvent.
  //
  // For now, we return false as a placeholder.  The proper implementation
  // would delegate to Chromium's AccessibilityManager.
  return false;
}

bool AstraAccessibilityService::IsCaretBrowsingEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefCaretBrowsingEnabled);
}

bool AstraAccessibilityService::IsStickyKeysEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefStickyKeysEnabled);
}

bool AstraAccessibilityService::IsSlowKeysEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefSlowKeysEnabled);
}

base::TimeDelta AstraAccessibilityService::GetSlowKeysDelay() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return base::Milliseconds(prefs::kDefaultSlowKeysDelayMs);
  }
  return base::Milliseconds(
      profile_->GetPrefs()->GetInteger(prefs::kPrefSlowKeysDelayMs));
}

bool AstraAccessibilityService::IsMouseKeysEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefMouseKeysEnabled);
}

bool AstraAccessibilityService::IsLargeCursorEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefLargeCursorEnabled);
}

int AstraAccessibilityService::GetLargeCursorSize() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultLargeCursorSize;
  }
  int size = profile_->GetPrefs()->GetInteger(prefs::kPrefLargeCursorSize);
  return Clamp(size, kMinLargeCursorSize, kMaxLargeCursorSize);
}

bool AstraAccessibilityService::IsMagnifierEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefMagnifierEnabled);
}

double AstraAccessibilityService::GetMagnifierScale() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultMagnifierScale;
  }
  double scale = profile_->GetPrefs()->GetDouble(prefs::kPrefMagnifierScale);
  return Clamp(scale, kMinMagnifierScale, kMaxMagnifierScale);
}

AstraMagnifierType AstraAccessibilityService::GetMagnifierType() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return static_cast<AstraMagnifierType>(prefs::kDefaultMagnifierType);
  }
  int type = profile_->GetPrefs()->GetInteger(prefs::kPrefMagnifierType);
  if (type < 0 || type > static_cast<int>(AstraMagnifierType::kFullscreen)) {
    return AstraMagnifierType::kDocked;
  }
  return static_cast<AstraMagnifierType>(type);
}

bool AstraAccessibilityService::IsSelectToSpeakEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefSelectToSpeakEnabled);
}

bool AstraAccessibilityService::IsDictationEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefDictationEnabled);
}

bool AstraAccessibilityService::IsVirtualKeyboardEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefVirtualKeyboardEnabled);
}

bool AstraAccessibilityService::IsAccessibilityEnabled() const {
  return IsHighContrastEnabled() || IsReducedMotionEnabled() ||
         IsScreenReaderEnabled() || IsCaretBrowsingEnabled() ||
         GetFontScale() != 1.0 || IsStickyKeysEnabled() ||
         IsSlowKeysEnabled() || IsMouseKeysEnabled() ||
         IsLargeCursorEnabled() || IsMagnifierEnabled() ||
         IsSelectToSpeakEnabled() || IsDictationEnabled() ||
         IsVirtualKeyboardEnabled() || GetContrastLevel() != AstraContrastLevel::kNormal ||
         IsNightLightEnabled() || IsColorInversionEnabled() ||
         GetAnimationReductionLevel() != AstraAnimationReduction::kOff ||
         GetMinimumFontSize() > 0 || GetFontWeightAdjustment() != 0 ||
         GetLetterSpacing() != 1.0 || GetLineHeight() != 1.0 ||
         IsAutoScrollEnabled();
}

// =========================================================================
// Text helpers
// =========================================================================

int AstraAccessibilityService::GetMinimumFontSize() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultMinimumFontSize;
  }
  int size = profile_->GetPrefs()->GetInteger(prefs::kPrefMinimumFontSize);
  return Clamp(size, kMinMinimumFontSize, kMaxMinimumFontSize);
}

int AstraAccessibilityService::GetFontWeightAdjustment() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultFontWeightAdjustment;
  }
  int adjustment =
      profile_->GetPrefs()->GetInteger(prefs::kPrefFontWeightAdjustment);
  return Clamp(adjustment, kMinFontWeightAdjustment, kMaxFontWeightAdjustment);
}

double AstraAccessibilityService::GetLetterSpacing() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultLetterSpacing;
  }
  double spacing = profile_->GetPrefs()->GetDouble(prefs::kPrefLetterSpacing);
  return Clamp(spacing, kMinLetterSpacing, kMaxLetterSpacing);
}

double AstraAccessibilityService::GetLineHeight() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultLineHeight;
  }
  double height = profile_->GetPrefs()->GetDouble(prefs::kPrefLineHeight);
  return Clamp(height, kMinLineHeight, kMaxLineHeight);
}

// =========================================================================
// Color and contrast
// =========================================================================

AstraContrastLevel AstraAccessibilityService::GetContrastLevel() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return static_cast<AstraContrastLevel>(prefs::kDefaultContrastLevel);
  }
  int level = profile_->GetPrefs()->GetInteger(prefs::kPrefContrastLevel);
  if (level < 0 || level > static_cast<int>(AstraContrastLevel::kHigh)) {
    return AstraContrastLevel::kNormal;
  }
  return static_cast<AstraContrastLevel>(level);
}

bool AstraAccessibilityService::IsNightLightEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefNightLightEnabled);
}

int AstraAccessibilityService::GetColorTemperature() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultColorTemperature;
  }
  int temp = profile_->GetPrefs()->GetInteger(prefs::kPrefColorTemperature);
  return Clamp(temp, kMinColorTemperature, kMaxColorTemperature);
}

bool AstraAccessibilityService::IsColorInversionEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefColorInversionEnabled);
}

// =========================================================================
// Animation and motion
// =========================================================================

AstraAnimationReduction AstraAccessibilityService::GetAnimationReductionLevel()
    const {
  if (!profile_ || !profile_->GetPrefs()) {
    return static_cast<AstraAnimationReduction>(
        prefs::kDefaultAnimationReductionLevel);
  }
  int level =
      profile_->GetPrefs()->GetInteger(prefs::kPrefAnimationReductionLevel);
  if (level < 0 || level > static_cast<int>(AstraAnimationReduction::kMax)) {
    return AstraAnimationReduction::kOff;
  }
  return static_cast<AstraAnimationReduction>(level);
}

bool AstraAccessibilityService::IsAutoScrollEnabled() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefAutoScrollEnabled);
}

double AstraAccessibilityService::GetScrollSpeed() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultScrollSpeed;
  }
  double speed = profile_->GetPrefs()->GetDouble(prefs::kPrefScrollSpeed);
  return Clamp(speed, kMinScrollSpeed, kMaxScrollSpeed);
}

// =========================================================================
// Overall accessibility
// =========================================================================

int AstraAccessibilityService::GetEnabledFeaturesCount() const {
  int count = 0;
  if (IsHighContrastEnabled()) count++;
  if (IsReducedMotionEnabled()) count++;
  if (IsScreenReaderEnabled()) count++;
  if (IsCaretBrowsingEnabled()) count++;
  if (IsStickyKeysEnabled()) count++;
  if (IsSlowKeysEnabled()) count++;
  if (IsMouseKeysEnabled()) count++;
  if (IsLargeCursorEnabled()) count++;
  if (IsMagnifierEnabled()) count++;
  if (IsSelectToSpeakEnabled()) count++;
  if (IsDictationEnabled()) count++;
  if (IsVirtualKeyboardEnabled()) count++;
  if (IsNightLightEnabled()) count++;
  if (IsColorInversionEnabled()) count++;
  if (IsAutoScrollEnabled()) count++;
  if (GetFontScale() != 1.0) count++;
  if (GetContrastLevel() != AstraContrastLevel::kNormal) count++;
  if (GetAnimationReductionLevel() != AstraAnimationReduction::kOff) count++;
  if (GetMinimumFontSize() > 0) count++;
  if (GetFontWeightAdjustment() != 0) count++;
  if (GetLetterSpacing() != 1.0) count++;
  if (GetLineHeight() != 1.0) count++;
  return count;
}

std::vector<std::string> AstraAccessibilityService::GetEnabledFeatureList()
    const {
  std::vector<std::string> features;
  if (IsHighContrastEnabled()) features.push_back("high_contrast");
  if (IsReducedMotionEnabled()) features.push_back("reduced_motion");
  if (IsScreenReaderEnabled()) features.push_back("screen_reader");
  if (IsCaretBrowsingEnabled()) features.push_back("caret_browsing");
  if (IsStickyKeysEnabled()) features.push_back("sticky_keys");
  if (IsSlowKeysEnabled()) features.push_back("slow_keys");
  if (IsMouseKeysEnabled()) features.push_back("mouse_keys");
  if (IsLargeCursorEnabled()) features.push_back("large_cursor");
  if (IsMagnifierEnabled()) features.push_back("magnifier");
  if (IsSelectToSpeakEnabled()) features.push_back("select_to_speak");
  if (IsDictationEnabled()) features.push_back("dictation");
  if (IsVirtualKeyboardEnabled()) features.push_back("virtual_keyboard");
  if (IsNightLightEnabled()) features.push_back("night_light");
  if (IsColorInversionEnabled()) features.push_back("color_inversion");
  if (IsAutoScrollEnabled()) features.push_back("auto_scroll");
  if (GetFontScale() != 1.0) features.push_back("font_scale");
  if (GetContrastLevel() != AstraContrastLevel::kNormal)
    features.push_back("contrast_level");
  if (GetAnimationReductionLevel() != AstraAnimationReduction::kOff)
    features.push_back("animation_reduction");
  if (GetMinimumFontSize() > 0) features.push_back("minimum_font_size");
  if (GetFontWeightAdjustment() != 0) features.push_back("font_weight");
  if (GetLetterSpacing() != 1.0) features.push_back("letter_spacing");
  if (GetLineHeight() != 1.0) features.push_back("line_height");
  return features;
}

void AstraAccessibilityService::ResetAllAccessibilitySettings() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetBoolean(prefs::kPrefHighContrastMode, prefs::kDefaultHighContrastMode);
  prefs->SetBoolean(prefs::kPrefReducedMotion, prefs::kDefaultReducedMotion);
  prefs->SetDouble(prefs::kPrefAccessibilityFontScale, prefs::kDefaultAccessibilityFontScale);
  prefs->SetBoolean(prefs::kPrefCaretBrowsingEnabled, prefs::kDefaultCaretBrowsingEnabled);
  prefs->SetBoolean(prefs::kPrefStickyKeysEnabled, prefs::kDefaultStickyKeysEnabled);
  prefs->SetBoolean(prefs::kPrefSlowKeysEnabled, prefs::kDefaultSlowKeysEnabled);
  prefs->SetInteger(prefs::kPrefSlowKeysDelayMs, prefs::kDefaultSlowKeysDelayMs);
  prefs->SetBoolean(prefs::kPrefMouseKeysEnabled, prefs::kDefaultMouseKeysEnabled);
  prefs->SetBoolean(prefs::kPrefLargeCursorEnabled, prefs::kDefaultLargeCursorEnabled);
  prefs->SetInteger(prefs::kPrefLargeCursorSize, prefs::kDefaultLargeCursorSize);
  prefs->SetBoolean(prefs::kPrefMagnifierEnabled, prefs::kDefaultMagnifierEnabled);
  prefs->SetDouble(prefs::kPrefMagnifierScale, prefs::kDefaultMagnifierScale);
  prefs->SetInteger(prefs::kPrefMagnifierType, prefs::kDefaultMagnifierType);
  prefs->SetBoolean(prefs::kPrefSelectToSpeakEnabled, prefs::kDefaultSelectToSpeakEnabled);
  prefs->SetBoolean(prefs::kPrefDictationEnabled, prefs::kDefaultDictationEnabled);
  prefs->SetBoolean(prefs::kPrefVirtualKeyboardEnabled, prefs::kDefaultVirtualKeyboardEnabled);
  prefs->SetInteger(prefs::kPrefMinimumFontSize, prefs::kDefaultMinimumFontSize);
  prefs->SetInteger(prefs::kPrefFontWeightAdjustment, prefs::kDefaultFontWeightAdjustment);
  prefs->SetDouble(prefs::kPrefLetterSpacing, prefs::kDefaultLetterSpacing);
  prefs->SetDouble(prefs::kPrefLineHeight, prefs::kDefaultLineHeight);
  prefs->SetInteger(prefs::kPrefContrastLevel, prefs::kDefaultContrastLevel);
  prefs->SetBoolean(prefs::kPrefNightLightEnabled, prefs::kDefaultNightLightEnabled);
  prefs->SetInteger(prefs::kPrefColorTemperature, prefs::kDefaultColorTemperature);
  prefs->SetBoolean(prefs::kPrefColorInversionEnabled, prefs::kDefaultColorInversionEnabled);
  prefs->SetInteger(prefs::kPrefAnimationReductionLevel, prefs::kDefaultAnimationReductionLevel);
  prefs->SetBoolean(prefs::kPrefAutoScrollEnabled, prefs::kDefaultAutoScrollEnabled);
  prefs->SetDouble(prefs::kPrefScrollSpeed, prefs::kDefaultScrollSpeed);
}

void AstraAccessibilityService::ApplyAccessibilityPreset(
    AstraAccessibilityPreset preset) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  switch (preset) {
    case AstraAccessibilityPreset::kNone:
      ResetAllAccessibilitySettings();
      break;

    case AstraAccessibilityPreset::kVisual:
      // Visual preset: high contrast, large font, magnifier, large cursor.
      SetHighContrastEnabled(true);
      SetFontScale(1.5);
      SetMagnifierEnabled(true);
      SetMagnifierScale(2.0);
      SetLargeCursorEnabled(true);
      SetLargeCursorSize(4);
      SetContrastLevel(AstraContrastLevel::kHigh);
      break;

    case AstraAccessibilityPreset::kMotor:
      // Motor preset: sticky keys, slow keys, mouse keys, virtual keyboard.
      SetStickyKeysEnabled(true);
      SetSlowKeysEnabled(true);
      SetSlowKeysDelay(base::Milliseconds(500));
      SetMouseKeysEnabled(true);
      SetVirtualKeyboardEnabled(true);
      break;

    case AstraAccessibilityPreset::kCognitive:
      // Cognitive preset: reduced motion, text-to-speech, simplified UI.
      SetReducedMotionEnabled(true);
      SetAnimationReductionLevel(AstraAnimationReduction::kMax);
      SetSelectToSpeakEnabled(true);
      SetFontScale(1.25);
      SetLineHeight(1.5);
      break;

    case AstraAccessibilityPreset::kCustom:
      // Custom preset is a state indicator, not a set of settings to apply.
      // It represents user-customized settings.
      break;
  }
}

// =========================================================================
// Settings mutation
// =========================================================================

void AstraAccessibilityService::SetHighContrastEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefHighContrastMode, enabled);
}

bool AstraAccessibilityService::ToggleHighContrast() {
  bool new_state = !IsHighContrastEnabled();
  SetHighContrastEnabled(new_state);
  return IsHighContrastEnabled();
}

void AstraAccessibilityService::SetReducedMotionEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefReducedMotion, enabled);
}

bool AstraAccessibilityService::ToggleReducedMotion() {
  bool new_state = !IsReducedMotionEnabled();
  SetReducedMotionEnabled(new_state);
  return IsReducedMotionEnabled();
}

void AstraAccessibilityService::SetFontScale(double font_scale) {
  font_scale = Clamp(font_scale, kMinFontScale, kMaxFontScale);
  SetDoublePref(prefs::kPrefAccessibilityFontScale, font_scale);
}

void AstraAccessibilityService::ResetFontScale() {
  SetFontScale(1.0);
}

void AstraAccessibilityService::SetCaretBrowsingEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefCaretBrowsingEnabled, enabled);
}

bool AstraAccessibilityService::ToggleCaretBrowsing() {
  bool new_state = !IsCaretBrowsingEnabled();
  SetCaretBrowsingEnabled(new_state);
  return IsCaretBrowsingEnabled();
}

void AstraAccessibilityService::SetStickyKeysEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefStickyKeysEnabled, enabled);
}

bool AstraAccessibilityService::ToggleStickyKeys() {
  bool new_state = !IsStickyKeysEnabled();
  SetStickyKeysEnabled(new_state);
  return IsStickyKeysEnabled();
}

void AstraAccessibilityService::SetSlowKeysEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefSlowKeysEnabled, enabled);
}

bool AstraAccessibilityService::ToggleSlowKeys() {
  bool new_state = !IsSlowKeysEnabled();
  SetSlowKeysEnabled(new_state);
  return IsSlowKeysEnabled();
}

void AstraAccessibilityService::SetSlowKeysDelay(base::TimeDelta delay) {
  int ms = static_cast<int>(delay.InMilliseconds());
  ms = Clamp(ms, kMinSlowKeysDelayMs, kMaxSlowKeysDelayMs);
  SetIntPref(prefs::kPrefSlowKeysDelayMs, ms);
}

void AstraAccessibilityService::SetMouseKeysEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefMouseKeysEnabled, enabled);
}

bool AstraAccessibilityService::ToggleMouseKeys() {
  bool new_state = !IsMouseKeysEnabled();
  SetMouseKeysEnabled(new_state);
  return IsMouseKeysEnabled();
}

void AstraAccessibilityService::SetLargeCursorEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefLargeCursorEnabled, enabled);
}

bool AstraAccessibilityService::ToggleLargeCursor() {
  bool new_state = !IsLargeCursorEnabled();
  SetLargeCursorEnabled(new_state);
  return IsLargeCursorEnabled();
}

void AstraAccessibilityService::SetLargeCursorSize(int size) {
  size = Clamp(size, kMinLargeCursorSize, kMaxLargeCursorSize);
  SetIntPref(prefs::kPrefLargeCursorSize, size);
}

void AstraAccessibilityService::SetMagnifierEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefMagnifierEnabled, enabled);
}

bool AstraAccessibilityService::ToggleMagnifier() {
  bool new_state = !IsMagnifierEnabled();
  SetMagnifierEnabled(new_state);
  return IsMagnifierEnabled();
}

void AstraAccessibilityService::SetMagnifierScale(double scale) {
  scale = Clamp(scale, kMinMagnifierScale, kMaxMagnifierScale);
  SetDoublePref(prefs::kPrefMagnifierScale, scale);
}

void AstraAccessibilityService::SetMagnifierType(AstraMagnifierType type) {
  SetIntPref(prefs::kPrefMagnifierType, static_cast<int>(type));
}

void AstraAccessibilityService::SetSelectToSpeakEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefSelectToSpeakEnabled, enabled);
}

bool AstraAccessibilityService::ToggleSelectToSpeak() {
  bool new_state = !IsSelectToSpeakEnabled();
  SetSelectToSpeakEnabled(new_state);
  return IsSelectToSpeakEnabled();
}

void AstraAccessibilityService::SetDictationEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefDictationEnabled, enabled);
}

bool AstraAccessibilityService::ToggleDictation() {
  bool new_state = !IsDictationEnabled();
  SetDictationEnabled(new_state);
  return IsDictationEnabled();
}

void AstraAccessibilityService::SetVirtualKeyboardEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefVirtualKeyboardEnabled, enabled);
}

bool AstraAccessibilityService::ToggleVirtualKeyboard() {
  bool new_state = !IsVirtualKeyboardEnabled();
  SetVirtualKeyboardEnabled(new_state);
  return IsVirtualKeyboardEnabled();
}

void AstraAccessibilityService::SetMinimumFontSize(int size) {
  size = Clamp(size, kMinMinimumFontSize, kMaxMinimumFontSize);
  SetIntPref(prefs::kPrefMinimumFontSize, size);
}

void AstraAccessibilityService::SetFontWeightAdjustment(int adjustment) {
  adjustment =
      Clamp(adjustment, kMinFontWeightAdjustment, kMaxFontWeightAdjustment);
  SetIntPref(prefs::kPrefFontWeightAdjustment, adjustment);
}

void AstraAccessibilityService::SetLetterSpacing(double multiplier) {
  multiplier = Clamp(multiplier, kMinLetterSpacing, kMaxLetterSpacing);
  SetDoublePref(prefs::kPrefLetterSpacing, multiplier);
}

void AstraAccessibilityService::SetLineHeight(double multiplier) {
  multiplier = Clamp(multiplier, kMinLineHeight, kMaxLineHeight);
  SetDoublePref(prefs::kPrefLineHeight, multiplier);
}

void AstraAccessibilityService::SetContrastLevel(AstraContrastLevel level) {
  SetIntPref(prefs::kPrefContrastLevel, static_cast<int>(level));
}

void AstraAccessibilityService::SetNightLightEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefNightLightEnabled, enabled);
}

bool AstraAccessibilityService::ToggleNightLight() {
  bool new_state = !IsNightLightEnabled();
  SetNightLightEnabled(new_state);
  return IsNightLightEnabled();
}

void AstraAccessibilityService::SetColorTemperature(int temperature) {
  temperature = Clamp(temperature, kMinColorTemperature, kMaxColorTemperature);
  SetIntPref(prefs::kPrefColorTemperature, temperature);
}

void AstraAccessibilityService::SetColorInversionEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefColorInversionEnabled, enabled);
}

bool AstraAccessibilityService::ToggleColorInversion() {
  bool new_state = !IsColorInversionEnabled();
  SetColorInversionEnabled(new_state);
  return IsColorInversionEnabled();
}

void AstraAccessibilityService::SetAnimationReductionLevel(
    AstraAnimationReduction level) {
  SetIntPref(prefs::kPrefAnimationReductionLevel, static_cast<int>(level));
}

void AstraAccessibilityService::SetAutoScrollEnabled(bool enabled) {
  SetBoolPref(prefs::kPrefAutoScrollEnabled, enabled);
}

bool AstraAccessibilityService::ToggleAutoScroll() {
  bool new_state = !IsAutoScrollEnabled();
  SetAutoScrollEnabled(new_state);
  return IsAutoScrollEnabled();
}

void AstraAccessibilityService::SetScrollSpeed(double speed) {
  speed = Clamp(speed, kMinScrollSpeed, kMaxScrollSpeed);
  SetDoublePref(prefs::kPrefScrollSpeed, speed);
}

// =========================================================================
// Utility methods
// =========================================================================

int AstraAccessibilityService::GetScaledFontSize(int base_size_pixels) const {
  if (base_size_pixels <= 0) {
    return 0;
  }
  double scale = GetFontScale();
  int scaled = static_cast<int>(std::round(base_size_pixels * scale));
  return std::max(1, scaled);
}

bool AstraAccessibilityService::AreAnimationsEnabled() const {
  if (IsReducedMotionEnabled()) {
    return false;
  }
  if (GetAnimationReductionLevel() == AstraAnimationReduction::kMax) {
    return false;
  }
  return true;
}

// static
std::vector<double> AstraAccessibilityService::GetFontScalePresets() {
  return std::vector<double>(std::begin(kFontScalePresets),
                             std::end(kFontScalePresets));
}

// static
double AstraAccessibilityService::SnapFontScaleToPreset(double font_scale) {
  if (font_scale <= kFontScalePresets[0]) {
    return kFontScalePresets[0];
  }
  if (font_scale >= kFontScalePresets[std::size(kFontScalePresets) - 1]) {
    return kFontScalePresets[std::size(kFontScalePresets) - 1];
  }

  double nearest = kFontScalePresets[0];
  double min_diff = std::fabs(font_scale - nearest);
  for (size_t i = 1; i < std::size(kFontScalePresets); ++i) {
    double diff = std::fabs(font_scale - kFontScalePresets[i]);
    if (diff < min_diff) {
      min_diff = diff;
      nearest = kFontScalePresets[i];
    }
  }
  return nearest;
}

// =========================================================================
// Pref change handling
// =========================================================================

void AstraAccessibilityService::OnPrefChanged(const std::string& pref_name) {
  // Notify specific observers based on which pref changed.
  if (pref_name == prefs::kPrefHighContrastMode) {
    bool enabled = IsHighContrastEnabled();
    for (auto& observer : observers_) {
      observer.OnHighContrastChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefReducedMotion) {
    bool enabled = IsReducedMotionEnabled();
    for (auto& observer : observers_) {
      observer.OnReducedMotionChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefAccessibilityFontScale) {
    double scale = GetFontScale();
    for (auto& observer : observers_) {
      observer.OnFontScaleChanged(scale);
    }
  } else if (pref_name == prefs::kPrefCaretBrowsingEnabled) {
    bool enabled = IsCaretBrowsingEnabled();
    for (auto& observer : observers_) {
      observer.OnCaretBrowsingChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefStickyKeysEnabled) {
    bool enabled = IsStickyKeysEnabled();
    for (auto& observer : observers_) {
      observer.OnStickyKeysChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefSlowKeysEnabled) {
    bool enabled = IsSlowKeysEnabled();
    for (auto& observer : observers_) {
      observer.OnSlowKeysChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefMouseKeysEnabled) {
    bool enabled = IsMouseKeysEnabled();
    for (auto& observer : observers_) {
      observer.OnMouseKeysChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefLargeCursorEnabled) {
    bool enabled = IsLargeCursorEnabled();
    for (auto& observer : observers_) {
      observer.OnLargeCursorChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefMagnifierEnabled) {
    bool enabled = IsMagnifierEnabled();
    for (auto& observer : observers_) {
      observer.OnMagnifierChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefSelectToSpeakEnabled) {
    bool enabled = IsSelectToSpeakEnabled();
    for (auto& observer : observers_) {
      observer.OnSelectToSpeakChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefDictationEnabled) {
    bool enabled = IsDictationEnabled();
    for (auto& observer : observers_) {
      observer.OnDictationChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefVirtualKeyboardEnabled) {
    bool enabled = IsVirtualKeyboardEnabled();
    for (auto& observer : observers_) {
      observer.OnVirtualKeyboardChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefMinimumFontSize) {
    int size = GetMinimumFontSize();
    for (auto& observer : observers_) {
      observer.OnMinimumFontSizeChanged(size);
    }
  } else if (pref_name == prefs::kPrefContrastLevel) {
    AstraContrastLevel level = GetContrastLevel();
    for (auto& observer : observers_) {
      observer.OnContrastLevelChanged(level);
    }
  } else if (pref_name == prefs::kPrefNightLightEnabled) {
    bool enabled = IsNightLightEnabled();
    for (auto& observer : observers_) {
      observer.OnNightLightChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefColorInversionEnabled) {
    bool enabled = IsColorInversionEnabled();
    for (auto& observer : observers_) {
      observer.OnColorInversionChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefAnimationReductionLevel) {
    AstraAnimationReduction level = GetAnimationReductionLevel();
    for (auto& observer : observers_) {
      observer.OnAnimationReductionChanged(level);
    }
  }

  // Catch-all notification.
  NotifyObservers();
}

void AstraAccessibilityService::NotifyObservers() {
  for (auto& observer : observers_) {
    observer.OnAccessibilitySettingsChanged();
  }
}

// =========================================================================
// Private helper methods
// =========================================================================

void AstraAccessibilityService::SetBoolPref(const char* pref_name, bool value) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetBoolean(pref_name) == value) {
    return;
  }
  prefs->SetBoolean(pref_name, value);
}

void AstraAccessibilityService::SetIntPref(const char* pref_name, int value) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetInteger(pref_name) == value) {
    return;
  }
  prefs->SetInteger(pref_name, value);
}

void AstraAccessibilityService::SetDoublePref(const char* pref_name,
                                              double value,
                                              double epsilon) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (std::fabs(prefs->GetDouble(pref_name) - value) < epsilon) {
    return;
  }
  prefs->SetDouble(pref_name, value);
}

}  // namespace astra
