#ifndef ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraTabStackService;

// =========================================================================
// Factory for AstraTabStackService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraTabStackService.
//
// Incognito behavior: kOwnInstance — stacks are a per-tab metadata concept.
// In incognito mode, stack state is ephemeral and does not persist —
// matching the semantics of incognito.  Stack relationships are isolated
// per browsing context.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// Chromium analog: TabGroupModelFactory
//   (chrome/browser/ui/tabs/tab_group_model_factory.h)
// =========================================================================

class AstraTabStackServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraTabStackService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraTabStackService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraTabStackServiceFactory* GetInstance();

  // Registers tab-stack-related profile prefs on the given registry.
  // Registers both the stack list pref and all stack setting prefs.
  //
  // Stack metadata (name, color, order, etc.) is persisted via
  // PrefService.  Tab membership travels with tabs through session restore.
  //
  // Setting prefs are defined as public static constexpr on
  // AstraTabStackService (e.g. AstraTabStackService::kPrefStackDefaultColor).
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraTabStackServiceFactory>;

  AstraTabStackServiceFactory();
  ~AstraTabStackServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_FACTORY_H_
