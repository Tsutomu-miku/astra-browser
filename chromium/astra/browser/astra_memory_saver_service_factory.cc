#include "astra/browser/astra_memory_saver_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_memory_saver_service.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraMemorySaverServiceFactory::AstraMemorySaverServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraMemorySaverService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Memory saver is per-profile instance.  An incognito window
              // has its own memory saver that manages only incognito tabs.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraMemorySaverServiceFactory::~AstraMemorySaverServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraMemorySaverServiceFactory* AstraMemorySaverServiceFactory::GetInstance() {
  static base::NoDestructor<AstraMemorySaverServiceFactory> instance;
  return instance.get();
}

// static
AstraMemorySaverService* AstraMemorySaverServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraMemorySaverService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraMemorySaverServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Memory saver prefs are registered as part of the shared Astra pref set.
  // See astra::prefs::RegisterProfilePrefs in astra_prefs.h.
  //
  // TODO(astra): Register all memory saver prefs here instead of relying on
  //   the shared pref registration.  Each service factory should own the
  //   registration of its own prefs, following Chromium's pattern.
  // Chromium pattern: each service's factory has a RegisterProfilePrefs()
  //   method called during profile initialization.
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraMemorySaverServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraMemorySaverService>(profile);
}

}  // namespace astra
