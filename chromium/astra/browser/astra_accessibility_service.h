#ifndef ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;
class PrefChangeRegistrar;

namespace content {
class BrowserContext;
}

namespace astra {

// =========================================================================
// Astra accessibility service
// =========================================================================
//
// Profile-scoped accessibility helper service.  Tracks accessibility
// settings and notifies Astra UI when accessibility settings change.
// Bridges Chromium's AccessibilityManager with Astra UI surfaces.
//
// This is a projection service: it does not own any accessibility state —
// Chromium's AccessibilityManager and PrefService are the source of truth.
// AstraAccessibilityService adapts Chromium accessibility state into a
// form convenient for Astra UI surfaces to observe and react to.
//
// Settings tracked:
//   - High contrast mode
//   - Reduced motion / reduced transitions
//   - Accessibility font scale
//   - Screen reader state
//   - Caret browsing
//   - Sticky keys
//   - Slow keys
//   - Mouse keys
//   - Large cursor
//   - Magnifier (docked / fullscreen)
//   - Select-to-speak
//   - Dictation
//   - Virtual keyboard
//   - Text helpers (minimum font size, weight, letter spacing, line height)
//   - Contrast level, night light, color temperature, color inversion
//   - Animation reduction level, auto-scroll, scroll speed
//
// Chromium subsystems reused:
//   - AccessibilityManager (chrome/browser/accessibility/)
//   - PrefService (components/prefs/)
//   - NativeTheme (ui/native_theme/) — high contrast detection
//
// TODO(astra): Proper integration with Chromium's AccessibilityManager.
//   Currently, this service reads from prefs and observes pref changes.
//   Full integration would observe AccessibilityManager for runtime
//   changes to accessibility features (e.g., screen reader detection,
//   caret browsing toggle from Chrome settings).
// Chromium owner: AccessibilityManager (chrome/browser/accessibility/
//   accessibility_manager.h)
// Patch point: AccessibilityManager::AddObserver() or the accessibility
//   pref change notification pipeline.
// =========================================================================

// Contrast level for Astra UI.
enum class AstraContrastLevel {
  kNormal = 0,
  kIncreased = 1,
  kHigh = 2,
};

// Magnifier display type.
enum class AstraMagnifierType {
  kDocked = 0,
  kFullscreen = 1,
};

// Animation reduction level.
enum class AstraAnimationReduction {
  kOff = 0,
  kSome = 1,
  kMax = 2,
};

// Accessibility preset.
// A preset is a bundle of accessibility settings optimized for a
// particular user need.  Applying a preset sets multiple settings at once.
enum class AstraAccessibilityPreset {
  kNone = 0,      // No accessibility features enabled.
  kVisual = 1,    // High contrast, large font, magnifier, large cursor.
  kMotor = 2,     // Sticky keys, slow keys, mouse keys, virtual keyboard.
  kCognitive = 3, // Reduced motion, text-to-speech, simplified UI.
  kCustom = 4,    // User-customized settings (not a specific preset).
};

// Observer interface for accessibility settings changes.
//
// Astra UI surfaces (sidebar, command palette, workspace overview, etc.)
// should implement this interface and register with the service to
// receive accessibility setting change notifications.
//
// All methods have empty default implementations so observers only need
// to override the ones they care about.
class AstraAccessibilityObserver : public base::CheckedObserver {
 public:
  // Called when high contrast mode is toggled on or off.
  virtual void OnHighContrastChanged(bool enabled) {}

  // Called when reduced motion / reduced transitions setting changes.
  virtual void OnReducedMotionChanged(bool enabled) {}

  // Called when the accessibility font scale factor changes.
  virtual void OnFontScaleChanged(double font_scale) {}

  // Called when screen reader state changes (detected or user-set).
  virtual void OnScreenReaderChanged(bool enabled) {}

  // Called when caret browsing mode changes.
  virtual void OnCaretBrowsingChanged(bool enabled) {}

  // Called when sticky keys mode changes.
  virtual void OnStickyKeysChanged(bool enabled) {}

