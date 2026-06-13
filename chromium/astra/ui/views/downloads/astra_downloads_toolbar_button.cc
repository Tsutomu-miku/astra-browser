#include "astra/ui/views/downloads/astra_downloads_toolbar_button.h"

#include <algorithm>
#include <cmath>

#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"
#include "astra/ui/views/downloads/astra_downloads_bubble_view.h"

namespace astra {

AstraDownloadsToolbarButton::AstraDownloadsToolbarButton(
    AstraDownloadsBubbleModel* model)
    : views::ImageButton(base::BindRepeating(
          &AstraDownloadsToolbarButton::OnButtonPressed,
          base::Unretained(this))),
      model_(model) {
  SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  SetTooltipText(u"Downloads");

  if (model_) {
    model_observation_.Observe(model_);
    UpdateStateFromModel();
  }
}

AstraDownloadsToolbarButton::~AstraDownloadsToolbarButton() = default;

void AstraDownloadsToolbarButton::SetModel(
    AstraDownloadsBubbleModel* model) {
  if (model_observation_.IsObserving()) {
    model_observation_.Reset();
  }
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
    UpdateStateFromModel();
  }
}

void AstraDownloadsToolbarButton::ShowBubble(
    AstraDownloadsBubbleDelegate* delegate) {
  if (IsBubbleShowing() || !model_) {
    return;
  }
  gfx::Rect anchor_rect = GetMirroredBounds();
  bubble_widget_ = AstraDownloadsBubbleView::ShowBubble(
      this, anchor_rect, model_, delegate);
}

void AstraDownloadsToolbarButton::HideBubble() {
  if (bubble_widget_) {
    bubble_widget_->Close();
    bubble_widget_ = nullptr;
  }
}

bool AstraDownloadsToolbarButton::IsBubbleShowing() const {
  return bubble_widget_ != nullptr && !bubble_widget_->IsClosed();
}

void AstraDownloadsToolbarButton::SetActiveCount(int count) {
  if (active_count_ == count) {
    return;
  }
  active_count_ = count;
  UpdateBadge();
  UpdateState();
  SchedulePaint();
}

void AstraDownloadsToolbarButton::SetOverallProgress(double progress) {
  progress = std::clamp(progress, 0.0, 1.0);
  if (std::fabs(overall_progress_ - progress) < 0.001) {
    return;
  }
  overall_progress_ = progress;
  SchedulePaint();
}

void AstraDownloadsToolbarButton::SetShowBadge(bool show) {
  if (show_badge_ == show) {
    return;
  }
  show_badge_ = show;
  SchedulePaint();
}

void AstraDownloadsToolbarButton::SetShowProgressRing(bool show) {
  if (show_progress_ring_ == show) {
    return;
  }
  show_progress_ring_ = show;
  SchedulePaint();
}

void AstraDownloadsToolbarButton::PlayCompleteAnimation() {
  if (animating_complete_) {
    return;
  }
  animating_complete_ = true;
  animation_frame_ = 0;
  animation_timer_.Start(
      FROM_HERE, kCompleteAnimationDuration / kAnimationFrameCount,
      base::BindRepeating(
          &AstraDownloadsToolbarButton::OnCompleteAnimationTick,
          base::Unretained(this)));
}

void AstraDownloadsToolbarButton::PlayStartAnimation() {
  if (animating_start_) {
    return;
  }
  animating_start_ = true;
  animation_frame_ = 0;
  animation_timer_.Start(
      FROM_HERE, kStartAnimationDuration / kAnimationFrameCount,
      base::BindRepeating(
          &AstraDownloadsToolbarButton::OnStartAnimationTick,
          base::Unretained(this)));
}

double AstraDownloadsToolbarButton::CalculateOverallProgress() const {
  if (!model_ || model_->GetActiveDownloadCount() == 0) {
    return 0.0;
  }
  // Simple average of all active download progress.
  // In production, we'd compute weighted average by file size.
  auto items = model_->GetDisplayDownloads();
  double total_progress = 0.0;
  int active_count = 0;
  for (const auto& item : items) {
    if (item.state == AstraDownloadState::kInProgress) {
      total_progress += AstraDownloadsBubbleModel::CalculateProgress(
          item.received_bytes, item.total_bytes);
      active_count++;
    }
  }
  if (active_count == 0) {
    return 0.0;
  }
  return total_progress / active_count;
}

