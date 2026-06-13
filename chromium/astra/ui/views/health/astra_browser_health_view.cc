// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/health/astra_browser_health_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
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
constexpr int kIssueRowHeight = 60;
constexpr int kSectionPadding = 16;
constexpr int kRowSpacing = 8;
constexpr int kMaxVisibleIssues = 4;
constexpr int kScoreDialSize = 100;
constexpr int kScoreStrokeWidth = 8;
constexpr int kProgressBarHeight = 8;

// Severity emoji.
std::u16string SeverityEmoji(
    AstraHealthIssueRowView::Severity severity) {
  switch (severity) {
    case AstraHealthIssueRowView::Severity::kInfo:
      return u"ℹ️";
    case AstraHealthIssueRowView::Severity::kWarning:
      return u"⚠️";
    case AstraHealthIssueRowView::Severity::kCritical:
      return u"🔴";
  }
  return u"ℹ️";
}

}  // namespace

// ===========================================================================
// AstraHealthIssueRowView
// ===========================================================================

AstraHealthIssueRowView::AstraHealthIssueRowView(
    const IssueInfo& info,
    ActionCallback action_callback)
    : issue_id_(info.issue_id),
      title_(info.title),
      description_(info.description),
      severity_(info.severity),
      action_label_(info.action_label),
      action_callback_(std::move(action_callback)) {
  BuildLayout();
}

AstraHealthIssueRowView::~AstraHealthIssueRowView() = default;

void AstraHealthIssueRowView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kIssueRowHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(8, 12),
      10));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Icon.
  icon_label_ = AddChildView(
      std::make_unique<views::Label>(SeverityEmoji(severity_)));
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetAutoColorReadabilityEnabled(false);
  icon_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Text column.
  auto* text_col = AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 2));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  title_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  desc_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(description_));
  desc_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc_label_->SetAutoColorReadabilityEnabled(false);
  desc_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  desc_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Action button.
  if (!action_label_.empty()) {
    action_button_ = AddChildView(
        views::MdTextButton::CreateSecondaryUiButton(
            base::BindRepeating(
                &AstraHealthIssueRowView::OnActionClicked,
                base::Unretained(this)),
            base::UTF8ToUTF16(action_label_)));
  }
}

void AstraHealthIssueRowView::OnActionClicked() {
  if (action_callback_) {
    action_callback_.Run(issue_id_);
  }
}

void AstraHealthIssueRowView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  desc_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraBrowserHealthView
// ===========================================================================

AstraBrowserHealthView::AstraBrowserHealthView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraBrowserHealthView::~AstraBrowserHealthView() = default;

void AstraBrowserHealthView::SetOverallScore(int score) {
  overall_score_ = std::clamp(score, 0, 100);
  RefreshScoreCard();
}

void AstraBrowserHealthView::SetCategoryScores(
    const std::vector<CategoryScore>& scores) {
  category_scores_ = scores;
  RefreshCategoryScores();
}

void AstraBrowserHealthView::SetIssues(
    const std::vector<AstraHealthIssueRowView::IssueInfo>& issues) {
  issues_ = issues;
  RefreshIssues();
}

void AstraBrowserHealthView::SetIssueActionCallback(
    IssueActionCallback callback) {
  issue_action_callback_ = std::move(callback);
}

void AstraBrowserHealthView::SetCleanupAllCallback(
    CleanupAllCallback callback) {
  cleanup_all_callback_ = std::move(callback);
}

void AstraBrowserHealthView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildScoreCard();
  BuildCategoryScores();
  BuildIssuesSection();
  BuildCleanupButton();
}

void AstraBrowserHealthView::BuildScoreCard() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(16, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));
  section->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Score dial (custom-drawn circular progress).
  score_dial_ = section->AddChildView(std::make_unique<views::View>());
  score_dial_->SetPreferredSize(gfx::Size(kScoreDialSize, kScoreDialSize));
  score_dial_->SetPaintCallback(
      base::BindRepeating(
          [](AstraBrowserHealthView* view, gfx::Canvas* canvas) {
            gfx::Rect bounds = view->score_dial_->GetContentsBounds();
            int cx = bounds.x() + bounds.width() / 2;
            int cy = bounds.y() + bounds.height() / 2;
            int radius = std::min(bounds.width(), bounds.height()) / 2
                         - kScoreStrokeWidth / 2;

            // Background ring.
            cc::PaintFlags bg_flags;
            bg_flags.setColor(SkColorSetA(SK_ColorGRAY, 60));
            bg_flags.setStyle(cc::PaintFlags::kStroke_Style);
            bg_flags.setStrokeWidth(kScoreStrokeWidth);
            bg_flags.setAntiAlias(true);
            canvas->DrawCircle(gfx::Point(cx, cy), radius, bg_flags);

            // Score arc (draw from top, clockwise).
            double sweep_angle = -360.0 * view->overall_score_ / 100.0;
            cc::PaintFlags arc_flags;
            arc_flags.setColor(ScoreColor(view->overall_score_));
            arc_flags.setStyle(cc::PaintFlags::kStroke_Style);
            arc_flags.setStrokeWidth(kScoreStrokeWidth);
            arc_flags.setAntiAlias(true);
            arc_flags.setStrokeCap(cc::PaintFlags::kRound_Cap);

            SkRect oval = SkRect::MakeLTRB(
                cx - radius, cy - radius,
                cx + radius, cy + radius);
            SkPath path;
            path.addArc(oval, -90, sweep_angle);
            canvas->DrawPath(path, arc_flags);
          },
          base::Unretained(this)));

  // Score number.
  score_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"100"));
  score_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  score_label_->SetAutoColorReadabilityEnabled(false);
  score_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));

  // Score description.
  score_desc_label_ = section->AddChildView(
      std::make_unique<views::Label>(ScoreLabel(overall_score_)));
  score_desc_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  score_desc_label_->SetAutoColorReadabilityEnabled(false);
  score_desc_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraBrowserHealthView::BuildCategoryScores() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  categories_container_ = section->AddChildView(
      std::make_unique<views::View>());
  categories_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), 6));
}

