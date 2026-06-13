#include "astra/ui/views/screenshot/astra_screenshot_region_overlay.h"

#include <cmath>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "astra/ui/color/astra_color_ids.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace astra {

namespace {

// Helper: get the center point of a handle for the specified handle on a rect.
gfx::Point GetHandleCenter(
    const gfx::Rect& rect,
    AstraScreenshotRegionOverlay::OverlayView::Handle handle) {
  using Handle = AstraScreenshotRegionOverlay::OverlayView::Handle;
  int cx = rect.CenterPoint().x();
  int cy = rect.CenterPoint().y();
  int left = rect.x();
  int top = rect.y();
  int right = rect.right();
  int bottom = rect.bottom();

  switch (handle) {
    case Handle::kTopLeft:
      return gfx::Point(left, top);
    case Handle::kTop:
      return gfx::Point(cx, top);
    case Handle::kTopRight:
      return gfx::Point(right, top);
    case Handle::kLeft:
      return gfx::Point(left, cy);
    case Handle::kRight:
      return gfx::Point(right, cy);
    case Handle::kBottomLeft:
      return gfx::Point(left, bottom);
    case Handle::kBottom:
      return gfx::Point(cx, bottom);
    case Handle::kBottomRight:
      return gfx::Point(right, bottom);
    case Handle::kNone:
      return gfx::Point();
  }
  return gfx::Point();
}

// Get the cursor type for a given handle.
ui::mojom::CursorType GetCursorForHandle(
    AstraScreenshotRegionOverlay::OverlayView::Handle handle) {
  using Handle = AstraScreenshotRegionOverlay::OverlayView::Handle;
  switch (handle) {
    case Handle::kTopLeft:
    case Handle::kBottomRight:
      return ui::mojom::CursorType::kNorthWestSouthEastResize;
    case Handle::kTop:
    case Handle::kBottom:
      return ui::mojom::CursorType::kNorthSouthResize;
    case Handle::kTopRight:
    case Handle::kBottomLeft:
      return ui::mojom::CursorType::kNorthEastSouthWestResize;
    case Handle::kLeft:
    case Handle::kRight:
      return ui::mojom::CursorType::kEastWestResize;
    case Handle::kNone:
      return ui::mojom::CursorType::kCrosshair;
  }
  return ui::mojom::CursorType::kCrosshair;
}

// Helper to clamp a value.
template <typename T>
T Clamp(T value, T min_val, T max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

// Convert OverlayView::Handle to model's AstraScreenshotRegionHandle.
AstraScreenshotRegionHandle HandleToModelHandle(
    AstraScreenshotRegionOverlay::OverlayView::Handle handle) {
  using OverlayHandle = AstraScreenshotRegionOverlay::OverlayView::Handle;
  switch (handle) {
    case OverlayHandle::kNone:
      return AstraScreenshotRegionHandle::kNone;
    case OverlayHandle::kTopLeft:
      return AstraScreenshotRegionHandle::kTopLeft;
    case OverlayHandle::kTop:
      return AstraScreenshotRegionHandle::kTop;
    case OverlayHandle::kTopRight:
      return AstraScreenshotRegionHandle::kTopRight;
    case OverlayHandle::kLeft:
      return AstraScreenshotRegionHandle::kLeft;
    case OverlayHandle::kRight:
      return AstraScreenshotRegionHandle::kRight;
    case OverlayHandle::kBottomLeft:
      return AstraScreenshotRegionHandle::kBottomLeft;
    case OverlayHandle::kBottom:
      return AstraScreenshotRegionHandle::kBottom;
    case OverlayHandle::kBottomRight:
      return AstraScreenshotRegionHandle::kBottomRight;
  }
  return AstraScreenshotRegionHandle::kNone;
}

}  // namespace

// =========================================================================
// AstraScreenshotRegionOverlay
// =========================================================================

// static
views::Widget* AstraScreenshotRegionOverlay::ShowOverlay(
    views::Widget* browser_widget,
    Delegate* delegate,
    AstraScreenshotCaptureModel* model) {
  DCHECK(browser_widget);
  DCHECK(delegate);

  auto* overlay =
      new AstraScreenshotRegionOverlay(delegate, model);
  views::Widget* widget = overlay->Show(browser_widget);
  return widget;
}

AstraScreenshotRegionOverlay::AstraScreenshotRegionOverlay(
    Delegate* delegate,
    AstraScreenshotCaptureModel* model)
    : delegate_(delegate), model_(model) {}

AstraScreenshotRegionOverlay::~AstraScreenshotRegionOverlay() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

views::Widget* AstraScreenshotRegionOverlay::Show(
    views::Widget* browser_widget) {
  DCHECK(browser_widget);

  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_CONTROL;
  params.ownership = views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET;
  params.parent = browser_widget->GetNativeView();
  params.delegate = nullptr;
  params.name = "AstraScreenshotRegionOverlay";
  params.bounds = browser_widget->GetClientAreaBoundsInScreen();

  widget_ = new views::Widget();
  widget_->Init(std::move(params));

  auto overlay_view = std::make_unique<OverlayView>();
  overlay_view_ = overlay_view.get();
  overlay_view_->set_delegate(delegate_);

  widget_->SetContentsView(std::move(overlay_view));
  widget_->SetSize(browser_widget->GetClientAreaBoundsInScreen().size());
  widget_->Show();

  overlay_view_->RequestFocus();

  // Apply settings from model if available.
  if (model_) {
    model_->AddObserver(this);
    ApplySettingsFromModel();
  }

  return widget_;
}

void AstraScreenshotRegionOverlay::ApplySettingsFromModel() {
  if (!model_ || !overlay_view_) return;

  overlay_view_->SetShowGrid(model_->GetShowGridInRegionSelection());
  overlay_view_->SetShowMagnifier(model_->GetShowMagnifierInRegionSelection());
  overlay_view_->SetAspectRatioLock(model_->GetRegionAspectRatioLock());
  overlay_view_->SetSnapToGrid(model_->GetSnapToGrid());
  overlay_view_->SetGridSize(model_->GetGridSizePixels());
}

void AstraScreenshotRegionOverlay::OnRegionChanged(const gfx::Rect& region) {
  if (overlay_view_ && !region.IsEmpty()) {
    overlay_view_->SetSelection(region);
  }
}

void AstraScreenshotRegionOverlay::OnCaptureSettingsChanged() {
  ApplySettingsFromModel();
}

// =========================================================================
// OverlayView
// =========================================================================

AstraScreenshotRegionOverlay::OverlayView::OverlayView() {
  set_can_process_events_within_subtree(true);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  SetPaintToLayer();
}

AstraScreenshotRegionOverlay::OverlayView::~OverlayView() = default;

void AstraScreenshotRegionOverlay::OverlayView::ClearSelection() {
  selection_ = gfx::Rect();
  has_selection_ = false;
  mode_ = Mode::kNone;
  active_handle_ = Handle::kNone;
  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::SetSelection(
    const gfx::Rect& selection) {
  selection_ = selection;
  has_selection_ = !selection.IsEmpty();
  SchedulePaint();
}

// =========================================================================
// Aspect ratio lock
// =========================================================================

void AstraScreenshotRegionOverlay::OverlayView::SetAspectRatioLock(
    AstraScreenshotAspectRatioLock mode) {
  if (aspect_ratio_lock_ == mode) return;
  aspect_ratio_lock_ = mode;

  // If a selection exists, snap it to the new aspect ratio.
  if (has_selection_ && mode != AstraScreenshotAspectRatioLock::kFree) {
    double ratio =
        AstraScreenshotCaptureModel::GetAspectRatioValue(mode);
    if (ratio > 0.0) {
      int w = selection_.width();
      int h = selection_.height();
      double current_ratio = static_cast<double>(w) / h;

      if (current_ratio > ratio) {
        // Wider — reduce width
        int new_w = static_cast<int>(h * ratio);
        selection_.set_width(new_w);
      } else {
        // Taller — reduce height
        int new_h = static_cast<int>(w / ratio);
        selection_.set_height(new_h);
      }
    }
  }

  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::CycleAspectRatioLock() {
  int current = static_cast<int>(aspect_ratio_lock_);
  int next = (current + 1) % 4;  // 4 modes: free, 4:3, 16:9, 1:1
  SetAspectRatioLock(static_cast<AstraScreenshotAspectRatioLock>(next));
}

// =========================================================================
// Grid / snap
// =========================================================================

void AstraScreenshotRegionOverlay::OverlayView::SetShowGrid(bool show) {
  if (show_grid_ == show) return;
  show_grid_ = show;
  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::SetSnapToGrid(bool enabled) {
  snap_to_grid_ = enabled;
}

void AstraScreenshotRegionOverlay::OverlayView::SetGridSize(int size) {
  grid_size_ = Clamp(size, kMinGridSize, kMaxGridSize);
  if (show_grid_) {
    SchedulePaint();
  }
}

void AstraScreenshotRegionOverlay::OverlayView::SetShowMagnifier(bool show) {
  if (show_magnifier_ == show) return;
  show_magnifier_ = show;
  SchedulePaint();
}

// =========================================================================
// Painting
// =========================================================================

void AstraScreenshotRegionOverlay::OverlayView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Paint grid first (under the overlay).
  if (show_grid_) {
    PaintGrid(canvas);
  }

  PaintOverlay(canvas);

  if (has_selection_ || mode_ == Mode::kCreating) {
    PaintSelectionBorder(canvas);
    PaintHandles(canvas);
    PaintDimensionTooltip(canvas);
  }

  // Crosshair is always shown during mouse interaction.
  if (has_selection_) {
    PaintCrosshair(canvas);
  }

  // Magnifier on top.
  if (show_magnifier_ && has_selection_) {
    PaintMagnifier(canvas);
  }
}

void AstraScreenshotRegionOverlay::OverlayView::PaintOverlay(
    gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();

  if (!has_selection_ && mode_ != Mode::kCreating) {
    canvas->DrawColor(overlay_color_);
    return;
  }

  gfx::Rect sel = selection_;

  // Top band.
  gfx::Rect top_band(0, 0, bounds.width(), sel.y());
  if (!top_band.IsEmpty()) {
    canvas->FillRect(top_band, overlay_color_);
  }

  // Bottom band.
  int bottom_y = sel.bottom();
  gfx::Rect bottom_band(0, bottom_y, bounds.width(),
                        bounds.height() - bottom_y);
  if (!bottom_band.IsEmpty()) {
    canvas->FillRect(bottom_band, overlay_color_);
  }

  // Left band.
  gfx::Rect left_band(0, sel.y(), sel.x(), sel.height());
  if (!left_band.IsEmpty()) {
    canvas->FillRect(left_band, overlay_color_);
  }

  // Right band.
  int right_x = sel.right();
  gfx::Rect right_band(right_x, sel.y(),
                       bounds.width() - right_x, sel.height());
  if (!right_band.IsEmpty()) {
    canvas->FillRect(right_band, overlay_color_);
  }
}

void AstraScreenshotRegionOverlay::OverlayView::PaintSelectionBorder(
    gfx::Canvas* canvas) {
  canvas->DrawRect(selection_, border_color_, kBorderThickness);
}

void AstraScreenshotRegionOverlay::OverlayView::PaintHandles(
    gfx::Canvas* canvas) {
  if (mode_ == Mode::kCreating) {
    return;
  }

  static const Handle kAllHandles[] = {
      Handle::kTopLeft, Handle::kTop,    Handle::kTopRight,
      Handle::kLeft,    Handle::kRight,  Handle::kBottomLeft,
      Handle::kBottom,  Handle::kBottomRight,
  };

  for (Handle handle : kAllHandles) {
    gfx::Point center = GetHandleCenter(selection_, handle);
    PaintHandle(canvas, center);
  }
}

void AstraScreenshotRegionOverlay::OverlayView::PaintHandle(
    gfx::Canvas* canvas,
    const gfx::Point& center) {
  gfx::Rect handle_rect = GetHandleRect(center);
  canvas->FillRect(handle_rect, handle_color_);
}

void AstraScreenshotRegionOverlay::OverlayView::PaintDimensionTooltip(
    gfx::Canvas* canvas) {
  // Show both dimensions and position.
  std::u16string size_text =
      base::NumberToString16(selection_.width()) + u" x " +
      base::NumberToString16(selection_.height()) + u" px";
  std::u16string pos_text =
      u"(" + base::NumberToString16(selection_.x()) + u", " +
      base::NumberToString16(selection_.y()) + u")";

  // Measure text (estimate for now).
  int size_width = static_cast<int>(size_text.length()) * 7 + kTooltipPaddingX * 2;
  int pos_width = static_cast<int>(pos_text.length()) * 7 + kTooltipPaddingX * 2;
  int text_width = std::max(size_width, pos_width);
  int text_height = 36 + kTooltipPaddingY * 2;  // 2 lines of text

  // Position tooltip below selection (or above if near bottom).
  int tooltip_x = selection_.x() + (selection_.width() - text_width) / 2;
  int tooltip_y = selection_.bottom() + kTooltipOffsetY;

  gfx::Rect bounds = GetLocalBounds();
  if (tooltip_x < 0) tooltip_x = 0;
  if (tooltip_x + text_width > bounds.width()) {
    tooltip_x = bounds.width() - text_width;
  }
  if (tooltip_y + text_height > bounds.height()) {
    tooltip_y = selection_.y() - kTooltipOffsetY - text_height;
  }

  // Draw background.
  gfx::Rect tooltip_bg(tooltip_x, tooltip_y, text_width, text_height);
  canvas->FillRoundRect(tooltip_bg, kTooltipCornerRadius,
                        tooltip_bg_color_);

  // Draw size text.
  canvas->DrawStringRect(
      size_text, gfx::FontList(), tooltip_text_color_,
      gfx::Rect(tooltip_x + kTooltipPaddingX, tooltip_y + kTooltipPaddingY,
                text_width - kTooltipPaddingX * 2, 18),
      gfx::ALIGN_CENTER, gfx::VALIGN_TOP);

  // Draw position text.
  canvas->DrawStringRect(
      pos_text, gfx::FontList(), tooltip_text_color_,
      gfx::Rect(tooltip_x + kTooltipPaddingX,
                tooltip_y + kTooltipPaddingY + 18,
                text_width - kTooltipPaddingX * 2, 18),
      gfx::ALIGN_CENTER, gfx::VALIGN_TOP);
}

void AstraScreenshotRegionOverlay::OverlayView::PaintGrid(
    gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();
  if (grid_size_ <= 0) return;

  // Draw vertical grid lines.
  for (int x = 0; x <= bounds.width(); x += grid_size_) {
    gfx::Rect line(x, 0, 1, bounds.height());
    canvas->FillRect(line, grid_color_);
  }

  // Draw horizontal grid lines.
  for (int y = 0; y <= bounds.height(); y += grid_size_) {
    gfx::Rect line(0, y, bounds.width(), 1);
    canvas->FillRect(line, grid_color_);
  }
}

void AstraScreenshotRegionOverlay::OverlayView::PaintCrosshair(
    gfx::Canvas* canvas) {
  if (mode_ != Mode::kCreating && mode_ != Mode::kResizing &&
      mode_ != Mode::kMoving) {
    return;
  }

  gfx::Rect bounds = GetLocalBounds();
  SkColor crosshair_color =
      SkColorSetA(border_color_, 128);  // Semi-transparent

  // Vertical line at cursor.
  if (cursor_position_.x() >= 0 && cursor_position_.x() < bounds.width()) {
    gfx::Rect vline(cursor_position_.x(), 0, 1, bounds.height());
    canvas->FillRect(vline, crosshair_color);
  }

  // Horizontal line at cursor.
  if (cursor_position_.y() >= 0 && cursor_position_.y() < bounds.height()) {
    gfx::Rect hline(0, cursor_position_.y(), bounds.width(), 1);
    canvas->FillRect(hline, crosshair_color);
  }
}

void AstraScreenshotRegionOverlay::OverlayView::PaintMagnifier(
    gfx::Canvas* canvas) {
  // Stub implementation: draw a circle outline at the cursor position
  // to indicate where the magnifier would be.
  //
  // TODO(astra): Implement actual pixel magnification.
  //   Chromium owner: Skia image sampling / magnification.
  //   Patch point: Would need access to the underlying screen/window bitmap.

  if (mode_ == Mode::kNone || mode_ == Mode::kCreating) {
    return;
  }

  int size = kMagnifierSize;
  int x = cursor_position_.x() - size / 2;
  int y = cursor_position_.y() - size / 2;

  // Draw magnifier background circle.
  gfx::Rect magnifier_rect(x, y, size, size);
  SkColor magnifier_bg = SkColorSetARGB(200, 30, 30, 30);
  canvas->FillRoundRect(magnifier_rect, size / 2, magnifier_bg);

  // Draw border.
  SkColor border = SkColorSetARGB(255, 255, 255, 255);
  canvas->DrawRoundRect(magnifier_rect, size / 2, border, 2);

  // Draw crosshair in center.
  int cx = cursor_position_.x();
  int cy = cursor_position_.y();
  int ch = kMagnifierCrosshairSize / 2;

  canvas->DrawLine(gfx::Point(cx - ch, cy), gfx::Point(cx + ch, cy),
                   SK_ColorWHITE, 1);
  canvas->DrawLine(gfx::Point(cx, cy - ch), gfx::Point(cx, cy + ch),
                   SK_ColorWHITE, 1);

  // Draw zoom level label.
  std::u16string zoom_text =
      base::NumberToString16(kMagnifierZoom) + u"x";
  canvas->DrawStringRect(
      zoom_text, gfx::FontList(), SK_ColorWHITE,
      gfx::Rect(x, y + size - 24, size, 20),
      gfx::ALIGN_CENTER, gfx::VALIGN_MIDDLE);
}

// =========================================================================
// Hit testing
// =========================================================================

AstraScreenshotRegionOverlay::OverlayView::Handle
AstraScreenshotRegionOverlay::OverlayView::HitTestHandle(
    const gfx::Point& point) const {
  if (!has_selection_ || mode_ == Mode::kCreating) {
    return Handle::kNone;
  }

  static const Handle kAllHandles[] = {
      Handle::kTopLeft, Handle::kTop,    Handle::kTopRight,
      Handle::kLeft,    Handle::kRight,  Handle::kBottomLeft,
      Handle::kBottom,  Handle::kBottomRight,
  };

  for (Handle handle : kAllHandles) {
    gfx::Point center = GetHandleCenter(selection_, handle);
    gfx::Rect hit_rect = GetHandleRect(center);
    hit_rect.Inset(-kHandleHitSlop, -kHandleHitSlop);
    if (hit_rect.Contains(point)) {
      return handle;
    }
  }

  return Handle::kNone;
}

bool AstraScreenshotRegionOverlay::OverlayView::IsInsideSelection(
    const gfx::Point& point) const {
  if (!has_selection_) {
    return false;
  }
  return selection_.Contains(point);
}

gfx::Rect AstraScreenshotRegionOverlay::OverlayView::GetHandleRect(
    const gfx::Point& center) const {
  return gfx::Rect(center.x() - kHandleSize / 2,
                   center.y() - kHandleSize / 2,
                   kHandleSize, kHandleSize);
}

// =========================================================================
// Selection manipulation
// =========================================================================

gfx::Rect AstraScreenshotRegionOverlay::OverlayView::NormalizeSelection(
    const gfx::Point& start,
    const gfx::Point& end) const {
  int x = std::min(start.x(), end.x());
  int y = std::min(start.y(), end.y());
  int width = std::abs(end.x() - start.x());
  int height = std::abs(end.y() - start.y());
  return gfx::Rect(x, y, width, height);
}

void AstraScreenshotRegionOverlay::OverlayView::ResizeSelection(
    Handle handle,
    const gfx::Point& point) {
  gfx::Point effective_point = point;
  if (snap_to_grid_) {
    effective_point = SnapPointToGrid(point);
  }

  gfx::Rect new_rect = drag_start_selection_;
  gfx::Point drag_delta = effective_point - drag_start_;

  switch (handle) {
    case Handle::kTopLeft:
      new_rect.set_origin(drag_start_selection_.origin() +
                          gfx::Vector2d(drag_delta.x(), drag_delta.y()));
      new_rect.set_width(drag_start_selection_.width() - drag_delta.x());
      new_rect.set_height(drag_start_selection_.height() - drag_delta.y());
      break;
    case Handle::kTop:
      new_rect.set_y(drag_start_selection_.y() + drag_delta.y());
      new_rect.set_height(drag_start_selection_.height() - drag_delta.y());
      break;
    case Handle::kTopRight:
      new_rect.set_y(drag_start_selection_.y() + drag_delta.y());
      new_rect.set_width(drag_start_selection_.width() + drag_delta.x());
      new_rect.set_height(drag_start_selection_.height() - drag_delta.y());
      break;
    case Handle::kLeft:
      new_rect.set_x(drag_start_selection_.x() + drag_delta.x());
      new_rect.set_width(drag_start_selection_.width() - drag_delta.x());
      break;
    case Handle::kRight:
      new_rect.set_width(drag_start_selection_.width() + drag_delta.x());
      break;
    case Handle::kBottomLeft:
      new_rect.set_x(drag_start_selection_.x() + drag_delta.x());
      new_rect.set_width(drag_start_selection_.width() - drag_delta.x());
      new_rect.set_height(drag_start_selection_.height() + drag_delta.y());
      break;
    case Handle::kBottom:
      new_rect.set_height(drag_start_selection_.height() + drag_delta.y());
      break;
    case Handle::kBottomRight:
      new_rect.set_width(drag_start_selection_.width() + drag_delta.x());
      new_rect.set_height(drag_start_selection_.height() + drag_delta.y());
      break;
    case Handle::kNone:
      return;
  }

  new_rect = new_rect.Standardized();

  // Apply aspect ratio lock if enabled.
  if (aspect_ratio_lock_ != AstraScreenshotAspectRatioLock::kFree) {
    // Determine the fixed point (opposite corner from the handle).
    gfx::Point fixed_point;
    switch (handle) {
      case Handle::kTopLeft:
        fixed_point = drag_start_selection_.bottom_right();
        break;
      case Handle::kTop:
        fixed_point = drag_start_selection_.bottom_center();
        break;
      case Handle::kTopRight:
        fixed_point = drag_start_selection_.bottom_left();
        break;
      case Handle::kLeft:
        fixed_point = drag_start_selection_.right_center();
        break;
      case Handle::kRight:
        fixed_point = drag_start_selection_.left_center();
        break;
      case Handle::kBottomLeft:
        fixed_point = drag_start_selection_.top_right();
        break;
      case Handle::kBottom:
        fixed_point = drag_start_selection_.top_center();
        break;
      case Handle::kBottomRight:
        fixed_point = drag_start_selection_.origin();
        break;
      case Handle::kNone:
        break;
    }
    ApplyAspectRatioToResize(handle, new_rect, fixed_point);
  }

  // Enforce minimum size.
  if (new_rect.width() < kMinSelectionSize) {
    new_rect.set_width(kMinSelectionSize);
  }
  if (new_rect.height() < kMinSelectionSize) {
    new_rect.set_height(kMinSelectionSize);
  }

  selection_ = new_rect;
  ClampSelectionToBounds();
  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::MoveSelection(
    const gfx::Vector2d& delta) {
  selection_ += delta;
  ClampSelectionToBounds();
  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::NudgeSelection(
    const gfx::Vector2d& delta) {
  if (!has_selection_) {
    return;
  }
  MoveSelection(delta);
}

void AstraScreenshotRegionOverlay::OverlayView::NudgeResize(
    const gfx::Vector2d& delta) {
  if (!has_selection_) {
    return;
  }

  gfx::Rect new_rect = selection_;
  new_rect.set_width(selection_.width() + delta.x());
  new_rect.set_height(selection_.height() + delta.y());

  if (new_rect.width() < kMinSelectionSize) {
    new_rect.set_width(kMinSelectionSize);
  }
  if (new_rect.height() < kMinSelectionSize) {
    new_rect.set_height(kMinSelectionSize);
  }

  if (aspect_ratio_lock_ != AstraScreenshotAspectRatioLock::kFree) {
    double ratio = GetCurrentAspectRatio();
    if (ratio > 0.0) {
      int w = new_rect.width();
      int h = new_rect.height();
      if (delta.x() != 0) {
        h = static_cast<int>(w / ratio);
      } else {
        w = static_cast<int>(h * ratio);
      }
      new_rect.set_width(w);
      new_rect.set_height(h);
    }
  }

  selection_ = new_rect;
  ClampSelectionToBounds();
  SchedulePaint();
}

void AstraScreenshotRegionOverlay::OverlayView::ClampSelectionToBounds() {
  gfx::Rect bounds = GetLocalBounds();
  if (selection_.x() < 0) {
    selection_.set_x(0);
  }
  if (selection_.y() < 0) {
    selection_.set_y(0);
  }
  if (selection_.right() > bounds.right()) {
    selection_.set_x(bounds.right() - selection_.width());
  }
  if (selection_.bottom() > bounds.bottom()) {
    selection_.set_y(bounds.bottom() - selection_.height());
  }
}

void AstraScreenshotRegionOverlay::OverlayView::ApplyAspectRatioToResize(
    Handle handle,
    gfx::Rect& rect,
    const gfx::Point& fixed_point) const {
  double ratio = GetCurrentAspectRatio();
  if (ratio <= 0.0) return;

  int new_width, new_height;
  bool is_corner = (handle == Handle::kTopLeft) ||
                   (handle == Handle::kTopRight) ||
                   (handle == Handle::kBottomLeft) ||
                   (handle == Handle::kBottomRight);

  if (is_corner) {
    int dx = std::abs(rect.right() - fixed_point.x());
    int dy = std::abs(rect.bottom() - fixed_point.y());
    if (dx > dy * ratio) {
      new_width = dx;
      new_height = static_cast<int>(dx / ratio);
    } else {
      new_height = dy;
      new_width = static_cast<int>(dy * ratio);
    }
  } else if (handle == Handle::kTop || handle == Handle::kBottom) {
    new_height = rect.height();
    new_width = static_cast<int>(new_height * ratio);
  } else {
    new_width = rect.width();
    new_height = static_cast<int>(new_width / ratio);
  }

  if (new_width < kMinSelectionSize) {
    new_width = kMinSelectionSize;
    new_height = static_cast<int>(new_width / ratio);
  }
  if (new_height < kMinSelectionSize) {
    new_height = kMinSelectionSize;
    new_width = static_cast<int>(new_height * ratio);
  }

  gfx::Rect new_rect(new_width, new_height);
  switch (handle) {
    case Handle::kTopLeft:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case Handle::kTop:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case Handle::kTopRight:
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height);
      break;
    case Handle::kLeft:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case Handle::kRight:
      new_rect.set_x(fixed_point.x());
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
    case Handle::kBottomLeft:
      new_rect.set_x(fixed_point.x() - new_width);
      new_rect.set_y(fixed_point.y());
      break;
    case Handle::kBottom:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y());
      break;
    case Handle::kBottomRight:
      new_rect.set_origin(fixed_point);
      break;
    case Handle::kNone:
      new_rect.set_x(fixed_point.x() - new_width / 2);
      new_rect.set_y(fixed_point.y() - new_height / 2);
      break;
  }

  rect = new_rect;
}

double AstraScreenshotRegionOverlay::OverlayView::GetCurrentAspectRatio()
    const {
  return AstraScreenshotCaptureModel::GetAspectRatioValue(aspect_ratio_lock_);
}

gfx::Point AstraScreenshotRegionOverlay::OverlayView::SnapPointToGrid(
    const gfx::Point& point) const {
  if (!snap_to_grid_ || grid_size_ <= 0) {
    return point;
  }
  int snapped_x = (point.x() + grid_size_ / 2) / grid_size_ * grid_size_;
  int snapped_y = (point.y() + grid_size_ / 2) / grid_size_ * grid_size_;
  return gfx::Point(snapped_x, snapped_y);
}

void AstraScreenshotRegionOverlay::OverlayView::SnapSelectionToGrid() {
  if (!has_selection_ || !snap_to_grid_ || grid_size_ <= 0) return;

  gfx::Rect snapped = selection_;
  snapped.set_x((selection_.x() + grid_size_ / 2) / grid_size_ * grid_size_);
  snapped.set_y((selection_.y() + grid_size_ / 2) / grid_size_ * grid_size_);
  int right =
      (selection_.right() + grid_size_ / 2) / grid_size_ * grid_size_;
  int bottom =
      (selection_.bottom() + grid_size_ / 2) / grid_size_ * grid_size_;
  snapped.set_width(right - snapped.x());
  snapped.set_height(bottom - snapped.y());

  if (snapped.width() < kMinSelectionSize) {
    snapped.set_width(kMinSelectionSize);
  }
  if (snapped.height() < kMinSelectionSize) {
    snapped.set_height(kMinSelectionSize);
  }

  selection_ = snapped;
  SchedulePaint();
}

// =========================================================================
// Confirmation / cancellation
// =========================================================================

void AstraScreenshotRegionOverlay::OverlayView::ConfirmSelection() {
  if (delegate_ && !selection_.IsEmpty()) {
    delegate_->OnRegionSelected(selection_);
  } else if (delegate_) {
    delegate_->OnRegionSelectionCancelled();
  }
}

void AstraScreenshotRegionOverlay::OverlayView::CancelSelection() {
  ClearSelection();
  if (delegate_) {
    delegate_->OnRegionSelectionCancelled();
  }
}

// =========================================================================
// Cursor
// =========================================================================

gfx::NativeCursor AstraScreenshotRegionOverlay::OverlayView::GetCursor(
    const ui::MouseEvent& event) {
  if (mode_ == Mode::kResizing) {
    return GetCursorForHandle(active_handle_);
  }
  if (mode_ == Mode::kMoving) {
    return ui::mojom::CursorType::kMove;
  }
  if (mode_ == Mode::kCreating) {
    return ui::mojom::CursorType::kCrosshair;
  }
  if (has_selection_) {
    Handle handle = HitTestHandle(event.location());
    if (handle != Handle::kNone) {
      return GetCursorForHandle(handle);
    }
    if (IsInsideSelection(event.location())) {
      return ui::mojom::CursorType::kMove;
    }
  }
  return ui::mojom::CursorType::kCrosshair;
}

// =========================================================================
// Color helpers
// =========================================================================

void AstraScreenshotRegionOverlay::OverlayView::UpdateColors() {
  const ui::ColorProvider* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  border_color_ = color_provider->GetColor(kColorAstraWorkspaceAccent);
  handle_color_ = color_provider->GetColor(kColorAstraWorkspaceAccent);
  overlay_color_ = SkColorSetARGB(140, 0, 0, 0);
  tooltip_bg_color_ = SkColorSetARGB(235, 0, 0, 0);
  tooltip_text_color_ = SK_ColorWHITE;
  grid_color_ = SkColorSetARGB(64, 255, 255, 255);

  SchedulePaint();
}

// =========================================================================
// Mouse events
// =========================================================================

bool AstraScreenshotRegionOverlay::OverlayView::OnMousePressed(
    const ui::MouseEvent& event) {
  // Right-click cancels.
  if (event.IsOnlyRightMouseButton()) {
    CancelSelection();
    return true;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return views::View::OnMousePressed(event);
  }

  gfx::Point location = event.location();
  if (snap_to_grid_) {
    location = SnapPointToGrid(location);
  }

  cursor_position_ = event.location();

  if (has_selection_) {
    Handle handle = HitTestHandle(event.location());
    if (handle != Handle::kNone) {
      mode_ = Mode::kResizing;
      active_handle_ = handle;
      drag_start_ = location;
      drag_start_selection_ = selection_;
      return true;
    }

    if (IsInsideSelection(event.location())) {
      mode_ = Mode::kMoving;
      drag_start_ = location;
      drag_start_selection_ = selection_;
      return true;
    }
  }

  // Start a new selection.
  mode_ = Mode::kCreating;
  has_selection_ = true;
  drag_start_ = location;
  drag_start_selection_ = gfx::Rect();
  selection_ = gfx::Rect(location.x(), location.y(), 0, 0);
  SchedulePaint();
  return true;
}

bool AstraScreenshotRegionOverlay::OverlayView::OnMouseDragged(
    const ui::MouseEvent& event) {
  cursor_position_ = event.location();
  gfx::Point location = event.location();

  switch (mode_) {
    case Mode::kCreating:
      selection_ = NormalizeSelection(drag_start_, location);
      if (snap_to_grid_) {
        SnapSelectionToGrid();
      }
      ClampSelectionToBounds();
      SchedulePaint();
      return true;

    case Mode::kMoving: {
      gfx::Vector2d delta = location - drag_start_;
      selection_ = drag_start_selection_ + delta;
      ClampSelectionToBounds();
      SchedulePaint();
      return true;
    }

    case Mode::kResizing:
      ResizeSelection(active_handle_, location);
      return true;

    case Mode::kNone:
      return views::View::OnMouseDragged(event);
  }

  return views::View::OnMouseDragged(event);
}

void AstraScreenshotRegionOverlay::OverlayView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (!event.IsOnlyLeftMouseButton()) {
    views::View::OnMouseReleased(event);
    return;
  }

  cursor_position_ = event.location();

  if (mode_ == Mode::kCreating) {
    mode_ = Mode::kNone;
    selection_ = NormalizeSelection(drag_start_, event.location());
    if (snap_to_grid_) {
      SnapSelectionToGrid();
    }
    ClampSelectionToBounds();

    if (selection_.width() < kMinSelectionSize &&
        selection_.height() < kMinSelectionSize) {
      // Single click — keep minimum size selection.
      selection_.set_width(kMinSelectionSize);
      selection_.set_height(kMinSelectionSize);
    }

    SchedulePaint();
    return;
  }

  if (mode_ == Mode::kMoving || mode_ == Mode::kResizing) {
    mode_ = Mode::kNone;
    active_handle_ = Handle::kNone;
    SchedulePaint();
    return;
  }

  views::View::OnMouseReleased(event);
}

void AstraScreenshotRegionOverlay::OverlayView::OnMouseMoved(
    const ui::MouseEvent& event) {
  cursor_position_ = event.location();
  // Only schedule repaint if magnifier or crosshair is visible.
  if (show_magnifier_ || mode_ == Mode::kCreating || mode_ == Mode::kResizing ||
      mode_ == Mode::kMoving) {
    SchedulePaint();
  }
  views::View::OnMouseMoved(event);
}

bool AstraScreenshotRegionOverlay::OverlayView::OnMouseWheel(
    const ui::MouseWheelEvent& event) {
  // TODO(astra): Consider using scroll wheel to adjust selection size
  //   or zoom the magnifier. For now, we ignore wheel events.
  return views::View::OnMouseWheel(event);
}

// =========================================================================
// Keyboard events
// =========================================================================

bool AstraScreenshotRegionOverlay::OverlayView::OnKeyPressed(
    const ui::KeyEvent& event) {
  // Track Shift key.
  if (event.key_code() == ui::VKEY_SHIFT) {
    shift_pressed_ = true;
    return true;
  }

  // R key: cycle aspect ratio modes.
  if (event.key_code() == ui::VKEY_R && !event.IsControlDown() &&
      !event.IsAltDown() && !event.IsCommandDown()) {
    CycleAspectRatioLock();
    return true;
  }

  // G key: toggle grid.
  if (event.key_code() == ui::VKEY_G && !event.IsControlDown() &&
      !event.IsAltDown() && !event.IsCommandDown()) {
    SetShowGrid(!show_grid_);
    return true;
  }

  // Escape cancels.
  if (event.key_code() == ui::VKEY_ESCAPE) {
    CancelSelection();
    return true;
  }

  // Enter confirms.
  if (event.key_code() == ui::VKEY_RETURN && has_selection_) {
    ConfirmSelection();
    return true;
  }

  // Arrow key support.
  int nudge_distance = shift_pressed_ ? kNudgeShiftDistance : kNudgeDistance;

  if (event.key_code() == ui::VKEY_LEFT) {
    if (event.IsControlDown() || event.IsAltDown()) {
      // Ctrl+Left / Alt+Left: resize from left edge.
      if (has_selection_) {
        gfx::Rect new_rect = selection_;
        new_rect.set_x(selection_.x() - nudge_distance);
        new_rect.set_width(selection_.width() + nudge_distance);
        if (aspect_ratio_lock_ != AstraScreenshotAspectRatioLock::kFree) {
          // Maintain aspect ratio.
          double ratio = GetCurrentAspectRatio();
          if (ratio > 0.0) {
            int new_h = static_cast<int>(new_rect.width() / ratio);
            new_rect.set_y(selection_.bottom() - new_h);
            new_rect.set_height(new_h);
          }
        }
        selection_ = new_rect;
        ClampSelectionToBounds();
        SchedulePaint();
      }
    } else if (shift_pressed_) {
      NudgeResize(gfx::Vector2d(-nudge_distance, 0));
    } else {
      NudgeSelection(gfx::Vector2d(-nudge_distance, 0));
    }
    return true;
  }
  if (event.key_code() == ui::VKEY_RIGHT) {
    if (event.IsControlDown() || event.IsAltDown()) {
      NudgeResize(gfx::Vector2d(nudge_distance, 0));
    } else if (shift_pressed_) {
      NudgeResize(gfx::Vector2d(nudge_distance, 0));
    } else {
      NudgeSelection(gfx::Vector2d(nudge_distance, 0));
    }
    return true;
  }
  if (event.key_code() == ui::VKEY_UP) {
    if (event.IsControlDown() || event.IsAltDown()) {
      if (has_selection_) {
        gfx::Rect new_rect = selection_;
        new_rect.set_y(selection_.y() - nudge_distance);
        new_rect.set_height(selection_.height() + nudge_distance);
        if (aspect_ratio_lock_ != AstraScreenshotAspectRatioLock::kFree) {
          double ratio = GetCurrentAspectRatio();
          if (ratio > 0.0) {
            int new_w = static_cast<int>(new_rect.height() * ratio);
            new_rect.set_x(selection_.right() - new_w);
            new_rect.set_width(new_w);
          }
        }
        selection_ = new_rect;
        ClampSelectionToBounds();
        SchedulePaint();
      }
    } else if (shift_pressed_) {
      NudgeResize(gfx::Vector2d(0, -nudge_distance));
    } else {
      NudgeSelection(gfx::Vector2d(0, -nudge_distance));
    }
    return true;
  }
  if (event.key_code() == ui::VKEY_DOWN) {
    if (event.IsControlDown() || event.IsAltDown()) {
      NudgeResize(gfx::Vector2d(0, nudge_distance));
    } else if (shift_pressed_) {
      NudgeResize(gfx::Vector2d(0, nudge_distance));
    } else {
      NudgeSelection(gfx::Vector2d(0, nudge_distance));
    }
    return true;
  }

  // Space bar confirms.
  if (event.key_code() == ui::VKEY_SPACE && has_selection_) {
    ConfirmSelection();
    return true;
  }

  return views::View::OnKeyPressed(event);
}

bool AstraScreenshotRegionOverlay::OverlayView::OnKeyReleased(
    const ui::KeyEvent& event) {
  if (event.key_code() == ui::VKEY_SHIFT) {
    shift_pressed_ = false;
    return true;
  }
  return views::View::OnKeyReleased(event);
}

bool AstraScreenshotRegionOverlay::OverlayView::OnDoubleClick(
    const ui::MouseEvent& event) {
  if (has_selection_ && IsInsideSelection(event.location())) {
    ConfirmSelection();
    return true;
  }

  if (!has_selection_) {
    // Double-click with no selection — select entire visible area.
    gfx::Rect bounds = GetLocalBounds();
    selection_ = gfx::Rect(10, 10, bounds.width() - 20, bounds.height() - 20);
    has_selection_ = true;
    SchedulePaint();
    return true;
  }

  return views::View::OnDoubleClick(event);
}

void AstraScreenshotRegionOverlay::OverlayView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraScreenshotRegionOverlay::OverlayView::OnGestureEvent(
    ui::GestureEvent* event) {
  // Basic gesture support (stub).
  // TODO(astra): Implement touch-based region selection.
  views::View::OnGestureEvent(event);
}

}  // namespace astra
