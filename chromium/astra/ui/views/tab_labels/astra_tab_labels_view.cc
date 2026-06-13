// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_labels/astra_tab_labels_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkCanvas.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPaint.h"
#include "skia/core/SkRect.h"
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
constexpr int kSectionPadding = 16;
constexpr int kLabelChipHeight = 32;
constexpr int kLabelChipSpacing = 6;
constexpr int kTabRowHeight = 48;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleTabs = 5;

}  // namespace

// ===========================================================================
// AstraTabLabelItemView
// ===========================================================================

AstraTabLabelItemView::AstraTabLabelItemView(
    const LabelInfo& info,
    ClickCallback click_callback,
    DeleteCallback delete_callback)
    : label_id_(info.label_id),
      name_(info.name),
      color_(info.color),
      tab_count_(info.tab_count),
      is_selected_(info.is_selected),
      click_callback_(std::move(click_callback)),
      delete_callback_(std::move(delete_callback)) {
  BuildLayout();
}

AstraTabLabelItemView::~AstraTabLabelItemView() = default;

void AstraTabLabelItemView::SetSelected(bool selected) {
  is_selected_ = selected;
  SchedulePaint();
}

void AstraTabLabelItemView::SetTabCount(int count) {
  tab_count_ = count;
  if (count_label_) {
    count_label_->SetText(
        base::UTF8ToUTF16("(" + std::to_string(count) + ")"));
  }
}

void AstraTabLabelItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(0, kLabelChipHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 10),
      6));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 16, SK_ColorGRAY));

  name_label_ = AddChildView(
      std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  count_label_ = AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16("(" + std::to_string(tab_count_) + ")")));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraTabLabelItemView::OnClicked() {
  if (click_callback_) {
    click_callback_.Run(label_id_);
  }
}

void AstraTabLabelItemView::OnDeleteClicked() {
  if (delete_callback_) {
    delete_callback_.Run(label_id_);
  }
}

void AstraTabLabelItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  PaintColorDot(canvas);
}

void AstraTabLabelItemView::PaintColorDot(gfx::Canvas* canvas) {
  int dot_size = 10;
  int y = (kLabelChipHeight - dot_size) / 2;
  int x = 8;

  SkPaint paint;
  paint.setColor(color_);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setAntiAlias(true);

  SkRect rect = SkRect::MakeXYWH(x, y, dot_size, dot_size);
  canvas->drawRoundRect(rect, dot_size / 2, dot_size / 2, paint);
}

void AstraTabLabelItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SkColor bg = is_selected_
                   ? cp->GetColor(ui::kColorButtonBackgroundProminent)
                   : cp->GetColor(ui::kColorDialogBackground);
  SetBackground(views::CreateRoundedRectBackground(bg, 16));

  SchedulePaint();
}

// ===========================================================================
// AstraTabLabeledTabItemView
// ===========================================================================

AstraTabLabeledTabItemView::AstraTabLabeledTabItemView(
    const TabInfo& info)
    : tab_id_(info.tab_id),
      title_(info.title),
      domain_(info.domain),
      label_ids_(info.label_ids) {
  BuildLayout();
}

AstraTabLabeledTabItemView::~AstraTabLabeledTabItemView() = default;

void AstraTabLabeledTabItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kTabRowHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(8, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  title_label_ = AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  domain_label_ = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(domain_)));
  domain_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  domain_label_->SetAutoColorReadabilityEnabled(false);
  domain_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  domain_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraTabLabeledTabItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  domain_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabLabelsView
// ===========================================================================

AstraTabLabelsView::AstraTabLabelsView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabLabelsView::~AstraTabLabelsView() = default;

void AstraTabLabelsView::SetLabels(
    const std::vector<AstraTabLabelItemView::LabelInfo>& labels) {
  labels_ = labels;
  RefreshLabels();
}

void AstraTabLabelsView::SetTabs(
    const std::vector<AstraTabLabeledTabItemView::TabInfo>& tabs) {
  tabs_ = tabs;
  RefreshTabs();
}