void AstraBrowserHealthView::BuildIssuesSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kRowSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  issues_count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Issues (0)"));
  issues_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  issues_count_label_->SetAutoColorReadabilityEnabled(false);
  issues_count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kIssueRowHeight * kMaxVisibleIssues +
                                kRowSpacing * (kMaxVisibleIssues - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  issues_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  issues_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kRowSpacing));
}

void AstraBrowserHealthView::BuildCleanupButton() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  cleanup_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraBrowserHealthView::OnCleanupAllClicked,
              base::Unretained(this)),
          u"✨ Clean up all"));
  cleanup_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraBrowserHealthView::RefreshScoreCard() {
  if (score_label_) {
    score_label_->SetText(
        base::UTF8ToUTF16(std::to_string(overall_score_)));
  }
  if (score_desc_label_) {
    score_desc_label_->SetText(ScoreLabel(overall_score_));
  }
  if (score_dial_) {
    score_dial_->SchedulePaint();
  }
}

void AstraBrowserHealthView::RefreshCategoryScores() {
  if (!categories_container_) return;

  categories_container_->RemoveAllChildViews();

  for (const auto& cat : category_scores_) {
    auto* row = categories_container_->AddChildView(
        std::make_unique<views::View>());
    row->SetPreferredSize(
        gfx::Size(kBubbleWidth - kSectionPadding * 2, 24));
    row->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        gfx::Insets(), 8));
    row->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

    // Name.
    auto* name_label = row->AddChildView(
        std::make_unique<views::Label>(
            base::UTF8ToUTF16(cat.emoji + " " + cat.name)));
    name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    name_label->SetAutoColorReadabilityEnabled(false);
    name_label->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_SECONDARY));
    name_label->SetPreferredSize(gfx::Size(100, 20));

    // Progress bar.
    auto* bar = row->AddChildView(std::make_unique<views::View>());
    bar->SetPreferredSize(gfx::Size(140, kProgressBarHeight));
    bar->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kUnbounded));

    int score = cat.score;
    bar->SetPaintCallback(
        base::BindRepeating(
            [score](gfx::Canvas* canvas) {
              gfx::Rect bounds(0, 0, 140, kProgressBarHeight);

              cc::PaintFlags bg;
              bg.setColor(SkColorSetA(SK_ColorGRAY, 60));
              bg.setStyle(cc::PaintFlags::kFill_Style);
              bg.setAntiAlias(true);
              canvas->DrawRoundRect(
                  gfx::Rect(bounds.x(), bounds.y(),
                            bounds.width(), kProgressBarHeight),
                  kProgressBarHeight / 2, bg);

              int filled = static_cast<int>(bounds.width() * score / 100);
              cc::PaintFlags fill;
              fill.setColor(ScoreColor(score));
              fill.setStyle(cc::PaintFlags::kFill_Style);
              fill.setAntiAlias(true);
              canvas->DrawRoundRect(
                  gfx::Rect(bounds.x(), bounds.y(),
                            filled, kProgressBarHeight),
                  kProgressBarHeight / 2, fill);
            }));

    // Score percentage.
    auto* score_label = row->AddChildView(
        std::make_unique<views::Label>(
            base::UTF8ToUTF16(std::to_string(score) + "%")));
    score_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    score_label->SetAutoColorReadabilityEnabled(false);
    score_label->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_SECONDARY));
    score_label->SetPreferredSize(gfx::Size(40, 20));
  }

  InvalidateLayout();
}

void AstraBrowserHealthView::RefreshIssues() {
  if (!issues_list_) return;

  issues_list_->RemoveAllChildViews();
  issue_rows_.clear();

  if (issues_count_label_) {
    issues_count_label_->SetText(base::UTF8ToUTF16(
        "Issues (" + std::to_string(issues_.size()) + ")"));
  }

  for (const auto& issue : issues_) {
    auto* row = issues_list_->AddChildView(
        std::make_unique<AstraHealthIssueRowView>(
            issue,
            base::BindRepeating(
                &AstraBrowserHealthView::OnIssueAction,
                base::Unretained(this))));
    issue_rows_.push_back(row);
  }

  InvalidateLayout();
}

void AstraBrowserHealthView::OnIssueAction(const std::string& issue_id) {
  if (issue_action_callback_) {
    issue_action_callback_.Run(issue_id);
  }
}

void AstraBrowserHealthView::OnCleanupAllClicked() {
  if (cleanup_all_callback_) {
    cleanup_all_callback_.Run();
  }
}

// static
std::u16string AstraBrowserHealthView::ScoreLabel(int score) {
  if (score >= 90) return u"Excellent";
  if (score >= 70) return u"Good";
  if (score >= 50) return u"Fair";
  if (score >= 30) return u"Needs attention";
  return u"Poor";
}

// static
SkColor AstraBrowserHealthView::ScoreColor(int score) {
  if (score >= 80) return SkColorSetRGB(97, 221, 170);   // Green
  if (score >= 50) return SkColorSetRGB(251, 191, 36);   // Yellow
  if (score >= 30) return SkColorSetRGB(249, 115, 22);   // Orange
  return SkColorSetRGB(245, 63, 63);                     // Red
}

std::u16string AstraBrowserHealthView::GetWindowTitle() const {
  return u"Browser Health";
}

void AstraBrowserHealthView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  score_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  score_desc_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  issues_count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

}  // namespace astra
