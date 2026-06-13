#ifndef ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraNewTabPageService;

// =========================================================================
// Factory for AstraNewTabPageService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraNewTabPageService.
//
// Incognito behavior: kRedirectedToOriginal — NTP data (shortcuts,
// workspaces, theme, layout) is product-level state shared with the main
// profile.  An incognito new tab still shows the same shortcuts and
// workspaces — only the browsing session is isolated.
//
// Guest: kOwnInstance — ephemeral NTP data for guest sessions.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// Pref keys are defined as public static constexpr in
// AstraNewTabPageService (see astra_new_tab_page_service.h).
// =========================================================================

class AstraNewTabPageServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraNewTabPageService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraNewTabPageService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraNewTabPageServiceFactory* GetInstance();

  // Registers all NTP-related profile prefs on the given registry.
  // Uses the pref key constants defined in AstraNewTabPageService.
  //
  // Registers prefs for:
  //   - Managed shortcuts (list of dicts)
  //   - NTP layout preset
  //   - Theme settings (background color, image, dark mode)
  //   - Section visibility toggles (shortcuts, workspaces, suggestions, etc.)
  //   - Grid dimensions (shortcut columns/rows)
  //   - Workspace card visibility and order
  //   - Suggestions (dismissed list, enabled flag)
  //   - Legacy prefs (custom shortcuts, layout mode, background type)
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraNewTabPageServiceFactory>;

  AstraNewTabPageServiceFactory();
  ~AstraNewTabPageServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_FACTORY_H_
