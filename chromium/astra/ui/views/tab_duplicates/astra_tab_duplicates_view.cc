// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_duplicates/astra_tab_duplicates_view.h"

#include <algorithm>
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
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/checkbox.h"
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
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kTabRowHeight = 32;
constexpr int kMaxVisibleGroups = 5;

}  // namespace

// ===========================================================================
// AstraDuplicateTabGroupView
// ===========================================================================

AstraDuplicateTabGroupView::AstraDuplicateTabGroupView(
    const GroupInfo& info,
    CloseTabCallback close_tab_callback,
    KeepTabCallback keep_tab_callback,
    CloseGroupCallback close_group_callback)
    : group_key_(info.group_key),
      label_(info.label),
      tabs_(info.tabs),
      duplicate_count_(info.duplicate_count),
      close_tab_callback_(std::move(close_tab_callback)),
      keep_tab_callback_(std::move(keep_tab_callback)),
      close_group_callback_(std::move(close_group_callback)) {
  BuildLayout();
}

AstraDuplicateTabGroupView::~AstraDuplicateTabGroupView() = default;

void AstraDuplicateTabGroupView::BuildLayout() {
  int total_height = 40 + static_cast<int>(tabs_.size()) * kTabRowHeight;
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, total_height));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12),
      6));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Header row.
  auto* header_row = AddChildView(std::make_unique<views::View>());
  header_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  header_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* icon = header_row->AddChildView(
      std::make_unique<views::Label>(u"📄"));
  icon->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  icon->SetAutoColorReadabilityEnabled(false);
  icon->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  header_label_ = header_row->AddChildView(
      std::make_unique<views::Label>(
          label_ + u" (" +
          base::UTF8ToUTF16(std::to_string(duplicate_count_)) +
          u" duplicates)"));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  header_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  header_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Tabs list.
  tabs_list_ = AddChildView(std::make_unique<views::View>());
  tabs_list_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 4));
  tabs_list_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  BuildTabRows();
}

void AstraDuplicateTabGroupView::BuildTabRows() {
  if (!tabs_list_) return;
  tabs_list_->RemoveAllChildViews();
  tab_rows_.clear();

  for (size_t i = 0; i < tabs_.size(); i++) {
    const auto& tab = tabs_[i];

    auto* row = tabs_list_->AddChildView(
        std::make_unique<views::View>());
    row->SetPreferredSize(
        gfx::Size(kBubbleWidth - kSectionPadding * 2 - 24, kTabRowHeight));
    row->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        gfx::Insets(), 6));
    row->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

    // Checkbox.
    auto* checkbox = row->AddChildView(
        std::make_unique<views::Checkbox>(
            views::Checkbox::PressedCallback(),
            std::u16string()));
    checkbox->SetChecked(!tab.is_active);  // non-active tabs checked by default

    // Title.
    auto* title = row->AddChildView(
        std::make_unique<views::Label>(tab.title));
    title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    title->SetAutoColorReadabilityEnabled(false);
    title->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            tab.is_active ? views::style::STYLE_PRIMARY
                          : views::style::STYLE_SECONDARY));
    title->SetElideBehavior(gfx::ELIDE_MIDDLE);
    title->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kUnbounded));

    // Active / pinned indicators.
    if (tab.is_active || tab.is_pinned) {
      std::u16string indicator;
      if (tab.is_active) indicator += u"● ";
      if (tab.is_pinned) indicator += u"📌";
      auto* ind_label = row->AddChildView(
          std::make_unique<views::Label>(indicator));
      ind_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
      ind_label->SetAutoColorReadabilityEnabled(false);
      ind_label->SetFontList(
          views::style::GetFont(
              views::style::CONTEXT_LABEL,
              views::style::STYLE_SECONDARY));
    }

    // Action button (keep for first, close for rest).
    if (i == 0) {
      auto* keep_btn = row->AddChildView(
          views::MdTextButton::CreateSecondaryUiButton(
              base::BindRepeating(
                  &AstraDuplicateTabGroupView::OnKeepTab,
                  base::Unretained(this),
                  tab.tab_id),
              u"Keep"));
    } else {
      auto* close_btn = row->AddChildView(
          views::MdTextButton::CreateProminent(
              base::BindRepeating(
                  &AstraDuplicateTabGroupView::OnCloseTab,
                  base::Unretained(this),
                  tab.tab_id),
              u"Close"));
    }

    tab_rows_.push_back(row);
  }
}

void AstraDuplicateTabGroupView::OnCloseTab(const std::string& tab_id) {
  if (close_tab_callback_) {
    close_tab_callback_.Run(tab_id);
  }
}

void AstraDuplicateTabGroupView::OnKeepTab(const std::string& tab_id) {
  if (keep_tab_callback_) {
    keep_tab_callback_.Run(tab_id);
  }
}

void AstraDuplicateTabGroupView::OnCloseDuplicates() {
  if (close_group_callback_) {
    close_group_callback_.Run(group_key_);
  }
}

