#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
}  // namespace views

namespace astra {

struct AstraStackTabInfo;
class AstraSidebarStackTabItemView;

// Delegate interface for AstraSidebarStackChildView actions.
// Implemented by the parent stack view to handle tab actions within
// the child area of an expanded stack.
//
// The child view is presentation-only — it never mutates tab or stack state
// directly.  All user actions are forwarded to the delegate.
//
// Chromium owner: TabStrip (chrome/browser/ui/views/tabs/tab_strip.h)
class AstraSidebarStackChildDelegate {
 public:
  virtual ~AstraSidebarStackChildDelegate() = default;

  // Called when the user clicks a tab item.
  // |tab_index| is the index within the stack.
  virtual void OnStackTabClicked(int tab_index) = 0;

  // Called when the user middle-clicks a tab item.
  virtual void OnStackTabMiddleClicked(int tab_index) = 0;

  // Called when the user clicks the close button on a tab item.
  virtual void OnStackTabClosed(int tab_index) = 0;

  // Called when the user starts dragging a tab item.
  virtual void OnStackTabDragStarted(int tab_index,
                                     const gfx::Point& mouse_location) = 0;

  // Called when a tab is dropped onto this child view.
  virtual void OnStackTabDropped(int to_tab_index,
                                 const std::string& from_stack_id,
                                 int from_tab_index) = 0;
};

// =========================================================================
// AstraSidebarStackChildView — child content area of an expanded stack
// =========================================================================
//
// Container view that holds the tab items within an expanded named stack.
// This is the area that appears below a stack header when the stack is
// expanded.  It manages a list of AstraSidebarStackTabItemView instances.
//
// The child view handles:
//   - Displaying tab items in a vertical list
//   - Tab item add/remove/update operations
//   - Active tab tracking
//   - Tab reordering within the stack
//   - Drag-drop target state
//
// Truth source:
//   - Tab items: derived from tabs with this stack_id
//   - Active state: TabStripModel active tab
//   - Order: follows TabStripModel order within the stack
//
// Chromium owner: TabStrip (chrome/browser/ui/views/tabs/tab_strip.h)
// TODO(astra): Consider whether to use views::RecyclerView for large tab
//   counts to improve performance.
class AstraSidebarStackChildView : public views::View {
 public:
  AstraSidebarStackChildView();
  AstraSidebarStackChildView(const AstraSidebarStackChildView&) = delete;
  AstraSidebarStackChildView& operator=(const AstraSidebarStackChildView&) =
      delete;
  ~AstraSidebarStackChildView() override;

  // -- Tab data management ------------------------------------------------

  // Replace all tabs with the given list.  Rebuilds all tab item views.
  void SetTabs(const std::vector<AstraStackTabInfo>& tabs);

  // Returns the number of tabs in this stack.
  int GetTabCount() const;

  // Returns the tab info at the given index.  Returns a default-constructed
  // AstraStackTabInfo if the index is out of range.
  AstraStackTabInfo GetTabAt(int index) const;

  // Add a new tab at the given position.  If position is -1, appends to end.
  void AddTab(const AstraStackTabInfo& tab, int position = -1);

  // Remove the tab at the given index.
  void RemoveTab(int index);

  // Update the tab at the given index with new info.
  void UpdateTab(int index, const AstraStackTabInfo& tab);

  // -- Active tab ---------------------------------------------------------

  // Set the active tab by index.  -1 clears active state.
  void SetActiveTab(int index);

  // Returns the index of the active tab, or -1 if none.
  int GetActiveTabIndex() const;

  // -- Reordering ---------------------------------------------------------

  // Move a tab from one position to another within the stack.
  void MoveTab(int from_index, int to_index);

  // -- Display options ----------------------------------------------------

  // Set the height of individual tab items in pixels.
  void SetTabHeight(int height);
  int GetTabHeight() const { return tab_height_; }

  // Set whether favicons are shown on tab items.
  void SetShowFavicons(bool show);
  bool GetShowFavicons() const { return show_favicons_; }

  // Set whether close buttons are shown on tab items.
  void SetShowCloseButtons(bool show);
  bool GetShowCloseButtons() const { return show_close_buttons_; }

  // -- Drag and drop ------------------------------------------------------

  // Enable or disable drag-and-drop for tabs within this child view.
  void SetDragDropEnabled(bool enabled);
  bool GetDragDropEnabled() const { return drag_drop_enabled_; }

  // -- View access --------------------------------------------------------

  // Returns the tab item view at the given index.  Returns nullptr if
  // the index is out of range.
  AstraSidebarStackTabItemView* GetTabViewAt(int index);

  // Remove all tabs.
  void ClearAllTabs();

  // -- Stack ID -----------------------------------------------------------

  // Get/set the ID of the stack this child view belongs to.
  const std::string& stack_id() const { return stack_id_; }
  void set_stack_id(const std::string& stack_id) { stack_id_ = stack_id; }

  // -- Delegate -----------------------------------------------------------

  // Set the delegate for child view actions.  Not owned.
  void set_delegate(AstraSidebarStackChildDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarStackChildDelegate* delegate() const { return delegate_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Create a tab item view from tab info.
  std::unique_ptr<AstraSidebarStackTabItemView> CreateTabItemView(
      const AstraStackTabInfo& info);

  // Update all tab item display options (favicons, close buttons, etc.).
  void UpdateAllTabItemOptions();

  // Rebuild the tab list from tab_infos_.  Used after operations that
  // change the order or count of tabs.
  void RebuildTabViews();

  // Stack ID this child view belongs to.
  std::string stack_id_;

  // Cached tab data (presentation-model data).
  std::vector<AstraStackTabInfo> tab_infos_;

  // Index of the active tab, or -1 if none.
  int active_tab_index_ = -1;

  // Display options.
  int tab_height_ = 28;
  bool show_favicons_ = true;
  bool show_close_buttons_ = true;
  bool drag_drop_enabled_ = true;

  // Layout.
  raw_ptr<views::BoxLayout> layout_ = nullptr;

  // Delegate.
  raw_ptr<AstraSidebarStackChildDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_
