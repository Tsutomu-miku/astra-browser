#include "astra/browser/astra_pip_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_pip_service.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraPipServiceFactory::AstraPipServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraPipService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // PiP windows are per-browsing-context.  An incognito window
              // has its own PiP service instance that doesn't affect the
              // main profile.  Preferences are still shared with the
              // original profile (via pref forwarding) but active PiP
              // state is per-instance.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraPipServiceFactory::~AstraPipServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraPipServiceFactory* AstraPipServiceFactory::GetInstance() {
  static base::NoDestructor<AstraPipServiceFactory> instance;
  return instance.get();
}

// static
AstraPipService* AstraPipServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraPipService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraPipServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Default PiP size preset: "small", "medium", or "large".
  // Default: "medium" — the standard video PiP size.
  registry->RegisterStringPref(prefs::kPrefPiPDefaultSize,
                               prefs::kDefaultPiPDefaultSize);

  // Whether PiP windows are always-on-top.
  // Default: true — PiP windows should stay above other windows.
  registry->RegisterBooleanPref(prefs::kPrefPiPAlwaysOnTop,
                                prefs::kDefaultPiPAlwaysOnTop);

  // Default snap position for new PiP windows.
  registry->RegisterStringPref(prefs::kPrefPiPSnapPosition,
                               prefs::kDefaultPiPSnapPosition);

  // Whether snap-to-corner is enabled for PiP windows.
  registry->RegisterBooleanPref(prefs::kPrefPiPSnapToCornerEnabled,
                                prefs::kDefaultPiPSnapToCornerEnabled);

  // PiP window opacity.
  registry->RegisterDoublePref(prefs::kPrefPiPOpacity,
                               prefs::kDefaultPiPOpacity);

  // Whether auto-PiP on tab switch is enabled.
  registry->RegisterBooleanPref(prefs::kPrefPiPAutoPipOnTabSwitch,
                                prefs::kDefaultPiPAutoPipOnTabSwitch);

  // Maximum number of concurrent PiP windows.
  registry->RegisterIntegerPref(prefs::kPrefPiPMaxWindows,
                                prefs::kDefaultPiPMaxWindows);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraPipServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraPipService>(profile);
}

}  // namespace astra
