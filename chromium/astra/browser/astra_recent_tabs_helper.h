#ifndef ASTRA_BROWSER_ASTRA_RECENT_TABS_HELPER_H_
#define ASTRA_BROWSER_ASTRA_RECENT_TABS_HELPER_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace sessions {
struct TabRestoreService;
struct TabEntry;
}  // namespace sessions

namespace astra {

// =========================================================================
// AstraRecentlyClosedTab — projected data for a recently closed tab
// =========================================================================
//
// A lightweight struct representing a single recently-closed tab entry,
// projected from Chromium's TabRestoreService.  This struct is a pure
// projection — it never mutates session state.
//
// Chromium owner: TabRestoreService (chrome/browser/sessions/tab_restore_service.h)
// Chromium type: sessions::TabRestoreService::Entry / Tab
//
// TODO(astra): Use the full sessions::TabRestoreService::Entry / Tab types
// from Chromium instead of this projection struct, once the overlay is
// building against a full Chromium checkout.  This struct mirrors the
// fields we need for sidebar presentation.
struct AstraRecentlyClosedTab {
  // Stable identifier for the tab entry, used to restore by id.
  // Corresponds to TabRestoreService::Entry::id.
  int entry_id = 0;

  // Tab title.  May be empty if the tab had no title.
  std::u16string title;

  // URL the tab was navigated to (last committed URL).
  GURL url;

  // When the tab was closed.  Used for "time ago" display.
  base::Time close_time;

  // Index of the entry within TabRestoreService's entries list.
  // Used by RestoreMostRecentEntry / positional restore.
  // 0 = most recently closed.
  int list_index = 0;

  // Workspace ID the tab belonged to, if known.
  // Empty string means the workspace is unknown or the tab was not
  // associated with a specific workspace.
  //
  // This is Astra-projected metadata — it comes from AstraTabFeatures
  // attached to the tab at close time, persisted through session restore.
  //
  // Chromium owner: AstraTabFeatures (content::WebContentsUserData)
  std::string workspace_id;

  // Whether a favicon is available for this tab.
  // The actual favicon image is owned by Chromium's FaviconService.
  // This is a presentation hint — the UI uses it to decide whether to
  // show a placeholder or request the real favicon.
  //
  // Chromium owner: FaviconService (components/favicon/core/)
  bool has_favicon = false;
};

// =========================================================================
// AstraRecentTabsHelper — projection helper for recently closed tabs
// =========================================================================
//
// Helper that wraps Chromium's TabRestoreService for the Astra sidebar
// recently closed section and command palette.  It provides a clean
// projection API for reading recently closed tabs, restoring them, and
// managing Astra-specific presentation preferences.
//
// This is a thin projection layer — the truth source for recently closed
// tab data is always sessions::TabRestoreService.  The helper never caches
// data; every call reads from the underlying service.
//
// Astra-specific presentation preferences (max count, sidebar visibility,
// timestamps) are persisted via the profile's PrefService.  These are
// purely presentation concerns and never affect the underlying tab restore
// data managed by Chromium.
//
// Why a helper instead of using TabRestoreService directly from the UI?
//   1. Encapsulates the service lookup (factory + profile).
//   2. Adapts the TabRestoreService API to Astra UI needs (e.g. limiting
//      count, projecting to AstraRecentlyClosedTab, filtering by workspace).
//   3. Gives the UI layer a stable Astra-owned interface that won't
//      churn as Chromium's API evolves.
//   4. Follows the same pattern as other Astra helpers — UI talks to
//      Astra-layer helpers, not raw Chromium APIs.
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
// Chromium factory: TabRestoreServiceFactory
//   (chrome/browser/sessions/tab_restore_service_factory.h)
//
// Chromium owner (stats): sessions::SessionService
//   (chrome/browser/sessions/session_service.h)
//   For total closed count and session-level statistics.
//
// TODO(astra): Add TabRestoreServiceObserver bridge for real-time updates.
// The sidebar currently calls Refresh() manually; with an observer bridge,
// it could reactively update whenever a tab is closed or restored.
// Chromium observer: TabRestoreServiceObserver
//   (chrome/browser/sessions/tab_restore_service_observer.h)
class AstraRecentTabsHelper {
 public:
  // =======================================================================
  // Observer interface for recent tab change notifications
  // =======================================================================
  //
  // The UI layer implements this observer to refresh the recent tabs view
  // when recently closed tab state or presentation settings change.
  // All observer methods have empty default implementations so observers
  // only need to override the events they care about.
  //
  // The browser layer never depends on Views code.
  class Observer : public base::CheckedObserver {
   public:
    // Called when the recently closed tabs list has changed in any way
    // (tab closed, tab restored, list cleared, etc.).
    // This is a catch-all notification — use the more granular events
    // below for targeted UI updates.
    // The UI should re-read the list via GetRecentlyClosedTabs() and
    // rebuild its projection.
    virtual void OnRecentTabsChanged() {}

