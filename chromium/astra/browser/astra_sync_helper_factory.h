#ifndef ASTRA_BROWSER_ASTRA_SYNC_HELPER_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_SYNC_HELPER_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraSyncHelper;

// =========================================================================
// Factory for AstraSyncHelper
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraSyncHelper.
//
// Sync state is profile-scoped and owned by Chromium's SyncService.
// The AstraSyncHelper is a projection layer that sits on top of
// SyncService and adds Astra-specific presentation preferences.
//
// Profile selections:
//   - Regular profile: kRedirectedToOriginal — sync is shared across
//     regular and incognito (the sync service is per-profile and
//     incognito uses the original profile's sync state).
//   - Guest: kOwnInstance — guest profiles have their own sync state.
//   - System: kNone — no sync helper for system profiles.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// Chromium reference: ProfileSyncServiceFactory
//   (chrome/browser/sync/profile_sync_service_factory.h)
// =========================================================================

class AstraSyncHelperFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraSyncHelper instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraSyncHelper* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraSyncHelperFactory* GetInstance();

  // Registers sync-related profile prefs on the given registry.
  // Sync prefs include presentation settings (show status, show errors,
  // wifi only, frequency, etc.) and data type enablement prefs.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraSyncHelperFactory>;

  AstraSyncHelperFactory();
  ~AstraSyncHelperFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SYNC_HELPER_FACTORY_H_
