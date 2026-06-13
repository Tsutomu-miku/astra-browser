// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_history_helper.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout. In this overlay repo, the
// types are forward-declared in the header and the real definitions
// come from Chromium at build time.
//
// Chromium owner: HistoryService
//   (components/history/core/browser/history_service.h)
// Chromium owner: URLRow
//   (components/history/core/browser/url_row.h)
// Chromium owner: VisitRow
//   (components/history/core/browser/visit_row.h)
// Chromium owner: TopSites
//   (components/history/core/browser/top_sites.h)
// #include "chrome/browser/history/history_service_factory.h"
// #include "components/history/core/browser/history_service.h"
// #include "components/history/core/browser/url_row.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Clamp helpers for history settings.
int ClampMaxResults(int value) {
  if (value < AstraHistoryHelper::kMinHistoryResults)
    return AstraHistoryHelper::kMinHistoryResults;
  if (value > AstraHistoryHelper::kMaxHistoryResults)
    return AstraHistoryHelper::kMaxHistoryResults;
  return value;
}

int ClampItemsPerDay(int value) {
  if (value < AstraHistoryHelper::kMinHistoryItemsPerDay)
    return AstraHistoryHelper::kMinHistoryItemsPerDay;
  if (value > AstraHistoryHelper::kMaxHistoryItemsPerDay)
    return AstraHistoryHelper::kMaxHistoryItemsPerDay;
  return value;
}

int ClampRetentionDays(int value) {
  if (value < AstraHistoryHelper::kMinRetentionDays)
    return AstraHistoryHelper::kMinRetentionDays;
  if (value > AstraHistoryHelper::kMaxRetentionDays)
    return AstraHistoryHelper::kMaxRetentionDays;
  return value;
}

// Returns midnight of the day containing |time| (local time).
base::Time StartOfDay(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  base::Time result;
  bool success = base::Time::FromLocalExploded(exploded, &result);
  if (!success) {
    // Fallback: truncate to day boundary using TimeDeltas.
    base::TimeDelta since_midnight =
        time - time.LocalMidnight();
    return time - since_midnight;
  }
  return result;
}

// Returns the start of the current day (midnight).
base::Time TodayStart() {
  return StartOfDay(base::Time::Now());
}

// Returns the start of the current week (Sunday midnight).
base::Time ThisWeekStart() {
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  // day_of_week: 0 = Sunday, 1 = Monday, ..., 6 = Saturday
  int days_since_sunday = exploded.day_of_week;
  base::Time week_start =
      StartOfDay(now) - base::Days(days_since_sunday);
  return week_start;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraHistoryHelper::AstraHistoryHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing HistoryService for live updates.
  //   Requires implementing history::HistoryServiceObserver.
  //   history::HistoryService* service = GetHistoryService();
  //   if (service) {
  //     service->AddObserver(this);
  //     is_observing_service_ = true;
  //   }
  //
  // Chromium observer: history::HistoryServiceObserver
  //   (components/history/core/browser/history_service_observer.h)
}

AstraHistoryHelper::~AstraHistoryHelper() {
  // Observers should already be cleaned up by Shutdown().
  DCHECK(!is_observing_service_);
}

void AstraHistoryHelper::Shutdown() {
  // TODO(astra): Remove observer from HistoryService.
  //   history::HistoryService* service = GetHistoryService();
  //   if (service && is_observing_service_) {
  //     service->RemoveObserver(this);
  //     is_observing_service_ = false;
  //   }
  is_observing_service_ = false;
  observers_.Clear();
  profile_ = nullptr;
}

// =========================================================================
// History item queries
// =========================================================================

int AstraHistoryHelper::GetHistoryItemCount() const {
  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return 0;
  }

  // TODO(astra): Query HistoryService for the total number of unique URLs.
  //   Chromium method: HistoryService::GetUniqueHostsCount or
  //   iterate through URL rows.
  //
  // For the overlay, return 0 as a placeholder.
  return 0;
}

int AstraHistoryHelper::GetVisitsToday() const {
  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return 0;
  }

  // TODO(astra): Query HistoryService for today's visit count.
  //   Use QueryHistory with today's time range and count visits.
  //
  // For the overlay, return 0 as a placeholder.
  return 0;
}

int AstraHistoryHelper::GetVisitsThisWeek() const {
  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return 0;
  }

  // TODO(astra): Query HistoryService for this week's visit count.
  //
  // For the overlay, return 0 as a placeholder.
  return 0;
}

