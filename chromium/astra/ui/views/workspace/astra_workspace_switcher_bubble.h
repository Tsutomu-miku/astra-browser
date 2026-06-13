// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_SWITCHER_BUBBLE_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_SWITCHER_BUBBLE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

struct AstraWorkspace;
class AstraWorkspaceService;

// =========================================================================
// AstraWorkspaceSwitcherItemView — single workspace in switcher
// =========================================================================
//
// A single row in the workspace switcher list.  Shows workspace name,
// tab count, accent color indicator, and active/hover state.
//
// Layout:
//   +-------------------------------------------+
//   |  ●  Workspace Name              12 tabs  |
//   |     Description · 2 windows                |
//   +-------------------------------------------+
//
// States:
//   - Default: normal background
//   - Hover: highlighted background
//   - Selected (active): accent color highlight + checkmark
//   - First/last: rounded top/bottom corners
// =========================================================================

class AstraWorkspaceSwitcherItemView : public views::Button {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  AstraWorkspaceSwitcherItemView(const AstraWorkspace& workspace,
                                 SelectCallback callback);
  ~AstraWorkspaceSwitcherItemView() override;

  AstraWorkspaceSwitcherItemView(const AstraWorkspaceSwitcherItemView&) = delete;
  AstraWorkspaceSwitcherItemView& operator=(
      const AstraWorkspaceSwitcherItemView&) = delete;

  // -- State ---------------------------------------------------------------

  void SetIsActive(bool is_active);
  bool IsActive() const { return is_active_; }

  void SetIsHovered(bool is_hovered);

  const std::string& workspace_id() const { return workspace_id_; }
  const std::u16string& workspace_name() const { return workspace_name_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  void BuildLayout();
  void UpdateVisualState();

  std::string workspace_id_;
  std::u16string workspace_name_;
  std::u16string description_;
  std::string accent_color_;
  int tab_count_ = 0;
  int window_count_ = 1;
  bool is_active_ = false;
  bool is_hovered_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::View> accent_dot_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::Label> checkmark_label_ = nullptr;
};

// =========================================================================
// AstraWorkspaceSwitcherBubble — workspace switcher popup
// =========================================================================
//
// A popup bubble for quickly switching between workspaces.  Inspired by
// Arc browser's Cmd+` workspace switcher.
//
// Layout:
//   +-------------------------------------------+
//   |  🔍 Search workspaces...                  |
//   +-------------------------------------------+
//   |  ●  Workspace 1  (active)         8 tabs  |
//   |  ●  Workspace 2                   12 tabs |
//   |  ●  Workspace 3                   5 tabs  |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  + New workspace                          |
//   +-------------------------------------------+
//
// Features:
//   - Search / filter workspaces by name
//   - Keyboard navigation (up/down arrows, enter to select)
//   - Shows all workspaces with tab counts and accent colors
//   - "New workspace" action at the bottom
//   - Current workspace is highlighted
//
// This is a presentation-only view.  Workspace data comes from
// AstraWorkspaceService.  Selection is reported via callback.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::Textfield (search input)
//   - views::ScrollView
// =========================================================================

class AstraWorkspaceSwitcherBubble
    : public views::BubbleDialogDelegateView {
 public:
  // Callback when a workspace is selected.
  using WorkspaceSelectedCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  // Callback when "new workspace" is requested.
  using NewWorkspaceCallback = base::RepeatingClosure;

  AstraWorkspaceSwitcherBubble(views::View* anchor_view,
                               AstraWorkspaceService* service,
                               WorkspaceSelectedCallback selected_callback,
                               NewWorkspaceCallback new_workspace_callback);
  ~AstraWorkspaceSwitcherBubble() override;

  AstraWorkspaceSwitcherBubble(const AstraWorkspaceSwitcherBubble&) = delete;
  AstraWorkspaceSwitcherBubble& operator=(
      const AstraWorkspaceSwitcherBubble&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetWorkspaces(const std::vector<AstraWorkspace>& workspaces);
  void SetActiveWorkspaceId(const std::string& workspace_id);

  // -- Search --------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

 private:
  void BuildUI();
  void BuildSearchField();
  void BuildWorkspaceList();
  void BuildNewWorkspaceButton();

  void RefreshWorkspaceList();
  void FilterWorkspaces();

  void OnWorkspaceSelected(const std::string& workspace_id);
  void OnNewWorkspaceClicked();
  void OnSearchTextChanged();

  // Navigate the list with keyboard.
  void MoveSelection(int delta);
  void ActivateSelected();

  raw_ptr<AstraWorkspaceService> service_ = nullptr;

  WorkspaceSelectedCallback selected_callback_;
  NewWorkspaceCallback new_workspace_callback_;

  // Full workspace list (unfiltered).
  std::vector<AstraWorkspace> all_workspaces_;
  std::string active_workspace_id_;

  // Currently hovered/selected index in the filtered list.
  int selected_index_ = -1;

  // Child views.
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> workspace_list_ = nullptr;
  raw_ptr<views::MdTextButton> new_workspace_button_ = nullptr;

  // Workspace items in current filtered list (owned by workspace_list_).
  std::vector<raw_ptr<AstraWorkspaceSwitcherItemView>> workspace_items_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_SWITCHER_BUBBLE_H_
