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
// Layout and sizing
// =========================================================================

void AstraSplitView::Layout() {
  if (!primary_view_ && !secondary_view_) {
    LayoutMinimap();
    return;
  }

  gfx::Rect bounds = GetLocalBounds();
  int total_size = 0;
  int divider_pos = 0;

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    total_size = bounds.width();
    int primary_width = static_cast<int>(total_size * ratio_);
    divider_pos = primary_width;

    // Position primary view (left side).
    if (primary_view_) {
      primary_view_->SetBounds(0, 0, primary_width, bounds.height());
    }

    // Position divider.
    int divider_thickness =
        show_handle_ ? divider_width_ : 0;
    divider_->SetBounds(primary_width, 0, divider_thickness, bounds.height());

    // Position secondary view (right side).
    int secondary_x = primary_width + divider_thickness;
    int secondary_width = total_size - secondary_x;
    if (secondary_view_) {
      secondary_view_->SetBounds(secondary_x, 0, secondary_width, bounds.height());
    }
  } else {
    total_size = bounds.height();
    int primary_height = static_cast<int>(total_size * ratio_);
    divider_pos = primary_height;

    // Position primary view (top side).
    if (primary_view_) {
      primary_view_->SetBounds(0, 0, bounds.width(), primary_height);
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

  LayoutMinimap();
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