void AstraDownloadsToolbarButton::UpdateStateFromModel() {
  if (!model_) {
    state_ = State::kIdle;
    active_count_ = 0;
    overall_progress_ = 0.0;
  } else {
    int active = model_->GetActiveDownloadCount();
    int total = model_->GetTotalDownloadCount();
    active_count_ = active;

    if (active > 0) {
      state_ = State::kDownloading;
    } else if (total > 0) {
      state_ = State::kCompleted;
    } else {
      state_ = State::kIdle;
    }

    overall_progress_ = CalculateOverallProgress();
    show_badge_ = model_->show_badge();
    show_progress_ring_ = model_->show_progress_ring();
  }

  UpdateBadge();
  UpdateIcon();
  UpdateTooltip();
  SchedulePaint();
}

void AstraDownloadsToolbarButton::UpdateBadge() {
  // Badge is painted directly in PaintButtonContents.
  SchedulePaint();
}

void AstraDownloadsToolbarButton::UpdateIcon() {
  // TODO(astra): Set real download icon from Chromium's resource bundle.
  // For now, we draw the icon in PaintButtonContents.
  SchedulePaint();
}

void AstraDownloadsToolbarButton::UpdateProgressRing() {
  SchedulePaint();
}

void AstraDownloadsToolbarButton::UpdateTooltip() {
  std::u16string tooltip;
  if (active_count_ > 0) {
    tooltip = base::UTF8ToUTF16(
        base::StringPrintf("%d download(s) in progress", active_count_));
  } else {
    tooltip = u"Downloads";
  }
  SetTooltipText(tooltip);
}

void AstraDownloadsToolbarButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();
  int center_x = bounds.x() + bounds.width() / 2;
  int center_y = bounds.y() + bounds.height() / 2;

  // Draw download icon (simple arrow-down shape).
  int icon_size = kIconSize;
  int icon_left = center_x - icon_size / 2;
  int icon_top = center_y - icon_size / 2;

  // Draw a simple download arrow icon.
  // In production, we'd use a real icon from the resource bundle.
  SkColor icon_color = SK_ColorBLACK;

  // Draw progress ring if active downloads exist and ring is enabled.
  if (show_progress_ring_ && active_count_ > 0) {
    PaintProgressRing(canvas);
  }

  // Draw badge if enabled and there are active downloads.
  if (show_badge_ && active_count_ > 0) {
    PaintBadge(canvas);
  }
}

void AstraDownloadsToolbarButton::PaintProgressRing(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();
  int center_x = bounds.x() + bounds.width() / 2;
  int center_y = bounds.y() + bounds.height() / 2;

  int ring_radius = kIconSize / 2 + kProgressRingInset;
  int stroke_width = kProgressRingStrokeWidth;

  // Draw background ring (track).
  SkPaint track_paint;
  track_paint.setStyle(SkPaint::kStroke_Style);
  track_paint.setStrokeWidth(stroke_width);
  track_paint.setColor(SkColorSetA(SK_ColorGRAY, 64));
  track_paint.setAntiAlias(true);
  canvas->DrawCircle(gfx::Point(center_x, center_y), ring_radius,
                     track_paint);

  // Draw progress arc.
  SkPaint progress_paint;
  progress_paint.setStyle(SkPaint::kStroke_Style);
  progress_paint.setStrokeWidth(stroke_width);
  progress_paint.setColor(SK_ColorBLUE);
  progress_paint.setAntiAlias(true);
  progress_paint.setStrokeCap(SkPaint::kRound_Cap);

  float sweep_angle = overall_progress_ * 360.0f;
  // Start from top (12 o'clock position) = -90 degrees.
  float start_angle = -90.0f;

  gfx::RectF arc_rect(
      center_x - ring_radius, center_y - ring_radius,
      ring_radius * 2, ring_radius * 2);
  canvas->DrawArc(arc_rect, start_angle, sweep_angle, false,
                   progress_paint);
}

