// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_workspace_menu_item_view.h"

#include <algorithm>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants — sized to match Chromium profile menu item density.
// Chromium owner: ProfileMenuView (chrome/browser/ui/views/profiles/)
constexpr int kRowHeightSmall = 32;
constexpr int kRowHeightMedium = 40;
constexpr int kRowHeightLarge = 48;

constexpr int kRowHorizontalPaddingSmall = 12;
constexpr int kRowHorizontalPaddingMedium = 16;
constexpr int kRowHorizontalPaddingLarge = 20;

constexpr int kRowChildSpacingSmall = 8;
constexpr int kRowChildSpacingMedium = 12;
constexpr int kRowChildSpacingLarge = 16;

constexpr int kColorDotSizeSmall = 8;
constexpr int kColorDotSizeMedium = 12;
constexpr int kColorDotSizeLarge = 16;

constexpr int kCheckmarkSizeSmall = 12;
constexpr int kCheckmarkSizeMedium = 16;
constexpr int kCheckmarkSizeLarge = 20;

constexpr int kReorderHandleWidth = 20;

// Font weight for active workspace name.
constexpr gfx::Font::Weight kActiveFontWeight = gfx::Font::Weight::BOLD;

}  // namespace

// ---------------------------------------------------------------------------
// AstraWorkspaceMenuItemView
// ---------------------------------------------------------------------------

AstraWorkspaceMenuItemView::AstraWorkspaceMenuItemView(
    const std::u16string& workspace_name,
    SkColor accent_color,
    int tab_count,
    bool is_active,
    ActivatedCallback callback)
    : Button(base::BindRepeating(&AstraWorkspaceMenuItemView::ButtonPressed,
                                 base::Unretained(this))),
      workspace_name_(workspace_name),
      accent_color_(accent_color),
      tab_count_(tab_count),
      is_active_(is_active),
      activated_callback_(std::move(callback)) {
  // Set button behavior: focusable, shows ink drop on hover/press.
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  SetFocusRingColorId(kColorAstraWorkspaceAccent);
  SetRequestFocusOnPress(false);

  UpdateSizeVariant();
}

AstraWorkspaceMenuItemView::~AstraWorkspaceMenuItemView() = default;

// ---------------------------------------------------------------------------
// Property updates
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::SetWorkspaceName(const std::u16string& name) {
  workspace_name_ = name;
  if (name_label_) {
    name_label_->SetText(name);
  }
  OnPropertyChanged(&workspace_name_, views::kPropertyEffectsLayout);
}

void AstraWorkspaceMenuItemView::SetAccentColor(SkColor color) {
  accent_color_ = color;
  UpdateColorDot();
}

void AstraWorkspaceMenuItemView::SetTabCount(int count) {
  tab_count_ = count;
  UpdateTabCountLabel();
}

void AstraWorkspaceMenuItemView::SetIsActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  UpdateActiveVisuals();
  SchedulePaint();
}

// ---------------------------------------------------------------------------
// Display mode
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::SetDisplayMode(
    AstraWorkspaceItemDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  UpdateDisplayModeVisibility();
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Size variant
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::SetSizeVariant(AstraWorkspaceItemSize size) {
  if (size_variant_ == size) {
    return;
  }
  size_variant_ = size;
  UpdateSizeVariant();
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Reorder handle
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::SetReorderHandleVisible(bool visible) {
  if (reorder_handle_visible_ == visible) {
    return;
  }
  reorder_handle_visible_ = visible;
  if (reorder_container_) {
    reorder_container_->SetVisible(visible);
  }
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// views::Button overrides
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::OnThemeChanged() {
  Button::OnThemeChanged();
  UpdateActiveVisuals();
  UpdateColorDot();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update tab count label color (secondary text).
  if (tab_count_label_) {
    tab_count_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

gfx::Size AstraWorkspaceMenuItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = Button::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), GetRowHeight()));
  return size;
}

void AstraWorkspaceMenuItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  Button::GetAccessibleNodeData(node_data);

  // Set accessible name to workspace name.
  node_data->SetName(workspace_name_);

  // Set role to menu item.
  node_data->role = ax::mojom::Role::kMenuItem;

  // Mark as checked / selected if this is the active workspace.
  if (is_active_) {
    node_data->SetCheckedState(ax::mojom::CheckedState::kTrue);
    node_data->AddState(ax::mojom::State::kSelected);
  } else {
    node_data->SetCheckedState(ax::mojom::CheckedState::kFalse);
  }

  // Add description with tab count.
  std::u16string description =
      base::NumberToString16(tab_count_) + u" tabs";
  node_data->SetDescription(description);

  // Add position info if reorder is available.
  if (reorder_handle_visible_) {
    description += u", reorderable";
    node_data->SetDescription(description);
  }
}

