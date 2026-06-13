#ifndef ASTRA_BROWSER_ASTRA_NOTE_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_NOTE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraNoteService;

// =========================================================================
// Factory for AstraNoteService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraNoteService.
//
// Incognito behavior: kRedirectedToOriginal — notes are user-level state
// that persists across sessions.  An incognito window still has access to
// the user's notes — only the browsing context is isolated.
//
// Guest: kOwnInstance — guest sessions get their own ephemeral notes.
// System: kNone — system profile has no user notes.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraNoteServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraNoteService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraNoteService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraNoteServiceFactory* GetInstance();

  // Registers note-related profile prefs on the given registry.
  // Notes prefs (kPrefNotes) are part of the shared Astra pref set.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraNoteServiceFactory>;

  AstraNoteServiceFactory();
  ~AstraNoteServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NOTE_SERVICE_FACTORY_H_
