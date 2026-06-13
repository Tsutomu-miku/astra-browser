#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

#include "astra/browser/astra_tab_stack_service.h"

class Browser;
class TabStripModel;

namespace astra {

class AstraSidebarStackHeaderView;
class AstraSidebarStackChildView;
class AstraSidebarStackTabItemView;

// =========================================================================
// AstraStackSortBy — sort order for tab stacks
// =========================================================================
//
// Determines how stacks are ordered in the sidebar.  kManual means the user
// controls the order explicitly (drag-and-drop); other values sort by the
// specified property.
//
// Chromium pattern: tab_groups::TabGroup follows similar sort semantics.
enum class AstraStackSortBy {
  kManual,       // User-controlled order (default)
  kName,         // Sort alphabetically by stack name
  kTabCount,     // Sort by number of tabs (descending)
  kLastAccessed, // Sort by most recently accessed (descending)
  kColor,        // Sort by color hue
};

// =========================================================================
// AstraStackInfo — metadata for a named tab stack
// =========================================================================
//
// Value type that carries all display-relevant metadata for a tab stack.
// Copies are cheap — views always receive copies so they cannot mutate
// model state directly.
//
// Truth source:
//   - Stack identity/name/color: AstraTabStackService
//   - Tab count: derived from tabs with this stack_id
//   - Expanded/collapsed: AstraTabStackService (persisted)
//   - Last accessed: derived from tab activity
//
// Chromium owner: tab_groups::TabGroup (chrome/browser/ui/tabs/tab_group.h)
struct AstraStackInfo {
  // Unique identifier for the stack.
  std::string stack_id;

  // Human-readable display name (UTF-16 for i18n).
  std::u16string name;

  // Accent color for the stack (used for left-edge color bar).
  SkColor color = SK_ColorGRAY;

  // Number of tabs in this stack.
  int tab_count = 0;

  // Whether the stack is expanded (tab items visible) or collapsed.
  bool is_expanded = true;

  // Whether the stack is pinned (shown at the top, not affected by sort).
  bool is_pinned = false;

  // Manual order index (only meaningful when sort_by == kManual).
  int order_index = 0;

  // Timestamp of last access (for kLastAccessed sort).
  base::Time last_accessed;

  // Timestamp of stack creation.
  base::Time created_time;

  // Whether any tab in the stack has unread state.
  bool has_unread = false;

  // Optional user note attached to the stack.
  std::u16string note;
};

// =========================================================================
// AstraSidebarStackDelegate — delegate interface for stack view actions
// =========================================================================
//
// Implemented by the parent sidebar controller or view to handle user
// actions on tab stacks (click, expand, rename, delete, drag-drop, etc.).
//
// The stack view is presentation-only — it never mutates stack state
// directly.  All user actions are forwarded to the delegate, which routes
// through AstraTabStackService and TabStripModel.
//
// Chromium owner: TabGroupController (chrome/browser/ui/tabs/tab_strip_model.h)
class AstraSidebarStackDelegate {
 public:
  virtual ~AstraSidebarStackDelegate() = default;

  // Called when the user clicks a stack header.
  virtual void OnStackClicked(const std::string& stack_id) = 0;

  // Called when a stack's expanded state changes via user interaction.
  virtual void OnStackExpandedChanged(const std::string& stack_id,
                                      bool expanded) = 0;

  // Called when a stack's color is changed by the user.
  virtual void OnStackColorChanged(const std::string& stack_id,
                                   SkColor color) = 0;

  // Called when a stack is renamed by the user.
  virtual void OnStackRenamed(const std::string& stack_id,
                              const std::u16string& new_name) = 0;

  // Called when the user clicks a tab item within a stack.
  virtual void OnTabClicked(const std::string& stack_id, int tab_index) = 0;

  // Called when the user middle-clicks a tab item within a stack.
  virtual void OnTabMiddleClicked(const std::string& stack_id,
                                  int tab_index) = 0;

  // Called when the user closes a tab item within a stack.
  virtual void OnTabClosed(const std::string& stack_id, int tab_index) = 0;

  // Called when the user requests creation of a new stack.
  virtual void OnNewStackRequested() = 0;

