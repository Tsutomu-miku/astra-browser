#include "astra/ui/views/downloads/astra_downloads_bubble_item_view.h"

#include "base/i18n/number_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

namespace astra {

AstraDownloadsBubbleItemView::AstraDownloadsBubbleItemView(
    const std::string& download_id)
    : download_id_(download_id) {
  SetEventTargetingPolicy(views::EventTargetingPolicy::kEntireHierarchy);
  BuildLayout();
}

AstraDownloadsBubbleItemView::~AstraDownloadsBubbleItemView() = default;

void AstraDownloadsBubbleItemView::SetFilename(const std::u16string& filename) {
  filename_ = filename;
  if (filename_label_) {
    filename_label_->SetText(filename_);
  }
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetURL(const GURL& url) {
  url_ = url;
}

void AstraDownloadsBubbleItemView::SetState(AstraDownloadState state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  UpdateStatusText();
  UpdateProgressBar();
  UpdateActionButtons();
  UpdateIcon();
}

void AstraDownloadsBubbleItemView::SetProgress(double progress) {
  progress_ = progress;
  UpdateProgressBar();
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetBytes(int64_t received, int64_t total) {
  received_bytes_ = received;
  total_bytes_ = total;
  progress_ = CalculateProgress(received, total);
  UpdateProgressBar();
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetSpeed(int64_t bytes_per_sec) {
  speed_bytes_per_sec_ = bytes_per_sec;
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetTimeRemaining(
    base::TimeDelta remaining) {
  time_remaining_ = remaining;
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetIsDangerous(bool dangerous) {
  is_dangerous_ = dangerous;
  UpdateIcon();
  UpdateStatusText();
  UpdateColors();
}

void AstraDownloadsBubbleItemView::SetDangerType(
    const std::string& danger_type) {
  danger_type_ = danger_type;
}

void AstraDownloadsBubbleItemView::UpdateFromItem(
    const AstraDownloadsBubbleItem& item) {
  filename_ = item.filename;
  url_ = item.url;
  received_bytes_ = item.received_bytes;
  total_bytes_ = item.total_bytes;
  speed_bytes_per_sec_ = item.speed_bytes_per_sec;
  state_ = item.state;
  time_remaining_ = item.time_remaining;
  is_dangerous_ = item.is_dangerous;
  danger_type_ = item.danger_type;
  progress_ =
      AstraDownloadsBubbleModel::CalculateProgress(received_bytes_,
                                                    total_bytes_);

  if (filename_label_) {
    filename_label_->SetText(filename_);
  }
  UpdateStatusText();
  UpdateProgressBar();
  UpdateActionButtons();
  UpdateIcon();
  UpdateColors();
}

void AstraDownloadsBubbleItemView::SetShowProgress(bool show) {
  show_progress_ = show;
  UpdateProgressBar();
}

void AstraDownloadsBubbleItemView::SetShowSpeed(bool show) {
  show_speed_ = show;
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetShowTimeRemaining(bool show) {
  show_time_remaining_ = show;
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::SetShowFileSize(bool show) {
  show_file_size_ = show;
  UpdateStatusText();
}

void AstraDownloadsBubbleItemView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, kHorizontalPadding));
  layout->SetDefault(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Icon
  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  icon_view_->SetProperty(views::kMarginsKey,
                          gfx::Insets::TLBR(0, 0, 0, kIconTextSpacing));

  // Text container (filename + status + progress)
  text_container_ =
      AddChildView(std::make_unique<views::View>());
  text_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded,
                               /*weight=*/1.0f));
  auto* text_layout = text_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  text_layout->SetOrientation(views::LayoutOrientation::kVertical);
  text_layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);

  // Filename label
  filename_label_ =
      text_container_->AddChildView(std::make_unique<views::Label>());
  filename_label_->SetText(filename_);
  filename_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  filename_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  filename_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Status label
  status_label_ =
      text_container_->AddChildView(std::make_unique<views::Label>());
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  status_label_->SetProperty(
      views::kMarginsKey, gfx::Insets::TLBR(2, 0, 0, 0));
  status_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Progress bar (shown for in-progress downloads)
  progress_bar_ =
      text_container_->AddChildView(std::make_unique<views::ProgressBar>());
  progress_bar_->SetPreferredSize(
      gfx::Size(0, kProgressBarHeight));
  progress_bar_->SetVisible(show_progress_ &&
                            state_ == AstraDownloadState::kInProgress);
  progress_bar_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(kTextProgressSpacing, 0, 0, 0));
  progress_bar_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Action button container
  action_container_ =
      AddChildView(std::make_unique<views::View>());
  action_container_->SetProperty(views::kMarginsKey,
                                 gfx::Insets::TLBR(0, kTextActionSpacing,
                                                   0, 0));
  auto* action_layout = action_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  action_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  action_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // Create action buttons (hidden by default, shown on hover)
  auto create_button = [this](const std::u16string& tooltip) {
    auto button = std::make_unique<views::ImageButton>(base::BindRepeating(
        []() {}));
    button->SetPreferredSize(
        gfx::Size(kActionButtonSize, kActionButtonSize));
    button->SetTooltipText(tooltip);
    return button;
  };

  pause_button_ = action_container_->AddChildView(create_button(u"Pause"));
  pause_button_->SetVisible(false);
  pause_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnPauseButtonPressed,
      base::Unretained(this)));

