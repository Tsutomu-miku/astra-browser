#ifndef ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraFocusModeService;

// =========================================================================
// Factory for AstraFocusModeService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraFocusModeService.
//
// Incognito behavior: kOwnInstance — focus mode is per-browsing-context.
// An incognito window has its own focus session that doesn't affect the
// main profile's active session.  Default duration and blocklist prefs
// are still shared with the original profile (via pref forwarding), but
// the active session state is per-instance.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraFocusModeServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraFocusModeService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraFocusModeService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraFocusModeServiceFactory* GetInstance();

  // Registers focus-mode-related profile prefs on the given registry.
  // Focus mode prefs include default duration, blocklist, and auto-start.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraFocusModeServiceFactory>;

  AstraFocusModeServiceFactory();
  ~AstraFocusModeServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_FACTORY_H_
