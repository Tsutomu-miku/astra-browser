#ifndef ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_MODEL_H_
#define ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_MODEL_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"

class PrefService;

namespace astra {

// Forward declaration: capture type enum is defined in the service layer.
// Defined in astra/browser/astra_screenshot_service.h.
enum class AstraScreenshotType;

// =========================================================================
// Astra screenshot capture model
// =========================================================================
//
// AstraScreenshotCaptureModel is the model layer for the Astra screenshot
// capture UI. It owns capture state, presentation settings, and region
// selection logic. Views (capture bubble, region overlay) observe the model
// and render accordingly.
//
// Truth sources:
//   - Capture state (bitmap, type, result) — owned by the model during the
//     capture flow; canonical capture data lives in AstraScreenshotService.
//   - Presentation settings — owned by PrefService (persisted).
//   - Region selection geometry — owned by the model during selection.
//
// Observer pattern: Views observe the model for state changes. All observer
// methods have empty default implementations so views can override only the
// methods they care about.
//
// Chromium subsystems reused:
//   - PrefService (settings persistence)
//   - base::ObserverList (observer pattern)
//   - Skia (bitmap data)
//
// Chromium pattern reference:
//   chrome/browser/ui/views/screenshot/screenshot_bubble.h
//   components/screenshot_capture/screenshot_capture_manager.h
//
// TODO(astra): Consider integrating this model with Chromium's screenshot
//   manager if we ever switch to the native screenshot capture pipeline.
//   Patch point: chrome/browser/screenshot/screenshot_manager.cc
// =========================================================================

// State of the screenshot capture process.
enum class AstraScreenshotCaptureState {
  kIdle,       // No capture in progress, no result.
  kCapturing,  // Capture is in progress (bitmap not yet ready).
  kReady,      // Capture completed, bitmap is available.
  kSaving,     // Save to file is in progress.
  kError,      // Last operation failed.
};

// Region selection handle identifiers.
// Identifies which edge or corner of the selection is being manipulated.
enum class AstraScreenshotRegionHandle {
  kNone = 0,
  kTopLeft,
  kTop,
  kTopRight,
  kLeft,
  kRight,
  kBottomLeft,
  kBottom,
  kBottomRight,
};

// Aspect ratio lock mode for region selection.
enum class AstraScreenshotAspectRatioLock {
  kFree = 0,     // No aspect ratio lock.
  kRatio4x3,     // 4:3 aspect ratio.
  kRatio16x9,    // 16:9 aspect ratio.
  kRatio1x1,     // 1:1 (square) aspect ratio.
};

// Image format for screenshot encoding.
// Mirrors AstraScreenshotImageFormat from the service, redefined here to
// avoid a hard dependency on the service header from the views layer.
enum class AstraScreenshotImageFormatModel {
  kPng = 0,
  kJpeg = 1,
  kWebP = 2,
};

// Default save location for screenshots.
enum class AstraScreenshotSaveLocation {
  kDownloads = 0,  // Save to Downloads folder.
  kClipboard = 1,  // Copy to clipboard only.
  kAsk = 2,        // Ask the user each time.
};

// =========================================================================
// Observer interface
// =========================================================================

class AstraScreenshotCaptureModelObserver : public base::CheckedObserver {
 public:
  // Called when a capture operation starts.
  virtual void OnCaptureStarted() {}

  // Called when a capture completes successfully and the bitmap is ready.
  virtual void OnCaptureCompleted(const SkBitmap& bitmap) {}

  // Called when a capture or save operation fails.
  virtual void OnCaptureFailed(const std::string& error) {}

  // Called when the selected region changes (during region selection).
  virtual void OnRegionChanged(const gfx::Rect& region) {}

  // Called when the capture type changes.
  virtual void OnCaptureTypeChanged(AstraScreenshotType type) {}

  // Called when a save operation completes successfully.
  virtual void OnSaveCompleted(const base::FilePath& path) {}

  // Called when a copy-to-clipboard operation completes successfully.
  virtual void OnCopyCompleted() {}

  // Called when any capture setting (format, quality, etc.) changes.
  virtual void OnCaptureSettingsChanged() {}

  // Called when the capture state changes (idle -> capturing -> ready, etc.).
  virtual void OnCaptureStateChanged(AstraScreenshotCaptureState state) {}

 protected:
  ~AstraScreenshotCaptureModelObserver() override = default;
};

// =========================================================================
// Recent capture entry
// =========================================================================

struct AstraScreenshotRecentCapture {
  std::string id;
  AstraScreenshotType type;
  gfx::Rect source_bounds;
  int64_t file_size_bytes = 0;
  base::Time timestamp;
  base::FilePath file_path;
};

// =========================================================================
// Model class
// =========================================================================

class AstraScreenshotCaptureModel {
 public:
  explicit AstraScreenshotCaptureModel(PrefService* pref_service);
  ~AstraScreenshotCaptureModel();

