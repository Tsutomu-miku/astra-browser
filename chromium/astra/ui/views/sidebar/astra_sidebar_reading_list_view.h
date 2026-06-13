#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_reading_list_service.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"

namespace views {
class Label;
class LabelButton;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSidebarReadingListDelegate
// =========================================================================
//
// Delegate interface for reading list user actions that need browser context.
// Implemented by the parent sidebar view or controller.
//
// The reading list view is pure presentation — it never performs browser
// navigation or mutates state outside the service layer.  Navigation and
// other side effects go through this delegate.
// =========================================================================

class AstraSidebarReadingListDelegate {
 public:
  virtual ~AstraSidebarReadingListDelegate() = default;

  // Called when a reading list item is clicked (primary action).
  // |open_in_new_tab| controls whether to open in a new tab or current tab.
  virtual void OnReadingListItemOpen(const GURL& url,
                                     bool open_in_new_tab) = 0;

  // Called when the user marks an entry as read/unread.
  virtual void OnReadingListToggleRead(const GURL& url) = 0;

  // Called when the user removes an entry from the reading list.
  virtual void OnReadingListRemove(const GURL& url) = 0;

  // Called when the user searches the reading list.
  virtual void OnReadingListSearch(const std::string& query) = 0;

  // Called when the user requests adding the current page to the reading list.
  virtual void OnAddCurrentPageRequested() = 0;
};

// =========================================================================
// AstraSidebarReadingListView
// =========================================================================
//
// Sidebar section showing reading list items from Chromium's ReadingListModel.
// Items are grouped into "Unread" and "Read" sections, each with a header
// and a list of AstraReadingListItemView rows.
//
// This is a pure projection view:
//   - It observes AstraReadingListService (which wraps ReadingListModel) for
//     changes and rebuilds the item list reactively.
//   - It never mutates reading list state directly — all user actions
//     (mark read, remove, open) go through the service / delegate.
//   - The truth source is Chromium's ReadingListModel.
//
// User actions:
//   - Click an item → opens the URL in the active tab
//   - Hover → shows "mark as read/unread", "open", and "remove" buttons
//   - Section headers are clickable to collapse/expand each group
//   - Search box filters items by title or URL
//
// Chromium subsystem reused:
//   - ReadingListModel (components/reading_list/core/reading_list_model.h)
//   - ReadingListEntry
//
// Chromium UI reference:
//   - ReadingListUI (chrome/browser/ui/webui/reading_list/)
//   - ReadLaterButtonView (chrome/browser/ui/views/toolbar/read_later_button.h)
//
// TODO(astra): Consider reusing AstraSidebarSectionView as the container
// for the reading list, similar to how favorites and pinned tabs use it.
// The reading list has two sub-groups (Unread / Read), which is a different
// structure than the flat list sections. If we reuse AstraSidebarSectionView,
// we'd need either nested sections or a different item type for group headers.
// For now, this is a standalone view to keep the group logic clean.
// =========================================================================

class AstraSidebarReadingListView : public views::View,
                                    public AstraReadingListServiceObserver,
                                    public AstraReadingListItemDelegate,
                                    public views::TextfieldController {
 public:
  explicit AstraSidebarReadingListView(
      AstraReadingListService* reading_list_service);
  AstraSidebarReadingListView(const AstraSidebarReadingListView&) = delete;
  AstraSidebarReadingListView& operator=(
      const AstraSidebarReadingListView&) = delete;
  ~AstraSidebarReadingListView() override;

  // Refresh the entire reading list from the service.
  void UpdateFromService();

  // Collapse or expand the unread group.
  void SetUnreadExpanded(bool expanded);
  bool unread_expanded() const { return unread_expanded_; }

  // Collapse or expand the read group.
  void SetReadExpanded(bool expanded);
  bool read_expanded() const { return read_expanded_; }

  // -- Delegate ------------------------------------------------------------

  void set_delegate(AstraSidebarReadingListDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarReadingListDelegate* delegate() const { return delegate_; }

  // -- Search --------------------------------------------------------------

  // Search entries by title or URL.
  void SearchEntries(const std::string& query);
  std::string GetSearchQuery() const;

  // -- Sorting -------------------------------------------------------------

  // Set the sort order for reading list items.
  void SetSortOrder(AstraReadingListSortOrder order);
  AstraReadingListSortOrder GetSortOrder() const { return sort_order_; }

  // -- Filter --------------------------------------------------------------

  // Set the display filter (all, unread, favorites, etc.).
  void SetFilter(AstraReadingListView filter);
  AstraReadingListView GetFilter() const { return filter_; }

  // -- Selection -----------------------------------------------------------

  // Set the selected item by URL. Empty URL clears selection.
  void SetSelectedItem(const GURL& url);
  GURL GetSelectedItem() const { return selected_url_; }
  void ClearSelection();

  // -- Bulk operations -----------------------------------------------------

  // Mark all visible unread items as read.
  void MarkAllAsRead();

  // Delete all visible read items.
  void DeleteAllRead();

  // -- Loading / empty state -----------------------------------------------

  // Show or hide the loading state.
  void SetLoading(bool loading);
  bool IsLoading() const { return is_loading_; }

  // Get item counts.
  int GetTotalItemCount() const;
  int GetUnreadCount() const;
  int GetReadCount() const;

  // -- AstraReadingListServiceObserver ------------------------------------

  void OnReadingListEntryAdded(const AstraReadingListEntry& entry) override;
  void OnReadingListEntryRemoved(const GURL& url) override;
  void OnReadingListEntryUpdated(const AstraReadingListEntry& entry) override;
  void OnReadingListEntryStatusChanged(const GURL& url,
                                       bool is_read) override;
  void OnReadingListModelLoaded() override;
  void OnReadingListReordered() override;
  void OnReadingListReloaded() override;

  // -- AstraReadingListItemDelegate --------------------------------------

  void OnReadingListItemClicked(const GURL& url) override;
  void OnReadingListToggleRead(const GURL& url) override;
  void OnReadingListRemove(const GURL& url) override;

  // -- TextfieldController (search box) -----------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Populate the unread items container from the service.
  void PopulateUnreadItems();

  // Populate the read items container from the service.
  void PopulateReadItems();

  // Create a single reading list item view for an entry.
  std::unique_ptr<AstraReadingListItemView> CreateItemView(
      const AstraReadingListEntry& entry);

  // Helper: extract a display-friendly domain string from a URL.
  static std::u16 GetDomainDisplayString(const GURL& url);

  // Find an item view by URL in a given container. Returns nullptr if not found.
  AstraReadingListItemView* FindItemInContainer(views::View* container,
                                                 const GURL& url) const;

  // Get filtered and sorted entries from the service.
  std::vector<AstraReadingListEntry> GetFilteredUnreadEntries() const;
  std::vector<AstraReadingListEntry> GetFilteredReadEntries() const;

  // Update empty state visibility for both sections.
  void UpdateEmptyStates();

  // Update loading state visibility.
  void UpdateLoadingState();

  // Update count labels for both sections.
  void UpdateCountLabels();

  // Handle the unread header click (toggle expand/collapse).
  void OnUnreadHeaderClicked();

  // Handle the read header click (toggle expand/collapse).
  void OnReadHeaderClicked();

  // Handle the "add" button press.
  void OnAddButtonPressed();

  raw_ptr<AstraReadingListService> reading_list_service_ = nullptr;

  // Observation of the reading list service for reactive UI updates.
  base::ScopedObservation<AstraReadingListService,
                          AstraReadingListServiceObserver>
      service_observation_{this};

  // Action delegate for user-initiated operations.
  raw_ptr<AstraSidebarReadingListDelegate> delegate_ = nullptr;

  // Current sort order.
  AstraReadingListSortOrder sort_order_ =
      AstraReadingListSortOrder::kByDateAdded;

  // Current display filter.
  AstraReadingListView filter_ = AstraReadingListView::kAll;

  // Selected item URL (empty if none selected).
  GURL selected_url_;

  // Whether we're in loading state.
  bool is_loading_ = false;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::LabelButton> add_button_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;

  raw_ptr<views::View> unread_header_ = nullptr;
  raw_ptr<views::Label> unread_label_ = nullptr;
  raw_ptr<views::Label> unread_count_label_ = nullptr;
  raw_ptr<views::View> unread_items_container_ = nullptr;
  raw_ptr<views::Label> unread_empty_label_ = nullptr;

  raw_ptr<views::View> read_header_ = nullptr;
  raw_ptr<views::Label> read_label_ = nullptr;
  raw_ptr<views::Label> read_count_label_ = nullptr;
  raw_ptr<views::View> read_items_container_ = nullptr;
  raw_ptr<views::Label> read_empty_label_ = nullptr;

  raw_ptr<views::View> loading_view_ = nullptr;

  // Collapse/expand state for each group.
  bool unread_expanded_ = true;
  bool read_expanded_ = false;  // Read items collapsed by default.
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_
