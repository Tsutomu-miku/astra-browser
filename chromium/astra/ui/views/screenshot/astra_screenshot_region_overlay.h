#ifndef ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_REGION_OVERLAY_H_
#define ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_REGION_OVERLAY_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget_delegate.h"

#include "astra/ui/views/screenshot/astra_screenshot_capture_model.h"

namespace views {
class Widget;
class View;
class Label;
class MdTextButton;
}  // namespace views

namespace astra {

// =========================================================================
// Astra screenshot region overlay
// =========================================================================
//
// Full-window overlay for region screenshot selection. When the user
// triggers "Capture Region" mode, this overlay appears over the entire
// browser window with a semi-transparent dark tint. The user clicks and
// drags to select a rectangular region; the selected area is shown as
// a clear (non-tinted) rectangle with a border and dimension tooltip.
//
// Controls:
//   - Click and drag: select a rectangular region
//   - Drag inside selection: move the selection
//   - Drag a handle: resize from that edge/corner
//   - Release mouse: finalize selection (shows handles, does not capture yet)
//   - Enter key or double-click: confirm selection and capture the region
//   - Escape key or right-click: cancel and close the overlay
//   - Arrow keys: nudge selection by 1 pixel
//   - Shift+Arrow keys: resize selection by 10 pixels
//   - Shift+drag: lock aspect ratio
//
// Visual features:
//   - Dark semi-transparent overlay outside selection
//   - 2px accent-color border around selection
//   - 8 resize handles (4 corners + 4 edge midpoints)
//   - Dimension tooltip showing width x height and position
//   - Crosshair cursor while selecting
//   - Optional grid display (configurable via model settings)
//   - Optional pixel grid (visible when zoomed in close)
//   - Optional magnifier (stub, configurable via model settings)
//   - Aspect ratio lock toggle button
//   - Snap-to-grid toggle button
//   - Size readout with position info
//
// Model/view pattern: The overlay observes an AstraScreenshotCaptureModel
// for settings and region state. User interactions update the model,
// and the overlay renders based on model state.
//
// Chromium subsystem reused: views::Widget + views::View for the overlay,
//   Skia for the semi-transparent overlay painting, ui::ColorProvider for
//   Astra color integration.
//
// Chromium pattern reference:
//   - Chrome DevTools element picker overlay
//   - Chrome screenshot region selection (chrome/browser/screenshots/)
//   - Native screenshot tools (macOS shift-cmd-4, Windows Snipping Tool)
//
// TODO(astra): Integrate with Chromium's region capture feature if available.
//   Chromium owner: ScreenshotManager region capture or DevTools element picker
//   (content/browser/screenshot/, third_party/blink/renderer/devtools/)
//   Patch point: May need to hook into the region capture UI flow.
// =========================================================================

class AstraScreenshotRegionOverlay
    : public AstraScreenshotCaptureModelObserver {
 public:
  // Delegate interface for region selection events.
  class Delegate {
   public:
    // Called when the user confirms a region selection.
    virtual void OnRegionSelected(const gfx::Rect& region) = 0;

    // Called when the user cancels the region selection.
    virtual void OnRegionSelectionCancelled() = 0;

   protected:
    ~Delegate() = default;
  };

  // Creates and shows the region selection overlay over the browser window.
  // |browser_widget| is the browser window's widget.
  // |delegate| receives selection events.
  // |model| optional capture model for settings and state.
  static views::Widget* ShowOverlay(views::Widget* browser_widget,
                                    Delegate* delegate,
                                    AstraScreenshotCaptureModel* model = nullptr);

  AstraScreenshotRegionOverlay(const AstraScreenshotRegionOverlay&) = delete;
  AstraScreenshotRegionOverlay& operator=(
      const AstraScreenshotRegionOverlay&) = delete;
  ~AstraScreenshotRegionOverlay() override;

  // -- Region management ---------------------------------------------------

  // Set the selected region.
  void SetRegion(const gfx::Rect& region);

  // Get the current selected region.
  gfx::Rect GetRegion() const;

  // Reset the selection (clear it).
  void ResetRegion();

  // Get the region size text (e.g. "1920 x 1080").
  std::string GetRegionSizeText() const;

  // -- Capture mode --------------------------------------------------------

  // Set the current capture mode.
  void SetMode(AstraScreenshotMode mode);

  // Get the current capture mode.
  AstraScreenshotMode GetMode() const { return mode_; }

  // -- Magnifier -----------------------------------------------------------

  // Set whether the magnifier is shown.
  void SetShowMagnifier(bool show);

  // Get whether the magnifier is shown.
  bool GetShowMagnifier() const { return show_magnifier_; }

  // Set the position of the magnifier (center point).
  void SetMagnifierPosition(const gfx::Point& position);

  // Get the current magnifier position.
  gfx::Point GetMagnifierPosition() const { return magnifier_position_; }

  // -- Grid ----------------------------------------------------------------

  // Set whether the grid overlay is shown.
  void SetShowGrid(bool show);

  // Get whether the grid overlay is shown.
  bool GetShowGrid() const { return show_grid_; }

  // Set the grid size in pixels.
  void SetGridSize(int size_px);

  // Get the grid size in pixels.
  int GetGridSize() const { return grid_size_; }

  // -- Pixel grid ----------------------------------------------------------

  // Set whether pixel-level grid is shown (when zoomed in).
  void SetShowPixelGrid(bool show);

  // Get whether pixel-level grid is shown.
  bool GetShowPixelGrid() const { return show_pixel_grid_; }

  // -- Aspect ratio constraint ---------------------------------------------

  // Set whether aspect ratio is constrained, and the ratio value.
  // Passing |constrain| = false disables the constraint (ratio is ignored).
  void SetAspectRatioConstraint(bool constrain, double ratio);

  // Whether aspect ratio is currently constrained.
  bool IsAspectRatioConstrained() const { return aspect_ratio_constrained_; }

  // Get the current aspect ratio (width / height). Returns 0.0 if not
  // constrained.
  double GetAspectRatio() const {
    return aspect_ratio_constrained_ ? aspect_ratio_ : 0.0;
  }

  // -- Selection state -----------------------------------------------------

  // Set whether the user is currently creating a selection (dragging).
  void SetIsSelecting(bool selecting);

  // Whether the user is currently selecting a region.
  bool IsSelecting() const { return is_selecting_; }

  // -- Resize state --------------------------------------------------------

  // Set whether the user is currently resizing the selection.
  void SetIsResizing(bool resizing);

  // Whether the user is currently resizing.
  bool IsResizing() const { return is_resizing_; }

  // -- Resize handle -------------------------------------------------------

  // Set the currently active resize handle.
  void SetResizeHandle(AstraResizeHandle handle);

  // Get the currently active resize handle.
  AstraResizeHandle GetResizeHandle() const { return resize_handle_; }

  // -- AstraScreenshotCaptureModelObserver --------------------------------

  void OnRegionChanged(const gfx::Rect& region) override;
  void OnCaptureSettingsChanged() override;
  void OnSettingsChanged(AstraScreenshotCaptureModel* model) override;

 private:
  // The actual overlay view.
  class OverlayView;

  explicit AstraScreenshotRegionOverlay(Delegate* delegate,
                                       AstraScreenshotCaptureModel* model);

  // Show the overlay as a child of |browser_widget|.
  views::Widget* Show(views::Widget* browser_widget);

  // Apply model settings to the overlay view.
  void ApplySettingsFromModel();

  // Update overlay view from current state.
  void UpdateViewFromState();

  raw_ptr<OverlayView> overlay_view_ = nullptr;
  raw_ptr<views::Widget> widget_ = nullptr;
  raw_ptr<Delegate> delegate_ = nullptr;
  raw_ptr<AstraScreenshotCaptureModel> model_ = nullptr;

  // Current capture mode.
  AstraScreenshotMode mode_ = AstraScreenshotMode::kRegion;

  // Magnifier state.
  bool show_magnifier_ = true;
  gfx::Point magnifier_position_;

  // Grid state.
  bool show_grid_ = false;
  int grid_size_ = 20;
  bool show_pixel_grid_ = false;

  // Aspect ratio constraint state.
  bool aspect_ratio_constrained_ = false;
  double aspect_ratio_ = 0.0;

  // Interaction state.
  bool is_selecting_ = false;
  bool is_resizing_ = false;
  AstraResizeHandle resize_handle_ = AstraResizeHandle::kNone;
};

// =========================================================================
// OverlayView — the actual view that paints the selection overlay
// =========================================================================
//
// This is a full-viewport view that:
//   - Paints a semi-transparent dark overlay over the entire area.
//   - Cuts out a clear rectangle where the user has selected.
//   - Draws an accent-colored border around the selected region.
//   - Draws 8 resize handles.
//   - Shows a dimension tooltip with current width x height + position.
//   - Handles mouse events to create, move, and resize the selection.
//   - Handles keyboard events (Escape, Enter, arrow keys, Shift+arrows).
//   - Optionally shows grid lines for alignment.
//   - Optionally shows pixel grid (for high zoom levels).
//   - Optionally shows a magnifier at the cursor.
//   - Provides aspect ratio lock and snap-to-grid controls.
//
// Interaction modes:
//   - kNone: no selection yet, click-drag to create
//   - kCreating: actively dragging to create a new selection
//   - kMoving: dragging inside the selection to move it
//   - kResizing: dragging a handle to resize
// =========================================================================

class AstraScreenshotRegionOverlay::OverlayView : public views::View {
 public:
  // Identifies which resize handle (if any) is under the cursor.
  enum class Handle {
    kNone,
    kTopLeft,
    kTop,
    kTopRight,
    kLeft,
    kRight,
    kBottomLeft,
    kBottom,
    kBottomRight,
  };

