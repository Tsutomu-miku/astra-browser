#ifndef ASTRA_BROWSER_ASTRA_HISTORY_HELPER_H_
#define ASTRA_BROWSER_ASTRA_HISTORY_HELPER_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace history {
class HistoryService;
}  // namespace history

namespace astra {

// =========================================================================
// AstraHistoryItem — projected history item data
// =========================================================================
//
// Projection of a single history entry for UI display.
//
// This is a presentation-only data structure — it mirrors a subset of
// Chromium's history::URLRow state. The truth source is always Chromium's
// HistoryService.
//
// Chromium owner: history::URLRow (components/history/core/browser/url_row.h)
// Chromium owner: history::VisitRow (components/history/core/browser/visit_row.h)
struct AstraHistoryItem {
  // The page URL.
  GURL url;

  // The page title. May be empty if the page had no title.
  std::u16string title;

  // Time of the last visit to this URL.
  base::Time visit_time;

  // Total number of visits to this URL.
  int visit_count = 0;

  // Whether this URL is bookmarked.
  // This is a projected value — bookmarks are owned by Chromium's
  // BookmarkModel, but the history UI often shows bookmark state.
  bool is_bookmarked = false;

  // Whether the URL was typed (navigated to by typing in the address bar)
  // as opposed to being followed from a link.
  bool is_typed = false;

  // The page transition type for the most recent visit.
  // Maps to ui::PageTransition (ui/base/page_transition_types.h).
  // Stored as int to avoid including the full header.
  int transition_type = 0;

  // The favicon URL for this page.
  // The actual favicon image is owned by Chromium's FaviconService.
  GURL favicon_url;
};

// =========================================================================
// AstraHistoryQuery — history query parameters
// =========================================================================
//
// Struct encapsulating parameters for a history query.
// Used by search and filter methods.
//
// The actual query execution is done by Chromium's HistoryService.
// This struct collects parameters for the Astra UI layer to pass through.
struct AstraHistoryQuery {
  // Text to search for (matches against title and URL).
  // Empty string means no text filter.
  std::u16string search_text;

  // Start of the time range (inclusive).
  // base::Time::Min() means no lower bound.
  base::Time begin_time;

  // End of the time range (inclusive).
  // base::Time::Max() means no upper bound.
  base::Time end_time;

  // Maximum number of results to return.
  // 0 means use the configured default (GetMaxHistoryResults()).
  int max_results = 0;

  // If true, only return typed URLs.
  bool only_typed = false;

  // Sort order for results.
  enum SortBy {
    kByTime,       // Sort by visit time (most recent first).
    kByVisitCount,  // Sort by visit count (highest first).
    kByTitle,       // Sort by title (alphabetical).
  };
  SortBy sort_by = kByTime;
};

// =========================================================================
// AstraHistoryObserver — observer interface
// =========================================================================
//
// Observer interface for AstraHistoryHelper. Notifies when history state
// changes or when history presentation settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh the history sidebar
// section when history state changes. The browser layer never depends on
// Views code.
//
// Chromium observer: history::HistoryServiceObserver
//   (components/history/core/browser/history_service_observer.h)
class AstraHistoryObserver : public base::CheckedObserver {
 public:
  // Called when a new history item has been added (a page was visited).
  // |url| is the URL that was added to history.
  virtual void OnHistoryItemAdded(const GURL& url) {}

  // Called when a history item has been removed.
  // |url| is the URL that was removed from history.
  virtual void OnHistoryItemRemoved(const GURL& url) {}

  // Called when all history items have been cleared.
  virtual void OnHistoryItemsCleared() {}

  // Called when old history entries have expired (auto-deleted due to
  // retention policy or expiration).
  virtual void OnHistoryExpired() {}

  // Called when history presentation settings have changed.
  // The UI should refresh its history section presentation.
  virtual void OnHistorySettingsChanged() {}

  // Called when an asynchronous history query has completed.
  // |query_id| is the identifier of the completed query.
  virtual void OnHistoryQueryCompleted(int query_id) {}