  resume_button_ = action_container_->AddChildView(create_button(u"Resume"));
  resume_button_->SetVisible(false);
  resume_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnResumeButtonPressed,
      base::Unretained(this)));

  cancel_button_ = action_container_->AddChildView(create_button(u"Cancel"));
  cancel_button_->SetVisible(false);
  cancel_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnCancelButtonPressed,
      base::Unretained(this)));

  open_button_ = action_container_->AddChildView(create_button(u"Open"));
  open_button_->SetVisible(false);
  open_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnOpenButtonPressed,
      base::Unretained(this)));

  show_in_folder_button_ =
      action_container_->AddChildView(create_button(u"Show in folder"));
  show_in_folder_button_->SetVisible(false);
  show_in_folder_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnShowInFolderButtonPressed,
      base::Unretained(this)));

  retry_button_ = action_container_->AddChildView(create_button(u"Retry"));
  retry_button_->SetVisible(false);
  retry_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnRetryButtonPressed,
      base::Unretained(this)));

  remove_button_ = action_container_->AddChildView(create_button(u"Remove"));
  remove_button_->SetVisible(false);
  remove_button_->SetCallback(base::BindRepeating(
      &AstraDownloadsBubbleItemView::OnRemoveButtonPressed,
      base::Unretained(this)));

  UpdateIcon();
  UpdateStatusText();
  UpdateActionButtons();
}

void AstraDownloadsBubbleItemView::UpdateStatusText() {
  if (!status_label_) {
    return;
  }
  std::u16string status;

  switch (state_) {
    case AstraDownloadState::kInProgress: {
      std::u16string speed_str;
      if (show_speed_ && speed_bytes_per_sec_ > 0) {
        speed_str = AstraDownloadsBubbleModel::FormatSpeed(
            speed_bytes_per_sec_);
      }

      std::u16string size_str;
      if (show_file_size_ && total_bytes_ > 0) {
        size_str =
            AstraDownloadsBubbleModel::FormatBytes(received_bytes_) +
            u" / " +
            AstraDownloadsBubbleModel::FormatBytes(total_bytes_);
      } else if (show_file_size_) {
        size_str = AstraDownloadsBubbleModel::FormatBytes(received_bytes_);
      }

      std::u16string time_str;
      if (show_time_remaining_ && !time_remaining_.is_zero() &&
          !time_remaining_.is_negative()) {
        time_str = u" · " +
                   AstraDownloadsBubbleModel::FormatTimeRemaining(
                       time_remaining_) +
                   u" left";
      }

      status = size_str;
      if (!speed_str.empty()) {
        status += u" · " + speed_str;
      }
      status += time_str;
      break;
    }
    case AstraDownloadState::kCompleted:
      if (show_file_size_) {
        status = AstraDownloadsBubbleModel::FormatBytes(total_bytes_);
      } else {
        status = GetStateLabel();
      }
      break;
    case AstraDownloadState::kFailed:
    case AstraDownloadState::kCancelled:
    case AstraDownloadState::kInterrupted:
      status = GetStateLabel();
      break;
  }

  if (is_dangerous_) {
    status = u"⚠ " + GetStateLabel() + u" — " + status;
  }

  status_label_->SetText(status);
}

void AstraDownloadsBubbleItemView::UpdateProgressBar() {
  if (!progress_bar_) {
    return;
  }
  bool show = show_progress_ && state_ == AstraDownloadState::kInProgress;
  progress_bar_->SetVisible(show);
  if (show) {
    progress_bar_->SetValue(progress_);
  }
}

