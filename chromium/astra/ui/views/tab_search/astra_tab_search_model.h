#ifndef ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_MODEL_H_
#define ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_MODEL_H_

#include <stddef.h>

#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "skia/include/core/SkColor.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace PrefService;

namespace astra {

// =========================================================================
// Tab search model — manages tab data, search state, and results
// =========================================================================
//
// AstraTabSearchModel owns tab search state and tab data projection:
//   - Full tab list (all tabs across all windows)
//   - Search query text
//   - Search mode (all tabs, workspace-specific, audio, etc.)
//   - Filtered and sorted search results
//   - Selected result index
//   - Sort order and presentation settings
//   - Tab groups metadata
//
// This is a pure model class — no Views dependencies.  It takes tab data as
// input (projected from Chromium's TabStripModel), performs filtering and
// ranking, and notifies observers of changes.
//
// The bubble (view layer) observes the model and renders its state.  User
// input flows from the view into the model (SetQuery, SwitchToTab, etc.),
// and the model notifies observers so the view can update.
//
// Chromium subsystems reused:
//   - TabStripModel owns actual tab data (the model projects it).
//   - TabRestoreService owns recently closed tabs.
//   - BookmarkModel owns bookmarks.
//   - PrefService for presentation settings persistence.
//   - TabGroupModel / TabGroupController for tab group metadata.
//
// TODO(astra): Wire to TabStripModel for real tab data.
//   Chromium component: chrome/browser/ui/tabs/tab_strip_model.h
//   Patch point: Browser::tab_strip_model()
//
// TODO(astra): Consider whether this model should be a ProfileKeyedService
//   so it can be shared across multiple browser windows.  For now it's
//   per-bubble instance.
// =========================================================================

// Search mode — which set of tabs to search within.
enum class AstraTabSearchMode {
  kAllTabs = 0,           // All open tabs across all windows.
  kCurrentWorkspace = 1,  // Tabs in the current workspace only.
  kOtherWorkspaces = 2,   // Tabs in other workspaces (not current).
  kRecentlyClosed = 3,    // Recently closed tabs (TabRestoreService).
  kFavorites = 4,         // Tabs marked as favorites / bookmarked.
  kAudioPlaying = 5,      // Tabs currently playing audio.
};

// Sort order for tab lists and search results.
enum class AstraTabSearchSortOrder {
  kByRecency = 0,     // Most recently active first.
  kByTitle = 1,       // Alphabetical by tab title (A-Z).
  kByPosition = 2,    // Original tab strip position (left to right).
};

// Result group category — mirrors the visual grouping in the UI.
enum class AstraTabSearchGroup {
  kOpenTabs = 0,
  kRecentlyClosed = 1,
  kBookmarks = 2,
};

// Data for a single tab in the search model.
//
// This is a projection of Chromium tab state — the actual WebContents and
// TabStripModel own the real state.  This struct holds display-oriented
// fields needed by the search UI.
struct AstraTabSearchItem {
  // Unique tab identifier (stable across tab moves).
  int tab_id = -1;

  // Tab title (from WebContents::GetTitle()).
  std::u16string title;

  // Full tab URL.
  GURL url;

  // Display hostname (e.g. "example.com" or "chrome://settings").
  std::u16string hostname;

  // Workspace the tab belongs to (empty if not assigned).
  std::string workspace_id;
  std::u16string workspace_name;

  // True if this is the currently active tab in its window.
  bool is_active = false;

  // True if the tab is pinned.
  bool is_pinned = false;

  // Tab group membership.
  bool is_in_group = false;
  std::string group_id;
  std::u16string group_name;
  SkColor group_color = SK_ColorTRANSPARENT;

  // Last time the tab was activated.
  base::Time last_active_time;

  // Search relevance score (populated during search).
  double relevance_score = 0.0;

  // Tab favicon (or placeholder).
  // TODO(astra): Populate with real favicon from FaviconService.
  //   Chromium component: chrome/browser/favicon/favicon_service.h
  gfx::ImageSkia favicon;

  // Audio state.
  bool is_audible = false;
  bool is_muted = false;

  // Position in the tab strip (0-based).
  int tab_index = -1;

  // Which browser window the tab belongs to.
  int window_id = 0;

  // True if the tab has crashed (WebContents is in a crashed state).
  bool has_crashed = false;

  // True if the tab is currently loading.
  bool is_loading = false;
};

// Metadata for a tab group.
struct AstraTabSearchGroupInfo {
  // Opaque group identifier (matches TabGroupId in Chromium).
  std::string group_id;

  // Display title of the group.
  std::u16string title;

  // Group accent color.
  SkColor color = SK_ColorTRANSPARENT;

  // Number of tabs in the group.
  int tab_count = 0;

  // Whether the group is collapsed (hidden tabs).
  bool collapsed = false;
};

// =========================================================================
// Observer interface
// =========================================================================
//
// AstraTabSearchObserver receives notifications when the model state
// changes.  All methods have empty default implementations so subclasses
// only need to override the ones they care about.
//
// Extends base::CheckedObserver for safe observer list management.
// =========================================================================
class AstraTabSearchObserver : public base::CheckedObserver {
 public:
  ~AstraTabSearchObserver() override = default;

