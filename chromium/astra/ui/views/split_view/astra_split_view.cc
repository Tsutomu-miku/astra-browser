#include "astra/ui/views/split_view/astra_split_view.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/i18n/rtl.h"
#include "astra/ui/color/astra_color_ids.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"

namespace astra {

// =========================================================================
// Helper functions for preset ratios
// =========================================================================

float SplitViewPresetToRatio(SplitViewPreset preset) {
  switch (preset) {
    case SplitViewPreset::kFiftyFifty:
      return 0.5f;
    case SplitViewPreset::kSeventyThirty:
      return 0.7f;
    case SplitViewPreset::kThirtySeventy:
      return 0.3f;
    case SplitViewPreset::kSixtyForty:
      return 0.6f;
    case SplitViewPreset::kFortySixty:
      return 0.4f;
  }
  return 0.5f;
}

absl::optional<SplitViewPreset> RatioToSplitViewPreset(
    float ratio, float tolerance) {
  // Check each preset and return the closest one within tolerance.
  absl::optional<SplitViewPreset> best;
  float best_distance = tolerance;

  SplitViewPreset presets[] = {
      SplitViewPreset::kFiftyFifty,
      SplitViewPreset::kSeventyThirty,
      SplitViewPreset::kThirtySeventy,
      SplitViewPreset::kSixtyForty,
      SplitViewPreset::kFortySixty,
  };

  for (SplitViewPreset preset : presets) {
    float preset_ratio = SplitViewPresetToRatio(preset);
    float distance = std::abs(ratio - preset_ratio);
    if (distance < best_distance) {
      best_distance = distance;
      best = preset;
    }
  }

  return best;
}

std::u16string SplitViewPresetToName(SplitViewPreset preset) {
  // TODO(astra): Use localized strings (ui::ResourceBundle).
  // For now, use hardcoded English strings as placeholders.
  switch (preset) {
    case SplitViewPreset::kFiftyFifty:
      return u"50/50 Split";
    case SplitViewPreset::kSeventyThirty:
      return u"70/30 Split";
    case SplitViewPreset::kThirtySeventy:
      return u"30/70 Split";
    case SplitViewPreset::kSixtyForty:
      return u"60/40 Split";
    case SplitViewPreset::kFortySixty:
      return u"40/60 Split";
  }
  return u"Split";
}

// =========================================================================
// Astra-prefixed enum helpers
// =========================================================================

double AstraSplitPresetToRatio(AstraSplitPreset preset) {
  switch (preset) {
    case AstraSplitPreset::kEqual:
      return 0.5;
    case AstraSplitPreset::kPrimaryLarge:
      return 0.7;
    case AstraSplitPreset::kSecondaryLarge:
      return 0.3;
    case AstraSplitPreset::kThreeQuarter:
      return 0.75;
    case AstraSplitPreset::kQuarter:
      return 0.25;
    case AstraSplitPreset::kGoldenRatio:
      return 0.618;  // phi - 1 = ~0.618 (golden ratio conjugate)
  }
  return 0.5;
}

absl::optional<AstraSplitPreset> RatioToAstraSplitPreset(
    double ratio, double tolerance) {
  absl::optional<AstraSplitPreset> best;
  double best_distance = tolerance;

  AstraSplitPreset presets[] = {
      AstraSplitPreset::kEqual,
      AstraSplitPreset::kPrimaryLarge,
      AstraSplitPreset::kSecondaryLarge,
      AstraSplitPreset::kThreeQuarter,
      AstraSplitPreset::kQuarter,
      AstraSplitPreset::kGoldenRatio,
  };

  for (AstraSplitPreset preset : presets) {
    double preset_ratio = AstraSplitPresetToRatio(preset);
    double distance = std::abs(ratio - preset_ratio);
    if (distance < best_distance) {
      best_distance = distance;
      best = preset;
    }
  }

  return best;
}

std::u16string AstraSplitPresetToName(AstraSplitPreset preset) {
  // TODO(astra): Use localized strings (ui::ResourceBundle).
  // For now, use hardcoded English strings as placeholders.
  switch (preset) {
    case AstraSplitPreset::kEqual:
      return u"Equal Split";
    case AstraSplitPreset::kPrimaryLarge:
      return u"Primary Large";
    case AstraSplitPreset::kSecondaryLarge:
      return u"Secondary Large";
    case AstraSplitPreset::kThreeQuarter:
      return u"Three Quarter";
    case AstraSplitPreset::kQuarter:
      return u"Quarter";
    case AstraSplitPreset::kGoldenRatio:
      return u"Golden Ratio";
  }
  return u"Split";
}

SplitViewOrientation ToLegacyOrientation(AstraSplitOrientation orientation) {
  switch (orientation) {
    case AstraSplitOrientation::kHorizontal:
      return SplitViewOrientation::kHorizontal;
    case AstraSplitOrientation::kVertical:
      return SplitViewOrientation::kVertical;
  }
  return SplitViewOrientation::kHorizontal;
}

AstraSplitOrientation FromLegacyOrientation(SplitViewOrientation orientation) {
  switch (orientation) {
    case SplitViewOrientation::kHorizontal:
      return AstraSplitOrientation::kHorizontal;
    case SplitViewOrientation::kVertical:
      return AstraSplitOrientation::kVertical;
  }
  return AstraSplitOrientation::kHorizontal;
}

std::string AstraSplitOrientationToString(AstraSplitOrientation orientation) {
  switch (orientation) {
    case AstraSplitOrientation::kHorizontal:
      return "horizontal";
    case AstraSplitOrientation::kVertical:
      return "vertical";
  }
  return "horizontal";
}

AstraSplitOrientation AstraSplitOrientationFromString(
    const std::string& value) {
  if (value == "vertical") {
    return AstraSplitOrientation::kVertical;
  }
  return AstraSplitOrientation::kHorizontal;
}

std::string AstraResizeModeToString(AstraResizeMode mode) {
  switch (mode) {
    case AstraResizeMode::kFixedRatio:
      return "fixed_ratio";
    case AstraResizeMode::kProportional:
      return "proportional";
    case AstraResizeMode::kMinSizePriority:
      return "min_size_priority";
  }
  return "fixed_ratio";
}

AstraResizeMode AstraResizeModeFromString(const std::string& value) {
  if (value == "proportional") {
    return AstraResizeMode::kProportional;
  }
  if (value == "min_size_priority") {
    return AstraResizeMode::kMinSizePriority;
  }
  return AstraResizeMode::kFixedRatio;
}

std::string AstraSplitPresetToString(AstraSplitPreset preset) {
  switch (preset) {
    case AstraSplitPreset::kEqual:
      return "equal";
    case AstraSplitPreset::kPrimaryLarge:
      return "primary_large";
    case AstraSplitPreset::kSecondaryLarge:
      return "secondary_large";
    case AstraSplitPreset::kThreeQuarter:
      return "three_quarter";
    case AstraSplitPreset::kQuarter:
      return "quarter";
    case AstraSplitPreset::kGoldenRatio:
      return "golden_ratio";
  }
  return "equal";
}

AstraSplitPreset AstraSplitPresetFromString(const std::string& value) {
  if (value == "primary_large") {
    return AstraSplitPreset::kPrimaryLarge;
  }
  if (value == "secondary_large") {
    return AstraSplitPreset::kSecondaryLarge;
  }
  if (value == "three_quarter") {
    return AstraSplitPreset::kThreeQuarter;
  }
  if (value == "quarter") {
    return AstraSplitPreset::kQuarter;
  }
  if (value == "golden_ratio") {
    return AstraSplitPreset::kGoldenRatio;
  }
  return AstraSplitPreset::kEqual;
}

std::string AstraSplitPaneToString(AstraSplitPane pane) {
  switch (pane) {
    case AstraSplitPane::kPrimary:
      return "primary";
    case AstraSplitPane::kSecondary:
      return "secondary";
  }
  return "primary";
}

AstraSplitPane AstraSplitPaneFromString(const std::string& value) {
  if (value == "secondary") {
    return AstraSplitPane::kSecondary;
  }
  return AstraSplitPane::kPrimary;
}

// =========================================================================
// AstraSplitDivider
// =========================================================================

AstraSplitDivider::AstraSplitDivider(SplitViewOrientation orientation)
    : orientation_(orientation) {
  // Make the divider receive mouse and keyboard events.
  SetCanProcessEventsWithinSubtree(true);
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Set an initial accessible name.
  SetAccessibleName(GetAccessibleName());
}

AstraSplitDivider::~AstraSplitDivider() = default;

void AstraSplitDivider::SetOrientation(SplitViewOrientation orientation) {
  if (orientation_ == orientation) {
    return;
  }
  orientation_ = orientation;
  SetAccessibleName(GetAccessibleName());
  SchedulePaint();
}

void AstraSplitDivider::SetDividerVisible(bool visible) {
  if (divider_visible_ == visible) {
    return;
  }
  divider_visible_ = visible;
  SetVisible(visible);
  SchedulePaint();
}

bool AstraSplitDivider::OnMousePressed(const ui::MouseEvent& event) {
  if (!divider_visible_) {
    return false;
  }
  if (event.IsOnlyLeftMouseButton()) {
    is_dragging_ = true;
    split_view_ = static_cast<AstraSplitView*>(parent());
    // Record the offset within the divider where the press happened,
    // so the divider stays under the cursor during drag.
    if (orientation_ == SplitViewOrientation::kHorizontal) {
      drag_offset_ = event.x();
    } else {
      drag_offset_ = event.y();
    }
    return true;
  }
  return false;
}

bool AstraSplitDivider::OnMouseDragged(const ui::MouseEvent& event) {
  if (!is_dragging_ || !split_view_ || !divider_visible_) {
    return false;
  }

  // Convert the event position to the parent (AstraSplitView) coordinate space.
  gfx::Point location = event.location();
  ConvertPointToTarget(this, parent(), &location);

  // Adjust by the drag offset so the divider tracks correctly.
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    location.Offset(-drag_offset_, 0);
  } else {
    location.Offset(0, -drag_offset_);
  }

  // Delegate to the parent split view to update the ratio.
  split_view_->OnDividerDragged(location);

  return true;
}

void AstraSplitDivider::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_ && split_view_) {
    is_dragging_ = false;
    split_view_->OnDividerDragEnded();
    split_view_ = nullptr;
  }
}

void AstraSplitDivider::OnMouseEntered(const ui::MouseEvent& event) {
  if (!divider_visible_) {
    return;
  }
  is_hovered_ = true;
  SchedulePaint();
}

void AstraSplitDivider::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  SchedulePaint();
}