std::vector<AstraHistoryItem> AstraHistoryHelper::GetMostVisited(
    int max_count) const {
  std::vector<AstraHistoryItem> result;

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return result;
  }

  // TODO(astra): Query TopSites / HistoryService for most visited URLs.
  //   Chromium owner: TopSites (components/history/core/browser/top_sites.h)
  //   or use HistoryService::QueryMostVisitedURLs.
  //
  // Real implementation sketch:
  //   - Get most visited URLs from TopSites or HistoryService
  //   - Project each URLRow into AstraHistoryItem
  //   - Sort by visit count descending
  //   - Limit to max_count
  //
  // For the overlay, return empty list.
  int effective_max = EffectiveMaxResults(max_count);
  if (effective_max <= 0) {
    return result;
  }

  return result;
}

std::vector<AstraHistoryItem> AstraHistoryHelper::GetRecentHistory(
    int max_count) const {
  std::vector<AstraHistoryItem> result;

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return result;
  }

  // TODO(astra): Query HistoryService for recent visits.
  //   Chromium method: HistoryService::QueryHistory with QueryOptions
  //     set to recent time range.
  //
  // For the overlay, return empty list.
  return result;
}

std::vector<AstraHistoryItem> AstraHistoryHelper::SearchHistory(
    const std::string& query,
    int max_results) const {
  std::vector<AstraHistoryItem> result;

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return result;
  }

  if (query.empty()) {
    return GetRecentHistory(max_results);
  }

  // TODO(astra): Use HistoryService's QueryHistory with text filter.
  //   Chromium method: HistoryService::QueryHistory
  //   with QueryOptions containing the search text.
  //
  // For the overlay, return empty list.
  return result;
}

std::vector<AstraHistoryItem> AstraHistoryHelper::GetHistoryForDay(
    base::Time day) const {
  base::Time day_start = StartOfDay(day);
  base::Time day_end = day_start + base::Days(1) - base::Microseconds(1);
  return GetHistoryForRange(day_start, day_end, 0);
}

std::vector<AstraHistoryItem> AstraHistoryHelper::GetHistoryForRange(
    base::Time begin,
    base::Time end,
    int max) const {
  std::vector<AstraHistoryItem> result;

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return result;
  }

  // TODO(astra): Query HistoryService for the time range.
  //   Chromium method: HistoryService::QueryHistory with
  //   QueryOptions set to the time range.
  //
  // For the overlay, return empty list.
  return result;
}

// =========================================================================
// URL-specific queries
// =========================================================================

base::Time AstraHistoryHelper::GetLastVisitTime(const GURL& url) const {
  if (!url.is_valid()) {
    return base::Time();
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return base::Time();
  }

  // TODO(astra): Query HistoryService for the last visit time.
  //   Chromium method: HistoryService::GetLastVisitTime or
  //   QueryURL with the specific URL.
  //
  // For the overlay, return null time.
  return base::Time();
}

int AstraHistoryHelper::GetVisitCount(const GURL& url) const {
  if (!url.is_valid()) {
    return 0;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return 0;
  }

  // TODO(astra): Query HistoryService for the visit count.
  //   Chromium: URLRow::visit_count()
  //
  // For the overlay, return 0.
  return 0;
}

bool AstraHistoryHelper::IsUrlInHistory(const GURL& url) const {
  if (!url.is_valid()) {
    return false;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return false;
  }

  // TODO(astra): Check if URL exists in HistoryService.
  //   Chromium method: HistoryService::QueryURL or similar.
  //
  // For the overlay, return false.
  return false;
}

std::vector<AstraHistoryItem> AstraHistoryHelper::GetTypedUrls(
    int max_count) const {
  std::vector<AstraHistoryItem> result;

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    return result;
  }

  // TODO(astra): Query HistoryService for typed URLs.
  //   Chromium: filter by ui::PAGE_TRANSITION_TYPED.
  //
  // For the overlay, return empty list.
  return result;
}

// =========================================================================
// History operations
// =========================================================================

void AstraHistoryHelper::RemoveHistoryItem(const GURL& url) {
  if (!url.is_valid()) {
    return;
  }

  if (!GetHistoryDeletionEnabled()) {
    return;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    // In the overlay, we can't actually remove from the history service,
    // but we still notify observers for testing purposes.
    NotifyHistoryItemRemoved(url);
    return;
  }

  // TODO(astra): Call HistoryService->ExpireHistoryForURLs() with the URL.
  //   Chromium method: HistoryService::ExpireHistoryForURLs
  //   (components/history/core/browser/history_service.h)
  //
  // After deletion, the HistoryServiceObserver will fire and we'll
  // notify our observers. For the overlay, we notify directly.
  NotifyHistoryItemRemoved(url);
}