std::u16string AstraDuplicateTabGroupView::FormatLastAccessed(
    base::Time time) {
  base::TimeDelta delta = base::Time::Now() - time;
  int minutes = delta.InMinutes();
  if (minutes < 1) return u"just now";
  if (minutes < 60) {
    return base::UTF8ToUTF16(std::to_string(minutes) + "m ago");
  }
  int hours = delta.InHours();
  if (hours < 24) {
    return base::UTF8ToUTF16(std::to_string(hours) + "h ago");
  }
  int days = delta.InDays();
  return base::UTF8ToUTF16(std::to_string(days) + "d ago");
}

void AstraDuplicateTabGroupView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  header_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabDuplicatesView
// ===========================================================================

AstraTabDuplicatesView::AstraTabDuplicatesView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabDuplicatesView::~AstraTabDuplicatesView() = default;

void AstraTabDuplicatesView::SetDuplicateGroups(
    const std::vector<AstraDuplicateTabGroupView::GroupInfo>& groups) {
  groups_ = groups;
  total_duplicate_tabs_ = 0;
  for (const auto& g : groups_) {
    total_duplicate_tabs_ += g.duplicate_count;
  }
  RefreshGroups();
  RefreshSummary();
}

void AstraTabDuplicatesView::SetMatchMode(MatchMode mode) {
  match_mode_ = mode;
  RefreshSummary();
}

void AstraTabDuplicatesView::SetCloseTabCallback(
    CloseTabCallback callback) {
  close_tab_callback_ = std::move(callback);
}

void AstraTabDuplicatesView::SetKeepTabCallback(
    KeepTabCallback callback) {
  keep_tab_callback_ = std::move(callback);
}

void AstraTabDuplicatesView::SetCloseAllDuplicatesCallback(
    CloseAllDuplicatesCallback callback) {
  close_all_callback_ = std::move(callback);
}

void AstraTabDuplicatesView::SetRefreshCallback(
    RefreshCallback callback) {
  refresh_callback_ = std::move(callback);
}

void AstraTabDuplicatesView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSummarySection();
  BuildGroupsList();
}

void AstraTabDuplicatesView::BuildSummarySection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  summary_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"Scanning for duplicates..."));
  summary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  summary_label_->SetAutoColorReadabilityEnabled(false);
  summary_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action buttons row.
  auto* buttons_row = section->AddChildView(
      std::make_unique<views::View>());
  buttons_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));

  close_all_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabDuplicatesView::OnCloseAllDuplicates,
              base::Unretained(this)),
          u"Close All Duplicates"));
  close_all_button_->SetEnabled(false);
  close_all_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  refresh_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabDuplicatesView::OnRefresh,
              base::Unretained(this)),
          u"🔄 Refresh"));
}

void AstraTabDuplicatesView::BuildGroupsList() {
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

  auto* header_label = section->AddChildView(
      std::make_unique<views::Label>(u"Duplicate Groups"));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label->SetAutoColorReadabilityEnabled(false);
  header_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(280);
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  groups_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  groups_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabDuplicatesView::RefreshGroups() {
  if (!groups_list_) return;

  groups_list_->RemoveAllChildViews();
  group_views_.clear();

  // Sort by duplicate count descending (most duplicates first).
  auto sorted = groups_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              return a.duplicate_count > b.duplicate_count;
            });

  for (const auto& group : sorted) {
    auto* item = groups_list_->AddChildView(
        std::make_unique<AstraDuplicateTabGroupView>(
            group,
            base::BindRepeating(
                &AstraTabDuplicatesView::OnCloseTab,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabDuplicatesView::OnKeepTab,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabDuplicatesView::OnCloseAllDuplicates)));
    group_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabDuplicatesView::RefreshSummary() {
  if (!summary_label_) return;

  if (groups_.empty()) {
    summary_label_->SetText(u"No duplicate tabs found ✓");
  } else {
    summary_label_->SetText(
        base::UTF8ToUTF16(
            "Found " + std::to_string(groups_.size()) +
            " duplicate groups (" +
            std::to_string(total_duplicate_tabs_) +
            " tabs) · " +
            base::UTF16ToUTF8(MatchModeLabel(match_mode_))));
  }

  if (close_all_button_) {
    close_all_button_->SetEnabled(total_duplicate_tabs_ > 0);
  }
}

void AstraTabDuplicatesView::OnCloseTab(const std::string& tab_id) {
  if (close_tab_callback_) {
    close_tab_callback_.Run(tab_id);
  }
}

void AstraTabDuplicatesView::OnKeepTab(const std::string& tab_id) {
  if (keep_tab_callback_) {
    keep_tab_callback_.Run(tab_id);
  }
}

void AstraTabDuplicatesView::OnCloseAllDuplicates() {
  if (close_all_callback_) {
    close_all_callback_.Run();
  }
}

void AstraTabDuplicatesView::OnRefresh() {
  if (refresh_callback_) {
    refresh_callback_.Run();
  }
}

std::u16string AstraTabDuplicatesView::MatchModeLabel(MatchMode mode) {
  switch (mode) {
    case MatchMode::kExactUrl:
      return u"exact URL";
    case MatchMode::kSameDomain:
      return u"same domain";
    case MatchMode::kSameHost:
      return u"same host";
  }
  return u"unknown";
}

std::u16string AstraTabDuplicatesView::GetWindowTitle() const {
  return u"Duplicate Tabs";
}

void AstraTabDuplicatesView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (summary_label_) {
    summary_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

}  // namespace astra