gfx::NativeCursor AstraSplitDivider::GetCursor(const ui::MouseEvent& event) {
  if (!divider_visible_) {
    return ui::mojom::CursorType::kPointer;
  }
  // Use the standard resize cursor appropriate to the orientation.
  // For a horizontal split (side-by-side), the divider is vertical and the
  // cursor should be east-west resize.
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    return ui::mojom::CursorType::kEastWestResize;
  }
  return ui::mojom::CursorType::kNorthSouthResize;
}

void AstraSplitDivider::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  if (!divider_visible_) {
    return;
  }

  // Draw the visual divider line (thinner than the hit target).
  const gfx::Rect bounds = GetLocalBounds();
  const int visual_thickness = kDividerVisualThickness;

  // Use the Astra split view divider color.
  // TODO(astra): Use a slightly brighter color on hover, or a glow effect,
  //   if UX wants it.  For now, the same color is used in both states.
  SkColor divider_color = SK_ColorGRAY;
  if (GetColorProvider()) {
    divider_color = GetColorProvider()->GetColor(kColorAstraSplitViewDivider);
  }

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    // Vertical divider line — draw a 1-DIP line in the center of the hit area.
    int line_x = bounds.x() + bounds.width() / 2 - visual_thickness / 2;
    canvas->FillRect(
        gfx::Rect(line_x, bounds.y(), visual_thickness, bounds.height()),
        divider_color);
  } else {
    // Horizontal divider line.
    int line_y = bounds.y() + bounds.height() / 2 - visual_thickness / 2;
    canvas->FillRect(
        gfx::Rect(bounds.x(), line_y, bounds.width(), visual_thickness),
        divider_color);
  }

  // If focused, draw a focus ring around the divider.
  if (HasFocus()) {
    // TODO(astra): Use views::FocusRing for proper focus ring rendering.
    // For now, draw a simple focus rectangle.
    SkColor focus_color = SK_ColorBLUE;
    canvas->DrawRect(bounds, focus_color);
  }
}

void AstraSplitDivider::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraSplitDivider::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);

  node_data->role = ax::mojom::Role::kSplitter;

  // Set orientation.
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    node_data->AddIntAttribute(
        ax::mojom::IntAttribute::kAriaOrientation,
        static_cast<int>(ax::mojom::Orientation::kVertical));
  } else {
    node_data->AddIntAttribute(
        ax::mojom::IntAttribute::kAriaOrientation,
        static_cast<int>(ax::mojom::Orientation::kHorizontal));
  }

  // Splitter is always focusable via keyboard.
  if (divider_visible_) {
    node_data->AddState(ax::mojom::State::kFocusable);
  } else {
    node_data->AddState(ax::mojom::State::kInvisible);
  }
  if (HasFocus()) {
    node_data->AddState(ax::mojom::State::kFocused);
  }
}

bool AstraSplitDivider::OnKeyPressed(const ui::KeyEvent& event) {
  if (!divider_visible_ ||
      !split_view_->settings().keyboard_navigation_enabled) {
    return views::View::OnKeyPressed(event);
  }

  // Handle arrow keys for keyboard-based resizing.
  int delta = 0;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    // Horizontal split — left/right arrows move the vertical divider.
    if (event.key_code() == ui::VKEY_LEFT) {
      delta = -keyboard_step_size_;
    } else if (event.key_code() == ui::VKEY_RIGHT) {
      delta = keyboard_step_size_;
    }
  } else {
    // Vertical split — up/down arrows move the horizontal divider.
    if (event.key_code() == ui::VKEY_UP) {
      delta = -keyboard_step_size_;
    } else if (event.key_code() == ui::VKEY_DOWN) {
      delta = keyboard_step_size_;
    }
  }

  // Handle Home/End keys to jump to minimum/maximum.
  if (event.key_code() == ui::VKEY_HOME) {
    // Jump to minimum primary pane size.
    delta = -10000;  // Large negative value — clamping will handle it.
  } else if (event.key_code() == ui::VKEY_END) {
    // Jump to maximum primary pane size.
    delta = 10000;  // Large positive value — clamping will handle it.
  }

  // Handle Escape key — reset to default ratio.
  if (event.key_code() == ui::VKEY_ESCAPE) {
    if (split_view_ && split_view_->IsMaximized()) {
      split_view_->Unmaximize();
      return true;
    }
  }

  if (delta != 0) {
    HandleKeyboardResize(delta);
    return true;
  }

  return views::View::OnKeyPressed(event);
}

bool AstraSplitDivider::OnKeyReleased(const ui::KeyEvent& event) {
  // After a keyboard resize, we need to notify observers that the ratio
  // has settled.  We do this on key release so that continuous key repeats
  // generate "changing" notifications and the final release generates
  // a "changed" notification.
  //
  // TODO(astra): This is a simplified approach.  A more robust implementation
  //   would track whether the key press actually caused a resize, and only
  //   notify on release if a resize occurred.
  if (split_view_ && divider_visible_) {
    split_view_->NotifyRatioChanged();
  }
  return views::View::OnKeyReleased(event);
}

void AstraSplitDivider::OnFocus() {
  views::View::OnFocus();
  SchedulePaint();
}

void AstraSplitDivider::OnBlur() {
  views::View::OnBlur();
  SchedulePaint();
}

std::u16string AstraSplitDivider::GetAccessibleName() const {
  // TODO(astra): Use localized strings (ui::ResourceBundle).
  // For now, use hardcoded English strings as placeholders.
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    return u"Split view divider — drag to resize panes horizontally";
  }
  return u"Split view divider — drag to resize panes vertically";
}

void AstraSplitDivider::HandleKeyboardResize(int delta) {
  if (!split_view_) {
    split_view_ = static_cast<AstraSplitView*>(parent());
    if (!split_view_) {
      return;
    }
  }

  // Compute new ratio from current position + delta.
  int current_pos = split_view_->ComputeDividerPosition();
  int new_pos = current_pos + delta;
  float new_ratio = split_view_->ComputeRatioFromPosition(new_pos);
  float clamped_ratio = split_view_->ClampRatio(new_ratio);

  if (clamped_ratio != split_view_->ratio()) {
    split_view_->SetRatio(clamped_ratio, /*animate=*/true);
    split_view_->NotifyRatioChanging();
  }
}

// =========================================================================
// AstraSplitMinimapView
// =========================================================================

AstraSplitMinimapView::AstraSplitMinimapView() = default;

AstraSplitMinimapView::~AstraSplitMinimapView() = default;

void AstraSplitMinimapView::UpdateLayout(float ratio,
                                         SplitViewOrientation orientation) {
  if (ratio_ == ratio && orientation_ == orientation) {
    return;
  }
  ratio_ = ratio;
  orientation_ = orientation;
  SchedulePaint();
}

void AstraSplitMinimapView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetLocalBounds();
  if (bounds.IsEmpty()) {
    return;
  }

  // Background.
  SkColor bg_color = SK_ColorWHITE;
  if (GetColorProvider()) {
    bg_color =
        GetColorProvider()->GetColor(kColorAstraSplitViewBackground);
  }
  canvas->FillRect(bounds, bg_color);

  // Border.
  SkColor border_color = SK_ColorGRAY;
  if (GetColorProvider()) {
    // TODO(astra): Use a dedicated color ID for the minimap border.
    border_color = GetColorProvider()->GetColor(kColorAstraSplitViewDivider);
  }
  canvas->DrawRect(bounds, border_color);

  // Draw the two panes and the divider as a schematic representation.
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    int divider_x = static_cast<int>(bounds.width() * ratio_);
    // Primary pane (left).
    canvas->FillRect(gfx::Rect(1, 1, divider_x - 1, bounds.height() - 2),
                     0xFFE0E0E0);
    // Secondary pane (right).
    canvas->FillRect(
        gfx::Rect(divider_x + 1, 1,
                  bounds.width() - divider_x - 2, bounds.height() - 2),
        0xFFD0D0D0);
    // Divider line.
    canvas->FillRect(
        gfx::Rect(divider_x, 1, 1, bounds.height() - 2),
        border_color);
  } else {
    int divider_y = static_cast<int>(bounds.height() * ratio_);
    // Primary pane (top).
    canvas->FillRect(gfx::Rect(1, 1, bounds.width() - 2, divider_y - 1),
                     0xFFE0E0E0);
    // Secondary pane (bottom).
    canvas->FillRect(
        gfx::Rect(1, divider_y + 1,
                  bounds.width() - 2, bounds.height() - divider_y - 2),
        0xFFD0D0D0);
    // Divider line.
    canvas->FillRect(
        gfx::Rect(1, divider_y, bounds.width() - 2, 1),
        border_color);
  }
}

gfx::Size AstraSplitMinimapView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(AstraSplitView::kMinimapWidth,
                   AstraSplitView::kMinimapHeight);
}

// =========================================================================
// AstraSplitViewButton
// =========================================================================

AstraSplitViewButton::AstraSplitViewButton(PressedCallback callback)
    : ImageButton(std::move(callback)) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetAccessibleName(GetAccessibleName());
  // Make the button have a reasonable touch target size.
  SetMinSize(gfx::Size(28, 28));
}

AstraSplitViewButton::~AstraSplitViewButton() = default;

void AstraSplitViewButton::SetAccessibleName(const std::u16string& name) {
  accessible_name_ = name;
  views::ImageButton::SetAccessibleName(name);
}

void AstraSplitViewButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::ImageButton::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  if (!accessible_name_.empty()) {
    node_data->SetName(accessible_name_);
  }
}

void AstraSplitViewButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  SchedulePaint();
}

// =========================================================================
// AstraSplitCloseButton
// =========================================================================

AstraSplitCloseButton::AstraSplitCloseButton(PressedCallback callback)
    : AstraSplitViewButton(std::move(callback)) {
  // TODO(astra): Use a proper vector icon from ui/gfx/vector_icon.
  //   Chromium owner: ui/gfx/vector_icons/
  //   For now, we paint a simple X icon.
  SetAccessibleName(u"Close pane");
  SetTooltipText(u"Close pane");
}

AstraSplitCloseButton::~AstraSplitCloseButton() = default;

void AstraSplitCloseButton::PaintButtonContents(gfx::Canvas* canvas) {
  // Draw a simple 'X' close icon.
  gfx::Rect bounds = GetLocalBounds();
  int size = std::min(bounds.width(), bounds.height());
  int padding = size / 4;
  int icon_size = size - padding * 2;
  int x = (bounds.width() - icon_size) / 2;
  int y = (bounds.height() - icon_size) / 2;

  SkColor icon_color = SK_ColorGRAY;
  if (GetColorProvider()) {
    // Use the default icon color from the color provider.
    icon_color = GetColorProvider()->GetColor(ui::kColorIcon);
  }

  // Draw two diagonal lines to form an X.
  cc::PaintFlags flags;
  flags.setColor(icon_color);
  flags.setStrokeWidth(2);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  canvas->DrawLine(gfx::Point(x, y),
                   gfx::Point(x + icon_size, y + icon_size), flags);
  canvas->DrawLine(gfx::Point(x + icon_size, y),
                   gfx::Point(x, y + icon_size), flags);
}

