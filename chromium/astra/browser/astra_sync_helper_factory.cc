// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_sync_helper_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_sync_helper.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSyncHelperFactory::AstraSyncHelperFactory()
    : ProfileKeyedServiceFactory(
          "AstraSyncHelper",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Regular profile: redirect to original.
              // Incognito shares the original profile's sync state.
              // The helper reads from the original profile's SyncService.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest profile: own instance.
              // Guest profiles have their own isolated sync state.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no helper needed.
              // System profiles don't have user sync.
              .Build()) {}

AstraSyncHelperFactory::~AstraSyncHelperFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraSyncHelperFactory* AstraSyncHelperFactory::GetInstance() {
  static base::NoDestructor<AstraSyncHelperFactory> instance;
  return instance.get();
}

// static
AstraSyncHelper* AstraSyncHelperFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraSyncHelper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraSyncHelperFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // -- Core sync presentation settings --

  // Whether sync is enabled in Astra UI.
  // Default: true — sync is on by default (matching Chromium behavior).
  registry->RegisterBooleanPref(prefs::kPrefSyncEnabled,
                                prefs::kDefaultSyncEnabled);

  // Whether sync status is shown in Astra UI surfaces.
  // Default: true — status is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowSyncStatus,
                                prefs::kDefaultShowSyncStatus);

  // Whether sync errors are shown in Astra UI.
  // Default: true — errors are shown by default for visibility.
  registry->RegisterBooleanPref(prefs::kPrefShowSyncErrors,
                                prefs::kDefaultShowSyncErrors);

  // Whether sync only happens on WiFi.
  // Default: false — sync on any network by default.
  registry->RegisterBooleanPref(prefs::kPrefSyncOnWifiOnly,
                                prefs::kDefaultSyncOnWifiOnly);

  // Sync frequency setting.
  // Values: "auto", "hourly", "daily".
  // Default: "auto" — Chromium manages sync timing.
  registry->RegisterStringPref(prefs::kPrefSyncFrequency,
                               prefs::kDefaultSyncFrequency);

  // Whether automatic sync is enabled.
  // Default: true — auto sync is on by default.
  registry->RegisterBooleanPref(prefs::kPrefAutoSync,
                                prefs::kDefaultAutoSync);

  // Whether all sync data is encrypted.
  // Default: false — matches Chromium default behavior.
  registry->RegisterBooleanPref(prefs::kPrefEncryptAllData,
                                prefs::kDefaultEncryptAllData);

  // Whether the account avatar is shown in UI.
  // Default: true — avatar is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowAccountAvatar,
                                prefs::kDefaultShowAccountAvatar);

  // Whether last sync time is shown in UI.
  // Default: true — last sync time is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowLastSyncTime,
                                prefs::kDefaultShowLastSyncTime);

  // Whether the sync icon is shown in the toolbar.
  // Default: true — icon is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowSyncIconInToolbar,
                                prefs::kDefaultShowSyncIconInToolbar);

  // -- Data type enablement prefs --
  //
  // These are presentation preferences that track which data types
  // the user has enabled in the Astra UI. The actual sync enablement
  // is controlled by Chromium's SyncService.
  //
  // All data types are enabled by default, matching Chromium behavior.

  registry->RegisterBooleanPref(prefs::kPrefSyncBookmarks,
                                prefs::kDefaultSyncBookmarks);
  registry->RegisterBooleanPref(prefs::kPrefSyncPasswords,
                                prefs::kDefaultSyncPasswords);
  registry->RegisterBooleanPref(prefs::kPrefSyncHistory,
                                prefs::kDefaultSyncHistory);
  registry->RegisterBooleanPref(prefs::kPrefSyncTabs,
                                prefs::kDefaultSyncTabs);
  registry->RegisterBooleanPref(prefs::kPrefSyncSettings,
                                prefs::kDefaultSyncSettings);
  registry->RegisterBooleanPref(prefs::kPrefSyncExtensions,
                                prefs::kDefaultSyncExtensions);
  registry->RegisterBooleanPref(prefs::kPrefSyncAutofill,
                                prefs::kDefaultSyncAutofill);
  registry->RegisterBooleanPref(prefs::kPrefSyncReadingList,
                                prefs::kDefaultSyncReadingList);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraSyncHelperFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraSyncHelper>(profile);
}

}  // namespace astra
