#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraWorkspaceService;

// =========================================================================
// Factory for AstraWorkspaceService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraWorkspaceService.
//
// Incognito behavior: kRedirectedToOriginal — workspace metadata is a
// product-level concern that should reflect the user's main profile state.
// A separate incognito window still uses the same workspace set; only the
// browsing context is isolated.  Guest sessions get their own instance
// (kOwnInstance) because they have no backing profile to redirect to.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// TODO(astra): Wire this into Chromium's profile keyed service registration
//   pipeline so the service is created at profile initialization time.
//   Two standard approaches:
//     1. Register via BrowserContextKeyedServiceFactory::RegisterFactory
//        in a chrome/browser/profiles/ patch.
//     2. Register explicitly via RegisterAstraProfileKeyedServices().
//   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
//   or the profile creation path.
// =========================================================================

class AstraWorkspaceServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraWorkspaceService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraWorkspaceService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraWorkspaceServiceFactory* GetInstance();

  // Registers workspace-related profile prefs on the given registry.
  // Delegates to astra::prefs::RegisterProfilePrefs which covers all Astra
  // prefs, including workspace ones.
  //
  // Chromium component: PrefRegistry / PrefService.
  // Patch point: called from factory registration or from a browser prefs
  // registration hook (e.g. chrome/browser/prefs/browser_prefs.cc).
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraWorkspaceServiceFactory>;

  AstraWorkspaceServiceFactory();
  ~AstraWorkspaceServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_FACTORY_H_