// =========================================================================
// AstraSplitSwapButton
// =========================================================================

AstraSplitSwapButton::AstraSplitSwapButton(PressedCallback callback)
    : AstraSplitViewButton(std::move(callback)) {
  SetAccessibleName(u"Swap panes");
  SetTooltipText(u"Swap primary and secondary panes");
}

AstraSplitSwapButton::~AstraSplitSwapButton() = default;

void AstraSplitSwapButton::PaintButtonContents(gfx::Canvas* canvas) {
  // Draw a simple swap icon (two arrows pointing in opposite directions).
  gfx::Rect bounds = GetLocalBounds();
  int size = std::min(bounds.width(), bounds.height());
  int padding = size / 5;
  int icon_size = size - padding * 2;
  int x = (bounds.width() - icon_size) / 2;
  int y = (bounds.height() - icon_size) / 2;

  SkColor icon_color = SK_ColorGRAY;
  if (GetColorProvider()) {
    icon_color = GetColorProvider()->GetColor(ui::kColorIcon);
  }

  cc::PaintFlags flags;
  flags.setColor(icon_color);
  flags.setStrokeWidth(2);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  // Top arrow (pointing right).
  int arrow_top = y + icon_size / 4;
  int arrow_bottom = y + icon_size * 3 / 4;
  int arrow_right = x + icon_size;
  int arrow_left = x;

  // Top line (rightward arrow).
  canvas->DrawLine(gfx::Point(arrow_left, arrow_top),
                   gfx::Point(arrow_right - 4, arrow_top), flags);
  // Arrowhead for top line.
  canvas->DrawLine(gfx::Point(arrow_right - 4, arrow_top),
                   gfx::Point(arrow_right - 8, arrow_top - 4), flags);
  canvas->DrawLine(gfx::Point(arrow_right - 4, arrow_top),
                   gfx::Point(arrow_right - 8, arrow_top + 4), flags);

  // Bottom line (leftward arrow).
  canvas->DrawLine(gfx::Point(arrow_right, arrow_bottom),
                   gfx::Point(arrow_left + 4, arrow_bottom), flags);
  // Arrowhead for bottom line.
  canvas->DrawLine(gfx::Point(arrow_left + 4, arrow_bottom),
                   gfx::Point(arrow_left + 8, arrow_bottom - 4), flags);
  canvas->DrawLine(gfx::Point(arrow_left + 4, arrow_bottom),
                   gfx::Point(arrow_left + 8, arrow_bottom + 4), flags);
}

// =========================================================================
// AstraSplitLayoutToggleButton
// =========================================================================

AstraSplitLayoutToggleButton::AstraSplitLayoutToggleButton(
    PressedCallback callback)
    : AstraSplitViewButton(std::move(callback)) {
  SetAccessibleName(u"Toggle split layout");
  SetTooltipText(u"Toggle between horizontal and vertical split");
}

AstraSplitLayoutToggleButton::~AstraSplitLayoutToggleButton() = default;

void AstraSplitLayoutToggleButton::SetLayoutMode(AstraSplitLayoutMode mode) {
  if (layout_mode_ == mode) {
    return;
  }
  layout_mode_ = mode;
  SchedulePaint();

  // Update accessible name and tooltip based on mode.
  // TODO(astra): Use localized strings.
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      SetAccessibleName(u"Switch to vertical split");
      SetTooltipText(u"Switch to vertical split");
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      SetAccessibleName(u"Switch to horizontal split");
      SetTooltipText(u"Switch to horizontal split");
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      SetAccessibleName(u"Switch to three-pane vertical split");
      SetTooltipText(u"Switch to three-pane vertical split");
      break;
    case AstraSplitLayoutMode::kThreePaneVertical:
      SetAccessibleName(u"Switch to three-pane horizontal split");
      SetTooltipText(u"Switch to three-pane horizontal split");
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      SetAccessibleName(u"Next layout mode");
      SetTooltipText(u"Next layout mode");
      break;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      SetAccessibleName(u"Next layout mode");
      SetTooltipText(u"Next layout mode");
      break;
    case AstraSplitLayoutMode::kPictureInPicture:
      SetAccessibleName(u"Picture in picture mode");
      SetTooltipText(u"Picture in picture mode");
      break;
    case AstraSplitLayoutMode::kTabShift:
      SetAccessibleName(u"Tab shift mode");
      SetTooltipText(u"Tab shift mode");
      break;
  }
}

void AstraSplitLayoutToggleButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();
  int size = std::min(bounds.width(), bounds.height());
  int padding = size / 5;
  int icon_size = size - padding * 2;
  int x = (bounds.width() - icon_size) / 2;
  int y = (bounds.height() - icon_size) / 2;

  SkColor icon_color = SK_ColorGRAY;
  SkColor divider_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    icon_color = GetColorProvider()->GetColor(ui::kColorIcon);
    divider_color = GetColorProvider()->GetColor(ui::kColorSeparator);
  }

  // Draw the layout icon based on the current mode.
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kGridThreeByTwo:
    case AstraSplitLayoutMode::kPictureInPicture:
    case AstraSplitLayoutMode::kTabShift: {
      // Two rectangles side by side (horizontal split indicator).
      int half = icon_size / 2;
      // Left pane.
      canvas->FillRect(gfx::Rect(x, y, half - 1, icon_size), icon_color);
      // Right pane.
      canvas->FillRect(gfx::Rect(x + half + 1, y, icon_size - half - 1,
                       icon_size), icon_color);
      break;
    }
    case AstraSplitLayoutMode::kTwoPaneVertical:
    case AstraSplitLayoutMode::kThreePaneVertical:
    case AstraSplitLayoutMode::kGridTwoByTwo: {
      // Two rectangles stacked (vertical split indicator).
      int half = icon_size / 2;
      // Top pane.
      canvas->FillRect(gfx::Rect(x, y, icon_size, half - 1), icon_color);
      // Bottom pane.
      canvas->FillRect(gfx::Rect(x, y + half + 1, icon_size,
                       icon_size - half - 1), icon_color);
      break;
    }
  }
}

// =========================================================================
// AstraSplitPaneHeader
// =========================================================================

AstraSplitPaneHeader::AstraSplitPaneHeader() {
  SetPaintToLayer();

  // Create the close button (hidden by default).
  close_button_ = AddChildView(std::make_unique<AstraSplitCloseButton>(
      base::BindRepeating(&AstraSplitPaneHeader::OnCloseButtonPressed,
                          base::Unretained(this))));
  close_button_->SetPane(pane_);
  close_button_->SetVisible(false);
}

AstraSplitPaneHeader::~AstraSplitPaneHeader() = default;

void AstraSplitPaneHeader::SetTitle(const std::u16string& title) {
  if (title_ == title) {
    return;
  }
  title_ = title;
  SchedulePaint();
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AstraSplitPaneHeader::SetShowCloseButton(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  if (close_button_) {
    close_button_->SetVisible(show);
  }
  Layout();
}

void AstraSplitPaneHeader::Layout() {
  gfx::Rect bounds = GetLocalBounds();
  const int kCloseButtonSize = 24;
  const int kCloseButtonMargin = 4;

  // Position the close button on the right side.
  if (close_button_ && show_close_button_) {
    int close_x = bounds.width() - kCloseButtonSize - kCloseButtonMargin;
    int close_y = (bounds.height() - kCloseButtonSize) / 2;
    close_button_->SetBounds(close_x, close_y, kCloseButtonSize,
                             kCloseButtonSize);
  }
}

gfx::Size AstraSplitPaneHeader::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Default height of 28 DIPs for the header bar.
  return gfx::Size(0, 28);
}

void AstraSplitPaneHeader::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetLocalBounds();

  // Draw background.
  SkColor bg_color = SK_ColorWHITE;
  if (GetColorProvider()) {
    bg_color = GetColorProvider()->GetColor(ui::kColorTabBackground);
  }
  canvas->FillRect(bounds, bg_color);

  // Draw bottom border.
  SkColor border_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    border_color = GetColorProvider()->GetColor(ui::kColorSeparator);
  }
  canvas->FillRect(gfx::Rect(0, bounds.height() - 1, bounds.width(), 1),
                   border_color);

  // Draw title text (if we have a title).
  if (!title_.empty()) {
    // TODO(astra): Use proper text rendering with gfx::Canvas::DrawStringRect
    //   or views::Label.  For now, we skip text rendering and rely on
    //   accessibility for the title.
    //
    // Chromium owner: ui/gfx/canvas.h — DrawStringRect.
    //   views::Label for proper label rendering.
  }
}

void AstraSplitPaneHeader::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kGrouping;
  if (!title_.empty()) {
    node_data->SetName(title_);
  } else {
    // Default accessible name based on pane position.
    node_data->SetName(pane_ == AstraSplitPane::kPrimary
                           ? u"Primary pane header"
                           : u"Secondary pane header");
  }
}

void AstraSplitPaneHeader::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraSplitPaneHeader::OnCloseButtonPressed() {
  if (close_callback_) {
    close_callback_.Run();
  }
}

// =========================================================================
// AstraSplitDividerToolbar
// =========================================================================

AstraSplitDividerToolbar::AstraSplitDividerToolbar() {
  SetPaintToLayer();

  // Create swap button.
  swap_button_ = AddChildView(std::make_unique<AstraSplitSwapButton>(
      base::BindRepeating([]() {
        // Callback is set later via SetSwapCallback.
      })));

  // Create layout toggle button.
  layout_toggle_button_ =
      AddChildView(std::make_unique<AstraSplitLayoutToggleButton>(
          base::BindRepeating([]() {
            // Callback is set later via SetLayoutToggleCallback.
          })));
}

AstraSplitDividerToolbar::~AstraSplitDividerToolbar() = default;

void AstraSplitDividerToolbar::SetSwapCallback(base::RepeatingClosure callback) {
  if (swap_button_) {
    swap_button_->SetCallback(
        base::BindRepeating([](base::RepeatingClosure cb, const ui::Event&) {
          if (cb) cb.Run();
        }, callback));
  }
}

void AstraSplitDividerToolbar::SetLayoutToggleCallback(
    base::RepeatingClosure callback) {
  if (layout_toggle_button_) {
    layout_toggle_button_->SetCallback(
        base::BindRepeating([](base::RepeatingClosure cb, const ui::Event&) {
          if (cb) cb.Run();
        }, callback));
  }
}

