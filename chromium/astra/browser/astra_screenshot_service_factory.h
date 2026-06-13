#ifndef ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraScreenshotService;

// =========================================================================
// Factory for AstraScreenshotService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraScreenshotService.
//
// Incognito behavior: kOwnInstance — screenshot operations are ephemeral and
// context-specific.  An incognito window has its own screenshot service
// instance to ensure that screenshot data (clipboard, observers) does not
// leak between incognito and regular profiles.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraScreenshotServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraScreenshotService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraScreenshotService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraScreenshotServiceFactory* GetInstance();

  // Registers screenshot-related profile prefs on the given registry.
  // Currently there are no persisted screenshot settings.
  //
  // TODO(astra): Register screenshot settings prefs (default format,
  //   save location preference, capture-with-browser-UI flag, etc.).
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraScreenshotServiceFactory>;

  AstraScreenshotServiceFactory();
  ~AstraScreenshotServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_FACTORY_H_