  // Called when the user requests deletion of a stack.
  virtual void OnDeleteStackRequested(const std::string& stack_id) = 0;

  // Called when the user reorders stacks via drag-and-drop.
  virtual void OnStackReordered(int from_index, int to_index) = 0;

  // Called when the user starts dragging a tab item.
  virtual void OnTabDragged(const std::string& from_stack_id,
                            int from_tab_index,
                            const gfx::Point& point) = 0;

  // Called when a tab is dropped onto a stack or tab position.
  virtual void OnTabDropped(const std::string& to_stack_id,
                            int to_tab_index,
                            const std::string& from_stack_id,
                            int from_tab_index) = 0;

  // Called when the user requests moving a tab to a different stack.
  virtual void OnMoveTabToStackRequested(const std::string& from_stack_id,
                                         int tab_index,
                                         const std::string& to_stack_id) = 0;
};

// =========================================================================
// AstraSidebarStackView — main tab stack sidebar section
// =========================================================================
//
// Sidebar section that projects Astra's named tab stacks as a collapsible
// list of sections.  Each stack has a header row and an expanded child
// area containing tab items.
//
// This is a presentation-only view:
//   - Truth source: AstraTabStackService (stack metadata) +
//     AstraTabFeatures (per-tab stack membership) + TabStripModel (tabs)
//   - The sidebar reads stack data and projects it into views
//   - All mutations dispatch back through the delegate / service APIs
//
// Observes AstraTabStackService for live updates to stack state and
// TabStripModel for tab state changes.
//
// Chromium owner: TabGroupModel (chrome/browser/ui/tabs/tab_group_model.h)
//   Sidebar projection of Chromium tab groups follows a similar pattern.
//
// TODO(astra): Consider whether to merge this with the hierarchical
//   stack view (parent/child stacking).  Named stacks and hierarchical
//   stacks serve different UX purposes but could share some UI code.
class AstraSidebarStackView : public views::View,
                              public TabStripModelObserver,
                              public AstraTabStackServiceObserver,
                              public AstraSidebarStackHeaderDelegate,
                              public AstraSidebarStackTabItemDelegate {
 public:
  explicit AstraSidebarStackView(Browser* browser);
  AstraSidebarStackView(const AstraSidebarStackView&) = delete;
  AstraSidebarStackView& operator=(const AstraSidebarStackView&) = delete;
  ~AstraSidebarStackView() override;

  // -- Stack data management ----------------------------------------------

  // Replace all stacks with the given list.  Rebuilds all views.
  void SetStacks(const std::vector<AstraStackInfo>& stacks);

  // Returns the number of stacks.
  int GetStackCount() const;

  // Returns the stack info at the given index.  Returns a default-constructed
  // AstraStackInfo if the index is out of range.
  AstraStackInfo GetStackAt(int index) const;

  // Add a new stack at the end of the list.
  void AddStack(const AstraStackInfo& stack);

  // Remove the stack at the given index.
  void RemoveStack(int index);

  // Update the stack at the given index with new info.
  void UpdateStack(int index, const AstraStackInfo& stack);

  // -- Selection ----------------------------------------------------------

  // Set the selected stack by index.  -1 clears selection.
  void SetSelectedStack(int index);

  // Returns the index of the selected stack, or -1 if none is selected.
  int GetSelectedStackIndex() const;

  // Clear the selection (no stack is selected).
  void ClearSelection();

  // -- Expansion ----------------------------------------------------------

  // Set the expanded state of a specific stack.
  void SetExpandedStack(int index, bool expanded);

  // Returns whether the stack at the given index is expanded.
  bool IsStackExpanded(int index) const;

  // Toggle the expanded state of a specific stack.
  void ToggleStackExpanded(int index);

  // Expand all stacks.
  void ExpandAllStacks();

  // Collapse all stacks.
  void CollapseAllStacks();

  // Returns the number of expanded stacks.
  int GetExpandedStackCount() const;

  // -- Reordering ---------------------------------------------------------

  // Move a stack from one position to another.
  void MoveStack(int from_index, int to_index);

  // -- Stack properties ---------------------------------------------------

  // Set the color of a stack.
  void SetStackColor(int index, SkColor color);

  // Get the color of a stack.
  SkColor GetStackColor(int index) const;

  // Rename a stack.
  void RenameStack(int index, const std::u16string& new_name);

  // Get the name of a stack.
  std::u16string GetStackName(int index) const;

  // Returns the number of tabs in the stack at the given index.
  int GetTabCountInStack(int index) const;

  // Returns the total number of tabs across all stacks.
  int GetTotalTabCount() const;

  // -- Display options ----------------------------------------------------

  // Set whether tab count badges are shown on stack headers.
  void SetShowStackCount(bool show);
  bool GetShowStackCount() const;

  // Set whether the "Add stack" button is visible.
  void SetShowAddStackButton(bool show);
  bool GetShowAddStackButton() const;

  // Set whether the "Collapse all" button is visible.
  void SetShowCollapseAllButton(bool show);
  bool GetShowCollapseAllButton() const;

  // -- Sorting ------------------------------------------------------------

  // Set the sort order for stacks.
  void SetSortStacksBy(AstraStackSortBy sort_by);
  AstraStackSortBy GetSortStacksBy() const;

  // -- Stack operations ---------------------------------------------------

  // Create a new stack with the given name and color.  Returns the index
  // of the new stack.
  int NewStack(const std::u16string& name, SkColor color);

  // Delete the stack at the given index.
  void DeleteStack(int index);

  // Close all tabs in the stack at the given index.
  void CloseAllTabsInStack(int index);

  // Move a tab from one stack/position to another stack/position.
  void MoveTabToStack(int from_stack, int from_tab, int to_stack, int to_tab);

  // -- Drag and drop ------------------------------------------------------

  // Enable or disable drag-and-drop for stacks and tabs.
  void SetDragDropEnabled(bool enabled);
  bool GetDragDropEnabled() const;

  // -- Visual options -----------------------------------------------------

  // Set whether stack color indicators are shown.
  void SetShowStackColors(bool show);
  bool GetShowStackColors() const;

  // Set compact mode (reduced height and padding).
  void SetCompactMode(bool compact);
  bool GetCompactMode() const;

  // Set the height of stack header rows in pixels.
  void SetStackHeight(int height_px);
  int GetStackHeight() const;

  // -- View accessors -----------------------------------------------------

  // Returns the header view for the section (section title + buttons).
  views::View* GetHeaderView();

  // Returns the stack header view at the given index.  Returns nullptr if
  // the index is out of range.
  AstraSidebarStackHeaderView* GetStackViewAt(int index);

  // Returns the tab item view at the given stack/tab index.  Returns
  // nullptr if either index is out of range.
  AstraSidebarStackTabItemView* GetStackTabItemAt(int stack_index,
                                                   int tab_index);

  // -- Delegate -----------------------------------------------------------

  // Set the delegate for stack view actions.  Not owned.
  void set_stack_delegate(AstraSidebarStackDelegate* delegate) {
    stack_delegate_ = delegate;
  }
  AstraSidebarStackDelegate* stack_delegate() const { return stack_delegate_; }

  // -- Model integration --------------------------------------------------

  // Rebuild all stacks and tabs from the current service and tab strip state.
  // This is the full-rebuild fallback; incremental updates are preferred
  // when possible via observer methods.
  void UpdateFromModel();

  // Set the section title (shown above the stacks list).
  void SetTitle(const std::u16string& title);

  // -- AstraTabStackServiceObserver ---------------------------------------

  void OnStackCreated(const AstraTabStack& stack) override;
  void OnStackDeleted(const AstraTabStackId& stack_id) override;
  void OnStackRenamed(const AstraTabStackId& stack_id,
                      const std::string& new_name) override;
  void OnStacksReordered() override;
  void OnTabAddedToStack(content::WebContents* web_contents,
                         const AstraTabStackId& stack_id) override;
  void OnTabRemovedFromStack(content::WebContents* web_contents,
                             const AstraTabStackId& stack_id) override;
  void OnStackCollapsed(const AstraTabStackId& stack_id) override;
  void OnStackExpanded(const AstraTabStackId& stack_id) override;

  // -- TabStripModelObserver ----------------------------------------------

  void OnTabStripModelChanged(TabStripModel* tab_strip_model,
                              const TabStripModelChange& change,
                              const TabStripSelectionChange& selection) override;
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

  // -- AstraSidebarStackHeaderDelegate ------------------------------------

  void OnStackToggleExpanded(const std::string& stack_id) override;
  void OnStackHeaderClicked(const std::string& stack_id) override;
  void OnStackMenuClicked(const std::string& stack_id,
                          const gfx::Point& anchor_point) override;

  // -- AstraSidebarStackTabItemDelegate -----------------------------------

  void OnStackTabClicked(content::WebContents* web_contents) override;
  void OnStackTabClosed(content::WebContents* web_contents) override;
  void OnStackTabDragStarted(content::WebContents* web_contents,
                             const gfx::Point& mouse_location) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the top-level layout (section header + stacks container + buttons).
  void BuildLayout();

  // Populate the stacks container from the current stacks_ data.
  void PopulateStacks();

  // Clear all stack views from the container.
  void ClearStackViews();

  // Create a stack header view for the given stack info.
  std::unique_ptr<AstraSidebarStackHeaderView> CreateStackHeader(
      const AstraStackInfo& info);

  // Create a child view (tab item container) for the given stack.
  std::unique_ptr<AstraSidebarStackChildView> CreateStackChildView(
      const AstraStackInfo& info);

  // Create a tab item view for a tab within a stack.
  std::unique_ptr<AstraSidebarStackTabItemView> CreateStackTabItem(
      content::WebContents* web_contents,
      const std::string& stack_id);

  // Handler for the "New stack" button.
  void OnNewStackButtonClicked();

  // Handler for the "Collapse all" button.
  void OnCollapseAllButtonClicked();

  // Create a new stack with default name and color.
  // Returns the ID of the new stack.
  std::string CreateNewStack();

  // Activate the first tab in the given stack.
  void ActivateFirstTabInStack(const std::string& stack_id);

  // Get the AstraTabStackService for this browser's profile.
  AstraTabStackService* GetStackService();

  // Find the index of a stack by its ID.  Returns -1 if not found.
  int FindStackIndexById(const std::string& stack_id) const;

  // Sort the stacks according to the current sort_by_ setting.
  void SortStacks();

  // Re-sort and refresh the view after a property change that affects sort.
  void RefreshSortOrder();

  // The browser whose tab strip we project.  Not owned.
  raw_ptr<Browser> browser_;

  // Observation of the tab stack service for stack state changes.
  base::ScopedObservation<AstraTabStackService,
                          AstraTabStackServiceObserver>
      stack_service_observation_{this};

  // Delegate for stack view actions.  Not owned.
  raw_ptr<AstraSidebarStackDelegate> stack_delegate_ = nullptr;

  // -- Stack data ---------------------------------------------------------

  // Current list of stacks (presentation-model data).
  std::vector<AstraStackInfo> stacks_;

  // Index of the selected stack, or -1 if none.
  int selected_stack_index_ = -1;

  // Sort order for stacks.
  AstraStackSortBy sort_by_ = AstraStackSortBy::kManual;

  // -- Display settings ---------------------------------------------------

  bool show_stack_count_ = true;
  bool show_add_stack_button_ = true;
  bool show_collapse_all_button_ = false;
  bool show_stack_colors_ = true;
  bool compact_mode_ = false;
  bool drag_drop_enabled_ = true;
  int stack_height_px_ = 32;

  // -- Child views --------------------------------------------------------

  raw_ptr<views::View> section_header_ = nullptr;
  raw_ptr<views::Label> section_title_ = nullptr;
  raw_ptr<views::LabelButton> new_stack_button_ = nullptr;
  raw_ptr<views::LabelButton> collapse_all_button_ = nullptr;
  raw_ptr<views::View> stacks_container_ = nullptr;

  // Map of stack ID to its header view.  Used for incremental updates.
  base::flat_map<std::string, raw_ptr<AstraSidebarStackHeaderView>>
      header_views_;

  // Map of stack ID to its child view (tab items container).
  base::flat_map<std::string, raw_ptr<AstraSidebarStackChildView>>
      child_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_
