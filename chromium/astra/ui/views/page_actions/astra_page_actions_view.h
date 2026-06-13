// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_VIEW_H_
#define ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/menu/menu_delegate.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#include "astra/ui/views/page_actions/astra_page_actions_model.h"

namespace views {
class ImageButton;
class Label;
class BubbleDialogDelegate;
}  // namespace views

namespace astra {

// Delegate for AstraPageActionView events.
class AstraPageActionDelegate {
 public:
  virtual ~AstraPageActionDelegate() = default;

  // Called when a page action is clicked (primary action).
  virtual void OnPageActionClicked(AstraPageActionType type) = 0;

  // Called when a page action is right-clicked (context menu).
  virtual void OnPageActionContextMenu(AstraPageActionType type,
                                       const gfx::Point& point) = 0;

  // Called when an extension page action is clicked.
  virtual void OnExtensionActionClicked(const std::string& extension_id) {}

  // Called when the overflow button is clicked.
  // Returns true if the delegate handled showing the overflow menu.
  virtual bool OnOverflowButtonClicked(views::View* anchor_view) {
    return false;
  }

  // Called when a pin action is toggled from a context menu.
  virtual void OnPageActionPinToggled(AstraPageActionType type, bool pinned) {}
};

// Individual page action icon button.
//
// Shows an icon with optional badge text and an active/attention state.
// This is a pure presentation view — it delegates all actions to its
// delegate and does not own any state.
//
// Similar to Chromium's PageActionIconView.
//
// Chromium owner: PageActionIconView
//   (chrome/browser/ui/page_action/page_action_icon_view.h)
class AstraPageActionView : public views::ImageButton {
 public:
  METADATA_HEADER(AstraPageActionView);

  explicit AstraPageActionView(AstraPageActionType type);
  AstraPageActionView(const AstraPageActionView&) = delete;
  AstraPageActionView& operator=(const AstraPageActionView&) = delete;
  ~AstraPageActionView() override;

  // -- Action identity ------------------------------------------------------

  AstraPageActionType GetActionType() const;

  // For extension actions, returns the extension ID.
  const std::string& GetExtensionId() const;
  void SetExtensionId(const std::string& extension_id);

  // -- Icon ---------------------------------------------------------------

  void SetIconImage(const gfx::ImageSkia& icon);

  // -- Label / tooltip ------------------------------------------------------

  void SetLabel(const std::u16string& label);
  const std::u16string& GetLabel() const;

  // -- Badge --------------------------------------------------------------

  void SetBadgeText(const std::u16string& text);
  const std::u16string& GetBadgeText() const;
  void SetBadgeColor(SkColor color);
  SkColor GetBadgeColor() const;
  bool HasBadge() const;

  // -- State --------------------------------------------------------------

  void SetActionState(AstraPageActionState state);
  AstraPageActionState GetActionState() const;

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraPageActionDelegate* delegate);

  // -- Icon size -----------------------------------------------------------

  void SetIconSize(int size_px);
  int GetIconSize() const;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Handle the primary action (left click / tap).
  void HandlePrimaryAction(const ui::Event& event);

  // Show the context menu at the given screen position.
  void ShowContextMenu(const gfx::Point& screen_point);

  // Update visual appearance based on current state.
  void UpdateVisuals();

  // The action type this view represents.
  AstraPageActionType type_;

  // For extension actions: the extension ID.
  std::string extension_id_;

  // Display label (used for tooltip and accessibility).
  std::u16string label_;

  // Badge text (empty = no badge).
  std::u16string badge_text_;

  // Badge background color.
  SkColor badge_color_ = SK_ColorRED;

  // Current action state.
  AstraPageActionState state_ = AstraPageActionState::kDefault;

  // Delegate for action handling (not owned).
  raw_ptr<AstraPageActionDelegate> delegate_ = nullptr;

  // Icon size in pixels.
  int icon_size_ = 20;

  // Default size of the click target.
  static constexpr int kDefaultSize = 28;
};