 protected:
  ~AstraHistoryObserver() override = default;
};

// =========================================================================
// AstraHistoryHelper — history projection helper
// =========================================================================
//
// Helper class for history-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// HistoryService APIs. It provides a clean interface for the Astra UI
// layer to query history state and perform history operations without
// directly depending on the full history subsystem.
//
// Truth source: Chromium's HistoryService
//   (components/history/core/browser/history_service.h)
// Chromium factory: HistoryServiceFactory
//   (chrome/browser/history/history_service_factory.h)
//
// History data is fully owned by Chromium's HistoryService.
// Astra only projects the state and adds presentation preferences.
//
// Presentation preferences (sort order, max results, display mode, etc.)
// are persisted via the profile's PrefService. These are purely
// presentation concerns and never affect the underlying history data
// managed by Chromium.
//
// TODO(astra): Proper HistoryServiceObserver integration.
//   Currently this helper does not observe the history service for live
//   updates. To get reactive updates, we need to implement
//   history::HistoryServiceObserver.
// Chromium observer: history::HistoryServiceObserver
//   (components/history/core/browser/history_service_observer.h)
class AstraHistoryHelper : public KeyedService {
 public:
  explicit AstraHistoryHelper(Profile* profile);
  AstraHistoryHelper(const AstraHistoryHelper&) = delete;
  AstraHistoryHelper& operator=(const AstraHistoryHelper&) = delete;
  ~AstraHistoryHelper() override;

  // -- History item queries ------------------------------------------------

  // Returns the total number of unique history items (URLs).
  // This is an approximate count since history can change between calls.
  //
  // TODO(astra): Query HistoryService for the actual count.
  //   In the overlay, returns 0 as a placeholder.
  //
  // Chromium method: HistoryService::GetUniqueHostsCount or similar.
  int GetHistoryItemCount() const;

  // Returns the number of visits today (since midnight).
  //
  // TODO(astra): Query HistoryService for today's visit count.
  int GetVisitsToday() const;

  // Returns the number of visits this week (since Sunday midnight).
  //
  // TODO(astra): Query HistoryService for this week's visit count.
  int GetVisitsThisWeek() const;

  // Returns the top |max_count| most visited URLs.
  // Results are sorted by visit count (highest first).
  //
  // If |max_count| is 0, uses the configured default from prefs.
  //
  // TODO(astra): Query HistoryService's most visited data.
  //   Chromium owner: history::TopSites / MostVisitedProvider
  //   (components/history/core/browser/top_sites.h)
  std::vector<AstraHistoryItem> GetMostVisited(int max_count) const;

  // Returns the |max_count| most recent history visits.
  // Results are sorted by visit time (most recent first).
  //
  // If |max_count| is 0, uses the configured default from prefs.
  //
  // TODO(astra): Query HistoryService for recent visits.
  //   Chromium method: HistoryService::QueryHistory
  std::vector<AstraHistoryItem> GetRecentHistory(int max_count) const;

  // Searches history by |query| text (matches title and URL).
  // Returns up to |max_results| matching items.
  //
  // If |max_results| is 0, uses the configured default from prefs.
  //
  // TODO(astra): Use HistoryService's QueryHistory with a text filter.
  //   Chromium method: HistoryService::QueryHistory
  std::vector<AstraHistoryItem> SearchHistory(
      const std::string& query,
      int max_results) const;

  // Returns history items for a specific calendar day.
  // |day| is any time within the day (midnight to midnight is queried).
  //
  // TODO(astra): Query HistoryService for the day's visits.
  std::vector<AstraHistoryItem> GetHistoryForDay(base::Time day) const;

  // Returns history items within the given time range.
  // |begin| is the start time (inclusive).
  // |end| is the end time (inclusive).
  // |max| is the maximum number of results (0 = default).
  //
  // TODO(astra): Query HistoryService for the time range.
  std::vector<AstraHistoryItem> GetHistoryForRange(base::Time begin,
                                                   base::Time end,
                                                   int max) const;

  // -- URL-specific queries -----------------------------------------------

  // Returns the last visit time for |url|.
  // Returns base::Time() if the URL is not in history.
  //
  // TODO(astra): Query HistoryService for the last visit time.
  base::Time GetLastVisitTime(const GURL& url) const;