  // Current interaction mode.
  enum class Mode {
    kNone,       // No active interaction
    kCreating,   // Creating a new selection by dragging
    kMoving,     // Moving the existing selection
    kResizing,   // Resizing from a handle
  };

  OverlayView();
  ~OverlayView() override;

  OverlayView(const OverlayView&) = delete;
  OverlayView& operator=(const OverlayView&) = delete;

  // Set the delegate to notify of selection events.
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Get the current selection rectangle, in this view's coordinates.
  const gfx::Rect& selection() const { return selection_; }

  // Whether a selection currently exists.
  bool has_selection() const { return has_selection_; }

  // Reset the selection (clear it).
  void ClearSelection();

  // Set the selection directly.
  void SetSelection(const gfx::Rect& selection);

  // -- Region size text ---------------------------------------------------

  // Get the region size as a human-readable string like "1920 x 1080".
  std::u16string GetRegionSizeText() const;

  // -- Aspect ratio lock --------------------------------------------------

  // Get/set aspect ratio lock mode (legacy enum).
  AstraScreenshotAspectRatioLock aspect_ratio_lock() const {
    return aspect_ratio_lock_;
  }
  void SetAspectRatioLock(AstraScreenshotAspectRatioLock mode);

  // Cycle through aspect ratio modes (free -> 4:3 -> 16:9 -> 1:1 -> free).
  void CycleAspectRatioLock();

