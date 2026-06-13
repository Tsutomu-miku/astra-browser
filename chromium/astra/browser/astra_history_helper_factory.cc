// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_history_helper_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_history_helper.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraHistoryHelperFactory::AstraHistoryHelperFactory()
    : ProfileKeyedServiceFactory(
          "AstraHistoryHelper",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Regular profile: redirect to original.
              // Incognito shares the original profile's history.
              // The helper reads from the original profile's HistoryService.
              // Incognito browsing doesn't add to history, so the same
              // projection service works for both regular and OTR profiles.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest profile: own instance.
              // Guest profiles have their own isolated history.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no helper needed.
              // System profiles don't have browsing history.
              .Build()) {}

AstraHistoryHelperFactory::~AstraHistoryHelperFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraHistoryHelperFactory* AstraHistoryHelperFactory::GetInstance() {
  static base::NoDestructor<AstraHistoryHelperFactory> instance;
  return instance.get();
}

// static
AstraHistoryHelper* AstraHistoryHelperFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraHistoryHelper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraHistoryHelperFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Whether the history section is shown in the sidebar.
  // Default: true — history is a core sidebar section.
  registry->RegisterBooleanPref(prefs::kPrefHistoryShowInSidebar,
                                prefs::kDefaultHistoryShowInSidebar);

  // Default sort order for history listings.
  // Values: "time_desc", "time_asc", "most_visited".
  // Default: "time_desc" — most recent first.
  registry->RegisterStringPref(prefs::kPrefHistorySortOrder,
                               prefs::kDefaultHistorySortOrder);

  // Maximum number of history results per query.
  // Default: 50.
  // Clamped: 10-500.
  registry->RegisterIntegerPref(prefs::kPrefHistoryMaxResults,
                                prefs::kDefaultHistoryMaxResults);

  // Whether favicons are shown in history listings.
  // Default: true — favicons help with visual identification.
  registry->RegisterBooleanPref(prefs::kPrefHistoryShowFavicons,
                                prefs::kDefaultHistoryShowFavicons);

  // Whether visit counts are shown in history listings.
  // Default: false — visit count is secondary information.
  registry->RegisterBooleanPref(prefs::kPrefHistoryShowVisitCount,
                                prefs::kDefaultHistoryShowVisitCount);

  // Whether visit times are shown in history listings.
  // Default: true — timestamps are essential for history.
  registry->RegisterBooleanPref(prefs::kPrefHistoryShowVisitTime,
                                prefs::kDefaultHistoryShowVisitTime);

  // History display mode.
  // Values: "list", "compact", "card".
  // Default: "list" — standard list view.
  registry->RegisterStringPref(prefs::kPrefHistoryDisplayMode,
                               prefs::kDefaultHistoryDisplayMode);

  // Whether history results are grouped by date.
  // Default: true — grouping by date is a common history pattern.
  registry->RegisterBooleanPref(prefs::kPrefHistoryGroupByDate,
                                prefs::kDefaultHistoryGroupByDate);

  // Maximum number of history items per day group.
  // Default: 20.
  // Clamped: 5-100.
  registry->RegisterIntegerPref(prefs::kPrefHistoryItemsPerDay,
                                prefs::kDefaultHistoryItemsPerDay);

  // Whether only typed URLs are shown.
  // Default: false — show all history by default.
  registry->RegisterBooleanPref(prefs::kPrefHistoryShowTypedUrlsOnly,
                                prefs::kDefaultHistoryShowTypedUrlsOnly);

  // Whether history deletion is enabled.
  // Default: true — deletion is allowed by default.
  // When false, delete/clear operations are no-ops (for policy-restricted
  // environments or child accounts).
  registry->RegisterBooleanPref(prefs::kPrefHistoryDeletionEnabled,
                                prefs::kDefaultHistoryDeletionEnabled);

  // Whether old history is auto-deleted based on retention policy.
  // Default: false — off by default, user opts in.
  registry->RegisterBooleanPref(prefs::kPrefHistoryAutoDelete,
                                prefs::kDefaultHistoryAutoDelete);

  // History retention period in days.
  // Default: 90 days.
  // Clamped: 1-3650 (about 10 years).
  registry->RegisterIntegerPref(prefs::kPrefHistoryRetentionDays,
                                prefs::kDefaultHistoryRetentionDays);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraHistoryHelperFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraHistoryHelper>(profile);
}

}  // namespace astra
