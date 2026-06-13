#include "astra/ui/views/omnibox/astra_location_bar_decoration.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/events/gesture_event.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

#include "astra/ui/color/astra_color_ids.h"

namespace astra {

namespace {

// Converts a hex color string to SkColor.
SkColor HexToSkColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') {
    return SK_ColorGRAY;
  }
  std::string hex_value = hex.substr(1);
  if (hex_value.size() == 6) {
    hex_value += "FF";
  }
  if (hex_value.size() != 8) {
    return SK_ColorGRAY;
  }
  unsigned int r, g, b, a;
  if (sscanf(hex_value.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a) != 4) {
    return SK_ColorGRAY;
  }
  return SkColorSetARGB(a, r, g, b);
}

// Stroke width for the hover ring around the workspace indicator.
constexpr int kHoverRingStrokeWidth = 2;

// Corner radius for action buttons in chip style.
constexpr int kChipCornerRadius = 16;

// Spacing between icon and label in labeled buttons.
constexpr int kIconLabelSpacing = 4;

}  // namespace

// =========================================================================
// WorkspaceIndicatorButton
// =========================================================================

AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    WorkspaceIndicatorButton(SkColor color)
    : color_(color) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::SetColor(
    SkColor color) {
  if (color_ != color) {
    color_ = color;
    SchedulePaint();
  }
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    SetWorkspaceName(const std::u16string& name) {
  workspace_name_ = name;
  SetTooltipText(name);
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    SetBadgeVisible(bool visible) {
  if (badge_visible_ == visible) {
    return;
  }
  badge_visible_ = visible;
  SetVisible(visible);
  SchedulePaint();
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::OnPaint(
    gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  if (!badge_visible_) {
    return;
  }

  gfx::Rect bounds = GetContentsBounds();
  gfx::Point center = bounds.CenterPoint();

  if (hovered_ || pressed_) {
    const auto* color_provider = GetColorProvider();
    SkColor ring_color = color_provider
                             ? color_provider->GetColor(
                                   kColorAstraWorkspaceAccentSubtle)
                             : SkColorSetARGB(0x20, 0, 0, 0);
    int ring_padding = pressed_ ? 2 : 1;
    float ring_radius = kDotSize / 2.0f + ring_padding + kHoverRingStrokeWidth;
    cc::PaintFlags ring_flags;
    ring_flags.setColor(ring_color);
    ring_flags.setStyle(cc::PaintFlags::kStroke_Style);
    ring_flags.setStrokeWidth(kHoverRingStrokeWidth);
    ring_flags.setAntiAlias(true);
    canvas->DrawCircle(center, ring_radius, ring_flags);
  }

  cc::PaintFlags flags;
  flags.setColor(color_);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  canvas->DrawCircle(center, kDotSize / 2.0f, flags);
}

bool AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    OnMousePressed(const ui::MouseEvent& event) {
  if (!event.IsOnlyLeftMouseButton() || !badge_visible_) {
    return false;
  }
  pressed_ = true;
  UpdateStateVisuals();
  return true;
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    OnMouseReleased(const ui::MouseEvent& event) {
  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }
  bool was_pressed = pressed_;
  pressed_ = false;
  UpdateStateVisuals();

  if (was_pressed && badge_visible_ &&
      GetLocalBounds().Contains(event.location()) && pressed_callback_) {
    pressed_callback_.Run();
  }
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    OnMouseEntered(const ui::MouseEvent& event) {
  hovered_ = true;
  UpdateStateVisuals();
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    OnMouseExited(const ui::MouseEvent& event) {
  hovered_ = false;
  pressed_ = false;
  UpdateStateVisuals();
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    OnGestureEvent(ui::GestureEvent* event) {
  if (!badge_visible_) {
    return;
  }
  if (event->type() == ui::ET_GESTURE_TAP && pressed_callback_) {
    pressed_callback_.Run();
    event->SetHandled();
  }
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  if (!workspace_name_.empty()) {
    node_data->SetName(workspace_name_);
    node_data->SetDescription(u"Current workspace. Click to switch.");
  } else {
    node_data->SetName(u"Astra workspace");
  }
}

std::u16string
AstraLocationBarDecorationView::WorkspaceIndicatorButton::GetTooltipText(
    const gfx::Point& p) const {
  if (!workspace_name_.empty()) {
    return workspace_name_;
  }
  return u"Astra workspace";
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::
    UpdateStateVisuals() {
  SchedulePaint();
}

// =========================================================================
// FocusModeBadge
// =========================================================================

AstraLocationBarDecorationView::FocusModeBadge::FocusModeBadge() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetTooltipText(u"Focus mode is active");
}

void AstraLocationBarDecorationView::FocusModeBadge::OnPaint(
    gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();
  gfx::Point center = bounds.CenterPoint();

  const auto* color_provider = GetColorProvider();
  SkColor bg_color =
      color_provider
          ? color_provider->GetColor(kColorAstraFocusModeIndicatorBackground)
          : SkColorSetARGB(0xE6, 0x20, 0x21, 0x24);
  SkColor fg_color =
      color_provider
          ? color_provider->GetColor(kColorAstraFocusModeIndicatorText)
          : SK_ColorWHITE;

  int badge_size = kFocusBadgeSize;
  gfx::Rect badge_rect(center.x() - badge_size / 2,
                       center.y() - badge_size / 2, badge_size, badge_size);
  cc::PaintFlags bg_flags;
  bg_flags.setColor(bg_color);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  bg_flags.setAntiAlias(true);
  canvas->DrawRoundRect(badge_rect, badge_size / 2.0, bg_flags);

  cc::PaintFlags fg_flags;
  fg_flags.setColor(fg_color);
  fg_flags.setStyle(cc::PaintFlags::kFill_Style);
  fg_flags.setAntiAlias(true);
  canvas->DrawCircle(center, badge_size / 4.0f, fg_flags);
}

void AstraLocationBarDecorationView::FocusModeBadge::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraLocationBarDecorationView::FocusModeBadge::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kStatus;
  node_data->SetName(u"Focus mode active");
}

std::u16string AstraLocationBarDecorationView::FocusModeBadge::GetTooltipText(
    const gfx::Point& p) const {
  return u"Focus mode is active";
}

// =========================================================================
// AstraLocationBarDecorationView — Construction / destruction
// =========================================================================

AstraLocationBarDecorationView::AstraLocationBarDecorationView(Edge edge)
    : edge_(edge) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // 1. Workspace indicator button (colored dot).
  workspace_indicator_ =
      AddChildView(std::make_unique<WorkspaceIndicatorButton>(SK_ColorGRAY));
  workspace_indicator_->SetPreferredSize(
      gfx::Size(kWorkspaceButtonSize, kWorkspaceButtonSize));
  workspace_indicator_->SetCallback(base::BindRepeating(
      &AstraLocationBarDecorationView::OnWorkspaceIndicatorPressed,
      base::Unretained(this)));

  // 2. Focus mode badge (hidden by default).
  focus_mode_badge_ = AddChildView(std::make_unique<FocusModeBadge>());
  focus_mode_badge_->SetPreferredSize(
      gfx::Size(kFocusBadgeSize, kFocusBadgeSize));
  focus_mode_badge_->SetVisible(false);

  // 3. Overflow button (created but hidden until needed).
  auto overflow = std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraLocationBarDecorationView::OnOverflowButtonPressed,
          base::Unretained(this)),
      u"\u2026");  // Ellipsis character
  overflow->SetPreferredSize(
      gfx::Size(kDefaultActionButtonSize, kDefaultActionButtonSize));
  overflow->SetTooltipText(u"More actions");
  overflow->SetAccessibleName(u"More actions");
  overflow->SetVisible(false);
  overflow_button_ = AddChildView(std::move(overflow));

  SetTooltipText(std::u16string());
}

AstraLocationBarDecorationView::~AstraLocationBarDecorationView() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

// =========================================================================
// Model binding
// =========================================================================

void AstraLocationBarDecorationView::SetModel(
    AstraOmniboxDecorationModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_) {
    model_->RemoveObserver(this);
  }
  model_ = model;
  if (model_) {
    model_->AddObserver(this);
  }
  RebuildActionButtons();
  UpdateVisibility();
  UpdateVisuals();
  InvalidateLayout();
}

// =========================================================================
// State update (convenience setters)
// =========================================================================

void AstraLocationBarDecorationView::UpdateWorkspace(
    const std::string& workspace_name,
    const std::string& accent_color) {
  workspace_name_ = base::UTF8ToUTF16(workspace_name);
  accent_color_ = accent_color;
  UpdateVisuals();
}

void AstraLocationBarDecorationView::SetFocusModeActive(bool active) {
  if (focus_mode_active_ == active) {
    return;
  }
  focus_mode_active_ = active;
  if (focus_mode_badge_) {
    focus_mode_badge_->SetVisible(active);
  }
  InvalidateLayout();
}

void AstraLocationBarDecorationView::SetDecorationVisible(bool visible) {
  if (model_) {
    model_->SetShowDecoration(visible);
  } else {
    SetVisible(visible);
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraLocationBarDecorationView::CalculatePreferredSize() const {
  int width = kHorizontalPadding * 2;
  int height = kDecorationHeight;

  if (workspace_indicator_ && workspace_indicator_->GetVisible()) {
    width += kWorkspaceButtonSize + kElementSpacing;
  }

  if (focus_mode_badge_ && focus_mode_badge_->GetVisible()) {
    width += kFocusBadgeSize + kElementSpacing;
  }

  // Count visible action buttons (up to max visible).
  int direct_count = GetDirectActionCount();
  if (model_ && model_->icon_size() == AstraDecorationIconSize::kLarge) {
    width += direct_count * (28 + kElementSpacing);
  } else if (model_ && model_->icon_size() == AstraDecorationIconSize::kSmall) {
    width += direct_count * (20 + kElementSpacing);
  } else {
    width += direct_count * (24 + kElementSpacing);
  }

  // Add width for labels if shown.
  if (model_ && model_->show_labels() &&
      model_->button_style() != AstraDecorationButtonStyle::kIconOnly) {
    width += direct_count * 40;  // Approximate label width.
  }

  // Overflow button if there are hidden actions.
  if (overflow_button_ && overflow_button_->GetVisible()) {
    width += kDefaultActionButtonSize + kElementSpacing;
  }

  return gfx::Size(width, height);
}

void AstraLocationBarDecorationView::Layout() {
  views::View::Layout();

  gfx::Rect bounds = GetContentsBounds();
  int x = bounds.x() + kHorizontalPadding;
  int center_y = bounds.CenterPoint().y();

  // For trailing edge, lay out right-to-left.
  bool reverse = (edge_ == Edge::kTrailing);
  if (reverse) {
    x = bounds.right() - kHorizontalPadding;
  }

  // 1. Workspace indicator.
  if (workspace_indicator_ && workspace_indicator_->GetVisible()) {
    int size = kWorkspaceButtonSize;
    int y = center_y - size / 2;
    if (reverse) {
      x -= size;
      workspace_indicator_->SetBounds(x, y, size, size);
      x -= kElementSpacing;
    } else {
      workspace_indicator_->SetBounds(x, y, size, size);
      x += size + kElementSpacing;
    }
  }

  // 2. Focus mode badge.
  if (focus_mode_badge_ && focus_mode_badge_->GetVisible()) {
    int size = kFocusBadgeSize;
    int y = center_y - size / 2;
    if (reverse) {
      x -= size;
      focus_mode_badge_->SetBounds(x, y, size, size);
      x -= kElementSpacing;
    } else {
      focus_mode_badge_->SetBounds(x, y, size, size);
      x += size + kElementSpacing;
    }
  }

  // 3. Action buttons.
  int direct_count = GetDirectActionCount();
  int shown = 0;
  for (const auto& [id, button] : action_buttons_) {
    if (!button->GetVisible()) {
      continue;
    }
    if (shown >= direct_count) {
      button->SetVisible(false);
      continue;
    }
    button->SetVisible(true);

    int btn_size = AstraOmniboxDecorationModel::GetIconSizeDp(
        model_ ? model_->icon_size() : AstraDecorationIconSize::kMedium);
    btn_size += 8;  // Add padding for click target.

    int btn_width = btn_size;
    if (model_ && model_->show_labels() &&
        model_->button_style() != AstraDecorationButtonStyle::kIconOnly) {
      btn_width += 40;  // Label width
    }

    int y = center_y - btn_size / 2;
    if (reverse) {
      x -= btn_width;
      button->SetBounds(x, y, btn_width, btn_size);
      x -= kElementSpacing;
    } else {
      button->SetBounds(x, y, btn_width, btn_size);
      x += btn_width + kElementSpacing;
    }
    ++shown;
  }

  // 4. Overflow button.
  if (overflow_button_) {
    bool has_overflow =
        action_buttons_.size() > static_cast<size_t>(direct_count);
    overflow_button_->SetVisible(has_overflow && model_ &&
                                  model_->show_overflow_menu());
    if (overflow_button_->GetVisible()) {
      int size = kDefaultActionButtonSize;
      int y = center_y - size / 2;
      if (reverse) {
        x -= size;
        overflow_button_->SetBounds(x, y, size, size);
      } else {
        overflow_button_->SetBounds(x, y, size, size);
      }
    }
  }
}

void AstraLocationBarDecorationView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
  UpdateButtonStyles();
}

bool AstraLocationBarDecorationView::OnMousePressed(
    const ui::MouseEvent& event) {
  // Let child views handle their own events.
  return views::View::OnMousePressed(event);
}

void AstraLocationBarDecorationView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  if (model_ && model_->hover_expansion()) {
    UpdateVisibility();
    InvalidateLayout();
  }
}

void AstraLocationBarDecorationView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  if (model_ && model_->hover_expansion()) {
    UpdateVisibility();
    InvalidateLayout();
  }
}