  // Called when the full tab list changes (tabs added, removed, reordered).
  virtual void OnTabListChanged(AstraTabSearchModel* model) {}

  // Called when a tab is activated (switched to).
  virtual void OnTabActivated(AstraTabSearchModel* model, int tab_index) {}

  // Called when a tab is closed.
  virtual void OnTabClosed(AstraTabSearchModel* model, int tab_index) {}

  // Called when search results change (query or mode changed).
  virtual void OnSearchResultsChanged(AstraTabSearchModel* model) {}

  // Called when the search mode changes.
  virtual void OnSearchModeChanged(AstraTabSearchModel* model,
                                   AstraTabSearchMode mode) {}

  // Called when the model is about to be destroyed.
  // Observers should remove themselves when they receive this.
  virtual void OnTabSearchModelShutdown(AstraTabSearchModel* model) {}
};

class AstraTabSearchModel {
 public:
  AstraTabSearchModel();
  ~AstraTabSearchModel();

  AstraTabSearchModel(const AstraTabSearchModel&) = delete;
  AstraTabSearchModel& operator=(const AstraTabSearchModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraTabSearchObserver* observer);
  void RemoveObserver(AstraTabSearchObserver* observer);

  // -- Tab list access ------------------------------------------------------

  // Total number of tabs across all windows (open tabs only).
  size_t GetTabCount() const;

  // Get the tab at the given index in the full tab list.
  // Returns nullptr if index is out of range.
  const AstraTabSearchItem* GetTabAt(int index) const;

  // Get the full list of open tabs (all windows, all workspaces).
  const std::vector<AstraTabSearchItem>& GetAllTabs() const { return tabs_; }

  // -- Search ---------------------------------------------------------------

  // Search all tabs by title and URL.  Returns filtered and ranked results.
  std::vector<AstraTabSearchItem> SearchTabs(
      const std::u16string& query) const;

  // Search tabs within a specific mode (e.g. current workspace, audio).
  std::vector<AstraTabSearchItem> SearchTabsInMode(
      const std::u16string& query,
      AstraTabSearchMode mode) const;

  // -- Search mode ----------------------------------------------------------

  // Set the current search mode.  Triggers observer notification.
  void SetSearchMode(AstraTabSearchMode mode);
  AstraTabSearchMode GetSearchMode() const { return search_mode_; }

  // -- Workspace filtering --------------------------------------------------

  // Get all tabs belonging to a specific workspace.
  std::vector<AstraTabSearchItem> GetTabsByWorkspace(
      const std::string& workspace_id) const;

  // Get the currently active tab (across all windows).
  // Returns nullptr if there are no open tabs.
  const AstraTabSearchItem* GetActiveTab() const;

  // -- Special collections --------------------------------------------------

  // Get recently closed tabs (from TabRestoreService projection).
  std::vector<AstraTabSearchItem> GetRecentlyClosedTabs(int max_count) const;

  // Get tabs that are currently playing audio.
  std::vector<AstraTabSearchItem> GetTabsWithAudio() const;

  // Get all pinned tabs.
  std::vector<AstraTabSearchItem> GetPinnedTabs() const;

  // -- Tab groups -----------------------------------------------------------

  // Get all tab groups across all windows.
  std::vector<AstraTabSearchGroupInfo> GetTabGroups() const;

  // Get all tabs in a specific tab group.
  std::vector<AstraTabSearchItem> GetTabsInGroup(
      const std::string& group_id) const;

  // -- Windows --------------------------------------------------------------

  // Number of browser windows with tabs.
  int GetWindowCount() const;

  // Get all tabs in a specific window.
  std::vector<AstraTabSearchItem> GetTabsInWindow(int window_id) const;

  // -- Tab actions ----------------------------------------------------------
  //
  // These delegate to Chromium's TabStripModel / Browser.  The model
  // records the intent and notifies observers; actual tab manipulation
  // happens in Chromium.

  // Switch to (activate) the tab at the given tab-strip index.
  // TODO(astra): Delegate to TabStripModel::ActivateTabAt.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  void SwitchToTab(int tab_index);

  // Close the tab at the given tab-strip index.
  // TODO(astra): Delegate to TabStripModel::CloseWebContentsAt.
  //   Chromium owner: TabStripModel::CloseWebContentsAt
  void CloseTab(int tab_index);

  // Move a tab to a different workspace.
  // TODO(astra): Integrate with Astra workspace service.
  //   Astra owner: AstraWorkspaceService (astra/browser/)
  void MoveTabToWorkspace(int tab_index, const std::string& workspace_id);

  // -- Data management ------------------------------------------------------

  // Set the full tab list (rebuild from TabStripModel projection).
  void SetTabList(std::vector<AstraTabSearchItem> tabs);

  // Refresh the tab list from the underlying data source.
  // TODO(astra): Wire to TabStripModel observer for automatic refresh.
  void RefreshTabList();

