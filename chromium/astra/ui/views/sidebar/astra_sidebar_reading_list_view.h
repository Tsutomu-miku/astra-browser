#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_reading_list_service.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"

namespace views {
class Label;
}  // namespace views

namespace astra {

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
//   - Hover → shows "mark as read/unread" and "remove" buttons
//   - Section headers are clickable to collapse/expand each group
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
                                    public AstraReadingListItemDelegate {
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

  // -- AstraReadingListServiceObserver ------------------------------------

  void OnReadingListEntryAdded(const AstraReadingListEntry& entry) override;
  void OnReadingListEntryRemoved(const GURL& url) override;
  void OnReadingListEntryUpdated(const AstraReadingListEntry& entry) override;
  void OnReadingListModelLoaded() override;

  // -- AstraReadingListItemDelegate --------------------------------------

  void OnReadingListItemClicked(const GURL& url) override;
  void OnReadingListToggleRead(const GURL& url) override;
  void OnReadingListRemove(const GURL& url) override;

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

  raw_ptr<AstraReadingListService> reading_list_service_ = nullptr;

  // Observation of the reading list service for reactive UI updates.
  base::ScopedObservation<AstraReadingListService,
                          AstraReadingListServiceObserver>
      service_observation_{this};

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> unread_header_ = nullptr;
  raw_ptr<views::Label> unread_label_ = nullptr;
  raw_ptr<views::Label> unread_count_label_ = nullptr;
  raw_ptr<views::View> unread_items_container_ = nullptr;

  raw_ptr<views::View> read_header_ = nullptr;
  raw_ptr<views::Label> read_label_ = nullptr;
  raw_ptr<views::Label> read_count_label_ = nullptr;
  raw_ptr<views::View> read_items_container_ = nullptr;

  // Collapse/expand state for each group.
  bool unread_expanded_ = true;
  bool read_expanded_ = false;  // Read items collapsed by default.
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_READING_LIST_VIEW_H_
