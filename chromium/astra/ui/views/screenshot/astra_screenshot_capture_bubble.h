#ifndef ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_
#define ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "third_party/skia/include/core/SkBitmap.h"
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
}  // namespace views

namespace astra {

class AstraScreenshotService;

// =========================================================================
// Astra screenshot capture bubble
// =========================================================================
//
// Bubble shown after a screenshot is captured. Provides a quick preview
// of the captured image along with action buttons and settings:
//   - Save to downloads
//   - Copy to clipboard
//   - Edit (opens image editor, stub)
//   - Share (opens share menu, stub)
//   - Discard
//   - Keep (cancel auto-dismiss)
//
// Presentation settings:
//   - Image format selector (PNG / JPEG / WebP)
//   - JPEG quality slider
//   - Copy-to-clipboard toggle
//   - Filename editable field
//
// Visual states:
//   - Loading / placeholder: shown while the capture is in progress
//   - Ready: default state with preview and action buttons
//   - In-progress: saving or copying, shows progress indicator
//   - Success: action completed, shows confirmation
//   - Error: action failed, shows error message
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
//   chrome/browser/ui/views/screenshot/screenshot_bubble.h
//
// TODO(astra): Integrate with Chromium's share/screenshot UI system.
//   Chromium owner: Share bubble / screenshot preview UI
//   (chrome/browser/share/, chrome/browser/screenshot/)
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

   protected:
    ~Delegate() = default;
  };

  // State of the bubble's action buttons and status indicator.
  enum class State {
    kReady,       // Default: all actions available
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

  ~AstraScreenshotCaptureBubble() override;

  AstraScreenshotCaptureBubble(const AstraScreenshotCaptureBubble&) = delete;
  AstraScreenshotCaptureBubble& operator=(
      const AstraScreenshotCaptureBubble&) = delete;

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

  // Update the quality slider visibility based on image format.
  void UpdateQualitySliderVisibility();

  // -- Formatting helpers ------------------------------------------------

  // Format a file size in bytes into a human-readable string.
  std::u16string FormatFileSize(int64_t bytes) const;

  // Format dimensions string like "1920 x 1080".
  std::u16string FormatDimensions(int width, int height) const;

  // Get a user-facing label for the capture type.
  std::u16string GetCaptureTypeLabel() const;

  // Generate a default filename for the screenshot.
  std::u16string GenerateDefaultFilename() const;

  // Estimate the file size based on the bitmap dimensions and settings.
  int64_t EstimateFileSize() const;

  // -- Settings sync helpers ---------------------------------------------

  // Apply model settings to the UI controls.
  void ApplySettingsFromModel();

  // -- Button handlers ---------------------------------------------------

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

  // -- Child views (owned by the view hierarchy) ------------------------

  raw_ptr<views::ImageView> preview_image_ = nullptr;
  raw_ptr<views::Label> placeholder_label_ = nullptr;
  raw_ptr<views::Throbber> preview_throbber_ = nullptr;

  raw_ptr<views::Textfield> filename_field_ = nullptr;
  raw_ptr<views::Label> dimensions_label_ = nullptr;
  raw_ptr<views::Label> file_size_label_ = nullptr;
  raw_ptr<views::Label> capture_type_label_ = nullptr;

  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::Throbber> status_throbber_ = nullptr;

  raw_ptr<views::MdTextButton> save_button_ = nullptr;
  raw_ptr<views::MdTextButton> copy_button_ = nullptr;
  raw_ptr<views::MdTextButton> edit_button_ = nullptr;
  raw_ptr<views::MdTextButton> share_button_ = nullptr;
  raw_ptr<views::MdTextButton> delete_button_ = nullptr;
  raw_ptr<views::MdTextButton> keep_button_ = nullptr;
  raw_ptr<views::MdTextButton> open_in_editor_button_ = nullptr;

  raw_ptr<views::Combobox> format_combobox_ = nullptr;
  raw_ptr<views::Slider> quality_slider_ = nullptr;
  raw_ptr<views::Label> quality_label_ = nullptr;
  raw_ptr<views::ToggleButton> copy_toggle_ = nullptr;
  raw_ptr<views::Label> copy_toggle_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SCREENSHOT_ASTRA_SCREENSHOT_CAPTURE_BUBBLE_H_
