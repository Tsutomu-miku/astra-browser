#include "astra/browser/astra_tab_stack_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_stack_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabStackServiceFactory::AstraTabStackServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraTabStackService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Stacks are a per-tab metadata concept.  In incognito mode,
              // stack state is ephemeral and does not persist — matching
              // the semantics of incognito.  Stack relationships are
              // isolated per browsing context.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {}

AstraTabStackServiceFactory::~AstraTabStackServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraTabStackServiceFactory* AstraTabStackServiceFactory::GetInstance() {
  static base::NoDestructor<AstraTabStackServiceFactory> instance;
  return instance.get();
}

// static
AstraTabStackService* AstraTabStackServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraTabStackService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraTabStackServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Register the stack list pref (stack metadata persistence).
  //
  // Stack metadata (name, color, order, collapsed state, note, pinned,
  // timestamps) is persisted via PrefService.  Per-tab stack membership
  // is stored on AstraTabFeatures (WebContentsUserData) and travels with
  // tabs through session restore.
  //
  // The list contains stack dictionaries with full stack metadata.
  //
  // Chromium owner: TabGroupModel persistence (analogous pattern)
  //   (chrome/browser/ui/tabs/tab_group_model.h)
  registry->RegisterListPref(prefs::kPrefTabStacks);

  // Register stack setting prefs via the service's static method.
  // Settings are defined as public static constexpr on the service class
  // so both the factory and service can reference them.
  AstraTabStackService::RegisterProfilePrefs(registry);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraTabStackServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraTabStackService>(profile);
}

}  // namespace astra