  AstraScreenshotCaptureModel(const AstraScreenshotCaptureModel&) = delete;
  AstraScreenshotCaptureModel& operator=(const AstraScreenshotCaptureModel&) =
      delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraScreenshotCaptureModelObserver* observer);
  void RemoveObserver(AstraScreenshotCaptureModelObserver* observer);

  // -- Capture state -------------------------------------------------------

  AstraScreenshotCaptureState capture_state() const { return capture_state_; }

  // The captured bitmap. May be null/empty if capture hasn't completed.
  const SkBitmap& capture_bitmap() const { return capture_bitmap_; }

  // Type of the current or last capture.
  AstraScreenshotType capture_type() const { return capture_type_; }
  void SetCaptureType(AstraScreenshotType type);

  // Source bounds of the capture (page/viewport coordinates).
  const gfx::Rect& source_bounds() const { return source_bounds_; }
  void set_source_bounds(const gfx::Rect& bounds) { source_bounds_ = bounds; }

  // Error message from the last failed operation. Empty if no error.
  const std::string& last_error() const { return last_error_; }

  // Start a new capture. Sets state to kCapturing and notifies observers.
  void StartCapture(AstraScreenshotType type);

  // Complete a capture with the given bitmap. Sets state to kReady.
  void CompleteCapture(const SkBitmap& bitmap,
                       AstraScreenshotType type,
                       const gfx::Rect& source_bounds);

  // Mark the current capture as failed. Sets state to kError.
  void FailCapture(const std::string& error_message);

  // Reset the model to idle state (clears bitmap, error, etc.).
  void ResetToIdle();

  // -- Save / copy operations ----------------------------------------------

  // Start a save operation. Sets state to kSaving.
  void StartSave();

  // Complete a save operation. Sets state back to kReady and notifies.
  void CompleteSave(const base::FilePath& path, int64_t file_size_bytes);

  // Complete a copy-to-clipboard operation. Notifies observers.
  void CompleteCopy();

  // -- Region selection ----------------------------------------------------

  // Selected region in viewport coordinates. Used for region-type captures.
  const gfx::Rect& selected_region() const { return selected_region_; }

  // Whether a region has been selected.
  bool has_selected_region() const { return has_selected_region_; }

  // Set the selected region from two points (start and end of a drag).
  // Normalizes the rectangle so width and height are always positive.
  void SetRegionFromPoints(const gfx::Point& start, const gfx::Point& end);

  // Set the selected region directly.
  void SetSelectedRegion(const gfx::Rect& region);

  // Constrain the current region to fit within |bounds|.
  // If the current region is outside |bounds|, it is moved and resized to fit.
  void ConstrainRegionToBounds(const gfx::Rect& bounds);

  // Snap the current region to the current aspect ratio lock setting.
  // |anchor_handle| specifies which handle is the anchor (stays fixed).
  // Default anchor is bottom-right (typical drag direction).
  void SnapRegionToAspectRatio(
      AstraScreenshotRegionHandle anchor_handle =
          AstraScreenshotRegionHandle::kBottomRight);

  // Move the selected region by |delta| pixels.
  void MoveRegion(const gfx::Vector2d& delta);

  // Resize the selected region by dragging |handle| to |point|.
  // Respects minimum size and aspect ratio lock settings.
  void ResizeRegion(AstraScreenshotRegionHandle handle,
                    const gfx::Point& point);

  // Nudge the region by |delta| (arrow key behavior).
  void NudgeRegion(const gfx::Vector2d& delta);

  // Nudge-resize from the bottom-right corner by |delta|
  // (Shift+arrow key behavior).
  void NudgeResizeRegion(const gfx::Vector2d& delta);

  // Snap the region edges to the nearest grid lines based on grid_size.
  void SnapRegionToGrid(int grid_size);

  // Clear the selected region.
  void ClearRegion();

  // -- Aspect ratio utilities ----------------------------------------------

  // Get the aspect ratio as a width/height ratio for the given lock mode.
  // Returns 0.0 for kFree (no fixed ratio).
  static double GetAspectRatioValue(AstraScreenshotAspectRatioLock mode);

  // -- Capture settings (persisted via PrefService) ------------------------

  // Default capture type.
  AstraScreenshotType GetDefaultCaptureType() const;
  void SetDefaultCaptureType(AstraScreenshotType type);

  // Whether to show the capture bubble after capture.
  bool GetShowCaptureBubble() const;
  void SetShowCaptureBubble(bool show);

  // Whether the capture bubble auto-dismisses.
  bool GetAutoDismissBubble() const;
  void SetAutoDismissBubble(bool auto_dismiss);

  // Auto-dismiss delay in seconds.
  int GetAutoDismissDelaySeconds() const;
  void SetAutoDismissDelaySeconds(int seconds);

  // Image format (PNG, JPEG, WebP).
  AstraScreenshotImageFormatModel GetImageFormat() const;
  void SetImageFormat(AstraScreenshotImageFormatModel format);

