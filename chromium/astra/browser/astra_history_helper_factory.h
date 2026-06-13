#ifndef ASTRA_BROWSER_ASTRA_HISTORY_HELPER_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_HISTORY_HELPER_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraHistoryHelper;

// =========================================================================
// Factory for AstraHistoryHelper
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraHistoryHelper.
//
// History data is profile-scoped and owned by Chromium's HistoryService.
// The AstraHistoryHelper is a projection layer that sits on top of
// HistoryService and adds Astra-specific presentation preferences.
//
// Profile selections:
//   - Regular profile: kRedirectedToOriginal — history is shared across
//     regular and incognito (the service reads from the original profile's
//     history service; incognito browsing doesn't add to history).
//   - Guest: kOwnInstance — guest profiles have their own history.
//   - System: kNone — no history service for system profiles.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// Chromium reference: HistoryServiceFactory
//   (chrome/browser/history/history_service_factory.h)
// =========================================================================

class AstraHistoryHelperFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraHistoryHelper instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraHistoryHelper* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraHistoryHelperFactory* GetInstance();

  // Registers history-related profile prefs on the given registry.
  // History prefs include presentation settings (sort order, display mode,
  // max results, etc.) and retention policy settings.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraHistoryHelperFactory>;

  AstraHistoryHelperFactory();
  ~AstraHistoryHelperFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_HISTORY_HELPER_FACTORY_H_