bool AstraWorkspaceMenuItemView::OnKeyPressed(const ui::KeyEvent& event) {
  // Enter or Space activates the item (handled by Button base class).
  if (event.key_code() == ui::VKEY_RETURN ||
      event.key_code() == ui::VKEY_SPACE) {
    return Button::OnKeyPressed(event);
  }

  // Alt+Up / Alt+Down for reordering when reorder handle is visible.
  if (reorder_handle_visible_ && reorder_callback_ &&
      event.IsAltDown()) {
    if (event.key_code() == ui::VKEY_UP) {
      reorder_callback_.Run(-1);
      return true;
    }
    if (event.key_code() == ui::VKEY_DOWN) {
      reorder_callback_.Run(1);
      return true;
    }
  }

  return false;
}

void AstraWorkspaceMenuItemView::OnMouseEntered(const ui::MouseEvent& event) {
  Button::OnMouseEntered(event);
  is_hovered_ = true;
  UpdateActiveVisuals();
}

void AstraWorkspaceMenuItemView::OnMouseExited(const ui::MouseEvent& event) {
  Button::OnMouseExited(event);
  is_hovered_ = false;
  UpdateActiveVisuals();
}

void AstraWorkspaceMenuItemView::OnFocus() {
  Button::OnFocus();
  SchedulePaint();
}

void AstraWorkspaceMenuItemView::OnBlur() {
  Button::OnBlur();
  SchedulePaint();
}