void AstraSplitDividerToolbar::UpdateLayoutMode(AstraSplitLayoutMode mode) {
  if (layout_toggle_button_) {
    layout_toggle_button_->SetLayoutMode(mode);
  }
}

void AstraSplitDividerToolbar::SetToolbarVisible(bool visible) {
  if (toolbar_visible_ == visible) {
    return;
  }
  toolbar_visible_ = visible;
  SetVisible(visible);
  SchedulePaint();
}

void AstraSplitDividerToolbar::Layout() {
  gfx::Rect bounds = GetLocalBounds();
  const int kButtonSize = 24;
  const int kButtonSpacing = 4;

  int total_width = kButtonSize * 2 + kButtonSpacing;
  int start_x = (bounds.width() - total_width) / 2;
  int y = (bounds.height() - kButtonSize) / 2;

  if (swap_button_) {
    swap_button_->SetBounds(start_x, y, kButtonSize, kButtonSize);
  }

  if (layout_toggle_button_) {
    layout_toggle_button_->SetBounds(start_x + kButtonSize + kButtonSpacing,
                                      y, kButtonSize, kButtonSize);
  }
}

gfx::Size AstraSplitDividerToolbar::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  const int kButtonSize = 24;
  const int kButtonSpacing = 4;
  const int kToolbarPadding = 4;
  return gfx::Size(kButtonSize * 2 + kButtonSpacing + kToolbarPadding * 2,
                   kButtonSize + kToolbarPadding * 2);
}

void AstraSplitDividerToolbar::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetLocalBounds();

  // Draw background (rounded rectangle).
  SkColor bg_color = SK_ColorWHITE;
  if (GetColorProvider()) {
    bg_color = GetColorProvider()->GetColor(ui::kColorDialogBackground);
  }

  // Draw a simple rounded background.
  // TODO(astra): Use views::Background or a proper rounded rect painter.
  //   Chromium owner: ui/views/background.h and ui/gfx/canvas_skia.h.
  canvas->FillRect(bounds, bg_color);

  // Draw border.
  SkColor border_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    border_color = GetColorProvider()->GetColor(ui::kColorSeparator);
  }
  canvas->DrawRect(bounds, border_color);
}

void AstraSplitDividerToolbar::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kToolbar;
  node_data->SetName(u"Split view toolbar");
}

void AstraSplitDividerToolbar::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// =========================================================================
// AstraSplitEmptyPaneView
// =========================================================================

AstraSplitEmptyPaneView::AstraSplitEmptyPaneView(
    base::RepeatingClosure open_tab_callback)
    : open_tab_callback_(std::move(open_tab_callback)) {
  SetPaintToLayer();

  // Create the "Open tab" button.
  open_tab_button_ =
      AddChildView(std::make_unique<AstraSplitViewButton>(base::BindRepeating(
          &AstraSplitEmptyPaneView::OnOpenTabButtonPressed,
          base::Unretained(this))));
  open_tab_button_->SetAccessibleName(u"Open tab in this pane");
  open_tab_button_->SetTooltipText(u"Open a new tab in this pane");
}

AstraSplitEmptyPaneView::~AstraSplitEmptyPaneView() = default;

void AstraSplitEmptyPaneView::SetMessage(const std::u16string& message) {
  if (message_ == message) {
    return;
  }
  message_ = message;
  SchedulePaint();
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AstraSplitEmptyPaneView::SetButtonLabel(const std::u16string& label) {
  if (open_tab_button_) {
    open_tab_button_->SetAccessibleName(label);
    open_tab_button_->SetTooltipText(label);
  }
}

void AstraSplitEmptyPaneView::SetButtonVisible(bool visible) {
  if (button_visible_ == visible) {
    return;
  }
  button_visible_ = visible;
  if (open_tab_button_) {
    open_tab_button_->SetVisible(visible);
  }
  Layout();
}

void AstraSplitEmptyPaneView::Layout() {
  gfx::Rect bounds = GetLocalBounds();
  const int kButtonSize = 36;
  const int kButtonOffsetY = 40;  // Below the message text area.

  if (open_tab_button_ && button_visible_) {
    int x = (bounds.width() - kButtonSize) / 2;
    int y = bounds.height() / 2 + kButtonOffsetY;
    open_tab_button_->SetBounds(x, y, kButtonSize, kButtonSize);
  }
}

gfx::Size AstraSplitEmptyPaneView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(200, 150);
}

void AstraSplitEmptyPaneView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetLocalBounds();

  // Draw background.
  SkColor bg_color = SK_ColorWHITE;
  if (GetColorProvider()) {
    bg_color = GetColorProvider()->GetColor(ui::kColorDialogBackground);
  }
  canvas->FillRect(bounds, bg_color);

  // Draw border.
  SkColor border_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    border_color = GetColorProvider()->GetColor(ui::kColorSeparator);
  }
  canvas->DrawRect(bounds, border_color);

  // Draw message text (simple placeholder - real implementation uses
  // gfx::Canvas::DrawStringRect or a views::Label).
  // TODO(astra): Use proper text rendering.
  //   Chromium owner: ui/gfx/canvas.h — DrawStringRect.
  //   For now, we just show a dashed border pattern to indicate empty state.
  //
  // Draw dashed inner border to indicate empty state.
  cc::PaintFlags flags;
  flags.setColor(border_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1);
  flags.setAntiAlias(true);
  // Dashed effect via multiple small rectangles (simplified).
  // Real implementation would use SkDashPathEffect.

  // Draw a simple icon placeholder (centered square with plus sign feel).
  const int kIconSize = 32;
  int icon_x = (bounds.width() - kIconSize) / 2;
  int icon_y = bounds.height() / 2 - kIconSize / 2 - 20;
  gfx::Rect icon_rect(icon_x, icon_y, kIconSize, kIconSize);
  canvas->DrawRect(icon_rect, border_color);

  // Draw plus sign inside the icon.
  int center_x = bounds.width() / 2;
  int center_y = bounds.height() / 2 - 20;
  const int kPlusSize = 12;
  canvas->DrawLine(
      gfx::Point(center_x - kPlusSize / 2, center_y),
      gfx::Point(center_x + kPlusSize / 2, center_y),
      flags);
  canvas->DrawLine(
      gfx::Point(center_x, center_y - kPlusSize / 2),
      gfx::Point(center_x, center_y + kPlusSize / 2),
      flags);
}

void AstraSplitEmptyPaneView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kGrouping;
  if (!message_.empty()) {
    node_data->SetName(message_);
  } else {
    node_data->SetName(u"Empty pane");
  }
}

void AstraSplitEmptyPaneView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraSplitEmptyPaneView::OnOpenTabButtonPressed() {
  if (open_tab_callback_) {
    open_tab_callback_.Run();
  }
}

// =========================================================================
// AstraSplitDropIndicator
// =========================================================================

AstraSplitDropIndicator::AstraSplitDropIndicator() {
  SetPaintToLayer();
  SetVisible(false);
}

AstraSplitDropIndicator::~AstraSplitDropIndicator() = default;

void AstraSplitDropIndicator::ShowForPane(const gfx::Rect& pane_bounds) {
  is_visible_ = true;
  SetVisible(true);
  SetBounds(pane_bounds.x(), pane_bounds.y(),
            pane_bounds.width(), pane_bounds.height());
  SchedulePaint();
}

void AstraSplitDropIndicator::Hide() {
  if (!is_visible_) {
    return;
  }
  is_visible_ = false;
  SetVisible(false);
}

void AstraSplitDropIndicator::SetDropValid(bool valid) {
  if (drop_valid_ == valid) {
    return;
  }
  drop_valid_ = valid;
  SchedulePaint();
}

void AstraSplitDropIndicator::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  if (!is_visible_) {
    return;
  }

  gfx::Rect bounds = GetLocalBounds();

  // Use a semi-transparent highlight color.
  SkColor highlight_color = drop_valid_ ? 0x334285F4 : 0x33D93025;
  if (GetColorProvider()) {
    // TODO(astra): Use proper color IDs for drop indicator.
    if (drop_valid_) {
      highlight_color = GetColorProvider()->GetColor(
          ui::kColorFocusableBorderFocused);
      // Make it semi-transparent.
      highlight_color = SkColorSetA(highlight_color, 0x33);
    } else {
      highlight_color = SkColorSetRGB(0xD9, 0x30, 0x25);
      highlight_color = SkColorSetA(highlight_color, 0x33);
    }
  }

  // Fill with highlight.
  canvas->FillRect(bounds, highlight_color);

  // Draw border.
  SkColor border_color = drop_valid_ ? 0xFF4285F4 : 0xFFD93025;
  if (GetColorProvider()) {
    if (drop_valid_) {
      border_color = GetColorProvider()->GetColor(
          ui::kColorFocusableBorderFocused);
    } else {
      border_color = SkColorSetRGB(0xD9, 0x30, 0x25);
    }
  }

  cc::PaintFlags flags;
  flags.setColor(border_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);
  canvas->DrawRect(gfx::Rect(1, 1, bounds.width() - 2, bounds.height() - 2),
                   flags);
}

void AstraSplitDropIndicator::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kGrouping;
  node_data->SetName(drop_valid_ ? u"Valid drop target" : u"Invalid drop target");
}

void AstraSplitDropIndicator::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// =========================================================================
// AstraSplitView
// =========================================================================

AstraSplitView::AstraSplitView() {
  // Set the background color from the Astra color system.
  // TODO(astra): Use SetBackground with a ColorProvider-based background
  //   for proper automatic theming.  For now, we paint the background
  //   manually in OnPaint or rely on the parent's background.
  SetPaintToLayer();

  // Create the divider and add it as a child.
  divider_ = AddChildView(std::make_unique<AstraSplitDivider>(orientation_));

  // Create the minimap (hidden by default).
  minimap_ = AddChildView(std::make_unique<AstraSplitMinimapView>());
  minimap_->SetVisible(false);

  // Initialize default snap points.
  ResetSnapPointsToDefaults();
}

AstraSplitView::~AstraSplitView() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewDestroyed();
  }
}

// =========================================================================
// Observer management
// =========================================================================

void AstraSplitView::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraSplitView::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Legacy child view helpers
// =========================================================================

void AstraSplitView::ReplacePrimaryView(views::View* new_view) {
  SetPrimaryView(new_view);
  NotifyViewReplaced(/*is_primary=*/true);
}

void AstraSplitView::ReplaceSecondaryView(views::View* new_view) {
  SetSecondaryView(new_view);
  NotifyViewReplaced(/*is_primary=*/false);
}

// =========================================================================
// Ratio and orientation
// =========================================================================

