// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_snooze/astra_tab_snooze_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
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

constexpr int kBubbleWidth = 340;
constexpr int kItemHeight = 76;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 4;

// Format time left until wake time.
std::u16string FormatTimeLeft(base::Time wake_at) {
  base::TimeDelta delta = wake_at - base::Time::Now();
  if (delta.is_negative() || delta.is_zero()) {
    return u"Ready now";
  }
  int hours = delta.InHours();
  int minutes = delta.InMinutes() % 60;
  int days = delta.InDays();

  if (days > 0) {
    std::string result = std::to_string(days) + "d";
    if (hours > 0) {
      result += " " + std::to_string(hours) + "h";
    }
    return base::UTF8ToUTF16(result);
  }
  if (hours > 0) {
    std::string result = std::to_string(hours) + "h";
    if (minutes > 0) {
      result += " " + std::to_string(minutes) + "m";
    }
    return base::UTF8ToUTF16(result + " left");
  }
  return base::UTF8ToUTF16(std::to_string(minutes) + " min left");
}

// Format wake time as clock time.
std::u16string FormatWakeTime(base::Time wake_at) {
  base::Time::Exploded exploded;
  wake_at.LocalExplode(&exploded);

  int hour = exploded.hour;
  int minute = exploded.minute;
  bool is_pm = hour >= 12;
  if (hour == 0) hour = 12;
  if (hour > 12) hour -= 12;

  std::string ampm = is_pm ? "PM" : "AM";
  char buf[32];
  snprintf(buf, sizeof(buf), "%d:%02d %s", hour, minute, ampm.c_str());
  return base::UTF8ToUTF16(buf);
}

}  // namespace

// ===========================================================================
// AstraSnoozedTabItemView
// ===========================================================================

AstraSnoozedTabItemView::AstraSnoozedTabItemView(
    const SnoozedTab& tab,
    WakeCallback wake_callback,
    EditCallback edit_callback,
    DismissCallback dismiss_callback)
    : tab_id_(tab.tab_id),
      title_(tab.title),
      domain_(tab.domain),
      wake_at_(tab.wake_at),
      snoozed_at_(tab.snoozed_at),
      wake_callback_(std::move(wake_callback)),
      edit_callback_(std::move(edit_callback)),
      dismiss_callback_(std::move(dismiss_callback)) {
  BuildLayout();
}

AstraSnoozedTabItemView::~AstraSnoozedTabItemView() = default;

void AstraSnoozedTabItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(8, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Top row: tab info + time left.
  auto* top_row = AddChildView(std::make_unique<views::View>());
  top_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  top_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon.
  auto* icon_label = top_row->AddChildView(
      std::make_unique<views::Label>(u"📄"));
  icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label->SetAutoColorReadabilityEnabled(false);
  icon_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Title.
  title_label_ = top_row->AddChildView(
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

  // Time left.
  time_left_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(
          u"⏰ " + FormatTimeLeft(wake_at_)));
  time_left_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  time_left_label_->SetAutoColorReadabilityEnabled(false);
  time_left_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Detail row: domain + wake time.
  detail_label_ = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          domain_ + " · wakes at " +
          base::UTF16ToUTF8(FormatWakeTime(wake_at_)))));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action buttons row.
  auto* buttons_row = AddChildView(std::make_unique<views::View>());
  buttons_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));
  buttons_row->SetMainAxisAlignment(
      views::BoxLayout::MainAxisAlignment::kEnd);

  wake_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSnoozedTabItemView::OnWakeClicked,
              base::Unretained(this)),
          u"Wake now"));

  edit_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSnoozedTabItemView::OnEditClicked,
              base::Unretained(this)),
          u"Edit"));

  dismiss_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSnoozedTabItemView::OnDismissClicked,
              base::Unretained(this)),
          u"Dismiss"));
}

void AstraSnoozedTabItemView::OnWakeClicked() {
  if (wake_callback_) {
    wake_callback_.Run(tab_id_);
  }
}

void AstraSnoozedTabItemView::OnEditClicked() {
  if (edit_callback_) {
    edit_callback_.Run(tab_id_);
  }
}

void AstraSnoozedTabItemView::OnDismissClicked() {
  if (dismiss_callback_) {
    dismiss_callback_.Run(tab_id_);
  }
}

void AstraSnoozedTabItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  time_left_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabSnoozeView
// ===========================================================================

AstraTabSnoozeView::AstraTabSnoozeView(views::View* anchor_view, Mode mode)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT),
      mode_(mode) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabSnoozeView::~AstraTabSnoozeView() = default;

void AstraTabSnoozeView::SetSnoozeTabTitle(const std::u16string& title) {
  snooze_tab_title_ = title;
  if (tab_title_label_) {
    tab_title_label_->SetText(title);
  }
}

void AstraTabSnoozeView::SetSnoozeTabDomain(const std::string& domain) {
  snooze_tab_domain_ = domain;
  if (tab_domain_label_) {
    tab_domain_label_->SetText(base::UTF8ToUTF16(domain));
  }
}

void AstraTabSnoozeView::SetSnoozePresets(
    const std::vector<SnoozePreset>& presets) {
  presets_ = presets;
  // TODO(astra): Rebuild preset grid.
}

void AstraTabSnoozeView::SetSnoozeSelectedCallback(
    SnoozeSelectedCallback callback) {
  snooze_selected_callback_ = std::move(callback);
}

void AstraTabSnoozeView::SetSnoozedTabs(
    const std::vector<AstraSnoozedTabItemView::SnoozedTab>& tabs) {
  snoozed_tabs_ = tabs;
  snoozed_count_ = static_cast<int>(tabs.size());
  BuildSnoozedTabList();
  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(
        "💤 " + std::to_string(snoozed_count_) + " tabs snoozed"));
  }
}

