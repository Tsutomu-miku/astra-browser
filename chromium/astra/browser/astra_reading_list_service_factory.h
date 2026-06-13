#ifndef ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraReadingListService;

// =========================================================================
// Factory for AstraReadingListService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraReadingListService.
//
// Incognito behavior: kRedirectedToOriginal — reading list entries are
// profile-level data that persists across sessions.  An incognito window
// reads and modifies the same reading list as the regular profile — only
// the browsing session is isolated.
//
// Guest: kOwnInstance — ephemeral reading list for guest sessions.
// System: kNone — system profile has no reading list.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// Note: The underlying ReadingListModel is owned by Chromium and obtained
// via its own factory.  AstraReadingListService is a thin projection layer
// that wraps ReadingListModel with Astra-specific observer translation.
// =========================================================================

class AstraReadingListServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraReadingListService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraReadingListService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraReadingListServiceFactory* GetInstance();

  // Registers reading-list-related profile prefs on the given registry.
  // The actual reading list data is stored by Chromium's ReadingListModel,
  // not by Astra.  This method registers Astra-specific presentation prefs.
  //
  // Chromium owner: ReadingListModel / reading_list_model_factory.cc
  //   (components/reading_list/core/)
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraReadingListServiceFactory>;

  AstraReadingListServiceFactory();
  ~AstraReadingListServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_FACTORY_H_
