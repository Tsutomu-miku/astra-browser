#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"

class Profile;

namespace history {
class HistoryService;
struct QueryResults;
}  // namespace history

namespace astra {

class AstraHistoryItemView;

// =========================================================================
// AstraHistoryItemInfo — presentation data for a history entry
// =========================================================================
//
// A lightweight data struct representing a history entry projected from
// Chromium's HistoryService.  This is presentation-facing — it carries
// only the fields needed for display and interaction.
//
// Truth source: history::URLRow / history::VisitRow (components/history/)
//
// TODO(astra): Wire to HistoryService for real history data.
//   Chromium owner: HistoryService (components/history/core/browser/)
struct AstraHistoryItemInfo {
  // Unique identifier for the history entry.
  std::string id;

  // Page title.
  std::u16string title;

  // Full URL of the page.
  GURL url;

  // Hostname (domain) for secondary display.
  std::u16string hostname;

  // Most recent visit time.
  base::Time visit_time;

  // Total number of visits.
  int visit_count = 1;

  // Whether this URL was typed directly by the user.
  bool is_typed_visit = false;

  // Whether the entry has a custom favicon.
  bool has_favicon = false;

  // Time group bucket (for grouping display).
  enum class TimeGroup {
    kToday,
    kYesterday,
    kLastWeek,
    kOlder,
  };

  TimeGroup time_group = TimeGroup::kToday;
};

// =========================================================================
// AstraSidebarHistoryDelegate — action callbacks for history section
// =========================================================================
//
// Delegate interface for user actions originating in the history section.
// The history section view is pure presentation — it never mutates
// Chromium's HistoryService directly.
class AstraSidebarHistoryDelegate {
 public:
  virtual ~AstraSidebarHistoryDelegate() = default;

  // Called when a history item is clicked (primary action — open URL).
  virtual void OnHistoryItemClicked(const std::string& item_id) = 0;

  // Called when a history item is middle-clicked (open in new tab).
  virtual void OnHistoryItemMiddleClicked(const std::string& item_id) = 0;

  // Called when a history item is right-clicked (context menu).
  // |point| is in screen coordinates.
  virtual void OnHistoryItemRightClicked(const std::string& item_id,
                                         const gfx::Point& point) = 0;

  // Called when a single history item is removed.
  virtual void OnRemoveHistoryItem(const std::string& item_id) = 0;

  // Called when the user requests clearing all history.
  virtual void OnClearAllHistoryRequested() = 0;

  // Called when the user requests removing all items for a domain.
  virtual void OnRemoveHistoryForDomain(const std::string& domain) = 0;

  // Called when the user types in the search box.
  virtual void OnSearchHistory(const std::u16string& query) = 0;
};

// =========================================================================
// AstraSidebarHistoryView — history sidebar section
// =========================================================================
//
// A sidebar section that shows recently visited pages, projected from
// Chromium's HistoryService.  Extends AstraSidebarSectionView for common
// section chrome (header, search, chevron, etc.) and adds history-specific
// presentation logic including date grouping.
//
// This is a presentation-only view. It never stores or mutates history
// state — it only queries and displays Chromium's HistoryService data.
// HistoryService is the single source of truth for browsing history.
//
// Layout (top to bottom):
//   - Section header ("History") with search and more options
//   - Loading indicator (shown while query is in flight)
//   - Empty state (shown when there is no history)
//   - "Today" group header + items visited today
//   - "Yesterday" group header + items visited yesterday
//   - "Last week" group header + items from last week
//   - "Older" group header + older items (optional)
//   - "Show full history" footer link (opens chrome://history)
//
// Chromium owner: HistoryService (components/history/core/browser/history_service.h)
// Chromium WebUI: chrome://history (chrome/browser/ui/webui/history/history_ui.cc)
//
// TODO(astra): Implement HistoryServiceObserver for reactive updates.
//   Chromium observer: HistoryServiceObserver
//     (components/history/core/browser/history_service_observer.h)
class AstraSidebarHistoryView : public AstraSidebarSectionView {
 public:
  // Time group buckets for grouping history entries.
  using TimeGroup = AstraHistoryItemInfo::TimeGroup;

