#ifndef ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_
#define ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

#include "astra/ui/views/screenshot/astra_screenshot_capture_model.h"

class Browser;

namespace views {
class ImageView;
class Label;
class MdTextButton;
class Throbber;
class Textfield;
class ToggleButton;
class Combobox;
class Slider;
class View;
}  // namespace views

namespace astra {

class AstraScreenshotService;

// =========================================================================
// Astra screenshot capture bubble
// =========================================================================
//
// The capture control bubble / toolbar shown during and after screenshot
// capture. Provides capture mode selection, format/quality settings,
// capture delay, annotation tools, and action buttons (copy, save, share).
//
// Bubble sections:
//   - Mode selector (full page, visible, region, window, element)
//   - Format selector (PNG, JPEG, WebP)
//   - Quality selector
//   - Delay selector
//   - Capture button
//   - Cancel button
//   - Annotation tools (collapsible)
//   - Copy/Save/Share actions
//
// Also shown after a screenshot is captured as a quick preview with:
//   - Preview image
//   - Save to downloads
//   - Copy to clipboard
//   - Edit
//   - Share
//   - Discard
//
// Model/view pattern: The bubble observes an AstraScreenshotCaptureModel
// for state changes. User actions are dispatched to the model and/or
// the Delegate interface.
//
// Auto-dismisses after configurable delay if the user doesn't interact.
// A "Keep" button cancels the auto-dismiss.
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
//   (ui/views/bubble/bubble_dialog_delegate_view.h)
//
// Chromium pattern reference:
//   chrome/browser/ui/views/share/share_bubble_view.h
//   chrome/browser/ui/views/screenshots/screenshot_bubble.h
//
// TODO(astra): Integrate with Chromium's share/screenshot UI system.
//   Chromium owner: Share bubble / screenshot preview UI
//   (chrome/browser/share/, chrome/browser/screenshots/)
//   Patch point: ShareManager or ScreenshotManager UI delegate.
// =========================================================================

class AstraScreenshotCaptureBubble
    : public views::BubbleDialogDelegateView,
      public AstraScreenshotCaptureModelObserver {
 public:
  // Delegate interface for bubble actions.
  // Implemented by the owner (e.g. AstraBrowserView) to handle user
  // actions from the capture bubble.
  class Delegate {
   public:
    // Called when the user clicks "Save to downloads".
    virtual void OnScreenshotSave() = 0;

    // Called when the user clicks "Copy to clipboard".
    virtual void OnScreenshotCopy() = 0;

    // Called when the user clicks "Edit".
    virtual void OnScreenshotEdit() = 0;

    // Called when the user clicks "Delete" or discards the capture.
    virtual void OnScreenshotDelete() = 0;

    // Called when the user clicks "Share".
    virtual void OnScreenshotShare() = 0;

    // Called when the bubble is about to close (for any reason).
    virtual void OnScreenshotBubbleClosed() = 0;

    // Called when the user clicks "Open in Editor".
    virtual void OnScreenshotOpenInEditor() = 0;

    // Called when the user starts a capture from the bubble.
    virtual void OnScreenshotCaptureStarted() = 0;

    // Called when the user cancels a capture from the bubble.
    virtual void OnScreenshotCaptureCancelled() = 0;

   protected:
    ~Delegate() = default;
  };

  // State of the bubble's action buttons and status indicator.
  enum class State {
    kReady,       // Default: all actions available
    kCapturing,   // Capture in progress
    kSaving,      // Save in progress
    kCopying,     // Copy in progress
    kSuccess,     // Last action succeeded
    kError,       // Last action failed
  };

  // Creates and shows the screenshot capture bubble anchored to
  // |anchor_view|. Returns the bubble widget (owned by the native
  // widget system).
  //
  // |bitmap| is the captured screenshot image. Can be empty for a
  //   placeholder / loading state.
  // |capture_type| indicates what kind of capture this is.
  // |source_bounds| is the region of the page that was captured.
  // |delegate| receives action callbacks.
  // |model| optional capture model to observe for state changes.
  //
  // TODO(astra): Track existing bubble instance to avoid duplicates.
  //   Currently callers (AstraBrowserView) track the widget pointer.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   Browser* browser,
                                   const SkBitmap& bitmap,
                                   AstraScreenshotType capture_type,
                                   const gfx::Rect& source_bounds,
                                   Delegate* delegate,
                                   AstraScreenshotCaptureModel* model = nullptr);

