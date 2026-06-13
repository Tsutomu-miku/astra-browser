// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_actions/astra_page_actions_view.h"

#include <algorithm>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/ext/image_operations.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/accessibility/view_ax_platform_node_delegate.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/menu/menu_config.h"
#include "ui/views/controls/menu/menu_delegate.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/menu/submenu_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/metadata_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Helper to draw a badge on a canvas.
void DrawBadge(gfx::Canvas* canvas,
               const gfx::Rect& bounds,
               const std::u16string& text,
               SkColor bg_color,
               SkColor text_color) {
  if (text.empty()) {
    return;
  }

  // Badge is drawn in the top-right corner of the icon bounds.
  const int kBadgeFontSize = 10;
  const int kBadgePaddingH = 3;
  const int kBadgePaddingV = 1;
  const int kBadgeRadius = 6;

  // Estimate text width.
  int text_width = static_cast<int>(text.length()) * kBadgeFontSize / 2;
  int badge_width = text_width + kBadgePaddingH * 2;
  int badge_height = kBadgeFontSize + kBadgePaddingV * 2;

  // Position at top-right, slightly overlapping the icon.
  int badge_x = bounds.right() - badge_width / 2;
  int badge_y = bounds.y() - badge_height / 2;

  // Clamp to view bounds.
  badge_x = std::max(bounds.x(), badge_x);
  badge_y = std::max(bounds.y(), badge_y);

  gfx::Rect badge_bounds(badge_x, badge_y, badge_width, badge_height);

  // Draw badge background.
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(bg_color);
  canvas->DrawRoundRect(badge_bounds, kBadgeRadius, flags);

  // Draw badge text.
  canvas->DrawStringRect(
      text, gfx::FontList(), text_color,
      gfx::Rect(badge_bounds.x(), badge_bounds.y(), badge_bounds.width(),
                badge_bounds.height()),
      gfx::HorizontalAlignment::ALIGN_CENTER,
      gfx::VerticalAlignment::ALIGN_MIDDLE);
}

}  // namespace

// ===========================================================================
// AstraPageActionView
// ===========================================================================

AstraPageActionView::AstraPageActionView(AstraPageActionType type)
    : type_(type) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetSize(gfx::Size(kDefaultSize, kDefaultSize));
  SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  SetMinSize(gfx::Size(kDefaultSize, kDefaultSize));
  UpdateVisuals();
}

AstraPageActionView::~AstraPageActionView() = default;

AstraPageActionType AstraPageActionView::GetActionType() const {
  return type_;
}

const std::string& AstraPageActionView::GetExtensionId() const {
  return extension_id_;
}

void AstraPageActionView::SetExtensionId(const std::string& extension_id) {
  extension_id_ = extension_id;
}

void AstraPageActionView::SetIconImage(const gfx::ImageSkia& icon) {
  // Resize icon to fit the icon size.
  if (icon.width() != icon_size_ || icon.height() != icon_size_) {
    gfx::ImageSkia resized =
        gfx::ImageSkiaOperations::CreateResizedImage(
            icon, skia::ImageOperations::RESIZE_BEST,
            gfx::Size(icon_size_, icon_size_));
    SetImage(views::Button::STATE_NORMAL, resized);
  } else {
    SetImage(views::Button::STATE_NORMAL, icon);
  }
}

void AstraPageActionView::SetLabel(const std::u16string& label) {
  if (label_ == label) {
    return;
  }
  label_ = label;
  SetTooltipText(label_);
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

const std::u16string& AstraPageActionView::GetLabel() const {
  return label_;
}

void AstraPageActionView::SetBadgeText(const std::u16string& text) {
  if (badge_text_ == text) {
    return;
  }
  badge_text_ = text;
  SchedulePaint();
}

const std::u16string& AstraPageActionView::GetBadgeText() const {
  return badge_text_;
}

void AstraPageActionView::SetBadgeColor(SkColor color) {
  if (badge_color_ == color) {
    return;
  }
  badge_color_ = color;
  SchedulePaint();
}

SkColor AstraPageActionView::GetBadgeColor() const {
  return badge_color_;
}

bool AstraPageActionView::HasBadge() const {
  return !badge_text_.empty();
}

void AstraPageActionView::SetActionState(AstraPageActionState state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  UpdateVisuals();
}

AstraPageActionState AstraPageActionView::GetActionState() const {
  return state_;
}

void AstraPageActionView::SetDelegate(AstraPageActionDelegate* delegate) {
  delegate_ = delegate;
}

void AstraPageActionView::SetIconSize(int size_px) {
  if (icon_size_ == size_px) {
    return;
  }
  icon_size_ = size_px;
  SchedulePaint();
}

int AstraPageActionView::GetIconSize() const {
  return icon_size_;
}

gfx::Size AstraPageActionView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kDefaultSize, kDefaultSize);
}