  // Set recently closed tabs (projected from TabRestoreService).
  void SetRecentlyClosedTabs(std::vector<AstraTabSearchItem> tabs);

  // Set tab groups metadata.
  void SetTabGroups(std::vector<AstraTabSearchGroupInfo> groups);

  // Set the current workspace ID (for mode filtering).
  void SetCurrentWorkspaceId(const std::string& workspace_id);
  const std::string& current_workspace_id() const {
    return current_workspace_id_;
  }

  // -- Settings -------------------------------------------------------------
  //
  // Presentation and behavior settings stored on the model (UI state only).
  // Changes trigger observer notifications where relevant.

  // Maximum number of search results to return.
  size_t max_search_results() const { return max_search_results_; }
  void set_max_search_results(size_t max);

  // Whether to show tab URLs in search results.
  bool show_tab_urls() const { return show_tab_urls_; }
  void set_show_tab_urls(bool show);

  // Whether to show workspace name in results.
  bool show_workspace_name() const { return show_workspace_name_; }
  void set_show_workspace_name(bool show);

  // Whether to show tab group indicators.
  bool show_tab_groups() const { return show_tab_groups_; }
  void set_show_tab_groups(bool show);

  // Whether to show favicons.
  bool show_favicons() const { return show_favicons_; }
  void set_show_favicons(bool show);

  // Whether to search within URLs (in addition to titles).
  bool search_in_urls() const { return search_in_urls_; }
  void set_search_in_urls(bool enabled);

  // Whether to search only in tab titles (not URLs or other fields).
  bool search_in_tab_titles_only() const { return search_in_tab_titles_only_; }
  void set_search_in_tab_titles_only(bool enabled);

  // Default search mode when the bubble opens.
  AstraTabSearchMode default_search_mode() const { return default_search_mode_; }
  void set_default_search_mode(AstraTabSearchMode mode);

  // Sort order for results.
  AstraTabSearchSortOrder sort_order() const { return sort_order_; }
  void SetSortOrder(AstraTabSearchSortOrder order);

  // Whether to close the tab search bubble when a tab is activated.
  bool close_tab_on_activate() const { return close_tab_on_activate_; }
  void set_close_tab_on_activate(bool enabled);

  // Whether to show the "Recently Closed" section in results.
  bool show_recently_closed_section() const {
    return show_recently_closed_section_;
  }
  void set_show_recently_closed_section(bool show);

  // -- Persistence ----------------------------------------------------------

  // Load presentation settings from PrefService.
  void LoadFromPrefs(PrefService* prefs);

  // Save presentation settings to PrefService.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Constants ------------------------------------------------------------

  static constexpr size_t kMaxSearchResultsMax = 100;
  static constexpr size_t kDefaultMaxSearchResults = 15;
  static constexpr int kDefaultRecentlyClosedCount = 10;

 private:
  // Helper: check if a tab matches the current search mode filter.
  bool PassesModeFilter(const AstraTabSearchItem& tab,
                        AstraTabSearchMode mode) const;

  // Helper: compute search relevance score for a tab.
  // Returns 0.0 if no match.
  double ComputeRelevanceScore(const AstraTabSearchItem& tab,
                               const std::u16string& query,
                               const std::u16string& lower_query) const;

  // Helper: check if a tab matches the search query.
  bool MatchesQuery(const AstraTabSearchItem& tab,
                    const std::u16string& lower_query) const;

  // Helper: sort search results according to current settings.
  void SortResults(std::vector<AstraTabSearchItem>& results) const;

  // Notify observers that the tab list changed.
  void NotifyTabListChanged();

  // Notify observers that search results changed.
  void NotifySearchResultsChanged();

  // Notify observers that search mode changed.
  void NotifySearchModeChanged();

  // Full list of open tabs.
  std::vector<AstraTabSearchItem> tabs_;

  // Recently closed tabs (from TabRestoreService projection).
  std::vector<AstraTabSearchItem> recently_closed_tabs_;

  // Tab groups metadata.
  std::vector<AstraTabSearchGroupInfo> tab_groups_;

  // Current workspace ID (used for mode filtering).
  std::string current_workspace_id_;

  // Current search mode.
  AstraTabSearchMode search_mode_ = AstraTabSearchMode::kAllTabs;

  // Settings (UI state only).
  size_t max_search_results_ = kDefaultMaxSearchResults;
  bool show_tab_urls_ = true;
  bool show_workspace_name_ = true;
  bool show_tab_groups_ = true;
  bool show_favicons_ = true;
  bool search_in_urls_ = true;
  bool search_in_tab_titles_only_ = false;
  AstraTabSearchMode default_search_mode_ = AstraTabSearchMode::kAllTabs;
  AstraTabSearchSortOrder sort_order_ = AstraTabSearchSortOrder::kByRecency;
  bool close_tab_on_activate_ = true;
  bool show_recently_closed_section_ = true;

  // Observers.
  base::ObserverList<AstraTabSearchObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_MODEL_H_
