#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"

#include <string>

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

#include "astra/ui/views/focus_mode/astra_focus_mode_menu_bubble.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

namespace astra {

namespace {

// Default indicator sizes for different styles.
constexpr int kIndicatorFullWidth = 180;
constexpr int kIndicatorFullHeight = 32;
constexpr int kIndicatorMinimalSize = 12;
constexpr int kIndicatorBadgeSize = 28;

// Corner radius for the rounded indicator background.
constexpr int kIndicatorCornerRadius = 16;

// Offset from the corners of the browser window.
constexpr int kIndicatorOffsetX = 16;
constexpr int kIndicatorOffsetY = 8;

// Color variants for different states.
constexpr SkColor kActiveBackgroundColor =
    SkColorSetARGB(0xE6, 0x20, 0x21, 0x24);  // Dark semi-transparent.
constexpr SkColor kPausedBackgroundColor =
    SkColorSetARGB(0xE6, 0x5C, 0x3A, 0x3A);  // Reddish for paused.
constexpr SkColor kBreakBackgroundColor =
    SkColorSetARGB(0xE6, 0x2D, 0x4A, 0x3A);  // Greenish for break.
constexpr SkColor kIndicatorTextColor = SK_ColorWHITE;

// Accent border color for different block levels.
constexpr SkColor kBlockLevelNoneBorder = SK_ColorTRANSPARENT;
constexpr SkColor kBlockLevelSocialBorder =
    SkColorSetARGB(0xFF, 0x42, 0x85, 0xF4);  // Blue.
constexpr SkColor kBlockLevelEntertainmentBorder =
    SkColorSetARGB(0xFF, 0xEA, 0x43, 0x35);  // Red.
constexpr SkColor kBlockLevelNewsBorder =
    SkColorSetARGB(0xFF, 0xFA, 0xBD, 0x2F);  // Yellow.
constexpr SkColor kBlockLevelStrictBorder =
    SkColorSetARGB(0xFF, 0x7B, 0x1F, 0xA8);  // Purple.
constexpr SkColor kBlockLevelCustomBorder =
    SkColorSetARGB(0xFF, 0x60, 0x7D, 0x8B);  // Slate.

// Pulse animation interval in milliseconds.
constexpr int kPulseIntervalMs = 2000;
constexpr int kPulseMinOpacity = 0x99;  // ~60% opacity for dimmed pulse.

// Border thickness for accent color.
constexpr int kAccentBorderThickness = 2;

// Returns the border color for a given block level.
SkColor GetBlockLevelBorderColor(AstraFocusBlockLevel level) {
  switch (level) {
    case AstraFocusBlockLevel::kNone:
      return kBlockLevelNoneBorder;
    case AstraFocusBlockLevel::kSocial:
      return kBlockLevelSocialBorder;
    case AstraFocusBlockLevel::kEntertainment:
      return kBlockLevelEntertainmentBorder;
    case AstraFocusBlockLevel::kNews:
      return kBlockLevelNewsBorder;
    case AstraFocusBlockLevel::kStrict:
      return kBlockLevelStrictBorder;
    case AstraFocusBlockLevel::kCustom:
      return kBlockLevelCustomBorder;
  }
  return kBlockLevelNoneBorder;
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraFocusModeIndicator
// ---------------------------------------------------------------------------

// static
AstraFocusModeIndicator* AstraFocusModeIndicator::Show(
    BrowserView* browser_view,
    AstraFocusModeModel* model) {
  if (!browser_view) {
    return nullptr;
  }

  auto* indicator = new AstraFocusModeIndicator(browser_view, model);
  indicator->CreateWidget();
  return indicator;
}

void AstraFocusModeIndicator::SetDelegate(
    AstraFocusModeIndicatorDelegate* delegate) {
  delegate_ = delegate;
}

AstraFocusModeIndicator::AstraFocusModeIndicator(
    BrowserView* browser_view,
    AstraFocusModeModel* model)
    : browser_view_(browser_view), model_(model) {
  // Use a horizontal box layout for the indicator content.
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 12),  // Vertical 0, horizontal 12px padding.
      8));  // Spacing between child views.

  // Set a rounded, semi-transparent background.
  SetBackground(views::CreateRoundedRectBackground(
      kActiveBackgroundColor, kIndicatorCornerRadius));
}

