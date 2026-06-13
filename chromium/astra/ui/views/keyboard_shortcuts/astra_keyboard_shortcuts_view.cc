// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/keyboard_shortcuts/astra_keyboard_shortcuts_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <map>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 380;
constexpr int kItemHeight = 32;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 2;
constexpr int kCategorySpacing = 4;
constexpr int kMaxVisibleItems = 12;

}  // namespace

// ===========================================================================
// AstraShortcutItemView
// ===========================================================================

AstraShortcutItemView::AstraShortcutItemView(const ShortcutInfo& info)
    : shortcut_id_(info.shortcut_id),
      description_(info.description),
      shortcut_(info.shortcut),
      category_(info.category),
      is_astra_(info.is_astra) {
  BuildLayout();
}

AstraShortcutItemView::~AstraShortcutItemView() = default;

void AstraShortcutItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 4),
      8));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Description.
  desc_label_ = AddChildView(
      std::make_unique<views::Label>(description_));
  desc_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc_label_->SetAutoColorReadabilityEnabled(false);
  desc_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  desc_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  desc_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Astra badge (if applicable).
  if (is_astra_) {
    auto* badge = AddChildView(
        std::make_unique<views::Label>(u" Astra "));
    badge->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    badge->SetAutoColorReadabilityEnabled(false);
    badge->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_SECONDARY));
    badge->SetBackground(views::CreateRoundedRectBackground(
        SkColorSetRGB(0x1A, 0x73, 0xE8), 4));
  }

  // Shortcut key.
  shortcut_label_ = AddChildView(
      std::make_unique<views::Label>(shortcut_));
  shortcut_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  shortcut_label_->SetAutoColorReadabilityEnabled(false);
  shortcut_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  shortcut_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraShortcutItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  desc_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  shortcut_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

// ===========================================================================
// AstraKeyboardShortcutsView
// ===========================================================================

AstraKeyboardShortcutsView::AstraKeyboardShortcutsView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraKeyboardShortcutsView::~AstraKeyboardShortcutsView() = default;

void AstraKeyboardShortcutsView::SetShortcuts(
    const std::vector<AstraShortcutItemView::ShortcutInfo>& shortcuts) {
  shortcuts_ = shortcuts;
  RefreshShortcuts();
}

void AstraKeyboardShortcutsView::SetSearchCallback(
    SearchCallback callback) {
  search_callback_ = std::move(callback);
}

void AstraKeyboardShortcutsView::SetShortcutCallback(
    ShortcutCallback callback) {
  shortcut_callback_ = std::move(callback);
}

void AstraKeyboardShortcutsView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSearchBar();
  BuildShortcutsList();
}

void AstraKeyboardShortcutsView::BuildSearchBar() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  search_field_ = section->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"🔍 Search shortcuts...");
  search_field_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraKeyboardShortcutsView::BuildShortcutsList() {
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

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(
      kItemHeight * kMaxVisibleItems + kCategorySpacing * 6);
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  shortcuts_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  shortcuts_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraKeyboardShortcutsView::RefreshShortcuts() {
  if (!shortcuts_list_) return;

  shortcuts_list_->RemoveAllChildViews();
  shortcut_views_.clear();

  // Group by category.
  std::vector<std::u16string> category_order;
  std::map<std::u16string,
           std::vector<AstraShortcutItemView::ShortcutInfo>>
      grouped;

  for (const auto& s : shortcuts_) {
    auto it = grouped.find(s.category);
    if (it == grouped.end()) {
      category_order.push_back(s.category);
    }
    grouped[s.category].push_back(s);
  }

  for (const auto& category : category_order) {
    // Category header.
    auto* header = shortcuts_list_->AddChildView(
        std::make_unique<views::Label>(category));
    header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    header->SetAutoColorReadabilityEnabled(false);
    header->SetFontList(
        views::style::GetFont(
            views::style::CONTEXT_LABEL,
            views::style::STYLE_PRIMARY));
    header->SetBorder(views::CreateSolidSidedBorder(
        0, 0, 1, 0, SkColorSetA(SK_ColorGRAY, 0x30)));
    header->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToZero,
            views::MaximumFlexSizeRule::kUnbounded));

    // Add a small spacer.
    auto* spacer = shortcuts_list_->AddChildView(
        std::make_unique<views::View>());
    spacer->SetPreferredSize(gfx::Size(0, kCategorySpacing - kItemSpacing));

    // Shortcuts in this category.
    for (const auto& shortcut : grouped[category]) {
      auto* item = shortcuts_list_->AddChildView(
          std::make_unique<AstraShortcutItemView>(shortcut));
      shortcut_views_.push_back(item);
    }
  }

  InvalidateLayout();
}

void AstraKeyboardShortcutsView::OnSearchTextChanged() {
  if (search_callback_ && search_field_) {
    search_callback_.Run(search_field_->GetText());
  }
}

std::u16string AstraKeyboardShortcutsView::GetWindowTitle() const {
  return u"Keyboard Shortcuts";
}

void AstraKeyboardShortcutsView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  // Sub-views handle their own theme changes.
}

}  // namespace astra
