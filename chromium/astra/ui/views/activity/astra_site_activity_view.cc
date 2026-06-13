// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/activity/astra_site_activity_view.h"

#include <algorithm>
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
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kSiteRowHeight = 52;
constexpr int kCategoryRowHeight = 28;
constexpr int kSectionPadding = 16;
constexpr int kRowSpacing = 6;
constexpr int kMaxVisibleSites = 6;
constexpr int kBarHeight = 6;

// Get color for a category.
SkColor CategoryColor(const std::string& category) {
  if (category == "work") return SkColorSetRGB(91, 143, 249);   // Blue
  if (category == "social") return SkColorSetRGB(249, 115, 22);   // Orange
  if (category == "entertainment") return SkColorSetRGB(245, 63, 63); // Red
  if (category == "news") return SkColorSetRGB(97, 221, 170);    // Green
  if (category == "productivity") return SkColorSetRGB(114, 98, 253); // Purple
  if (category == "shopping") return SkColorSetRGB(236, 72, 153); // Pink
  if (category == "education") return SkColorSetRGB(251, 191, 36); // Yellow
  return SK_ColorGRAY;
}

}  // namespace

// ===========================================================================
// AstraSiteActivityRowView
// ===========================================================================

AstraSiteActivityRowView::AstraSiteActivityRowView(
    const SiteInfo& info,
    base::TimeDelta max_time)
    : domain_(info.domain),
      category_(info.category),
      time_spent_(info.time_spent),
      visit_count_(info.visit_count),
      max_time_(max_time) {
  BuildLayout();
}

AstraSiteActivityRowView::~AstraSiteActivityRowView() = default;

void AstraSiteActivityRowView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kSiteRowHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(8, 12),
      10));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Icon.
  auto* icon_label = AddChildView(
      std::make_unique<views::Label>(u"📄"));
  icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label->SetAutoColorReadabilityEnabled(false);
  icon_label->SetFontList(
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

  domain_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(domain_)));
  domain_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  domain_label_->SetAutoColorReadabilityEnabled(false);
  domain_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  domain_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  detail_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(visit_count_) + " visits · " + category_)));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Time label.
  time_label_ = AddChildView(
      std::make_unique<views::Label>(
          AstraSiteActivityView::FormatDuration(time_spent_)));
  time_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  time_label_->SetAutoColorReadabilityEnabled(false);
  time_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
}

void AstraSiteActivityRowView::Update(
    const SiteInfo& info, base::TimeDelta max_time) {
  domain_ = info.domain;
  category_ = info.category;
  time_spent_ = info.time_spent;
  visit_count_ = info.visit_count;
  max_time_ = max_time;

  if (domain_label_) {
    domain_label_->SetText(base::UTF8ToUTF16(domain_));
  }
  if (detail_label_) {
    detail_label_->SetText(base::UTF8ToUTF16(
        std::to_string(visit_count_) + " visits · " + category_));
  }
  if (time_label_) {
    time_label_->SetText(
        AstraSiteActivityView::FormatDuration(time_spent_));
  }
  SchedulePaint();
}

void AstraSiteActivityRowView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Draw time bar at the bottom.
  if (max_time_.is_positive() && time_spent_.is_positive()) {
    gfx::Rect bounds = GetContentsBounds();
    int bar_width = bounds.width() - 24;
    int bar_x = bounds.x() + 12;
    int bar_y = bounds.bottom() - kBarHeight - 3;

    double ratio = time_spent_.InSecondsF() / max_time_.InSecondsF();
    if (ratio > 1.0) ratio = 1.0;
    int filled_width = static_cast<int>(bar_width * ratio);

    // Background bar.
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetA(SK_ColorGRAY, 60));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(
        gfx::Rect(bar_x, bar_y, bar_width, kBarHeight),
        kBarHeight / 2, bg_flags);

    // Filled bar.
    cc::PaintFlags fill_flags;
    fill_flags.setColor(CategoryColor(category_));
    fill_flags.setStyle(cc::PaintFlags::kFill_Style);
    fill_flags.setAntiAlias(true);
    canvas->DrawRoundRect(
        gfx::Rect(bar_x, bar_y, filled_width, kBarHeight),
        kBarHeight / 2, fill_flags);
  }
}