AstraFocusModeIndicator::~AstraFocusModeIndicator() {
  // Stop the pulse timer.
  StopPulseAnimation();

  // Stop observing the widget if we were.
  if (widget_ && observing_widget_) {
    widget_->RemoveObserver(this);
  }
}

// -- Active state ----------------------------------------------------------

void AstraFocusModeIndicator::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  UpdateIndicatorColor();
  UpdateIndicatorText();

  if (active && !is_paused_) {
    StartPulseAnimation();
  } else {
    StopPulseAnimation();
  }

  SchedulePaint();
}

bool AstraFocusModeIndicator::IsActive() const {
  return is_active_;
}

// -- Time display ----------------------------------------------------------

void AstraFocusModeIndicator::SetTimeRemaining(base::TimeDelta remaining) {
  remaining_time_ = remaining;
  UpdateIndicatorText();
}

base::TimeDelta AstraFocusModeIndicator::GetTimeRemaining() const {
  return remaining_time_;
}

// -- Timer visibility ------------------------------------------------------

void AstraFocusModeIndicator::SetTimerVisible(bool visible) {
  if (timer_visible_ == visible) {
    return;
  }
  timer_visible_ = visible;
  UpdateIndicatorText();
}

bool AstraFocusModeIndicator::IsTimerVisible() const {
  return timer_visible_;
}

// -- Block level -----------------------------------------------------------

void AstraFocusModeIndicator::SetBlockLevel(AstraFocusBlockLevel level) {
  if (block_level_ == level) {
    return;
  }
  block_level_ = level;
  // Update the accent/border color based on block level.
  SkColor border_color = GetBlockLevelBorderColor(level);
  if (accent_color_ == SK_ColorTRANSPARENT) {
    // Only use block level color if no custom accent color was set.
    // We don't set accent_color_ here to preserve custom colors.
  }
  // Re-render with updated border.
  SchedulePaint();
}

AstraFocusBlockLevel AstraFocusModeIndicator::GetBlockLevel() const {
  return block_level_;
}

// -- Paused state ----------------------------------------------------------

void AstraFocusModeIndicator::SetPaused(bool paused) {
  if (is_paused_ == paused) {
    return;
  }
  is_paused_ = paused;
  UpdateIndicatorColor();
  UpdateIndicatorText();

  if (paused) {
    StopPulseAnimation();
  } else if (is_active_) {
    StartPulseAnimation();
  }
}

bool AstraFocusModeIndicator::IsPaused() const {
  return is_paused_;
}

// -- Menu on click ---------------------------------------------------------

void AstraFocusModeIndicator::SetShowMenuOnClick(bool show) {
  show_menu_on_click_ = show;
}

bool AstraFocusModeIndicator::GetShowMenuOnClick() const {
  return show_menu_on_click_;
}

// -- Accent color ----------------------------------------------------------

void AstraFocusModeIndicator::SetAccentColor(SkColor color) {
  accent_color_ = color;
  SchedulePaint();
}

SkColor AstraFocusModeIndicator::GetAccentColor() const {
  return accent_color_;
}

// -- Pulse animation -------------------------------------------------------

void AstraFocusModeIndicator::StartPulseAnimation() {
  if (pulse_timer_.IsRunning()) {
    is_pulsing_ = true;
    return;
  }
  pulse_cycle_ = 0;
  pulse_visible_ = true;
  is_pulsing_ = true;
  pulse_timer_.Start(FROM_HERE,
                     base::Milliseconds(kPulseIntervalMs / 2),
                     base::BindRepeating(&AstraFocusModeIndicator::OnPulseTick,
                                         base::Unretained(this)));
}

void AstraFocusModeIndicator::StopPulseAnimation() {
  pulse_timer_.Stop();
  is_pulsing_ = false;
  // Reset to full opacity.
  if (widget_) {
    widget_->SetOpacity(1.0f);
  }
  pulse_visible_ = true;
}

bool AstraFocusModeIndicator::IsPulsing() const {
  return is_pulsing_;
}

// -- Legacy API ------------------------------------------------------------

void AstraFocusModeIndicator::UpdateRemainingTime(base::TimeDelta remaining) {
  SetTimeRemaining(remaining);
}

