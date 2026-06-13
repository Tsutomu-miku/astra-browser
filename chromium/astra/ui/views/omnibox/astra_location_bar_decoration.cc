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

// Stroke width for the hover ring around the workspace indicator.
constexpr int kHoverRingStrokeWidth = 2;

// Spacing between icon and label in labeled buttons.
constexpr int kIconLabelSpacing = 4;

// Default badge size (diameter).
constexpr int kBadgeSize = 14;

// Badge text size.
constexpr int kBadgeFontSize = 10;

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

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::SetBadgeText(
    const std::u16string& text,
    SkColor color) {
  if (badge_text_ == text && badge_color_ == color && has_badge_) {
    return;
  }
  has_badge_ = !text.empty();
  badge_text_ = text;
  badge_color_ = color;
  SchedulePaint();
}

void AstraLocationBarDecorationView::WorkspaceIndicatorButton::ClearBadge() {
  if (!has_badge_) {
    return;
  }
  has_badge_ = false;
  badge_text_.clear();
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

  // Draw badge if present.
  if (has_badge_ && !badge_text_.empty()) {
    gfx::Point badge_center(
        bounds.right() - kBadgeSize / 2 - 1,
        bounds.y() + kBadgeSize / 2 + 1);

    cc::PaintFlags badge_bg_flags;
    badge_bg_flags.setColor(badge_color_);
    badge_bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    badge_bg_flags.setAntiAlias(true);
    canvas->DrawCircle(badge_center, kBadgeSize / 2.0f, badge_bg_flags);

    // Badge text (simple approach: draw centered text).
    // Full text rendering requires font lists; this is a simplified version.
    if (badge_text_.size() <= 2) {
      canvas->DrawStringRectWithFlags(
          badge_text_, gfx::FontList(), SK_ColorWHITE,
          gfx::Rect(badge_center.x() - kBadgeSize / 2,
                    badge_center.y() - kBadgeSize / 2,
                    kBadgeSize, kBadgeSize),
          gfx::Canvas::TEXT_ALIGN_CENTER | gfx::Canvas::NO_SUBPIXEL_RENDERING);
    }
  }
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

void AstraLocationBarDecorationView::FocusModeBadge::SetActive(bool active) {
  if (active_ == active) {
    return;
  }
  active_ = active;
  SetVisible(active);
  SchedulePaint();
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
// DecorationButton
// =========================================================================

AstraLocationBarDecorationView::DecorationButton::DecorationButton(
    base::RepeatingClosure callback,
    AstraOmniboxDecorationType type)
    : LabelButton(std::move(callback), std::u16string()),
      type_(type) {
  SetFocusForPlatform();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

void AstraLocationBarDecorationView::DecorationButton::SetBadge(
    const std::u16string& text,
    SkColor color) {
  if (badge_text_ == text && badge_color_ == color && has_badge_) {
    return;
  }
  has_badge_ = !text.empty();
  badge_text_ = text;
  badge_color_ = color;
  SchedulePaint();
}

void AstraLocationBarDecorationView::DecorationButton::ClearBadge() {
  if (!has_badge_) {
    return;
  }
  has_badge_ = false;
  badge_text_.clear();
  SchedulePaint();
}

void AstraLocationBarDecorationView::DecorationButton::SetActive(bool active) {
  if (active_ == active) {
    return;
  }
  active_ = active;
  SchedulePaint();
}

void AstraLocationBarDecorationView::DecorationButton::OnPaint(
    gfx::Canvas* canvas) {
  views::LabelButton::OnPaint(canvas);

  // Draw badge if present.
  if (has_badge_ && !badge_text_.empty()) {
    gfx::Rect bounds = GetContentsBounds();
    gfx::Point badge_center(
        bounds.right() - kBadgeSize / 2 - 2,
        bounds.y() + kBadgeSize / 2 + 2);

    cc::PaintFlags badge_bg_flags;
    badge_bg_flags.setColor(badge_color_);
    badge_bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    badge_bg_flags.setAntiAlias(true);
    canvas->DrawCircle(badge_center, kBadgeSize / 2.0f, badge_bg_flags);

    // Badge text.
    if (badge_text_.size() <= 2) {
      canvas->DrawStringRectWithFlags(
          badge_text_, gfx::FontList(), SK_ColorWHITE,
          gfx::Rect(badge_center.x() - kBadgeSize / 2,
                    badge_center.y() - kBadgeSize / 2,
                    kBadgeSize, kBadgeSize),
          gfx::Canvas::TEXT_ALIGN_CENTER | gfx::Canvas::NO_SUBPIXEL_RENDERING);
    }
  }
}

void AstraLocationBarDecorationView::DecorationButton::OnThemeChanged() {
  views::LabelButton::OnThemeChanged();
  SchedulePaint();
}

// =========================================================================
// AstraLocationBarDecorationView — Construction / destruction
// =========================================================================

AstraLocationBarDecorationView::AstraLocationBarDecorationView(
    AstraDecorationPosition position)
    : position_(position) {
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
  RebuildDecorationButtons();
  UpdateAllDecorations();
  InvalidateLayout();
}

// =========================================================================
// Decoration view access
// =========================================================================

views::View* AstraLocationBarDecorationView::GetDecorationView(
    AstraOmniboxDecorationType type) {
  // Check special decorations first.
  if (type == AstraOmniboxDecorationType::kWorkspaceIndicator) {
    return workspace_indicator_;
  }
  if (type == AstraOmniboxDecorationType::kFocusModeBadge) {
    return focus_mode_badge_;
  }
  // Check regular decoration buttons.
  for (const auto& [t, button] : decoration_buttons_) {
    if (t == type) {
      return button;
    }
  }
  return nullptr;
}

int AstraLocationBarDecorationView::GetDecorationCount() const {
  int count = 0;
  if (workspace_indicator_ && workspace_indicator_->GetVisible()) {
    ++count;
  }
  if (focus_mode_badge_ && focus_mode_badge_->GetVisible()) {
    ++count;
  }
  for (const auto& [type, button] : decoration_buttons_) {
    if (button && button->GetVisible()) {
      ++count;
    }
  }
  return count;
}

// =========================================================================
// Position
// =========================================================================

void AstraLocationBarDecorationView::SetPosition(
    AstraDecorationPosition position) {
  if (position_ == position) {
    return;
  }
  position_ = position;
  InvalidateLayout();
}

// =========================================================================
// Compact mode
// =========================================================================

void AstraLocationBarDecorationView::SetCompactMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;
  InvalidateLayout();
}

// =========================================================================
// Icon size
// =========================================================================

void AstraLocationBarDecorationView::SetIconSize(int size_px) {
  if (icon_size_px_ == size_px) {
    return;
  }
  icon_size_px_ = size_px;
  InvalidateLayout();
}

// =========================================================================
// Animation
// =========================================================================

void AstraLocationBarDecorationView::SetAnimationEnabled(bool enabled) {
  animation_enabled_ = enabled;
}

// =========================================================================
// Spacing
// =========================================================================

void AstraLocationBarDecorationView::SetSpacing(int spacing_px) {
  if (spacing_px_ == spacing_px) {
    return;
  }
  spacing_px_ = spacing_px;
  InvalidateLayout();
}

// =========================================================================
// Bulk update
// =========================================================================

void AstraLocationBarDecorationView::UpdateAllDecorations() {
  UpdateVisibility();
  UpdateVisuals();
  UpdateLayout();
}

// =========================================================================
// Bubble management
// =========================================================================

void AstraLocationBarDecorationView::ShowBubbleForDecoration(
    AstraOmniboxDecorationType type) {
  if (model_) {
    model_->ShowDecorationBubble(type);
  }
  if (delegate_) {
    delegate_->ShowBubbleForDecoration(type);
  }
  open_bubble_type_ = type;
}

void AstraLocationBarDecorationView::HideAllBubbles() {
  if (model_) {
    model_->HideAllBubbles();
  }
  open_bubble_type_ = AstraOmniboxDecorationType::kNone;
}

AstraOmniboxDecorationType
AstraLocationBarDecorationView::GetOpenBubbleType() const {
  if (model_) {
    return model_->GetOpenBubbleType();
  }
  return open_bubble_type_;
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraLocationBarDecorationView::CalculatePreferredSize() const {
  int width = kHorizontalPadding * 2;
  int height = kDecorationHeight;

  int spacing = GetEffectiveSpacing();
  int btn_size = GetEffectiveIconSize() + 8;  // + padding for click target

  if (workspace_indicator_ && workspace_indicator_->GetVisible()) {
    width += kWorkspaceButtonSize + spacing;
  }

  if (focus_mode_badge_ && focus_mode_badge_->GetVisible()) {
    width += kFocusBadgeSize + spacing;
  }

  // Count visible decoration buttons.
  for (const auto& [type, button] : decoration_buttons_) {
    if (button && button->GetVisible()) {
      width += btn_size + spacing;
    }
  }

  // Remove trailing spacing if we added any elements.
  if (width > kHorizontalPadding * 2) {
    width -= spacing;
  }

  return gfx::Size(width, height);
}

void AstraLocationBarDecorationView::Layout() {
  views::View::Layout();

  gfx::Rect bounds = GetContentsBounds();
  int x = bounds.x() + kHorizontalPadding;
  int center_y = bounds.CenterPoint().y();
  int spacing = GetEffectiveSpacing();
  int btn_size = GetEffectiveIconSize() + 8;  // + padding for click target

  // For trailing position, lay out right-to-left.
  bool reverse = (position_ == AstraDecorationPosition::kTrailing);
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
      x -= spacing;
    } else {
      workspace_indicator_->SetBounds(x, y, size, size);
      x += size + spacing;
    }
  }

  // 2. Focus mode badge.
  if (focus_mode_badge_ && focus_mode_badge_->GetVisible()) {
    int size = kFocusBadgeSize;
    int y = center_y - size / 2;
    if (reverse) {
      x -= size;
      focus_mode_badge_->SetBounds(x, y, size, size);
      x -= spacing;
    } else {
      focus_mode_badge_->SetBounds(x, y, size, size);
      x += size + spacing;
    }
  }

  // 3. Decoration buttons.
  for (const auto& [type, button] : decoration_buttons_) {
    if (!button || !button->GetVisible()) {
      continue;
    }

    int y = center_y - btn_size / 2;
    if (reverse) {
      x -= btn_size;
      button->SetBounds(x, y, btn_size, btn_size);
      x -= spacing;
    } else {
      button->SetBounds(x, y, btn_size, btn_size);
      x += btn_size + spacing;
    }
  }
}

void AstraLocationBarDecorationView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
}

