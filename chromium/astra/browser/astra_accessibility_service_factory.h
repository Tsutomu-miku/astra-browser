#ifndef ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

// =========================================================================
// Factory for AstraAccessibilityService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraAccessibilityService.
//
// Incognito behavior: kRedirectedToOriginal — accessibility settings are
// shared between regular and incognito profiles because they are a
// user-level preference, not a browsing-context preference.  High contrast,
// reduced motion, and font scale apply to all windows of the same profile.
//
// System profile: kNone — accessibility service is not needed for the
// system profile (which is used for background tasks and sign-in).
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

class AstraAccessibilityServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraAccessibilityService instance for |profile|.
  // Returns nullptr for system or guest profiles.
  static AstraAccessibilityService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraAccessibilityServiceFactory* GetInstance();

  // Registers accessibility-related profile prefs on the given registry.
  // Delegates to astra::prefs::RegisterProfilePrefs (which covers all Astra
  // prefs, including accessibility ones).  Also registers any
  // accessibility-specific prefs that are not part of the shared pref set.
  //
  // Chromium component: PrefRegistry / PrefService.
  // Patch point: called from factory registration or from a browser prefs
  // registration hook (e.g. chrome/browser/prefs/browser_prefs.cc).
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraAccessibilityServiceFactory>;

  AstraAccessibilityServiceFactory();
  ~AstraAccessibilityServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_ACCESSIBILITY_SERVICE_FACTORY_H_
