// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/focus_mode/astra_focus_session_history_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
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
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kHistoryWidth = 360;
constexpr int kSectionPadding = 16;
constexpr int kCardSpacing = 8;
constexpr int kStatCardSpacing = 12;

std::u16string FormatDurationLong(base::TimeDelta duration) {
  int hours = duration.InHours();
  int minutes = duration.InMinutes() % 60;

  if (hours > 0) {
    return base::UTF8ToUTF16(
        std::to_string(hours) + "h " + std::to_string(minutes) + "m");
  }
  return base::UTF8ToUTF16(std::to_string(minutes) + "m");
}

std::u16string FormatDate(base::Time date) {
  base::Time::Exploded exploded;
  date.LocalExplode(&exploded);

  base::Time now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.LocalExplode(&now_exploded);

  bool is_today = (exploded.year == now_exploded.year &&
                   exploded.month == now_exploded.month &&
                   exploded.day_of_month == now_exploded.day_of_month);

  if (is_today) return u"Today";

  base::Time yesterday = now - base::Days(1);
  base::Time::Exploded y_exploded;
  yesterday.LocalExplode(&y_exploded);
  bool is_yesterday = (exploded.year == y_exploded.year &&
                       exploded.month == y_exploded.month &&
                       exploded.day_of_month == y_exploded.day_of_month);

  if (is_yesterday) return u"Yesterday";

  static const char* kMonthNames[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  std::string date_str = std::string(kMonthNames[exploded.month - 1]) + " " +
                      std::to_string(exploded.day_of_month);
  return base::UTF8ToUTF16(date_str);
}

}  // namespace

// ===========================================================================
// AstraFocusSessionRowView
// ===========================================================================

AstraFocusSessionRowView::AstraFocusSessionRowView(
    const AstraFocusSession& session)
    : session_(session) {
  BuildLayout();
}

AstraFocusSessionRowView::~AstraFocusSessionRowView() = default;

void AstraFocusSessionRowView::BuildLayout() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12), 4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));
  SetPreferredSize(gfx::Size(kHistoryWidth - kSectionPadding * 2, 56));

  // Top row: date + duration + status.
  auto* top_row = AddChildView(std::make_unique<views::View>());
  top_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  top_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  date_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(FormatDate(session_.start_time)));
  date_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  date_label_->SetAutoColorReadabilityEnabled(false);
  date_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  date_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  duration_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(FormatDurationLong(session_.duration)));
  duration_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  duration_label_->SetAutoColorReadabilityEnabled(false);
  duration_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  status_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(
          session_.is_completed ? u"✅" : u"⏹"));
  status_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  status_label_->SetAutoColorReadabilityEnabled(false);

  // Bottom row: details.
  details_label_ = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(session_.phase_work_count) + " work phases · " +
          (session_.is_pomodoro ? "Pomodoro · " : "") +
          std::to_string(session_.distraction_count) + " blocked")));
  details_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  details_label_->SetAutoColorReadabilityEnabled(false);
  details_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraFocusSessionRowView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  date_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  duration_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  details_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraFocusSessionHistoryView
// ===========================================================================

AstraFocusSessionHistoryView::AstraFocusSessionHistoryView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kHistoryWidth);

  BuildUI();
}

AstraFocusSessionHistoryView::~AstraFocusSessionHistoryView() = default;

void AstraFocusSessionHistoryView::SetSessions(
    const std::vector<AstraFocusSession>& sessions) {
  sessions_ = sessions;
  RefreshSessionList();

  // Recompute stats from sessions.
  weekly_stats_ = CalculateWeeklyStats(sessions_);
  total_stats_ = CalculateTotalStats(sessions_);
  RefreshStats();
}

void AstraFocusSessionHistoryView::SetWeeklyStats(
    const AstraFocusStats& stats) {
  weekly_stats_ = stats;
  RefreshStats();
}

void AstraFocusSessionHistoryView::SetTotalStats(
    const AstraFocusStats& stats) {
  total_stats_ = stats;
  RefreshStats();
}

void AstraFocusSessionHistoryView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildWeeklyStats();
  BuildTotalStats();
  BuildSessionList();
}

