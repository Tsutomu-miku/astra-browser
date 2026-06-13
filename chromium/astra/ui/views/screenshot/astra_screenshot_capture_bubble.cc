#include "astra/ui/views/screenshot/astra_screenshot_capture_bubble.h"

#include <memory>
#include <string>
#include <utility>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "astra/browser/astra_screenshot_service.h"
#include "astra/ui/color/astra_color_ids.h"
#include "chrome/browser/ui/browser.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/throbber.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Bubble sizing constants.
constexpr int kBubbleWidth = 360;
constexpr int kPreviewMaxHeight = 180;
constexpr int kButtonRowSpacing = 8;
constexpr int kBubbleInteriorPadding = 16;
constexpr int kPreviewBottomPadding = 12;
constexpr int kInfoRowSpacing = 6;
constexpr int kStatusRowSpacing = 8;
constexpr int kSettingsRowSpacing = 10;
constexpr int kSettingsSectionSpacing = 12;

// Format helper: human-readable file size.
// Uses IEC units (1 KB = 1024 bytes) which matches Chrome's download UI.
std::u16string FormatFileSize(int64_t bytes) {
  constexpr int64_t kKB = 1024;
  constexpr int64_t kMB = kKB * 1024;
  constexpr int64_t kGB = kMB * 1024;

  if (bytes >= kGB) {
    double gb = static_cast<double>(bytes) / kGB;
    return base::NumberToString16(base::StringPrintf("%.1f GB", gb));
  }
  if (bytes >= kMB) {
    double mb = static_cast<double>(bytes) / kMB;
    return base::NumberToString16(base::StringPrintf("%.1f MB", mb));
  }
  if (bytes >= kKB) {
    double kb = static_cast<double>(bytes) / kKB;
    return base::NumberToString16(base::StringPrintf("%.0f KB", kb));
  }
  return base::NumberToString16(bytes) + u" B";
}

// Get extension for a given image format.
const char* GetFormatExtension(AstraScreenshotFormat format) {
  switch (format) {
    case AstraScreenshotFormat::kPng:
      return "png";
    case AstraScreenshotFormat::kJpeg:
      return "jpg";
    case AstraScreenshotFormat::kWebP:
      return "webp";
  }
  return "png";
}

// Get the label for a capture mode.
std::u16string GetCaptureModeLabel(AstraScreenshotMode mode) {
  switch (mode) {
    case AstraScreenshotMode::kFullPage:
      return u"Full Page";
    case AstraScreenshotMode::kVisibleArea:
      return u"Visible Area";
    case AstraScreenshotMode::kRegion:
      return u"Region";
    case AstraScreenshotMode::kWindow:
      return u"Window";
    case AstraScreenshotMode::kElement:
      return u"Element";
  }
  return u"Screenshot";
}

// Get the label for a format.
std::u16string GetFormatLabel(AstraScreenshotFormat format) {
  switch (format) {
    case AstraScreenshotFormat::kPng:
      return u"PNG";
    case AstraScreenshotFormat::kJpeg:
      return u"JPEG";
    case AstraScreenshotFormat::kWebP:
      return u"WebP";
  }
  return u"PNG";
}

// Get the label for a quality level.
std::u16string GetQualityLabel(AstraScreenshotQuality quality) {
  switch (quality) {
    case AstraScreenshotQuality::kLow:
      return u"Low";
    case AstraScreenshotQuality::kMedium:
      return u"Medium";
    case AstraScreenshotQuality::kHigh:
      return u"High";
    case AstraScreenshotQuality::kMaximum:
      return u"Maximum";
  }
  return u"High";
}

// Get the label for a delay in seconds.
std::u16string GetDelayLabel(int seconds) {
  if (seconds == 0) return u"None";
  if (seconds == 1) return u"1 second";
  return base::NumberToString16(seconds) + u" seconds";
}

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraScreenshotCaptureBubble::ShowBubble(
    views::View* anchor_view,
    Browser* browser,
    const SkBitmap& bitmap,
    AstraScreenshotType capture_type,
    const gfx::Rect& source_bounds,
    Delegate* delegate,
    AstraScreenshotCaptureModel* model) {
  DCHECK(anchor_view);
  DCHECK(browser);

  auto* bubble = new AstraScreenshotCaptureBubble(
      anchor_view, browser, bitmap, capture_type, source_bounds, delegate,
      model);

  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::BOTTOM_RIGHT);

  widget->Show();

  return widget;
}

// static
views::Widget* AstraScreenshotCaptureBubble::Show(
    const gfx::Rect& anchor_rect,
    Browser* browser,
    Delegate* delegate,
    AstraScreenshotCaptureModel* model) {
  DCHECK(browser);
  DCHECK(delegate);

  // Create an empty bitmap for the capture-mode bubble.
  SkBitmap empty_bitmap;

  // Create a dummy anchor view from the rect.
  // TODO(astra): Use BubbleDialogDelegateView with anchor rect properly.
  //   For now we create a temporary view as anchor.
  auto* anchor_view = new views::View();
  anchor_view->SetBoundsRect(anchor_rect);

  auto* bubble = new AstraScreenshotCaptureBubble(
      anchor_view, browser, empty_bitmap,
      static_cast<AstraScreenshotType>(
          model ? static_cast<int>(model->GetCaptureMode()) : 1),
      anchor_rect, delegate, model);

  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::BOTTOM_RIGHT);

  widget->Show();

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraScreenshotCaptureBubble::AstraScreenshotCaptureBubble(
    views::View* anchor_view,
    Browser* browser,
    const SkBitmap& bitmap,
    AstraScreenshotType capture_type,
    const gfx::Rect& source_bounds,
    Delegate* delegate,
    AstraScreenshotCaptureModel* model)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::BOTTOM_RIGHT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser),
      delegate_(delegate),
      model_(model),
      bitmap_(bitmap),
      capture_type_(capture_type),
      source_bounds_(source_bounds),
      file_size_bytes_(EstimateFileSize()),
      filename_(GenerateDefaultFilename()) {
  // Set bubble properties.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(true);
  SetTitle(u"Screenshot");
  SetShowCloseButton(true);
  set_fixed_width(kBubbleWidth);

  set_close_on_deactivate(false);

  SetAccessibleRole(ax::mojom::Role::kDialog);
  SetAccessibleName(GetAccessibleName());

  // Load settings from model if available.
  if (model_) {
    capture_mode_ = model_->GetCaptureMode();
    format_ = model_->GetFormat();
    quality_ = model_->GetQuality();
    capture_delay_seconds_ = model_->GetCaptureDelay();
    active_tool_ = model_->GetActiveTool();
    auto_dismiss_delay_ = base::Seconds(model_->GetAutoDismissDelaySeconds());
    auto_dismiss_kept_ = !model_->GetAutoDismissBubble();
  } else {
    auto_dismiss_delay_ = base::Seconds(5);
  }
}

