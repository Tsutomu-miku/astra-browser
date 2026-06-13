// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_shelf/astra_downloads_shelf_item_view.h"

#include <algorithm>
#include <cmath>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// File extension -> icon color mapping (simple heuristic).
SkColor GetIconColorForFile(const std::u16string& filename) {
  if (filename.empty())
    return SK_ColorGRAY;

  size_t dot_pos = filename.find_last_of('.');
  if (dot_pos == std::u16string::npos)
    return SK_ColorGRAY;

  std::u16string ext = filename.substr(dot_pos + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (ext == u"pdf")
    return SkColorSetRGB(0xE5, 0x39, 0x35);
  if (ext == u"zip" || ext == u"rar" || ext == u"7z" || ext == u"tar" ||
      ext == u"gz")
    return SkColorSetRGB(0xFF, 0xA0, 0x00);
  if (ext == u"mp3" || ext == u"wav" || ext == u"flac" || ext == u"aac")
    return SkColorSetRGB(0x42, 0x85, 0xF4);
  if (ext == u"mp4" || ext == u"avi" || ext == u"mkv" || ext == u"mov" ||
      ext == u"webm")
    return SkColorSetRGB(0x9C, 0x27, 0xB0);
  if (ext == u"jpg" || ext == u"jpeg" || ext == u"png" || ext == u"gif" ||
      ext == u"webp" || ext == u"svg" || ext == u"bmp")
    return SkColorSetRGB(0x2E, 0x7D, 0x32);
  if (ext == u"txt" || ext == u"md" || ext == u"log")
    return SkColorSetRGB(0x60, 0x7D, 0x8B);
  if (ext == u"html" || ext == u"htm" || ext == u"css" || ext == u"js" ||
      ext == u"json")
    return SkColorSetRGB(0xFF, 0x6D, 0x00);
  return SkColorSetRGB(0x5C, 0x6B, 0xC0);
}

// Draw a simple file icon on a canvas.
void DrawFileIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color,
                  bool is_dangerous) {
  if (bounds.IsEmpty())
    return;

  SkColor icon_color = is_dangerous ? SkColorSetRGB(0xD3, 0x2F, 0x2F) : color;

  // Simple file icon shape: rectangle with folded corner.
  int x = bounds.x();
  int y = bounds.y();
  int w = bounds.width();
  int h = bounds.height();
  int corner_size = w / 4;

  cc::PaintFlags flags;
  flags.setColor(icon_color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(x, y);
  path.lineTo(x + w - corner_size, y);
  path.lineTo(x + w, y + corner_size);
  path.lineTo(x + w, y + h);
  path.lineTo(x, y + h);
  path.close();
  canvas->DrawPath(path, flags);

  // Fold line.
  flags.setColor(SkColorSetA(SK_ColorWHITE, 0x33));
  flags.setStrokeWidth(1);
  canvas->DrawLine(x + w - corner_size, y, x + w - corner_size,
                   y + corner_size, flags);
  canvas->DrawLine(x + w - corner_size, y + corner_size, x + w,
                   y + corner_size, flags);
}

}  // namespace

// =========================================================================
// AstraDownloadsShelfItemView
// =========================================================================

AstraDownloadsShelfItemView::AstraDownloadsShelfItemView(
    int download_id,
    const std::u16string& filename)
    : download_id_(download_id), filename_(filename) {
  SetCanProcessEventsWithinSubtree(true);
  BuildLayout();
}

AstraDownloadsShelfItemView::~AstraDownloadsShelfItemView() = default;

// -- Data setters -----------------------------------------------------------

void AstraDownloadsShelfItemView::SetFilename(const std::u16string& filename) {
  filename_ = filename;
  if (filename_label_)
    filename_label_->SetText(filename_);
  UpdateIcon();
  UpdateDisplay();
}

void AstraDownloadsShelfItemView::SetURL(const GURL& url) {
  url_ = url;
  UpdateIcon();
}