void AstraDownloadsBubbleItemView::UpdateActionButtons() {
  if (!action_container_ || !pause_button_) {
    return;
  }

  // Show primary action based on state.
  bool show_pause = is_hovered_ && state_ == AstraDownloadState::kInProgress;
  bool show_resume = is_hovered_ && state_ == AstraDownloadState::kInProgress &&
                     false;  // Pause/resume are mutually exclusive in UI
  bool show_cancel = is_hovered_ && state_ == AstraDownloadState::kInProgress;
  bool show_open = is_hovered_ && state_ == AstraDownloadState::kCompleted;
  bool show_in_folder =
      is_hovered_ && state_ == AstraDownloadState::kCompleted;
  bool show_retry = is_hovered_ &&
                    (state_ == AstraDownloadState::kFailed ||
                     state_ == AstraDownloadState::kCancelled ||
                     state_ == AstraDownloadState::kInterrupted);
  bool show_remove = is_hovered_ &&
                     (state_ == AstraDownloadState::kCompleted ||
                      state_ == AstraDownloadState::kFailed ||
                      state_ == AstraDownloadState::kCancelled);

  pause_button_->SetVisible(show_pause);
  cancel_button_->SetVisible(show_cancel);
  open_button_->SetVisible(show_open);
  show_in_folder_button_->SetVisible(show_in_folder);
  retry_button_->SetVisible(show_retry);
  remove_button_->SetVisible(show_remove);
  resume_button_->SetVisible(false);  // Use pause toggle for simplicity

  action_container_->SetVisible(is_hovered_);
}

void AstraDownloadsBubbleItemView::UpdateIcon() {
  // TODO(astra): Use real file type icons from Chromium's resource bundle.
  // For now, use a placeholder colored based on state.
  if (!icon_view_) {
    return;
  }
  // In a real implementation, we'd set the appropriate icon from resources.
  // For now, the icon view is empty and the layout still works.
}

void AstraDownloadsBubbleItemView::UpdateColors() {
  if (!filename_label_ || !status_label_) {
    return;
  }
  // Colors would come from the color provider in a real implementation.
  // For now, default text colors are used.
}

std::u16string AstraDownloadsBubbleItemView::GetStateLabel() const {
  switch (state_) {
    case AstraDownloadState::kInProgress:
      return u"Downloading...";
    case AstraDownloadState::kCompleted:
      return u"Completed";
    case AstraDownloadState::kFailed:
      return u"Failed";
    case AstraDownloadState::kCancelled:
      return u"Cancelled";
    case AstraDownloadState::kInterrupted:
      return u"Interrupted";
  }
  return u"";
}

std::u16string AstraDownloadsBubbleItemView::FormatBytes(
    int64_t bytes) const {
  return AstraDownloadsBubbleModel::FormatBytes(bytes);
}

std::u16string AstraDownloadsBubbleItemView::FormatSpeed(
    int64_t bytes_per_sec) const {
  return AstraDownloadsBubbleModel::FormatSpeed(bytes_per_sec);
}

std::u16string AstraDownloadsBubbleItemView::FormatTimeRemaining(
    base::TimeDelta remaining) const {
  return AstraDownloadsBubbleModel::FormatTimeRemaining(remaining);
}

void AstraDownloadsBubbleItemView::OnPauseButtonPressed() {
  if (delegate_) {
    delegate_->OnPauseDownload(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnResumeButtonPressed() {
  if (delegate_) {
    delegate_->OnResumeDownload(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnCancelButtonPressed() {
  if (delegate_) {
    delegate_->OnCancelDownload(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnOpenButtonPressed() {
  if (delegate_) {
    delegate_->OnOpenDownload(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnShowInFolderButtonPressed() {
  if (delegate_) {
    delegate_->OnShowDownloadInFolder(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnRetryButtonPressed() {
  if (delegate_) {
    delegate_->OnRetryDownload(download_id_);
  }
}

void AstraDownloadsBubbleItemView::OnRemoveButtonPressed() {
  if (delegate_) {
    delegate_->OnRemoveDownload(download_id_);
  }
}

gfx::Size AstraDownloadsBubbleItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(0);
  return gfx::Size(width, kItemHeight);
}

void AstraDownloadsBubbleItemView::Layout() {
  views::View::Layout();
}

void AstraDownloadsBubbleItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

bool AstraDownloadsBubbleItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && delegate_) {
    delegate_->OnDownloadItemClicked(download_id_);
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraDownloadsBubbleItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateActionButtons();
  views::View::OnMouseEntered(event);
}

void AstraDownloadsBubbleItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  UpdateActionButtons();
  views::View::OnMouseExited(event);
}

void AstraDownloadsBubbleItemView::OnGestureEvent(
    ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP && delegate_) {
    delegate_->OnDownloadItemClicked(download_id_);
    event->SetHandled();
    return;
  }
  views::View::OnGestureEvent(event);
}

void AstraDownloadsBubbleItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(filename_);
}

std::u16string AstraDownloadsBubbleItemView::GetTooltipText(
    const gfx::Point& p) const {
  return filename_ + u"\n" + GetStateLabel();
}

}  // namespace astra