void AstraPageActionView::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  UpdateVisuals();
}

void AstraPageActionView::OnPaint(gfx::Canvas* canvas) {
  views::ImageButton::OnPaint(canvas);

  // Draw badge on top if we have one.
  if (!badge_text_.empty()) {
    gfx::Rect content_bounds = GetContentsBounds();
    // Center the icon within the button bounds.
    int icon_x = content_bounds.x() + (content_bounds.width() - icon_size_) / 2;
    int icon_y = content_bounds.y() + (content_bounds.height() - icon_size_) / 2;
    gfx::Rect icon_bounds(icon_x, icon_y, icon_size_, icon_size_);

    SkColor text_color = SK_ColorWHITE;
    DrawBadge(canvas, icon_bounds, badge_text_, badge_color_, text_color);
  }
}

std::u16string AstraPageActionView::GetTooltipText(
    const gfx::Point& p) const {
  return label_;
}

void AstraPageActionView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::ImageButton::GetAccessibleNodeData(node_data);
  if (!label_.empty()) {
    node_data->SetName(label_);
  }
  node_data->role = ax::mojom::Role::kButton;
}

bool AstraPageActionView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsRightMouseButton()) {
    return true;  // We'll handle it in OnMouseReleased.
  }
  return views::ImageButton::OnMousePressed(event);
}

void AstraPageActionView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsRightMouseButton() && HitTestPoint(event.location())) {
    gfx::Point screen_point = event.location();
    ConvertPointToScreen(this, &screen_point);
    ShowContextMenu(screen_point);
    return;
  }
  views::ImageButton::OnMouseReleased(event);
}

void AstraPageActionView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_LONG_PRESS) {
    gfx::Point screen_point = event->location();
    ConvertPointToScreen(this, &screen_point);
    ShowContextMenu(screen_point);
    event->SetHandled();
    return;
  }
  if (event->type() == ui::ET_GESTURE_TAP) {
    HandlePrimaryAction(*event);
    event->SetHandled();
    return;
  }
  views::ImageButton::OnGestureEvent(event);
}

void AstraPageActionView::HandlePrimaryAction(const ui::Event& event) {
  if (!delegate_) {
    return;
  }
  if (type_ == AstraPageActionType::kExtensionAction) {
    delegate_->OnExtensionActionClicked(extension_id_);
  } else {
    delegate_->OnPageActionClicked(type_);
  }
}

void AstraPageActionView::ShowContextMenu(const gfx::Point& screen_point) {
  if (!delegate_) {
    return;
  }
  if (type_ == AstraPageActionType::kExtensionAction) {
    // Extension context menu handled by the delegate.
    delegate_->OnPageActionContextMenu(type_, screen_point);
  } else {
    delegate_->OnPageActionContextMenu(type_, screen_point);
  }
}

void AstraPageActionView::UpdateVisuals() {
  // Adjust opacity based on state.
  float opacity = 1.0f;
  switch (state_) {
    case AstraPageActionState::kDefault:
      opacity = 1.0f;
      break;
    case AstraPageActionState::kActive:
      opacity = 1.0f;
      break;
    case AstraPageActionState::kDisabled:
      opacity = 0.4f;
      break;
    case AstraPageActionState::kAttention:
      opacity = 1.0f;
      break;
    case AstraPageActionState::kError:
      opacity = 1.0f;
      break;
  }

  SetEnabled(state_ != AstraPageActionState::kDisabled);

  SchedulePaint();
}

// ===========================================================================
// AstraPageActionsView
// ===========================================================================

AstraPageActionsView::AstraPageActionsView() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), spacing_px_,
      views::BoxLayout::MainAxisAlignment::kCenter,
      views::BoxLayout::CrossAxisAlignment::kCenter));

  // Create overflow button.
  overflow_button_ = AddChildView(std::make_unique<views::ImageButton>());
  overflow_button_->SetTooltipText(u"More actions");
  overflow_button_->SetAccessibleName(u"More page actions");
  overflow_button_->SetMinSize(gfx::Size(kOverflowButtonSize, kOverflowButtonSize));
  overflow_button_->SetVisible(false);  // Hidden until we have overflow items.
  overflow_button_->SetCallback(base::BindRepeating(
      &AstraPageActionsView::OnOverflowButtonPressed, base::Unretained(this)));
}

AstraPageActionsView::AstraPageActionsView(AstraPageActionsModel* model)
    : AstraPageActionsView() {
  SetModel(model);
}

