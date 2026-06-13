#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "ui/views/view.h"

class Browser;
class TabStripModel;

namespace tab_groups {
class TabGroupId;
}  // namespace tab_groups

namespace astra {

class AstraTabGroupHeaderView;
class AstraTabGroupTabItemView;

// Sidebar section that projects Chromium's tab groups as a collapsible tree.
//
// Each tab group from TabStripModel's group model is shown as:
//   - A group header row (colored dot, name, tab count, chevron)
//   - A nested list of tab items (one per tab in the group)
//
// This is a presentation-only view:
//   - Truth source: Chromium's TabGroupModel (via TabStripModel)
//   - The sidebar reads group data and projects it into views
//   - Sidebar collapse state is presentation-only (independent of tab strip)
//   - All mutations (add tab to group, rename, close group) are dispatched
//     back through TabStripModel / TabGroupController APIs
//
// Observes TabStripModel for live updates to group and tab state.
//
// Chromium owner: TabGroupModel (chrome/browser/ui/tabs/tab_group_model.h),
//   TabGroup (chrome/browser/ui/tabs/tab_group.h)
// Chromium observer: TabStripModelObserver (chrome/browser/ui/tabs/tab_strip_model_observer.h)
//
// TODO(astra): Add proper TabGroupModelObserver if/when it becomes a
//   separate public observer interface. Currently we observe TabStripModel
//   and infer group changes from tab changes, which works because group
//   membership changes trigger tab move events in TabStripModel.
//   Chromium owner: TabGroupModelObserver (internal to tab_groups/)
class AstraSidebarTabGroupsView : public views::View,
                                  public TabStripModelObserver {
 public:
  explicit AstraSidebarTabGroupsView(Browser* browser);
  AstraSidebarTabGroupsView(const AstraSidebarTabGroupsView&) = delete;
  AstraSidebarTabGroupsView& operator=(const AstraSidebarTabGroupsView&) =
      delete;
  ~AstraSidebarTabGroupsView() override;

  // Rebuild all groups and tabs from the current TabStripModel state.
  // This is the full-rebuild fallback; incremental updates are preferred
  // when possible via TabStripModelObserver methods.
  void UpdateFromModel();

  // Set the section title (shown above the groups list).
  void SetTitle(const std::u16string& title);

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
  // Build the top-level layout (section header + groups container).
  void BuildLayout();

  // Populate the groups container from TabStripModel data.
  void PopulateGroups(TabStripModel* tab_strip);

  // Clear all group views from the container.
  void ClearGroups();

  // Create a group header view for the given group.
  std::unique_ptr<views::View> CreateGroupHeader(
      const tab_groups::TabGroupId& group_id,
      const TabGroup* group);

  // Create a tab item view for a tab within a group.
  std::unique_ptr<AstraTabGroupTabItemView> CreateGroupTabItem(
      int tab_index,
      const tab_groups::TabGroupId& group_id);

  // Toggle expanded state for a specific group in the sidebar.
  void ToggleGroupExpanded(const tab_groups::TabGroupId& group_id);

  // Create a new tab in the specified group.
  void NewTabInGroup(const tab_groups::TabGroupId& group_id);

  // Activate a tab at the given index.
  void ActivateTab(int tab_index);

  // Close a tab at the given index.
  void CloseTab(int tab_index);

  // Get the group color as a TabGroupColorId.
  tab_groups::TabGroupColorId GetGroupColor(
      const tab_groups::TabGroupId& group_id) const;

  // The browser whose tab strip we project. Not owned.
  raw_ptr<Browser> browser_;

  // Child views.
  raw_ptr<views::Label> section_title_ = nullptr;
  raw_ptr<views::View> groups_container_ = nullptr;

  // Sidebar-local expanded state per group. Maps group id to whether the
  // group is expanded (tabs visible) in the sidebar. This is independent
  // of the tab strip's own collapsed state — the sidebar shows all groups
  // in a tree structure with its own collapse/expand behavior.
  //
  // This is presentation state, not product state. It's okay to store this
  // on the view because it describes how the sidebar projects the groups,
  // not the groups themselves.
  base::flat_map<tab_groups::TabGroupId, bool> expanded_state_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_TAB_GROUPS_VIEW_H_
