// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_sleep/astra_tab_sleep_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
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
constexpr int kItemHeight = 68;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 5;

// Format relative time string (e.g. "3h ago", "2d ago").
std::u16string FormatRelativeTime(base::Time time) {
  if (time.is_null()) return u"—";

  base::TimeDelta delta = base::Time::Now() - time;
  int days = delta.InDays();

  if (days == 0) {
    int hours = delta.InHours();
    if (hours == 0) {
      int minutes = delta.InMinutes();
      if (minutes < 1) return u"just now";
      return base::UTF8ToUTF16(
          std::to_string(minutes) + " min ago");
    }
    return base::UTF8ToUTF16(
        std::to_string(hours) + "h ago");
  }
  if (days == 1) return u"yesterday";
  if (days < 7) {
    return base::UTF8ToUTF16(
        std::to_string(days) + "d ago");
  }
  int weeks = days / 7;
  if (weeks == 1) return u"1w ago";
  return base::UTF8ToUTF16(
      std::to_string(weeks) + "w ago");
}

// Format memory size (e.g. "15 MB", "1.2 GB").
std::u16string FormatMemoryBytes(int64_t bytes) {
  if (bytes < 1024) return u"0 KB";
  if (bytes < 1024 * 1024) {
    return base::UTF8ToUTF16(
        std::to_string(bytes / 1024) + " KB");
  }
  if (bytes < 1024LL * 1024 * 1024) {
    int mb = bytes / (1024 * 1024);
    return base::UTF8ToUTF16(std::to_string(mb) + " MB");
  }
  int gb = bytes / (1024 * 1024 * 1024);
  return base::UTF8ToUTF16(std::to_string(gb) + " GB");
}

}  // namespace

// ===========================================================================
// AstraSleepingTabItemView
// ===========================================================================

AstraSleepingTabItemView::AstraSleepingTabItemView(
    const TabInfo& tab_info,
    WakeCallback wake_callback,
    CloseCallback close_callback)
    : tab_id_(tab_info.tab_id),
      title_(tab_info.title),
      domain_(tab_info.domain),
      sleep_time_(tab_info.sleep_time),
      memory_saved_bytes_(tab_info.memory_saved_bytes),
      wake_callback_(std::move(wake_callback)),
      close_callback_(std::move(close_callback)) {
  BuildLayout();
}

AstraSleepingTabItemView::~AstraSleepingTabItemView() = default;

void AstraSleepingTabItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(10, 12),
      12));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Favicon placeholder.
  favicon_label_ = AddChildView(
      std::make_unique<views::Label>(u"📄"));
  favicon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  favicon_label_->SetAutoColorReadabilityEnabled(false);
  favicon_label_->SetFontList(
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

  detail_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          domain_ + " · slept " +
          base::UTF16ToUTF8(FormatRelativeTime(sleep_time_)))));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  memory_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(u"💾 " +
          FormatMemoryBytes(memory_saved_bytes_) + u" saved"));
  memory_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  memory_label_->SetAutoColorReadabilityEnabled(false);
  memory_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Wake button.
  wake_button_ = AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSleepingTabItemView::OnWakeClicked,
              base::Unretained(this)),
          u"Wake up"));
}

void AstraSleepingTabItemView::OnWakeClicked() {
  if (wake_callback_) {
    wake_callback_.Run(tab_id_);
  }
}

void AstraSleepingTabItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  memory_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabSleepView
// ===========================================================================

AstraTabSleepView::AstraTabSleepView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabSleepView::~AstraTabSleepView() = default;

void AstraTabSleepView::SetSleepingTabs(
    const std::vector<AstraSleepingTabItemView::TabInfo>& tabs) {
  sleeping_tabs_ = tabs;
  sleeping_tab_count_ = static_cast<int>(tabs.size());
  RefreshTabList();
  RefreshStats();
}

void AstraTabSleepView::SetAutoSleepEnabled(bool enabled) {
  auto_sleep_enabled_ = enabled;
}

void AstraTabSleepView::SetAutoSleepMinutes(int minutes) {
  auto_sleep_minutes_ = minutes;
}

void AstraTabSleepView::SetShowSleepIndicator(bool show) {
  show_sleep_indicator_ = show;
}

void AstraTabSleepView::SetTotalMemorySaved(int64_t bytes) {
  total_memory_saved_ = bytes;
  RefreshStats();
}

void AstraTabSleepView::SetSleepingTabCount(int count) {
  sleeping_tab_count_ = count;
  if (count_label_) {
    count_label_->SetText(u"Sleeping (" +
        base::UTF8ToUTF16(std::to_string(count)) + u")");
  }
}

