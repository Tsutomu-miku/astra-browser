#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

#include "astra/browser/astra_tab_stack_service.h"

class Browser;
class TabStripModel;

namespace astra {

class AstraSidebarStackHeaderView;
class AstraSidebarStackTabItemView;

// Sidebar section that projects Astra's named tab stacks as a collapsible
// tree of sections.
//
// Each named stack from AstraTabStackService is shown as:
//   - A stack header row (color accent bar, name, tab count, chevron, menu)
//   - A nested list of tab items (one per tab in the stack)
//
// This is a presentation-only view:
//   - Truth source: AstraTabStackService (stack metadata) +
//     AstraTabFeatures (per-tab stack membership) + TabStripModel (tabs)
//   - The sidebar reads stack data and projects it into views
//   - All mutations (add tab to stack, rename, delete stack) are dispatched
//     back through AstraTabStackService APIs
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
  // Build the top-level layout (section header + stacks container + new button).
  void BuildLayout();

  // Populate the stacks container from AstraTabStackService data.
  void PopulateStacks();

  // Clear all stack views from the container.
  void ClearStacks();

  // Create a stack header view for the given stack.
  std::unique_ptr<AstraSidebarStackHeaderView> CreateStackHeader(
      const AstraTabStack& stack);

  // Create a tab item view for a tab within a stack.
  std::unique_ptr<AstraSidebarStackTabItemView> CreateStackTabItem(
      content::WebContents* web_contents,
      const std::string& stack_id);

  // Handler for the "New stack" button.
  void OnNewStackButtonClicked();

  // Create a new stack with default name and color.
  // Returns the ID of the new stack.
  std::string CreateNewStack();

  // Activate the first tab in the given stack.
  void ActivateFirstTabInStack(const std::string& stack_id);

  // Get the AstraTabStackService for this browser's profile.
  AstraTabStackService* GetStackService();

  // The browser whose tab strip we project. Not owned.
  raw_ptr<Browser> browser_;

  // Observation of the tab stack service for stack state changes.
  base::ScopedObservation<AstraTabStackService,
                          AstraTabStackServiceObserver>
      stack_service_observation_{this};

  // Child views.
  raw_ptr<views::Label> section_title_ = nullptr;
  raw_ptr<views::View> stacks_container_ = nullptr;
  raw_ptr<views::LabelButton> new_stack_button_ = nullptr;

  // Map of stack ID to its header view.  Used for incremental updates.
  base::flat_map<std::string, raw_ptr<AstraSidebarStackHeaderView>>
      header_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_VIEW_H_