  // Show the bubble anchored to |anchor_rect|.
  static views::Widget* Show(const gfx::Rect& anchor_rect,
                             Browser* browser,
                             Delegate* delegate,
                             AstraScreenshotCaptureModel* model = nullptr);

  ~AstraScreenshotCaptureBubble() override;

  AstraScreenshotCaptureBubble(const AstraScreenshotCaptureBubble&) = delete;
  AstraScreenshotCaptureBubble& operator=(
      const AstraScreenshotCaptureBubble&) = delete;

  // -- Visibility ----------------------------------------------------------

  // Hide the bubble (closes the widget).
  void Hide();

  // Whether the bubble is currently visible.
  bool IsVisible() const;

  // -- Model management ----------------------------------------------------

  // Get the associated capture model.
  AstraScreenshotCaptureModel* GetModel() const { return model_; }

  // Set the associated capture model.
  void SetModel(AstraScreenshotCaptureModel* model);

  // -- Capture mode --------------------------------------------------------

  // Set the active capture mode.
  void SetCaptureMode(AstraScreenshotMode mode);

  // Get the active capture mode.
  AstraScreenshotMode GetCaptureMode() const { return capture_mode_; }

  // -- Format / quality ----------------------------------------------------

  // Set the output image format.
  void SetFormat(AstraScreenshotFormat format);

  // Get the output image format.
  AstraScreenshotFormat GetFormat() const { return format_; }

  // Set the output quality level.
  void SetQuality(AstraScreenshotQuality quality);

  // Get the output quality level.
  AstraScreenshotQuality GetQuality() const { return quality_; }

  // -- Capture delay -------------------------------------------------------

  // Set the capture delay in seconds.
  void SetCaptureDelay(int delay_seconds);

  // Get the capture delay in seconds.
  int GetCaptureDelay() const { return capture_delay_seconds_; }

  // -- Capture operations --------------------------------------------------

  // Start the capture process.
  void StartCapture();

  // Cancel the capture process.
  void CancelCapture();

  // Whether a capture is currently in progress.
  bool IsCapturing() const { return state_ == State::kCapturing; }

  // -- Annotation tools ----------------------------------------------------

  // Set whether annotation tools are visible/expanded.
  void SetShowAnnotationTools(bool show);

  // Get whether annotation tools are visible/expanded.
  bool GetShowAnnotationTools() const { return show_annotation_tools_; }

  // Set the active annotation tool.
  void SetActiveTool(AstraAnnotationTool tool);

  // Get the active annotation tool.
  AstraAnnotationTool GetActiveTool() const { return active_tool_; }

  // Undo the last annotation.
  void UndoAnnotation();

  // Redo the last undone annotation.
  void RedoAnnotation();

  // -- Post-capture actions ------------------------------------------------

  // Copy the screenshot to clipboard.
  void CopyToClipboard();

  // Save the screenshot to file.
  void SaveToFile();

  // Share the screenshot (opens share sheet).
  void Share();

  // -- Legacy post-capture API ---------------------------------------------

  // Update the displayed file size after the save completes.
  void UpdateFileSize(int64_t file_size_bytes);

  // Update the displayed filename.
  void UpdateFilename(const std::u16string& filename);

  // Mark that the "copy to clipboard" action completed successfully.
  void SetCopyCompleted();

  // Mark that the "save to downloads" action completed successfully.
  void SetSaveCompleted();

  // Show an error state with the given message.
  void SetError(const std::u16string& error_message);

  // Update the preview bitmap. Used when the capture finishes loading.
  void SetPreviewBitmap(const SkBitmap& bitmap);

  // -- AstraScreenshotCaptureModelObserver --------------------------------

