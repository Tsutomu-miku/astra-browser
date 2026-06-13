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
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
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
// capture UI. It owns capture state, presentation settings, region
// selection logic, annotation state, and capture result metadata.
// Views (capture bubble, region overlay) observe the model and render
// accordingly.
//
// Truth sources:
//   - Capture state (bitmap, type, result) — owned by the model during the
//     capture flow; canonical capture data lives in AstraScreenshotService.
//   - Presentation settings — owned by PrefService (persisted).
//   - Region selection geometry — owned by the model during selection.
//   - Annotation state — owned by the model during annotation flow.
//
// Observer pattern: Views observe the model for state changes. All observer
// methods have empty default implementations so views can override only the
// methods they care about.
//
// Chromium subsystems reused:
//   - PrefService (settings persistence)
//   - base::ObserverList (observer pattern)
//   - Skia (bitmap data, colors)
//
// Chromium pattern reference:
//   chrome/browser/ui/views/screenshot/screenshot_bubble.h
//   components/screenshot_capture/screenshot_capture_manager.h
//
// TODO(astra): Consider integrating this model with Chromium's screenshot
//   manager if we ever switch to the native screenshot capture pipeline.
//   Patch point: chrome/browser/screenshot/screenshot_manager.cc
// =========================================================================

// =========================================================================
// Capture mode — what kind of screenshot to capture
// =========================================================================
//
// Identifies the capture mode for the screenshot operation. Determines
// which capture path is used and which UI controls are shown.
//
// Chromium subsystems reused:
//   - content::WebContents::GetContentBitmap (visible area)
//   - chrome/browser/screenshots/ (full page, if available)
//   - Element screenshot via DevTools or element capture APIs
//   - Window capture via SkiaPixel/Widget capture
enum class AstraScreenshotMode {
  kFullPage = 0,     // Capture entire scrollable page.
  kVisibleArea = 1,  // Capture visible viewport only.
  kRegion = 2,       // Capture user-selected rectangular region.
  kWindow = 3,       // Capture entire browser window (chrome + content).
  kElement = 4,      // Capture specific DOM element (via selection).
};

// =========================================================================
// Capture format — image encoding format
// =========================================================================
//
// Image format used for screenshot encoding and file output.
//
// Chromium subsystems reused:
//   - third_party/skia (PNG, JPEG, WebP encoding via SkEncoder)
//   - ui/gfx/codec (image encoding utilities)
enum class AstraScreenshotFormat {
  kPng = 0,    // Lossless PNG — default, best quality, largest files.
  kJpeg = 1,   // Lossy JPEG — smaller files, configurable quality.
  kWebP = 2,   // WebP — lossy or lossless, best compression ratio.
};

// =========================================================================
// Capture quality — quality level for lossy formats
// =========================================================================
//
// Quality level used for lossy image formats (JPEG, WebP lossy).
// Provides discrete quality presets instead of a continuous slider.
enum class AstraScreenshotQuality {
  kLow = 0,      // Low quality, smallest file size (approx. 30%).
  kMedium = 1,   // Medium quality, balanced (approx. 60%).
  kHigh = 2,     // High quality, larger files (approx. 85%).
  kMaximum = 3,  // Maximum quality, largest files (approx. 100% / lossless).
};

// =========================================================================
// Annotation tool — tool for screenshot markup
// =========================================================================
//
// Available annotation tools for post-capture image editing.
// Each tool has its own settings (color, thickness, text size).
enum class AstraAnnotationTool {
  kNone = 0,       // No active tool (selection / view mode).
  kArrow = 1,      // Arrow annotation tool.
  kRectangle = 2,  // Rectangle / box annotation.
  kCircle = 3,     // Circle / oval annotation.
  kText = 4,       // Text annotation.
  kBlur = 5,       // Blur / pixelate tool for redacting content.
  kHighlight = 6,  // Highlighter tool (semi-transparent color).
  kCrop = 7,       // Crop tool.
  kPen = 8,        // Freehand pen / brush tool.
};

// =========================================================================
// Resize handle — identifies a resize handle position
// =========================================================================
//
// Identifies which edge or corner of the selection or crop region
// is being manipulated during resize operations.
enum class AstraResizeHandle {
  kNone = 0,
  kTopLeft = 1,
  kTop = 2,
  kTopRight = 3,
  kLeft = 4,
  kRight = 5,
  kBottomLeft = 6,
  kBottom = 7,
  kBottomRight = 8,
};

