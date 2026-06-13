// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_ACTIONS_ASTRA_TAB_MULTISELECT_BAR_H_
#define ASTRA_UI_VIEWS_TAB_ACTIONS_ASTRA_TAB_MULTISELECT_BAR_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabMultiSelectBar — batch actions for multi-tab selection
// =========================================================================
//
// A bottom bar that appears when multiple tabs are selected, providing
// batch actions: pin, close, move to workspace, group, bookmark, etc.
//
// Layout:
//   +------------------------------------------------------+
//   |  5 tabs selected     [Pin] [Group] [Bookmark] [X |
//   +------------------------------------------------------+
//
// This is a presentation-only view.  Action callbacks are invoked when
// buttons are clicked; the caller handles actual tab manipulation.
//
// Chromium subsystems reused:
//   - views::MdTextButton (action buttons)
//   - views::View (base class)
// =========================================================================

class AstraTabMultiSelectBar : public views::View {
 public:
  // Callback types for actions.
  using ActionCallback = base::RepeatingClosure;
  using MoveToWorkspaceCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  AstraTabMultiSelectBar();
  ~AstraTabMultiSelectBar() override;

  AstraTabMultiSelectBar(const AstraTabMultiSelectBar&) = delete;
  AstraTabMultiSelectBar& operator=(
      const AstraTabMultiSelectBar&) = delete;

  // -- Selection state ----------------------------------------------------

  void SetSelectedTabCount(int count);
  int selected_tab_count() const { return selected_count_; }

  void SetVisible(bool visible) override;

  // -- Action callbacks ---------------------------------------------------

  void SetCloseCallback(ActionCallback callback);
  void SetPinCallback(ActionCallback callback);
  void SetGroupCallback(ActionCallback callback);
  void SetBookmarkCallback(ActionCallback callback);
  void SetMoveToWorkspaceCallback(MoveToWorkspaceCallback callback);
  void SetDeselectAllCallback(ActionCallback callback);

  // -- Button visibility --------------------------------------------------

  void SetPinButtonVisible(bool visible);
  void SetGroupButtonVisible(bool visible);
  void SetBookmarkButtonVisible(bool visible);
  void SetMoveButtonVisible(bool visible);

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  void BuildLayout();
  void UpdateCountLabel();

  int selected_count_ = 0;

  // Callbacks.
  ActionCallback close_callback_;
  ActionCallback pin_callback_;
  ActionCallback group_callback_;
  ActionCallback bookmark_callback_;
  MoveToWorkspaceCallback move_to_workspace_callback_;
  ActionCallback deselect_all_callback_;

  // Child views.
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::MdTextButton> pin_button_ = nullptr;
  raw_ptr<views::MdTextButton> group_button_ = nullptr;
  raw_ptr<views::MdTextButton> bookmark_button_ = nullptr;
  raw_ptr<views::MdTextButton> move_button_ = nullptr;
  raw_ptr<views::MdTextButton> close_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_ACTIONS_ASTRA_TAB_MULTISELECT_BAR_H_