void AstraDownloadsShelfItemView::SetState(AstraDownloadState state) {
  state_ = state;
  UpdateDisplay();
  UpdateActionButtons();
  UpdateIcon();
  SchedulePaint();
}

void AstraDownloadsShelfItemView::SetProgress(double progress) {
  progress_ = std::clamp(progress, 0.0, 1.0);
  if (progress_bar_)
    progress_bar_->SetValue(progress_);
  UpdateDisplay();
}

void AstraDownloadsShelfItemView::SetBytes(int64_t received, int64_t total) {
  received_bytes_ = received;
  total_bytes_ = total;
  UpdateDisplay();
}

void AstraDownloadsShelfItemView::SetSpeed(int64_t bytes_per_sec) {
  speed_bytes_per_sec_ = bytes_per_sec;
  UpdateDisplay();
}

void AstraDownloadsShelfItemView::SetDanger(bool is_dangerous) {
  is_dangerous_ = is_dangerous;
  UpdateIcon();
  UpdateDisplay();
  SchedulePaint();
}

// -- views::View ------------------------------------------------------------

gfx::Size AstraDownloadsShelfItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kItemWidth, kItemHeight);
}

void AstraDownloadsShelfItemView::Layout() {
  if (!icon_view_ || !text_container_ || !action_container_ || !progress_bar_)
    return;

  int x = kHorizontalPadding;
  int y = (kItemHeight - kIconSize) / 2;

  // Icon.
  icon_view_->SetBounds(x, y, kIconSize, kIconSize);
  x += kIconSize + kIconTextSpacing;

  // Action buttons (right side).
  int action_width = 0;
  if (action_container_ && action_container_->GetVisible()) {
    action_width = action_container_->GetPreferredSize().width();
    int action_x = width() - kHorizontalPadding - action_width;
    int action_y = (kItemHeight - kActionButtonSize) / 2;
    action_container_->SetBounds(action_x, action_y, action_width,
                                 kActionButtonSize);
  }

  // Text container fills space between icon and action buttons.
  int text_x = x;
  int text_width = width() - x - kHorizontalPadding -
                   (action_width > 0 ? action_width + kIconTextSpacing : 0);
  int text_height = kItemHeight - kProgressBarHeight - 4;
  text_container_->SetBounds(text_x, 2, text_width, text_height);

  // Progress bar at bottom.
  progress_bar_->SetBounds(kHorizontalPadding, kItemHeight - kProgressBarHeight,
                           width() - 2 * kHorizontalPadding, kProgressBarHeight);

  // Layout text children: filename on top, status below.
  if (filename_label_ && status_label_) {
    int label_width = text_container_->width();
    filename_label_->SetBounds(0, 0, label_width, text_height / 2 + 2);
    status_label_->SetBounds(0, text_height / 2 + 2, label_width,
                             text_height / 2 - 2);
  }
}

void AstraDownloadsShelfItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

bool AstraDownloadsShelfItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && event.GetClickCount() == 1) {
    if (delegate_)
      delegate_->OnDownloadClicked(download_id_);
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraDownloadsShelfItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateActionButtons();
  SchedulePaint();
}

void AstraDownloadsShelfItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  UpdateActionButtons();
  SchedulePaint();
}

void AstraDownloadsShelfItemView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    if (delegate_)
      delegate_->OnDownloadClicked(download_id_);
    event->SetHandled();
  }
}

void AstraDownloadsShelfItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->SetName(GetStateLabel() + u" " + filename_);
}

std::u16string AstraDownloadsShelfItemView::GetTooltipText(
    const gfx::Point& p) const {
  std::u16string tooltip = filename_ + u"\n" + GetStateLabel();
  if (total_bytes_ > 0) {
    tooltip += u" " + FormatBytes(total_bytes_);
  }
  if (!url_.is_empty()) {
    tooltip += u"\n" + base::UTF8ToUTF16(url_.spec());
  }
  return tooltip;
}

// -- Private methods --------------------------------------------------------

