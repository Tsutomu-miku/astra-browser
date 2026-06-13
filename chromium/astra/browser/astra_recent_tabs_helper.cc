#include "astra/browser/astra_recent_tabs_helper.h"

#include "base/i18n/case_conversion.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_prefs.h"

// TODO(astra): These includes reference Chromium headers that are only
// available in a full Chromium checkout.  In this overlay repo, the types
// are forward-declared in the header (astra_recent_tabs_helper.h) and
// the real definitions come from Chromium at build time.
//
// The real implementation would include:
//   #include "chrome/browser/sessions/tab_restore_service.h"
//   #include "chrome/browser/sessions/tab_restore_service_factory.h"
//   #include "chrome/browser/sessions/tab_restore_service_observer.h"
//   #include "chrome/browser/sessions/session_service.h"
//   #include "chrome/browser/sessions/session_service_factory.h"
//   #include "components/sessions/core/tab_restore_types.h"
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
// Chromium factory: TabRestoreServiceFactory
//   (chrome/browser/sessions/tab_restore_service_factory.h)
// Chromium owner: sessions::SessionService
//   (chrome/browser/sessions/session_service.h)

namespace astra {

namespace {

// Placeholder returned when TabRestoreService is not available.
constexpr size_t kEmptyEntryCount = 0;

// Placeholder stats when SessionService is not available.
constexpr size_t kEmptyTotalClosedCount = 0;
constexpr size_t kEmptySessionCount = 0;

// Helper to convert a string to lowercase for case-insensitive comparison.
std::u16string ToLower(const std::u16string& s) {
  return base::i18n::ToLower(s);
}

// Lowercase a GURL's spec for search matching.
std::u16string UrlToLower(const GURL& url) {
  if (!url.is_valid())
    return std::u16string();
  // Use host + path for matching, converted to lowercase.
  std::u16string host16;
  // For simplicity in the overlay, we use the spec.
  // TODO(astra): Use proper URL decomposition for better search matching.
  std::string spec = url.spec();
  return base::i18n::ToLower(
      std::u16string(spec.begin(), spec.end()));
}

}  // namespace

// =========================================================================
// AstraRecentTabsHelper — thin wrapper around TabRestoreService
// =========================================================================
//
// The real implementation of each method delegates directly to
// sessions::TabRestoreService.  In the overlay repo, we return empty /
// null values because the service isn't linked.  The method signatures and
// documentation describe the intended behavior against a full Chromium
// build.

// -- Query methods ---------------------------------------------------------

std::vector<AstraRecentlyClosedTab> AstraRecentTabsHelper::GetRecentlyClosedTabs(
    Profile* profile,
    size_t max_count) {
  std::vector<AstraRecentlyClosedTab> result;

  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    // TabRestoreService not available — return empty list.
    // This is expected in the overlay repo where Chromium services
    // are not linked.  In a full build, the service is always available
    // for regular profiles (it may be null for incognito / OTR profiles,
    // since recently closed tabs are profile-scoped, not OTR-scoped).
    return result;
  }

  size_t effective_max = EffectiveMaxCount(profile, max_count);

  // TODO(astra): In a full Chromium build, iterate over
  // service->entries() and project each Tab entry into an
  // AstraRecentlyClosedTab.  Skip Window entries (we only show individual
  // tabs in the sidebar, not whole windows).
  //
  // Real implementation sketch:
  //
  //   const auto& entries = service->entries();
  //   for (size_t i = 0; i < entries.size() && result.size() < effective_max; ++i) {
  //     const auto& entry = entries[i];
  //     if (entry->type != TabRestoreService::TAB) {
  //       continue;
  //     }
  //     const Tab* tab = static_cast<const Tab*>(entry.get());
  //     AstraRecentlyClosedTab projected;
  //     projected.entry_id = tab->id;
  //     projected.title = tab->title;
  //     projected.url = tab->navigations.empty()
  //         ? GURL() : tab->navigations.back().virtual_url();
  //     projected.close_time = tab->timestamp;
  //     projected.list_index = static_cast<int>(i);
  //     projected.has_favicon = tab->favicon.is_valid();
  //     // Extract workspace_id from AstraTabFeatures metadata stored on
  //     // the tab entry (via session extra data).
  //     // TODO(astra): Read workspace_id from tab extra data.
  //     projected.workspace_id = "";
  //     result.push_back(std::move(projected));
  //   }
  //
  // Chromium type: sessions::TabRestoreService::Entry
  // Chromium type: sessions::TabRestoreService::Tab
  // Chromium method: TabRestoreService::entries()
  //   (chrome/browser/sessions/tab_restore_service.h)