void AstraHistoryHelper::RemoveHistoryForRange(base::Time begin,
                                               base::Time end) {
  if (!GetHistoryDeletionEnabled()) {
    return;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    NotifyHistoryExpired();
    return;
  }

  // TODO(astra): Call HistoryService->ExpireHistoryBetween().
  //   Chromium method: HistoryService::ExpireHistoryBetween
  //
  NotifyHistoryExpired();
}

void AstraHistoryHelper::ClearAllHistory() {
  if (!GetHistoryDeletionEnabled()) {
    return;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    NotifyHistoryItemsCleared();
    return;
  }

  // TODO(astra): Clear all history via HistoryService.
  //   Chromium method: ExpireHistoryBetween with full time range.
  //
  NotifyHistoryItemsCleared();
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraHistoryHelper::RemoveUrls(const std::vector<GURL>& urls) {
  if (!GetHistoryDeletionEnabled()) {
    return;
  }

  history::HistoryService* service = GetHistoryService();
  if (!service) {
    // Notify for each URL in the overlay.
    for (const auto& url : urls) {
      if (url.is_valid()) {
        NotifyHistoryItemRemoved(url);
      }
    }
    return;
  }

  // TODO(astra): Use HistoryService::ExpireHistoryForURLs with a vector.
  //
  for (const auto& url : urls) {
    if (url.is_valid()) {
      NotifyHistoryItemRemoved(url);
    }
  }
}

void AstraHistoryHelper::ClearHistoryForLastDays(int days) {
  if (days <= 0) {
    return;
  }

  base::Time now = base::Time::Now();
  base::Time begin = now - base::Days(days);
  RemoveHistoryForRange(begin, now);
}

void AstraHistoryHelper::DeleteOldHistory() {
  if (!GetHistoryDeletionEnabled()) {
    return;
  }

  int retention_days = GetHistoryRetentionDays();
  if (retention_days <= 0) {
    return;
  }

  base::Time cutoff = base::Time::Now() - base::Days(retention_days);
  base::Time min_time = base::Time::Min();

  // Delete everything older than the cutoff.
  RemoveHistoryForRange(min_time, cutoff);
}

void AstraHistoryHelper::ExpireHistoryByRetention() {
  if (!GetAutoDeleteHistory()) {
    return;
  }

  DeleteOldHistory();
}

// =========================================================================
// Presentation settings — show in sidebar
// =========================================================================

bool AstraHistoryHelper::GetShowHistoryInSidebar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryShowInSidebar;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryShowInSidebar);
}

void AstraHistoryHelper::SetShowHistoryInSidebar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryShowInSidebar) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryShowInSidebar, show);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleShowHistoryInSidebar() {
  bool new_state = !GetShowHistoryInSidebar();
  SetShowHistoryInSidebar(new_state);
  return GetShowHistoryInSidebar();
}

// =========================================================================
// Presentation settings — sort order
// =========================================================================

std::string AstraHistoryHelper::GetHistorySortOrder() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistorySortOrder;
  }
  return prefs->GetString(prefs::kPrefHistorySortOrder);
}

void AstraHistoryHelper::SetHistorySortOrder(const std::string& order) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetString(prefs::kPrefHistorySortOrder) == order) {
    return;
  }

  prefs->SetString(prefs::kPrefHistorySortOrder, order);
  NotifyHistorySettingsChanged();
}

// =========================================================================
// Presentation settings — max results
// =========================================================================

int AstraHistoryHelper::GetMaxHistoryResults() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultMaxHistoryResults;
  }
  return prefs->GetInteger(prefs::kPrefHistoryMaxResults);
}

void AstraHistoryHelper::SetMaxHistoryResults(int max_results) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampMaxResults(max_results);
  int current = prefs->GetInteger(prefs::kPrefHistoryMaxResults);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefHistoryMaxResults, clamped);
  NotifyHistorySettingsChanged();
}

// =========================================================================
// Presentation settings — show favicons
// =========================================================================

bool AstraHistoryHelper::GetShowHistoryFavicons() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryShowFavicons;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryShowFavicons);
}

void AstraHistoryHelper::SetShowHistoryFavicons(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryShowFavicons) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryShowFavicons, show);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleShowHistoryFavicons() {
  bool new_state = !GetShowHistoryFavicons();
  SetShowHistoryFavicons(new_state);
  return GetShowHistoryFavicons();
}