  // Set a custom aspect ratio constraint.
  void SetAspectRatioConstraint(bool constrain, double ratio);

  // Whether a custom aspect ratio is currently constrained.
  bool is_aspect_ratio_constrained() const {
    return aspect_ratio_constrained_;
  }

  // Get the custom aspect ratio. Returns 0.0 if not constrained.
  double custom_aspect_ratio() const {
    return aspect_ratio_constrained_ ? custom_aspect_ratio_ : 0.0;
  }

  // -- Grid / snap --------------------------------------------------------

  // Whether grid display is enabled.
  bool show_grid() const { return show_grid_; }
  void SetShowGrid(bool show);

  // Whether snap-to-grid is enabled.
  bool snap_to_grid() const { return snap_to_grid_; }
  void SetSnapToGrid(bool enabled);

  // Grid size in pixels.
  int grid_size() const { return grid_size_; }
  void SetGridSize(int size);

  // Whether the pixel grid is shown.
  bool show_pixel_grid() const { return show_pixel_grid_; }
  void SetShowPixelGrid(bool show);

  // -- Magnifier ----------------------------------------------------------

  // Whether the magnifier is shown.
  bool show_magnifier() const { return show_magnifier_; }
  void SetShowMagnifier(bool show);

  // -- Selection / resize state -------------------------------------------

  // Get the current interaction mode.
  Mode interaction_mode() const { return mode_; }

  // Get the active resize handle (only valid when mode_ == kResizing).
  Handle active_handle() const { return active_handle_; }

  // Whether the user is currently creating a selection.
  bool is_selecting() const { return mode_ == Mode::kCreating; }

  // Whether the user is currently resizing.
  bool is_resizing() const { return mode_ == Mode::kResizing; }

