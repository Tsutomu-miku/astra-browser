#ifndef ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraDownloadsHelper;

// =========================================================================
// Factory for AstraDownloadsHelper
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraDownloadsHelper.
//
// Incognito behavior: kRedirectedToOriginal — downloads are shared across
// regular and incognito windows in the same profile.  Downloads from
// incognito still go to the same download manager.
//
// Guest: kOwnInstance — guest sessions get their own download manager.
// System: kNone — system profile has no user downloads.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraDownloadsHelperFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraDownloadsHelper instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraDownloadsHelper* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraDownloadsHelperFactory* GetInstance();

  // Registers downloads-related profile prefs on the given registry.
  // Downloads prefs are part of the shared Astra pref set.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraDownloadsHelperFactory>;

  AstraDownloadsHelperFactory();
  ~AstraDownloadsHelperFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_FACTORY_H_