void AstraSiteActivityRowView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  domain_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  time_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraSiteActivityView
// ===========================================================================

AstraSiteActivityView::AstraSiteActivityView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraSiteActivityView::~AstraSiteActivityView() = default;

void AstraSiteActivityView::SetTopSites(
    const std::vector<AstraSiteActivityRowView::SiteInfo>& sites) {
  top_sites_ = sites;
  RefreshSites();
}

void AstraSiteActivityView::SetCategories(
    const std::vector<Category>& categories) {
  categories_ = categories;
  RefreshCategories();

  // Update total.
  base::TimeDelta total;
  for (const auto& cat : categories) {
    total += cat.second;
  }
  total_time_ = total;
  RefreshTotal();
}

void AstraSiteActivityView::SetTotalTime(base::TimeDelta total) {
  total_time_ = total;
  RefreshTotal();
}

void AstraSiteActivityView::SetTimeRange(TimeRange range) {
  time_range_ = range;

  // Update button states.
  if (day_button_) {
    day_button_->SetProminent(range == TimeRange::kDay);
  }
  if (week_button_) {
    week_button_->SetProminent(range == TimeRange::kWeek);
  }
  if (month_button_) {
    month_button_->SetProminent(range == TimeRange::kMonth);
  }
}

void AstraSiteActivityView::SetTimeRangeChangedCallback(
    TimeRangeChangedCallback callback) {
  time_range_callback_ = std::move(callback);
}

void AstraSiteActivityView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildTimeRangeRow();
  BuildCategoriesSection();
  BuildSitesSection();
}

void AstraSiteActivityView::BuildTimeRangeRow() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  // Time range buttons.
  day_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSiteActivityView::OnDayClicked,
              base::Unretained(this)),
          u"Day"));
  day_button_->SetProminent(true);

  week_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSiteActivityView::OnWeekClicked,
              base::Unretained(this)),
          u"Week"));

  month_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSiteActivityView::OnMonthClicked,
              base::Unretained(this)),
          u"Month"));

  // Spacer.
  auto* spacer = section->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Total label.
  total_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Total: —"));
  total_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  total_label_->SetAutoColorReadabilityEnabled(false);
  total_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
}

void AstraSiteActivityView::BuildCategoriesSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 6));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Categories"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  categories_container_ = section->AddChildView(
      std::make_unique<views::View>());
  categories_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), 4));
}

void AstraSiteActivityView::BuildSitesSection() {
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

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Top Sites"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kSiteRowHeight * kMaxVisibleSites +
                                kRowSpacing * (kMaxVisibleSites - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  sites_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  sites_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kRowSpacing));
}

void AstraSiteActivityView::RefreshCategories() {
  if (!categories_container_) return;

  categories_container_->RemoveAllChildViews();

  // Find max time for scaling.
  base::TimeDelta max_time;
  for (const auto& cat : categories_) {
    if (cat.second > max_time) max_time = cat.second;
  }
  if (!max_time.is_positive()) max_time = base::Seconds(1);

  // Sort by time descending.
  auto sorted = categories_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              return a.second > b.second;
            });

  for (const auto& cat : sorted) {
    auto* row = categories_container_->AddChildView(
        std::make_unique<views::View>());
    row->SetPreferredSize(
        gfx::Size(kBubbleWidth - kSectionPadding * 2, kCategoryRowHeight));
    row->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        gfx::Insets(), 8));
    row->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

    // Emoji + name.
    auto* name_label = row->AddChildView(
        std::make_unique<views::Label>(
            base::UTF8ToUTF16(CategoryEmoji(cat.first) + " " + cat.first)));
    name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    name_label->SetAutoColorReadabilityEnabled(false);
    name_label->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_SECONDARY));
    name_label->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kPreferred,
            /*flex_weight=*/1));

    // Time label.
    auto* time_label = row->AddChildView(
        std::make_unique<views::Label>(FormatDuration(cat.second)));
    time_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    time_label->SetAutoColorReadabilityEnabled(false);
    time_label->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_SECONDARY));

    // Bar visualization (painted via background for simplicity).
    // We'll add a bar view.
    auto* bar_container = row->AddChildView(std::make_unique<views::View>());
    bar_container->SetPreferredSize(
        gfx::Size(120, kBarHeight));
    bar_container->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kPreferred,
            /*flex_weight=*/2));

    // Paint the bar.
    bar_container->SetPaintCallback(
        base::BindRepeating(
            [](base::TimeDelta time, base::TimeDelta max,
               const std::string& category, gfx::Canvas* canvas) {
              gfx::Rect bounds = gfx::Rect(0, 0, 120, kBarHeight);

              double ratio = time.InSecondsF() / max.InSecondsF();
              if (ratio > 1.0) ratio = 1.0;
              int filled = static_cast<int>(bounds.width() * ratio);

              // BG.
              cc::PaintFlags bg;
              bg.setColor(SkColorSetA(SK_ColorGRAY, 60));
              bg.setStyle(cc::PaintFlags::kFill_Style);
              bg.setAntiAlias(true);
              canvas->DrawRoundRect(
                  gfx::Rect(bounds.x(), bounds.y() + (bounds.height() - kBarHeight) / 2,
                            bounds.width(), kBarHeight),
                  kBarHeight / 2, bg);

              // Fill.
              cc::PaintFlags fill;
              fill.setColor(CategoryColor(category));
              fill.setStyle(cc::PaintFlags::kFill_Style);
              fill.setAntiAlias(true);
              canvas->DrawRoundRect(
                  gfx::Rect(bounds.x(), bounds.y() + (bounds.height() - kBarHeight) / 2,
                            filled, kBarHeight),
                  kBarHeight / 2, fill);
            },
            cat.second, max_time, cat.first));
  }

  InvalidateLayout();
}