  void OnCaptureStarted() override;
  void OnCaptureCompleted(const SkBitmap& bitmap) override;
  void OnCaptureFailed(const std::string& error) override;
  void OnSaveCompleted(const base::FilePath& path) override;
  void OnCopyCompleted() override;
  void OnCaptureSettingsChanged() override;
  void OnCaptureStateChanged(AstraScreenshotCaptureState state) override;
  void OnCaptureModeChanged(AstraScreenshotMode mode) override;
  void OnCaptureProgress(AstraScreenshotCaptureModel* model,
                         double progress) override;
  void OnAnnotationAdded(AstraScreenshotCaptureModel* model) override;
  void OnAnnotationUndoRedoChanged(
      AstraScreenshotCaptureModel* model) override;
  void OnSettingsChanged(AstraScreenshotCaptureModel* model) override;
  void OnScreenshotModelShutdown(AstraScreenshotCaptureModel* model) override;

  // -- views::BubbleDialogDelegateView -----------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnThemeChanged() override;

 private:
  AstraScreenshotCaptureBubble(views::View* anchor_view,
                               Browser* browser,
                               const SkBitmap& bitmap,
                               AstraScreenshotType capture_type,
                               const gfx::Rect& source_bounds,
                               Delegate* delegate,
                               AstraScreenshotCaptureModel* model);

  // Build the bubble contents.
  void Init() override;

  // Get text for the accessible name of the bubble.
  std::u16string GetAccessibleName() const;

  // -- Auto-dismiss timer ------------------------------------------------

  // Start the auto-dismiss timer.
  void StartAutoDismissTimer();

  // Reset the auto-dismiss timer (e.g., when the user hovers).
  void ResetAutoDismissTimer();

  // Cancel the auto-dismiss timer (e.g., when the user clicks Keep).
  void CancelAutoDismissTimer();

  // Called when the auto-dismiss timer fires. Closes the bubble.
  void OnAutoDismissTimerFired();

  // -- State helpers -----------------------------------------------------

  // Set the current action state and update the UI accordingly.
  void SetState(State state);

  // Update the enabled state of action buttons based on current state.
  void UpdateButtonStates();

  // Update the status label and throbber visibility based on current state.
  void UpdateStatusIndicator();

  // Refresh colors from the color provider.
  void UpdateColors();

  // Update visibility of info labels based on settings.
  void UpdateInfoVisibility();

  // Update the quality selector visibility based on image format.
  void UpdateQualityVisibility();

  // Update annotation tools section visibility.
  void UpdateAnnotationToolsVisibility();

  // Update capture mode selector from model.
  void UpdateModeSelectorFromModel();

  // -- Formatting helpers ------------------------------------------------

  // Format a file size in bytes into a human-readable string.
  std::u16string FormatFileSize(int64_t bytes) const;

  // Format dimensions string like "1920 x 1080".
  std::u16string FormatDimensions(int width, int height) const;

  // Get a user-facing label for the capture type.
  std::u16string GetCaptureTypeLabel() const;

  // Get a user-facing label for the capture mode.
  std::u16string GetCaptureModeLabel() const;

  // Generate a default filename for the screenshot.
  std::u16string GenerateDefaultFilename() const;

  // Estimate the file size based on the bitmap dimensions and settings.
  int64_t EstimateFileSize() const;

  // -- Settings sync helpers ---------------------------------------------

  // Apply model settings to the UI controls.
  void ApplySettingsFromModel();

  // -- Button handlers ---------------------------------------------------

  void OnCaptureButtonClicked();
  void OnCancelButtonClicked();
  void OnSaveButtonClicked();
  void OnCopyButtonClicked();
  void OnEditButtonClicked();
  void OnDeleteButtonClicked();
  void OnShareButtonClicked();
  void OnKeepButtonClicked();
  void OnOpenInEditorClicked();
  void OnCopyToggleToggled();
  void OnFormatChanged();
  void OnQualityChanged();
  void OnFilenameChanged();
  void OnModeChanged();
  void OnDelayChanged();
  void OnAnnotationToolsToggled();
  void OnToolSelected(AstraAnnotationTool tool);
  void OnUndoClicked();
  void OnRedoClicked();
  void OnShareClicked();

