#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

class Browser;
class TabStripModel;

namespace tab_groups {
class TabGroupId;
}  // namespace tab_groups

namespace views {
class Label;
class LabelButton;
}  // namespace views

namespace astra {

class AstraTabGroupHeaderView;
class AstraTabGroupTabItemView;

// Delegate interface for AstraSidebarTabGroupsView user actions.
//
// The delegate is implemented by the parent sidebar view or controller.
// All user-initiated mutations (rename, delete, reorder, etc.) flow
// through this delegate so the view itself remains a pure projection.
//
// Chromium owner: TabGroupController (chrome/browser/ui/tabs/tab_group_controller.h)
class AstraSidebarTabGroupsDelegate {
 public:
  virtual ~AstraSidebarTabGroupsDelegate() = default;

  // Called when a group header is clicked.
  virtual void OnGroupClicked(const std::string& group_id) = 0;

  // Called when a group's expanded state changes.
  virtual void OnGroupExpandedChanged(const std::string& group_id,
                                      bool expanded) = 0;

  // Called when a group's color is changed.
  virtual void OnGroupColorChanged(const std::string& group_id,
                                   SkColor color) = 0;

  // Called when a group is renamed.
  virtual void OnGroupRenamed(const std::string& group_id,
                              const std::u16string& new_name) = 0;

  // Called when a tab item is clicked (activate tab).
  virtual void OnTabClicked(const std::string& group_id, int tab_index) = 0;

  // Called when a tab item is middle-clicked (close tab).
  virtual void OnTabMiddleClicked(const std::string& group_id,
                                  int tab_index) = 0;

  // Called when a tab's close button is clicked.
  virtual void OnTabClosed(const std::string& group_id, int tab_index) = 0;

  // Called when the "new group" button is clicked.
  virtual void OnNewGroupRequested() = 0;

  // Called when a group delete is requested.
  virtual void OnDeleteGroupRequested(const std::string& group_id) = 0;

  // Called when groups are reordered via drag-and-drop.
  virtual void OnGroupReordered(int from_index, int to_index) = 0;

  // Called when a tab drag starts (for drag preview).
  virtual void OnTabDragged(const std::string& from_group_id,
                            int from_tab_index,
                            const gfx::Point& point) = 0;

  // Called when a tab is dropped onto a group or another tab.
  virtual void OnTabDropped(const std::string& to_group_id,
                            int to_tab_index,
                            const std::string& from_group_id,
                            int from_tab_index) = 0;

  // Called when "ungroup tab" is requested from context menu.
  virtual void OnUngroupTabRequested(const std::string& group_id,
                                     int tab_index) = 0;