  // Called when slow keys mode changes.
  virtual void OnSlowKeysChanged(bool enabled) {}

  // Called when mouse keys mode changes.
  virtual void OnMouseKeysChanged(bool enabled) {}

  // Called when large cursor mode changes.
  virtual void OnLargeCursorChanged(bool enabled) {}

  // Called when magnifier mode changes.
  virtual void OnMagnifierChanged(bool enabled) {}

  // Called when select-to-speak mode changes.
  virtual void OnSelectToSpeakChanged(bool enabled) {}

  // Called when dictation mode changes.
  virtual void OnDictationChanged(bool enabled) {}

  // Called when virtual keyboard mode changes.
  virtual void OnVirtualKeyboardChanged(bool enabled) {}

  // Called when the minimum font size changes.
  virtual void OnMinimumFontSizeChanged(int size) {}

  // Called when the contrast level changes.
  virtual void OnContrastLevelChanged(AstraContrastLevel level) {}

  // Called when night light mode changes.
  virtual void OnNightLightChanged(bool enabled) {}

  // Called when color inversion mode changes.
  virtual void OnColorInversionChanged(bool enabled) {}

  // Called when the animation reduction level changes.
  virtual void OnAnimationReductionChanged(AstraAnimationReduction level) {}

  // Called when any accessibility setting changes.
  // Use this for catch-all updates (e.g., full UI refresh).
  virtual void OnAccessibilitySettingsChanged() {}

 protected:
  ~AstraAccessibilityObserver() override = default;
};

class AstraAccessibilityService : public KeyedService {
 public:
  explicit AstraAccessibilityService(Profile* profile);
  AstraAccessibilityService(const AstraAccessibilityService&) = delete;
  AstraAccessibilityService& operator=(const AstraAccessibilityService&) =
      delete;
  ~AstraAccessibilityService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraAccessibilityObserver* observer);
  void RemoveObserver(AstraAccessibilityObserver* observer);

  // -- Settings queries ----------------------------------------------------

  // Returns true if high contrast mode is enabled.
  // Checks both the user pref and the system-level high contrast setting.
  bool IsHighContrastEnabled() const;

  // Returns true if reduced motion is preferred.
  // Checks both the user pref and the system-level prefers-reduced-motion
  // setting.
  bool IsReducedMotionEnabled() const;

  // Returns the accessibility font scale factor.
  // 1.0 = default, 1.5 = 50% larger, etc.
  double GetFontScale() const;

  // Returns true if a screen reader is detected or enabled.
  // TODO(astra): Integrate with Chromium's screen reader detection.
  // Chromium owner: AccessibilityManager::IsSpokenFeedbackEnabled()
  bool IsScreenReaderEnabled() const;

  // Returns true if caret browsing is enabled.
  bool IsCaretBrowsingEnabled() const;

  // Returns true if sticky keys is enabled.
  bool IsStickyKeysEnabled() const;

  // Returns true if slow keys is enabled.
  bool IsSlowKeysEnabled() const;

  // Returns the slow keys acceptance delay.
  base::TimeDelta GetSlowKeysDelay() const;

  // Returns true if mouse keys is enabled.
  bool IsMouseKeysEnabled() const;

  // Returns true if large cursor is enabled.
  bool IsLargeCursorEnabled() const;

  // Returns the large cursor size (1-5).
  int GetLargeCursorSize() const;

  // Returns true if magnifier is enabled.
  bool IsMagnifierEnabled() const;

  // Returns the magnifier scale factor (1.0-10.0).
  double GetMagnifierScale() const;

  // Returns the magnifier type (docked or fullscreen).
  AstraMagnifierType GetMagnifierType() const;

  // Returns true if select-to-speak is enabled.
  bool IsSelectToSpeakEnabled() const;

  // Returns true if dictation is enabled.
  bool IsDictationEnabled() const;

  // Returns true if virtual keyboard is enabled.
  bool IsVirtualKeyboardEnabled() const;