  return result;
}

std::vector<AstraRecentlyClosedTab> AstraRecentTabsHelper::GetRecentTabs(
    Profile* profile,
    size_t max_count) {
  return GetRecentlyClosedTabs(profile, max_count);
}

size_t AstraRecentTabsHelper::GetRecentTabCount(Profile* profile) {
  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    return kEmptyEntryCount;
  }

  // TODO(astra): Count only TAB-type entries, not WINDOW-type entries.
  // For now, return the total entry count as a placeholder.
  return service->entries().size();
}

bool AstraRecentTabsHelper::HasRecentlyClosedTabs(Profile* profile) {
  return GetRecentTabCount(profile) > 0;
}

// -- Filtering -------------------------------------------------------------

std::vector<AstraRecentlyClosedTab>
AstraRecentTabsHelper::GetRecentTabsForWorkspace(
    Profile* profile,
    const std::string& workspace_id,
    size_t max_count) {
  std::vector<AstraRecentlyClosedTab> result;

  size_t effective_max = EffectiveMaxCount(profile, max_count);
  auto all_tabs = GetRecentlyClosedTabs(profile, 0);  // Get all, then filter

  for (const auto& tab : all_tabs) {
    if (tab.workspace_id == workspace_id) {
      result.push_back(tab);
      if (result.size() >= effective_max) {
        break;
      }
    }
  }

  return result;
}

std::vector<AstraRecentlyClosedTab>
AstraRecentTabsHelper::GetRecentTabsInTimeRange(
    Profile* profile,
    base::Time since,
    base::Time until,
    size_t max_count) {
  std::vector<AstraRecentlyClosedTab> result;

  size_t effective_max = EffectiveMaxCount(profile, max_count);
  auto all_tabs = GetRecentlyClosedTabs(profile, 0);  // Get all, then filter

  for (const auto& tab : all_tabs) {
    if (tab.close_time >= since && tab.close_time <= until) {
      result.push_back(tab);
      if (result.size() >= effective_max) {
        break;
      }
    }
  }

  return result;
}

// -- Search ----------------------------------------------------------------

std::vector<AstraRecentlyClosedTab> AstraRecentTabsHelper::SearchRecentTabs(
    Profile* profile,
    const std::u16string& query,
    size_t max_count) {
  std::vector<AstraRecentlyClosedTab> result;

  if (query.empty()) {
    return GetRecentlyClosedTabs(profile, max_count);
  }

  size_t effective_max = EffectiveMaxCount(profile, max_count);
  auto all_tabs = GetRecentlyClosedTabs(profile, 0);
  std::u16string query_lower = ToLower(query);

  for (const auto& tab : all_tabs) {
    if (TabMatchesQuery(tab, query_lower)) {
      result.push_back(tab);
      if (result.size() >= effective_max) {
        break;
      }
    }
  }

  return result;
}

bool AstraRecentTabsHelper::TabMatchesQuery(
    const AstraRecentlyClosedTab& tab,
    const std::u16string& query_lower) {
  if (query_lower.empty())
    return true;

  // Search in title (case-insensitive).
  std::u16string title_lower = ToLower(tab.title);
  if (title_lower.find(query_lower) != std::u16string::npos) {
    return true;
  }

  // Search in URL (case-insensitive).
  if (tab.url.is_valid()) {
    std::u16string url_lower = UrlToLower(tab.url);
    if (url_lower.find(query_lower) != std::u16string::npos) {
      return true;
    }
  }

  return false;
}

// -- Restore operations ----------------------------------------------------

content::WebContents* AstraRecentTabsHelper::RestoreMostRecentTab(
    Profile* profile) {
  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    return nullptr;
  }
  if (service->entries().empty()) {
    return nullptr;
  }

  // TODO(astra): Call service->RestoreMostRecentEntry(nullptr) in a full
  // Chromium build.  The first parameter is the Browser* to restore into,
  // or nullptr to use the last active browser.  The return value is the
  // restored tab's WebContents.
  //
  // After restoring, notify observers:
  //   NotifyRecentTabRestored(most_recent_id);
  //   NotifyRecentTabsChanged();
  //
  // Chromium method: TabRestoreService::RestoreMostRecentEntry()
  //   (chrome/browser/sessions/tab_restore_service.h)
  return nullptr;
}

