// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reading_progress/astra_reading_progress_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "skia/core/SkRect.h"
#include "skia/core/SkCanvas.h"
#include "skia/core/SkPaint.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kItemHeight = 72;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 4;
constexpr int kProgressBarHeight = 6;
constexpr int kProgressBarRadius = 3;

}  // namespace

// ===========================================================================
// AstraReadingProgressItemView
// ===========================================================================

AstraReadingProgressItemView::AstraReadingProgressItemView(
    const ArticleInfo& info,
    OpenCallback open_callback,
    RemoveCallback remove_callback)
    : article_id_(info.article_id),
      title_(info.title),
      domain_(info.domain),
      progress_percent_(info.progress_percent),
      total_words_(info.total_words),
      read_words_(info.read_words),
      time_remaining_(info.time_remaining),
      last_read_(info.last_read),
      open_callback_(std::move(open_callback)),
      remove_callback_(std::move(remove_callback)) {
  BuildLayout();
}

AstraReadingProgressItemView::~AstraReadingProgressItemView() = default;

void AstraReadingProgressItemView::SetProgress(int percent) {
  progress_percent_ = std::clamp(percent, 0, 100);
  if (detail_label_) {
    detail_label_->SetText(
        base::UTF8ToUTF16(
            std::to_string(progress_percent_) + "% · " +
            base::UTF16ToUTF8(FormatTimeRemaining(time_remaining_)) +
            " left"));
  }
  SchedulePaint();
}

void AstraReadingProgressItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Title.
  title_label_ = AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Detail row: domain + progress.
  auto* detail_row = AddChildView(std::make_unique<views::View>());
  detail_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  detail_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  detail_label_ = detail_row->AddChildView(
      std::make_unique<views::Label>(
          domain_ + u" · " +
          base::UTF8ToUTF16(
              std::to_string(progress_percent_) + "% · " +
              base::UTF16ToUTF8(FormatTimeRemaining(time_remaining_)) +
              " left")));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  detail_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraReadingProgressItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  PaintProgressBar(canvas);
}

void AstraReadingProgressItemView::PaintProgressBar(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();
  int x = 12;
  int y = bounds.height() - 10 - kProgressBarHeight;
  int width = bounds.width() - 24;
  int height = kProgressBarHeight;

  SkPaint bg_paint;
  bg_paint.setColor(SkColorSetA(SK_ColorGRAY, 0x30));
  bg_paint.setStyle(SkPaint::kFill_Style);
  bg_paint.setAntiAlias(true);

  SkRect bg_rect = SkRect::MakeXYWH(x, y, width, height);
  canvas->drawRoundRect(bg_rect, kProgressBarRadius, kProgressBarRadius,
                        bg_paint);

  int progress_width =
      static_cast<int>(width * progress_percent_ / 100.0);
  if (progress_width < kProgressBarRadius * 2) {
    progress_width = kProgressBarRadius * 2;
  }

  SkPaint fg_paint;
  const auto* cp = GetColorProvider();
  SkColor accent =
      cp ? cp->GetColor(ui::kColorButtonBackgroundProminent)
         : SkColorSetRGB(0x1A, 0x73, 0xE8);
  fg_paint.setColor(accent);
  fg_paint.setStyle(SkPaint::kFill_Style);
  fg_paint.setAntiAlias(true);

  SkRect fg_rect = SkRect::MakeXYWH(x, y, progress_width, height);
  canvas->drawRoundRect(fg_rect, kProgressBarRadius, kProgressBarRadius,
                        fg_paint);
}

void AstraReadingProgressItemView::OnOpenClicked() {
  if (open_callback_) {
    open_callback_.Run(article_id_);
  }
}

void AstraReadingProgressItemView::OnRemoveClicked() {
  if (remove_callback_) {
    remove_callback_.Run(article_id_);
  }
}

std::u16string AstraReadingProgressItemView::FormatTimeRemaining(
    base::TimeDelta delta) {
  int minutes = delta.InMinutes();
  if (minutes < 1) return u"< 1 min";
  if (minutes < 60) {
    return base::UTF8ToUTF16(std::to_string(minutes) + " min");
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  if (mins == 0) {
    return base::UTF8ToUTF16(std::to_string(hours) + "h");
  }
  return base::UTF8ToUTF16(
      std::to_string(hours) + "h " + std::to_string(mins) + "m");
}

void AstraReadingProgressItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
  SchedulePaint();
}