void AstraTabSleepView::SetWakeTabCallback(WakeTabCallback callback) {
  wake_tab_callback_ = std::move(callback);
}

void AstraTabSleepView::SetCloseTabCallback(CloseTabCallback callback) {
  close_tab_callback_ = std::move(callback);
}

void AstraTabSleepView::SetWakeAllCallback(WakeAllCallback callback) {
  wake_all_callback_ = std::move(callback);
}

void AstraTabSleepView::SetSleepAllInactiveCallback(
    SleepAllInactiveCallback callback) {
  sleep_all_callback_ = std::move(callback);
}

void AstraTabSleepView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildInfoSection();
  BuildStatsSection();
  BuildTabList();
  BuildSettingsSection();
  BuildActionButtons();
}

void AstraTabSleepView::BuildInfoSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 6));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  info_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"💤 Sleeping tabs free up memory by "
          u"unloading inactive tabs."));
  info_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  info_label_->SetAutoColorReadabilityEnabled(false);
  info_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  info_label_->SetMultiLine(true);
}

void AstraTabSleepView::BuildStatsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 16));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));
  section->SetMainAxisAlignment(
      views::BoxLayout::MainAxisAlignment::kCenter);

  stats_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"💾 0 MB saved"));
  stats_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_label_->SetAutoColorReadabilityEnabled(false);
  stats_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Sleeping (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraTabSleepView::BuildTabList() {
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

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kItemHeight * kMaxVisibleItems +
                                kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  tab_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  tab_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabSleepView::BuildSettingsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"⚙ Settings"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* auto_sleep_row = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraTabSleepView::OnAutoSleepToggled,
              base::Unretained(this)),
          u"Auto-sleep after " +
              base::UTF8ToUTF16(
                  std::to_string(auto_sleep_minutes_)) +
              u" minutes"));
  auto_sleep_row->SetChecked(auto_sleep_enabled_);

  auto* indicator_row = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraTabSleepView::OnShowIndicatorToggled,
              base::Unretained(this)),
          u"Show sleep indicator in tab strip"));
  indicator_row->SetChecked(show_sleep_indicator_);
}

void AstraTabSleepView::BuildActionButtons() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetMainAxisAlignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween));

  wake_all_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabSleepView::OnWakeAllClicked,
              base::Unretained(this)),
          u"Wake all"));

  sleep_all_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabSleepView::OnSleepAllClicked,
              base::Unretained(this)),
          u"Sleep all inactive"));
}

void AstraTabSleepView::RefreshTabList() {
  if (!tab_list_) return;

  tab_list_->RemoveAllChildViews();
  tab_items_.clear();

  if (count_label_) {
    count_label_->SetText(u"Sleeping (" +
        base::UTF8ToUTF16(
            std::to_string(sleeping_tab_count_)) +
        u")");
  }

  for (const auto& tab : sleeping_tabs_) {
    auto* item = tab_list_->AddChildView(
        std::make_unique<AstraSleepingTabItemView>(
            tab,
            base::BindRepeating(
                &AstraTabSleepView::OnWakeTab,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabSleepView::OnCloseTab,
                base::Unretained(this))));
    tab_items_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabSleepView::RefreshStats() {
  if (stats_label_) {
    stats_label_->SetText(
        u"💾 " + FormatMemoryBytes(total_memory_saved_) + u" saved");
  }
  if (count_label_) {
    count_label_->SetText(u"Sleeping (" +
        base::UTF8ToUTF16(
            std::to_string(sleeping_tab_count_)) +
        u")");
  }
}

void AstraTabSleepView::OnWakeTab(const std::string& tab_id) {
  if (wake_tab_callback_) {
    wake_tab_callback_.Run(tab_id);
  }
}

void AstraTabSleepView::OnCloseTab(const std::string& tab_id) {
  if (close_tab_callback_) {
    close_tab_callback_.Run(tab_id);
  }
}

void AstraTabSleepView::OnWakeAllClicked() {
  if (wake_all_callback_) {
    wake_all_callback_.Run();
  }
}

void AstraTabSleepView::OnSleepAllClicked() {
  if (sleep_all_callback_) {
    sleep_all_callback_.Run();
  }
}

void AstraTabSleepView::OnAutoSleepToggled() {
  auto_sleep_enabled_ = !auto_sleep_enabled_;
  // TODO(astra): Persist auto-sleep setting.
}

void AstraTabSleepView::OnShowIndicatorToggled() {
  show_sleep_indicator_ = !show_sleep_indicator_;
  // TODO(astra): Persist sleep indicator setting.
}

std::u16string AstraTabSleepView::GetWindowTitle() const {
  return u"Sleeping Tabs";
}

void AstraTabSleepView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  info_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  stats_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

}  // namespace astra
