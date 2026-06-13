// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_actions/astra_tab_multiselect_bar.h"

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
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBarHeight = 44;
constexpr int kBarHorizontalPadding = 16;
constexpr int kButtonSpacing = 8;

}  // namespace

// ===========================================================================
// AstraTabMultiSelectBar
// ===========================================================================

AstraTabMultiSelectBar::AstraTabMultiSelectBar() {
  BuildLayout();
}

AstraTabMultiSelectBar::~AstraTabMultiSelectBar() = default;

void AstraTabMultiSelectBar::BuildLayout() {
  SetPreferredSize(gfx::Size(0, kBarHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(0, kBarHorizontalPadding),
      kButtonSpacing));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));

  // Count label on the left.
  count_label_ = AddChildView(
      std::make_unique<views::Label>(u"0 tabs selected"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  count_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Action buttons on the right.
  pin_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              [](AstraTabMultiSelectBar* bar) {
                if (bar->pin_callback_) bar->pin_callback_.Run();
              },
              base::Unretained(this)),
          u"Pin"));

  group_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              [](AstraTabMultiSelectBar* bar) {
                if (bar->group_callback_) bar->group_callback_.Run();
              },
              base::Unretained(this)),
          u"Group"));

  bookmark_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              [](AstraTabMultiSelectBar* bar) {
                if (bar->bookmark_callback_)
                  bar->bookmark_callback_.Run();
              },
              base::Unretained(this)),
          u"Bookmark"));

  move_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              [](AstraTabMultiSelectBar* bar) {
                // TODO(astra): Show workspace picker menu.
              },
              base::Unretained(this)),
          u"Move to..."));

  close_button_ = AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              [](AstraTabMultiSelectBar* bar) {
                if (bar->close_callback_) bar->close_callback_.Run();
              },
              base::Unretained(this)),
          u"Close tabs"));
}

void AstraTabMultiSelectBar::SetSelectedTabCount(int count) {
  selected_count_ = count;
  UpdateCountLabel();
}

void AstraTabMultiSelectBar::SetVisible(bool visible) {
  views::View::SetVisible(visible);
}

void AstraTabMultiSelectBar::UpdateCountLabel() {
  if (!count_label_) return;
  count_label_->SetText(base::UTF8ToUTF16(
      std::to_string(selected_count_) + " tab" +
      (selected_count_ != 1 ? "s" : "") + " selected"));
}

// -- Callbacks -------------------------------------------------------------

void AstraTabMultiSelectBar::SetCloseCallback(ActionCallback callback) {
  close_callback_ = std::move(callback);
}

void AstraTabMultiSelectBar::SetPinCallback(ActionCallback callback) {
  pin_callback_ = std::move(callback);
}

void AstraTabMultiSelectBar::SetGroupCallback(ActionCallback callback) {
  group_callback_ = std::move(callback);
}

void AstraTabMultiSelectBar::SetBookmarkCallback(ActionCallback callback) {
  bookmark_callback_ = std::move(callback);
}

void AstraTabMultiSelectBar::SetMoveToWorkspaceCallback(
    MoveToWorkspaceCallback callback) {
  move_to_workspace_callback_ = std::move(callback);
}

void AstraTabMultiSelectBar::SetDeselectAllCallback(ActionCallback callback) {
  deselect_all_callback_ = std::move(callback);
}

// -- Button visibility -----------------------------------------------------

void AstraTabMultiSelectBar::SetPinButtonVisible(bool visible) {
  if (pin_button_) pin_button_->SetVisible(visible);
}

void AstraTabMultiSelectBar::SetGroupButtonVisible(bool visible) {
  if (group_button_) group_button_->SetVisible(visible);
}

void AstraTabMultiSelectBar::SetBookmarkButtonVisible(bool visible) {
  if (bookmark_button_) bookmark_button_->SetVisible(visible);
}

void AstraTabMultiSelectBar::SetMoveButtonVisible(bool visible) {
  if (move_button_) move_button_->SetVisible(visible);
}

// -- views::View -----------------------------------------------------------

void AstraTabMultiSelectBar::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  SetBackground(views::CreateSolidBackground(
      cp->GetColor(ui::kColorDialogBackground)));
}

gfx::Size AstraTabMultiSelectBar::CalculatePreferredSize() const {
  return gfx::Size(views::View::CalculatePreferredSize().width(), kBarHeight);
}

}  // namespace astra