// ===========================================================================
// AstraReadingProgressView
// ===========================================================================

AstraReadingProgressView::AstraReadingProgressView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraReadingProgressView::~AstraReadingProgressView() = default;

void AstraReadingProgressView::SetArticles(
    const std::vector<AstraReadingProgressItemView::ArticleInfo>& articles) {
  articles_ = articles;
  RefreshArticles();
}

void AstraReadingProgressView::SetWeeklyStats(const WeeklyStats& stats) {
  stats_ = stats;
  RefreshStats();
}

void AstraReadingProgressView::SetOpenArticleCallback(
    OpenArticleCallback callback) {
  open_callback_ = std::move(callback);
}

void AstraReadingProgressView::SetRemoveArticleCallback(
    RemoveArticleCallback callback) {
  remove_callback_ = std::move(callback);
}

void AstraReadingProgressView::SetViewAllCallback(
    ViewAllCallback callback) {
  view_all_callback_ = std::move(callback);
}

void AstraReadingProgressView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildStatsSection();
  BuildArticlesList();
}

void AstraReadingProgressView::BuildStatsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 6));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  articles_read_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"This week: 0 articles · 0 min read"));
  articles_read_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  articles_read_label_->SetAutoColorReadabilityEnabled(false);
  articles_read_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  streak_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"🔥 Streak: 0 days"));
  streak_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  streak_label_->SetAutoColorReadabilityEnabled(false);
  streak_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
}

void AstraReadingProgressView::BuildArticlesList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kItemSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Currently reading (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(kItemHeight * kMaxVisibleItems +
                              kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  articles_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  articles_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraReadingProgressView::RefreshArticles() {
  if (!articles_list_) return;

  articles_list_->RemoveAllChildViews();
  article_views_.clear();

  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(
        "Currently reading (" +
        std::to_string(articles_.size()) + ")"));
  }

  // Sort by last read time descending (most recently read first).
  auto sorted = articles_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              return a.last_read > b.last_read;
            });

  for (const auto& article : sorted) {
    auto* item = articles_list_->AddChildView(
        std::make_unique<AstraReadingProgressItemView>(
            article,
            base::BindRepeating(
                &AstraReadingProgressView::OnOpenArticle,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraReadingProgressView::OnRemoveArticle,
                base::Unretained(this))));
    article_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraReadingProgressView::RefreshStats() {
  if (articles_read_label_) {
    articles_read_label_->SetText(
        base::UTF8ToUTF16(
            "This week: " +
            std::to_string(stats_.articles_read) +
            " articles · " +
            base::UTF16ToUTF8(FormatReadTime(stats_.total_read_time)) +
            " read"));
  }
  if (streak_label_) {
    streak_label_->SetText(base::UTF8ToUTF16(
        "🔥 Streak: " +
        std::to_string(stats_.current_streak_days) + " days" +
        (stats_.longest_streak_days > stats_.current_streak_days
             ? " (best: " + std::to_string(stats_.longest_streak_days) + ")"
             : "")));
  }
}

std::u16string AstraReadingProgressView::FormatReadTime(
    base::TimeDelta delta) {
  int minutes = delta.InMinutes();
  if (minutes < 1) return u"< 1 min";
  if (minutes < 60) {
    return base::UTF8ToUTF16(std::to_string(minutes) + " min");
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  if (mins == 0) {
    return base::UTF8ToUTF16(std::to_string(hours) + "h");
  }
  return base::UTF8ToUTF16(
      std::to_string(hours) + "h " + std::to_string(mins) + "m");
}

void AstraReadingProgressView::OnOpenArticle(
    const std::string& article_id) {
  if (open_callback_) {
    open_callback_.Run(article_id);
  }
}

void AstraReadingProgressView::OnRemoveArticle(
    const std::string& article_id) {
  if (remove_callback_) {
    remove_callback_.Run(article_id);
  }
}

std::u16string AstraReadingProgressView::GetWindowTitle() const {
  return u"Reading Progress";
}

void AstraReadingProgressView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (articles_read_label_) {
    articles_read_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (streak_label_) {
    streak_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
  if (count_label_) {
    count_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
}

}  // namespace astra