content::WebContents* AstraRecentTabsHelper::RestoreTabById(
    Profile* profile,
    int entry_id) {
  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    return nullptr;
  }

  // TODO(astra): Call service->RestoreEntryById(nullptr, entry_id, ...)
  // in a full Chromium build.  This restores a specific entry by its id.
  //
  // After restoring, notify observers:
  //   NotifyRecentTabRestored(entry_id);
  //   NotifyRecentTabsChanged();
  //
  // Chromium method: TabRestoreService::RestoreEntryById()
  //   (chrome/browser/sessions/tab_restore_service.h)
  return nullptr;
}

size_t AstraRecentTabsHelper::RestoreAll(Profile* profile) {
  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    return 0;
  }

  // TODO(astra): Iterate entries in reverse order (so they restore in
  // the correct order) and call RestoreEntryById for each TAB-type entry.
  // Skip Window entries (or restore them differently).
  //
  // After all restores, notify observers:
  //   NotifyRecentTabsChanged();
  //
  // For now, return 0 as a placeholder.
  size_t restored = 0;
  // Real implementation:
  //   const auto& entries = service->entries();
  //   for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
  //     if ((*it)->type == TabRestoreService::TAB) {
  //       if (service->RestoreEntryById(nullptr, (*it)->id, ...)) {
  //         ++restored;
  //       }
  //     }
  //   }
  return restored;
}

// -- Bulk operations -------------------------------------------------------

void AstraRecentTabsHelper::ClearAllRecentTabs(Profile* profile) {
  sessions::TabRestoreService* service = GetTabRestoreService(profile);
  if (!service) {
    return;
  }

  // TODO(astra): Call service->ClearEntries() in a full Chromium build.
  // This removes all entries from TabRestoreService.
  //
  // Chromium method: TabRestoreService::ClearEntries()
  //   (chrome/browser/sessions/tab_restore_service.h)
  //
  // After clearing, notify observers:
  NotifyRecentTabsCleared();
  NotifyRecentTabsChanged();
}

// -- Statistics ------------------------------------------------------------

size_t AstraRecentTabsHelper::GetTotalClosedCount(Profile* profile) {
  // TODO(astra): In a full Chromium build, derive this from SessionService
  // or by tracking tab close events.  TabRestoreService only tracks the
  // most recent N entries; total closed count would need the full history.
  //
  // One approach: accumulate from SessionService's command count or by
  // subscribing to tab close events and incrementing a counter.
  //
  // Chromium owner: SessionService (chrome/browser/sessions/session_service.h)
  // Chromium owner: TabStripModelObserver (for tab close events)
  //
  // For the overlay, return the placeholder value.
  return kEmptyTotalClosedCount;
}

size_t AstraRecentTabsHelper::GetSessionCount(Profile* profile) {
  // TODO(astra): In a full Chromium build, query SessionService for the
  // number of persisted sessions.
  //
  // Chromium owner: SessionService / SessionBackend
  //   (chrome/browser/sessions/session_service.h)
  //
  // For the overlay, return the placeholder value.
  return kEmptySessionCount;
}

// -- Presentation settings -------------------------------------------------

int AstraRecentTabsHelper::GetMaxRecentTabs(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return kDefaultMaxRecentTabs;
  }
  return prefs->GetInteger(prefs::kPrefRecentTabsMaxCount);
}

void AstraRecentTabsHelper::SetMaxRecentTabs(Profile* profile, int max_count) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  // Clamp to valid range.
  int clamped = max_count;
  if (clamped < 1) {
    clamped = 1;
  }
  if (clamped > kMaxRecentTabsLimit) {
    clamped = kMaxRecentTabsLimit;
  }

  int current = prefs->GetInteger(prefs::kPrefRecentTabsMaxCount);
  if (current == clamped) {
    return;  // No change.
  }

  prefs->SetInteger(prefs::kPrefRecentTabsMaxCount, clamped);
  NotifyRecentPresentationChanged();
  // Presentation change also affects what's shown, so notify list changed.
  NotifyRecentTabsChanged();
}

bool AstraRecentTabsHelper::GetShowInSidebar(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return kDefaultShowInSidebar;
  }
  return prefs->GetBoolean(prefs::kPrefRecentTabsShowInSidebar);
}

