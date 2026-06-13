// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_ACTIVITY_ASTRA_SITE_ACTIVITY_VIEW_H_
#define ASTRA_UI_VIEWS_ACTIVITY_ASTRA_SITE_ACTIVITY_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSiteActivityRowView — single site activity row
// =========================================================================
//
// A row showing domain, time spent, and a horizontal bar chart.
//
// Layout:
//   +-------------------------------------------+
//   |  📄 example.com              2h 30m ████ |
//   |     15 visits · category: Work           |
//   +-------------------------------------------+
// =========================================================================

class AstraSiteActivityRowView : public views::View {
 public:
  struct SiteInfo {
    std::string domain;
    std::string category;  // "work", "social", "entertainment", "news", etc.
    base::TimeDelta time_spent;
    int visit_count = 0;
  };

  AstraSiteActivityRowView(const SiteInfo& info,
                           base::TimeDelta max_time);
  ~AstraSiteActivityRowView() override;

  AstraSiteActivityRowView(const AstraSiteActivityRowView&) = delete;
  AstraSiteActivityRowView& operator=(
      const AstraSiteActivityRowView&) = delete;

  const std::string& domain() const { return domain_; }

  void Update(const SiteInfo& info, base::TimeDelta max_time);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  void BuildLayout();

  std::string domain_;
  std::string category_;
  base::TimeDelta time_spent_;
  int visit_count_ = 0;
  base::TimeDelta max_time_;

  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> time_label_ = nullptr;
};

// =========================================================================
// AstraSiteActivityView — site activity dashboard
// =========================================================================
//
// A bubble / side panel showing time spent on different websites,
// with category breakdown and time period selection.
//
// Layout:
//   +-------------------------------------------+
//   |  Site Activity                [Close]   |
//   +-------------------------------------------+
//   |  [Day] [Week] [Month]  Total: 6h 12m    |
//   +-------------------------------------------+
//   |  Categories                               |
//   |  💼 Work        3h 45m  ████████████▌   |
//   |  🎯 Focus       1h 30m  █████▌           |
//   |  📰 News          45m   ██▌              |
//   |  🎮 Entertainment  12m   ▌               |
//   +-------------------------------------------+
//   |  Top Sites                                |
//   |  ┌─────────────────────────────────────┐ |
//   |  │ 📄 docs.google.com  1h 20m  ██████ │ |
//   |  │    8 visits · Work                  │ |
//   |  └─────────────────────────────────────┘ |
//   |  ┌─────────────────────────────────────┐ |
//   │ │ 📄 github.com        45m   ███▌     │ |
//   |  │    12 visits · Work                 │ |
//   |  └─────────────────────────────────────┘ |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Activity data comes from Astra's
// activity tracking service, which samples from Chromium's tab
// activation state.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
//   - Custom Skia drawing for time bars
// =========================================================================

class AstraSiteActivityView : public views::BubbleDialogDelegateView {
 public:
  enum class TimeRange { kDay, kWeek, kMonth };

  using TimeRangeChangedCallback =
      base::RepeatingCallback<void(TimeRange range)>;
  using Category = std::pair<std::string, base::TimeDelta>;

  explicit AstraSiteActivityView(views::View* anchor_view);
  ~AstraSiteActivityView() override;

  AstraSiteActivityView(const AstraSiteActivityView&) = delete;
  AstraSiteActivityView& operator=(const AstraSiteActivityView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetTopSites(
      const std::vector<AstraSiteActivityRowView::SiteInfo>& sites);

  void SetCategories(const std::vector<Category>& categories);

  void SetTotalTime(base::TimeDelta total);

  void SetTimeRange(TimeRange range);

  // -- Callbacks -----------------------------------------------------------

  void SetTimeRangeChangedCallback(TimeRangeChangedCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  struct CategoryRow {
    std::string name;
    std::string emoji;
    base::TimeDelta time;
  };

  void BuildUI();
  void BuildTimeRangeRow();
  void BuildCategoriesSection();
  void BuildSitesSection();

  void RefreshCategories();
  void RefreshSites();
  void RefreshTotal();

  void OnDayClicked();
  void OnWeekClicked();
  void OnMonthClicked();

  static std::string CategoryEmoji(const std::string& category);
  static std::u16string FormatDuration(base::TimeDelta delta);

  // Data.
  std::vector<AstraSiteActivityRowView::SiteInfo> top_sites_;
  std::vector<Category> categories_;
  base::TimeDelta total_time_;
  TimeRange time_range_ = TimeRange::kDay;

  // Callbacks.
  TimeRangeChangedCallback time_range_callback_;

  // Child views.
  raw_ptr<views::MdTextButton> day_button_ = nullptr;
  raw_ptr<views::MdTextButton> week_button_ = nullptr;
  raw_ptr<views::MdTextButton> month_button_ = nullptr;
  raw_ptr<views::Label> total_label_ = nullptr;
  raw_ptr<views::View> categories_container_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> sites_list_ = nullptr;

  // Site rows (owned by sites_list_).
  std::vector<raw_ptr<AstraSiteActivityRowView>> site_rows_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_ACTIVITY_ASTRA_SITE_ACTIVITY_VIEW_H_
