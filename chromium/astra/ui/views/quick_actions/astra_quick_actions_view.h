// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_QUICK_ACTIONS_ASTRA_QUICK_ACTIONS_VIEW_H_
#define ASTRA_UI_VIEWS_QUICK_ACTIONS_ASTRA_QUICK_ACTIONS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
}  // namespace views

namespace astra {

// =========================================================================
// AstraQuickActionItemView — single action tile in the grid
// =========================================================================
//
// A square-ish tile with an icon on top and a label below. Clicking triggers
// the action callback.
//
// Layout:
//   +----------+
//   |    📋    |
//   |  Action  |
//   +----------+
// =========================================================================

class AstraQuickActionItemView : public views::View {
 public:
  using ActionCallback = base::RepeatingClosure;

  enum class ActionIcon {
    // Tab actions.
    kNewTab,
    kCloseTab,
    kPinTab,
    kMuteTab,
    kDuplicateTab,
    kSleepTab,

    // Navigation.
    kBack,
    kForward,
    kReload,
    kBookmark,

    // Astra features.
    kFocusMode,
    kWorkspace,
    kSidebar,
    kSplitView,
    kReadingList,

    // Tools.
    kFind,
    kPrint,
    kZoomIn,
    kZoomOut,
    kFullscreen,
    kDevTools,
  };

  struct ActionInfo {
    std::string action_id;
    std::u16string label;
    ActionIcon icon = ActionIcon::kNewTab;
  };

  AstraQuickActionItemView(const ActionInfo& info,
                           ActionCallback callback);
  ~AstraQuickActionItemView() override;

  AstraQuickActionItemView(const AstraQuickActionItemView&) = delete;
  AstraQuickActionItemView& operator=(const AstraQuickActionItemView&) = delete;

  const std::string& action_id() const { return action_id_; }

  void SetActive(bool active);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  void BuildLayout();
  void OnPaintBackground(gfx::Canvas* canvas);

  std::string action_id_;
  std::u16string label_;
  ActionIcon icon_;
  ActionCallback callback_;
  bool is_active_ = false;
  bool is_hovered_ = false;
  bool is_pressed_ = false;

  raw_ptr<views::View> icon_view_ = nullptr;
  raw_ptr<views::Label> label_view_ = nullptr;
};

// =========================================================================
// AstraQuickActionsView — quick actions control center
// =========================================================================
//
// A bubble showing a grid of quick action tiles for fast access to common
// browser and Astra features.
//
// Layout:
//   +-------------------------------------------+
//   |  Quick Actions                  [Close]  |
//   +-------------------------------------------+
//   |  Tabs                                     |
//   |  [📄 New] [🔇 Mute] [📌 Pin] [💤 Sleep] |
//   |  [🔄 Duplicate]           [✕ Close]      |
//   +-------------------------------------------+
//   |  Astra                                    |
//   |  [🎯 Focus] [📁 Workspace] [📚 Reading]  |
//   |  [⬅ Sidebar] [↔ Split]                   |
//   +-------------------------------------------+
//   |  Tools                                    |
//   |  [🔍 Find] [🖨 Print] [📺 Fullscreen]   |
//   |  [🔧 DevTools]  [➕ Zoom] [➖ Zoom]       |
//   +-------------------------------------------+
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - Custom Skia drawing for icons
// =========================================================================

class AstraQuickActionsView : public views::BubbleDialogDelegateView {
 public:
  using ActionTriggeredCallback =
      base::RepeatingCallback<void(const std::string& action_id)>;

  explicit AstraQuickActionsView(views::View* anchor_view);
  ~AstraQuickActionsView() override;

  AstraQuickActionsView(const AstraQuickActionsView&) = delete;
  AstraQuickActionsView& operator=(const AstraQuickActionsView&) = delete;

  void SetActionTriggeredCallback(ActionTriggeredCallback callback);

  void SetActionActive(const std::string& action_id, bool active);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  struct ActionSection {
    std::u16string title;
    std::vector<AstraQuickActionItemView::ActionInfo> actions;
  };

  void BuildUI();
  void BuildSection(const ActionSection& section);
  void OnActionTriggered(const std::string& action_id);

  // Sections.
  std::vector<ActionSection> sections_;

  // Callback.
  ActionTriggeredCallback action_callback_;

  // Action item views (owned by their parent views).
  std::vector<raw_ptr<AstraQuickActionItemView>> action_items_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_QUICK_ACTIONS_ASTRA_QUICK_ACTIONS_VIEW_H_