void AstraTabLabelsView::SetSelectedLabel(const std::string& label_id) {
  selected_label_id_ = label_id;
  RefreshLabels();
  RefreshTabs();
}

void AstraTabLabelsView::SetNewLabelCallback(
    NewLabelCallback callback) {
  new_label_callback_ = std::move(callback);
}

void AstraTabLabelsView::SetDeleteLabelCallback(
    DeleteLabelCallback callback) {
  delete_label_callback_ = std::move(callback);
}

void AstraTabLabelsView::SetLabelSelectedCallback(
    LabelSelectedCallback callback) {
  label_selected_callback_ = std::move(callback);
}

void AstraTabLabelsView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildNewLabelSection();
  BuildLabelsRow();
  BuildTabsSection();
}

void AstraTabLabelsView::BuildNewLabelSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  new_label_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabLabelsView::OnNewLabel,
              base::Unretained(this)),
          u"🏷️ New Label"));
  new_label_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabLabelsView::BuildLabelsRow() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kItemSpacing));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Labels"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  labels_row_ = section->AddChildView(std::make_unique<views::View>());
  labels_row_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  labels_row_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kPreferred));
}

void AstraTabLabelsView::BuildTabsSection() {
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

  tabs_header_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Tabs (0)"));
  tabs_header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tabs_header_label_->SetAutoColorReadabilityEnabled(false);
  tabs_header_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(kTabRowHeight * kMaxVisibleTabs +
                              kItemSpacing * (kMaxVisibleTabs - 1));
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  tabs_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  tabs_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabLabelsView::RefreshLabels() {
  if (!labels_row_) return;

  labels_row_->RemoveAllChildViews();
  label_views_.clear();

  // Sort by tab count descending.
  auto sorted = labels_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              return a.tab_count > b.tab_count;
            });

  for (const auto& label : sorted) {
    auto label_info = label;
    label_info.is_selected = (label.label_id == selected_label_id_);

    auto* chip = labels_row_->AddChildView(
        std::make_unique<AstraTabLabelItemView>(
            label_info,
            base::BindRepeating(
                &AstraTabLabelsView::OnLabelSelected,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabLabelsView::OnDeleteLabel,
                base::Unretained(this))));
    label_views_.push_back(chip);
  }

  InvalidateLayout();
}

void AstraTabLabelsView::RefreshTabs() {
  if (!tabs_list_) return;

  tabs_list_->RemoveAllChildViews();
  tab_views_.clear();

  // Filter tabs by selected label if any.
  std::vector<AstraTabLabeledTabItemView::TabInfo> filtered;
  if (selected_label_id_.empty()) {
    filtered = tabs_;
  } else {
    for (const auto& tab : tabs_) {
      for (const auto& id : tab.label_ids) {
        if (id == selected_label_id_) {
          filtered.push_back(tab);
          break;
        }
      }
    }
  }

  if (tabs_header_label_) {
    if (selected_label_id_.empty()) {
      tabs_header_label_->SetText(
          base::UTF8ToUTF16(
              "All tabs (" + std::to_string(tabs_.size()) + ")"));
    } else {
      tabs_header_label_->SetText(
          base::UTF8ToUTF16(
              "Tabs with label (" +
              std::to_string(filtered.size()) + ")"));
    }
  }

  for (const auto& tab : filtered) {
    auto* item = tabs_list_->AddChildView(
        std::make_unique<AstraTabLabeledTabItemView>(tab));
    tab_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabLabelsView::OnNewLabel() {
  if (new_label_callback_) {
    new_label_callback_.Run();
  }
}

void AstraTabLabelsView::OnLabelSelected(const std::string& label_id) {
  if (label_selected_callback_) {
    label_selected_callback_.Run(label_id);
  }
}

void AstraTabLabelsView::OnDeleteLabel(const std::string& label_id) {
  if (delete_label_callback_) {
    delete_label_callback_.Run(label_id);
  }
}

std::u16string AstraTabLabelsView::GetWindowTitle() const {
  return u"Tab Labels";
}

void AstraTabLabelsView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (tabs_header_label_) {
    tabs_header_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
}

}  // namespace astra
