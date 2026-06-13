#include "astra/browser/astra_note_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_note_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNoteServiceFactory::AstraNoteServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraNoteService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Notes are user-level state that persists across sessions.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance — ephemeral notes for guest sessions.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user notes.
              .Build()) {}

AstraNoteServiceFactory::~AstraNoteServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraNoteServiceFactory* AstraNoteServiceFactory::GetInstance() {
  static base::NoDestructor<AstraNoteServiceFactory> instance;
  return instance.get();
}

// static
AstraNoteService* AstraNoteServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraNoteService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraNoteServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Notes list: list of dicts, each describing one note.
  // Default: empty list — users start with no notes.
  // Notes are persisted per-profile via PrefService.
  //
  // These prefs live on the regular profile. Incognito redirects to the
  // original profile (see factory constructor), so incognito windows share
  // the same notes — only browsing context is isolated.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration (chrome/browser/profiles/).
  //
  // TODO(astra): Consider registering these as syncable prefs so notes
  // follow the user across devices.
  // Chromium subsystem: sync driver / PrefService sync integration.
  registry->RegisterListPref(AstraNoteService::kPrefNotes);

  // Default sort order for note lists.
  // Default: kDateDescending (most recent first).
  registry->RegisterIntegerPref(AstraNoteService::kPrefNoteSortOrder,
                                AstraNoteService::kDefaultNoteSortOrder);

  // Default note color.
  registry->RegisterStringPref(AstraNoteService::kPrefDefaultNoteColor,
                                AstraNoteService::kDefaultNoteColor);

  // Auto-save interval in seconds.
  registry->RegisterIntegerPref(
      AstraNoteService::kPrefAutoSaveIntervalSeconds,
      AstraNoteService::kDefaultAutoSaveIntervalSeconds);

  // Show word count.
  registry->RegisterBooleanPref(AstraNoteService::kPrefShowWordCount,
                                 AstraNoteService::kDefaultShowWordCount);

  // Default workspace for new notes.
  registry->RegisterStringPref(AstraNoteService::kPrefDefaultWorkspace,
                                AstraNoteService::kDefaultWorkspace);

  // Max search results.
  registry->RegisterIntegerPref(AstraNoteService::kPrefMaxSearchResults,
                                AstraNoteService::kDefaultMaxSearchResults);

  // Trash enabled.
  registry->RegisterBooleanPref(AstraNoteService::kPrefTrashEnabled,
                                 AstraNoteService::kDefaultTrashEnabled);

  // Auto-tag from page.
  registry->RegisterBooleanPref(AstraNoteService::kPrefAutoTagFromPage,
                                 AstraNoteService::kDefaultAutoTagFromPage);

  // Note font size.
  registry->RegisterIntegerPref(AstraNoteService::kPrefNoteFontSize,
                                AstraNoteService::kDefaultNoteFontSize);

  // Note line height.
  registry->RegisterDoublePref(AstraNoteService::kPrefNoteLineHeight,
                                AstraNoteService::kDefaultNoteLineHeight);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraNoteServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraNoteService>(profile);
}

}  // namespace astra