void AstraSplitView::SetRatio(float ratio, bool animate) {
  float clamped = ClampRatio(ratio,
      (orientation_ == SplitViewOrientation::kHorizontal)
          ? width()
          : height());

  if (ratio_ == clamped) {
    return;
  }

  // Snap to preset if enabled.
  if (settings_.snap_to_presets) {
    clamped = MaybeSnapToPreset(clamped);
  }

  if (ratio_ == clamped) {
    return;
  }

  if (animate && divider_ && layer()) {
    // Animate the divider position.
    // TODO(astra): Use views::AnimationBuilder for a cleaner animation
    //   that also animates the child view bounds.  For now, we just
    //   animate the ratio change and let Layout() handle positioning.
    //
    // For a smooth animation, we should animate the divider's layer
    // and the two pane views' bounds simultaneously.  A simpler approach
    // is to just invalidate layout on each animation frame.
    //
    // For the skeleton implementation, we just change the ratio immediately.
    // Animation is left as a TODO(astra) for the polish phase.
    //
    // TODO(astra): Implement proper layer-based divider animation.
    //   Chromium pattern: views::AnimationBuilder + layer transforms
    //   or bounds animations.  Reference: ui/views/animation/animation_builder.h.
    ratio_ = clamped;
    InvalidateLayout();
    UpdateMinimap();
  } else {
    ratio_ = clamped;
    InvalidateLayout();
    UpdateMinimap();
  }
}

void AstraSplitView::SetPresetRatio(SplitViewPreset preset, bool animate) {
  float preset_ratio = SplitViewPresetToRatio(preset);
  SetRatio(preset_ratio, animate);
}

absl::optional<SplitViewPreset> AstraSplitView::GetCurrentPreset(
    float tolerance) const {
  return RatioToSplitViewPreset(ratio_, tolerance);
}

void AstraSplitView::SetOrientation(SplitViewOrientation orientation) {
  if (orientation_ == orientation) {
    return;
  }
  orientation_ = orientation;
  divider_->SetOrientation(orientation);
  InvalidateLayout();
  SchedulePaint();
  UpdateMinimap();
  NotifyOrientationChanged();
}

void AstraSplitView::ToggleOrientation() {
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    SetOrientation(SplitViewOrientation::kVertical);
  } else {
    SetOrientation(SplitViewOrientation::kHorizontal);
  }
}

void AstraSplitView::SwapViews() {
  // Just swap the pointers — Layout() positions them by role, not by index.
  std::swap(primary_view_, secondary_view_);
  // The ratio stays the same; the "primary side" still gets ratio_ space.
  // Conceptually the user sees the two views swap positions.
  InvalidateLayout();
  NotifyViewsSwapped();
}

void AstraSplitView::MaximizePane(bool primary) {
  if (is_maximized_ && primary_maximized_ == primary) {
    return;
  }

  // Save the current ratio before maximizing so we can restore it.
  if (!is_maximized_) {
    pre_maximize_ratio_ = ratio_;
  }

  is_maximized_ = true;
  primary_maximized_ = primary;

  // Set the ratio to maximize the requested pane.
  // Primary maximized = ratio is as large as possible (close to 1.0).
  // Secondary maximized = ratio is as small as possible (close to 0.0).
  float max_ratio = primary ? (1.0f - kMinPaneRatio) : kMinPaneRatio;
  SetRatio(max_ratio, /*animate=*/true);

  NotifyMaximized(primary);
}

void AstraSplitView::Unmaximize() {
  if (!is_maximized_) {
    return;
  }

  is_maximized_ = false;
  float restore_ratio = pre_maximize_ratio_;
  SetRatio(restore_ratio, /*animate=*/true);

  NotifyUnmaximized();
}

// =========================================================================
// Settings
// =========================================================================

void AstraSplitView::ApplySettings(const AstraSplitViewSettings& settings) {
  settings_ = settings;

  // Apply divider visibility.
  divider_->SetDividerVisible(settings.divider_visible);

  // Apply keyboard navigation step size? No, that's a separate property.

  // Update minimap visibility setting (but only if the user explicitly
  // toggled the minimap on).  The runtime visibility is controlled by
  // SetMinimapVisible.
  if (!settings.minimap_enabled && minimap_visible_) {
    SetMinimapVisible(false);
  }

  InvalidateLayout();
  NotifySettingsChanged();
}

// =========================================================================
// Minimap
// =========================================================================

void AstraSplitView::SetMinimapVisible(bool visible) {
  if (minimap_visible_ == visible) {
    return;
  }

  if (visible && !settings_.minimap_enabled) {
    // Can't show minimap if it's disabled in settings.
    return;
  }

  minimap_visible_ = visible;
  if (minimap_) {
    minimap_->SetVisible(visible);
    if (visible) {
      UpdateMinimap();
      LayoutMinimap();
    }
  }
}

// =========================================================================
// Extended API implementations (Astra naming convention)
// =========================================================================

void AstraSplitView::SetRatio(double ratio) {
  SetRatio(static_cast<float>(ratio), /*animate=*/false);
}

double AstraSplitView::GetRatio() const {
  return ratio_;
}

void AstraSplitView::SetOrientation(AstraSplitOrientation orientation) {
  SetOrientation(ToLegacyOrientation(orientation));
}

AstraSplitOrientation AstraSplitView::GetOrientation() const {
  return FromLegacyOrientation(orientation_);
}

void AstraSplitView::SetDividerWidth(int width) {
  if (divider_width_ == width) {
    return;
  }
  divider_width_ = width;
  InvalidateLayout();
  SchedulePaint();
}

int AstraSplitView::GetDividerWidth() const {
  return divider_width_;
}

void AstraSplitView::SetShowHandle(bool show) {
  if (show_handle_ == show) {
    return;
  }
  show_handle_ = show;
  divider_->SetDividerVisible(show);
  SchedulePaint();
}

bool AstraSplitView::GetShowHandle() const {
  return show_handle_;
}

void AstraSplitView::SetPrimaryView(views::View* view) {
  // This is the implementation of the extended API SetPrimaryView.
  // It delegates to the internal logic.
  if (primary_view_ == view) {
    return;
  }

  // Remove the old primary view if it exists.
  if (primary_view_ && GetIndexOf(primary_view_).has_value()) {
    RemoveChildViewT(primary_view_);
  }

  primary_view_ = view;

  if (primary_view_) {
    // Insert at the beginning (before divider).
    AddChildView(primary_view_);
    // Move the primary view to index 0 so the visual order is:
    // primary, divider, secondary.
    ReorderChildView(primary_view_, 0);
  }

  InvalidateLayout();
}

void AstraSplitView::SetSecondaryView(views::View* view) {
  if (secondary_view_ == view) {
    return;
  }

  // Remove the old secondary view if it exists.
  if (secondary_view_ && GetIndexOf(secondary_view_).has_value()) {
    RemoveChildViewT(secondary_view_);
  }

  secondary_view_ = view;

  if (secondary_view_) {
    // Add at the end (after divider).
    AddChildView(secondary_view_);
  }

  InvalidateLayout();
}

views::View* AstraSplitView::GetPrimaryView() {
  return primary_view_;
}

const views::View* AstraSplitView::GetPrimaryView() const {
  return primary_view_;
}

views::View* AstraSplitView::GetSecondaryView() {
  return secondary_view_;
}

const views::View* AstraSplitView::GetSecondaryView() const {
  return secondary_view_;
}

void AstraSplitView::SetDividerPosition(int position) {
  gfx::Rect bounds = GetLocalBounds();
  int total_size =
      (orientation_ == SplitViewOrientation::kHorizontal) ? bounds.width()
                                                           : bounds.height();
  if (total_size <= 0) {
    return;
  }
  double new_ratio = static_cast<double>(position) / total_size;
  SetRatio(new_ratio, /*animate=*/false);
}

int AstraSplitView::GetDividerPosition() const {
  return ComputeDividerPosition();
}

void AstraSplitView::SetFocusedPane(AstraSplitPane pane) {
  if (focused_pane_ == pane) {
    return;
  }
  focused_pane_ = pane;
  // TODO(astra): Add visual focus indicator on the active pane border.
  //   Chromium pattern: views::FocusRing or custom border painting.
  SchedulePaint();
}

AstraSplitPane AstraSplitView::GetFocusedPane() const {
  return focused_pane_;
}

void AstraSplitView::SetPaneLabels(const std::u16string& primary_label,
                                   const std::u16string& secondary_label) {
  if (primary_label_ == primary_label && secondary_label_ == secondary_label) {
    return;
  }
  primary_label_ = primary_label;
  secondary_label_ = secondary_label;
  if (show_pane_labels_) {
    SchedulePaint();
  }
}

void AstraSplitView::ShowPaneLabels(bool show) {
  if (show_pane_labels_ == show) {
    return;
  }
  show_pane_labels_ = show;
  SchedulePaint();
}

bool AstraSplitView::IsDraggingDivider() const {
  return is_dragging_;
}

// =========================================================================
// Layout modes and multi-pane support
// =========================================================================

void AstraSplitView::SetLayoutMode(AstraSplitLayoutMode mode) {
  if (layout_mode_ == mode) {
    return;
  }

  layout_mode_ = mode;

  // Map layout mode to orientation and ratio for 2-pane compatibility.
  // TODO(astra): Full multi-pane support requires multiple dividers and
  //   a grid layout manager.  For now, we adapt the 2-pane layout to
  //   reflect the mode for presentation purposes.
  //   Chromium owner: views::GridLayout (ui/views/layout/grid_layout.h)
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kGridTwoByTwo:
    case AstraSplitLayoutMode::kGridThreeByTwo:
    case AstraSplitLayoutMode::kPictureInPicture:
    case AstraSplitLayoutMode::kTabShift:
      SetOrientation(SplitViewOrientation::kHorizontal);
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
    case AstraSplitLayoutMode::kThreePaneVertical:
      SetOrientation(SplitViewOrientation::kVertical);
      break;
  }

  // Update the divider toolbar layout toggle button.
  if (divider_toolbar_) {
    divider_toolbar_->UpdateLayoutMode(mode);
  }

  InvalidateLayout();
}

int AstraSplitView::GetPaneCount() const {
  // TODO(astra): Return actual pane count based on layout mode.
  //   For 2-pane modes: 2 panes.
  //   For 3-pane modes: 3 panes (not yet implemented).
  //   For grid modes: columns * rows panes (not yet implemented).
  //   Currently we support 2 panes in all modes for the skeleton.
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
    case AstraSplitLayoutMode::kTwoPaneVertical:
      return 2;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneVertical:
      // TODO(astra): Implement 3-pane layout.
      return 2;  // Still 2 implemented panes
    case AstraSplitLayoutMode::kGridTwoByTwo:
      // TODO(astra): Implement 2x2 grid layout.
      return 2;  // Still 2 implemented panes
    case AstraSplitLayoutMode::kGridThreeByTwo:
      // TODO(astra): Implement 3x2 grid layout.
      return 2;  // Still 2 implemented panes
  }
  return 2;
}