AstraScreenshotCaptureBubble::~AstraScreenshotCaptureBubble() {
  if (model_) {
    model_->RemoveObserver(this);
  }
  if (delegate_) {
    delegate_->OnScreenshotBubbleClosed();
  }
}

// =========================================================================
// Visibility
// =========================================================================

void AstraScreenshotCaptureBubble::Hide() {
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

bool AstraScreenshotCaptureBubble::IsVisible() const {
  return GetWidget() && GetWidget()->IsVisible();
}

// =========================================================================
// Model management
// =========================================================================

void AstraScreenshotCaptureBubble::SetModel(AstraScreenshotCaptureModel* model) {
  if (model_ == model) return;

  if (model_) {
    model_->RemoveObserver(this);
  }
  model_ = model;
  if (model_) {
    model_->AddObserver(this);
    ApplySettingsFromModel();
  }
}

// =========================================================================
// Capture mode
// =========================================================================

void AstraScreenshotCaptureBubble::SetCaptureMode(AstraScreenshotMode mode) {
  if (capture_mode_ == mode) return;
  capture_mode_ = mode;

  if (model_) {
    model_->SetCaptureMode(mode);
  }

  if (mode_combobox_) {
    mode_combobox_->SetSelectedIndex(static_cast<int>(mode));
  }
}

// =========================================================================
// Format / quality
// =========================================================================

void AstraScreenshotCaptureBubble::SetFormat(AstraScreenshotFormat format) {
  if (format_ == format) return;
  format_ = format;

  if (model_) {
    model_->SetFormat(format);
  }

  if (format_combobox_) {
    format_combobox_->SetSelectedIndex(static_cast<int>(format));
  }
  UpdateQualityVisibility();

  // Re-estimate file size.
  file_size_bytes_ = EstimateFileSize();
  if (file_size_label_) {
    file_size_label_->SetText(FormatFileSize(file_size_bytes_));
  }
}

void AstraScreenshotCaptureBubble::SetQuality(AstraScreenshotQuality quality) {
  if (quality_ == quality) return;
  quality_ = quality;

  if (model_) {
    model_->SetQuality(quality);
  }

  if (quality_combobox_) {
    quality_combobox_->SetSelectedIndex(static_cast<int>(quality));
  }

  // Re-estimate file size.
  file_size_bytes_ = EstimateFileSize();
  if (file_size_label_) {
    file_size_label_->SetText(FormatFileSize(file_size_bytes_));
  }
}

// =========================================================================
// Capture delay
// =========================================================================

void AstraScreenshotCaptureBubble::SetCaptureDelay(int delay_seconds) {
  if (capture_delay_seconds_ == delay_seconds) return;
  capture_delay_seconds_ = delay_seconds;

  if (model_) {
    model_->SetCaptureDelay(delay_seconds);
  }

  if (delay_combobox_) {
    // Find the index matching the delay.
    for (int i = 0; i <= 10; i++) {
      if (i == delay_seconds) {
        delay_combobox_->SetSelectedIndex(i);
        break;
      }
    }
  }
}

// =========================================================================
// Capture operations
// =========================================================================

void AstraScreenshotCaptureBubble::StartCapture() {
  if (state_ == State::kCapturing) return;

  SetState(State::kCapturing);

  if (model_) {
    model_->StartCapture();
  }

  if (delegate_) {
    delegate_->OnScreenshotCaptureStarted();
  }

  if (placeholder_label_) {
    placeholder_label_->SetVisible(true);
    placeholder_label_->SetText(u"Capturing...");
  }
  if (preview_image_) {
    preview_image_->SetVisible(false);
  }
  if (preview_throbber_) {
    preview_throbber_->SetVisible(true);
    preview_throbber_->Start();
  }
}

void AstraScreenshotCaptureBubble::CancelCapture() {
  if (state_ != State::kCapturing) return;

  SetState(State::kReady);

  if (model_) {
    model_->CancelCapture();
  }

  if (delegate_) {
    delegate_->OnScreenshotCaptureCancelled();
  }

  if (preview_throbber_) {
    preview_throbber_->Stop();
    preview_throbber_->SetVisible(false);
  }
}

// =========================================================================
// Annotation tools
// =========================================================================

void AstraScreenshotCaptureBubble::SetShowAnnotationTools(bool show) {
  if (show_annotation_tools_ == show) return;
  show_annotation_tools_ = show;
  UpdateAnnotationToolsVisibility();
}

void AstraScreenshotCaptureBubble::SetActiveTool(AstraAnnotationTool tool) {
  if (active_tool_ == tool) return;
  active_tool_ = tool;

  if (model_) {
    model_->SetActiveTool(tool);
  }

  // Update tool button states.
  // TODO(astra): Update individual tool button states.
}

void AstraScreenshotCaptureBubble::UndoAnnotation() {
  if (model_) {
    model_->UndoAnnotation();
  }
}

void AstraScreenshotCaptureBubble::RedoAnnotation() {
  if (model_) {
    model_->RedoAnnotation();
  }
}

// =========================================================================
// Post-capture actions
// =========================================================================

void AstraScreenshotCaptureBubble::CopyToClipboard() {
  if (!delegate_) return;
  CancelAutoDismissTimer();
  SetState(State::kCopying);
  delegate_->OnScreenshotCopy();
}

void AstraScreenshotCaptureBubble::SaveToFile() {
  if (!delegate_) return;
  CancelAutoDismissTimer();
  SetState(State::kSaving);
  delegate_->OnScreenshotSave();
}

void AstraScreenshotCaptureBubble::Share() {
  if (!delegate_) return;
  CancelAutoDismissTimer();
  delegate_->OnScreenshotShare();
}

// =========================================================================
// Legacy post-capture API
// =========================================================================

void AstraScreenshotCaptureBubble::UpdateFileSize(int64_t file_size_bytes) {
  file_size_bytes_ = file_size_bytes;
  if (file_size_label_) {
    file_size_label_->SetText(FormatFileSize(file_size_bytes_));
  }
}

void AstraScreenshotCaptureBubble::UpdateFilename(
    const std::u16string& filename) {
  filename_ = filename;
  if (filename_field_) {
    filename_field_->SetText(filename_);
  }
}

void AstraScreenshotCaptureBubble::SetCopyCompleted() {
  SetState(State::kSuccess);
  if (copy_button_) {
    copy_button_->SetText(u"Copied!");
    copy_button_->SetEnabled(false);
  }
  if (status_label_) {
    status_label_->SetText(u"Image copied to clipboard");
    status_label_->SetVisible(true);
  }
}

void AstraScreenshotCaptureBubble::SetSaveCompleted() {
  SetState(State::kSuccess);
  if (save_button_) {
    save_button_->SetText(u"Saved");
    save_button_->SetEnabled(false);
  }
  if (status_label_) {
    status_label_->SetText(u"Image saved to Downloads");
    status_label_->SetVisible(true);
  }
}

void AstraScreenshotCaptureBubble::SetError(const std::u16string& error_message) {
  error_message_ = error_message;
  SetState(State::kError);
  if (status_label_) {
    status_label_->SetText(error_message_);
    status_label_->SetVisible(true);
  }
}

void AstraScreenshotCaptureBubble::SetPreviewBitmap(const SkBitmap& bitmap) {
  bitmap_ = bitmap;
  file_size_bytes_ = EstimateFileSize();

  if (preview_image_) {
    gfx::Size preview_size(bitmap_.width(), bitmap_.height());
    if (preview_size.height() > kPreviewMaxHeight) {
      float scale =
          static_cast<float>(kPreviewMaxHeight) / preview_size.height();
      preview_size.set_width(static_cast<int>(preview_size.width() * scale));
      preview_size.set_height(kPreviewMaxHeight);
    }
    int max_width = kBubbleWidth - kBubbleInteriorPadding * 2;
    if (preview_size.width() > max_width) {
      float scale = static_cast<float>(max_width) / preview_size.width();
      preview_size.set_width(max_width);
      preview_size.set_height(
          static_cast<int>(preview_size.height() * scale));
    }

    gfx::ImageSkia image = gfx::ImageSkia::CreateFrom1xBitmap(bitmap_);
    preview_image_->SetImage(image);
    preview_image_->SetImageSize(preview_size);
    preview_image_->SetVisible(true);
  }

  if (placeholder_label_) {
    placeholder_label_->SetVisible(false);
  }
  if (preview_throbber_) {
    preview_throbber_->Stop();
    preview_throbber_->SetVisible(false);
  }

  if (file_size_label_) {
    file_size_label_->SetText(FormatFileSize(file_size_bytes_));
  }
  if (dimensions_label_) {
    dimensions_label_->SetText(
        FormatDimensions(source_bounds_.width(), source_bounds_.height()));
  }

  UpdateButtonStates();
}

// =========================================================================
// AstraScreenshotCaptureModelObserver
// =========================================================================

void AstraScreenshotCaptureBubble::OnCaptureStarted() {
  SetState(State::kCapturing);
  if (placeholder_label_) {
    placeholder_label_->SetVisible(true);
    placeholder_label_->SetText(u"Capturing...");
  }
  if (preview_image_) {
    preview_image_->SetVisible(false);
  }
  if (preview_throbber_) {
    preview_throbber_->SetVisible(true);
    preview_throbber_->Start();
  }
}

void AstraScreenshotCaptureBubble::OnCaptureCompleted(const SkBitmap& bitmap) {
  if (model_) {
    capture_type_ = model_->capture_type();
    source_bounds_ = model_->source_bounds();
  }
  SetPreviewBitmap(bitmap);
  SetState(State::kReady);

  // Auto-copy if enabled in settings.
  if (model_ && model_->GetAutoCopyToClipboard() && delegate_) {
    delegate_->OnScreenshotCopy();
  }
}

void AstraScreenshotCaptureBubble::OnCaptureFailed(const std::string& error) {
  SetError(base::UTF8ToUTF16(error));
}

void AstraScreenshotCaptureBubble::OnSaveCompleted(const base::FilePath& path) {
  SetSaveCompleted();
}

void AstraScreenshotCaptureBubble::OnCopyCompleted() {
  SetCopyCompleted();
}

void AstraScreenshotCaptureBubble::OnCaptureSettingsChanged() {
  // Update UI to reflect new settings.
  ApplySettingsFromModel();
  // Re-estimate file size since format/quality may have changed.
  file_size_bytes_ = EstimateFileSize();
  if (file_size_label_) {
    file_size_label_->SetText(FormatFileSize(file_size_bytes_));
  }
  UpdateInfoVisibility();
  UpdateQualityVisibility();
}

void AstraScreenshotCaptureBubble::OnCaptureStateChanged(
    AstraScreenshotCaptureState state) {
  switch (state) {
    case AstraScreenshotCaptureState::kIdle:
      SetState(State::kReady);
      break;
    case AstraScreenshotCaptureState::kCapturing:
      OnCaptureStarted();
      break;
    case AstraScreenshotCaptureState::kReady:
      SetState(State::kReady);
      break;
    case AstraScreenshotCaptureState::kSaving:
      SetState(State::kSaving);
      break;
    case AstraScreenshotCaptureState::kError:
      if (model_) {
        SetError(base::UTF8ToUTF16(model_->last_error()));
      }
      break;
  }
}

void AstraScreenshotCaptureBubble::OnCaptureModeChanged(
    AstraScreenshotMode mode) {
  capture_mode_ = mode;
  if (mode_combobox_) {
    mode_combobox_->SetSelectedIndex(static_cast<int>(mode));
  }
}

void AstraScreenshotCaptureBubble::OnCaptureProgress(
    AstraScreenshotCaptureModel* model,
    double progress) {
  // Update progress indicator if we have one.
  // TODO(astra): Add a progress bar or update the throbber.
}

void AstraScreenshotCaptureBubble::OnAnnotationAdded(
    AstraScreenshotCaptureModel* model) {
  // Update undo/redo button states.
  if (undo_button_) {
    undo_button_->SetEnabled(model->CanUndo());
  }
  if (redo_button_) {
    redo_button_->SetEnabled(model->CanRedo());
  }
}

void AstraScreenshotCaptureBubble::OnAnnotationUndoRedoChanged(
    AstraScreenshotCaptureModel* model) {
  if (undo_button_) {
    undo_button_->SetEnabled(model->CanUndo());
  }
  if (redo_button_) {
    redo_button_->SetEnabled(model->CanRedo());
  }
}

void AstraScreenshotCaptureBubble::OnSettingsChanged(
    AstraScreenshotCaptureModel* model) {
  ApplySettingsFromModel();
}

void AstraScreenshotCaptureBubble::OnScreenshotModelShutdown(
    AstraScreenshotCaptureModel* model) {
  if (model_ == model) {
    model_->RemoveObserver(this);
    model_ = nullptr;
  }
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraScreenshotCaptureBubble::Init() {
  // Main vertical layout.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(0, kBubbleInteriorPadding),
      kInfoRowSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // --- Preview image container ---
  auto preview_container = std::make_unique<views::View>();
  preview_container->SetLayoutManager(std::make_unique<views::FillLayout>());
  preview_container->SetPreferredSize(
      gfx::Size(kBubbleWidth - kBubbleInteriorPadding * 2, kPreviewMaxHeight));

  // Preview image.
  auto preview_image = std::make_unique<views::ImageView>();
  preview_image_ = preview_image.get();
  preview_image_->SetHorizontalAlignment(views::ImageView::Alignment::kCenter);
  preview_image_->SetVerticalAlignment(views::ImageView::Alignment::kCenter);

  if (!bitmap_.isNull() && !bitmap_.empty()) {
    gfx::Size preview_size(bitmap_.width(), bitmap_.height());
    if (preview_size.height() > kPreviewMaxHeight) {
      float scale =
          static_cast<float>(kPreviewMaxHeight) / preview_size.height();
      preview_size.set_width(static_cast<int>(preview_size.width() * scale));
      preview_size.set_height(kPreviewMaxHeight);
    }
    int max_width = kBubbleWidth - kBubbleInteriorPadding * 2;
    if (preview_size.width() > max_width) {
      float scale = static_cast<float>(max_width) / preview_size.width();
      preview_size.set_width(max_width);
      preview_size.set_height(
          static_cast<int>(preview_size.height() * scale));
    }

    gfx::ImageSkia image = gfx::ImageSkia::CreateFrom1xBitmap(bitmap_);
    preview_image_->SetImage(image);
    preview_image_->SetImageSize(preview_size);
    preview_image_->SetVisible(true);
  } else {
    preview_image_->SetVisible(false);
  }
  preview_container->AddChildView(std::move(preview_image));

  // Placeholder label.
  auto placeholder_label = std::make_unique<views::Label>(u"Capturing...");
  placeholder_label_ = placeholder_label.get();
  placeholder_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  placeholder_label_->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  placeholder_label_->SetVisible(bitmap_.isNull() || bitmap_.empty());
  preview_container->AddChildView(std::move(placeholder_label));

  // Loading throbber.
  auto preview_throbber = std::make_unique<views::Throbber>();
  preview_throbber_ = preview_throbber.get();
  preview_throbber_->SetVisible(bitmap_.isNull() || bitmap_.empty());
  if (bitmap_.isNull() || bitmap_.empty()) {
    preview_throbber_->Start();
  }
  preview_container->AddChildView(std::move(preview_throbber));

  AddChildView(std::move(preview_container));

  // Spacer.
  auto preview_spacer = std::make_unique<views::View>();
  preview_spacer->SetPreferredSize(
      gfx::Size(0, kPreviewBottomPadding - kInfoRowSpacing));
  AddChildView(std::move(preview_spacer));

  // --- Filename field (editable) ---
  auto filename_field = std::make_unique<views::Textfield>();
  filename_field_ = filename_field.get();
  filename_field_->SetText(filename_);
  filename_field_->SetAccessibleName(u"Screenshot filename");
  filename_field_->SetPlaceholderText(u"Screenshot filename");
  AddChildView(std::move(filename_field));

  // --- Info row: capture type + dimensions ---
  auto info_row = std::make_unique<views::View>();
  auto* info_layout = info_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 0),
      kInfoRowSpacing));
  info_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);

  // Capture type label.
  auto type_label = std::make_unique<views::Label>(GetCaptureTypeLabel());
  type_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  capture_type_label_ = type_label.get();
  info_row->AddChildView(std::move(type_label));

  // Dimensions label.
  auto dims_label = std::make_unique<views::Label>(
      FormatDimensions(source_bounds_.width(), source_bounds_.height()));
  dims_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  dimensions_label_ = dims_label.get();
  info_row->AddChildView(std::move(dims_label));

  AddChildView(std::move(info_row));

  // --- File size label ---
  auto size_label = std::make_unique<views::Label>(
      FormatFileSize(file_size_bytes_));
  size_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  file_size_label_ = size_label.get();
  AddChildView(std::move(size_label));

  // --- Status row ---
  auto status_row = std::make_unique<views::View>();
  auto* status_layout = status_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          kStatusRowSpacing));
  status_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto status_throbber = std::make_unique<views::Throbber>();
  status_throbber_ = status_throbber.get();
  status_throbber_->SetVisible(false);
  status_row->AddChildView(std::move(status_throbber));

  auto status_label = std::make_unique<views::Label>(u"");
  status_label_ = status_label.get();
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetVisible(false);
  status_layout->SetFlexForView(status_label_, 1);
  status_row->AddChildView(std::move(status_label));

  AddChildView(std::move(status_row));

  // --- Capture controls section: mode + delay ---
  auto capture_controls_container = std::make_unique<views::View>();
  auto* capture_layout = capture_controls_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(8, 0),
          kSettingsRowSpacing));
  capture_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Mode selector row.
  auto mode_row = std::make_unique<views::View>();
  auto* mode_layout = mode_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          8));
  mode_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto mode_label = std::make_unique<views::Label>(u"Capture:");
  mode_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  mode_row->AddChildView(std::move(mode_label));

  auto mode_combobox = std::make_unique<views::Combobox>();
  mode_combobox_ = mode_combobox.get();
  mode_combobox_->SetAccessibleName(u"Capture mode");
  mode_combobox_->AppendText(u"Full Page");
  mode_combobox_->AppendText(u"Visible Area");
  mode_combobox_->AppendText(u"Region");
  mode_combobox_->AppendText(u"Window");
  mode_combobox_->AppendText(u"Element");
  mode_combobox_->SetSelectedIndex(static_cast<int>(capture_mode_));
  mode_combobox_->SetCallback(base::BindRepeating(
      &AstraScreenshotCaptureBubble::OnModeChanged,
      base::Unretained(this)));
  mode_layout->SetFlexForView(mode_combobox_, 1);
  mode_row->AddChildView(std::move(mode_combobox));

  capture_controls_container->AddChildView(std::move(mode_row));

  // Delay selector row.
  auto delay_row = std::make_unique<views::View>();
  auto* delay_layout = delay_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          8));
  delay_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto delay_label = std::make_unique<views::Label>(u"Delay:");
  delay_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  delay_row->AddChildView(std::move(delay_label));

  auto delay_combobox = std::make_unique<views::Combobox>();
  delay_combobox_ = delay_combobox.get();
  delay_combobox_->SetAccessibleName(u"Capture delay");
  for (int i = 0; i <= 10; i++) {
    delay_combobox_->AppendText(GetDelayLabel(i));
  }
  delay_combobox_->SetSelectedIndex(capture_delay_seconds_);
  delay_combobox_->SetCallback(base::BindRepeating(
      &AstraScreenshotCaptureBubble::OnDelayChanged,
      base::Unretained(this)));
  delay_layout->SetFlexForView(delay_combobox_, 1);
  delay_row->AddChildView(std::move(delay_combobox));

  capture_controls_container->AddChildView(std::move(delay_row));

  AddChildView(std::move(capture_controls_container));

  // --- Settings section: format + quality + copy toggle ---
  //
  // This section provides quick access to capture settings right in the
  // bubble so users don't need to go to settings to change format or quality.

  auto settings_container = std::make_unique<views::View>();
  auto* settings_layout = settings_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(8, 0),
          kSettingsRowSpacing));
  settings_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Format selector row.
  auto format_row = std::make_unique<views::View>();
  auto* format_layout = format_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          8));
  format_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto format_label = std::make_unique<views::Label>(u"Format:");
  format_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  format_row->AddChildView(std::move(format_label));

  auto format_combobox = std::make_unique<views::Combobox>();
  format_combobox_ = format_combobox.get();
  format_combobox_->SetAccessibleName(u"Image format");
  format_combobox_->AppendText(u"PNG");
  format_combobox_->AppendText(u"JPEG");
  format_combobox_->AppendText(u"WebP");
  format_combobox_->SetSelectedIndex(static_cast<int>(format_));
  format_combobox_->SetCallback(base::BindRepeating(
      &AstraScreenshotCaptureBubble::OnFormatChanged,
      base::Unretained(this)));
  format_layout->SetFlexForView(format_combobox_, 1);
  format_row->AddChildView(std::move(format_combobox));

  settings_container->AddChildView(std::move(format_row));

  // Quality selector row (only visible for JPEG/WebP).
  auto quality_row = std::make_unique<views::View>();
  auto* quality_layout = quality_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          8));
  quality_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto quality_label = std::make_unique<views::Label>(u"Quality:");
  quality_label_ = quality_label.get();
  quality_row->AddChildView(std::move(quality_label));

  auto quality_combobox = std::make_unique<views::Combobox>();
  quality_combobox_ = quality_combobox.get();
  quality_combobox_->SetAccessibleName(u"Image quality");
  quality_combobox_->AppendText(u"Low");
  quality_combobox_->AppendText(u"Medium");
  quality_combobox_->AppendText(u"High");
  quality_combobox_->AppendText(u"Maximum");
  quality_combobox_->SetSelectedIndex(static_cast<int>(quality_));
  quality_combobox_->SetCallback(base::BindRepeating(
      &AstraScreenshotCaptureBubble::OnQualityChanged,
      base::Unretained(this)));
  quality_layout->SetFlexForView(quality_combobox_, 1);
  quality_row->AddChildView(std::move(quality_combobox));

  // Start with quality visible (or not, based on format).
  bool quality_visible = (format_ != AstraScreenshotFormat::kPng);
  quality_row->SetVisible(quality_visible);

  settings_container->AddChildView(std::move(quality_row));

  // Copy to clipboard toggle row.
  auto copy_toggle_row = std::make_unique<views::View>();
  auto* copy_toggle_layout = copy_toggle_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          8));
  copy_toggle_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto copy_toggle_label = std::make_unique<views::Label>(
      u"Copy to clipboard automatically");
  copy_toggle_label_ = copy_toggle_label.get();
  copy_toggle_layout->SetFlexForView(copy_toggle_label_, 1);
  copy_toggle_row->AddChildView(std::move(copy_toggle_label));

  auto copy_toggle = std::make_unique<views::ToggleButton>();
  copy_toggle_ = copy_toggle.get();
  copy_toggle_->SetAccessibleName(u"Copy to clipboard automatically");
  copy_toggle_->SetIsOn(auto_copy_to_clipboard_);
  copy_toggle_->SetToggledCallback(base::BindRepeating(
      &AstraScreenshotCaptureBubble::OnCopyToggleToggled,
      base::Unretained(this)));
  copy_toggle_row->AddChildView(std::move(copy_toggle));

  settings_container->AddChildView(std::move(copy_toggle_row));

  AddChildView(std::move(settings_container));

  // --- Annotation tools toggle ---
  auto annotation_toggle_row = std::make_unique<views::View>();
  auto* annotation_toggle_layout = annotation_toggle_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          kButtonRowSpacing));
  annotation_toggle_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  auto annotation_toggle_button = views::MdTextButton::CreateSecondary(
      base::BindRepeating(
          &AstraScreenshotCaptureBubble::OnAnnotationToolsToggled,
          base::Unretained(this)),
      u"Edit");
  annotation_toggle_button_ = annotation_toggle_button.get();
  annotation_toggle_button_->SetAccessibleName(u"Show annotation tools");
  annotation_toggle_row->AddChildView(std::move(annotation_toggle_button));

  auto undo_button = views::MdTextButton::CreateSecondary(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnUndoClicked,
                          base::Unretained(this)),
      u"Undo");
  undo_button_ = undo_button.get();
  undo_button_->SetAccessibleName(u"Undo annotation");
  undo_button_->SetEnabled(false);
  annotation_toggle_row->AddChildView(std::move(undo_button));

  auto redo_button = views::MdTextButton::CreateSecondary(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnRedoClicked,
                          base::Unretained(this)),
      u"Redo");
  redo_button_ = redo_button.get();
  redo_button_->SetAccessibleName(u"Redo annotation");
  redo_button_->SetEnabled(false);
  annotation_toggle_row->AddChildView(std::move(redo_button));

  AddChildView(std::move(annotation_toggle_row));

  // --- Annotation tools section (collapsible) ---
  auto annotation_tools_section = std::make_unique<views::View>();
  auto* annotation_layout = annotation_tools_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(4, 0),
          4));
  annotation_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  annotation_tools_section_ = annotation_tools_section.get();
  annotation_tools_section_->SetVisible(show_annotation_tools_);

  // Tool buttons (simplified as text buttons for now).
  // TODO(astra): Replace with icon buttons for annotation tools.
  const std::pair<AstraAnnotationTool, const char*> kTools[] = {
      {AstraAnnotationTool::kArrow, "Arrow"},
      {AstraAnnotationTool::kRectangle, "Rect"},
      {AstraAnnotationTool::kCircle, "Circle"},
      {AstraAnnotationTool::kText, "Text"},
      {AstraAnnotationTool::kBlur, "Blur"},
      {AstraAnnotationTool::kHighlight, "Highlight"},
      {AstraAnnotationTool::kCrop, "Crop"},
      {AstraAnnotationTool::kPen, "Pen"},
  };

  for (const auto& tool : kTools) {
    auto tool_button = views::MdTextButton::CreateSecondary(
        base::BindRepeating(&AstraScreenshotCaptureBubble::OnToolSelected,
                            base::Unretained(this), tool.first),
        base::UTF8ToUTF16(tool.second));
    tool_button->SetAccessibleName(
        base::UTF8ToUTF16(std::string("Annotation tool: ") + tool.second));
    annotation_tools_section_->AddChildView(std::move(tool_button));
  }

  AddChildView(std::move(annotation_tools_section));

  // --- Action button row ---
  auto button_row = std::make_unique<views::View>();
  auto* button_layout = button_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          kButtonRowSpacing));
  button_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  // Capture button (primary).
  auto capture_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnCaptureButtonClicked,
                          base::Unretained(this)),
      u"Capture");
  capture_button_ = capture_button.get();
  capture_button_->SetProminent(true);
  capture_button_->SetAccessibleName(u"Take screenshot");
  button_row->AddChildView(std::move(capture_button));

  // Save button.
  auto save_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnSaveButtonClicked,
                          base::Unretained(this)),
      u"Save");
  save_button_ = save_button.get();
  save_button_->SetProminent(true);
  save_button_->SetAccessibleName(u"Save screenshot to Downloads");
  button_row->AddChildView(std::move(save_button));

  // Copy button.
  auto copy_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnCopyButtonClicked,
                          base::Unretained(this)),
      u"Copy");
  copy_button_ = copy_button.get();
  copy_button_->SetAccessibleName(u"Copy screenshot to clipboard");
  button_row->AddChildView(std::move(copy_button));

  // Share button.
  auto share_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnShareButtonClicked,
                          base::Unretained(this)),
      u"Share");
  share_button_ = share_button.get();
  share_button_->SetAccessibleName(u"Share screenshot");
  button_row->AddChildView(std::move(share_button));

  // Spacer to push delete button to the right.
  auto button_spacer = std::make_unique<views::View>();
  button_layout->SetFlexForView(button_spacer.get(), 1);
  button_row->AddChildView(std::move(button_spacer));

  // Delete button (destructive).
  auto delete_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnDeleteButtonClicked,
                          base::Unretained(this)),
      u"Discard");
  delete_button_ = delete_button.get();
  delete_button_->SetAccessibleName(u"Discard screenshot");
  button_row->AddChildView(std::move(delete_button));

  AddChildView(std::move(button_row));

  // --- Secondary action row: Edit + Open in Editor ---
  auto secondary_row = std::make_unique<views::View>();
  auto* secondary_layout = secondary_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          kButtonRowSpacing));
  secondary_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  // Edit button.
  auto edit_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnEditButtonClicked,
                          base::Unretained(this)),
      u"Edit");
  edit_button_ = edit_button.get();
  edit_button_->SetAccessibleName(u"Edit screenshot");
  secondary_row->AddChildView(std::move(edit_button));

  // Open in Editor button (more prominent edit option).
  auto open_in_editor_button = views::MdTextButton::Create(
      base::BindRepeating(
          &AstraScreenshotCaptureBubble::OnOpenInEditorClicked,
          base::Unretained(this)),
      u"Open in Editor");
  open_in_editor_button_ = open_in_editor_button.get();
  open_in_editor_button_->SetAccessibleName(u"Open screenshot in editor");
  secondary_row->AddChildView(std::move(open_in_editor_button));

  AddChildView(std::move(secondary_row));

  // --- Keep button (for auto-dismiss) ---
  auto keep_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraScreenshotCaptureBubble::OnKeepButtonClicked,
                          base::Unretained(this)),
      u"Keep");
  keep_button_ = keep_button.get();
  keep_button_->SetAccessibleName(u"Keep the screenshot bubble open");
  AddChildView(std::move(keep_button));

  // Apply settings from model if available.
  if (model_) {
    model_->AddObserver(this);
    ApplySettingsFromModel();
  }

  // Initialize states.
  UpdateButtonStates();
  UpdateInfoVisibility();
  UpdateQualityVisibility();
  UpdateAnnotationToolsVisibility();

  // Start the auto-dismiss timer.
  StartAutoDismissTimer();
}

void AstraScreenshotCaptureBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  CancelAutoDismissTimer();
}

void AstraScreenshotCaptureBubble::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);
  if (active && !auto_dismiss_kept_) {
    ResetAutoDismissTimer();
  }
}

void AstraScreenshotCaptureBubble::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

// =========================================================================
// Accessible name
// =========================================================================

std::u16string AstraScreenshotCaptureBubble::GetAccessibleName() const {
  return u"Screenshot: " + GetCaptureTypeLabel() + u", " +
         FormatDimensions(source_bounds_.width(), source_bounds_.height());
}

// =========================================================================
// Auto-dismiss timer
// =========================================================================

void AstraScreenshotCaptureBubble::StartAutoDismissTimer() {
  if (auto_dismiss_kept_) {
    return;
  }

  auto_dismiss_timer_.Start(
      FROM_HERE, auto_dismiss_delay_,
      base::BindOnce(&AstraScreenshotCaptureBubble::OnAutoDismissTimerFired,
                     base::Unretained(this)));
}

void AstraScreenshotCaptureBubble::ResetAutoDismissTimer() {
  if (auto_dismiss_kept_) {
    return;
  }
  if (auto_dismiss_timer_.IsRunning()) {
    auto_dismiss_timer_.Reset();
  }
}

void AstraScreenshotCaptureBubble::CancelAutoDismissTimer() {
  auto_dismiss_kept_ = true;
  auto_dismiss_timer_.Stop();

  if (keep_button_) {
    keep_button_->SetVisible(false);
  }
}

