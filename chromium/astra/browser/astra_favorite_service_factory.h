#ifndef ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraFavoriteService;

// =========================================================================
// Factory for AstraFavoriteService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraFavoriteService.
//
// Incognito behavior: kRedirectedToOriginal — favorite folder metadata is
// a product-level concern that should reflect the user's main profile state.
// A separate incognito window still uses the same favorite folders — only
// the browsing context is isolated.  Guest sessions get their own instance
// (kOwnInstance) because they have no backing profile to redirect to.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// TODO(astra): Wire this into Chromium's profile keyed service registration
//   pipeline so the service is created at profile initialization time.
//   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
// =========================================================================

class AstraFavoriteServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraFavoriteService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraFavoriteService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraFavoriteServiceFactory* GetInstance();

  // Registers favorite-related profile prefs on the given registry.
  //
  // TODO(astra): Register favorite folder persistence prefs here.
  //   Currently folder state is in-memory only; pref keys need to be
  //   defined and registered.
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraFavoriteServiceFactory>;

  AstraFavoriteServiceFactory();
  ~AstraFavoriteServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_FACTORY_H_
