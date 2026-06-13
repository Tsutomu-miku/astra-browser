#include "astra/browser/astra_downloads_helper_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_downloads_helper.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraDownloadsHelperFactory::AstraDownloadsHelperFactory()
    : ProfileKeyedServiceFactory(
          "AstraDownloadsHelper",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Downloads are shared between regular and incognito windows
              // of the same profile.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance — guest sessions get their own
              // ephemeral downloads.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user downloads.
              .Build()) {}

AstraDownloadsHelperFactory::~AstraDownloadsHelperFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraDownloadsHelperFactory* AstraDownloadsHelperFactory::GetInstance() {
  static base::NoDestructor<AstraDownloadsHelperFactory> instance;
  return instance.get();
}

// static
AstraDownloadsHelper* AstraDownloadsHelperFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraDownloadsHelper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraDownloadsHelperFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Downloads presentation preferences.
  //
  // Download state is fully owned by Chromium's DownloadManager.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: DownloadManager (content/public/browser/)
  // Chromium factory: DownloadManager is accessed via BrowserContext.
  //
  // These Astra-specific prefs control only presentation — they never
  // store download data.  See AstraDownloadsHelper for the projection
  // layer that reads from DownloadManager.

  // Whether downloads are shown in the sidebar.
  // Default: true — downloads section is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsShowInSidebar,
                                prefs::kDefaultDownloadsShowInSidebar);

  // Whether download notifications are shown.
  // Default: true — notifications are shown by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsShowNotifications,
                                prefs::kDefaultDownloadsShowNotifications);

  // Whether downloads auto-open when complete.
  // Default: false — auto-open is off by default for safety.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsAutoOpen,
                                prefs::kDefaultDownloadsAutoOpen);

  // Download sort order.
  // Values: "newest_first" or "oldest_first".
  // Default: "newest_first" — most recent downloads first.
  registry->RegisterStringPref(prefs::kPrefDownloadsSortOrder,
                               prefs::kDefaultDownloadsSortOrder);

  // Maximum number of recent downloads to show in the sidebar.
  // Default: 20.
  // Clamped: 5 to 100.
  registry->RegisterIntegerPref(prefs::kPrefDownloadsMaxRecent,
                                prefs::kDefaultDownloadsMaxRecent);

  // Whether download speed is shown in the UI.
  // Default: true — speed is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsShowSpeed,
                                prefs::kDefaultDownloadsShowSpeed);

  // Whether file size is shown in the UI.
  // Default: true — file size is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsShowFileSize,
                                prefs::kDefaultDownloadsShowFileSize);

  // Whether download progress bar is shown.
  // Default: true — progress bar is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsShowProgress,
                                prefs::kDefaultDownloadsShowProgress);

  // Downloads display mode.
  // Values: "list" or "compact".
  // Default: "list" — full list view.
  registry->RegisterStringPref(prefs::kPrefDownloadsDisplayMode,
                               prefs::kDefaultDownloadsDisplayMode);

  // Whether to prompt the user for download location.
  // Default: true — ask where to save by default.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsPromptForLocation,
                                prefs::kDefaultDownloadsPromptForLocation);

  // Whether to show safe browsing warnings for dangerous downloads.
  // Default: true — warnings are shown by default for safety.
  registry->RegisterBooleanPref(prefs::kPrefDownloadsSafeBrowsingWarnings,
                                prefs::kDefaultDownloadsSafeBrowsingWarnings);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraDownloadsHelperFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraDownloadsHelper>(profile);
}

}  // namespace astra