// Container view for page action icons.
//
// Arranges page action icons horizontally in the toolbar/location bar
// area.  Supports:
//   - Pinned actions (always visible in the main row)
//   - Overflow button (shows remaining actions in a menu)
//   - Observes AstraPageActionsModel for state changes
//
// Chromium owner: PageActionIconContainer / PageActionIconController
//   (chrome/browser/ui/page_action/page_action_icon_container.h)
//
// TODO(astra): Integrate with LocationBarView via a patch to
// chrome/browser/ui/views/location_bar/location_bar_view.cc to add
// the Astra page actions container to the trailing decoration row.
class AstraPageActionsView : public views::View,
                             public AstraPageActionsObserver,
                             public AstraPageActionDelegate {
 public:
  METADATA_HEADER(AstraPageActionsView);

  AstraPageActionsView();
  explicit AstraPageActionsView(AstraPageActionsModel* model);
  AstraPageActionsView(const AstraPageActionsView&) = delete;
  AstraPageActionsView& operator=(const AstraPageActionsView&) = delete;
  ~AstraPageActionsView() override;

  // -- Model binding --------------------------------------------------------

  void SetModel(AstraPageActionsModel* model);
  AstraPageActionsModel* GetModel() const { return model_; }

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraPageActionDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraPageActionDelegate* GetDelegate() const { return delegate_; }

  // -- Action view access ---------------------------------------------------

  // Get the view for a specific action type, or nullptr if not found.
  AstraPageActionView* GetActionView(AstraPageActionType type);

  // Get the overflow button.
  views::ImageButton* overflow_button() { return overflow_button_; }

  // Returns the number of pinned action views currently shown.
  size_t GetPinnedActionCount() const;

  // Returns the total number of action views (including those in overflow).
  size_t GetTotalActionCount() const;

  // -- Icon size -----------------------------------------------------------

  void SetIconSize(int size_px);
  int GetIconSize() const { return icon_size_px_; }

  // -- Spacing -------------------------------------------------------------

  void SetSpacing(int spacing_px);
  int GetSpacing() const { return spacing_px_; }

  // -- Overflow menu --------------------------------------------------------

  // Show the overflow menu anchored to the overflow button.
  void ShowOverflowMenu();

  // Returns whether the overflow menu is currently showing.
  bool IsOverflowMenuShowing() const;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

  // -- AstraPageActionsObserver: -------------------------------------------

  void OnActionsChanged(AstraPageActionsModel* model) override;
  void OnActionChanged(AstraPageActionsModel* model,
                       AstraPageActionType type) override;
  void OnPageActionsModelShutdown(AstraPageActionsModel* model) override;

  // -- AstraPageActionDelegate: --------------------------------------------

  void OnPageActionClicked(AstraPageActionType type) override;
  void OnPageActionContextMenu(AstraPageActionType type,
                               const gfx::Point& point) override;
  void OnExtensionActionClicked(const std::string& extension_id) override;
  bool OnOverflowButtonClicked(views::View* anchor_view) override;

 private:
  // Rebuild all action views from the model.
  void RebuildActionViews();

  // Update all action views from the current model state.
  void UpdateActionViews();

  // Update a single action view from the model.
  void UpdateActionView(AstraPageActionView* view,
                        const AstraPageActionItem& item);

  // Update visibility of the overflow button based on model state.
  void UpdateOverflowButtonVisibility();

  // Called when the overflow button is pressed.
  void OnOverflowButtonPressed();

  // Build and run the overflow menu.
  void RunOverflowMenu();

  // Get default icon for a given action type (fallback if no icon set).
  const char* GetDefaultIconName(AstraPageActionType type) const;

  // Model for state (not owned).
  raw_ptr<AstraPageActionsModel> model_ = nullptr;

  // Delegate for action forwarding (not owned).
  raw_ptr<AstraPageActionDelegate> delegate_ = nullptr;

  // Observation of the model.
  base::ScopedObservation<AstraPageActionsModel, AstraPageActionsObserver>
      model_observation_{this};

  // Action views (owned by the view hierarchy).
  std::vector<raw_ptr<AstraPageActionView>> action_views_;

  // Overflow button (owned by the view hierarchy).
  raw_ptr<views::ImageButton> overflow_button_ = nullptr;

  // Menu runner for the overflow menu.
  std::unique_ptr<views::MenuRunner> overflow_menu_runner_;

  // Icon size in pixels.
  int icon_size_px_ = 20;

  // Spacing between action icons in pixels.
  int spacing_px_ = 2;

  // Default action button size.
  static constexpr int kActionButtonSize = 28;

  // Overflow button size.
  static constexpr int kOverflowButtonSize = 28;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_VIEW_H_