void AstraSiteActivityView::RefreshSites() {
  if (!sites_list_) return;

  sites_list_->RemoveAllChildViews();
  site_rows_.clear();

  // Find max time for scaling.
  base::TimeDelta max_time;
  for (const auto& site : top_sites_) {
    if (site.time_spent > max_time) max_time = site.time_spent;
  }
  if (!max_time.is_positive()) max_time = base::Seconds(1);

  for (const auto& site : top_sites_) {
    auto* row = sites_list_->AddChildView(
        std::make_unique<AstraSiteActivityRowView>(site, max_time));
    site_rows_.push_back(row);
  }

  InvalidateLayout();
}

void AstraSiteActivityView::RefreshTotal() {
  if (total_label_) {
    total_label_->SetText(u"Total: " + FormatDuration(total_time_));
  }
}

void AstraSiteActivityView::OnDayClicked() {
  SetTimeRange(TimeRange::kDay);
  if (time_range_callback_) {
    time_range_callback_.Run(TimeRange::kDay);
  }
}

void AstraSiteActivityView::OnWeekClicked() {
  SetTimeRange(TimeRange::kWeek);
  if (time_range_callback_) {
    time_range_callback_.Run(TimeRange::kWeek);
  }
}

void AstraSiteActivityView::OnMonthClicked() {
  SetTimeRange(TimeRange::kMonth);
  if (time_range_callback_) {
    time_range_callback_.Run(TimeRange::kMonth);
  }
}

// static
std::string AstraSiteActivityView::CategoryEmoji(const std::string& category) {
  if (category == "work") return "💼";
  if (category == "social") return "💬";
  if (category == "entertainment") return "🎮";
  if (category == "news") return "📰";
  if (category == "productivity") return "⚡";
  if (category == "shopping") return "🛒";
  if (category == "education") return "📚";
  if (category == "focus") return "🎯";
  return "📄";
}

// static
std::u16string AstraSiteActivityView::FormatDuration(base::TimeDelta delta) {
  if (delta.is_zero()) return u"0m";

  int hours = delta.InHours();
  int minutes = delta.InMinutes() % 60;
  int seconds = delta.InSeconds() % 60;

  if (hours > 0) {
    std::string result = std::to_string(hours) + "h";
    if (minutes > 0) {
      result += " " + std::to_string(minutes) + "m";
    }
    return base::UTF8ToUTF16(result);
  }
  if (minutes > 0) {
    return base::UTF8ToUTF16(std::to_string(minutes) + "m");
  }
  return base::UTF8ToUTF16(std::to_string(seconds) + "s");
}

std::u16string AstraSiteActivityView::GetWindowTitle() const {
  return u"Site Activity";
}

void AstraSiteActivityView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  total_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

}  // namespace astra
