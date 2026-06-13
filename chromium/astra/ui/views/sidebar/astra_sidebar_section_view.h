#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class Label;
class Textfield;
}  // namespace views

namespace astra {

class AstraSidebarItemView;
class AstraSidebarDropIndicatorView;
struct AstraSidebarDragData;
struct AstraSidebarDropResult;

// Identifies which sidebar section this is. Used by drag-and-drop to
// determine valid drop targets and what action to perform on drop.
enum class AstraSidebarSectionType {
  kFavorites,
  kPinnedTabs,
  kOpenTabs,
  kReadingList,
  kTabGroups,
  kWorkspaces,
  kBookmarks,
  kHistory,
  kDownloads,
};

// Sort order options for sidebar section items.
//
// These are presentation-side sort modes. Actual sort logic is applied
// by each concrete section view based on the selected order.
//
// Chromium reference: bookmarks side panel sort options
//   (chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_view.h)
enum class AstraSidebarSortOrder {
  kAlphabetical,   // Sort by title A-Z
  kDateAdded,      // Sort by date added (newest first)
  kDateModified,   // Sort by date modified (newest first)
  kMostVisited,    // Sort by visit count (most first)
  kManual,         // Manual / user-defined order
};

// Filter options for sidebar section items.
//
// Presentation-side filters. Each concrete section applies the filter
// to its projected data from Chromium services.
enum class AstraSidebarFilter {
  kAll,        // Show all items
  kUnread,     // Show only unread items
  kRead,       // Show only read items
  kPinned,     // Show only pinned items
  kFavorites,  // Show only favorited items
};

// Delegate interface for sidebar section drop events.
// Implemented by the parent AstraSidebarView to validate drops and
// perform drop actions. The section view itself does not know about
// services or tab state — it only handles presentation and hit-testing.
class AstraSidebarSectionDropDelegate {
 public:
  virtual ~AstraSidebarSectionDropDelegate() = default;

  // Called when a drag enters the section. Returns whether the drop
  // is valid for this section and what the drop result would be.
  // |drag_data| describes the item being dragged.
  // |y_in_section| is the y position in the section's local coordinates.
  virtual AstraSidebarDropResult OnDragEnterSection(
      AstraSidebarSectionType section_type,
      const AstraSidebarDragData& drag_data,
      int y_in_section) = 0;

  // Called when the drag moves within the section. Returns updated drop
  // result based on the new position.
  virtual AstraSidebarDropResult OnDragOverSection(
      AstraSidebarSectionType section_type,
      const AstraSidebarDragData& drag_data,
      int y_in_section) = 0;

  // Called when the drag leaves the section.
  virtual void OnDragLeaveSection(
      AstraSidebarSectionType section_type) = 0;

  // Called when a drop occurs in the section.
  // Returns true if the drop was handled.
  virtual bool OnDropInSection(
      AstraSidebarSectionType section_type,
      const AstraSidebarDragData& drag_data,
      const AstraSidebarDropResult& drop_result) = 0;
};

// Delegate interface for section header actions.
// Implemented by the parent sidebar view to handle section-level actions
// like add button clicks and more button context menus.
class AstraSidebarSectionHeaderDelegate {
 public:
  virtual ~AstraSidebarSectionHeaderDelegate() = default;

  // Called when the add button (+) is clicked in the section header.
  virtual void OnSectionAddClicked(AstraSidebarSectionType section_type) = 0;

  // Called when the more button (⋮) is clicked in the section header.
  // |point| is in screen coordinates.
  virtual void OnSectionMoreClicked(AstraSidebarSectionType section_type,
                                    const gfx::Point& point) = 0;