void AstraScreenshotCaptureBubble::OnAutoDismissTimerFired() {
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

// =========================================================================
// State management
// =========================================================================

void AstraScreenshotCaptureBubble::SetState(State state) {
  state_ = state;
  UpdateButtonStates();
  UpdateStatusIndicator();
}

void AstraScreenshotCaptureBubble::UpdateButtonStates() {
  if (!save_button_ || !copy_button_ || !edit_button_ ||
      !share_button_ || !delete_button_ || !open_in_editor_button_ ||
      !capture_button_) {
    return;
  }

  bool has_image = !bitmap_.isNull() && !bitmap_.empty();

  switch (state_) {
    case State::kReady:
      capture_button_->SetEnabled(true);
      save_button_->SetEnabled(has_image);
      copy_button_->SetEnabled(has_image);
      edit_button_->SetEnabled(has_image);
      share_button_->SetEnabled(has_image);
      open_in_editor_button_->SetEnabled(has_image);
      delete_button_->SetEnabled(true);
      break;
    case State::kCapturing:
      capture_button_->SetEnabled(false);
      save_button_->SetEnabled(false);
      copy_button_->SetEnabled(false);
      edit_button_->SetEnabled(false);
      share_button_->SetEnabled(false);
      open_in_editor_button_->SetEnabled(false);
      delete_button_->SetEnabled(false);
      break;
    case State::kSaving:
      capture_button_->SetEnabled(false);
      save_button_->SetEnabled(false);
      copy_button_->SetEnabled(false);
      edit_button_->SetEnabled(false);
      share_button_->SetEnabled(false);
      open_in_editor_button_->SetEnabled(false);
      delete_button_->SetEnabled(false);
      break;
    case State::kCopying:
      capture_button_->SetEnabled(false);
      save_button_->SetEnabled(false);
      copy_button_->SetEnabled(false);
      edit_button_->SetEnabled(false);
      share_button_->SetEnabled(false);
      open_in_editor_button_->SetEnabled(false);
      delete_button_->SetEnabled(false);
      break;
    case State::kSuccess:
      // Keep other actions available after one succeeds.
      capture_button_->SetEnabled(true);
      copy_button_->SetEnabled(has_image);
      edit_button_->SetEnabled(has_image);
      share_button_->SetEnabled(has_image);
      open_in_editor_button_->SetEnabled(has_image);
      delete_button_->SetEnabled(true);
      break;
    case State::kError:
      capture_button_->SetEnabled(true);
      save_button_->SetEnabled(has_image);
      copy_button_->SetEnabled(has_image);
      edit_button_->SetEnabled(has_image);
      share_button_->SetEnabled(has_image);
      open_in_editor_button_->SetEnabled(has_image);
      delete_button_->SetEnabled(true);
      break;
  }
}

void AstraScreenshotCaptureBubble::UpdateStatusIndicator() {
  if (!status_label_ || !status_throbber_) {
    return;
  }

  switch (state_) {
    case State::kReady:
      status_label_->SetVisible(false);
      status_throbber_->SetVisible(false);
      status_throbber_->Stop();
      break;
    case State::kCapturing:
      status_label_->SetText(u"Capturing...");
      status_label_->SetVisible(true);
      status_throbber_->SetVisible(true);
      status_throbber_->Start();
      break;
    case State::kSaving:
      status_label_->SetText(u"Saving...");
      status_label_->SetVisible(true);
      status_throbber_->SetVisible(true);
      status_throbber_->Start();
      break;
    case State::kCopying:
      status_label_->SetText(u"Copying...");
      status_label_->SetVisible(true);
      status_throbber_->SetVisible(true);
      status_throbber_->Start();
      break;
    case State::kSuccess:
      status_label_->SetVisible(true);
      status_throbber_->SetVisible(false);
      status_throbber_->Stop();
      if (GetColorProvider()) {
        status_label_->SetAutoColorReadabilityEnabled(false);
        status_label_->SetEnabledColor(
            GetColorProvider()->GetColor(kColorAstraWorkspaceAccent));
      }
      break;
    case State::kError:
      status_label_->SetText(error_message_);
      status_label_->SetVisible(true);
      status_throbber_->SetVisible(false);
      status_throbber_->Stop();
      break;
  }
}

void AstraScreenshotCaptureBubble::UpdateColors() {
  const ui::ColorProvider* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (capture_type_label_ && file_size_label_) {
    SkColor secondary_color = color_provider->GetColor(
        ui::kColorLabelForegroundSecondary);
    capture_type_label_->SetAutoColorReadabilityEnabled(false);
    capture_type_label_->SetEnabledColor(secondary_color);
    file_size_label_->SetAutoColorReadabilityEnabled(false);
    file_size_label_->SetEnabledColor(secondary_color);
  }

  UpdateStatusIndicator();

  if (capture_type_label_) {
    capture_type_label_->SetAutoColorReadabilityEnabled(false);
    capture_type_label_->SetEnabledColor(
        color_provider->GetColor(kColorAstraWorkspaceAccent));
  }
}

void AstraScreenshotCaptureBubble::UpdateInfoVisibility() {
  bool show_filename = true;
  bool show_dimensions = true;
  bool show_file_size = true;

  if (model_) {
    show_filename = model_->GetShowFilenameInBubble();
    show_dimensions = model_->GetShowDimensionsInBubble();
    show_file_size = model_->GetShowFileSizeInBubble();
  }

  if (filename_field_) {
    filename_field_->SetVisible(show_filename);
  }
  if (dimensions_label_) {
    dimensions_label_->SetVisible(show_dimensions);
  }
  if (file_size_label_) {
    file_size_label_->SetVisible(show_file_size);
  }
}

void AstraScreenshotCaptureBubble::UpdateQualityVisibility() {
  if (!quality_combobox_ || !format_combobox_) return;

  // Quality selector is only relevant for lossy formats (JPEG, WebP).
  int selected = format_combobox_->GetSelectedIndex().value_or(0);
  bool show_quality = (selected == 1 || selected == 2);  // JPEG or WebP

  // Find the quality row parent and set visibility.
  views::View* quality_row = quality_combobox_->parent();
  if (quality_row) {
    quality_row->SetVisible(show_quality);
  }
}

void AstraScreenshotCaptureBubble::UpdateAnnotationToolsVisibility() {
  if (annotation_tools_section_) {
    annotation_tools_section_->SetVisible(show_annotation_tools_);
  }
}

void AstraScreenshotCaptureBubble::UpdateModeSelectorFromModel() {
  if (!model_ || !mode_combobox_) return;
  mode_combobox_->SetSelectedIndex(static_cast<int>(model_->GetCaptureMode()));
}

// =========================================================================
// Formatting helpers
// =========================================================================

std::u16string AstraScreenshotCaptureBubble::FormatDimensions(
    int width,
    int height) const {
  return base::NumberToString16(width) + u" x " +
         base::NumberToString16(height) + u" px";
}

std::u16string AstraScreenshotCaptureBubble::GetCaptureTypeLabel() const {
  switch (capture_type_) {
    case AstraScreenshotType::kVisibleArea:
      return u"Visible Area";
    case AstraScreenshotType::kFullPage:
      return u"Full Page";
    case AstraScreenshotType::kRegion:
      return u"Region";
  }
  return u"Screenshot";
}

std::u16string AstraScreenshotCaptureBubble::GetCaptureModeLabel() const {
  return astra::GetCaptureModeLabel(capture_mode_);
}

std::u16string AstraScreenshotCaptureBubble::GenerateDefaultFilename() const {
  base::Time now = base::Time::Now();
  std::u16string date_str = base::TimeFormatShortDate(now);
  return u"Screenshot " + date_str + u".png";
}

int64_t AstraScreenshotCaptureBubble::EstimateFileSize() const {
  if (bitmap_.isNull() || bitmap_.empty()) {
    return 0;
  }

  int pixel_count = bitmap_.width() * bitmap_.height();
  AstraScreenshotFormat format = format_;
  int quality_percent = AstraScreenshotCaptureModel::QualityToPercent(quality_);

  switch (format) {
    case AstraScreenshotFormat::kPng:
      return pixel_count;
    case AstraScreenshotFormat::kJpeg: {
      double quality_factor = quality_percent / 100.0;
      double bytes_per_pixel = 0.05 + quality_factor * 1.45;
      return static_cast<int64_t>(pixel_count * bytes_per_pixel);
    }
    case AstraScreenshotFormat::kWebP:
      return static_cast<int64_t>(pixel_count * 0.6);
  }
  return pixel_count;
}

std::u16string AstraScreenshotCaptureBubble::FormatFileSize(
    int64_t bytes) const {
  return astra::FormatFileSize(bytes);
}

// =========================================================================
// Settings sync helpers
// =========================================================================

void AstraScreenshotCaptureBubble::ApplySettingsFromModel() {
  if (!model_) return;

  // Capture mode
  capture_mode_ = model_->GetCaptureMode();
  if (mode_combobox_) {
    mode_combobox_->SetSelectedIndex(static_cast<int>(capture_mode_));
  }

  // Format
  format_ = model_->GetFormat();
  if (format_combobox_) {
    format_combobox_->SetSelectedIndex(static_cast<int>(format_));
  }

  // Quality
  quality_ = model_->GetQuality();
  if (quality_combobox_) {
    quality_combobox_->SetSelectedIndex(static_cast<int>(quality_));
  }

  // Delay
  capture_delay_seconds_ = model_->GetCaptureDelay();
  if (delay_combobox_) {
    delay_combobox_->SetSelectedIndex(capture_delay_seconds_);
  }

  // Copy toggle
  auto_copy_to_clipboard_ = model_->GetAutoCopyToClipboard();
  if (copy_toggle_) {
    copy_toggle_->SetIsOn(auto_copy_to_clipboard_);
  }

  // Active tool
  active_tool_ = model_->GetActiveTool();

  // Auto-dismiss
  auto_dismiss_delay_ = base::Seconds(model_->GetAutoDismissDelaySeconds());
  auto_dismiss_kept_ = !model_->GetAutoDismissBubble();
  if (keep_button_) {
    keep_button_->SetVisible(!auto_dismiss_kept_);
  }
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraScreenshotCaptureBubble::OnCaptureButtonClicked() {
  StartCapture();
}

void AstraScreenshotCaptureBubble::OnCancelButtonClicked() {
  CancelCapture();
}

void AstraScreenshotCaptureBubble::OnSaveButtonClicked() {
  SaveToFile();
}

void AstraScreenshotCaptureBubble::OnCopyButtonClicked() {
  CopyToClipboard();
}

void AstraScreenshotCaptureBubble::OnEditButtonClicked() {
  CancelAutoDismissTimer();
  if (delegate_) {
    delegate_->OnScreenshotEdit();
  }
}

void AstraScreenshotCaptureBubble::OnDeleteButtonClicked() {
  CancelAutoDismissTimer();
  if (delegate_) {
    delegate_->OnScreenshotDelete();
  }
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

void AstraScreenshotCaptureBubble::OnShareButtonClicked() {
  Share();
}

void AstraScreenshotCaptureBubble::OnKeepButtonClicked() {
  CancelAutoDismissTimer();
  if (save_button_ && save_button_->GetEnabled()) {
    save_button_->RequestFocus();
  }
}

void AstraScreenshotCaptureBubble::OnOpenInEditorClicked() {
  CancelAutoDismissTimer();
  if (delegate_) {
    delegate_->OnScreenshotOpenInEditor();
  }
}

void AstraScreenshotCaptureBubble::OnCopyToggleToggled() {
  if (!copy_toggle_ || !model_) return;
  bool enabled = copy_toggle_->GetIsOn();
  model_->SetAutoCopyToClipboard(enabled);
  auto_copy_to_clipboard_ = enabled;
}

void AstraScreenshotCaptureBubble::OnFormatChanged() {
  if (!format_combobox_) return;

  int idx = format_combobox_->GetSelectedIndex().value_or(0);
  auto format = static_cast<AstraScreenshotFormat>(idx);
  SetFormat(format);
}

void AstraScreenshotCaptureBubble::OnQualityChanged() {
  if (!quality_combobox_) return;

  int idx = quality_combobox_->GetSelectedIndex().value_or(0);
  auto quality = static_cast<AstraScreenshotQuality>(idx);
  SetQuality(quality);
}

void AstraScreenshotCaptureBubble::OnFilenameChanged() {
  if (!filename_field_) return;
  filename_ = filename_field_->GetText();
}

void AstraScreenshotCaptureBubble::OnModeChanged() {
  if (!mode_combobox_) return;

  int idx = mode_combobox_->GetSelectedIndex().value_or(0);
  auto mode = static_cast<AstraScreenshotMode>(idx);
  SetCaptureMode(mode);
}

void AstraScreenshotCaptureBubble::OnDelayChanged() {
  if (!delay_combobox_) return;

  int idx = delay_combobox_->GetSelectedIndex().value_or(0);
  SetCaptureDelay(idx);
}

void AstraScreenshotCaptureBubble::OnAnnotationToolsToggled() {
  SetShowAnnotationTools(!show_annotation_tools_);
}

void AstraScreenshotCaptureBubble::OnToolSelected(AstraAnnotationTool tool) {
  SetActiveTool(tool);
}

void AstraScreenshotCaptureBubble::OnUndoClicked() {
  UndoAnnotation();
}

void AstraScreenshotCaptureBubble::OnRedoClicked() {
  RedoAnnotation();
}

void AstraScreenshotCaptureBubble::OnShareClicked() {
  Share();
}

}  // namespace astra
