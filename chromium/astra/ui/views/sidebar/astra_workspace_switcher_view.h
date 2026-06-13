#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_WORKSPACE_SWITCHER_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_WORKSPACE_SWITCHER_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace views {
class Label;
class ImageButton;
}  // namespace views

namespace astra {

class AstraWorkspaceService;

// The top workspace switcher area in the sidebar.
// Shows the current active workspace name and a chevron to expand/collapse
// the workspace list.
//
// TODO(astra): Implement the workspace dropdown / sheet. For now this is
// a static header showing the active workspace name with a click handler
// stub.
//
// This view reads from AstraWorkspaceService but does not own workspace data.
// Workspace state lives in the profile-keyed service.
class AstraWorkspaceSwitcherView : public views::View {
 public:
  explicit AstraWorkspaceSwitcherView(AstraWorkspaceService* workspace_service);
  AstraWorkspaceSwitcherView(const AstraWorkspaceSwitcherView&) = delete;
  AstraWorkspaceSwitcherView& operator=(const AstraWorkspaceSwitcherView&) = delete;
  ~AstraWorkspaceSwitcherView() override;

  // Refresh displayed workspace name from the service.
  void UpdateFromService();

  // Called when a workspace is clicked. Dispatches to the service via
  // command delegate — does not mutate state directly.
  // TODO(astra): Wire to actual workspace selection command.
  void OnWorkspaceClicked(const std::string& workspace_id);

  // -- Workspace metadata (called by parent / controller) ---------------

  // Sets the number of tabs across all windows in the active workspace.
  // Shown as a subtitle under the workspace name.
  void SetTabCount(int tab_count);

  // Sets the number of windows in the active workspace.
  // Shown alongside tab count (e.g. "2 windows · 8 tabs").
  void SetWindowCount(int window_count);

  // Sets whether this switcher represents the current window's workspace.
  // When true, shows a visual indicator (e.g. a checkmark or highlighted
  // accent bar).
  void SetIsCurrentWindowWorkspace(bool is_current);

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  void OnSwitcherPressed();

  // Updates the count label text from current tab_count_ and window_count_.
  void UpdateCountLabel();

  raw_ptr<AstraWorkspaceService> workspace_service_;
  raw_ptr<views::Label> workspace_name_label_;
  raw_ptr<views::Label> workspace_count_label_;
  raw_ptr<views::ImageButton> chevron_button_;
  bool expanded_ = false;
  int tab_count_ = 0;
  int window_count_ = 0;
  bool is_current_window_workspace_ = true;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_WORKSPACE_SWITCHER_VIEW_H_