  // Called when a context menu is requested on the section header.
  // |point| is in screen coordinates.
  virtual void OnSectionHeaderContextMenu(AstraSidebarSectionType section_type,
                                          const gfx::Point& point) = 0;
};

// A labeled sidebar section with a header, content area, and optional
// footer.  Sections include: Workspaces (via switcher), Favorites,
// Pinned Tabs, Open Tabs, Bookmarks, History, Downloads.
//
// This is a presentation container. Item data is projected from Chromium
// models by the parent AstraSidebarView or concrete subclass. The section
// does not own or cache product state.
//
// Layout structure:
//   +-- header_view_ ---------------------------------------+
//   |  [icon] [title] [count_badge] [spacer] [search] [+] [⋮]  |
//   +-- content_view_ (scrollable) -------------------------+
//   |  item_0                                               |
//   |  item_1                                               |
//   |  ...                                                  |
//   |  [empty state]  or  [loading state]                   |
//   +-- footer_view_ ---------------------------------------+
//   |  "Show more" link  |  status text                      |
//   +-------------------------------------------------------+
//
// Header elements (all optional except title):
//   - Section icon (left side)
//   - Section title text
//   - Chevron (expand/collapse)
//   - Item count badge
//   - Search box
//   - Add button (+)
//   - More button (⋮)
//   - Context menu (via more button or right-click)
//
// Drop target: the section view can accept dragged items and shows a
// drop indicator to visualize where the item will land. Actual drop
// validation and handling is delegated to the parent sidebar.
//
// TODO(astra): Wire header icon to vector icon resources from
//   ui/resources/vector_icons/ once building against full Chromium.
//   Chromium owner: ui/resources/vector_icons/
class AstraSidebarSectionView : public views::View {
 public:
  explicit AstraSidebarSectionView(const std::u16string& title,
                                   AstraSidebarSectionType type);
  AstraSidebarSectionView(const AstraSidebarSectionView&) = delete;
  AstraSidebarSectionView& operator=(const AstraSidebarSectionView&) = delete;
  ~AstraSidebarSectionView() override;

  // -- Title ---------------------------------------------------------------

  // Set the section title text shown in the header.
  void SetTitle(const std::u16string& title);
  // Get the current section title.
  const std::u16string& GetTitle() const { return title_; }

  // -- Expand / collapse ---------------------------------------------------

  // Set whether the section is expanded (content visible) or collapsed
  // (only header visible).  Sections are expanded by default.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return is_expanded_; }

  // Toggle the expanded state.
  void ToggleExpanded();

  // -- Item count ----------------------------------------------------------

  // Set the total item count shown in the header badge.
  // The count is a presentation value set by the parent/controller.
  void SetItemCount(int count);
  int GetItemCount() const { return item_count_; }

  // Set whether the item count badge is visible in the header.
  void SetShowItemCount(bool show);
  bool GetShowItemCount() const { return show_item_count_; }

  // -- Chevron -------------------------------------------------------------

  // Set whether the expand/collapse chevron is visible.
  void SetShowChevron(bool show);
  bool GetShowChevron() const { return show_chevron_; }

  // -- Search --------------------------------------------------------------

  // Set whether the search box is visible in the header.
  void SetShowSearch(bool show);
  bool GetShowSearch() const { return show_search_; }

  // Set the current search query text.
  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Add button ----------------------------------------------------------

  // Set whether the "add" button (+) is visible in the header.
  void SetShowAddButton(bool show);
  bool GetShowAddButton() const { return show_add_button_; }

  // -- More button ---------------------------------------------------------

  // Set whether the "more" button (⋮) is visible in the header.
  void SetShowMoreButton(bool show);
  bool GetShowMoreButton() const { return show_more_button_; }

  // -- Context menu --------------------------------------------------------

  // Set whether right-clicking the section header shows a context menu.
  void SetShowContextMenu(bool show);
  bool GetShowContextMenu() const { return show_context_menu_; }

  // -- Section color -------------------------------------------------------

  // Set a tint color for the section (used for icon and accent elements).
  void SetSectionColor(SkColor color);
  SkColor GetSectionColor() const { return section_color_; }

  // -- Drag handle ---------------------------------------------------------

  // Set whether the section shows a drag handle (for reordering sections).
  void SetShowDragHandle(bool show);
  bool GetShowDragHandle() const { return show_drag_handle_; }

  // -- Keyboard navigation --------------------------------------------------

  // Set the index of the currently selected item (for keyboard navigation).
  // -1 means no item is selected.
  void SetSelectedItemIndex(int index);
  int GetSelectedItemIndex() const { return selected_item_index_; }