  // Returns true if any accessibility feature is active.
  // Useful for deciding whether to add extra focus rings or ARIA labels
  // that are unnecessary when no assistive technology is present.
  bool IsAccessibilityEnabled() const;

  // -- Text helpers --------------------------------------------------------

  // Returns the minimum font size in pixels.
  int GetMinimumFontSize() const;

  // Returns the font weight adjustment (-100 to 300).
  int GetFontWeightAdjustment() const;

  // Returns the letter spacing multiplier.
  double GetLetterSpacing() const;

  // Returns the line height multiplier.
  double GetLineHeight() const;

  // -- Color and contrast --------------------------------------------------

  // Returns the current contrast level.
  AstraContrastLevel GetContrastLevel() const;

  // Returns true if night light / warm colors is enabled.
  bool IsNightLightEnabled() const;

  // Returns the color temperature (0-100, warmness level).
  int GetColorTemperature() const;

  // Returns true if color inversion is enabled.
  bool IsColorInversionEnabled() const;

  // -- Animation and motion ------------------------------------------------

  // Returns the animation reduction level.
  AstraAnimationReduction GetAnimationReductionLevel() const;

  // Returns true if auto-scroll is enabled.
  bool IsAutoScrollEnabled() const;

  // Returns the scroll speed multiplier.
  double GetScrollSpeed() const;

  // -- Overall accessibility ----------------------------------------------

  // Returns the count of enabled accessibility features.
  int GetEnabledFeaturesCount() const;

  // Returns the list of names of enabled accessibility features.
  std::vector<std::string> GetEnabledFeatureList() const;

  // Resets all accessibility settings to their default values.
  void ResetAllAccessibilitySettings();

  // Applies an accessibility preset.
  // Note: Applying a preset other than kNone or kCustom sets the
  // corresponding preset's settings.  kCustom has no effect (it is
  // the state when the user has customized settings beyond a preset).
  void ApplyAccessibilityPreset(AstraAccessibilityPreset preset);

  // -- Settings mutation ---------------------------------------------------

  // Enables or disables high contrast mode for Astra UI.
  // Fires OnHighContrastChanged and OnAccessibilitySettingsChanged observers.
  void SetHighContrastEnabled(bool enabled);

  // Toggles high contrast mode.  Returns the new state.
  bool ToggleHighContrast();

  // Enables or disables reduced motion mode for Astra UI.
  // Fires OnReducedMotionChanged and OnAccessibilitySettingsChanged observers.
  void SetReducedMotionEnabled(bool enabled);

  // Toggles reduced motion mode.  Returns the new state.
  bool ToggleReducedMotion();

  // Sets the accessibility font scale factor.
  // Values are clamped to [0.5, 3.0].
  // Fires OnFontScaleChanged and OnAccessibilitySettingsChanged observers.
  void SetFontScale(double font_scale);

  // Resets the font scale to the default (1.0).
  void ResetFontScale();

  // Enables or disables caret browsing.
  void SetCaretBrowsingEnabled(bool enabled);

  // Toggles caret browsing.  Returns the new state.
  bool ToggleCaretBrowsing();

  // Enables or disables sticky keys.
  void SetStickyKeysEnabled(bool enabled);

  // Toggles sticky keys.  Returns the new state.
  bool ToggleStickyKeys();

  // Enables or disables slow keys.
  void SetSlowKeysEnabled(bool enabled);

  // Toggles slow keys.  Returns the new state.
  bool ToggleSlowKeys();

  // Sets the slow keys acceptance delay.
  // Values are clamped to [10ms, 5000ms].
  void SetSlowKeysDelay(base::TimeDelta delay);

  // Enables or disables mouse keys.
  void SetMouseKeysEnabled(bool enabled);

  // Toggles mouse keys.  Returns the new state.
  bool ToggleMouseKeys();

  // Enables or disables large cursor.
  void SetLargeCursorEnabled(bool enabled);

  // Toggles large cursor.  Returns the new state.
  bool ToggleLargeCursor();

  // Sets the large cursor size.
  // Values are clamped to [1, 5].
  void SetLargeCursorSize(int size);