    // Called when a tab is closed and added to the recently closed list.
    // |tab| contains the projected info for the newly closed tab.
    virtual void OnTabClosedToRecent(const AstraRecentlyClosedTab& tab) {}

    // Called when a recently closed tab is restored (re-opened).
    // |entry_id| is the ID of the tab that was restored.
    virtual void OnRecentTabRestored(int entry_id) {}

    // Called when all recently closed tabs are cleared.
    virtual void OnRecentTabsCleared() {}

    // Called when recent tabs presentation settings change (e.g. max
    // count, show in sidebar, show timestamps).
    // The UI should refresh its recent tabs section presentation.
    virtual void OnRecentPresentationChanged() {}

   protected:
    ~Observer() override = default;
  };

  AstraRecentTabsHelper() = delete;
  AstraRecentTabsHelper(const AstraRecentTabsHelper&) = delete;
  AstraRecentTabsHelper& operator=(const AstraRecentTabsHelper&) = delete;

  // -- Query methods -------------------------------------------------------

  // Get a list of recently closed tabs for |profile|, up to |max_count|.
  // Tabs are returned in most-recently-closed-first order (index 0 = most
  // recent).
  //
  // Returns an empty vector if the TabRestoreService is not available or
  // has no entries.
  //
  // If |max_count| is 0, the configured maximum (GetMaxRecentTabs()) is
  // used.
  //
  // Chromium method: TabRestoreService::entries()
  // Chromium method: TabRestoreService::IsLoaded()
  static std::vector<AstraRecentlyClosedTab> GetRecentlyClosedTabs(
      Profile* profile,
      size_t max_count = 0);

  // Alias for GetRecentlyClosedTabs.  Provided for API consistency with
  // other query methods and for callers that prefer the "recent tabs"
  // terminology.
  static std::vector<AstraRecentlyClosedTab> GetRecentTabs(
      Profile* profile,
      size_t max_count = 0);

  // Returns the number of recently closed tab entries available.
  // Returns 0 if the service is not available.
  //
  // This counts all TAB-type entries in TabRestoreService, not just the
  // ones shown (which may be limited by max_count).
  static size_t GetRecentTabCount(Profile* profile);

  // Returns true if there are any recently closed tabs.
  static bool HasRecentlyClosedTabs(Profile* profile);

  // -- Filtering -----------------------------------------------------------

  // Get recently closed tabs filtered by workspace ID.
  // Only tabs that were associated with the given workspace are returned.
  //
  // If |workspace_id| is empty, returns tabs with no workspace association.
  //
  // Workspace association comes from AstraTabFeatures metadata attached
  // to each tab at close time.
  static std::vector<AstraRecentlyClosedTab> GetRecentTabsForWorkspace(
      Profile* profile,
      const std::string& workspace_id,
      size_t max_count = 0);

  // Get recently closed tabs that were closed within the given time range.
  // |since| is the earliest close time to include.
  // |until| is the latest close time to include (defaults to Now).
  //
  // Tabs are returned in most-recently-closed-first order.
  static std::vector<AstraRecentlyClosedTab> GetRecentTabsInTimeRange(
      Profile* profile,
      base::Time since,
      base::Time until = base::Time::Max(),
      size_t max_count = 0);