// =========================================================================
// AstraOmniboxDecorationModelObserver
// =========================================================================

void AstraLocationBarDecorationView::OnActionAdded(
    const std::string& action_id) {
  RebuildActionButtons();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnActionRemoved(
    const std::string& action_id) {
  RebuildActionButtons();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnActionVisibilityChanged(
    const std::string& action_id,
    bool visible) {
  // Update individual button visibility.
  for (auto& [id, button] : action_buttons_) {
    if (id == action_id) {
      button->SetVisible(visible);
      break;
    }
  }
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnActionOrderChanged() {
  RebuildActionButtons();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnDecorationSettingsChanged() {
  UpdateVisibility();
  UpdateButtonStyles();
  UpdateVisuals();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnOmniboxFocusChanged(bool focused) {
  UpdateVisibility();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnSecurityStateChanged(
    AstraSecurityLevel level) {
  // Security state affects visual styling of the decoration.
  // TODO(astra): Adjust colors based on security level.
  SchedulePaint();
}

// =========================================================================
// Test helpers
// =========================================================================

views::LabelButton* AstraLocationBarDecorationView::GetActionButtonForTest(
    const std::string& action_id) {
  for (auto& [id, button] : action_buttons_) {
    if (id == action_id) {
      return button;
    }
  }
  return nullptr;
}

// =========================================================================
// Private methods
// =========================================================================

void AstraLocationBarDecorationView::RebuildActionButtons() {
  // Remove existing action buttons.
  for (auto& [id, button] : action_buttons_) {
    if (button) {
      RemoveChildViewT(button);
    }
  }
  action_buttons_.clear();

  if (!model_) {
    InvalidateLayout();
    return;
  }

  // Create buttons for each action from the model.
  auto actions = model_->GetAllActions();
  for (const auto& action : actions) {
    auto button = CreateActionButton(action);
    button->SetVisible(action.is_visible);
    action_buttons_.emplace_back(action.id,
                                 AddChildView(std::move(button)));
  }

  UpdateButtonStyles();
}

void AstraLocationBarDecorationView::UpdateVisuals() {
  if (workspace_indicator_) {
    SkColor color = HexToSkColor(accent_color_);
    workspace_indicator_->SetColor(color);
    workspace_indicator_->SetWorkspaceName(workspace_name_);
  }

  SchedulePaint();
}

void AstraLocationBarDecorationView::UpdateVisibility() {
  bool visible = ShouldBeVisible();
  SetVisible(visible);

  // Update workspace indicator visibility based on model settings.
  if (model_ && workspace_indicator_) {
    workspace_indicator_->SetBadgeVisible(model_->show_workspace());
  }
}

void AstraLocationBarDecorationView::UpdateButtonStyles() {
  if (!model_) {
    return;
  }

  for (auto& [id, button] : action_buttons_) {
    const AstraDecorationAction* action = model_->GetAction(id);
    if (!action) {
      continue;
    }

    // Update label text based on show_labels setting.
    if (model_->button_style() == AstraDecorationButtonStyle::kIconOnly) {
      button->SetText(std::u16string());
      button->SetTooltipText(action->tooltip);
    } else {
      button->SetText(
          AstraOmniboxDecorationModel::FormatActionLabel(action->label));
    }

    // Update button styling.
    if (model_->button_style() == AstraDecorationButtonStyle::kChip) {
      // Chip style: rounded, with background.
      button->SetStyle(views::Button::STYLE_BUTTON);
    } else {
      button->SetStyle(views::Button::STYLE_BUTTON);
    }

    // Update accessibility.
    button->SetAccessibleName(action->label);
    if (!action->shortcut.empty()) {
      button->SetTooltipText(action->label + u" (" + action->shortcut + u")");
    } else {
      button->SetTooltipText(action->tooltip);
    }
  }
}

void AstraLocationBarDecorationView::OnWorkspaceIndicatorPressed() {
  if (delegate_) {
    delegate_->OnWorkspaceIndicatorClicked();
  }
}

void AstraLocationBarDecorationView::OnActionButtonPressed(
    const std::string& action_id) {
  if (delegate_) {
    delegate_->OnActionClicked(action_id);
  }
}

void AstraLocationBarDecorationView::OnOverflowButtonPressed() {
  if (delegate_) {
    delegate_->OnOverflowMenuClicked();
  }
}

std::unique_ptr<views::LabelButton>
AstraLocationBarDecorationView::CreateActionButton(
    const AstraDecorationAction& action) {
  auto button = std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraLocationBarDecorationView::OnActionButtonPressed,
          base::Unretained(this), action.id),
      std::u16string());
  button->SetPreferredSize(
      gfx::Size(kDefaultActionButtonSize, kDefaultActionButtonSize));
  button->SetTooltipText(action.tooltip);
  button->SetAccessibleName(action.label);
  button->SetFocusForPlatform();
  return button;
}

bool AstraLocationBarDecorationView::ShouldBeVisible() const {
  if (!model_) {
    return GetVisible();
  }
  if (!model_->show_decoration()) {
    return false;
  }
  if (model_->show_on_focus_only() && !model_->omnibox_focused() &&
      !is_hovered_) {
    return false;
  }
  return true;
}

int AstraLocationBarDecorationView::GetDirectActionCount() const {
  if (!model_) {
    return 0;
  }
  size_t visible_count = model_->GetVisibleActionCount();
  size_t max_direct = static_cast<size_t>(model_->max_visible_actions());

  // If hover expansion is enabled and we're hovered, show more actions.
  if (model_->hover_expansion() && is_hovered_) {
    max_direct = std::min(max_direct + 2,
                           static_cast<size_t>(AstraOmniboxDecorationModel::
                                                   kMaxVisibleActions));
  }

  return static_cast<int>(std::min(visible_count, max_direct));
}

}  // namespace astra