void AstraFocusSessionHistoryView::BuildWeeklyStats() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kCardSpacing));

  auto* title = section->AddChildView(
      std::make_unique<views::Label>(u"This Week"));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Stat cards row.
  auto* cards_row = section->AddChildView(
      std::make_unique<views::View>());
  cards_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kScaleToZero,
                      views::MaximumFlexSizeRule::kUnbounded))
      .SetInteriorMargin(gfx::Insets());
  cards_row->SetPreferredSize(gfx::Size(kHistoryWidth - kSectionPadding * 2, 72));

  // Focus time card.
  weekly_focus_card_ = cards_row->AddChildView(
      std::make_unique<views::View>());
  weekly_focus_card_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(8, 10), 2));
  weekly_focus_card_->SetBorder(
      views::CreateRoundedRectBorder(1, 6, SK_ColorGRAY));
  weekly_focus_card_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  weekly_focus_value_ = weekly_focus_card_->AddChildView(
      std::make_unique<views::Label>(u"0m"));
  weekly_focus_value_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  weekly_focus_value_->SetAutoColorReadabilityEnabled(false);
  weekly_focus_value_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));

  auto* focus_label = weekly_focus_card_->AddChildView(
      std::make_unique<views::Label>(u"Focus time"));
  focus_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  focus_label->SetAutoColorReadabilityEnabled(false);
  focus_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Sessions card.
  weekly_sessions_card_ = cards_row->AddChildView(
      std::make_unique<views::View>());
  weekly_sessions_card_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(8, 10), 2));
  weekly_sessions_card_->SetBorder(
      views::CreateRoundedRectBorder(1, 6, SK_ColorGRAY));
  weekly_sessions_card_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  weekly_sessions_value_ = weekly_sessions_card_->AddChildView(
      std::make_unique<views::Label>(u"0"));
  weekly_sessions_value_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  weekly_sessions_value_->SetAutoColorReadabilityEnabled(false);
  weekly_sessions_value_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));

  auto* sessions_label = weekly_sessions_card_->AddChildView(
      std::make_unique<views::Label>(u"Sessions"));
  sessions_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  sessions_label->SetAutoColorReadabilityEnabled(false);
  sessions_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Streak card.
  weekly_streak_card_ = cards_row->AddChildView(
      std::make_unique<views::View>());
  weekly_streak_card_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(8, 10), 2));
  weekly_streak_card_->SetBorder(
      views::CreateRoundedRectBorder(1, 6, SK_ColorGRAY));
  weekly_streak_card_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  weekly_streak_value_ = weekly_streak_card_->AddChildView(
      std::make_unique<views::Label>(u"0d"));
  weekly_streak_value_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  weekly_streak_value_->SetAutoColorReadabilityEnabled(false);
  weekly_streak_value_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));

  auto* streak_label = weekly_streak_card_->AddChildView(
      std::make_unique<views::Label>(u"Streak"));
  streak_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  streak_label->SetAutoColorReadabilityEnabled(false);
  streak_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraFocusSessionHistoryView::BuildTotalStats() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kStatCardSpacing));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  auto* title = section->AddChildView(
      std::make_unique<views::Label>(u"All Time"));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  total_focus_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Total focus: 0m"));
  total_focus_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  total_focus_label_->SetAutoColorReadabilityEnabled(false);
  total_focus_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  total_sessions_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Sessions: 0"));
  total_sessions_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  total_sessions_label_->SetAutoColorReadabilityEnabled(false);
  total_sessions_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  total_distractions_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Distractions blocked: 0"));
  total_distractions_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  total_distractions_label_->SetAutoColorReadabilityEnabled(false);
  total_distractions_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  total_pomodoro_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Pomodoro cycles: 0"));
  total_pomodoro_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  total_pomodoro_label_->SetAutoColorReadabilityEnabled(false);
  total_pomodoro_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraFocusSessionHistoryView::BuildSessionList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, 0), kCardSpacing));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* title_row = section->AddChildView(
      std::make_unique<views::View>());
  title_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kSectionPadding), 0));

  auto* title = title_row->AddChildView(
      std::make_unique<views::Label>(u"Recent Sessions"));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  session_scroll_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  session_scroll_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  session_scroll_->SetClipHeight(200);

  session_list_ = session_scroll_->SetContents(
      std::make_unique<views::View>());
  session_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kSectionPadding), kCardSpacing));
}

void AstraFocusSessionHistoryView::RefreshStats() {
  if (weekly_focus_value_) {
    weekly_focus_value_->SetText(
        FormatDurationLong(weekly_stats_.focus_time_this_week));
  }
  if (weekly_sessions_value_) {
    weekly_sessions_value_->SetText(
        base::UTF8ToUTF16(
            std::to_string(weekly_stats_.sessions_this_week)));
  }
  if (weekly_streak_value_) {
    weekly_streak_value_->SetText(
        base::UTF8ToUTF16(
            std::to_string(weekly_stats_.current_streak_days) + "d"));
  }

  if (total_focus_label_) {
    total_focus_label_->SetText(u"Total focus: " +
        FormatDurationLong(total_stats_.total_focus_time));
  }
  if (total_sessions_label_) {
    total_sessions_label_->SetText(u"Sessions: " +
        base::UTF8ToUTF16(
            std::to_string(total_stats_.total_sessions)));
  }
  if (total_distractions_label_) {
    total_distractions_label_->SetText(u"Distractions blocked: " +
        base::UTF8ToUTF16(
            std::to_string(total_stats_.total_distractions_blocked)));
  }
  if (total_pomodoro_label_) {
    total_pomodoro_label_->SetText(u"Pomodoro cycles: " +
        base::UTF8ToUTF16(
            std::to_string(total_stats_.pomodoro_cycles_completed)));
  }
}

void AstraFocusSessionHistoryView::RefreshSessionList() {
  if (!session_list_) return;

  session_list_->RemoveAllChildViews();

  // Sort sessions by start time, newest first.
  std::vector<AstraFocusSession> sorted = sessions_;
  std::sort(sorted.begin(), sorted.end(),
             [](const AstraFocusSession& a, const AstraFocusSession& b) {
               return a.start_time > b.start_time;
             });

  // Show up to 20 most recent.
  size_t show_count = std::min(sorted.size(), size_t{20});
  for (size_t i = 0; i < show_count; i++) {
    session_list_->AddChildView(
        std::make_unique<AstraFocusSessionRowView>(sorted[i]));
  }

  InvalidateLayout();
}

std::u16string AstraFocusSessionHistoryView::FormatDuration(
    base::TimeDelta duration) const {
  return FormatDurationLong(duration);
}

std::u16string AstraFocusSessionHistoryView::GetWindowTitle() const {
  return u"Focus History";
}

void AstraFocusSessionHistoryView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

}  // namespace astra