  // Enables or disables magnifier.
  void SetMagnifierEnabled(bool enabled);

  // Toggles magnifier.  Returns the new state.
  bool ToggleMagnifier();

  // Sets the magnifier scale factor.
  // Values are clamped to [1.0, 10.0].
  void SetMagnifierScale(double scale);

  // Sets the magnifier type (docked or fullscreen).
  void SetMagnifierType(AstraMagnifierType type);

  // Enables or disables select-to-speak.
  void SetSelectToSpeakEnabled(bool enabled);

  // Toggles select-to-speak.  Returns the new state.
  bool ToggleSelectToSpeak();

  // Enables or disables dictation.
  void SetDictationEnabled(bool enabled);

  // Toggles dictation.  Returns the new state.
  bool ToggleDictation();

  // Enables or disables virtual keyboard.
  void SetVirtualKeyboardEnabled(bool enabled);

  // Toggles virtual keyboard.  Returns the new state.
  bool ToggleVirtualKeyboard();

  // Sets the minimum font size in pixels.
  // Values are clamped to [0, 72].
  void SetMinimumFontSize(int size);

  // Sets the font weight adjustment.
  // Values are clamped to [-100, 300].
  void SetFontWeightAdjustment(int adjustment);

  // Sets the letter spacing multiplier.
  // Values are clamped to [0.5, 3.0].
  void SetLetterSpacing(double multiplier);

  // Sets the line height multiplier.
  // Values are clamped to [0.8, 3.0].
  void SetLineHeight(double multiplier);

  // Sets the contrast level.
  void SetContrastLevel(AstraContrastLevel level);

  // Enables or disables night light / warm colors.
  void SetNightLightEnabled(bool enabled);

  // Toggles night light.  Returns the new state.
  bool ToggleNightLight();

  // Sets the color temperature.
  // Values are clamped to [0, 100].
  void SetColorTemperature(int temperature);

  // Enables or disables color inversion.
  void SetColorInversionEnabled(bool enabled);

  // Toggles color inversion.  Returns the new state.
  bool ToggleColorInversion();

  // Sets the animation reduction level.
  void SetAnimationReductionLevel(AstraAnimationReduction level);

  // Enables or disables auto-scroll.
  void SetAutoScrollEnabled(bool enabled);

  // Toggles auto-scroll.  Returns the new state.
  bool ToggleAutoScroll();

  // Sets the scroll speed multiplier.
  // Values are clamped to [0.25, 5.0].
  void SetScrollSpeed(double speed);

  // -- Utility methods -----------------------------------------------------

  // Returns a scaled font size based on the current font scale factor.
  // |base_size_pixels| is the base font size in pixels.
  int GetScaledFontSize(int base_size_pixels) const;

  // Returns whether animations should be enabled.
  // True if reduced motion is disabled (animations allowed) or if the system
  // prefers reduced motion.
  bool AreAnimationsEnabled() const;

  // Returns the list of available font scale presets.
  // Common presets: 1.0 (default), 1.25, 1.5, 1.75, 2.0.
  static std::vector<double> GetFontScalePresets();

  // Returns the font scale to the nearest preset value.
  // Returns the nearest preset value.
  static double SnapFontScaleToPreset(double font_scale);

 private:
  // Called when any accessibility pref changes.  Notifies observers.
  void OnPrefChanged(const std::string& pref_name);

  // Notifies all observers of the current accessibility state.
  void NotifyObservers();

  // Helper to set a boolean pref with idempotency check.
  void SetBoolPref(const char* pref_name, bool value);

  // Helper to set an integer pref with idempotency check.
  void SetIntPref(const char* pref_name, int value);

  // Helper to set a double pref with idempotency check.
  void SetDoublePref(const char* pref_name, double value, double epsilon = 0.001);

  raw_ptr<Profile> profile_;

  // Pref change observer — listens for accessibility pref changes.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  // Observers (Astra UI surfaces).
  base::ObserverList<AstraAccessibilityObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_H_