void AstraFocusModeIndicator::UpdatePausedState(bool is_paused) {
  SetPaused(is_paused);
}

void AstraFocusModeIndicator::UpdatePhase(AstraFocusPhase phase) {
  current_phase_ = phase;
  UpdateIndicatorColor();
}

void AstraFocusModeIndicator::ApplySettings(const AstraFocusModeModel* model) {
  if (!model) {
    return;
  }
  is_active_ = model->IsActive();
  is_paused_ = model->IsSessionPaused();
  timer_visible_ = model->GetShowTimer();
  block_level_ = model->GetBlockLevel();

  UpdateIndicatorSize();
  UpdateIndicatorText();
  UpdateIndicatorColor();
  PositionIndicator();

  // Start/stop pulse based on state.
  if (model->IsActive() && !model->IsSessionPaused()) {
    StartPulseAnimation();
  } else {
    StopPulseAnimation();
  }
}

void AstraFocusModeIndicator::UpdateStats() {
  // Stats are shown in the menu bubble, not the indicator itself.
  // If the menu is open, refresh it.
  if (menu_bubble_) {
    menu_bubble_->UpdateStats();
  }
}

void AstraFocusModeIndicator::Close() {
  CloseMenuBubble();
  StopPulseAnimation();
  if (widget_) {
    widget_->Close();
    // widget_ will be cleared in OnWidgetDestroying.
  }
}

// -- Widget creation -------------------------------------------------------

void AstraFocusModeIndicator::CreateWidget() {
  if (!browser_view_) {
    return;
  }

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.ownership = views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET;

  // Set the preferred size of this view.
  SetPreferredSize(gfx::Size(kIndicatorFullWidth, kIndicatorFullHeight));

  // Position the indicator based on the model's position setting.
  // Default to top-right.
  gfx::Rect browser_bounds = browser_view_->GetBoundsInScreen();
  int x = browser_bounds.right() - kIndicatorOffsetX - kIndicatorFullWidth;
  int y = browser_bounds.y() + kIndicatorOffsetY;
  params.bounds = gfx::Rect(x, y, kIndicatorFullWidth, kIndicatorFullHeight);

  // Use the browser view's native window as the parent.
  params.parent = browser_view_->GetWidget()->GetNativeView();

  // Translucent window so rounded corners look right.
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.delegate = new views::WidgetDelegateView();

  widget_ = new views::Widget();
  widget_->Init(std::move(params));

  // Set this view as the widget's content.
  views::View* contents = widget_->GetContentsView();
  contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  contents->AddChildView(this);

  // Observe the widget so we know when it's destroyed.
  widget_->AddObserver(this);
  observing_widget_ = true;

  // Create the indicator button label.
  indicator_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraFocusModeIndicator::OnIndicatorClicked,
                          base::Unretained(this)),
      u"Focus Mode 25:00"));
  indicator_button_->SetTextColor(views::Button::STATE_NORMAL,
                                  kIndicatorTextColor);
  indicator_button_->SetTextColor(views::Button::STATE_HOVERED,
                                  kIndicatorTextColor);
  indicator_button_->SetTextColor(views::Button::STATE_PRESSED,
                                  kIndicatorTextColor);

  indicator_button_->SetBorder(views::NullBorder());
  indicator_button_->SetFocusForPlatform();
  indicator_button_->SetAccessibleName(u"Focus mode indicator");

  // Show the widget inactive (doesn't steal focus).
  widget_->ShowInactive();

  // Start pulse animation if not paused.
  if (!is_paused_) {
    StartPulseAnimation();
  }

  // Apply initial settings from model if available.
  if (model_) {
    ApplySettings(model_);
  }
}

// -- View overrides --------------------------------------------------------

views::Widget* AstraFocusModeIndicator::GetWidget() {
  return widget_;
}

void AstraFocusModeIndicator::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update colors from color provider.
  //   Chromium owner: ColorProvider
  //   Patch point: Use GetColorProvider() for dynamic theming.
}

bool AstraFocusModeIndicator::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    is_dragging_ = true;
    drag_start_point_ = event.location();
    if (widget_) {
      widget_start_position_ = widget_->GetWindowBoundsInScreen().origin();
    }
    return true;
  }
  return false;
}