void AstraDownloadsShelfItemView::BuildLayout() {
  // Icon view (draws file icon).
  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetImageSize(gfx::Size(kIconSize, kIconSize));

  // Text container (filename + status).
  auto text_container = std::make_unique<views::View>();
  text_container_ = AddChildView(std::move(text_container));

  // Filename label.
  auto filename_label = std::make_unique<views::Label>(filename_);
  filename_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  filename_label->SetAutoColorReadabilityEnabled(false);
  filename_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  filename_label_ = text_container_->AddChildView(std::move(filename_label));

  // Status label.
  auto status_label = std::make_unique<views::Label>(GetStateLabel());
  status_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label->SetAutoColorReadabilityEnabled(false);
  status_label->SetElideBehavior(gfx::ELIDE_TAIL);
  status_label_ = text_container_->AddChildView(std::move(status_label));

  // Progress bar.
  auto progress_bar = std::make_unique<views::ProgressBar>(kProgressBarHeight);
  progress_bar->SetValue(0.0);
  progress_bar_ = AddChildView(std::move(progress_bar));

  // Action button container.
  auto action_container = std::make_unique<views::View>();
  action_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kActionButtonSpacing));
  action_container_ = AddChildView(std::move(action_container));

  // Pause button.
  auto pause_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfItemView::OnPausePressed,
                          base::Unretained(this)));
  pause_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  pause_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  pause_button_ = action_container_->AddChildView(std::move(pause_button));

  // Resume button.
  auto resume_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfItemView::OnResumePressed,
                          base::Unretained(this)));
  resume_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  resume_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  resume_button_ = action_container_->AddChildView(std::move(resume_button));

  // Cancel button.
  auto cancel_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfItemView::OnCancelPressed,
                          base::Unretained(this)));
  cancel_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  cancel_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  cancel_button_ = action_container_->AddChildView(std::move(cancel_button));

  // Show in folder button.
  auto show_in_folder_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfItemView::OnShowInFolderPressed,
                          base::Unretained(this)));
  show_in_folder_button->SetImageHorizontalAlignment(
      views::ImageButton::ALIGN_CENTER);
  show_in_folder_button->SetImageVerticalAlignment(
      views::ImageButton::ALIGN_MIDDLE);
  show_in_folder_button_ =
      action_container_->AddChildView(std::move(show_in_folder_button));

  // Dismiss button.
  auto dismiss_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfItemView::OnDismissPressed,
                          base::Unretained(this)));
  dismiss_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  dismiss_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  dismiss_button_ = action_container_->AddChildView(std::move(dismiss_button));

  UpdateDisplay();
  UpdateActionButtons();
  UpdateIcon();
  UpdateColors();
}

void AstraDownloadsShelfItemView::UpdateDisplay() {
  if (status_label_)
    status_label_->SetText(GetStateLabel());

  if (progress_bar_) {
    bool show_progress =
        state_ == AstraDownloadState::kInProgress ||
        state_ == AstraDownloadState::kPaused;
    progress_bar_->SetVisible(show_progress);
    if (show_progress)
      progress_bar_->SetValue(progress_);
  }

  if (filename_label_) {
    // Elide filename to fit.
    gfx::FontList font_list;
    int available_width =
        text_container_ ? text_container_->width() : kItemWidth - 60;
    std::u16string elided = gfx::ElideText(
        filename_, font_list, available_width, gfx::ELIDE_MIDDLE);
    filename_label_->SetText(elided);
  }
}

void AstraDownloadsShelfItemView::UpdateActionButtons() {
  if (!action_container_)
    return;

  bool show_actions = is_hovered_;
  action_container_->SetVisible(show_actions);

  if (!show_actions) {
    Layout();
    return;
  }

  // Show state-appropriate buttons.
  bool is_in_progress = state_ == AstraDownloadState::kInProgress;
  bool is_paused = state_ == AstraDownloadState::kPaused;
  bool is_complete = state_ == AstraDownloadState::kCompleted;
  bool is_failed = state_ == AstraDownloadState::kFailed ||
                   state_ == AstraDownloadState::kInterrupted ||
                   state_ == AstraDownloadState::kCancelled;

  if (pause_button_)
    pause_button_->SetVisible(is_in_progress);
  if (resume_button_)
    resume_button_->SetVisible(is_paused);
  if (cancel_button_)
    cancel_button_->SetVisible(is_in_progress || is_paused);
  if (show_in_folder_button_)
    show_in_folder_button_->SetVisible(is_complete);
  if (dismiss_button_)
    dismiss_button_->SetVisible(is_complete || is_failed);

  Layout();
}