  // Select the next item (down arrow). Returns true if selection changed.
  bool SelectNextItem();

  // Select the previous item (up arrow). Returns true if selection changed.
  bool SelectPreviousItem();

  // Activate the currently selected item (Enter key). Returns true if handled.
  bool ActivateSelectedItem();

  // -- Animation -----------------------------------------------------------

  // Set whether expand/collapse uses smooth animation.
  void SetAnimated(bool animated);
  bool GetAnimated() const { return animated_; }

  // -- Header delegate -----------------------------------------------------

  // Set the header action delegate. Not owned by this view.
  void set_header_delegate(AstraSidebarSectionHeaderDelegate* delegate) {
    header_delegate_ = delegate;
  }

  // -- Drag and drop -------------------------------------------------------

  // Set whether items in this section can be reordered via drag-and-drop.
  void SetDragDropEnabled(bool enabled);
  bool GetDragDropEnabled() const { return drag_drop_enabled_; }

  // -- Sub-view access -----------------------------------------------------

  // Get the header view (top bar with title, buttons, etc.).
  views::View* GetHeaderView() { return header_view_; }
  const views::View* GetHeaderView() const { return header_view_; }

  // Get the content view (scrollable area containing items).
  views::View* GetContentView() { return content_view_; }
  const views::View* GetContentView() const { return content_view_; }

  // Get the footer view (bottom bar with "show more" link).
  views::View* GetFooterView() { return footer_view_; }
  const views::View* GetFooterView() const { return footer_view_; }

  // -- Item management -----------------------------------------------------

  // Add an item view to the content area. Ownership transfers to the
  // view hierarchy.  Returns the raw pointer.
  void AddItemView(views::View* item);

  // Remove all items from the content area.
  void RemoveAllItems();

  // Get the item view at the given index, or nullptr if out of bounds.
  views::View* GetItemViewAt(int index);
  const views::View* GetItemViewAt(int index) const;

  // Get the number of item views currently in the content area.
  int GetItemViewCount() const;

  // -- Sort order ----------------------------------------------------------

  // Set the current sort order.  Concrete subclasses apply the sort
  // to their projected data.  The base class stores the preference.
  void SetSortOrder(AstraSidebarSortOrder order);
  AstraSidebarSortOrder GetSortOrder() const { return sort_order_; }

  // -- Filter --------------------------------------------------------------

  // Set the current filter.  Concrete subclasses apply the filter
  // to their projected data.  The base class stores the preference.
  void SetFilter(AstraSidebarFilter filter);
  AstraSidebarFilter GetFilter() const { return filter_; }

  // -- Loading / empty state -----------------------------------------------

  // Show or hide the loading state indicator.
  void SetLoading(bool loading);
  bool IsLoading() const { return is_loading_; }

  // Show or hide the empty state message.
  void SetEmpty(bool empty);
  bool IsEmpty() const { return is_empty_; }

  // Set the empty state message text.
  void SetEmptyStateText(const std::u16string& text);

  // -- Item helpers (backwards compat with AstraSidebarItemView) -----------

  // Add an AstraSidebarItemView to the section. Ownership transfers to
  // the view hierarchy.
  AstraSidebarItemView* AddItem(std::unique_ptr<AstraSidebarItemView> item);

  // Insert an item at the given index.
  AstraSidebarItemView* InsertItemAt(size_t index,
                                     std::unique_ptr<AstraSidebarItemView> item);

  // Remove the item at the given index.
  void RemoveItemAt(size_t index);

  // Get the item at the given index, or nullptr if out of bounds.
  AstraSidebarItemView* GetItemAt(size_t index) const;

  // Remove all items from the section.
  void ClearItems();

  // Get the number of items currently in the section.
  size_t GetItemCountTyped() const;

  // Set section visibility and update layout accordingly.
  void SetSectionVisible(bool visible);

  // Get the type of this section.
  AstraSidebarSectionType section_type() const { return section_type_; }

  // -- Drop target ---------------------------------------------------------

