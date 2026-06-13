#include "astra/ui/views/sidebar/astra_download_item_view.h"

#include <string>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kDownloadItemHeight = 52;
constexpr int kDownloadItemVerticalPadding = 6;
constexpr int kDownloadItemIconSize = 16;
constexpr int kDownloadItemCornerRadius = 6;
constexpr int kDownloadProgressBarHeight = 3;
constexpr int kActionButtonSize = 16;
constexpr int kActionButtonSpacing = 4;

// Astra color IDs for download item styling.
constexpr ui::ColorId kDownloadItemTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kDownloadItemSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kDownloadItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kDownloadDangerousColorId =
    kColorAstraSidebarItemSelectedBackground;

// Progress bar colors.
// TODO(astra): Add Astra-specific progress bar color IDs to astra_color_ids.h.
constexpr ui::ColorId kDownloadProgressColor = ui::kColorFocusableBorderFocused;
constexpr ui::ColorId kDownloadProgressBgColor =
    ui::kColorTrackLayerActiveBackground;

// Divisor thresholds for human-readable byte formatting.
constexpr int64_t kKilobyte = 1024;
constexpr int64_t kMegabyte = 1024 * kKilobyte;
constexpr int64_t kGigabyte = 1024 * kMegabyte;

}  // namespace

AstraDownloadItemView::AstraDownloadItemView(
    const std::string& download_id,
    const std::u16string& filename,
    AstraDownloadState state,
    int64_t received_bytes,
    int64_t total_bytes)
    : download_id_(download_id),
      filename_(filename),
      state_(state),
      received_bytes_(received_bytes),
      total_bytes_(total_bytes) {
  // Initialize title and secondary text.
  SetTitle(filename);

  // Set up icon.
  icon_view()->SetVisible(true);
  UpdateIcon();

  // Calculate initial progress.
  if (total_bytes_ > 0) {
    progress_ = static_cast<double>(received_bytes_) /
                static_cast<double>(total_bytes_);
  }

  // Update initial state.
  UpdateStatusText();
}

AstraDownloadItemView::~AstraDownloadItemView() = default;

void AstraDownloadItemView::BuildLayout() {
  // Start with base class layout (icon + text container + trailing).
  AstraSidebarItemView::BuildLayout();

  // Add progress bar below the text container.
  // We add it to the main view after the text container row.
  // The base class uses a horizontal BoxLayout for the top row.
  // We need a vertical layout for the whole item.
  //
  // Since the base class uses a horizontal layout for the row,
  // we'll replace it with a vertical layout that contains
  // the top row and the progress bar.

  // Actually, let's add the progress bar as a child and position it manually
  // in Layout() override.

  progress_bar_ = AddChildView(std::make_unique<views::ProgressBar>());
  progress_bar_->SetPreferredSize(
      gfx::Size(0, kDownloadProgressBarHeight));

  // Action buttons — add to trailing container.
  // Pause button.
  pause_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraDownloadItemView::OnPauseButtonPressed,
                              base::Unretained(this))));
  pause_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  pause_button_->SetTooltipText(u"Pause download");
  pause_button_->SetVisible(false);

  // Cancel button.
  cancel_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraDownloadItemView::OnCancelButtonPressed,
                              base::Unretained(this))));
  cancel_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  cancel_button_->SetTooltipText(u"Cancel download");
  cancel_button_->SetVisible(false);

  // Resume button.
  resume_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraDownloadItemView::OnResumeButtonPressed,
                              base::Unretained(this))));
  resume_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  resume_button_->SetTooltipText(u"Resume download");
  resume_button_->SetVisible(false);

  // Open button.
  open_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraDownloadItemView::OnOpenButtonPressed,
                              base::Unretained(this))));
  open_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  open_button_->SetTooltipText(u"Open file");
  open_button_->SetVisible(false);

  // Show in folder button.
  show_in_folder_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraDownloadItemView::OnShowInFolderButtonPressed,
              base::Unretained(this))));
  show_in_folder_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  show_in_folder_button_->SetTooltipText(u"Show in folder");
  show_in_folder_button_->SetVisible(false);

  UpdateProgressBar();
  UpdateActionButtonsVisibility();
}

// =========================================================================
// Download info
// =========================================================================

void AstraDownloadItemView::SetDownloadInfo(const GURL& url,
                                            const std::u16string& filename,
                                            int64_t total_bytes) {
  url_ = url;
  filename_ = filename;
  total_bytes_ = total_bytes;

  SetTitle(filename);
  UpdateStatusText();
  UpdateProgressBar();
  SchedulePaint();
}

// =========================================================================
// Progress
// =========================================================================

void AstraDownloadItemView::SetProgress(double progress) {
  if (progress_ == progress) {
    return;
  }
  progress_ = progress;
  UpdateProgressBar();
}

// =========================================================================
// State
// =========================================================================

void AstraDownloadItemView::SetState(AstraDownloadState state) {
  if (state_ == state) {
    return;
  }
  state_ = state;

  UpdateStatusText();
  UpdateProgressBar();
  UpdateActionButtonsVisibility();
  UpdateIcon();
  SchedulePaint();
}