// =========================================================================
// Presentation settings — show visit count
// =========================================================================

bool AstraHistoryHelper::GetShowVisitCount() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryShowVisitCount;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryShowVisitCount);
}

void AstraHistoryHelper::SetShowVisitCount(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryShowVisitCount) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryShowVisitCount, show);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleShowVisitCount() {
  bool new_state = !GetShowVisitCount();
  SetShowVisitCount(new_state);
  return GetShowVisitCount();
}

// =========================================================================
// Presentation settings — show visit time
// =========================================================================

bool AstraHistoryHelper::GetShowVisitTime() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryShowVisitTime;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryShowVisitTime);
}

void AstraHistoryHelper::SetShowVisitTime(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryShowVisitTime) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryShowVisitTime, show);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleShowVisitTime() {
  bool new_state = !GetShowVisitTime();
  SetShowVisitTime(new_state);
  return GetShowVisitTime();
}

// =========================================================================
// Presentation settings — display mode
// =========================================================================

std::string AstraHistoryHelper::GetHistoryDisplayMode() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryDisplayMode;
  }
  return prefs->GetString(prefs::kPrefHistoryDisplayMode);
}

void AstraHistoryHelper::SetHistoryDisplayMode(const std::string& mode) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetString(prefs::kPrefHistoryDisplayMode) == mode) {
    return;
  }

  prefs->SetString(prefs::kPrefHistoryDisplayMode, mode);
  NotifyHistorySettingsChanged();
}

// =========================================================================
// Presentation settings — group by date
// =========================================================================

bool AstraHistoryHelper::GetGroupHistoryByDate() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryGroupByDate;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryGroupByDate);
}

void AstraHistoryHelper::SetGroupHistoryByDate(bool group) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryGroupByDate) == group) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryGroupByDate, group);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleGroupHistoryByDate() {
  bool new_state = !GetGroupHistoryByDate();
  SetGroupHistoryByDate(new_state);
  return GetGroupHistoryByDate();
}

// =========================================================================
// Presentation settings — items per day
// =========================================================================

int AstraHistoryHelper::GetHistoryItemsPerDay() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultHistoryItemsPerDay;
  }
  return prefs->GetInteger(prefs::kPrefHistoryItemsPerDay);
}

void AstraHistoryHelper::SetHistoryItemsPerDay(int items_per_day) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampItemsPerDay(items_per_day);
  int current = prefs->GetInteger(prefs::kPrefHistoryItemsPerDay);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefHistoryItemsPerDay, clamped);
  NotifyHistorySettingsChanged();
}

// =========================================================================
// Presentation settings — show typed URLs only
// =========================================================================

bool AstraHistoryHelper::GetShowTypedUrlsOnly() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryShowTypedUrlsOnly;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryShowTypedUrlsOnly);
}

void AstraHistoryHelper::SetShowTypedUrlsOnly(bool only_typed) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryShowTypedUrlsOnly) == only_typed) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryShowTypedUrlsOnly, only_typed);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleShowTypedUrlsOnly() {
  bool new_state = !GetShowTypedUrlsOnly();
  SetShowTypedUrlsOnly(new_state);
  return GetShowTypedUrlsOnly();
}

// =========================================================================
// Presentation settings — deletion enabled
// =========================================================================

bool AstraHistoryHelper::GetHistoryDeletionEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryDeletionEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryDeletionEnabled);
}

void AstraHistoryHelper::SetHistoryDeletionEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryDeletionEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryDeletionEnabled, enabled);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleHistoryDeletionEnabled() {
  bool new_state = !GetHistoryDeletionEnabled();
  SetHistoryDeletionEnabled(new_state);
  return GetHistoryDeletionEnabled();
}

// =========================================================================
// Presentation settings — auto delete
// =========================================================================

bool AstraHistoryHelper::GetAutoDeleteHistory() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultHistoryAutoDelete;
  }
  return prefs->GetBoolean(prefs::kPrefHistoryAutoDelete);
}

void AstraHistoryHelper::SetAutoDeleteHistory(bool auto_delete) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefHistoryAutoDelete) == auto_delete) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefHistoryAutoDelete, auto_delete);
  NotifyHistorySettingsChanged();
}

bool AstraHistoryHelper::ToggleAutoDeleteHistory() {
  bool new_state = !GetAutoDeleteHistory();
  SetAutoDeleteHistory(new_state);
  return GetAutoDeleteHistory();
}