  // -- Members -----------------------------------------------------------

  raw_ptr<Browser> browser_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // Optional capture model to observe.
  raw_ptr<AstraScreenshotCaptureModel> model_ = nullptr;

  // The captured screenshot bitmap. Owned by the bubble for the duration
  // of its display. The caller (service) retains ownership of the source
  // bitmap; we make a copy for display.
  SkBitmap bitmap_;

  // Type of capture that produced this screenshot.
  AstraScreenshotType capture_type_;

  // The region of the page that was captured (in page/viewport coordinates).
  gfx::Rect source_bounds_;

  // Current capture mode (full page, visible, region, window, element).
  AstraScreenshotMode capture_mode_ = AstraScreenshotMode::kVisibleArea;

  // Current output format.
  AstraScreenshotFormat format_ = AstraScreenshotFormat::kPng;

  // Current output quality.
  AstraScreenshotQuality quality_ = AstraScreenshotQuality::kHigh;

  // Capture delay in seconds.
  int capture_delay_seconds_ = 0;

  // Estimated or actual file size in bytes.
  int64_t file_size_bytes_ = 0;

  // Display filename.
  std::u16string filename_;

  // Current action state of the bubble.
  State state_ = State::kReady;

  // Error message shown when state_ == State::kError.
  std::u16string error_message_;

  // Auto-dismiss delay.
  base::TimeDelta auto_dismiss_delay_ = base::Seconds(5);

  // Whether auto-dismiss has been explicitly kept / cancelled.
  bool auto_dismiss_kept_ = false;

  // Timer for auto-dismiss.
  base::OneShotTimer auto_dismiss_timer_;

  // Whether annotation tools section is expanded.
  bool show_annotation_tools_ = false;

  // Currently active annotation tool.
  AstraAnnotationTool active_tool_ = AstraAnnotationTool::kNone;

  // -- Child views (owned by the view hierarchy) ------------------------

  // Preview section.
  raw_ptr<views::ImageView> preview_image_ = nullptr;
  raw_ptr<views::Label> placeholder_label_ = nullptr;
  raw_ptr<views::Throbber> preview_throbber_ = nullptr;

  // Info section.
  raw_ptr<views::Textfield> filename_field_ = nullptr;
  raw_ptr<views::Label> dimensions_label_ = nullptr;
  raw_ptr<views::Label> file_size_label_ = nullptr;
  raw_ptr<views::Label> capture_type_label_ = nullptr;

  // Status section.
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::Throbber> status_throbber_ = nullptr;

  // Capture controls section.
  raw_ptr<views::Combobox> mode_combobox_ = nullptr;
  raw_ptr<views::Combobox> format_combobox_ = nullptr;
  raw_ptr<views::Combobox> quality_combobox_ = nullptr;
  raw_ptr<views::Combobox> delay_combobox_ = nullptr;
  raw_ptr<views::MdTextButton> capture_button_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;

  // Action buttons.
  raw_ptr<views::MdTextButton> save_button_ = nullptr;
  raw_ptr<views::MdTextButton> copy_button_ = nullptr;
  raw_ptr<views::MdTextButton> edit_button_ = nullptr;
  raw_ptr<views::MdTextButton> share_button_ = nullptr;
  raw_ptr<views::MdTextButton> delete_button_ = nullptr;
  raw_ptr<views::MdTextButton> keep_button_ = nullptr;
  raw_ptr<views::MdTextButton> open_in_editor_button_ = nullptr;

  // Settings controls.
  raw_ptr<views::Slider> quality_slider_ = nullptr;
  raw_ptr<views::Label> quality_label_ = nullptr;
  raw_ptr<views::ToggleButton> copy_toggle_ = nullptr;
  raw_ptr<views::Label> copy_toggle_label_ = nullptr;

  // Annotation tools section.
  raw_ptr<views::View> annotation_tools_section_ = nullptr;
  raw_ptr<views::MdTextButton> annotation_toggle_button_ = nullptr;
  raw_ptr<views::MdTextButton> undo_button_ = nullptr;
  raw_ptr<views::MdTextButton> redo_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_