bool AstraFocusModeIndicator::OnMouseDragged(const ui::MouseEvent& event) {
  if (!is_dragging_ || !widget_) {
    return false;
  }
  gfx::Vector2d delta = event.location() - drag_start_point_;
  gfx::Point new_position = widget_start_position_ + delta;
  widget_->SetBounds(gfx::Rect(new_position, widget_->GetWindowBoundsInScreen().size()));
  return true;
}

void AstraFocusModeIndicator::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    // If this was just a click (no significant movement), toggle the menu.
    gfx::Vector2d delta = event.location() - drag_start_point_;
    if (std::abs(delta.x()) < 5 && std::abs(delta.y()) < 5) {
      OnIndicatorClicked();
    }
  }
}

void AstraFocusModeIndicator::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(u"Focus mode indicator");
  if (is_paused_) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription,
        "Focus mode is paused");
  } else {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription,
        "Focus mode is active");
  }
}

// -- WidgetObserver --------------------------------------------------------

void AstraFocusModeIndicator::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(widget, widget_);
  widget_->RemoveObserver(this);
  observing_widget_ = false;
  widget_ = nullptr;
}

void AstraFocusModeIndicator::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  // Reposition the indicator if the browser window moves.
  // TODO(astra): Reposition the indicator when the browser window moves or resizes.
  //   Currently, the indicator position can drift if the browser window is moved or resized.
  //   We should listen to the browser widget's bounds changes and reposition.
}

// -- Private helpers -------------------------------------------------------

std::u16string AstraFocusModeIndicator::FormatTime(
    base::TimeDelta delta) const {
  // Format as MM:SS for durations under an hour, or H:MM for longer.
  if (delta < base::Hours(1)) {
    int total_seconds = static_cast<int>(delta.InSeconds());
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    return base::ASCIIToUTF16(base::StringPrintf("%02d:%02d",
                                                 minutes, seconds));
  } else {
    int total_minutes = static_cast<int>(delta.InMinutes());
    int hours = total_minutes / 60;
    int minutes = total_minutes % 60;
    return base::ASCIIToUTF16(base::StringPrintf("%dh %02dm",
                                                 hours, minutes));
  }
}

void AstraFocusModeIndicator::OnIndicatorClicked() {
  if (show_menu_on_click_) {
    ToggleMenuBubble();
  }
}

void AstraFocusModeIndicator::ToggleMenuBubble() {
  if (menu_bubble_) {
    CloseMenuBubble();
  } else {
    ShowMenuBubble();
  }
}

void AstraFocusModeIndicator::ShowMenuBubble() {
  if (menu_bubble_ || !widget_) {
    return;
  }

  size_t distraction_count = 0;
  if (delegate_) {
    distraction_count = delegate_->GetDistractionsBlockedCount();
  }

  menu_bubble_ = AstraFocusModeMenuBubble::Show(
      this, delegate_, model_, remaining_time_, distraction_count,
      is_paused_, current_phase_);
}

void AstraFocusModeIndicator::CloseMenuBubble() {
  if (menu_bubble_) {
    menu_bubble_->Close();
    menu_bubble_ = nullptr;
  }
}

void AstraFocusModeIndicator::OnPulseTick() {
  if (!widget_) {
    return;
  }
  ++pulse_cycle_;
  if (pulse_cycle_ % 2 == 0) {
    widget_->SetOpacity(static_cast<float>(kPulseMinOpacity) / 255.0f);
  } else {
    widget_->SetOpacity(1.0f);
  }
}

void AstraFocusModeIndicator::UpdateIndicatorText() {
  if (!indicator_button_) {
    return;
  }

  if (!model_ || model_->indicator_style() == AstraIndicatorStyle::kMinimal) {
    indicator_button_->SetText(u"");
    return;
  }

  bool show_timer = timer_visible_;
  std::u16string label = u"Focus";

  if (is_paused_) {
    label = u"Paused";
  } else if (current_phase_ == AstraFocusPhase::kShortBreak ||
             current_phase_ == AstraFocusPhase::kLongBreak) {
    label = u"Break";
  }

  if (show_timer && remaining_time_ > base::TimeDelta()) {
    std::u16string time_str = FormatTime(remaining_time_);
    indicator_button_->SetText(label + u" " + time_str);
  } else {
    indicator_button_->SetText(label + u" Mode");
  }
}

