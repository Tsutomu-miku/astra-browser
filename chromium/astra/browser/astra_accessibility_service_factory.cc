#include "astra/browser/astra_accessibility_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_accessibility_service.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraAccessibilityServiceFactory::AstraAccessibilityServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraAccessibilityService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Accessibility settings are user-level, not per-context.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to redirect to).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraAccessibilityServiceFactory::~AstraAccessibilityServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraAccessibilityServiceFactory*
AstraAccessibilityServiceFactory::GetInstance() {
  static base::NoDestructor<AstraAccessibilityServiceFactory> instance;
  return instance.get();
}

// static
AstraAccessibilityService*
AstraAccessibilityServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraAccessibilityService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraAccessibilityServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // High contrast mode for Astra UI surfaces.
  registry->RegisterBooleanPref(prefs::kPrefHighContrastMode,
                                prefs::kDefaultHighContrastMode);

  // Reduced motion mode for Astra UI surfaces.
  registry->RegisterBooleanPref(prefs::kPrefReducedMotion,
                                prefs::kDefaultReducedMotion);

  // Accessibility font scale factor for Astra UI surfaces.
  registry->RegisterDoublePref(prefs::kPrefAccessibilityFontScale,
                               prefs::kDefaultAccessibilityFontScale);

  // Caret browsing.
  registry->RegisterBooleanPref(prefs::kPrefCaretBrowsingEnabled,
                                prefs::kDefaultCaretBrowsingEnabled);

  // Sticky keys.
  registry->RegisterBooleanPref(prefs::kPrefStickyKeysEnabled,
                                prefs::kDefaultStickyKeysEnabled);

  // Slow keys.
  registry->RegisterBooleanPref(prefs::kPrefSlowKeysEnabled,
                                prefs::kDefaultSlowKeysEnabled);
  registry->RegisterIntegerPref(prefs::kPrefSlowKeysDelayMs,
                                prefs::kDefaultSlowKeysDelayMs);

  // Mouse keys.
  registry->RegisterBooleanPref(prefs::kPrefMouseKeysEnabled,
                                prefs::kDefaultMouseKeysEnabled);

  // Large cursor.
  registry->RegisterBooleanPref(prefs::kPrefLargeCursorEnabled,
                                prefs::kDefaultLargeCursorEnabled);
  registry->RegisterIntegerPref(prefs::kPrefLargeCursorSize,
                                prefs::kDefaultLargeCursorSize);

  // Magnifier.
  registry->RegisterBooleanPref(prefs::kPrefMagnifierEnabled,
                                prefs::kDefaultMagnifierEnabled);
  registry->RegisterDoublePref(prefs::kPrefMagnifierScale,
                               prefs::kDefaultMagnifierScale);
  registry->RegisterIntegerPref(prefs::kPrefMagnifierType,
                                prefs::kDefaultMagnifierType);

  // Select-to-speak.
  registry->RegisterBooleanPref(prefs::kPrefSelectToSpeakEnabled,
                                prefs::kDefaultSelectToSpeakEnabled);

  // Dictation.
  registry->RegisterBooleanPref(prefs::kPrefDictationEnabled,
                                prefs::kDefaultDictationEnabled);

  // Virtual keyboard.
  registry->RegisterBooleanPref(prefs::kPrefVirtualKeyboardEnabled,
                                prefs::kDefaultVirtualKeyboardEnabled);

  // Text helpers.
  registry->RegisterIntegerPref(prefs::kPrefMinimumFontSize,
                                prefs::kDefaultMinimumFontSize);
  registry->RegisterIntegerPref(prefs::kPrefFontWeightAdjustment,
                                prefs::kDefaultFontWeightAdjustment);
  registry->RegisterDoublePref(prefs::kPrefLetterSpacing,
                               prefs::kDefaultLetterSpacing);
  registry->RegisterDoublePref(prefs::kPrefLineHeight,
                               prefs::kDefaultLineHeight);

  // Contrast and color.
  registry->RegisterIntegerPref(prefs::kPrefContrastLevel,
                                prefs::kDefaultContrastLevel);
  registry->RegisterBooleanPref(prefs::kPrefNightLightEnabled,
                                prefs::kDefaultNightLightEnabled);
  registry->RegisterIntegerPref(prefs::kPrefColorTemperature,
                                prefs::kDefaultColorTemperature);
  registry->RegisterBooleanPref(prefs::kPrefColorInversionEnabled,
                                prefs::kDefaultColorInversionEnabled);

  // Animation and motion.
  registry->RegisterIntegerPref(prefs::kPrefAnimationReductionLevel,
                                prefs::kDefaultAnimationReductionLevel);
  registry->RegisterBooleanPref(prefs::kPrefAutoScrollEnabled,
                                prefs::kDefaultAutoScrollEnabled);
  registry->RegisterDoublePref(prefs::kPrefScrollSpeed,
                               prefs::kDefaultScrollSpeed);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraAccessibilityServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraAccessibilityService>(profile);
}

}  // namespace astra
