#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "ui/views/view.h"
#include "url/gurl.h"

class Profile;

namespace history {
class HistoryService;
struct QueryResults;
}  // namespace history

namespace views {
class Label;
class LabelButton;
class MenuRunner;
}  // namespace views

namespace astra {

class AstraHistoryItemView;

// A sidebar section that shows recently visited pages, projected from
// Chromium's HistoryService.
//
// This is a presentation-only view. It never stores or mutates history
// state — it only queries and displays Chromium's HistoryService data.
// HistoryService is the single source of truth for browsing history.
//
// Layout (top to bottom):
//   - Section header ("History")
//   - Loading indicator (shown while query is in flight)
//   - Empty state (shown when there is no history)
//   - "Today" group header + items visited today
//   - "Yesterday" group header + items visited yesterday
//   - "Last 7 days" group header + items from the past week
//   - "Show full history" footer link (opens chrome://history)
//
// The view observes HistoryService via HistoryServiceObserver and
// refreshes its projection whenever history changes (pages added,
// removed, etc.).
//
// Chromium owner: HistoryService (components/history/core/browser/history_service.h)
// Chromium WebUI: chrome://history (chrome/browser/ui/webui/history/history_ui.cc)
//
// TODO(astra): Implement HistoryServiceObserver for reactive updates.
// Currently, the view refreshes on construction and can be manually
// refreshed via Refresh(). Real-time observer support requires wiring
// into HistoryService's observer list.
// Chromium observer: HistoryServiceObserver
//   (components/history/core/browser/history_service_observer.h)
//
// Accessibility: The history section has an accessible name of "History"
// and each history item announces its title and URL via screen reader.
// Arrow key navigation moves between items in the list.
class AstraSidebarHistoryView : public views::View,
                                public AstraHistoryItemView::Delegate {
 public:
  // Delegate interface for actions that need browser-level context,
  // such as opening a URL in a tab. The parent sidebar view implements
  // this so the history section doesn't need direct access to Browser.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Open a URL from history. |in_new_tab| controls whether the URL
    // opens in the active tab or a new tab.
    virtual void OpenHistoryURL(const GURL& url, bool in_new_tab) = 0;

    // Open the full chrome://history page.
    virtual void OpenFullHistory() = 0;
  };

  // Time group buckets for grouping history entries.
  enum class TimeGroup {
    kToday,
    kYesterday,
    kLast7Days,
  };

  explicit AstraSidebarHistoryView(Profile* profile);
  AstraSidebarHistoryView(const AstraSidebarHistoryView&) = delete;
  AstraSidebarHistoryView& operator=(const AstraSidebarHistoryView&) = delete;
  ~AstraSidebarHistoryView() override;

  // Set the delegate for navigation actions. Not owned.
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Refresh the history list from HistoryService.
  // Initiates an asynchronous query; results arrive via OnHistoryQueryComplete.
  void Refresh();