void AstraDownloadsShelfItemView::UpdateIcon() {
  if (!icon_view_)
    return;

  // Generate icon image from file type.
  gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), /*image_scale=*/1.0f,
                     false);
  DrawFileIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize),
               GetIconColorForFile(filename_), is_dangerous_);
  icon_view_->SetImage(
      gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(kIconSize, kIconSize)));
}

void AstraDownloadsShelfItemView::UpdateColors() {
  if (!GetWidget())
    return;

  // Use default text color from native theme.
  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);
  SkColor secondary_color =
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelSecondaryColor);

  if (filename_label_)
    filename_label_->SetEnabledColor(text_color);
  if (status_label_)
    status_label_->SetEnabledColor(secondary_color);
}

// -- Button handlers --------------------------------------------------------

void AstraDownloadsShelfItemView::OnPausePressed() {
  if (delegate_)
    delegate_->OnPauseDownload(download_id_);
}

void AstraDownloadsShelfItemView::OnResumePressed() {
  if (delegate_)
    delegate_->OnResumeDownload(download_id_);
}

void AstraDownloadsShelfItemView::OnCancelPressed() {
  if (delegate_)
    delegate_->OnCancelDownload(download_id_);
}

void AstraDownloadsShelfItemView::OnShowInFolderPressed() {
  if (delegate_)
    delegate_->OnShowInFolder(download_id_);
}

void AstraDownloadsShelfItemView::OnDismissPressed() {
  if (delegate_)
    delegate_->OnDismissItem(download_id_);
}

// -- Format helpers ---------------------------------------------------------

std::u16string AstraDownloadsShelfItemView::FormatBytes(int64_t bytes) const {
  if (bytes < 0)
    return u"--";
  if (bytes < 1024)
    return base::NumberToString16(bytes) + u" B";
  if (bytes < 1024 * 1024)
    return base::StringPrintf(u"%.1f KB", bytes / 1024.0);
  if (bytes < 1024 * 1024 * 1024)
    return base::StringPrintf(u"%.1f MB", bytes / (1024.0 * 1024.0));
  return base::StringPrintf(u"%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

std::u16string AstraDownloadsShelfItemView::FormatSpeed(
    int64_t bytes_per_sec) const {
  if (bytes_per_sec <= 0)
    return u"--";
  return FormatBytes(bytes_per_sec) + u"/s";
}

std::u16string AstraDownloadsShelfItemView::GetStateLabel() const {
  switch (state_) {
    case AstraDownloadState::kInProgress: {
      std::u16string progress_str =
          base::StringPrintf(u"%.1f%%", progress_ * 100.0);
      std::u16string speed_str = FormatSpeed(speed_bytes_per_sec_);
      if (total_bytes_ > 0) {
        return FormatBytes(received_bytes_) + u" / " +
               FormatBytes(total_bytes_) + u" (" + progress_str + u") " +
               speed_str;
      }
      return progress_str + u" " + speed_str;
    }
    case AstraDownloadState::kPaused:
      return u"Paused";
    case AstraDownloadState::kCompleted:
      if (total_bytes_ > 0)
        return u"Complete — " + FormatBytes(total_bytes_);
      return u"Complete";
    case AstraDownloadState::kFailed:
      return u"Failed";
    case AstraDownloadState::kCancelled:
      return u"Cancelled";
    case AstraDownloadState::kInterrupted:
      return u"Interrupted";
    case AstraDownloadState::kDangerous:
      return u"Dangerous — review";
    case AstraDownloadState::kUnknown:
      return u"Unknown";
  }
  return u"";
}

}  // namespace astra
