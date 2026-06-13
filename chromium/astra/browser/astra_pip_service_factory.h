#ifndef ASTRA_BROWSER_ASTRA_PIP_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_PIP_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraPipService;

// =========================================================================
// Factory for AstraPipService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraPipService.
//
// Incognito behavior: kOwnInstance — PiP windows are per-browsing-context.
// An incognito window has its own PiP service instance that doesn't affect
// the main profile.  Preferences are still shared with the original profile
// (via pref forwarding) but active PiP state is per-instance.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraPipServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraPipService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraPipService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraPipServiceFactory* GetInstance();

  // Registers PiP-related profile prefs on the given registry.
  // PiP prefs include default size and always-on-top settings.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraPipServiceFactory>;

  AstraPipServiceFactory();
  ~AstraPipServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_PIP_SERVICE_FACTORY_H_