void AstraDownloadsToolbarButton::PaintBadge(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();

  // Position badge in top-right corner.
  int badge_x = bounds.right() - kBadgeSize - 2;
  int badge_y = bounds.y() + 2;

  // Draw badge background (circle).
  SkPaint badge_paint;
  badge_paint.setStyle(SkPaint::kFill_Style);
  badge_paint.setColor(SK_ColorRED);
  badge_paint.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(badge_x + kBadgeSize / 2,
                                 badge_y + kBadgeSize / 2),
                     kBadgeSize / 2, badge_paint);

  // Draw badge text.
  std::u16string text;
  if (active_count_ > 9) {
    text = u"9+";
  } else {
    text = base::NumberToString16(active_count_);
  }

  canvas->DrawStringRectWithFlags(
      text, gfx::FontList(), SK_ColorWHITE,
      gfx::Rect(badge_x, badge_y, kBadgeSize, kBadgeSize),
      gfx::Canvas::TEXT_ALIGN_CENTER | gfx::Canvas::VALIGN_MIDDLE |
          gfx::Canvas::NO_SUBPIXEL_RENDERING);
}

void AstraDownloadsToolbarButton::OnButtonPressed() {
  // Toggle bubble visibility.
  if (IsBubbleShowing()) {
    HideBubble();
  } else {
    // The delegate should be set by the caller. For the basic toggle,
    // we show the bubble without a delegate (actions are no-ops).
    ShowBubble(nullptr);
  }
}

void AstraDownloadsToolbarButton::OnCompleteAnimationTick() {
  animation_frame_++;
  SchedulePaint();
  if (animation_frame_ >= kAnimationFrameCount) {
    OnAnimationFinished();
  }
}

void AstraDownloadsToolbarButton::OnStartAnimationTick() {
  animation_frame_++;
  SchedulePaint();
  if (animation_frame_ >= kAnimationFrameCount) {
    OnAnimationFinished();
  }
}

void AstraDownloadsToolbarButton::OnAnimationFinished() {
  animating_complete_ = false;
  animating_start_ = false;
  animation_timer_.Stop();
}

gfx::Size AstraDownloadsToolbarButton::CalculatePreferredSize() const {
  return gfx::Size(kButtonSize, kButtonSize);
}

void AstraDownloadsToolbarButton::Layout() {
  views::ImageButton::Layout();
}

void AstraDownloadsToolbarButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  SchedulePaint();
}

void AstraDownloadsToolbarButton::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::ImageButton::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(u"Downloads");
  if (active_count_ > 0) {
    node_data->AddState(ax::mojom::State::kLiveRegion);
  }
}

bool AstraDownloadsToolbarButton::OnMousePressed(
    const ui::MouseEvent& event) {
  return views::ImageButton::OnMousePressed(event);
}

void AstraDownloadsToolbarButton::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  views::ImageButton::OnMouseEntered(event);
}

void AstraDownloadsToolbarButton::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  views::ImageButton::OnMouseExited(event);
}

// -- AstraDownloadsBubbleObserver ---------------------------------------

void AstraDownloadsToolbarButton::OnDownloadsChanged(
    AstraDownloadsBubbleModel* model) {
  DCHECK_EQ(model, model_);
  UpdateStateFromModel();
}

void AstraDownloadsToolbarButton::OnDownloadUpdated(
    AstraDownloadsBubbleModel* model,
    const std::string& download_id) {
  DCHECK_EQ(model, model_);
  // Update progress ring.
  overall_progress_ = CalculateOverallProgress();
  SchedulePaint();
}

void AstraDownloadsToolbarButton::OnDownloadStarted(
    AstraDownloadsBubbleModel* model,
    const std::string& download_id) {
  DCHECK_EQ(model, model_);
  UpdateStateFromModel();
  PlayStartAnimation();
}

void AstraDownloadsToolbarButton::OnDownloadCompleted(
    AstraDownloadsBubbleModel* model,
    const std::string& download_id) {
  DCHECK_EQ(model, model_);
  UpdateStateFromModel();
  PlayCompleteAnimation();
}

void AstraDownloadsToolbarButton::OnActiveCountChanged(
    AstraDownloadsBubbleModel* model,
    int active_count) {
  DCHECK_EQ(model, model_);
  SetActiveCount(active_count);
}

void AstraDownloadsToolbarButton::OnDownloadsBubbleModelShutdown(
    AstraDownloadsBubbleModel* model) {
  DCHECK_EQ(model, model_);
  model_observation_.Reset();
  model_ = nullptr;
  HideBubble();
  UpdateStateFromModel();
}

}  // namespace astra
