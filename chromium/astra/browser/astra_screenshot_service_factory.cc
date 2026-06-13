#include "astra/browser/astra_screenshot_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_screenshot_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraScreenshotServiceFactory::AstraScreenshotServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraScreenshotService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Screenshot operations are ephemeral and context-specific.
              // An incognito window has its own screenshot service instance
              // to ensure that screenshot data (clipboard, observers) does
              // not leak between incognito and regular profiles.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraScreenshotServiceFactory::~AstraScreenshotServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraScreenshotServiceFactory* AstraScreenshotServiceFactory::GetInstance() {
  static base::NoDestructor<AstraScreenshotServiceFactory> instance;
  return instance.get();
}

// static
AstraScreenshotService* AstraScreenshotServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraScreenshotService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraScreenshotServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // TODO(astra): Register screenshot settings prefs (default format,
  //   save location preference, capture-with-browser-UI flag, etc.).
  //   Currently there are no persisted screenshot settings.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: registered via factory pref registration pipeline.
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraScreenshotServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraScreenshotService>(profile);
}

}  // namespace astra