void AstraRecentTabsHelper::SetShowInSidebar(Profile* profile, bool show) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  bool current = prefs->GetBoolean(prefs::kPrefRecentTabsShowInSidebar);
  if (current == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefRecentTabsShowInSidebar, show);
  NotifyRecentPresentationChanged();
}

bool AstraRecentTabsHelper::ToggleShowInSidebar(Profile* profile) {
  bool new_state = !GetShowInSidebar(profile);
  SetShowInSidebar(profile, new_state);
  return new_state;
}

bool AstraRecentTabsHelper::GetShowTimestamps(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return kDefaultShowTimestamps;
  }
  return prefs->GetBoolean(prefs::kPrefRecentTabsShowTimestamps);
}

void AstraRecentTabsHelper::SetShowTimestamps(Profile* profile, bool show) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  bool current = prefs->GetBoolean(prefs::kPrefRecentTabsShowTimestamps);
  if (current == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefRecentTabsShowTimestamps, show);
  NotifyRecentPresentationChanged();
}

bool AstraRecentTabsHelper::ToggleShowTimestamps(Profile* profile) {
  bool new_state = !GetShowTimestamps(profile);
  SetShowTimestamps(profile, new_state);
  return new_state;
}

// -- Observers -------------------------------------------------------------

void AstraRecentTabsHelper::AddObserver(Observer* observer) {
  if (observer) {
    GetObservers().AddObserver(observer);
  }
}

void AstraRecentTabsHelper::RemoveObserver(Observer* observer) {
  if (observer) {
    GetObservers().RemoveObserver(observer);
  }
}

void AstraRecentTabsHelper::NotifyRecentTabsChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnRecentTabsChanged();
  }
}

void AstraRecentTabsHelper::NotifyTabClosedToRecent(
    const AstraRecentlyClosedTab& tab) {
  for (auto& observer : GetObservers()) {
    observer.OnTabClosedToRecent(tab);
  }
  // Also fire the catch-all notification.
  NotifyRecentTabsChanged();
}

void AstraRecentTabsHelper::NotifyRecentTabRestored(int entry_id) {
  for (auto& observer : GetObservers()) {
    observer.OnRecentTabRestored(entry_id);
  }
  // Also fire the catch-all notification.
  NotifyRecentTabsChanged();
}

void AstraRecentTabsHelper::NotifyRecentTabsCleared() {
  for (auto& observer : GetObservers()) {
    observer.OnRecentTabsCleared();
  }
  // Also fire the catch-all notification.
  NotifyRecentTabsChanged();
}

void AstraRecentTabsHelper::NotifyRecentPresentationChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnRecentPresentationChanged();
  }
}

base::ObserverList<AstraRecentTabsHelper::Observer>&
AstraRecentTabsHelper::GetObservers() {
  static base::ObserverList<Observer> observers;
  return observers;
}

// -- Internal helpers ------------------------------------------------------

sessions::TabRestoreService* AstraRecentTabsHelper::GetTabRestoreService(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }

  // TODO(astra): Use TabRestoreServiceFactory::GetForProfile(profile) when
  // building against the full Chromium source tree.  In the overlay, we
  // return nullptr as a placeholder since the real service factory isn't
  // linked.
  //
  // Chromium factory: TabRestoreServiceFactory
  //   (chrome/browser/sessions/tab_restore_service_factory.h)
  // The service is a BrowserContextKeyedService, one per profile.
  //
  // Note: TabRestoreService is not available for off-the-record (incognito)
  // profiles.  Recently closed tabs are profile-scoped and shared across
  // all windows of the same profile.
  //
  // Patch point: None needed — we just call the existing factory.
  return nullptr;
}

PrefService* AstraRecentTabsHelper::GetPrefs(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

size_t AstraRecentTabsHelper::EffectiveMaxCount(Profile* profile,
                                                size_t requested) {
  if (requested > 0) {
    // Clamp requested to the hard limit.
    if (requested > static_cast<size_t>(kMaxRecentTabsLimit)) {
      return static_cast<size_t>(kMaxRecentTabsLimit);
    }
    return requested;
  }
  // Use the configured pref value.
  int pref_value = GetMaxRecentTabs(profile);
  return static_cast<size_t>(pref_value);
}

}  // namespace astra