void AstraTabSnoozeView::SetSnoozedCount(int count) {
  snoozed_count_ = count;
  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(
        "💤 " + std::to_string(count) + " tabs snoozed"));
  }
}

void AstraTabSnoozeView::SetWakeTabCallback(WakeTabCallback callback) {
  wake_tab_callback_ = std::move(callback);
}

void AstraTabSnoozeView::SetEditSnoozeCallback(EditSnoozeCallback callback) {
  edit_snooze_callback_ = std::move(callback);
}

void AstraTabSnoozeView::SetDismissSnoozeCallback(
    DismissSnoozeCallback callback) {
  dismiss_snooze_callback_ = std::move(callback);
}

void AstraTabSnoozeView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  if (mode_ == Mode::kSnoozeTab) {
    BuildSnoozeTabUI();
  } else {
    BuildSnoozedListUI();
  }
}

void AstraTabSnoozeView::BuildSnoozeTabUI() {
  BuildTabHeader();
  BuildPresetGrid();
  BuildSnoozeButton();
}

void AstraTabSnoozeView::BuildSnoozedListUI() {
  BuildSnoozedListHeader();
  BuildSnoozedTabList();
}

void AstraTabSnoozeView::BuildTabHeader() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(16, kSectionPadding), 4));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  tab_title_label_ = section->AddChildView(
      std::make_unique<views::Label>(snooze_tab_title_));
  tab_title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_title_label_->SetAutoColorReadabilityEnabled(false);
  tab_title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));
  tab_title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  tab_domain_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16(snooze_tab_domain_)));
  tab_domain_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_domain_label_->SetAutoColorReadabilityEnabled(false);
  tab_domain_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  tab_domain_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraTabSnoozeView::BuildPresetGrid() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"When should it come back?"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  preset_grid_ = section->AddChildView(std::make_unique<views::View>());
  preset_grid_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap)
      .SetColumnGap(8)
      .SetRowGap(8);

  // Default presets.
  std::vector<SnoozePreset> default_presets = {
      {"later_today", u"Later today", "⏰", base::Hours(3)},
      {"tomorrow", u"Tomorrow", "🌅", base::Hours(24)},
      {"weekend", u"This weekend", "📅", base::Days(3)},
      {"custom", u"Custom...", "🎯", base::Minutes(0)},
  };

  for (const auto& preset : default_presets) {
    auto* button = preset_grid_->AddChildView(
        views::MdTextButton::CreateSecondaryUiButton(
            base::BindRepeating(
                &AstraTabSnoozeView::OnPresetClicked,
                base::Unretained(this),
                preset.preset_id),
            base::UTF8ToUTF16(preset.emoji + " ") + preset.label));
    button->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kPreferred,
            /*flex_weight=*/1));
    preset_buttons_.push_back(button);
  }
}

void AstraTabSnoozeView::BuildSnoozeButton() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  snooze_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabSnoozeView::OnSnoozeClicked,
              base::Unretained(this)),
          u"💤 Snooze"));
  snooze_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabSnoozeView::BuildSnoozedListHeader() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 4));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16(
              "💤 " + std::to_string(snoozed_count_) + " tabs snoozed")));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
}

void AstraTabSnoozeView::BuildSnoozedTabList() {
  if (!tab_list_) {
    // First time: create scroll view and list.
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

  tab_list_->RemoveAllChildViews();
  tab_items_.clear();

  for (const auto& tab : snoozed_tabs_) {
    auto* item = tab_list_->AddChildView(
        std::make_unique<AstraSnoozedTabItemView>(
            tab,
            base::BindRepeating(
                &AstraTabSnoozeView::OnWakeTab,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabSnoozeView::OnEditSnooze,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabSnoozeView::OnDismissSnooze,
                base::Unretained(this))));
    tab_items_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabSnoozeView::OnPresetClicked(const std::string& preset_id) {
  selected_preset_ = preset_id;

  // Update button styles.
  for (size_t i = 0; i < preset_buttons_.size(); ++i) {
    bool matches = (i < presets_.size() &&
                    presets_[i].preset_id == preset_id);
    preset_buttons_[i]->SetProminent(matches);
  }

  if (snooze_selected_callback_) {
    snooze_selected_callback_.Run(preset_id);
  }
}

void AstraTabSnoozeView::OnSnoozeClicked() {
  if (snooze_selected_callback_ && !selected_preset_.empty()) {
    snooze_selected_callback_.Run(selected_preset_);
  }
}

void AstraTabSnoozeView::OnWakeTab(const std::string& tab_id) {
  if (wake_tab_callback_) {
    wake_tab_callback_.Run(tab_id);
  }
}

void AstraTabSnoozeView::OnEditSnooze(const std::string& tab_id) {
  if (edit_snooze_callback_) {
    edit_snooze_callback_.Run(tab_id);
  }
}

void AstraTabSnoozeView::OnDismissSnooze(const std::string& tab_id) {
  if (dismiss_snooze_callback_) {
    dismiss_snooze_callback_.Run(tab_id);
  }
}

std::u16string AstraTabSnoozeView::GetWindowTitle() const {
  if (mode_ == Mode::kSnoozeTab) {
    return u"Snooze Tab";
  }
  return u"Snoozed Tabs";
}

void AstraTabSnoozeView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (tab_title_label_) {
    tab_title_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
  if (tab_domain_label_) {
    tab_domain_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (count_label_) {
    count_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
}

}  // namespace astra