bool AstraSplitView::IsGridLayout() const {
  return layout_mode_ == AstraSplitLayoutMode::kGridTwoByTwo ||
         layout_mode_ == AstraSplitLayoutMode::kGridThreeByTwo;
}

void AstraSplitView::CycleNextLayoutMode() {
  // Cycle through all layout modes in order.
  AstraSplitLayoutMode next;
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      next = AstraSplitLayoutMode::kTwoPaneVertical;
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      next = AstraSplitLayoutMode::kThreePaneHorizontal;
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      next = AstraSplitLayoutMode::kThreePaneVertical;
      break;
    case AstraSplitLayoutMode::kThreePaneVertical:
      next = AstraSplitLayoutMode::kGridTwoByTwo;
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      next = AstraSplitLayoutMode::kGridThreeByTwo;
      break;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      next = AstraSplitLayoutMode::kPictureInPicture;
      break;
    case AstraSplitLayoutMode::kPictureInPicture:
      next = AstraSplitLayoutMode::kTabShift;
      break;
    case AstraSplitLayoutMode::kTabShift:
      next = AstraSplitLayoutMode::kTwoPaneHorizontal;
      break;
  }
  SetLayoutMode(next);
}

void AstraSplitView::CyclePreviousLayoutMode() {
  // Cycle backwards through layout modes.
  AstraSplitLayoutMode prev;
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      prev = AstraSplitLayoutMode::kTabShift;
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      prev = AstraSplitLayoutMode::kTwoPaneHorizontal;
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      prev = AstraSplitLayoutMode::kTwoPaneVertical;
      break;
    case AstraSplitLayoutMode::kThreePaneVertical:
      prev = AstraSplitLayoutMode::kThreePaneHorizontal;
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      prev = AstraSplitLayoutMode::kThreePaneVertical;
      break;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      prev = AstraSplitLayoutMode::kGridTwoByTwo;
      break;
    case AstraSplitLayoutMode::kPictureInPicture:
      prev = AstraSplitLayoutMode::kGridThreeByTwo;
      break;
    case AstraSplitLayoutMode::kTabShift:
      prev = AstraSplitLayoutMode::kPictureInPicture;
      break;
  }
  SetLayoutMode(prev);
}

// =========================================================================
// Pane headers and control buttons
// =========================================================================

void AstraSplitView::SetShowPaneHeaders(bool show) {
  if (show_pane_headers_ == show) {
    return;
  }
  show_pane_headers_ = show;
  UpdatePaneHeaders();
  InvalidateLayout();
}

void AstraSplitView::SetPaneTitle(AstraSplitPane pane,
                                  const std::u16string& title) {
  if (pane == AstraSplitPane::kPrimary && primary_header_) {
    primary_header_->SetTitle(title);
    primary_label_ = title;
  } else if (pane == AstraSplitPane::kSecondary && secondary_header_) {
    secondary_header_->SetTitle(title);
    secondary_label_ = title;
  }
}

void AstraSplitView::SetShowDividerToolbar(bool show) {
  if (show_divider_toolbar_ == show) {
    return;
  }
  show_divider_toolbar_ = show;
  UpdateDividerToolbar();
  InvalidateLayout();
}

void AstraSplitView::UpdatePaneHeaders() {
  if (show_pane_headers_) {
    // Create primary header if needed.
    if (!primary_header_) {
      primary_header_ = AddChildView(std::make_unique<AstraSplitPaneHeader>());
      primary_header_->SetPane(AstraSplitPane::kPrimary);
      primary_header_->SetTitle(primary_label_);
      primary_header_->SetShowCloseButton(true);
      primary_header_->SetCloseCallback(
          base::BindRepeating(&AstraSplitView::OnCloseButtonPressed,
                              base::Unretained(this),
                              AstraSplitPane::kPrimary));
    }
    // Create secondary header if needed.
    if (!secondary_header_) {
      secondary_header_ = AddChildView(std::make_unique<AstraSplitPaneHeader>());
      secondary_header_->SetPane(AstraSplitPane::kSecondary);
      secondary_header_->SetTitle(secondary_label_);
      secondary_header_->SetShowCloseButton(true);
      secondary_header_->SetCloseCallback(
          base::BindRepeating(&AstraSplitView::OnCloseButtonPressed,
                              base::Unretained(this),
                              AstraSplitPane::kSecondary));
    }

    primary_header_->SetVisible(true);
    secondary_header_->SetVisible(true);
  } else {
    // Hide headers but keep them in the hierarchy for fast toggle.
    if (primary_header_) {
      primary_header_->SetVisible(false);
    }
    if (secondary_header_) {
      secondary_header_->SetVisible(false);
    }
  }
}

void AstraSplitView::UpdateDividerToolbar() {
  if (show_divider_toolbar_) {
    if (!divider_toolbar_) {
      divider_toolbar_ = AddChildView(std::make_unique<AstraSplitDividerToolbar>());
      divider_toolbar_->SetSwapCallback(
          base::BindRepeating(&AstraSplitView::OnSwapButtonPressed,
                              base::Unretained(this)));
      divider_toolbar_->SetLayoutToggleCallback(
          base::BindRepeating(&AstraSplitView::OnLayoutToggleButtonPressed,
                              base::Unretained(this)));
      divider_toolbar_->UpdateLayoutMode(layout_mode_);
    }
    divider_toolbar_->SetVisible(true);
    divider_toolbar_->SetToolbarVisible(true);
  } else {
    if (divider_toolbar_) {
      divider_toolbar_->SetVisible(false);
    }
  }
}

void AstraSplitView::LayoutPaneHeaders() {
  if (!show_pane_headers_) {
    return;
  }

  gfx::Rect bounds = GetLocalBounds();
  const int kHeaderHeight = 28;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    // Horizontal split — headers are at top of each pane.
    int primary_width = static_cast<int>(bounds.width() * ratio_);
    int divider_thickness = show_handle_ ? divider_width_ : 0;

    if (primary_header_ && primary_header_->GetVisible()) {
      primary_header_->SetBounds(0, 0, primary_width, kHeaderHeight);
    }

    int secondary_x = primary_width + divider_thickness;
    int secondary_width = bounds.width() - secondary_x;
    if (secondary_header_ && secondary_header_->GetVisible()) {
      secondary_header_->SetBounds(secondary_x, 0, secondary_width, kHeaderHeight);
    }
  } else {
    // Vertical split — headers are at top of each pane.
    int primary_height = static_cast<int>(bounds.height() * ratio_);
    int divider_thickness = show_handle_ ? divider_width_ : 0;

    if (primary_header_ && primary_header_->GetVisible()) {
      primary_header_->SetBounds(0, 0, bounds.width(), kHeaderHeight);
    }

    int secondary_y = primary_height + divider_thickness;
    if (secondary_header_ && secondary_header_->GetVisible()) {
      secondary_header_->SetBounds(0, secondary_y, bounds.width(), kHeaderHeight);
    }
  }
}

void AstraSplitView::LayoutDividerToolbar() {
  if (!show_divider_toolbar_ || !divider_toolbar_) {
    return;
  }

  gfx::Size pref = divider_toolbar_->GetPreferredSize();
  gfx::Rect bounds = GetLocalBounds();

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    // Horizontal split — toolbar is in the middle of the vertical divider.
    int divider_x = static_cast<int>(bounds.width() * ratio_);
    int toolbar_x = divider_x - pref.width() / 2;
    int toolbar_y = (bounds.height() - pref.height()) / 2;
    // Clamp to view bounds.
    toolbar_x = std::max(0, std::min(toolbar_x, bounds.width() - pref.width()));
    toolbar_y = std::max(0, std::min(toolbar_y, bounds.height() - pref.height()));
    divider_toolbar_->SetBounds(toolbar_x, toolbar_y, pref.width(), pref.height());
  } else {
    // Vertical split — toolbar is in the middle of the horizontal divider.
    int divider_y = static_cast<int>(bounds.height() * ratio_);
    int toolbar_x = (bounds.width() - pref.width()) / 2;
    int toolbar_y = divider_y - pref.height() / 2;
    toolbar_x = std::max(0, std::min(toolbar_x, bounds.width() - pref.width()));
    toolbar_y = std::max(0, std::min(toolbar_y, bounds.height() - pref.height()));
    divider_toolbar_->SetBounds(toolbar_x, toolbar_y, pref.width(), pref.height());
  }
}

void AstraSplitView::OnSwapButtonPressed() {
  SwapViews();
}

void AstraSplitView::OnLayoutToggleButtonPressed() {
  CycleNextLayoutMode();
}

void AstraSplitView::OnCloseButtonPressed(AstraSplitPane pane) {
  ClosePane(pane);
}

// =========================================================================
// Pane operations
// =========================================================================

void AstraSplitView::ClosePane(AstraSplitPane pane) {
  // TODO(astra): Implement proper pane closing for multi-pane layouts.
  //   For 2-pane mode, closing one pane leaves the other full-width.
  //   For multi-pane, closing a pane rebalances the remaining panes.
  //   Chromium owner: views::View hierarchy manipulation.

  if (pane == AstraSplitPane::kPrimary) {
    // Make the secondary pane the primary and hide the divider.
    // For now, just set ratio to minimum and notify.
    SetRatio(kMinPaneRatio, /*animate=*/true);
  } else {
    SetRatio(1.0f - kMinPaneRatio, /*animate=*/true);
  }

  NotifyPaneClosed(pane);
}

void AstraSplitView::NotifyPaneClosed(AstraSplitPane pane) {
  // The Observer interface doesn't have a pane closed method yet.
  // TODO(astra): Add OnSplitPaneClosed to the Observer interface if needed.
  // For now, this is a no-op for the legacy observer interface.
}

// =========================================================================
// Focus indicator
// =========================================================================

void AstraSplitView::SetShowFocusIndicator(bool show) {
  if (show_focus_indicator_ == show) {
    return;
  }
  show_focus_indicator_ = show;
  SchedulePaint();
}

// =========================================================================
// Empty pane placeholder
// =========================================================================

void AstraSplitView::SetEmptyPaneVisible(AstraSplitPane pane, bool visible) {
  raw_ptr<AstraSplitEmptyPaneView>& empty_pane =
      (pane == AstraSplitPane::kPrimary) ? primary_empty_pane_
                                          : secondary_empty_pane_;

  if (visible && !empty_pane) {
    // Create the empty pane view.
    empty_pane = AddChildView(std::make_unique<AstraSplitEmptyPaneView>(
        base::BindRepeating(&AstraSplitView::OnEmptyPaneOpenTab,
                            base::Unretained(this), pane)));
    empty_pane->SetMessage(
        pane == AstraSplitPane::kPrimary ? u"Primary pane is empty"
                                          : u"Secondary pane is empty");
  }

  if (empty_pane) {
    empty_pane->SetVisible(visible);
  }

  InvalidateLayout();
}