void AstraFocusModeIndicator::UpdateIndicatorColor() {
  SkColor bg_color = GetCurrentBackgroundColor();
  SetBackground(views::CreateRoundedRectBackground(
      bg_color, kIndicatorCornerRadius));

  // Apply accent color as border if set, or use block level color.
  SkColor border_color = accent_color_;
  if (border_color == SK_ColorTRANSPARENT) {
    border_color = GetBlockLevelBorderColor(block_level_);
  }

  if (border_color != SK_ColorTRANSPARENT) {
    // Set a border with the accent color.
    SetBorder(views::CreateRoundedRectBorder(
        kAccentBorderThickness, kIndicatorCornerRadius, border_color));
  } else {
    SetBorder(views::NullBorder());
  }

  SchedulePaint();
}

void AstraFocusModeIndicator::UpdateIndicatorSize() {
  if (!widget_ || !model_) {
    return;
  }

  int width = kIndicatorFullWidth;
  int height = kIndicatorFullHeight;

  switch (model_->indicator_style()) {
    case AstraIndicatorStyle::kMinimal:
      width = kIndicatorMinimalSize;
      height = kIndicatorMinimalSize;
      break;
    case AstraIndicatorStyle::kBadge:
      width = kIndicatorBadgeSize;
      height = kIndicatorBadgeSize;
      break;
    case AstraIndicatorStyle::kFull:
      width = kIndicatorFullWidth;
      height = kIndicatorFullHeight;
      break;
  }

  SetPreferredSize(gfx::Size(width, height));

  gfx::Rect current_bounds = widget_->GetWindowBoundsInScreen();
  gfx::Rect new_bounds(current_bounds.origin(), gfx::Size(width, height));
  widget_->SetBounds(new_bounds);
}

void AstraFocusModeIndicator::PositionIndicator() {
  if (!widget_ || !browser_view_ || !model_) {
    return;
  }

  gfx::Rect browser_bounds = browser_view_->GetBoundsInScreen();
  gfx::Size size = widget_->GetWindowBoundsInScreen().size();
  gfx::Point position;

  switch (model_->indicator_position()) {
    case AstraIndicatorPosition::kLeft:
      position.set_x(browser_bounds.x() + kIndicatorOffsetX);
      position.set_y(browser_bounds.y() + kIndicatorOffsetY);
      break;
    case AstraIndicatorPosition::kRight:
      position.set_x(browser_bounds.right() - kIndicatorOffsetX - size.width());
      position.set_y(browser_bounds.y() + kIndicatorOffsetY);
      break;
    case AstraIndicatorPosition::kHidden:
      // Hidden position — don't show.
      widget_->Hide();
      return;
    // Legacy positions.
    case AstraIndicatorPosition::kTopLeft:
      position.set_x(browser_bounds.x() + kIndicatorOffsetX);
      position.set_y(browser_bounds.y() + kIndicatorOffsetY);
      break;
    case AstraIndicatorPosition::kTopRight:
      position.set_x(browser_bounds.right() - kIndicatorOffsetX - size.width());
      position.set_y(browser_bounds.y() + kIndicatorOffsetY);
      break;
    case AstraIndicatorPosition::kBottomLeft:
      position.set_x(browser_bounds.x() + kIndicatorOffsetX);
      position.set_y(browser_bounds.bottom() - kIndicatorOffsetY - size.height());
      break;
    case AstraIndicatorPosition::kBottomRight:
      position.set_x(browser_bounds.right() - kIndicatorOffsetX - size.width());
      position.set_y(browser_bounds.bottom() - kIndicatorOffsetY - size.height());
      break;
  }

  widget_->SetBounds(gfx::Rect(position, size));
  if (!widget_->IsVisible()) {
    widget_->ShowInactive();
  }
}

SkColor AstraFocusModeIndicator::GetCurrentBackgroundColor() const {
  if (is_paused_) {
    return kPausedBackgroundColor;
  }
  if (current_phase_ == AstraFocusPhase::kShortBreak ||
      current_phase_ == AstraFocusPhase::kLongBreak) {
    return kBreakBackgroundColor;
  }
  return kActiveBackgroundColor;
}

}  // namespace astra
