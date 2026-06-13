// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_profile_menu_footer_view.h"

#include <algorithm>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kFooterRowHeight = 36;
constexpr int kFooterHorizontalPadding = 16;
constexpr int kFooterButtonSpacing = 8;
constexpr int kSeparatorTopPadding = 8;
constexpr int kManageWorkspacesPadding = 8;

// Text styles.
constexpr int kFooterFontSizeDelta = 0;
constexpr gfx::Font::Weight kFooterFontWeight = gfx::Font::Weight::NORMAL;

}  // namespace

// ---------------------------------------------------------------------------
// AstraProfileMenuFooterView
// ---------------------------------------------------------------------------

AstraProfileMenuFooterView::AstraProfileMenuFooterView(Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);

  // Vertical layout: separator | manage workspaces | button row.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // --- Separator ---
  separator_ = AddChildView(std::make_unique<views::Separator>());
  separator_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSeparatorTopPadding, 0)));

  // --- "Manage workspaces" link (full-width, left-aligned) ---
  manage_workspaces_button_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuFooterView::OnManageWorkspacesClicked,
              base::Unretained(this)),
          u"Manage workspaces"));
  manage_workspaces_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  manage_workspaces_button_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kManageWorkspacesPadding, kFooterHorizontalPadding)));
  manage_workspaces_button_->SetMinSize(gfx::Size(0, kFooterRowHeight));
  manage_workspaces_button_->SetFocusBehavior(
      views::View::FocusBehavior::ALWAYS);
  manage_workspaces_button_->SetTextColorId(
      views::Button::STATE_NORMAL, kColorAstraWorkspaceAccent);

  // --- Button row (Settings | Help | Exit) ---
  auto button_row = std::make_unique<views::View>();
  auto* row_layout =
      button_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kFooterHorizontalPadding),
          kFooterButtonSpacing));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);
  row_layout->set_minimum_cross_axis_size(kFooterRowHeight);

  // Settings button.
  settings_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuFooterView::OnSettingsButtonClicked,
              base::Unretained(this)),
          u"Settings"));
  settings_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  settings_button_->SetFontList(
      settings_button_->font_list().Derive(
          kFooterFontSizeDelta, gfx::Font::NORMAL, kFooterFontWeight));

  // Help button.
  help_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuFooterView::OnHelpButtonClicked,
              base::Unretained(this)),
          u"Help"));
  help_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  help_button_->SetFontList(
      help_button_->font_list().Derive(
          kFooterFontSizeDelta, gfx::Font::NORMAL, kFooterFontWeight));

  // Exit button.
  exit_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuFooterView::OnExitButtonClicked,
              base::Unretained(this)),
          u"Exit"));
  exit_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  exit_button_->SetFontList(
      exit_button_->font_list().Derive(
          kFooterFontSizeDelta, gfx::Font::NORMAL, kFooterFontWeight));

  AddChildView(std::move(button_row));
}

AstraProfileMenuFooterView::~AstraProfileMenuFooterView() = default;

// ---------------------------------------------------------------------------
// views::View overrides
// ---------------------------------------------------------------------------

void AstraProfileMenuFooterView::OnThemeChanged() {
  View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update separator color.
  if (separator_) {
    separator_->SetColorId(ui::kColorSeparator);
  }

  // Update text colors for buttons.
  if (settings_button_) {
    settings_button_->SetTextColorId(
        views::Button::STATE_NORMAL, ui::kColorLabelForeground);
  }
  if (help_button_) {
    help_button_->SetTextColorId(
        views::Button::STATE_NORMAL, ui::kColorLabelForeground);
  }
  if (exit_button_) {
    exit_button_->SetTextColorId(
        views::Button::STATE_NORMAL, ui::kColorLabelForeground);
  }

  // "Manage workspaces" uses accent color.
  if (manage_workspaces_button_) {
    manage_workspaces_button_->SetTextColorId(
        views::Button::STATE_NORMAL, kColorAstraWorkspaceAccent);
  }
}

gfx::Size AstraProfileMenuFooterView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return View::CalculatePreferredSize(available_size);
}

// ---------------------------------------------------------------------------
// Button click handlers
// ---------------------------------------------------------------------------

void AstraProfileMenuFooterView::OnSettingsButtonClicked() {
  if (delegate_) {
    delegate_->OnSettingsClicked();
  }
}

void AstraProfileMenuFooterView::OnHelpButtonClicked() {
  if (delegate_) {
    delegate_->OnHelpClicked();
  }
}

void AstraProfileMenuFooterView::OnManageWorkspacesClicked() {
  if (delegate_) {
    delegate_->OnManageWorkspacesClicked();
  }
}

void AstraProfileMenuFooterView::OnExitButtonClicked() {
  if (delegate_) {
    delegate_->OnExitClicked();
  }
}

}  // namespace astra