AstraPageActionsView::~AstraPageActionsView() = default;

void AstraPageActionsView::SetModel(AstraPageActionsModel* model) {
  if (model_ == model) {
    return;
  }

  model_observation_.Reset();
  model_ = model;

  if (model_) {
    model_observation_.Observe(model_);
    RebuildActionViews();
  } else {
    // Clear all action views.
    action_views_.clear();
    // Remove all children except overflow button.
    std::vector<raw_ptr<views::View>> to_remove;
    for (views::View* child : children()) {
      if (child != overflow_button_) {
        to_remove.push_back(child);
      }
    }
    for (auto* child : to_remove) {
      RemoveChildViewT(child);
    }
  }

  UpdateOverflowButtonVisibility();
  InvalidateLayout();
}

AstraPageActionView* AstraPageActionsView::GetActionView(
    AstraPageActionType type) {
  for (auto* view : action_views_) {
    if (view->GetActionType() == type) {
      return view;
    }
  }
  return nullptr;
}

size_t AstraPageActionsView::GetPinnedActionCount() const {
  size_t count = 0;
  for (auto* view : action_views_) {
    if (view->GetVisible()) {
      count++;
    }
  }
  return count;
}

size_t AstraPageActionsView::GetTotalActionCount() const {
  return action_views_.size();
}

void AstraPageActionsView::SetIconSize(int size_px) {
  if (icon_size_px_ == size_px) {
    return;
  }
  icon_size_px_ = size_px;
  for (auto* view : action_views_) {
    view->SetIconSize(size_px);
  }
  InvalidateLayout();
}

void AstraPageActionsView::SetSpacing(int spacing_px) {
  if (spacing_px_ == spacing_px) {
    return;
  }
  spacing_px_ = spacing_px;
  auto* layout = static_cast<views::BoxLayout*>(GetLayoutManager());
  if (layout) {
    layout->set_between_child_spacing(spacing_px_);
  }
  InvalidateLayout();
}

void AstraPageActionsView::ShowOverflowMenu() {
  if (IsOverflowMenuShowing()) {
    return;
  }
  RunOverflowMenu();
}

bool AstraPageActionsView::IsOverflowMenuShowing() const {
  return overflow_menu_runner_ && overflow_menu_runner_->IsRunning();
}

gfx::Size AstraPageActionsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraPageActionsView::Layout() {
  views::View::Layout();
}

void AstraPageActionsView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // Update overflow button icon.
  if (overflow_button_) {
    // Use a simple dots icon from the theme (stub).
    // TODO(astra): Use proper vector icon from Chromium's icon set.
  }
  for (auto* view : action_views_) {
    view->OnThemeChanged();
  }
}

void AstraPageActionsView::OnActionsChanged(AstraPageActionsModel* model) {
  DCHECK_EQ(model_, model);
  RebuildActionViews();
}

void AstraPageActionsView::OnActionChanged(AstraPageActionsModel* model,
                                           AstraPageActionType type) {
  DCHECK_EQ(model_, model);
  const AstraPageActionItem* item = model_->GetAction(type);
  if (!item) {
    return;
  }
  AstraPageActionView* view = GetActionView(type);
  if (view) {
    UpdateActionView(view, *item);
  }
}

void AstraPageActionsView::OnPageActionsModelShutdown(
    AstraPageActionsModel* model) {
  DCHECK_EQ(model_, model);
  model_observation_.Reset();
  model_ = nullptr;
}

void AstraPageActionsView::OnPageActionClicked(AstraPageActionType type) {
  if (delegate_) {
    delegate_->OnPageActionClicked(type);
  }
}

void AstraPageActionsView::OnPageActionContextMenu(AstraPageActionType type,
                                                   const gfx::Point& point) {
  if (delegate_) {
    delegate_->OnPageActionContextMenu(type, point);
  }
}

void AstraPageActionsView::OnExtensionActionClicked(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnExtensionActionClicked(extension_id);
  }
}

bool AstraPageActionsView::OnOverflowButtonClicked(views::View* anchor_view) {
  if (delegate_) {
    return delegate_->OnOverflowButtonClicked(anchor_view);
  }
  return false;
}