// =========================================================================
// Bytes
// =========================================================================

void AstraDownloadItemView::SetBytesReceived(int64_t bytes) {
  if (received_bytes_ == bytes) {
    return;
  }
  received_bytes_ = bytes;

  // Update progress.
  if (total_bytes_ > 0) {
    progress_ = static_cast<double>(received_bytes_) /
                static_cast<double>(total_bytes_);
  }

  UpdateStatusText();
  UpdateProgressBar();
}

void AstraDownloadItemView::SetTotalBytes(int64_t bytes) {
  if (total_bytes_ == bytes) {
    return;
  }
  total_bytes_ = bytes;

  // Update progress.
  if (total_bytes_ > 0) {
    progress_ = static_cast<double>(received_bytes_) /
                static_cast<double>(total_bytes_);
  }

  UpdateStatusText();
  UpdateProgressBar();
}

// =========================================================================
// Time remaining
// =========================================================================

void AstraDownloadItemView::SetTimeRemaining(base::TimeDelta remaining) {
  if (time_remaining_ == remaining) {
    return;
  }
  time_remaining_ = remaining;
  UpdateStatusText();
}

// =========================================================================
// Dangerous download
// =========================================================================

void AstraDownloadItemView::SetIsDangerous(bool dangerous) {
  if (is_dangerous_ == dangerous) {
    return;
  }
  is_dangerous_ = dangerous;
  // TODO(astra): Show dangerous download warning (different icon, color).
  OnThemeChanged();
}

// =========================================================================
// Action availability
// =========================================================================

bool AstraDownloadItemView::CanResume() const {
  return state_ == AstraDownloadState::kPaused ||
         state_ == AstraDownloadState::kInterrupted;
}

bool AstraDownloadItemView::CanPause() const {
  return state_ == AstraDownloadState::kInProgress;
}

bool AstraDownloadItemView::CanCancel() const {
  return state_ == AstraDownloadState::kInProgress ||
         state_ == AstraDownloadState::kPaused;
}

// =========================================================================
// Visibility controls
// =========================================================================

void AstraDownloadItemView::ShowProgressBar(bool show) {
  if (show_progress_bar_ == show) {
    return;
  }
  show_progress_bar_ = show;
  UpdateProgressBar();
  InvalidateLayout();
}

void AstraDownloadItemView::ShowPauseButton(bool show) {
  if (show_pause_button_ == show) {
    return;
  }
  show_pause_button_ = show;
  UpdateActionButtonsVisibility();
}

void AstraDownloadItemView::ShowCancelButton(bool show) {
  if (show_cancel_button_ == show) {
    return;
  }
  show_cancel_button_ = show;
  UpdateActionButtonsVisibility();
}

void AstraDownloadItemView::ShowOpenButton(bool show) {
  if (show_open_button_ == show) {
    return;
  }
  show_open_button_ = show;
  UpdateActionButtonsVisibility();
}

void AstraDownloadItemView::ShowInFolderButton(bool show) {
  if (show_in_folder_button_ == show) {
    return;
  }
  show_in_folder_button_ = show;
  UpdateActionButtonsVisibility();
}

// =========================================================================
// Status text
// =========================================================================

void AstraDownloadItemView::UpdateStatusText() {
  std::u16 status_text;

  switch (state_) {
    case AstraDownloadState::kInProgress:
      if (total_bytes_ > 0) {
        status_text = FormatBytes(received_bytes_) + u" / " +
                      FormatBytes(total_bytes_);
      } else {
        status_text = FormatBytes(received_bytes_);
      }
      if (time_remaining_ > base::TimeDelta()) {
        status_text += u" \u00b7 " + FormatTimeRemaining();
      }
      break;

    case AstraDownloadState::kPaused:
      status_text = GetStateLabel();
      if (total_bytes_ > 0) {
        status_text += u" \u00b7 " + FormatBytes(received_bytes_) + u" / " +
                       FormatBytes(total_bytes_);
      }
      break;

    case AstraDownloadState::kComplete:
      status_text = GetStateLabel();
      if (total_bytes_ > 0) {
        status_text += u" \u00b7 " + FormatBytes(total_bytes_);
      }
      break;

    case AstraDownloadState::kFailed:
    case AstraDownloadState::kCancelled:
    case AstraDownloadState::kInterrupted:
      status_text = GetStateLabel();
      break;
  }

  SetSecondaryText(status_text);
}

// =========================================================================
// Progress bar
// =========================================================================

void AstraDownloadItemView::UpdateProgressBar() {
  if (!progress_bar_) {
    return;
  }

  // Progress bar is shown for active downloads if enabled.
  bool show_progress = show_progress_bar_ &&
                       (state_ == AstraDownloadState::kInProgress ||
                        state_ == AstraDownloadState::kPaused);
  progress_bar_->SetVisible(show_progress);

  if (show_progress && total_bytes_ > 0) {
    progress_bar_->SetValue(progress_);
  } else if (show_progress) {
    // Total size unknown — show indeterminate.
    progress_bar_->SetValue(0.0);
  }
}