  // -- views::View -------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;
  gfx::NativeCursor GetCursor(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  bool OnDoubleClick(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // -- Painting ----------------------------------------------------------

  // Paint the semi-transparent overlay with a clear cutout for the
  // selected region.
  void PaintOverlay(gfx::Canvas* canvas);

  // Paint the selection border.
  void PaintSelectionBorder(gfx::Canvas* canvas);

  // Paint all 8 resize handles.
  void PaintHandles(gfx::Canvas* canvas);

  // Paint a single resize handle at the given center point.
  void PaintHandle(gfx::Canvas* canvas, const gfx::Point& center);

  // Paint the dimension tooltip near the selection.
  void PaintDimensionTooltip(gfx::Canvas* canvas);

  // Paint grid lines.
  void PaintGrid(gfx::Canvas* canvas);

  // Paint pixel grid (for high zoom levels).
  void PaintPixelGrid(gfx::Canvas* canvas);

  // Paint the crosshair / guide lines.
  void PaintCrosshair(gfx::Canvas* canvas);

  // Paint the magnifier at the cursor (stub implementation).
  void PaintMagnifier(gfx::Canvas* canvas);

  // -- Hit testing -------------------------------------------------------

  // Returns which resize handle is at the given point, or kNone.
  Handle HitTestHandle(const gfx::Point& point) const;

  // Returns whether the point is inside the selection (not on a handle).
  bool IsInsideSelection(const gfx::Point& point) const;

  // Get the bounding rect of a handle centered at the given point.
  gfx::Rect GetHandleRect(const gfx::Point& center) const;

  // -- Selection manipulation --------------------------------------------

  // Compute the selection rectangle from start and end points.
  gfx::Rect NormalizeSelection(const gfx::Point& start,
                               const gfx::Point& end) const;

  // Resize the selection by dragging |handle| to |point|.
  void ResizeSelection(Handle handle, const gfx::Point& point);

  // Move the selection by |delta|. Clamps to view bounds.
  void MoveSelection(const gfx::Vector2d& delta);

  // Nudge the selection by |delta| pixels (arrow key behavior).
  void NudgeSelection(const gfx::Vector2d& delta);

  // Nudge-resize from the bottom-right corner (Shift+arrow behavior).
  void NudgeResize(const gfx::Vector2d& delta);

  // Clamp selection to view bounds.
  void ClampSelectionToBounds();

  // Apply aspect ratio lock to a resize operation.
  void ApplyAspectRatioToResize(Handle handle,
                                gfx::Rect& rect,
                                const gfx::Point& fixed_point) const;

  // Get the current aspect ratio value (0.0 = free).
  double GetCurrentAspectRatio() const;

  // Snap a point to the nearest grid line.
  gfx::Point SnapPointToGrid(const gfx::Point& point) const;

  // Snap selection edges to grid lines.
  void SnapSelectionToGrid();

  // -- Confirmation / cancellation ---------------------------------------

  void ConfirmSelection();
  void CancelSelection();

  // -- Color helpers -----------------------------------------------------

  void UpdateColors();

  // -- Members -----------------------------------------------------------

  raw_ptr<Delegate> delegate_ = nullptr;

  // Current interaction mode.
  Mode mode_ = Mode::kNone;

  // Which handle is being dragged (only valid when mode_ == kResizing).
  Handle active_handle_ = Handle::kNone;

  // Start point of the current drag operation.
  gfx::Point drag_start_;

  // Selection at the start of the current drag operation.
  gfx::Rect drag_start_selection_;

  // Current selection rectangle.
  gfx::Rect selection_;

  // Whether a selection currently exists.
  bool has_selection_ = false;

  // Whether Shift key is pressed.
  bool shift_pressed_ = false;

  // Current cursor position (for magnifier and crosshair).
  gfx::Point cursor_position_;

  // -- Aspect ratio / grid / magnifier settings --------------------------

  AstraScreenshotAspectRatioLock aspect_ratio_lock_ =
      AstraScreenshotAspectRatioLock::kFree;
  bool aspect_ratio_constrained_ = false;
  double custom_aspect_ratio_ = 0.0;

  bool show_grid_ = false;
  bool snap_to_grid_ = false;
  int grid_size_ = 20;
  bool show_pixel_grid_ = false;
  bool show_magnifier_ = true;

  // -- Colors (cached from ColorProvider on theme change) ----------------

  SkColor overlay_color_ = SkColorSetARGB(128, 0, 0, 0);
  SkColor border_color_ = SK_ColorWHITE;
  SkColor handle_color_ = SK_ColorWHITE;
  SkColor tooltip_bg_color_ = SkColorSetARGB(230, 0, 0, 0);
  SkColor tooltip_text_color_ = SK_ColorWHITE;
  SkColor grid_color_ = SkColorSetARGB(64, 255, 255, 255);
  SkColor pixel_grid_color_ = SkColorSetARGB(32, 255, 255, 255);

  // -- Constants ---------------------------------------------------------

  static constexpr int kBorderThickness = 2;
  static constexpr int kHandleSize = 10;
  static constexpr int kHandleHitSlop = 4;
  static constexpr int kMinSelectionSize = 10;

  static constexpr int kTooltipPaddingX = 10;
  static constexpr int kTooltipPaddingY = 5;
  static constexpr int kTooltipOffsetY = 10;
  static constexpr int kTooltipCornerRadius = 6;

  static constexpr int kNudgeDistance = 1;
  static constexpr int kNudgeShiftDistance = 10;

  static constexpr int kMagnifierSize = 120;
  static constexpr int kMagnifierZoom = 2;
  static constexpr int kMagnifierCrosshairSize = 20;

  static constexpr int kMinGridSize = 5;
  static constexpr int kMaxGridSize = 200;
  static constexpr int kDefaultGridSize = 20;

  static constexpr int kPixelGridThreshold = 400;  // Zoom threshold for pixel grid
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_REGION_OVERLAY_H_