// =========================================================================
// Presentation settings — retention days
// =========================================================================

int AstraHistoryHelper::GetHistoryRetentionDays() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultHistoryRetentionDays;
  }
  return prefs->GetInteger(prefs::kPrefHistoryRetentionDays);
}

void AstraHistoryHelper::SetHistoryRetentionDays(int days) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampRetentionDays(days);
  int current = prefs->GetInteger(prefs::kPrefHistoryRetentionDays);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefHistoryRetentionDays, clamped);
  NotifyHistorySettingsChanged();
}

// =========================================================================
// Utility methods
// =========================================================================

// static
std::u16string AstraHistoryHelper::FormatVisitTime(base::Time visit_time) {
  if (visit_time.is_null()) {
    return u"Never";
  }

  base::Time now = base::Time::Now();
  if (visit_time > now) {
    // Future time — shouldn't happen normally, but handle gracefully.
    return u"Just now";
  }

  base::TimeDelta delta = now - visit_time;

  // Less than 1 minute ago.
  if (delta < base::Minutes(1)) {
    return u"Just now";
  }

  // Less than 1 hour ago.
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    if (minutes <= 1) {
      return u"1 minute ago";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d minutes ago", minutes));
  }

  // Less than 1 day ago.
  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    if (hours <= 1) {
      return u"1 hour ago";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d hours ago", hours));
  }

  // Today (same calendar day).
  if (IsSameDay(visit_time, now)) {
    int hours = static_cast<int>(delta.InHours());
    if (hours <= 1) {
      return u"1 hour ago";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d hours ago", hours));
  }

  // Yesterday.
  base::Time yesterday_start = StartOfDay(now) - base::Days(1);
  base::Time yesterday_end = StartOfDay(now);
  if (visit_time >= yesterday_start && visit_time < yesterday_end) {
    return u"Yesterday";
  }

  // Less than 1 week ago.
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    if (days <= 1) {
      return u"Yesterday";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d days ago", days));
  }

  // Less than 1 month (approx 30 days).
  if (delta < base::Days(30)) {
    int days = static_cast<int>(delta.InDays());
    if (days < 7) {
      return base::UTF8ToUTF16(base::StringPrintf("%d days ago", days));
    }
    int weeks = days / 7;
    if (weeks <= 1) {
      return u"1 week ago";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d weeks ago", weeks));
  }

  // Less than 1 year.
  if (delta < base::Days(365)) {
    int months = static_cast<int>(delta.InDays() / 30);
    if (months <= 1) {
      return u"1 month ago";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d months ago", months));
  }

  // More than 1 year.
  int years = static_cast<int>(delta.InDays() / 365);
  if (years <= 1) {
    return u"1 year ago";
  }
  return base::UTF8ToUTF16(base::StringPrintf("%d years ago", years));
}

// static
std::u16string AstraHistoryHelper::FormatRelativeTime(base::TimeDelta delta) {
  if (delta.is_negative()) {
    return u"0 seconds";
  }

  if (delta < base::Seconds(1)) {
    return u"0 seconds";
  }

  if (delta < base::Minutes(1)) {
    int seconds = static_cast<int>(delta.InSeconds());
    if (seconds == 1) {
      return u"1 second";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d seconds", seconds));
  }

  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    if (minutes == 1) {
      return u"1 minute";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d minutes", minutes));
  }

  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    if (hours == 1) {
      return u"1 hour";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d hours", hours));
  }

  if (delta < base::Days(30)) {
    int days = static_cast<int>(delta.InDays());
    if (days == 1) {
      return u"1 day";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d days", days));
  }

  if (delta < base::Days(365)) {
    int months = static_cast<int>(delta.InDays() / 30);
    if (months == 0) {
      return u"1 month";
    }
    if (months == 1) {
      return u"1 month";
    }
    return base::UTF8ToUTF16(base::StringPrintf("%d months", months));
  }

  int years = static_cast<int>(delta.InDays() / 365);
  if (years == 1) {
    return u"1 year";
  }
  return base::UTF8ToUTF16(base::StringPrintf("%d years", years));
}

// static
std::u16string AstraHistoryHelper::TruncateTitle(const std::u16string& title,
                                                  int max_length) {
  if (max_length <= 0) {
    return std::u16string();
  }

  if (static_cast<int>(title.length()) <= max_length) {
    return title;
  }

  // Ellipsis takes 1 character, so we take max_length - 1 characters
  // from the title and append the ellipsis.
  if (max_length <= 1) {
    return u"…";
  }

  std::u16string result = title.substr(0, max_length - 1);
  result += u"…";
  return result;
}

// static
std::string AstraHistoryHelper::GetDomainName(const GURL& url) {
  if (!url.is_valid()) {
    return std::string();
  }

  std::string host = url.host();
  if (host.empty()) {
    return std::string();
  }

  // Remove "www." prefix if present.
  if (base::StartsWith(host, "www.", base::CompareCase::INSENSITIVE_ASCII)) {
    host = host.substr(4);
  }

  return host;
}

// static
bool AstraHistoryHelper::IsSameDay(base::Time a, base::Time b) {
  if (a.is_null() || b.is_null()) {
    return false;
  }

  base::Time::Exploded a_exploded;
  a.LocalExplode(&a_exploded);

  base::Time::Exploded b_exploded;
  b.LocalExplode(&b_exploded);

  return a_exploded.year == b_exploded.year &&
         a_exploded.month == b_exploded.month &&
         a_exploded.day_of_month == b_exploded.day_of_month;
}

// static
std::map<base::Time, std::vector<AstraHistoryItem>>
AstraHistoryHelper::GroupByDate(const std::vector<AstraHistoryItem>& items) {
  std::map<base::Time, std::vector<AstraHistoryItem>> groups;

  for (const auto& item : items) {
    if (item.visit_time.is_null()) {
      // Skip items with no visit time.
      continue;
    }

    base::Time day_start = StartOfDay(item.visit_time);

    // Insert into the appropriate day's group.
    auto it = groups.find(day_start);
    if (it == groups.end()) {
      std::vector<AstraHistoryItem> day_items;
      day_items.push_back(item);
      groups[day_start] = std::move(day_items);
    } else {
      it->second.push_back(item);
    }
  }

  // Sort each day's items by visit time (most recent first).
  for (auto& entry : groups) {
    std::sort(entry.second.begin(), entry.second.end(),
              [](const AstraHistoryItem& a, const AstraHistoryItem& b) {
                return a.visit_time > b.visit_time;
              });
  }

  return groups;
}

// =========================================================================
// Observers
// =========================================================================

void AstraHistoryHelper::AddObserver(AstraHistoryObserver* observer) {
  if (observer) {
    observers_.AddObserver(observer);
  }
}

void AstraHistoryHelper::RemoveObserver(AstraHistoryObserver* observer) {
  if (observer) {
    observers_.RemoveObserver(observer);
  }
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraHistoryHelper::NotifyHistoryItemAdded(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnHistoryItemAdded(url);
  }
}

void AstraHistoryHelper::NotifyHistoryItemRemoved(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnHistoryItemRemoved(url);
  }
}

void AstraHistoryHelper::NotifyHistoryItemsCleared() {
  for (auto& observer : observers_) {
    observer.OnHistoryItemsCleared();
  }
}

void AstraHistoryHelper::NotifyHistoryExpired() {
  for (auto& observer : observers_) {
    observer.OnHistoryExpired();
  }
}

void AstraHistoryHelper::NotifyHistorySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnHistorySettingsChanged();
  }
}

void AstraHistoryHelper::NotifyHistoryQueryCompleted(int query_id) {
  for (auto& observer : observers_) {
    observer.OnHistoryQueryCompleted(query_id);
  }
}

// =========================================================================
// Internal helpers
// =========================================================================

history::HistoryService* AstraHistoryHelper::GetHistoryService() const {
  if (!profile_) {
    return nullptr;
  }

  // TODO(astra): Use HistoryServiceFactory::GetForProfile() when building
  //   against the full Chromium source tree. In the overlay, we return
  //   nullptr as a placeholder since the real service factory isn't linked.
  //
  // Chromium factory: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  // The history service is a BrowserContextKeyedService, one per profile.
  //
  // Note: HistoryService is not available for off-the-record (incognito)
  // profiles. History is profile-scoped and shared across all windows
  // of the same profile.
  //
  // Patch point: None needed — we just call the existing factory.

  return nullptr;
}

PrefService* AstraHistoryHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

int AstraHistoryHelper::EffectiveMaxResults(int requested) const {
  if (requested > 0) {
    // Clamp requested to the hard limit.
    if (requested > kMaxHistoryResults) {
      return kMaxHistoryResults;
    }
    if (requested < kMinHistoryResults) {
      return kMinHistoryResults;
    }
    return requested;
  }
  // Use the configured pref value.
  return GetMaxHistoryResults();
}

}  // namespace astra