  // -- Search --------------------------------------------------------------

  // Search recently closed tabs by title or URL.
  // The query is matched case-insensitively against both title and URL.
  //
  // Returns matching tabs in most-recently-closed-first order.
  // An empty query returns all recent tabs (up to max_count).
  static std::vector<AstraRecentlyClosedTab> SearchRecentTabs(
      Profile* profile,
      const std::u16string& query,
      size_t max_count = 0);

  // -- Restore operations --------------------------------------------------

  // Restore the most recently closed tab.
  // Returns the WebContents of the restored tab, or nullptr on failure.
  //
  // The restored tab opens in the active browser window for |profile|.
  // If no browser window exists, Chromium may create one.
  //
  // Chromium method: TabRestoreService::RestoreMostRecentEntry()
  //   (chrome/browser/sessions/tab_restore_service.h)
  static content::WebContents* RestoreMostRecentTab(Profile* profile);

  // Restore a specific recently closed tab by its entry id.
  // Returns the WebContents of the restored tab, or nullptr if not found.
  //
  // Chromium method: TabRestoreService::RestoreEntryById()
  //   (chrome/browser/sessions/tab_restore_service.h)
  static content::WebContents* RestoreTabById(Profile* profile, int entry_id);

  // Restore all recently closed tabs.
  // Returns the number of tabs restored.
  //
  // Chromium method: Iterate entries() and call RestoreEntryById for each.
  // TODO(astra): Batch restore may be more efficient through a dedicated
  // Chromium API.  For now, restore entries one at a time.
  static size_t RestoreAll(Profile* profile);

  // -- Bulk operations -----------------------------------------------------

  // Clear all recently closed tabs.
  //
  // This removes all entries from TabRestoreService for |profile|.
  // Fires OnRecentTabsCleared and OnRecentTabsChanged observer notifications.
  //
  // Chromium method: TabRestoreService::ClearEntries()
  //   (chrome/browser/sessions/tab_restore_service.h)
  static void ClearAllRecentTabs(Profile* profile);

  // -- Statistics ----------------------------------------------------------

  // Returns the total number of tabs closed in the current session.
  //
  // This is a cumulative session stat — it counts all tabs closed since
  // the browser started, including ones that have fallen off the
  // recently closed list due to the max count limit.
  //
  // TODO(astra): Derive from SessionService / session stats.
  // Chromium owner: SessionService (chrome/browser/sessions/session_service.h)
  static size_t GetTotalClosedCount(Profile* profile);

  // Returns the number of sessions (browser restarts) tracked for the
  // profile.
  //
  // This is useful for the "recent sessions" / history UI that shows
  // tabs from previous browsing sessions.
  //
  // TODO(astra): Derive from SessionService session list.
  // Chromium owner: SessionService (chrome/browser/sessions/session_service.h)
  static size_t GetSessionCount(Profile* profile);

  // -- Presentation settings -----------------------------------------------

  // Returns the maximum number of recently closed tabs to show in the
  // sidebar and command palette.
  //
  // Persisted via PrefService.  Default: kDefaultMaxRecentTabs.
  //
  // This is a presentation limit — TabRestoreService may track more
  // entries than we display.
  static int GetMaxRecentTabs(Profile* profile);

  // Sets the maximum number of recently closed tabs to show.
  // The value is clamped to the valid range [1, kMaxRecentTabsLimit].
  // Fires OnRecentPresentationChanged observer notification.
  static void SetMaxRecentTabs(Profile* profile, int max_count);

  // Returns whether the recently closed section is shown in the sidebar.
  //
  // Persisted via PrefService.  Default: kDefaultShowInSidebar.
  //
  // This is a presentation preference — it only controls whether the
  // section appears in the sidebar.  Recently closed tabs are still
  // tracked by Chromium regardless.
  static bool GetShowInSidebar(Profile* profile);