  // JPEG quality (1-100).
  int GetJpegQuality() const;
  void SetJpegQuality(int quality);

  // Show filename in capture bubble.
  bool GetShowFilenameInBubble() const;
  void SetShowFilenameInBubble(bool show);

  // Show dimensions in capture bubble.
  bool GetShowDimensionsInBubble() const;
  void SetShowDimensionsInBubble(bool show);

  // Show file size in capture bubble.
  bool GetShowFileSizeInBubble() const;
  void SetShowFileSizeInBubble(bool show);

  // Default save location.
  AstraScreenshotSaveLocation GetDefaultSaveLocation() const;
  void SetDefaultSaveLocation(AstraScreenshotSaveLocation location);

  // Copy to clipboard after capture.
  bool GetCopyToClipboardAfterCapture() const;
  void SetCopyToClipboardAfterCapture(bool enabled);

  // Show grid in region selection.
  bool GetShowGridInRegionSelection() const;
  void SetShowGridInRegionSelection(bool show);

  // Show magnifier in region selection.
  bool GetShowMagnifierInRegionSelection() const;
  void SetShowMagnifierInRegionSelection(bool show);

  // Region aspect ratio lock mode.
  AstraScreenshotAspectRatioLock GetRegionAspectRatioLock() const;
  void SetRegionAspectRatioLock(AstraScreenshotAspectRatioLock mode);

  // Grid size in pixels for snap-to-grid.
  int GetGridSizePixels() const;
  void SetGridSizePixels(int size);

  // Whether snap-to-grid is enabled.
  bool GetSnapToGrid() const;
  void SetSnapToGrid(bool enabled);

  // Maximum number of recent captures to remember.
  int GetMaxRecentCaptures() const;
  void SetMaxRecentCaptures(int max);

  // -- Recent captures -----------------------------------------------------

  // Get the list of recent captures (most recent first).
  const std::vector<AstraScreenshotRecentCapture>& recent_captures() const {
    return recent_captures_;
  }

  // Add a capture to the recent captures list.
  // If the list exceeds max_recent_captures_, the oldest is removed.
  void AddRecentCapture(const AstraScreenshotRecentCapture& capture);

  // Clear all recent captures.
  void ClearRecentCaptures();

  // -- Utility methods -----------------------------------------------------

  // Estimate file size in bytes for the current capture bitmap and settings.
  int64_t EstimateFileSize() const;

  // Generate a default filename for a screenshot.
  // Includes timestamp and capture type.
  static std::string GenerateDefaultFilename(
      AstraScreenshotType type,
      AstraScreenshotImageFormatModel format);

  // Clamp JPEG quality to valid range (1-100).
  static int ClampJpegQuality(int quality);

  // Clamp grid size to valid range.
  static int ClampGridSize(int size);

  // Clamp max recent captures to valid range.
  static int ClampMaxRecentCaptures(int max);

  // Clamp auto-dismiss delay to valid range (1-60 seconds).
  static int ClampAutoDismissDelay(int seconds);

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

 private:
  // -- State change helpers ------------------------------------------------

  void SetCaptureState(AstraScreenshotCaptureState state);

  // Notify observers that the region changed.
  void NotifyRegionChanged();

  // Notify observers that capture settings changed.
  void NotifySettingsChanged();

  // Notify observers that capture type changed.
  void NotifyCaptureTypeChanged();

  // -- Helpers -------------------------------------------------------------

  // Get the aspect ratio value from the current setting.
  double GetCurrentAspectRatio() const;

  // Apply aspect ratio constraint to a resize operation.
  void ApplyAspectRatioToResize(AstraScreenshotRegionHandle handle,
                                gfx::Rect& rect,
                                const gfx::Point& fixed_point) const;

  // -- Members -------------------------------------------------------------

  raw_ptr<PrefService> pref_service_ = nullptr;

  // Capture state.
  AstraScreenshotCaptureState capture_state_ =
      AstraScreenshotCaptureState::kIdle;
  AstraScreenshotType capture_type_;
  SkBitmap capture_bitmap_;
  gfx::Rect source_bounds_;
  std::string last_error_;

  // Region selection state.
  gfx::Rect selected_region_;
  bool has_selected_region_ = false;
  gfx::Point drag_start_;
  gfx::Rect drag_start_selection_;

  // Recent captures (in-memory only).
  std::vector<AstraScreenshotRecentCapture> recent_captures_;

  // Observers.
  base::ObserverList<AstraScreenshotCaptureModelObserver> observers_;

  // -- Constants -----------------------------------------------------------

  // Minimum selection size in DIPs.
  static constexpr int kMinSelectionSize = 10;

  // Minimum and maximum auto-dismiss delay.
  static constexpr int kMinAutoDismissSeconds = 1;
  static constexpr int kMaxAutoDismissSeconds = 60;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_MODEL_H_