// =========================================================================
// Legacy enums (kept for backward compatibility)
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

// Image format for screenshot encoding (legacy enum, kept for compatibility).
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

  // -- New/expanded observer methods -------------------------------------

  // Called when the active capture mode changes.
  virtual void OnCaptureModeChanged(AstraScreenshotMode mode) {}

  // Called when a capture operation starts (model-passed variant).
  virtual void OnCaptureStarted(AstraScreenshotCaptureModel* model) {}

  // Called when capture progress updates (0.0 to 1.0 range).
  virtual void OnCaptureProgress(AstraScreenshotCaptureModel* model,
                                 double progress) {}

  // Called when a capture completes and is saved to a file.
  virtual void OnCaptureCompleted(AstraScreenshotCaptureModel* model,
                                  const base::FilePath& path) {}

  // Called when a capture fails with an error.
  virtual void OnCaptureFailed(AstraScreenshotCaptureModel* model,
                               const std::string& error) {}

  // Called when an annotation is added to the annotation stack.
  virtual void OnAnnotationAdded(AstraScreenshotCaptureModel* model) {}

  // Called when the undo/redo state changes (undo count, redo count).
  virtual void OnAnnotationUndoRedoChanged(
      AstraScreenshotCaptureModel* model) {}

  // Called when model settings (format, quality, delay, etc.) change.
  virtual void OnSettingsChanged(AstraScreenshotCaptureModel* model) {}

  // Called when the model is about to be destroyed.
  virtual void OnScreenshotModelShutdown(AstraScreenshotCaptureModel* model) {}

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
  // =======================================================================
  // Settings pref keys (public static constexpr)
  // =======================================================================
  //
  // These are the pref keys used for screenshot settings persistence.
  // They are declared as public static constexpr so that UI layers and
  // the settings page can reference them without magic strings.
  //
  // TODO(astra): Wire these to Chromium's PrefService registration in
  //   AstraScreenshotServiceFactory::RegisterProfilePrefs().
  //   Chromium owner: PrefRegistrySimple / PrefService
  //   Patch point: astra/browser/astra_prefs.h and astra_prefs.cc

  // Default capture mode (AstraScreenshotMode as int).
  static constexpr const char kPrefDefaultCaptureMode[] =
      "astra.screenshot.default_capture_mode";

  // Default image format (AstraScreenshotFormat as int).
  static constexpr const char kPrefDefaultFormat[] =
      "astra.screenshot.default_format";

  // Default image quality (AstraScreenshotQuality as int).
  static constexpr const char kPrefDefaultQuality[] =
      "astra.screenshot.default_quality";

  // Default save directory path (string).
  static constexpr const char kPrefDefaultSavePath[] =
      "astra.screenshot.default_save_path";

  // File name template string for generated screenshot filenames.
  static constexpr const char kPrefFileNameTemplate[] =
      "astra.screenshot.file_name_template";

  // Whether to auto-copy to clipboard after capture (bool).
  static constexpr const char kPrefAutoCopyToClipboard[] =
      "astra.screenshot.auto_copy_to_clipboard";

  // Whether to show the magnifier during region selection (bool).
  static constexpr const char kPrefShowMagnifier[] =
      "astra.screenshot.show_magnifier";

  // Whether to show grid overlay during region selection (bool).
  static constexpr const char kPrefShowGrid[] =
      "astra.screenshot.show_grid";

  // Whether to show pixel-level grid when zoomed (bool).
  static constexpr const char kPrefShowPixelGrid[] =
      "astra.screenshot.show_pixel_grid";

  // Capture delay in seconds before taking the shot (int).
  static constexpr const char kPrefCaptureDelaySeconds[] =
      "astra.screenshot.capture_delay_seconds";

  // Whether to show a notification after capture (bool).
  static constexpr const char kPrefShowNotification[] =
      "astra.screenshot.show_notification";

  // Whether to play shutter sound on capture (bool).
  static constexpr const char kPrefPlayShutterSound[] =
      "astra.screenshot.play_shutter_sound";

  // Default annotation tool (AstraAnnotationTool as int).
  static constexpr const char kPrefDefaultTool[] =
      "astra.screenshot.default_tool";

  // Default annotation tool color (SkColor as int).
  static constexpr const char kPrefDefaultToolColor[] =
      "astra.screenshot.default_tool_color";

  // Default annotation tool thickness in pixels (int).
  static constexpr const char kPrefDefaultToolThickness[] =
      "astra.screenshot.default_tool_thickness";

  // Default annotation text size in pixels (int).
  static constexpr const char kPrefAnnotationTextSize[] =
      "astra.screenshot.annotation_text_size";

  // Maximum undo steps for annotation (int).
  static constexpr const char kPrefMaxUndoSteps[] =
      "astra.screenshot.max_undo_steps";

  // Whether to include shadow in window capture (bool).
  static constexpr const char kPrefIncludeShadowInWindowCapture[] =
      "astra.screenshot.include_shadow_in_window_capture";

  // =======================================================================
  // Default values
  // =======================================================================

  static constexpr AstraScreenshotMode kDefaultCaptureMode =
      AstraScreenshotMode::kVisibleArea;
  static constexpr AstraScreenshotFormat kDefaultFormat =
      AstraScreenshotFormat::kPng;
  static constexpr AstraScreenshotQuality kDefaultQuality =
      AstraScreenshotQuality::kHigh;
  static constexpr int kDefaultCaptureDelaySeconds = 0;
  static constexpr bool kDefaultAutoCopyToClipboard = false;
  static constexpr bool kDefaultShowMagnifier = true;
  static constexpr bool kDefaultShowGrid = false;
  static constexpr bool kDefaultShowPixelGrid = false;
  static constexpr bool kDefaultShowNotification = true;
  static constexpr bool kDefaultPlayShutterSound = false;
  static constexpr AstraAnnotationTool kDefaultTool =
      AstraAnnotationTool::kNone;
  static constexpr SkColor kDefaultToolColor = SK_ColorRED;
  static constexpr int kDefaultToolThickness = 3;
  static constexpr int kDefaultAnnotationTextSize = 14;
  static constexpr int kDefaultMaxUndoSteps = 50;
  static constexpr bool kDefaultIncludeShadowInWindowCapture = false;
  static constexpr int kMinCaptureDelaySeconds = 0;
  static constexpr int kMaxCaptureDelaySeconds = 10;
  static constexpr int kMinToolThickness = 1;
  static constexpr int kMaxToolThickness = 50;
  static constexpr int kMinTextSize = 8;
  static constexpr int kMaxTextSize = 72;
  static constexpr int kMinMaxUndoSteps = 1;
  static constexpr int kMaxMaxUndoSteps = 500;

  explicit AstraScreenshotCaptureModel(PrefService* pref_service);
  ~AstraScreenshotCaptureModel();

  AstraScreenshotCaptureModel(const AstraScreenshotCaptureModel&) = delete;
  AstraScreenshotCaptureModel& operator=(const AstraScreenshotCaptureModel&) =
      delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraScreenshotCaptureModelObserver* observer);
  void RemoveObserver(AstraScreenshotCaptureModelObserver* observer);

  // -- Capture mode --------------------------------------------------------

  // Set the active capture mode.
  // Notifies observers via OnCaptureModeChanged() and OnSettingsChanged().
  void SetCaptureMode(AstraScreenshotMode mode);

  // Get the active capture mode.
  AstraScreenshotMode GetCaptureMode() const { return capture_mode_; }

  // -- Capture format ------------------------------------------------------

  // Set the output image format.
  void SetFormat(AstraScreenshotFormat format);

  // Get the output image format.
  AstraScreenshotFormat GetFormat() const { return format_; }

  // Get a list of all available formats.
  static std::vector<AstraScreenshotFormat> GetFormats();

  // -- Capture quality -----------------------------------------------------

  // Set the output quality level (for lossy formats).
  void SetQuality(AstraScreenshotQuality quality);

  // Get the output quality level.
  AstraScreenshotQuality GetQuality() const { return quality_; }

  // Get a list of all available quality levels.
  static std::vector<AstraScreenshotQuality> GetQualities();

  // Convert a quality enum to a percentage value (0-100).
  static int QualityToPercent(AstraScreenshotQuality quality);

  // -- Region selection ----------------------------------------------------

  // Set the selected region for region-mode capture.
  void SetRegion(const gfx::Rect& region);

  // Get the currently selected region.
  const gfx::Rect& GetRegion() const { return selected_region_; }

  // Whether the current region is valid (non-empty and positive size).
  bool IsRegionValid() const;

  // Reset the selected region to empty / invalid state.
  void ResetRegion();

  // -- Magnifier -----------------------------------------------------------

  // Set whether the magnifier is shown during region selection.
  void SetShowMagnifier(bool show);

  // Get whether the magnifier is shown.
  bool GetShowMagnifier() const { return show_magnifier_; }

  // -- Grid ----------------------------------------------------------------

  // Set whether the grid overlay is shown.
  void SetShowGrid(bool show);

  // Get whether the grid overlay is shown.
  bool GetShowGrid() const { return show_grid_; }

  // -- Pixel grid ----------------------------------------------------------

  // Set whether pixel-level grid is shown (when zoomed in).
  void SetShowPixelGrid(bool show);

  // Get whether pixel-level grid is shown.
  bool GetShowPixelGrid() const { return show_pixel_grid_; }

  // -- Capture delay -------------------------------------------------------

  // Set the capture delay in seconds (countdown before capture).
  // Values outside valid range are clamped.
  void SetCaptureDelay(int delay_seconds);

  // Get the capture delay in seconds.
  int GetCaptureDelay() const { return capture_delay_seconds_; }

  // -- Auto-copy to clipboard ----------------------------------------------

  // Set whether screenshots are automatically copied to clipboard.
  void SetAutoCopyToClipboard(bool auto_copy);

  // Get whether auto-copy is enabled.
  bool GetAutoCopyToClipboard() const { return auto_copy_to_clipboard_; }

  // -- Save path -----------------------------------------------------------

  // Set the default save path for screenshots.
  void SetDefaultSavePath(const base::FilePath& path);

  // Get the default save path.
  const base::FilePath& GetDefaultSavePath() const { return default_save_path_; }

  // -- File name template --------------------------------------------------

  // Set the file name template string.
  void SetFileNameTemplate(const std::string& template_str);

  // Get the file name template string.
  const std::string& GetFileNameTemplate() const {
    return file_name_template_;
  }

  // -- Capture state -------------------------------------------------------

  // Begin the capture process. Sets state to kCapturing and notifies.
  void StartCapture();

  // Cancel an in-progress capture.
  void CancelCapture();

  // Whether a capture is currently in progress.
  bool IsCapturing() const {
    return capture_state_ == AstraScreenshotCaptureState::kCapturing;
  }

  // Get the current capture progress (0.0 to 1.0).
  double GetCaptureProgress() const { return capture_progress_; }

  // Get the file path of the last successfully captured and saved screenshot.
  const base::FilePath& GetLastCapturePath() const { return last_capture_path_; }

  // Get the file size in bytes of the last capture.
  int64_t GetLastCaptureSize() const { return last_capture_size_bytes_; }

  // Get the dimensions (in pixels) of the last capture.
  gfx::Size GetLastCaptureDimensions() const {
    return last_capture_dimensions_;
  }

  // Whether the last capture operation succeeded.
  bool CaptureSucceeded() const {
    return capture_state_ == AstraScreenshotCaptureState::kReady &&
           !capture_bitmap_.isNull();
  }

  // Whether the last capture operation failed.
  bool CaptureFailed() const {
    return capture_state_ == AstraScreenshotCaptureState::kError;
  }

  // Get the last error message. Empty if no error.
  const std::string& GetError() const { return last_error_; }

  // -- Annotation tools ----------------------------------------------------

  // Set the active annotation tool.
  void SetActiveTool(AstraAnnotationTool tool);

  // Get the active annotation tool.
  AstraAnnotationTool GetActiveTool() const { return active_tool_; }

  // Set the current tool color.
  void SetToolColor(SkColor color);

  // Get the current tool color.
  SkColor GetToolColor() const { return tool_color_; }

  // Set the current tool thickness in pixels.
  void SetToolThickness(int thickness);

  // Get the current tool thickness in pixels.
  int GetToolThickness() const { return tool_thickness_; }

  // Set the annotation text size in pixels.
  void SetTextSize(int size);

  // Get the annotation text size in pixels.
  int GetTextSize() const { return text_size_; }

  // Undo the last annotation operation.
  // Returns true if an undo was performed.
  bool UndoAnnotation();

  // Redo the last undone annotation operation.
  // Returns true if a redo was performed.
  bool RedoAnnotation();

  // Whether there are annotations that can be undone.
  bool CanUndo() const { return undo_index_ > 0; }

  // Whether there are undone annotations that can be redone.
  bool CanRedo() const {
    return undo_index_ < static_cast<int>(annotations_.size());
  }

  // Clear all annotations (both undo and redo stacks).
  void ClearAnnotations();

  // Get the total number of annotations in the current stack.
  int GetAnnotationCount() const { return undo_index_; }

  // -- Legacy capture state methods (kept for compatibility) ---------------

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

  // Start a new capture (legacy overload). Sets state to kCapturing and
  // notifies observers.
  void StartCapture(AstraScreenshotType type);

  // Complete a capture with the given bitmap. Sets state to kReady.
  void CompleteCapture(const SkBitmap& bitmap,
                       AstraScreenshotType type,
                       const gfx::Rect& source_bounds);

  // Mark the current capture as failed. Sets state to kError.
  void FailCapture(const std::string& error_message);

  // Reset the model to idle state (clears bitmap, error, etc.).
  void ResetToIdle();

  // -- Legacy save / copy operations ---------------------------------------

  // Start a save operation. Sets state to kSaving.
  void StartSave();

  // Complete a save operation. Sets state back to kReady and notifies.
  void CompleteSave(const base::FilePath& path, int64_t file_size_bytes);

  // Complete a copy-to-clipboard operation. Notifies observers.
  void CompleteCopy();

  // -- Legacy region selection methods -------------------------------------

  // Selected region in viewport coordinates. Used for region-type captures.
  const gfx::Rect& selected_region() const { return selected_region_; }

  // Whether a region has been selected.
  bool has_selected_region() const { return has_selected_region_; }

  // Set the selected region from two points (start and end of a drag).
  // Normalizes the rectangle so width and height are always positive.
  void SetRegionFromPoints(const gfx::Point& start, const gfx::Point& end);

  // Set the selected region directly (legacy alias).
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

  // Clear the selected region (legacy alias).
  void ClearRegion();

  // -- Legacy aspect ratio utilities ---------------------------------------

  // Get the aspect ratio as a width/height ratio for the given lock mode.
  // Returns 0.0 for kFree (no fixed ratio).
  static double GetAspectRatioValue(AstraScreenshotAspectRatioLock mode);

  // -- Legacy capture settings (persisted via PrefService) -----------------

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

  // Image format (PNG, JPEG, WebP) — legacy enum.
  AstraScreenshotImageFormatModel GetImageFormat() const;
  void SetImageFormat(AstraScreenshotImageFormatModel format);

  // JPEG quality (1-100) — legacy continuous value.
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

  // Copy to clipboard after capture (legacy setting).
  bool GetCopyToClipboardAfterCapture() const;
  void SetCopyToClipboardAfterCapture(bool enabled);

  // Show grid in region selection (legacy setting).
  bool GetShowGridInRegionSelection() const;
  void SetShowGridInRegionSelection(bool show);

  // Show magnifier in region selection (legacy setting).
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

  // Generate a filename using the configured template.
  std::string GenerateFileNameFromTemplate() const;

  // Clamp JPEG quality to valid range (1-100).
  static int ClampJpegQuality(int quality);

  // Clamp grid size to valid range.
  static int ClampGridSize(int size);

  // Clamp max recent captures to valid range.
  static int ClampMaxRecentCaptures(int max);

  // Clamp auto-dismiss delay to valid range (1-60 seconds).
  static int ClampAutoDismissDelay(int seconds);

  // Clamp capture delay to valid range (0-10 seconds).
  static int ClampCaptureDelay(int seconds);

  // Clamp tool thickness to valid range.
  static int ClampToolThickness(int thickness);

  // Clamp text size to valid range.
  static int ClampTextSize(int size);

  // Clamp max undo steps to valid range.
  static int ClampMaxUndoSteps(int steps);

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

  // For testing: set capture progress directly.
  void SetCaptureProgressForTesting(double progress) {
    capture_progress_ = progress;
  }

  // For testing: set capture progress and notify observers.
  void UpdateCaptureProgressForTesting(double progress);

 private:
  // -- State change helpers ------------------------------------------------

  void SetCaptureState(AstraScreenshotCaptureState state);

  // Notify observers that the region changed.
  void NotifyRegionChanged();

  // Notify observers that capture settings changed.
  void NotifySettingsChanged();

  // Notify observers that capture type changed.
  void NotifyCaptureTypeChanged();

  // Notify observers that capture mode changed.
  void NotifyCaptureModeChanged();

  // Notify observers that an annotation was added.
  void NotifyAnnotationAdded();

  // Notify observers that undo/redo state changed.
  void NotifyAnnotationUndoRedoChanged();

  // Notify observers of capture progress.
  void NotifyCaptureProgress();

  // Notify observers that capture completed (file saved).
  void NotifyCaptureCompletedFile();

  // Notify observers that capture failed (model variant).
  void NotifyCaptureFailedModel();

  // Notify observers of model shutdown.
  void NotifyShutdown();

  // Notify observers of settings changed (model variant).
  void NotifySettingsChangedModel();

  // -- Helpers -------------------------------------------------------------

  // Get the aspect ratio value from the current setting.
  double GetCurrentAspectRatio() const;

  // Apply aspect ratio constraint to a resize operation.
  void ApplyAspectRatioToResize(AstraScreenshotRegionHandle handle,
                                gfx::Rect& rect,
                                const gfx::Point& fixed_point) const;

  // Apply settings from PrefService to member variables.
  void LoadSettingsFromPrefs();

  // Sync format setting: keep legacy format_legacy_ and format_ in sync.
  void SyncFormatSetting();

  // -- Members -------------------------------------------------------------

  raw_ptr<PrefService> pref_service_ = nullptr;

  // -- New expanded model state -------------------------------------------

  // Current capture mode.
  AstraScreenshotMode capture_mode_ = kDefaultCaptureMode;

  // Output format.
  AstraScreenshotFormat format_ = kDefaultFormat;

  // Output quality.
  AstraScreenshotQuality quality_ = kDefaultQuality;

  // Capture delay in seconds.
  int capture_delay_seconds_ = kDefaultCaptureDelaySeconds;

  // Whether to show magnifier during region selection.
  bool show_magnifier_ = kDefaultShowMagnifier;

  // Whether to show grid overlay.
  bool show_grid_ = kDefaultShowGrid;

  // Whether to show pixel-level grid.
  bool show_pixel_grid_ = kDefaultShowPixelGrid;

  // Whether to auto-copy to clipboard.
  bool auto_copy_to_clipboard_ = kDefaultAutoCopyToClipboard;

  // Default save directory path.
  base::FilePath default_save_path_;

  // File name template string.
  std::string file_name_template_;

  // Capture progress (0.0 to 1.0).
  double capture_progress_ = 0.0;

  // Path of the last saved capture.
  base::FilePath last_capture_path_;

  // File size in bytes of the last capture.
  int64_t last_capture_size_bytes_ = 0;

  // Dimensions of the last capture.
  gfx::Size last_capture_dimensions_;

  // -- Annotation state ---------------------------------------------------

  // Currently active annotation tool.
  AstraAnnotationTool active_tool_ = kDefaultTool;

  // Current tool color.
  SkColor tool_color_ = kDefaultToolColor;

  // Current tool thickness in pixels.
  int tool_thickness_ = kDefaultToolThickness;

  // Annotation text size in pixels.
  int text_size_ = kDefaultAnnotationTextSize;

  // Maximum undo steps.
  int max_undo_steps_ = kDefaultMaxUndoSteps;

  // Annotation stack (opaque identifiers for each annotation).
  // In a real implementation these would be Skia draw commands or
  // annotation objects. Here we use integer IDs for state tracking.
  std::vector<int> annotations_;

  // Current position in the annotation stack (undo index).
  // 0 = nothing to undo, annotations_.size() = nothing to redo.
  int undo_index_ = 0;

  // Counter for generating unique annotation IDs.
  int next_annotation_id_ = 1;

  // -- Legacy state -------------------------------------------------------

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

  // Legacy image format (kept in sync with format_).
  AstraScreenshotImageFormatModel format_legacy_ =
      AstraScreenshotImageFormatModel::kPng;

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
