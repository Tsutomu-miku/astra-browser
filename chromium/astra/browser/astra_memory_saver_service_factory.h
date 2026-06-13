#ifndef ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraMemorySaverService;

// =========================================================================
// Factory for AstraMemorySaverService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraMemorySaverService.
//
// Incognito behavior: kOwnInstance — memory saver is per-profile instance.
// An incognito window has its own memory saver that manages only the
// incognito tabs.  Suspension state is per-tab (WebContentsUserData) and
// does not carry across incognito / regular boundaries.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraMemorySaverServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraMemorySaverService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraMemorySaverService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraMemorySaverServiceFactory* GetInstance();

  // Registers memory-saver-related profile prefs on the given registry.
  // Memory saver prefs include enabled state, timeout, and active-workspace
  // suspension policy.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraMemorySaverServiceFactory>;

  AstraMemorySaverServiceFactory();
  ~AstraMemorySaverServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_FACTORY_H_