bool AstraSplitView::IsEmptyPaneVisible(AstraSplitPane pane) const {
  const raw_ptr<AstraSplitEmptyPaneView>& empty_pane =
      (pane == AstraSplitPane::kPrimary) ? primary_empty_pane_
                                          : secondary_empty_pane_;
  return empty_pane && empty_pane->GetVisible();
}

void AstraSplitView::SetEmptyPaneMessage(AstraSplitPane pane,
                                          const std::u16string& message) {
  raw_ptr<AstraSplitEmptyPaneView>& empty_pane =
      (pane == AstraSplitPane::kPrimary) ? primary_empty_pane_
                                          : secondary_empty_pane_;
  if (empty_pane) {
    empty_pane->SetMessage(message);
  }
}

void AstraSplitView::LayoutEmptyPanes() {
  gfx::Rect bounds = GetLocalBounds();
  const int kHeaderHeight = 28;
  int header_offset = show_pane_headers_ ? kHeaderHeight : 0;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    int primary_width = static_cast<int>(bounds.width() * ratio_);
    int divider_thickness = show_handle_ ? divider_width_ : 0;

    if (primary_empty_pane_ && primary_empty_pane_->GetVisible()) {
      primary_empty_pane_->SetBounds(0, header_offset, primary_width,
                                     bounds.height() - header_offset);
    }

    int secondary_x = primary_width + divider_thickness;
    int secondary_width = bounds.width() - secondary_x;
    if (secondary_empty_pane_ && secondary_empty_pane_->GetVisible()) {
      secondary_empty_pane_->SetBounds(
          secondary_x, header_offset, secondary_width,
          bounds.height() - header_offset);
    }
  } else {
    int primary_height = static_cast<int>(bounds.height() * ratio_);
    int divider_thickness = show_handle_ ? divider_width_ : 0;

    if (primary_empty_pane_ && primary_empty_pane_->GetVisible()) {
      primary_empty_pane_->SetBounds(0, header_offset, bounds.width(),
                                     primary_height - header_offset);
    }

    int secondary_y = primary_height + divider_thickness;
    int secondary_height = bounds.height() - secondary_y;
    if (secondary_empty_pane_ && secondary_empty_pane_->GetVisible()) {
      secondary_empty_pane_->SetBounds(
          0, secondary_y, bounds.width(), secondary_height);
    }
  }
}

void AstraSplitView::OnEmptyPaneOpenTab(AstraSplitPane pane) {
  // TODO(astra): Notify observers that the user requested a new tab
  //   in the empty pane.  The controller handles actual tab creation.
  //   Chromium owner: TabStripModel::AddWebContents
  //   (chrome/browser/ui/tabs/tab_strip_model.h)
  DLOG(INFO) << "Open tab requested in "
             << (pane == AstraSplitPane::kPrimary ? "primary" : "secondary")
             << " pane";
}

// =========================================================================
// Tab drop indicator
// =========================================================================

void AstraSplitView::ShowDropIndicator(AstraSplitPane pane, bool valid) {
  drop_indicator_visible_ = true;
  drop_indicator_pane_ = pane;
  drop_valid_ = valid;

  if (!drop_indicator_) {
    drop_indicator_ = AddChildView(std::make_unique<AstraSplitDropIndicator>());
  }

  drop_indicator_->SetDropValid(valid);
  drop_indicator_->ShowForPane(GetPaneBounds(pane));

  // Bring the indicator to the front.
  ReorderChildView(drop_indicator_, GetIndexOf(drop_indicator_).value_or(-1));
}

void AstraSplitView::HideDropIndicator() {
  if (!drop_indicator_visible_) {
    return;
  }
  drop_indicator_visible_ = false;
  if (drop_indicator_) {
    drop_indicator_->Hide();
  }
}

bool AstraSplitView::IsDropIndicatorVisible() const {
  return drop_indicator_visible_;
}

void AstraSplitView::LayoutDropIndicator() {
  if (!drop_indicator_visible_ || !drop_indicator_) {
    return;
  }
  drop_indicator_->ShowForPane(GetPaneBounds(drop_indicator_pane_));
}

// =========================================================================
// Divider context menu
// =========================================================================

void AstraSplitView::ShowDividerContextMenu(const gfx::Point& screen_point) {
  // TODO(astra): Implement using views::MenuRunner.
  //   The context menu should include:
  //     - Toggle orientation
  //     - Layout presets (50/50, 70/30, etc.)
  //     - Toggle pane headers
  //     - Toggle minimap
  //     - Maximize/restore pane
  //   Chromium owner: views::MenuRunner (ui/views/controls/menu/menu_runner.h)
  //
  // For now, this is a no-op placeholder.
  DLOG(INFO) << "Divider context menu requested at "
             << screen_point.ToString();
}

// =========================================================================
// Snap points visual feedback
// =========================================================================

void AstraSplitView::SetShowSnapIndicators(bool show) {
  if (show_snap_indicators_ == show) {
    return;
  }
  show_snap_indicators_ = show;
  SchedulePaint();
}

void AstraSplitView::SetSnapPoints(const std::vector<double>& points) {
  snap_points_ = points;
  std::sort(snap_points_.begin(), snap_points_.end());
  if (show_snap_indicators_) {
    SchedulePaint();
  }
}

void AstraSplitView::ResetSnapPointsToDefaults() {
  snap_points_ = {0.25, 1.0 / 3.0, 0.5, 2.0 / 3.0, 0.75};
  if (show_snap_indicators_) {
    SchedulePaint();
  }
}

void AstraSplitView::PaintSnapIndicators(gfx::Canvas* canvas) {
  if (!show_snap_indicators_ || snap_points_.empty() || !divider_) {
    return;
  }

  gfx::Rect bounds = GetLocalBounds();
  if (bounds.IsEmpty()) {
    return;
  }

  SkColor indicator_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    indicator_color = GetColorProvider()->GetColor(ui::kColorSeparator);
  }

  cc::PaintFlags flags;
  flags.setColor(indicator_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1);
  flags.setAntiAlias(true);

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    // Horizontal split — snap indicators are vertical lines on the top and
    // bottom edges, aligned with each snap point.
    const int kIndicatorLength = 8;
    for (double snap : snap_points_) {
      int x = static_cast<int>(bounds.width() * snap);
      // Top indicator.
      canvas->DrawLine(
          gfx::Point(x, 0),
          gfx::Point(x, kIndicatorLength),
          flags);
      // Bottom indicator.
      canvas->DrawLine(
          gfx::Point(x, bounds.height() - kIndicatorLength),
          gfx::Point(x, bounds.height()),
          flags);
    }
  } else {
    // Vertical split — snap indicators are horizontal lines on left and
    // right edges.
    const int kIndicatorLength = 8;
    for (double snap : snap_points_) {
      int y = static_cast<int>(bounds.height() * snap);
      // Left indicator.
      canvas->DrawLine(
          gfx::Point(0, y),
          gfx::Point(kIndicatorLength, y),
          flags);
      // Right indicator.
      canvas->DrawLine(
          gfx::Point(bounds.width() - kIndicatorLength, y),
          gfx::Point(bounds.width(), y),
          flags);
    }
  }
}

// =========================================================================
// Pane action buttons
// =========================================================================

void AstraSplitView::SetShowPaneActionButtons(bool show) {
  if (show_pane_action_buttons_ == show) {
    return;
  }
  show_pane_action_buttons_ = show;
  // TODO(astra): Create/destroy action buttons on pane headers.
  //   Buttons would include: swap, maximize, close, new tab in pane.
  //   For now, we just update the flag.
  InvalidateLayout();
}

// =========================================================================
// Smooth resizing animations
// =========================================================================

void AstraSplitView::SetAnimateResizing(bool animate) {
  animate_resizing_ = animate;
}

void AstraSplitView::SetAnimationDurationMs(int duration_ms) {
  if (duration_ms < 0) {
    duration_ms = 0;
  }
  animation_duration_ms_ = duration_ms;
}

// =========================================================================
// Focus indicator painting
// =========================================================================

void AstraSplitView::PaintFocusIndicator(gfx::Canvas* canvas) {
  if (!show_focus_indicator_) {
    return;
  }

  gfx::Rect pane_bounds = GetPaneBounds(
      focused_pane_ == AstraSplitPane::kPrimary ? AstraSplitPane::kPrimary
                                                 : AstraSplitPane::kSecondary);
  if (pane_bounds.IsEmpty()) {
    return;
  }

  // Draw a focus ring around the active pane.
  SkColor focus_color = SK_ColorBLUE;
  if (GetColorProvider()) {
    focus_color = GetColorProvider()->GetColor(ui::kColorFocusableBorderFocused);
  }

  cc::PaintFlags flags;
  flags.setColor(focus_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  // Inset by 1 pixel so the border is fully inside the bounds.
  gfx::Rect inset_bounds = pane_bounds;
  inset_bounds.Inset(gfx::Insets::VH(1, 1));
  canvas->DrawRect(inset_bounds, flags);
}

// =========================================================================
// Accessibility
// =========================================================================

std::u16string AstraSplitView::GetAccessibleName() const {
  // TODO(astra): Use localized strings (ui::ResourceBundle).
  std::u16string name = u"Split view";
  switch (layout_mode_) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      name = u"Horizontal split view, two panes";
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      name = u"Vertical split view, two panes";
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
      name = u"Horizontal split view, three panes";
      break;
    case AstraSplitLayoutMode::kThreePaneVertical:
      name = u"Vertical split view, three panes";
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
      name = u"Grid split view, two by two";
      break;
    case AstraSplitLayoutMode::kGridThreeByTwo:
      name = u"Grid split view, three by two";
      break;
    case AstraSplitLayoutMode::kPictureInPicture:
      name = u"Picture-in-picture split view";
      break;
    case AstraSplitLayoutMode::kTabShift:
      name = u"Tab shift split view";
      break;
  }
  return name;
}

void AstraSplitView::SetAccessibleDescription(const std::u16string& description) {
  if (accessible_description_ == description) {
    return;
  }
  accessible_description_ = description;
  NotifyAccessibilityEvent(ax::mojom::Event::kDescriptionChanged, true);
}

// =========================================================================
// Layout and sizing
// =========================================================================

