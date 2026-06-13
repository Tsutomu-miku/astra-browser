#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace views {
class Label;
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

// A labeled sidebar section with a header and a vertical list of items.
// Sections include: Workspaces (via switcher), Favorites, Pinned Tabs,
// and Open Tabs.
//
// This is a presentation container. Item data is projected from Chromium
// models by the parent AstraSidebarView. The section does not own or cache
// tab/workspace state.
//
// Drop target: the section view can accept dragged items and shows a
// drop indicator to visualize where the item will land. Actual drop
// validation and handling is delegated to the parent sidebar.
class AstraSidebarSectionView : public views::View {
 public:
  explicit AstraSidebarSectionView(const std::u16string& title,
                                   AstraSidebarSectionType type);
  AstraSidebarSectionView(const AstraSidebarSectionView&) = delete;
  AstraSidebarSectionView& operator=(const AstraSidebarSectionView&) = delete;
  ~AstraSidebarSectionView() override;

  // Add an item to the end of the section. Ownership is transferred to
  // the view hierarchy.
  AstraSidebarItemView* AddItem(std::unique_ptr<AstraSidebarItemView> item);

  // Insert an item at the given index. Ownership is transferred to
  // the view hierarchy.
  // TODO(astra): Used for incremental sidebar updates when we switch from
  // full rebuild to per-change item insertion. Currently unused until
  // TabStripModelObserver incremental updates are fully implemented.
  // Chromium owner: views::View (AddChildViewAt)
  AstraSidebarItemView* InsertItemAt(size_t index,
                                     std::unique_ptr<AstraSidebarItemView> item);

  // Remove the item at the given index. The item is destroyed.
  // TODO(astra): Used for incremental sidebar updates. Currently unused
  // until TabStripModelObserver incremental updates are fully implemented.
  void RemoveItemAt(size_t index);

  // Get the item at the given index, or nullptr if out of bounds.
  // TODO(astra): Used for incremental sidebar updates. Currently unused.
  AstraSidebarItemView* GetItemAt(size_t index) const;

  // Remove all items from the section.
  void ClearItems();

  // Get the number of items currently in the section.
  size_t GetItemCount() const;

  // Set section visibility and update layout accordingly.
  void SetSectionVisible(bool visible);

  // Get the type of this section.
  AstraSidebarSectionType section_type() const { return section_type_; }

  // -- Drop target --------------------------------------------------------

  // Set the drop delegate. Not owned by this view.
  void set_drop_delegate(AstraSidebarSectionDropDelegate* delegate) {
    drop_delegate_ = delegate;
  }

  // Called by the drag controller when a drag enters this section.
  // |drag_data| describes the item being dragged.
  // |y_in_section| is the y position in section local coordinates.
  void OnDragEnter(const AstraSidebarDragData& drag_data, int y_in_section);

  // Called when the drag moves within this section.
  void OnDragOver(const AstraSidebarDragData& drag_data, int y_in_section);

  // Called when the drag leaves this section.
  void OnDragLeave();

  // Called when a drop occurs in this section. Returns true if handled.
  bool OnDrop(const AstraSidebarDragData& drag_data);

  // Compute the insertion index for a given y position in the section's
  // items container. Returns -1 if the position is not over any item area.
  int GetInsertIndexFromY(int y_in_section) const;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 private:
  // Returns the y coordinate (in section local coords) where the items
  // container starts (below the header).
  int GetItemsContainerY() const;

  raw_ptr<views::Label> header_label_;
  raw_ptr<views::View> items_container_;
  AstraSidebarSectionType section_type_;

  // Drop target state.
  raw_ptr<AstraSidebarSectionDropDelegate> drop_delegate_ = nullptr;
  raw_ptr<AstraSidebarDropIndicatorView> drop_indicator_ = nullptr;
  bool has_active_drag_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_SECTION_VIEW_H_