  // Returns the visit count for |url|.
  // Returns 0 if the URL is not in history.
  //
  // TODO(astra): Query HistoryService for the visit count.
  int GetVisitCount(const GURL& url) const;

  // Returns true if |url| exists in the history database.
  //
  // TODO(astra): Query HistoryService for URL presence.
  bool IsUrlInHistory(const GURL& url) const;

  // Returns typed URLs only (URLs navigated to by typing).
  // Up to |max_count| results are returned.
  //
  // If |max_count| is 0, uses the configured default from prefs.
  //
  // TODO(astra): Query HistoryService for typed URLs.
  //   Chromium method: HistoryService::GetMostRecentURLs with filter
  std::vector<AstraHistoryItem> GetTypedUrls(int max_count) const;

  // -- History operations --------------------------------------------------

  // Removes a specific URL from history.
  // Fires OnHistoryItemRemoved observer notification.
  //
  // TODO(astra): Call HistoryService->ExpireHistoryForURLs()
  //   Chromium method: HistoryService::ExpireHistoryForURLs
  void RemoveHistoryItem(const GURL& url);

  // Removes all history items within the given time range.
  // Fires OnHistoryItemRemoved for each URL and OnHistoryExpired.
  //
  // TODO(astra): Call HistoryService->ExpireHistoryBetween()
  //   Chromium method: HistoryService::ExpireHistoryBetween
  void RemoveHistoryForRange(base::Time begin, base::Time end);

  // Clears all browsing history.
  // Fires OnHistoryItemsCleared observer notification.
  //
  // TODO(astra): Clear all history via HistoryService.
  void ClearAllHistory();

  // -- Bulk operations -----------------------------------------------------

  // Bulk removes multiple URLs from history.
  // Fires OnHistoryItemRemoved for each URL.
  //
  // TODO(astra): Use HistoryService::ExpireHistoryForURLs with a vector.
  void RemoveUrls(const std::vector<GURL>& urls);

  // Clears history for the last |days| days.
  // Days = 1 clears today's history.
  //
  // TODO(astra): Call RemoveHistoryForRange with computed time range.
  void ClearHistoryForLastDays(int days);

  // Deletes history older than the retention period.
  // Uses the history_retention_days setting.
  // If auto_delete is disabled, this still performs the deletion
  // (manual invocation).
  //
  // Fires OnHistoryExpired observer notification.
  //
  // TODO(astra): Delete old history based on retention policy.
  void DeleteOldHistory();

  // Applies the retention policy (deletes history older than
  // history_retention_days if auto_delete_history is enabled).
  // This is a no-op if auto_delete_history is false.
  //
  // Fires OnHistoryExpired observer notification if any deletions occurred.
  //
  // TODO(astra): Periodic expiry based on retention policy.
  void ExpireHistoryByRetention();

  // -- Presentation settings -----------------------------------------------

  // Returns whether the history section is shown in the sidebar.
  //
  // Persisted via PrefService. Default: true.
  //
  // This is a presentation preference — it only controls whether the
  // section appears in the sidebar. History is still tracked by Chromium.
  bool GetShowHistoryInSidebar() const;

  // Sets whether the history section is shown in the sidebar.
  // Fires OnHistorySettingsChanged observer notification.
  void SetShowHistoryInSidebar(bool show);

  // Toggles the show history in sidebar setting.
  // Returns the new state.
  bool ToggleShowHistoryInSidebar();

  // Returns the history sort order.
  // Values: "time_desc", "time_asc", "most_visited".
  //
  // Persisted via PrefService. Default: "time_desc".
  std::string GetHistorySortOrder() const;

  // Sets the history sort order.
  // Fires OnHistorySettingsChanged observer notification.
  void SetHistorySortOrder(const std::string& order);

  // Returns the maximum number of history results per query.
  //
  // Persisted via PrefService. Default: 50.
  // Clamped between 10 and 500.
  int GetMaxHistoryResults() const;

  // Sets the maximum number of history results per query.
  // Values are clamped to the valid range [10, 500].
  // Fires OnHistorySettingsChanged observer notification.
  void SetMaxHistoryResults(int max_results);