void AstraSplitView::Layout() {
  if (!primary_view_ && !secondary_view_) {
    LayoutPaneHeaders();
    LayoutDividerToolbar();
    LayoutMinimap();
    LayoutEmptyPanes();
    LayoutDropIndicator();
    return;
  }

  gfx::Rect bounds = GetLocalBounds();
  int total_size = 0;
  int divider_pos = 0;

  // Calculate header offset (if headers are shown).
  const int kHeaderHeight = 28;
  int header_offset = show_pane_headers_ ? kHeaderHeight : 0;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    total_size = bounds.width();
    int primary_width = static_cast<int>(total_size * ratio_);
    divider_pos = primary_width;

    // Position primary view (left side), below the header.
    if (primary_view_) {
      primary_view_->SetBounds(0, header_offset, primary_width,
                                bounds.height() - header_offset);
    }

    // Position divider.
    int divider_thickness =
        show_handle_ ? divider_width_ : 0;
    divider_->SetBounds(primary_width, header_offset, divider_thickness,
                         bounds.height() - header_offset);

    // Position secondary view (right side), below the header.
    int secondary_x = primary_width + divider_thickness;
    int secondary_width = total_size - secondary_x;
    if (secondary_view_) {
      secondary_view_->SetBounds(secondary_x, header_offset, secondary_width,
                                  bounds.height() - header_offset);
    }
  } else {
    total_size = bounds.height();
    int primary_height = static_cast<int>(total_size * ratio_);
    divider_pos = primary_height;

    // Position primary view (top side), below the header.
    if (primary_view_) {
      primary_view_->SetBounds(0, header_offset, bounds.width(),
                                primary_height - header_offset);
    }

    // Position divider.
    int divider_thickness =
        show_handle_ ? divider_width_ : 0;
    divider_->SetBounds(0, primary_height, bounds.width(), divider_thickness);

    // Position secondary view (bottom side).
    int secondary_y = primary_height + divider_thickness;
    int secondary_height = total_size - secondary_y;
    if (secondary_view_) {
      secondary_view_->SetBounds(0, secondary_y, bounds.width(), secondary_height);
    }
  }

  // Layout pane headers and divider toolbar.
  LayoutPaneHeaders();
  LayoutDividerToolbar();

  LayoutMinimap();

  // Layout empty pane placeholders and drop indicator.
  LayoutEmptyPanes();
  LayoutDropIndicator();
}

// =========================================================================
// Pane bounds helper
// =========================================================================

gfx::Rect AstraSplitView::GetPaneBounds(AstraSplitPane pane) const {
  gfx::Rect bounds = GetLocalBounds();
  if (bounds.IsEmpty()) {
    return gfx::Rect();
  }

  const int kHeaderHeight = 28;
  int header_offset = show_pane_headers_ ? kHeaderHeight : 0;
  int divider_thickness = show_handle_ ? divider_width_ : 0;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    int primary_width = static_cast<int>(bounds.width() * ratio_);
    if (pane == AstraSplitPane::kPrimary) {
      return gfx::Rect(0, header_offset, primary_width,
                       bounds.height() - header_offset);
    } else {
      int secondary_x = primary_width + divider_thickness;
      return gfx::Rect(secondary_x, header_offset,
                       bounds.width() - secondary_x,
                       bounds.height() - header_offset);
    }
  } else {
    int primary_height = static_cast<int>(bounds.height() * ratio_);
    if (pane == AstraSplitPane::kPrimary) {
      return gfx::Rect(0, header_offset, bounds.width(),
                       primary_height - header_offset);
    } else {
      int secondary_y = primary_height + divider_thickness;
      return gfx::Rect(0, secondary_y, bounds.width(),
                       bounds.height() - secondary_y);
    }
  }
}

// =========================================================================
// Custom painting
// =========================================================================

void AstraSplitView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Paint snap point indicators (visual guides).
  PaintSnapIndicators(canvas);

  // Paint focus indicator around the active pane.
  PaintFocusIndicator(canvas);
}

gfx::Size AstraSplitView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // The preferred size is the sum of both children's preferred sizes plus
  // the divider, on the split axis.  On the other axis, take the max.
  //
  // TODO(astra): Implement proper preferred size calculation that combines
  // both child views' preferred sizes.  For now, return a reasonable default
  // since the split view fills its container in practice.
  return gfx::Size(800, 600);
}

void AstraSplitView::OnThemeChanged() {
  views::View::OnThemeChanged();

  // Update background color from color provider.
  if (GetColorProvider()) {
    SkColor bg_color =
        GetColorProvider()->GetColor(kColorAstraSplitViewBackground);
    SetBackground(views::CreateSolidBackground(bg_color));
  }

  SchedulePaint();
}

void AstraSplitView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  views::View::OnBoundsChanged(previous_bounds);

  // When the split view is resized, re-clamp the ratio to ensure both panes
  // still meet the minimum pixel size requirement.
  int total_size =
      (orientation_ == SplitViewOrientation::kHorizontal)
          ? width()
          : height();
  if (total_size > 0) {
    float clamped = ClampRatio(ratio_, total_size);
    if (clamped != ratio_) {
      ratio_ = clamped;
      InvalidateLayout();
      UpdateMinimap();
    }
  }
}

// =========================================================================
// Public static: ratio clamping
// =========================================================================

float AstraSplitView::ClampRatio(float ratio, int total_size) {
  // Apply ratio-based minimum first.
  if (ratio < kMinPaneRatio) {
    ratio = kMinPaneRatio;
  }
  if (ratio > 1.0f - kMinPaneRatio) {
    ratio = 1.0f - kMinPaneRatio;
  }

  // Also apply pixel-based minimum if total_size is provided.
  if (total_size > 0) {
    float min_pixel_ratio =
        static_cast<float>(kMinPaneSizeDips) / total_size;
    float max_pixel_ratio = 1.0f - min_pixel_ratio;

    if (ratio < min_pixel_ratio) {
      ratio = min_pixel_ratio;
    }
    if (ratio > max_pixel_ratio) {
      ratio = max_pixel_ratio;
    }
  }

  // Final sanity clamp to [0, 1].
  if (ratio < 0.0f) return 0.0f;
  if (ratio > 1.0f) return 1.0f;
  return ratio;
}

// =========================================================================
// Divider drag callbacks
// =========================================================================

void AstraSplitView::OnDividerDragStarted(const gfx::Point& location) {
  is_dragging_ = true;
  NotifyRatioChanging();
}

void AstraSplitView::OnDividerDragged(const gfx::Point& location) {
  gfx::Rect bounds = GetLocalBounds();
  float new_ratio = 0.5f;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    int total_width = bounds.width();
    if (total_width > 0) {
      // The divider position is at |location.x()|.  The ratio corresponds
      // to where the divider sits along the total width.
      new_ratio = static_cast<float>(location.x()) / total_width;
    }
  } else {
    int total_height = bounds.height();
    if (total_height > 0) {
      new_ratio = static_cast<float>(location.y()) / total_height;
    }
  }

  float old_ratio = ratio_;
  SetRatio(new_ratio, /*animate=*/false);

  if (ratio_ != old_ratio) {
    NotifyRatioChanging();
  }
}

void AstraSplitView::OnDividerDragEnded() {
  is_dragging_ = false;

  // Snap to preset if enabled after drag ends.
  if (settings_.snap_to_presets) {
    float snapped = MaybeSnapToPreset(ratio_);
    if (snapped != ratio_) {
      SetRatio(snapped, /*animate=*/true);
    }
  }

  NotifyRatioChanged();
}

// =========================================================================
// Private helpers
// =========================================================================

int AstraSplitView::ComputeDividerPosition() const {
  gfx::Rect bounds = GetLocalBounds();
  if (orientation_ == SplitViewOrientation::kHorizontal) {
    return static_cast<int>(bounds.width() * ratio_);
  }
  return static_cast<int>(bounds.height() * ratio_);
}

float AstraSplitView::ComputeRatioFromPosition(int position) const {
  gfx::Rect bounds = GetLocalBounds();
  int total_size =
      (orientation_ == SplitViewOrientation::kHorizontal) ? bounds.width()
                                                          : bounds.height();
  if (total_size <= 0) {
    return 0.5f;
  }
  return static_cast<float>(position) / total_size;
}

void AstraSplitView::AnimateDividerToPosition(int new_position) {
  // TODO(astra): Implement layer-based animation for the divider and pane
  //   bounds.  Reference Chromium's views::AnimationBuilder pattern.
  //
  // The cleanest approach is to animate the divider's layer position and
  // then finalize the layout at the end of the animation.  Alternatively,
  // we could use a SlideAnimation and call Layout() on each frame.
  //
  // For now, this is a no-op — SetRatio() handles layout immediately.
}

float AstraSplitView::MaybeSnapToPreset(float ratio) {
  if (!settings_.snap_to_presets) {
    return ratio;
  }

  auto preset = RatioToSplitViewPreset(
      ratio,
      static_cast<float>(settings_.snap_distance_dips) /
          static_cast<float>((orientation_ == SplitViewOrientation::kHorizontal)
                                 ? width()
                                 : height()));
  if (preset.has_value()) {
    return SplitViewPresetToRatio(*preset);
  }
  return ratio;
}

void AstraSplitView::UpdateMinimap() {
  if (minimap_ && minimap_visible_) {
    minimap_->UpdateLayout(ratio_, orientation_);
  }
}

void AstraSplitView::LayoutMinimap() {
  if (!minimap_ || !minimap_visible_) {
    return;
  }

  gfx::Size pref = minimap_->GetPreferredSize();
  gfx::Rect bounds = GetLocalBounds();

  // Position in the bottom-right corner with a small margin.
  const int kMargin = 8;
  int x = bounds.width() - pref.width() - kMargin;
  int y = bounds.height() - pref.height() - kMargin;

  minimap_->SetBounds(x, y, pref.width(), pref.height());
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraSplitView::NotifyRatioChanging() {
  for (Observer& observer : observers_) {
    observer.OnSplitRatioChanging(ratio_);
  }
}

void AstraSplitView::NotifyRatioChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitRatioChanged(ratio_);
  }
}

void AstraSplitView::NotifyOrientationChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitOrientationChanged(orientation_);
  }
}

void AstraSplitView::NotifyViewsSwapped() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewsSwapped();
  }
}

void AstraSplitView::NotifyViewReplaced(bool is_primary) {
  for (Observer& observer : observers_) {
    observer.OnSplitViewReplaced(is_primary);
  }
}

void AstraSplitView::NotifySettingsChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewSettingsChanged(settings_);
  }
}

void AstraSplitView::NotifyMaximized(bool primary_maximized) {
  for (Observer& observer : observers_) {
    observer.OnSplitViewMaximized(primary_maximized);
  }
}

void AstraSplitView::NotifyUnmaximized() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewUnmaximized();
  }
}

}  // namespace astra