  // Called when "close all tabs in group" is requested.
  virtual void OnCloseAllTabsInGroupRequested(const std::string& group_id) = 0;
};

// Sidebar section that projects tab groups as a collapsible tree.
//
// Each tab group is shown as:
//   - A group header row (colored dot, name, tab count, chevron)
//   - A nested list of tab items (one per tab in the group)
//
// This is primarily a presentation view. The data model is a vector of
// AstraTabGroupInfo structs that the parent populates. When integrated
// with Chromium, the data comes from TabStripModel's group model.
//
// Truth hierarchy:
//   - Chromium's TabGroupModel (via TabStripModel) — owns group truth.
//   - AstraSidebarTabGroupsView — projects group data into views,
//     holds sidebar-local presentation state (expanded, selection).
//
// Chromium owner: TabGroupModel (chrome/browser/ui/tabs/tab_group_model.h)
//   TabGroup (chrome/browser/ui/tabs/tab_group.h)
// Chromium observer: TabStripModelObserver
//   (chrome/browser/ui/tabs/tab_strip_model_observer.h)
//
// TODO(astra): Wire to TabStripModel and tab_groups for real data.
//   Currently the view can operate with AstraTabGroupInfo data only.
//   The Browser-based constructor provides TabStripModel integration.
class AstraSidebarTabGroupsView : public views::View,
                                  public TabStripModelObserver {
 public:
  // Default constructor for tests. Does not connect to TabStripModel.
  AstraSidebarTabGroupsView();

  // Construct with a Browser for TabStripModel integration.
  explicit AstraSidebarTabGroupsView(Browser* browser);

  AstraSidebarTabGroupsView(const AstraSidebarTabGroupsView&) = delete;
  AstraSidebarTabGroupsView& operator=(const AstraSidebarTabGroupsView&) =
      delete;
  ~AstraSidebarTabGroupsView() override;

  // -- Delegate ------------------------------------------------------------

  void set_delegate(AstraSidebarTabGroupsDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarTabGroupsDelegate* delegate() const { return delegate_; }

  // -- Group management ----------------------------------------------------

  // Set all groups at once (full rebuild).
  void SetTabGroups(const std::vector<AstraTabGroupInfo>& groups);

  // Get the number of groups.
  int GetGroupCount() const;

  // Get group info at the given index.
  AstraTabGroupInfo GetGroupAt(int index) const;

  // Add a new group at the end of the list.
  void AddGroup(const AstraTabGroupInfo& group);

  // Remove the group at the given index.
  void RemoveGroup(int index);

  // Update the group at the given index with new info.
  void UpdateGroup(int index, const AstraTabGroupInfo& group);

  // -- Selection -----------------------------------------------------------

  // Set the selected group by index. -1 clears selection.
  void SetSelectedGroup(int index);

  // Get the index of the selected group. Returns -1 if none selected.
  int GetSelectedGroupIndex() const;

  // Clear the current selection.
  void ClearSelection();

  // -- Expansion -----------------------------------------------------------

  // Set whether a specific group is expanded.
  void SetGroupExpanded(int index, bool expanded);

  // Check if a group is expanded.
  bool IsGroupExpanded(int index) const;

  // Toggle the expanded state of a group.
  void ToggleGroupExpanded(int index);

  // Expand all groups.
  void ExpandAllGroups();

  // Collapse all groups.
  void CollapseAllGroups();

  // Get the number of expanded groups.
  int GetExpandedGroupCount() const;

  // -- Reordering ----------------------------------------------------------

  // Move a group from one position to another.
  void MoveGroup(int from_index, int to_index);

  // -- Color ---------------------------------------------------------------

  // Set the color of a group.
  void SetGroupColor(int index, SkColor color);

  // Get the color of a group.
  SkColor GetGroupColor(int index) const;

  // -- Naming --------------------------------------------------------------

  // Rename a group.
  void RenameGroup(int index, const std::u16string& new_name);

  // Get the name of a group.
  std::u16string GetGroupName(int index) const;

  // -- Tab counts ----------------------------------------------------------

  // Get the number of tabs in a specific group.
  int GetTabCountInGroup(int index) const;

  // Get the total number of tabs across all groups.
  int GetTotalTabCount() const;

  // -- Display options -----------------------------------------------------

  // Set whether tab count badges are shown on group headers.
  void SetShowTabCount(bool show);
  bool GetShowTabCount() const;

  // Set whether the "add group" button is shown.
  void SetShowAddGroupButton(bool show);
  bool GetShowAddGroupButton() const;

  // -- Sorting -------------------------------------------------------------

  // Set the sort order for groups.
  void SetSortGroupsBy(AstraTabGroupSortBy sort_by);
  AstraTabGroupSortBy GetSortGroupsBy() const;

  // -- Group operations ----------------------------------------------------

  // Create a new group (delegate will handle actual creation).
  void NewGroup(const std::u16string& name, SkColor color);

  // Delete a group (delegate will handle actual deletion).
  void DeleteGroup(int index);

  // Close all tabs in a group.
  void CloseAllTabsInGroup(int index);

  // -- Tab operations between groups ---------------------------------------

  // Move a tab from one group to another.
  void MoveTabToGroup(int from_group, int from_tab, int to_group, int to_tab);

  // Remove a tab from its group (ungroup).
  void UngroupTab(int group_index, int tab_index);

  // -- Drag and drop -------------------------------------------------------

  // Set whether drag-and-drop is enabled.
  void SetDragDropEnabled(bool enabled);
  bool GetDragDropEnabled() const;

  // -- Color display -------------------------------------------------------

  // Set whether group color indicators are shown.
  void SetShowGroupColors(bool show);
  bool GetShowGroupColors() const;

  // -- Compact mode --------------------------------------------------------

  // Set whether compact mode is enabled (smaller rows, less padding).
  void SetCompactMode(bool compact);
  bool GetCompactMode() const;

  // -- View access ---------------------------------------------------------

  // Get the header view for a group by index.
  AstraTabGroupHeaderView* GetGroupHeaderViewAt(int index);

  // Get the tab item view at the given indices.
  AstraTabGroupTabItemView* GetGroupTabItemAt(int group_index, int tab_index);

  // Get the number of visible tab items in a group.
  int GetGroupTabCountAt(int group_index) const;

  // -- Section title -------------------------------------------------------

  // Set the section title (shown above the groups list).
  void SetTitle(const std::u16string& title);

  // -- TabStripModel integration -------------------------------------------

  // Rebuild all groups and tabs from the current TabStripModel state.
  // Only valid when constructed with a Browser.
  void UpdateFromModel();

  // -- TabStripModelObserver ----------------------------------------------

  void OnTabStripModelChanged(TabStripModel* tab_strip_model,
                              const TabStripModelChange& change,
                              const TabStripSelectionChange& selection) override;
  void OnTabGroupChanged(const TabGroupChange& change) override;
  void OnTabGroupVisualsChanged(
      TabStripModel* tab_strip_model,
      const tab_groups::TabGroupId& group) override;
  void OnTabInsertedAt(TabStripModel* tab_strip_model,
                       int index,
                       bool foreground) override;
  void OnTabRemovedAt(TabStripModel* tab_strip_model,
                      int index,
                      bool was_active) override;
  void OnTabMoved(TabStripModel* tab_strip_model,
                  int from_index,
                  int to_index) override;
  void OnActiveTabChanged(TabStripModel* tab_strip_model,
                          int old_index,
                          int new_index,
                          const TabStripSelectionChange& selection) override;
  void OnTabChanged(TabStripModel* tab_strip_model,
                    int index,
                    TabChangeType change_type) override;

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // A group entry in the internal model: info + views.
  struct GroupEntry {
    AstraTabGroupInfo info;
    raw_ptr<AstraTabGroupHeaderView> header = nullptr;
    std::vector<raw_ptr<AstraTabGroupTabItemView, VectorExperimental>> tabs;
    raw_ptr<views::View> tab_container = nullptr;
  };

  // Build the top-level layout (section header + groups container).
  void BuildLayout();

  // Rebuild all views from the internal groups_ list.
  void RebuildAllViews();

  // Rebuild just the tab items for a specific group.
  void RebuildGroupTabs(GroupEntry& entry);

  // Clear all group views from the container.
  void ClearGroups();

  // Create a group header view for the given group info.
  std::unique_ptr<AstraTabGroupHeaderView> CreateGroupHeader(
      const AstraTabGroupInfo& info);

  // Create a tab item view for a tab within a group.
  std::unique_ptr<AstraTabGroupTabItemView> CreateGroupTabItem(
      const AstraTabGroupTabInfo& info);

  // Find the index of a group by its ID. Returns -1 if not found.
  int FindGroupIndexById(const std::string& group_id) const;

  // Apply current sort order to the groups list.
  void ApplySortOrder();

  // Apply current display options to all views.
  void ApplyDisplayOptions();

  // -- Handlers for user actions (called from child views) -----------------

  void HandleGroupHeaderClicked(int group_index);
  void HandleTabClicked(int group_index, int tab_index);
  void HandleTabClosed(int group_index, int tab_index);
  void HandleAddGroupClicked();

  // -- TabStripModel helpers -----------------------------------------------

  // Populate the groups list from TabStripModel data.
  void PopulateGroupsFromModel(TabStripModel* tab_strip);

  // Convert a TabGroup to AstraTabGroupInfo.
  AstraTabGroupInfo TabGroupToInfo(const tab_groups::TabGroupId& group_id,
                                   const TabGroup* group) const;

  // The browser whose tab strip we project. Null for test-only instances.
  raw_ptr<Browser> browser_ = nullptr;

  // Action delegate. Not owned.
  raw_ptr<AstraSidebarTabGroupsDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::Label> section_title_ = nullptr;
  raw_ptr<views::View> groups_container_ = nullptr;
  raw_ptr<views::LabelButton> add_group_button_ = nullptr;

  // Internal group data model (projection state).
  // These are the groups currently displayed, in display order.
  std::vector<GroupEntry> groups_;

  // Selected group index. -1 means no selection.
  int selected_group_index_ = -1;

  // Current sort order.
  AstraTabGroupSortBy sort_by_ = AstraTabGroupSortBy::kManual;

  // Display options.
  bool show_tab_count_ = true;
  bool show_add_group_button_ = false;
  bool show_group_colors_ = true;
  bool drag_drop_enabled_ = true;
  bool compact_mode_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_