// =========================================================================
// Action buttons
// =========================================================================

void AstraDownloadItemView::UpdateActionButtonsVisibility() {
  if (!pause_button_ || !cancel_button_ || !resume_button_ ||
      !open_button_ || !show_in_folder_button_) {
    return;
  }

  bool show_actions = is_hovered_internal_;

  // Pause button: visible for in-progress downloads.
  bool pause_visible = show_actions && show_pause_button_ && CanPause();
  pause_button_->SetVisible(pause_visible);

  // Cancel button: visible for active downloads (in-progress or paused).
  bool cancel_visible = show_actions && show_cancel_button_ && CanCancel();
  cancel_button_->SetVisible(cancel_visible);

  // Resume button: visible for paused downloads.
  bool resume_visible = show_actions && CanResume();
  resume_button_->SetVisible(resume_visible);

  // Open button: visible for completed downloads.
  bool open_visible = show_actions && show_open_button_ &&
                      state_ == AstraDownloadState::kComplete;
  open_button_->SetVisible(open_visible);

  // Show in folder button: visible for completed downloads.
  bool show_in_folder_visible = show_actions && show_in_folder_button_ &&
                                state_ == AstraDownloadState::kComplete;
  show_in_folder_button_->SetVisible(show_in_folder_visible);
}

// =========================================================================
// Icon
// =========================================================================

void AstraDownloadItemView::UpdateIcon() {
  // TODO(astra): Use real file-type icon from chrome/browser/ui/icons/.
  //   For now, we just show a placeholder.
  //   Chromium owner: DownloadItemView::GetIconForDownload
  if (is_dangerous_) {
    // Dangerous downloads get a warning-style icon.
    // TODO(astra): Use a danger/warning vector icon.
  }
}

// =========================================================================
// Formatting helpers
// =========================================================================

std::u16 AstraDownloadItemView::FormatBytes(int64_t bytes) const {
  if (bytes < 0) {
    return u"Unknown";
  }

  if (bytes < kKilobyte) {
    return base::NumberToString16(bytes) + u" B";
  }
  if (bytes < kMegabyte) {
    double kb = static_cast<double>(bytes) / kKilobyte;
    return base::NumberToString16(base::StringPrintf(L"%.1f", kb)) + u" KB";
  }
  if (bytes < kGigabyte) {
    double mb = static_cast<double>(bytes) / kMegabyte;
    return base::NumberToString16(base::StringPrintf(L"%.1f", mb)) + u" MB";
  }
  double gb = static_cast<double>(bytes) / kGigabyte;
  return base::NumberToString16(base::StringPrintf(L"%.1f", gb)) + u" GB";
}

std::u16 AstraDownloadItemView::FormatTimeRemaining() const {
  if (time_remaining_ <= base::TimeDelta()) {
    return std::u16string();
  }

  int64_t seconds = time_remaining_.InSeconds();
  if (seconds < 60) {
    return base::NumberToString16(seconds) + u"s left";
  }

  int64_t minutes = seconds / 60;
  if (minutes < 60) {
    return base::NumberToString16(minutes) + u" min left";
  }

  int64_t hours = minutes / 60;
  return base::NumberToString16(hours) + u"h left";
}

std::u16 AstraDownloadItemView::GetStateLabel() const {
  switch (state_) {
    case AstraDownloadState::kInProgress:
      return u"Downloading";
    case AstraDownloadState::kComplete:
      return u"Complete";
    case AstraDownloadState::kPaused:
      return u"Paused";
    case AstraDownloadState::kFailed:
      return u"Failed";
    case AstraDownloadState::kCancelled:
      return u"Cancelled";
    case AstraDownloadState::kInterrupted:
      return u"Interrupted";
  }
  return std::u16string();
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraDownloadItemView::OnPauseButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadPauseRequested(download_id_);
  }
}

void AstraDownloadItemView::OnCancelButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadCancelRequested(download_id_);
  }
}

void AstraDownloadItemView::OnResumeButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadResumeRequested(download_id_);
  }
}

void AstraDownloadItemView::OnOpenButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadOpenRequested(download_id_);
  }
}

void AstraDownloadItemView::OnShowInFolderButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadShowInFolderRequested(download_id_);
  }
}

// =========================================================================
// Click handling
// =========================================================================

void AstraDownloadItemView::OnItemClicked() {
  if (delegate_) {
    delegate_->OnDownloadItemClicked(download_id_);
  }
}

// =========================================================================
// Hover handling
// =========================================================================

void AstraDownloadItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraDownloadItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseExited(event);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraDownloadItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kDownloadItemHeight));
  return size;
}

void AstraDownloadItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label()) {
    SkColor text_color = is_dangerous_
                             ? color_provider->GetColor(kDownloadDangerousColorId)
                             : color_provider->GetColor(kDownloadItemTextColorId);
    title_label()->SetEnabledColor(text_color);
  }
  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kDownloadItemSecondaryTextColorId));
  }
}

}  // namespace astra