  // Returns whether favicons are shown in history listings.
  //
  // Persisted via PrefService. Default: true.
  bool GetShowHistoryFavicons() const;

  // Sets whether favicons are shown in history listings.
  // Fires OnHistorySettingsChanged observer notification.
  void SetShowHistoryFavicons(bool show);

  // Toggles show history favicons. Returns the new state.
  bool ToggleShowHistoryFavicons();

  // Returns whether visit counts are shown in history listings.
  //
  // Persisted via PrefService. Default: false.
  bool GetShowVisitCount() const;

  // Sets whether visit counts are shown.
  // Fires OnHistorySettingsChanged observer notification.
  void SetShowVisitCount(bool show);

  // Toggles show visit count. Returns the new state.
  bool ToggleShowVisitCount();

  // Returns whether visit times are shown in history listings.
  //
  // Persisted via PrefService. Default: true.
  bool GetShowVisitTime() const;

  // Sets whether visit times are shown.
  // Fires OnHistorySettingsChanged observer notification.
  void SetShowVisitTime(bool show);

  // Toggles show visit time. Returns the new state.
  bool ToggleShowVisitTime();

  // Returns the history display mode.
  // Values: "list", "compact", "card".
  //
  // Persisted via PrefService. Default: "list".
  std::string GetHistoryDisplayMode() const;

  // Sets the history display mode.
  // Fires OnHistorySettingsChanged observer notification.
  void SetHistoryDisplayMode(const std::string& mode);

  // Returns whether history results are grouped by date.
  //
  // Persisted via PrefService. Default: true.
  bool GetGroupHistoryByDate() const;

  // Sets whether history results are grouped by date.
  // Fires OnHistorySettingsChanged observer notification.
  void SetGroupHistoryByDate(bool group);

  // Toggles group history by date. Returns the new state.
  bool ToggleGroupHistoryByDate();

  // Returns the maximum number of history items per day group.
  //
  // Persisted via PrefService. Default: 20.
  // Clamped between 5 and 100.
  int GetHistoryItemsPerDay() const;

  // Sets the maximum number of history items per day group.
  // Values are clamped to the valid range [5, 100].
  // Fires OnHistorySettingsChanged observer notification.
  void SetHistoryItemsPerDay(int items_per_day);

  // Returns whether only typed URLs are shown.
  //
  // Persisted via PrefService. Default: false.
  bool GetShowTypedUrlsOnly() const;

  // Sets whether only typed URLs are shown.
  // Fires OnHistorySettingsChanged observer notification.
  void SetShowTypedUrlsOnly(bool only_typed);

  // Toggles show typed URLs only. Returns the new state.
  bool ToggleShowTypedUrlsOnly();

  // Returns whether history deletion is enabled.
  //
  // Persisted via PrefService. Default: true.
  // When false, delete/clear operations are no-ops (for policy-restricted
  // environments or child accounts).
  bool GetHistoryDeletionEnabled() const;

  // Sets whether history deletion is enabled.
  // Fires OnHistorySettingsChanged observer notification.
  void SetHistoryDeletionEnabled(bool enabled);

  // Toggles history deletion enabled. Returns the new state.
  bool ToggleHistoryDeletionEnabled();

  // Returns whether old history is auto-deleted.
  //
  // Persisted via PrefService. Default: false.
  bool GetAutoDeleteHistory() const;

  // Sets whether old history is auto-deleted.
  // Fires OnHistorySettingsChanged observer notification.
  void SetAutoDeleteHistory(bool auto_delete);

  // Toggles auto delete history. Returns the new state.
  bool ToggleAutoDeleteHistory();

  // Returns the history retention period in days.
  //
  // Persisted via PrefService. Default: 90.
  // Clamped between 1 and 3650.
  int GetHistoryRetentionDays() const;

  // Sets the history retention period in days.
  // Values are clamped to the valid range [1, 3650].
  // Fires OnHistorySettingsChanged observer notification.
  void SetHistoryRetentionDays(int days);

  // -- Utility methods -----------------------------------------------------

