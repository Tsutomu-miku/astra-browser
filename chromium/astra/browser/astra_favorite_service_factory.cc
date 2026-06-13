#include "astra/browser/astra_favorite_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_favorite_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraFavoriteServiceFactory::AstraFavoriteServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraFavoriteService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Favorite folders are user-level metadata, not per-context.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to redirect to).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraFavoriteServiceFactory::~AstraFavoriteServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraFavoriteServiceFactory* AstraFavoriteServiceFactory::GetInstance() {
  static base::NoDestructor<AstraFavoriteServiceFactory> instance;
  return instance.get();
}

// static
AstraFavoriteService* AstraFavoriteServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraFavoriteService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraFavoriteServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // TODO(astra): Register AstraFavoriteService folder persistence prefs.
  //   Currently folder state is in-memory only; pref keys need to be
  //   defined (folder hierarchy, expanded state, order) and registered.
  //
  // Chromium owner: PrefService / PrefRegistry.
  // Patch point: factory pref registration pipeline.
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraFavoriteServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraFavoriteService>(profile);
}

}  // namespace astra