  // Sets whether the recently closed section is shown in the sidebar.
  // Fires OnRecentPresentationChanged observer notification.
  static void SetShowInSidebar(Profile* profile, bool show);

  // Toggles whether the recently closed section is shown in the sidebar.
  // Returns the new state.
  static bool ToggleShowInSidebar(Profile* profile);

  // Returns whether timestamps are shown next to recently closed tabs.
  //
  // Persisted via PrefService.  Default: kDefaultShowTimestamps.
  //
  // This is a presentation preference — it controls whether the UI shows
  // "time ago" labels next to each recently closed tab.
  static bool GetShowTimestamps(Profile* profile);

  // Sets whether timestamps are shown.
  // Fires OnRecentPresentationChanged observer notification.
  static void SetShowTimestamps(Profile* profile, bool show);

  // Toggles timestamps visibility.  Returns the new state.
  static bool ToggleShowTimestamps(Profile* profile);

  // -- Constants -----------------------------------------------------------

  // Default max number of recently closed tabs to show.
  static constexpr int kDefaultMaxRecentTabs = 10;

  // Hard upper limit for the max recent tabs setting.
  // Used to clamp user-set values to a reasonable maximum.
  static constexpr int kMaxRecentTabsLimit = 100;

  // Default: show recently closed section in the sidebar.
  static constexpr bool kDefaultShowInSidebar = true;

  // Default: show timestamps next to recently closed tabs.
  static constexpr bool kDefaultShowTimestamps = true;

  // -- Observers -----------------------------------------------------------

  // Registers an observer for recent tabs change notifications.
  //
  // TODO(astra): Wire up to TabRestoreServiceObserver once we have the
  // observation bridge implementation.  Currently observers are
  // registered and notified for presentation setting changes, but
  // real-time tab close/restore events come through the Chromium
  // observer bridge which is not yet implemented in the overlay.
  static void AddObserver(Observer* observer);

  // Unregisters an observer.
  static void RemoveObserver(Observer* observer);

  // Notify all observers that the recent tabs list has changed.
  //
  // TODO(astra): This should be called from the TabRestoreServiceObserver
  // bridge implementation.  Currently it is a manual trigger for testing
  // and for presentation setting change propagation.
  static void NotifyRecentTabsChanged();

  // Notify all observers that a tab has been closed and added to recent.
  static void NotifyTabClosedToRecent(const AstraRecentlyClosedTab& tab);

  // Notify all observers that a recent tab has been restored.
  static void NotifyRecentTabRestored(int entry_id);

  // Notify all observers that all recent tabs have been cleared.
  static void NotifyRecentTabsCleared();

  // Notify all observers that presentation settings have changed.
  static void NotifyRecentPresentationChanged();

 private:
  // Get the TabRestoreService for |profile| via the factory.
  // Returns nullptr if the service is not available.
  //
  // TODO(astra): Use TabRestoreServiceFactory::GetForProfile() when
  // building against a full Chromium checkout.  In the overlay, the
  // service may not be linked, so we return nullptr as a placeholder.
  // Chromium factory: TabRestoreServiceFactory
  //   (chrome/browser/sessions/tab_restore_service_factory.h)
  static sessions::TabRestoreService* GetTabRestoreService(Profile* profile);

  // Returns the static observer list.  Wrapped in a function to avoid
  // static initialization order issues.
  static base::ObserverList<Observer>& GetObservers();

  // Helper to get the PrefService from a profile, with null checks.
  // Returns nullptr if profile is null or has no pref service.
  static PrefService* GetPrefs(Profile* profile);

  // Helper to get the effective max count.
  // If |requested| is 0, returns the configured pref value.
  // Otherwise returns |requested| clamped to the limit.
  static size_t EffectiveMaxCount(Profile* profile, size_t requested);

  // Internal helper to check if a tab matches a search query.
  // Case-insensitive match against title and URL.
  static bool TabMatchesQuery(const AstraRecentlyClosedTab& tab,
                              const std::u16string& query_lower);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_RECENT_TABS_HELPER_H_