  // Formats a visit time as a relative time string.
  // e.g. "2 hours ago", "5 minutes ago", "Yesterday", "Last week".
  //
  // This is a static utility method — it doesn't depend on profile state.
  static std::u16string FormatVisitTime(base::Time visit_time);

  // Formats a time delta as a human-readable relative string.
  // e.g. "2 hours", "5 minutes", "3 days".
  //
  // This is a static utility method.
  static std::u16string FormatRelativeTime(base::TimeDelta delta);

  // Truncates a title to |max_length| characters, adding an ellipsis.
  // If the title is shorter than max_length, returns it unchanged.
  //
  // This is a static utility method.
  static std::u16string TruncateTitle(const std::u16string& title,
                                       int max_length);

  // Extracts the domain name from a URL.
  // e.g. "https://www.example.com/path" -> "example.com"
  //
  // Returns an empty string for invalid URLs.
  //
  // This is a static utility method.
  static std::string GetDomainName(const GURL& url);

  // Returns true if |a| and |b| are on the same calendar day.
  //
  // This is a static utility method.
  static bool IsSameDay(base::Time a, base::Time b);

  // Groups history items by calendar day.
  // Returns a map where the key is the start of the day (midnight) and
  // the value is the list of items for that day.
  // Items within each day are sorted by visit time (most recent first).
  //
  // This is a static utility method.
  static std::map<base::Time, std::vector<AstraHistoryItem>> GroupByDate(
      const std::vector<AstraHistoryItem>& items);

  // -- Constants -----------------------------------------------------------

  // Default max history results per query.
  static constexpr int kDefaultMaxHistoryResults = 50;

  // Minimum allowed max history results.
  static constexpr int kMinHistoryResults = 10;

  // Maximum allowed max history results (hard upper limit).
  static constexpr int kMaxHistoryResults = 500;

  // Default max items per day group.
  static constexpr int kDefaultHistoryItemsPerDay = 20;

  // Minimum allowed items per day.
  static constexpr int kMinHistoryItemsPerDay = 5;

  // Maximum allowed items per day.
  static constexpr int kMaxHistoryItemsPerDay = 100;

  // Default history retention days.
  static constexpr int kDefaultHistoryRetentionDays = 90;

  // Minimum retention days.
  static constexpr int kMinRetentionDays = 1;

  // Maximum retention days.
  static constexpr int kMaxRetentionDays = 3650;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraHistoryObserver* observer);
  void RemoveObserver(AstraHistoryObserver* observer);

  // -- Notification helpers (public for testing) --------------------------

  // Notify all observers that a history item has been added.
  void NotifyHistoryItemAdded(const GURL& url);

  // Notify all observers that a history item has been removed.
  void NotifyHistoryItemRemoved(const GURL& url);

  // Notify all observers that all history items have been cleared.
  void NotifyHistoryItemsCleared();

  // Notify all observers that old history entries have expired.
  void NotifyHistoryExpired();

  // Notify all observers that history settings have changed.
  void NotifyHistorySettingsChanged();

  // Notify all observers that a history query has completed.
  void NotifyHistoryQueryCompleted(int query_id);

  // -- KeyedService --------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the HistoryService for the associated profile.
  // Returns nullptr if the service is not available.
  //
  // TODO(astra): Use HistoryServiceFactory::GetForProfile() when building
  //   against the full Chromium source tree. In the overlay, we return
  //   nullptr as a placeholder since the real service isn't linked.
  //
  // Chromium factory: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  // The history service is a BrowserContextKeyedService, one per profile.
  //
  // Patch point: None needed — we just call the existing factory.
  history::HistoryService* GetHistoryService() const;

  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // Helper to compute the effective max results count.
  // If |requested| is 0, returns the configured pref value.
  // Otherwise returns |requested| clamped to the valid range.
  int EffectiveMaxResults(int requested) const;

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for history state changes.
  base::ObserverList<AstraHistoryObserver> observers_;

  // Tracks whether we're currently observing the history service.
  // TODO(astra): Flip this to true once HistoryServiceObserver
  //   integration is implemented.
  bool is_observing_service_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_HISTORY_HELPER_H_