  // Set the drop delegate. Not owned by this view.
  void set_drop_delegate(AstraSidebarSectionDropDelegate* delegate) {
    drop_delegate_ = delegate;
  }

  // Called by the drag controller when a drag enters this section.
  void OnDragEnter(const AstraSidebarDragData& drag_data, int y_in_section);

  // Called when the drag moves within this section.
  void OnDragOver(const AstraSidebarDragData& drag_data, int y_in_section);

  // Called when the drag leaves this section.
  void OnDragLeave();

  // Called when a drop occurs in this section. Returns true if handled.
  bool OnDrop(const AstraSidebarDragData& drag_data);

  // Compute the insertion index for a given y position.
  int GetInsertIndexFromY(int y_in_section) const;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

 protected:
  // Called when the add button is clicked.  Subclasses can override.
  virtual void OnAddButtonClicked();

  // Called when the more button is clicked.  Subclasses can override.
  virtual void OnMoreButtonClicked();

  // Called when the search query changes.  Subclasses can override.
  virtual void OnSearchQueryChanged(const std::u16string& query);

  // Called when the "show more" footer link is clicked.
  virtual void OnShowMoreClicked();

  // Called when the section header is clicked (toggles expand/collapse).
  virtual void OnHeaderClicked();

  // Returns the items container view (inside content_view_).
  views::View* items_container() { return items_container_; }
  const views::View* items_container() const { return items_container_; }

  // Returns the scroll view wrapping the content.
  views::ScrollView* scroll_view() { return scroll_view_; }

 private:
  // Build the header view with all child elements.
  void BuildHeader();

  // Build the content view (scrollable items area + empty/loading states).
  void BuildContent();

  // Build the footer view.
  void BuildFooter();

  // Update the visibility of header elements based on current flags.
  void UpdateHeaderVisibility();

  // Update the visibility of content states (items vs empty vs loading).
  void UpdateContentState();

  // Update the count badge text.
  void UpdateItemCountBadge();

  // Update the chevron rotation based on expanded state.
  void UpdateChevron();

  // Returns the y coordinate of the items container (in section coords).
  int GetItemsContainerY() const;

  // Section type identifier.
  AstraSidebarSectionType section_type_;

  // Cached state values.
  std::u16string title_;
  bool is_expanded_ = true;
  int item_count_ = 0;
  bool show_item_count_ = false;
  bool show_chevron_ = true;
  bool show_search_ = false;
  std::u16string search_query_;
  bool show_add_button_ = false;
  bool show_more_button_ = false;
  bool show_context_menu_ = true;
  bool show_drag_handle_ = false;
  SkColor section_color_ = SK_ColorTRANSPARENT;
  bool drag_drop_enabled_ = false;
  AstraSidebarSortOrder sort_order_ = AstraSidebarSortOrder::kManual;
  AstraSidebarFilter filter_ = AstraSidebarFilter::kAll;
  bool is_loading_ = false;
  bool is_empty_ = true;
  int selected_item_index_ = -1;
  bool animated_ = false;

  // Header child views (owned by view hierarchy).
  raw_ptr<views::View> header_view_ = nullptr;
  raw_ptr<views::ImageView> header_icon_ = nullptr;
  raw_ptr<views::ImageView> drag_handle_view_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::Label> count_badge_ = nullptr;
  raw_ptr<views::ImageView> chevron_view_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> add_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  // Content child views (owned by view hierarchy).
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_view_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::Label> empty_state_label_ = nullptr;
  raw_ptr<views::View> loading_state_view_ = nullptr;

  // Footer child views (owned by view hierarchy).
  raw_ptr<views::View> footer_view_ = nullptr;
  raw_ptr<views::Label> show_more_label_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;

  // Drop target state.
  raw_ptr<AstraSidebarSectionDropDelegate> drop_delegate_ = nullptr;
  raw_ptr<AstraSidebarDropIndicatorView> drop_indicator_ = nullptr;
  bool has_active_drag_ = false;

  // Header action delegate (not owned).
  raw_ptr<AstraSidebarSectionHeaderDelegate> header_delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_