void AstraPageActionsView::RebuildActionViews() {
  // Clear existing action views.
  action_views_.clear();

  // Remove all existing action child views (keep overflow button).
  std::vector<raw_ptr<views::View>> to_remove;
  for (views::View* child : children()) {
    if (child != overflow_button_) {
      to_remove.push_back(child);
    }
  }
  for (auto* child : to_remove) {
    RemoveChildViewT(child);
  }

  if (!model_) {
    UpdateOverflowButtonVisibility();
    InvalidateLayout();
    return;
  }

  auto pinned_actions = model_->GetPinnedActions();

  // Insert action views before the overflow button.
  for (const auto& item : pinned_actions) {
    auto action_view = std::make_unique<AstraPageActionView>(item.type);
    action_view->SetDelegate(this);
    action_view->SetIconSize(icon_size_px_);
    if (item.type == AstraPageActionType::kExtensionAction) {
      action_view->SetExtensionId(item.extension_id);
    }
    UpdateActionView(action_view.get(), item);

    // Add before the overflow button.
    AstraPageActionView* raw_view = action_view.get();
    AddChildViewAt(std::move(action_view), 0);

    // Re-add to front in correct order (we'll sort).
    action_views_.push_back(raw_view);
  }

  // Reverse because we added at index 0 repeatedly.
  std::reverse(action_views_.begin(), action_views_.end());

  // Re-insert overflow button at the end.
  if (overflow_button_) {
    ReorderChildView(overflow_button_, -1);
  }

  UpdateOverflowButtonVisibility();
  InvalidateLayout();
}

void AstraPageActionsView::UpdateActionViews() {
  if (!model_) {
    return;
  }
  for (auto* view : action_views_) {
    const AstraPageActionItem* item = model_->GetAction(view->GetActionType());
    if (item) {
      UpdateActionView(view, *item);
    }
  }
}

void AstraPageActionsView::UpdateActionView(AstraPageActionView* view,
                                            const AstraPageActionItem& item) {
  view->SetLabel(item.label);
  view->SetActionState(item.state);
  if (!item.badge_text.empty()) {
    view->SetBadgeText(item.badge_text);
    view->SetBadgeColor(item.badge_color);
  } else {
    view->SetBadgeText(std::u16string());
  }
  view->SetVisible(item.visible);
}

void AstraPageActionsView::UpdateOverflowButtonVisibility() {
  if (!model_) {
    overflow_button_->SetVisible(false);
    return;
  }

  auto overflow_actions = model_->GetOverflowActions();
  overflow_button_->SetVisible(!overflow_actions.empty());
}

void AstraPageActionsView::OnOverflowButtonPressed() {
  if (delegate_ && delegate_->OnOverflowButtonClicked(overflow_button_)) {
    return;  // Delegate handled it.
  }
  // Default: show our own overflow menu.
  RunOverflowMenu();
}

void AstraPageActionsView::RunOverflowMenu() {
  if (!model_) {
    return;
  }

  auto overflow_actions = model_->GetOverflowActions();
  if (overflow_actions.empty()) {
    return;
  }

  // Build menu items.
  auto menu_delegate = std::make_unique<views::MenuDelegate>();
  auto* menu_item = new views::MenuItemView(menu_delegate.get());

  for (const auto& item : overflow_actions) {
    views::MenuItemView* sub_item =
        menu_item->AppendMenuItem(0, item.label, views::MenuItemView::NORMAL);
    // Store the action type in the menu item's command id.
    // We use a simple approach: store type in the command_id as an int.
    sub_item->SetCommand(static_cast<int>(item.type));
    sub_item->SetEnabled(item.state != AstraPageActionState::kDisabled);
  }

  overflow_menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_item, views::MenuRunner::HAS_MNEMONICS |
                     views::MenuRunner::CONTEXT_MENU);

  gfx::Point anchor_point = overflow_button_->GetMirroredPosition();
  anchor_point.set_y(overflow_button_->height());

  overflow_menu_runner_->RunMenuAt(
      GetWidget(), nullptr, gfx::Rect(anchor_point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, ui::MENU_SOURCE_MOUSE);
}

const char* AstraPageActionsView::GetDefaultIconName(
    AstraPageActionType type) const {
  switch (type) {
    case AstraPageActionType::kBookmarkStar:
      return "bookmark";
    case AstraPageActionType::kZoom:
      return "zoom";
    case AstraPageActionType::kTranslate:
      return "translate";
    case AstraPageActionType::kFind:
      return "find";
    case AstraPageActionType::kPrint:
      return "print";
    case AstraPageActionType::kSidebar:
      return "sidebar";
    case AstraPageActionType::kScreenshot:
      return "screenshot";
    case AstraPageActionType::kReadingList:
      return "reading_list";
    case AstraPageActionType::kNote:
      return "note";
    case AstraPageActionType::kFavorite:
      return "favorite";
    case AstraPageActionType::kFocusMode:
      return "focus_mode";
    case AstraPageActionType::kSplitView:
      return "split_view";
    case AstraPageActionType::kCommandPalette:
      return "command_palette";
    default:
      return "generic";
  }
}

}  // namespace astra
