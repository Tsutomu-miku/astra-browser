#ifndef ASTRA_BROWSER_ASTRA_SAFETY_HELPER_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_SAFETY_HELPER_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraSafetyHelper;

// =========================================================================
// Factory for AstraSafetyHelper
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraSafetyHelper.
//
// Incognito behavior: kOwnInstance — safety settings are per-profile,
//   incognito inherits but can have its own presentation settings.
//
// Guest: kOwnInstance — guest sessions get their own safety state.
// System: kNone — system profile has no user-facing safety features.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraSafetyHelperFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraSafetyHelper instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraSafetyHelper* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraSafetyHelperFactory* GetInstance();

  // Registers safety-related profile prefs on the given registry.
  //
  // Safe browsing state is fully owned by Chromium's SafeBrowsingService.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: SafeBrowsingService
  //   (components/safe_browsing/core/browser/safe_browsing_service.h)
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraSafetyHelperFactory>;

  AstraSafetyHelperFactory();
  ~AstraSafetyHelperFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SAFETY_HELPER_FACTORY_H_