  // Set the maximum number of history items to show.
  void set_max_items(size_t max) { max_items_ = max; }
  size_t max_items() const { return max_items_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- AstraHistoryItemView::Delegate -------------------------------------

  void OnHistoryItemClicked(const GURL& url) override;
  void OnHistoryItemRemoved(const GURL& url) override;

  // -- Keyboard navigation ------------------------------------------------

  // Move keyboard focus to the next history item (Down arrow).
  // Returns true if focus moved.
  bool MoveFocusToNextItem();

  // Move keyboard focus to the previous history item (Up arrow).
  // Returns true if focus moved.
  bool MoveFocusToPreviousItem();

  // Returns the total number of visible history items across all groups.
  size_t GetVisibleItemCount() const;

  // Returns the item view at the given flat index across all groups,
  // or nullptr if out of bounds.
  AstraHistoryItemView* GetItemAtFlatIndex(size_t index) const;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Get HistoryService from the profile. Returns nullptr if not available.
  // TODO(astra): Use HistoryServiceFactory::GetForProfile(profile) when
  // building against a full Chromium checkout. In the overlay repo, we
  // forward-declare and access via the standard factory pattern.
  // Chromium owner: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  history::HistoryService* GetHistoryService();

  // Callback for the async history query. Populates the item views with
  // the results, grouped by time bucket.
  // The |results| parameter is a QueryResults struct containing URLRows
  // and visit info.
  //
  // TODO(astra): Use history::QueryResults from
  // components/history/core/browser/history_types.h. For the overlay,
  // we accept a simplified vector or use the full type.
  void OnHistoryQueryComplete(const history::QueryResults& results);

  // Group a history entry into a time bucket (Today, Yesterday, Last 7 days).
  TimeGroup GroupTime(base::Time visit_time) const;

  // Clear all history items and group containers.
  void ClearItems();

  // Add a history entry item to the appropriate time group.
  // |title| is the page title, |url| is the page URL, |visit_time| is
  // the most recent visit time.
  void AddHistoryItem(const std::u16string& title,
                      const GURL& url,
                      base::Time visit_time);

  // Get or create the items container for a given time group.
  views::View* GetGroupContainer(TimeGroup group);

  // Ensure the group header for |group| exists and is visible.
  void EnsureGroupHeader(TimeGroup group);

  // -- Loading and empty states -------------------------------------------

  // Show the loading state indicator and hide items.
  void ShowLoadingState();

  // Hide the loading state indicator.
  void HideLoadingState();

  // Show the empty state (no history items).
  void ShowEmptyState();

  // Hide the empty state.
  void HideEmptyState();

  // Update the visibility of groups, loading indicator, and empty state
  // based on the current number of items.
  void UpdateStateVisibility();

  // -- Context menu -------------------------------------------------------

  // Show the context menu for a history item at the given screen point.
  // TODO(astra): Use views::MenuRunner and MenuModelAdapter for a proper
  // Chromium-style context menu.
  // Chromium owner: views::MenuRunner (ui/views/controls/menu/menu_runner.h)
  void ShowItemContextMenu(AstraHistoryItemView* item,
                           const gfx::Point& screen_point);

  // Handle "Remove from history" context menu action.
  void OnRemoveFromHistory(const GURL& url);

  // -- Callbacks ----------------------------------------------------------

  // Callback for the "Show full history" link.
  void OnShowFullHistoryClicked();

  // Callback when a history item is clicked.
  void OnHistoryItemClicked(const GURL& url);

  raw_ptr<Profile> profile_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // Maximum number of history items to display.
  size_t max_items_ = 15;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Label> header_label_ = nullptr;

  // Loading state indicator.
  raw_ptr<views::Label> loading_label_ = nullptr;

  // Empty state message.
  raw_ptr<views::Label> empty_state_label_ = nullptr;

  // Group containers (for Today, Yesterday, Last 7 days).
  raw_ptr<views::View> today_group_ = nullptr;
  raw_ptr<views::Label> today_header_ = nullptr;
  raw_ptr<views::View> today_items_ = nullptr;

  raw_ptr<views::View> yesterday_group_ = nullptr;
  raw_ptr<views::Label> yesterday_header_ = nullptr;
  raw_ptr<views::View> yesterday_items_ = nullptr;

  raw_ptr<views::View> last7days_group_ = nullptr;
  raw_ptr<views::Label> last7days_header_ = nullptr;
  raw_ptr<views::View> last7days_items_ = nullptr;

  // "Show full history" footer link.
  raw_ptr<views::LabelButton> show_full_history_link_ = nullptr;

  // Loading state indicator.
  bool is_loading_ = false;

  // Weak pointer factory for canceling pending async history queries.
  // When this view is destroyed, any in-flight callbacks are silently
  // dropped instead of accessing freed memory.
  //
  // TODO(astra): Replace with base::CancelableTaskTracker for proper
  // integration with HistoryService's task tracking system. HistoryService
  // returns a CancelableTaskTracker::TaskId from QueryHistory, and the
  // tracker can cancel pending tasks explicitly.
  // Chromium owner: base::CancelableTaskTracker (base/task/cancelable_task_tracker.h)
  base::WeakPtrFactory<AstraSidebarHistoryView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_HISTORY_VIEW_H_