void AstraWorkspaceMenuItemView::Layout() {
  Button::Layout();

  // Position reorder buttons vertically if they exist.
  if (reorder_container_ && reorder_up_ && reorder_down_) {
    gfx::Rect bounds = reorder_container_->GetLocalBounds();
    int half_height = bounds.height() / 2;

    gfx::Rect up_bounds(0, 0, bounds.width(), half_height);
    gfx::Rect down_bounds(0, half_height, bounds.width(), bounds.height() - half_height);

    reorder_up_->SetBoundsRect(up_bounds);
    reorder_down_->SetBoundsRect(down_bounds);
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AstraWorkspaceMenuItemView::UpdateActiveVisuals() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (!name_label_) {
    return;
  }

  // Name label: bold if active, normal otherwise.
  gfx::Font::Weight weight =
      is_active_ ? kActiveFontWeight : gfx::Font::Weight::NORMAL;
  name_label_->SetFontList(
      name_label_->font_list().Derive(0, gfx::Font::NORMAL, weight));

  // Text color.
  ui::ColorId text_color_id = ui::kColorLabelForeground;
  if (is_active_) {
    text_color_id = kColorAstraWorkspaceAccent;
  }
  name_label_->SetEnabledColor(color_provider->GetColor(text_color_id));

  // Background: subtle accent tint if active, hover if hovered.
  if (is_active_) {
    SetBackground(views::CreateSolidBackground(
        color_provider->GetColor(kColorAstraWorkspaceAccentSubtle)));
  } else if (is_hovered_) {
    SetBackground(views::CreateSolidBackground(
        color_provider->GetColor(ui::kColorHoverButtonBackground)));
  } else {
    SetBackground(nullptr);
  }

  // Checkmark visibility.
  if (checkmark_indicator_) {
    checkmark_indicator_->SetVisible(is_active_);
    if (is_active_) {
      checkmark_indicator_->SetBackground(views::CreateRoundedRectBackground(
          color_provider->GetColor(kColorAstraWorkspaceAccent),
          GetColorDotSize() / 4));
    }
  }
}

void AstraWorkspaceMenuItemView::UpdateColorDot() {
  if (!color_dot_) {
    return;
  }
  color_dot_->SetBackground(
      views::CreateRoundedRectBackground(accent_color_, GetColorDotSize() / 2));
  SchedulePaint();
}

void AstraWorkspaceMenuItemView::UpdateTabCountLabel() {
  if (!tab_count_label_) {
    return;
  }
  tab_count_label_->SetText(
      base::NumberToString16(tab_count_) + u" tabs");
}

void AstraWorkspaceMenuItemView::UpdateDisplayModeVisibility() {
  bool show_icons = (display_mode_ == AstraWorkspaceItemDisplayMode::kIconsOnly ||
                     display_mode_ == AstraWorkspaceItemDisplayMode::kIconsAndNames);
  bool show_names = (display_mode_ == AstraWorkspaceItemDisplayMode::kNamesOnly ||
                     display_mode_ == AstraWorkspaceItemDisplayMode::kIconsAndNames);

  if (color_dot_) {
    color_dot_->SetVisible(show_icons);
  }
  if (name_label_) {
    name_label_->SetVisible(show_names);
  }
  if (tab_count_label_) {
    tab_count_label_->SetVisible(show_names);
  }
  if (checkmark_indicator_) {
    checkmark_indicator_->SetVisible(is_active_ && show_names);
  }
}

void AstraWorkspaceMenuItemView::UpdateSizeVariant() {
  // Rebuild child views with the current size variant.
  RemoveAllChildViews();

  int row_height = GetRowHeight();
  int horizontal_padding = [&]() {
    switch (size_variant_) {
      case AstraWorkspaceItemSize::kSmall:
        return kRowHorizontalPaddingSmall;
      case AstraWorkspaceItemSize::kMedium:
        return kRowHorizontalPaddingMedium;
      case AstraWorkspaceItemSize::kLarge:
        return kRowHorizontalPaddingLarge;
    }
    return kRowHorizontalPaddingMedium;
  }();

  int child_spacing = [&]() {
    switch (size_variant_) {
      case AstraWorkspaceItemSize::kSmall:
        return kRowChildSpacingSmall;
      case AstraWorkspaceItemSize::kMedium:
        return kRowChildSpacingMedium;
      case AstraWorkspaceItemSize::kLarge:
        return kRowChildSpacingLarge;
    }
    return kRowChildSpacingMedium;
  }();

  // Horizontal layout: reorder handle | color dot | name (flex) | tab count | checkmark.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, horizontal_padding), child_spacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_minimum_cross_axis_size(row_height);

  // --- Reorder handle (up/down buttons) ---
  auto reorder_container = std::make_unique<views::View>();
  reorder_container->SetPreferredSize(
      gfx::Size(kReorderHandleWidth, row_height));

  auto reorder_up = std::make_unique<views::View>();
  reorder_up->SetTooltipText(u"Move up");
  reorder_up_ = reorder_up.get();
  reorder_container->AddChildView(std::move(reorder_up));

  auto reorder_down = std::make_unique<views::View>();
  reorder_down->SetTooltipText(u"Move down");
  reorder_down_ = reorder_down.get();
  reorder_container->AddChildView(std::move(reorder_down));

  reorder_container_ = AddChildView(std::move(reorder_container));
  reorder_container_->SetVisible(reorder_handle_visible_);

  // --- Color dot (accent color indicator) ---
  int dot_size = GetColorDotSize();
  auto color_dot = std::make_unique<views::View>();
  color_dot->SetPreferredSize(gfx::Size(dot_size, dot_size));
  color_dot->SetBackground(views::CreateRoundedRectBackground(
      accent_color_, dot_size / 2));
  color_dot_ = AddChildView(std::move(color_dot));

  // --- Workspace name label ---
  auto name_label = std::make_unique<views::Label>();
  name_label->SetText(workspace_name_);
  name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label->SetAutoColorReadabilityEnabled(false);
  name_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label_ = AddChildView(std::move(name_label));
  layout->SetFlexForView(name_label_, 1);

  // --- Tab count label (secondary text) ---
  auto tab_count_label = std::make_unique<views::Label>();
  tab_count_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  tab_count_label->SetAutoColorReadabilityEnabled(false);
  tab_count_label->SetFontList(
      tab_count_label->font_list().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  tab_count_label_ = AddChildView(std::move(tab_count_label));

  // --- Checkmark indicator (active workspace) ---
  int checkmark_size = [&]() {
    switch (size_variant_) {
      case AstraWorkspaceItemSize::kSmall:
        return kCheckmarkSizeSmall;
      case AstraWorkspaceItemSize::kMedium:
        return kCheckmarkSizeMedium;
      case AstraWorkspaceItemSize::kLarge:
        return kCheckmarkSizeLarge;
    }
    return kCheckmarkSizeMedium;
  }();

  auto checkmark = std::make_unique<views::View>();
  checkmark->SetPreferredSize(gfx::Size(checkmark_size, checkmark_size));
  // TODO(astra): Replace with a real checkmark icon.
  checkmark_indicator_ = AddChildView(std::move(checkmark));

  // Initialize state.
  UpdateTabCountLabel();
  UpdateDisplayModeVisibility();

  // Apply theme.
  if (GetColorProvider()) {
    OnThemeChanged();
  }
}

void AstraWorkspaceMenuItemView::ButtonPressed() {
  if (activated_callback_) {
    activated_callback_.Run();
  }
}

void AstraWorkspaceMenuItemView::OnReorderUpClicked() {
  if (reorder_callback_) {
    reorder_callback_.Run(-1);
  }
}

void AstraWorkspaceMenuItemView::OnReorderDownClicked() {
  if (reorder_callback_) {
    reorder_callback_.Run(1);
  }
}

int AstraWorkspaceMenuItemView::GetRowHeight() const {
  switch (size_variant_) {
    case AstraWorkspaceItemSize::kSmall:
      return kRowHeightSmall;
    case AstraWorkspaceItemSize::kMedium:
      return kRowHeightMedium;
    case AstraWorkspaceItemSize::kLarge:
      return kRowHeightLarge;
  }
  return kRowHeightMedium;
}

int AstraWorkspaceMenuItemView::GetColorDotSize() const {
  switch (size_variant_) {
    case AstraWorkspaceItemSize::kSmall:
      return kColorDotSizeSmall;
    case AstraWorkspaceItemSize::kMedium:
      return kColorDotSizeMedium;
    case AstraWorkspaceItemSize::kLarge:
      return kColorDotSizeLarge;
  }
  return kColorDotSizeMedium;
}

}  // namespace astra