void AstraLocationBarDecorationView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  // TODO(astra): Handle hover badge visibility if
  // show_badges_on_hover_only is enabled.
}

void AstraLocationBarDecorationView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  // TODO(astra): Handle hover badge visibility if
  // show_badges_on_hover_only is enabled.
}

// =========================================================================
// AstraOmniboxDecorationObserver
// =========================================================================

void AstraLocationBarDecorationView::OnDecorationVisibilityChanged(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type,
    bool visible) {
  // Update special decorations.
  if (type == AstraOmniboxDecorationType::kWorkspaceIndicator &&
      workspace_indicator_) {
    workspace_indicator_->SetBadgeVisible(visible);
  } else if (type == AstraOmniboxDecorationType::kFocusModeBadge &&
             focus_mode_badge_) {
    focus_mode_badge_->SetVisible(visible);
  } else {
    // Update regular decoration buttons.
    for (auto& [t, button] : decoration_buttons_) {
      if (t == type) {
        button->SetVisible(visible);
        break;
      }
    }
  }
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnDecorationActiveChanged(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type,
    bool active) {
  if (type == AstraOmniboxDecorationType::kFocusModeBadge &&
      focus_mode_badge_) {
    focus_mode_badge_->SetActive(active);
  } else {
    for (auto& [t, button] : decoration_buttons_) {
      if (t == type) {
        button->SetActive(active);
        break;
      }
    }
  }
}

void AstraLocationBarDecorationView::OnDecorationBadgeChanged(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type) {
  const AstraOmniboxDecorationItem* item = model->GetDecorationByType(type);
  if (!item) {
    return;
  }

  if (type == AstraOmniboxDecorationType::kWorkspaceIndicator &&
      workspace_indicator_) {
    if (!item->badge_text.empty()) {
      workspace_indicator_->SetBadgeText(item->badge_text,
                                          item->badge_color);
    } else {
      workspace_indicator_->ClearBadge();
    }
  } else {
    for (auto& [t, button] : decoration_buttons_) {
      if (t == type) {
        if (!item->badge_text.empty()) {
          button->SetBadge(item->badge_text, item->badge_color);
        } else {
          button->ClearBadge();
        }
        break;
      }
    }
  }
}

void AstraLocationBarDecorationView::OnDecorationsReordered(
    AstraOmniboxDecorationModel* model) {
  RebuildDecorationButtons();
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnDecorationExecuted(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type) {
  if (delegate_) {
    delegate_->OnDecorationClicked(type);
  }
}

void AstraLocationBarDecorationView::OnBubbleShown(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type) {
  open_bubble_type_ = type;
}

void AstraLocationBarDecorationView::OnBubbleHidden(
    AstraOmniboxDecorationModel* model,
    AstraOmniboxDecorationType type) {
  if (open_bubble_type_ == type) {
    open_bubble_type_ = AstraOmniboxDecorationType::kNone;
  }
}

void AstraLocationBarDecorationView::OnWorkspaceChanged(
    AstraOmniboxDecorationModel* model,
    const std::u16string& name) {
  if (workspace_indicator_) {
    workspace_indicator_->SetWorkspaceName(name);
    workspace_indicator_->SetColor(model->GetWorkspaceColor());
  }
}

void AstraLocationBarDecorationView::OnFocusModeChanged(
    AstraOmniboxDecorationModel* model,
    bool active) {
  if (focus_mode_badge_) {
    focus_mode_badge_->SetActive(active);
    focus_mode_badge_->SetVisible(active && model->GetShowFocusModeBadge());
  }
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnOmniboxDecorationModelShutdown(
    AstraOmniboxDecorationModel* model) {
  if (model_ == model) {
    model_ = nullptr;
  }
}

// =========================================================================
// Private methods
// =========================================================================

void AstraLocationBarDecorationView::RebuildDecorationButtons() {
  // Remove existing decoration buttons.
  for (auto& [type, button] : decoration_buttons_) {
    if (button) {
      RemoveChildViewT(button);
    }
  }
  decoration_buttons_.clear();

  if (!model_) {
    InvalidateLayout();
    return;
  }

  // Create buttons for each decoration from the model (except the special
  // workspace and focus mode decorations which have their own views).
  const auto& decorations = model_->GetDecorations();
  for (const auto& item : decorations) {
    if (item.type == AstraOmniboxDecorationType::kWorkspaceIndicator ||
        item.type == AstraOmniboxDecorationType::kFocusModeBadge) {
      // These are handled by dedicated child views.
      continue;
    }
    auto button = CreateDecorationButton(item);
    button->SetVisible(item.is_visible);
    button->SetActive(item.is_active);
    if (!item.badge_text.empty()) {
      button->SetBadge(item.badge_text, item.badge_color);
    }
    decoration_buttons_.emplace_back(item.type,
                                     AddChildView(std::move(button)));
  }
}

void AstraLocationBarDecorationView::UpdateVisuals() {
  if (model_ && workspace_indicator_) {
    workspace_indicator_->SetColor(model_->GetWorkspaceColor());
    workspace_indicator_->SetWorkspaceName(model_->GetCurrentWorkspaceName());
  }

  if (model_ && focus_mode_badge_) {
    focus_mode_badge_->SetActive(model_->IsFocusModeActive());
  }

  SchedulePaint();
}

void AstraLocationBarDecorationView::UpdateVisibility() {
  if (!model_) {
    return;
  }

  // Update workspace indicator visibility.
  if (workspace_indicator_) {
    workspace_indicator_->SetBadgeVisible(model_->GetShowWorkspaceIndicator());
  }

  // Update focus mode badge visibility.
  if (focus_mode_badge_) {
    focus_mode_badge_->SetVisible(
        model_->IsFocusModeActive() && model_->GetShowFocusModeBadge());
  }

  // Update regular decoration button visibility.
  for (auto& [type, button] : decoration_buttons_) {
    button->SetVisible(model_->IsDecorationVisible(type));
  }
}

void AstraLocationBarDecorationView::UpdateLayout() {
  InvalidateLayout();
}

void AstraLocationBarDecorationView::OnDecorationButtonPressed(
    AstraOmniboxDecorationType type) {
  if (model_) {
    const AstraOmniboxDecorationItem* item = model_->GetDecorationByType(type);
    if (item && item->has_bubble) {
      ShowBubbleForDecoration(type);
    } else {
      model_->ExecuteDecoration(type);
    }
  }
}

void AstraLocationBarDecorationView::OnWorkspaceIndicatorPressed() {
  if (model_) {
    model_->ShowDecorationBubble(
        AstraOmniboxDecorationType::kWorkspaceIndicator);
  }
  if (delegate_) {
    delegate_->OnDecorationClicked(
        AstraOmniboxDecorationType::kWorkspaceIndicator);
  }
}

std::unique_ptr<AstraLocationBarDecorationView::DecorationButton>
AstraLocationBarDecorationView::CreateDecorationButton(
    const AstraOmniboxDecorationItem& item) {
  auto button = std::make_unique<DecorationButton>(
      base::BindRepeating(
          &AstraLocationBarDecorationView::OnDecorationButtonPressed,
          base::Unretained(this), item.type),
      item.type);
  int btn_size = GetEffectiveIconSize() + 8;
  button->SetPreferredSize(gfx::Size(btn_size, btn_size));
  button->SetTooltipText(item.tooltip);
  button->SetAccessibleName(item.accessibility_label);
  return button;
}

int AstraLocationBarDecorationView::GetEffectiveIconSize() const {
  if (compact_mode_) {
    return static_cast<int>(icon_size_px_ * kCompactScaleFactor);
  }
  return icon_size_px_;
}

int AstraLocationBarDecorationView::GetEffectiveSpacing() const {
  if (compact_mode_) {
    return static_cast<int>(spacing_px_ * kCompactScaleFactor);
  }
  return spacing_px_;
}

}  // namespace astra