  explicit AstraSidebarHistoryView(Profile* profile);
  AstraSidebarHistoryView(const AstraSidebarHistoryView&) = delete;
  AstraSidebarHistoryView& operator=(const AstraSidebarHistoryView&) = delete;
  ~AstraSidebarHistoryView() override;

  // -- Delegate ------------------------------------------------------------

  void set_delegate(AstraSidebarHistoryDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarHistoryDelegate* delegate() const { return delegate_; }

  // -- History data projection --------------------------------------------

  // Set the full list of history items. Replaces all existing items.
  void SetHistoryItems(const std::vector<AstraHistoryItemInfo>& items);

  // Get the total number of history items.
  int GetHistoryItemCount() const;

  // Get history item info at the given index.
  AstraHistoryItemInfo GetHistoryItemAt(int index) const;

  // Add a history item to the appropriate group.
  void AddHistoryItem(const AstraHistoryItemInfo& item);

  // Remove the history item at the given flat index.
  void RemoveHistoryItem(int index);

  // Clear all history items (presentation only — does not delete history).
  void ClearAllHistory();

  // -- Selection -----------------------------------------------------------

  // Set the selected item by flat index. -1 clears selection.
  void SetSelectedItem(int index);
  int GetSelectedIndex() const { return selected_index_; }
  void ClearSelection();

  // -- Date grouping -------------------------------------------------------

  // Set whether to group items by date.
  void SetGroupByDate(bool group);
  bool GetGroupByDate() const { return group_by_date_; }

  // Toggle visibility of individual date groups.
  void SetShowToday(bool show);
  bool GetShowToday() const { return show_today_; }

  void SetShowYesterday(bool show);
  bool GetShowYesterday() const { return show_yesterday_; }

  void SetShowLastWeek(bool show);
  bool GetShowLastWeek() const { return show_last_week_; }

  void SetShowOlder(bool show);
  bool GetShowOlder() const { return show_older_; }

  // Get item counts for each date group.
  int GetTodayCount() const;
  int GetYesterdayCount() const;
  int GetLastWeekCount() const;
  int GetOlderCount() const;

  // Get the total number of date groups that have items and are visible.
  int GetGroupCount() const;

  // Get info about the group at the given visible group index.
  struct GroupInfo {
    TimeGroup group;
    std::u16string title;
    int item_count = 0;
    bool is_expanded = true;
  };
  GroupInfo GetGroupAt(int index) const;

  // -- Search --------------------------------------------------------------

  // Filter history items by search query.
  void SearchHistory(const std::u16string& query);

  // Get the number of items matching the current search.
  int GetSearchResultsCount() const;

  // Set whether to show search results (vs full list).
  void SetShowSearchResults(bool show);
  bool GetShowSearchResults() const { return show_search_results_; }

  // -- Domain operations ---------------------------------------------------

  // Remove all history items for a given domain (presentation only).
  void RemoveItemsForDomain(const std::string& domain);

  // Get the count of unique domains in the current items.
  int GetDomainCount() const;

  // -- Display options -----------------------------------------------------

  // Set whether to show favicons in history items.
  void SetShowFavicons(bool show);
  bool GetShowFavicons() const { return show_favicons_; }

  // Set whether to show visit time in history items.
  void SetShowTime(bool show);
  bool GetShowTime() const { return show_time_; }

  // Set the maximum number of items per date group.
  void SetMaxItemsPerGroup(int max);
  int GetMaxItemsPerGroup() const { return max_items_per_group_; }

  // -- Group expand/collapse -----------------------------------------------

  // Expand a date group by index.
  void ExpandGroup(int group_index);

  // Collapse a date group by index.
  void CollapseGroup(int group_index);

  // Check if a date group is expanded.
  bool IsGroupExpanded(int group_index) const;

  // Toggle a date group's expanded state.
  void ToggleGroup(int group_index);

  // -- Refresh from service ------------------------------------------------

  // Refresh the history list from HistoryService.
  // Initiates an asynchronous query; results arrive via OnHistoryQueryComplete.
  void Refresh();

  // Set the maximum number of history items to show.
  void set_max_items(size_t max) { max_items_ = max; }
  size_t max_items() const { return max_items_; }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- Keyboard navigation -------------------------------------------------

  bool MoveFocusToNextItem();
  bool MoveFocusToPreviousItem();
  size_t GetVisibleItemCount() const;
  AstraHistoryItemView* GetItemAtFlatIndex(size_t index) const;

 protected:
  // AstraSidebarSectionView overrides.
  void OnSearchQueryChanged(const std::u16string& query) override;
  void OnShowMoreClicked() override;
  void OnMoreButtonClicked() override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildHistoryLayout();

  // Get HistoryService from the profile. Returns nullptr if not available.
  // TODO(astra): Use HistoryServiceFactory::GetForProfile(profile).
  history::HistoryService* GetHistoryService();

  // Callback for the async history query.
  void OnHistoryQueryComplete(const history::QueryResults& results);

  // Group a history entry into a time bucket.
  static TimeGroup GroupTime(base::Time visit_time);

  // Clear all history item views and group containers.
  void ClearItemsViews();

  // Add a history entry item to the appropriate time group.
  void AddHistoryItemView(const AstraHistoryItemInfo& info);

  // Get or create the items container for a given time group.
  views::View* GetGroupContainer(TimeGroup group);

  // Ensure the group header for |group| exists and is visible.
  void EnsureGroupHeader(TimeGroup group);

  // Update the visibility of groups based on settings and item counts.
  void UpdateGroupsVisibility();

  // Rebuild all groups from the current items_ vector.
  void RebuildGroupsFromItems();

  // Get the title string for a time group.
  static std::u16string GetGroupTitle(TimeGroup group);

  // -- Loading and empty states -------------------------------------------

  void ShowLoadingState();
  void HideLoadingState();
  void ShowEmptyState();
  void HideEmptyState();
  void UpdateStateVisibility();

  // -- Context menu -------------------------------------------------------

  void ShowItemContextMenu(AstraHistoryItemView* item,
                           const gfx::Point& screen_point);
  void OnRemoveFromHistory(const GURL& url);

  // -- Callbacks ----------------------------------------------------------

  void OnShowFullHistoryClicked();
  void OnHistoryItemClicked(const GURL& url);
  void OnHistoryItemRemoved(const GURL& url);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<AstraSidebarHistoryDelegate> delegate_ = nullptr;

  // Cached history items (projection from HistoryService).
  std::vector<AstraHistoryItemInfo> history_items_;

  // Selection state.
  int selected_index_ = -1;

  // Display options.
  bool group_by_date_ = true;
  bool show_today_ = true;
  bool show_yesterday_ = true;
  bool show_last_week_ = true;
  bool show_older_ = true;
  bool show_favicons_ = true;
  bool show_time_ = true;
  int max_items_per_group_ = 15;
  bool show_search_results_ = false;
  size_t max_items_ = 50;

  // Group expansion state.
  bool today_expanded_ = true;
  bool yesterday_expanded_ = true;
  bool last_week_expanded_ = true;
  bool older_expanded_ = true;

  // Group views (owned by view hierarchy, referenced via raw_ptr).
  raw_ptr<views::View> today_group_ = nullptr;
  raw_ptr<views::Label> today_header_ = nullptr;
  raw_ptr<views::View> today_items_ = nullptr;

  raw_ptr<views::View> yesterday_group_ = nullptr;
  raw_ptr<views::Label> yesterday_header_ = nullptr;
  raw_ptr<views::View> yesterday_items_ = nullptr;

  raw_ptr<views::View> last_week_group_ = nullptr;
  raw_ptr<views::Label> last_week_header_ = nullptr;
  raw_ptr<views::View> last_week_items_ = nullptr;

  raw_ptr<views::View> older_group_ = nullptr;
  raw_ptr<views::Label> older_header_ = nullptr;
  raw_ptr<views::View> older_items_ = nullptr;

  // Loading state indicator.
  bool is_loading_ = false;

  // Weak pointer factory for canceling pending async history queries.
  //
  // TODO(astra): Replace with base::CancelableTaskTracker for proper
  // integration with HistoryService's task tracking system.
  // Chromium owner: base::CancelableTaskTracker (base/task/cancelable_task_tracker.h)
  base::WeakPtrFactory<AstraSidebarHistoryView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_
