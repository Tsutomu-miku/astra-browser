// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_switcher_bubble.h"

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
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 320;
constexpr int kItemHeight = 52;
constexpr int kSearchHeight = 40;
constexpr int kMaxVisibleItems = 8;
constexpr int kAccentDotSize = 10;

// Parse a hex color string like "#RRGGBB" to SkColor.
SkColor ParseHexColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') return SK_ColorGRAY;
  std::string clean = hex.substr(1);
  if (clean.size() != 6) return SK_ColorGRAY;
  unsigned int val = 0;
  if (sscanf(clean.c_str(), "%x", &val) != 1) return SK_ColorGRAY;
  return SkColorSetRGB(SkColorGetR(val), SkColorGetG(val), SkColorGetB(val));
}

}  // namespace

// ===========================================================================
// AstraWorkspaceSwitcherItemView
// ===========================================================================

AstraWorkspaceSwitcherItemView::AstraWorkspaceSwitcherItemView(
    const AstraWorkspace& workspace,
    SelectCallback callback)
    : views::Button(base::BindRepeating(
          &AstraWorkspaceSwitcherItemView::OnItemClicked,
          base::Unretained(this))),
      workspace_id_(workspace.id),
      workspace_name_(workspace.name),
      description_(workspace.description),
      accent_color_(workspace.color),
      tab_count_(workspace.tab_count),
      window_count_(workspace.window_count),
      is_active_(workspace.is_active),
      select_callback_(std::move(callback)) {
  BuildLayout();
}

AstraWorkspaceSwitcherItemView::~AstraWorkspaceSwitcherItemView() = default;

void AstraWorkspaceSwitcherItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(10, 14), 12));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Accent color dot.
  accent_dot_ = AddChildView(std::make_unique<views::View>());
  accent_dot_->SetPreferredSize(
      gfx::Size(kAccentDotSize, kAccentDotSize));
  accent_dot_->SetBackground(
      views::CreateRoundedRectBackground(
          ParseHexColor(accent_color_), kAccentDotSize / 2));

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

  name_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(workspace_name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  detail_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(window_count_) + " window" +
          (window_count_ != 1 ? "s" : ""))));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  detail_label_->SetElideBehavior(gfx::ELIDE_END);

  // Tab count + checkmark.
  auto* right_col = AddChildView(std::make_unique<views::View>());
  right_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  right_col->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  tab_count_label_ = right_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(tab_count_) + " tabs")));
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  checkmark_label_ = right_col->AddChildView(
      std::make_unique<views::Label>(u"✓"));
  checkmark_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  checkmark_label_->SetAutoColorReadabilityEnabled(false);
  checkmark_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  checkmark_label_->SetVisible(is_active_);

  UpdateVisualState();
}

void AstraWorkspaceSwitcherItemView::SetIsActive(bool is_active) {
  if (is_active_ == is_active) return;
  is_active_ = is_active;
  if (checkmark_label_) {
    checkmark_label_->SetVisible(is_active_);
  }
  UpdateVisualState();
}

void AstraWorkspaceSwitcherItemView::SetIsHovered(bool is_hovered) {
  if (is_hovered_ == is_hovered) return;
  is_hovered_ = is_hovered;
  UpdateVisualState();
}

void AstraWorkspaceSwitcherItemView::UpdateVisualState() {
  // Update background based on state.
  SkColor bg = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg = SkColorSetA(SK_ColorBLUE, 0x1A);  // Light blue tint
  } else if (is_hovered_) {
    bg = SkColorSetA(SK_ColorBLACK, 0x0A);  // Subtle hover
  }
  SetBackground(views::CreateSolidBackground(bg));

  SchedulePaint();
}

void AstraWorkspaceSwitcherItemView::OnItemClicked() {
  if (select_callback_) {
    select_callback_.Run(workspace_id_);
  }
}

void AstraWorkspaceSwitcherItemView::OnThemeChanged() {
  views::Button::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  tab_count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  checkmark_label_->SetEnabledColor(
      cp->GetColor(ui::kColorAccent));

  UpdateVisualState();
}

void AstraWorkspaceSwitcherItemView::OnPaintBackground(gfx::Canvas* canvas) {
  views::Button::OnPaintBackground(canvas);
}

// ===========================================================================
// AstraWorkspaceSwitcherBubble
// ===========================================================================

AstraWorkspaceSwitcherBubble::AstraWorkspaceSwitcherBubble(
    views::View* anchor_view,
    AstraWorkspaceService* service,
    WorkspaceSelectedCallback selected_callback,
    NewWorkspaceCallback new_workspace_callback)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_LEFT),
      service_(service),
      selected_callback_(std::move(selected_callback)),
      new_workspace_callback_(std::move(new_workspace_callback)) {
  SetShowCloseButton(false);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraWorkspaceSwitcherBubble::~AstraWorkspaceSwitcherBubble() = default;

void AstraWorkspaceSwitcherBubble::SetWorkspaces(
    const std::vector<AstraWorkspace>& workspaces) {
  all_workspaces_ = workspaces;
  RefreshWorkspaceList();
}

void AstraWorkspaceSwitcherBubble::SetActiveWorkspaceId(
    const std::string& workspace_id) {
  active_workspace_id_ = workspace_id;
  for (size_t i = 0; i < workspace_items_.size(); i++) {
    workspace_items_[i]->SetIsActive(
        workspace_items_[i]->workspace_id() == workspace_id);
  }
}

void AstraWorkspaceSwitcherBubble::SetSearchQuery(
    const std::u16string& query) {
  if (search_field_) {
    search_field_->SetText(query);
  }
  FilterWorkspaces();
}

void AstraWorkspaceSwitcherBubble::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  // Search field.
  BuildSearchField();

  // Workspace list (scrollable).
  BuildWorkspaceList();

  // New workspace button.
  BuildNewWorkspaceButton();
}

void AstraWorkspaceSwitcherBubble::BuildSearchField() {
  search_field_ = AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search workspaces...");
  search_field_->SetPreferredSize(
      gfx::Size(kBubbleWidth, kSearchHeight));
  search_field_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));
  search_field_->set_controller(
      base::BindRepeating(
          &AstraWorkspaceSwitcherBubble::OnSearchTextChanged,
          base::Unretained(this)));

  // Request focus when shown.
  search_field_->RequestFocus();
}

void AstraWorkspaceSwitcherBubble::BuildWorkspaceList() {
  scroll_view_ = AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kItemHeight * kMaxVisibleItems);
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  workspace_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  workspace_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));

  // Populate if we have service data.
  if (service_) {
    // TODO(astra): Populate from service.
    // For now, we'll rely on SetWorkspaces being called externally.
  }
}

void AstraWorkspaceSwitcherBubble::BuildNewWorkspaceButton() {
  new_workspace_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraWorkspaceSwitcherBubble::OnNewWorkspaceClicked,
              base::Unretained(this)),
          u"+ New workspace"));
  new_workspace_button_->SetBorder(
      views::CreateSolidSidedBorder(1, 0, 0, 0, SK_ColorGRAY));
  new_workspace_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  new_workspace_button_->SetEnabledTextColors(SK_ColorBLUE);
}

void AstraWorkspaceSwitcherBubble::RefreshWorkspaceList() {
  if (!workspace_list_) return;

  workspace_list_->RemoveAllChildViews();
  workspace_items_.clear();
  selected_index_ = -1;

  for (size_t i = 0; i < all_workspaces_.size(); i++) {
    const auto& ws = all_workspaces_[i];
    auto* item = workspace_list_->AddChildView(
        std::make_unique<AstraWorkspaceSwitcherItemView>(
            ws,
            base::BindRepeating(
                &AstraWorkspaceSwitcherBubble::OnWorkspaceSelected,
                base::Unretained(this))));
    workspace_items_.push_back(item);

    if (ws.id == active_workspace_id_) {
      selected_index_ = static_cast<int>(i);
    }
  }

  InvalidateLayout();
}

void AstraWorkspaceSwitcherBubble::FilterWorkspaces() {
  if (!search_field_) return;

  std::u16string query = search_field_->GetText();

  // Hide items that don't match query.
  for (size_t i = 0; i < workspace_items_.size(); i++) {
    bool matches = query.empty() ||
        workspace_items_[i]->workspace_name().find(query) !=
            std::u16string::npos;
    workspace_items_[i]->SetVisible(matches);
  }

  InvalidateLayout();
}

void AstraWorkspaceSwitcherBubble::OnWorkspaceSelected(
    const std::string& workspace_id) {
  if (selected_callback_) {
    selected_callback_.Run(workspace_id);
  }
  GetWidget()->Close();
}

void AstraWorkspaceSwitcherBubble::OnNewWorkspaceClicked() {
  if (new_workspace_callback_) {
    new_workspace_callback_.Run();
  }
  GetWidget()->Close();
}

void AstraWorkspaceSwitcherBubble::OnSearchTextChanged() {
  FilterWorkspaces();
}

void AstraWorkspaceSwitcherBubble::MoveSelection(int delta) {
  if (workspace_items_.empty()) return;

  // Find the next visible item.
  int count = static_cast<int>(workspace_items_.size());
  int new_index = selected_index_;

  for (int i = 0; i < count; i++) {
    new_index += delta;
    if (new_index < 0) new_index = count - 1;
    if (new_index >= count) new_index = 0;

    if (workspace_items_[new_index]->GetVisible()) {
      if (selected_index_ >= 0 &&
          selected_index_ < static_cast<int>(workspace_items_.size())) {
        workspace_items_[selected_index_]->SetIsHovered(false);
      }
      selected_index_ = new_index;
      workspace_items_[selected_index_]->SetIsHovered(true);
      break;
    }
  }
}

void AstraWorkspaceSwitcherBubble::ActivateSelected() {
  if (selected_index_ >= 0 &&
      selected_index_ < static_cast<int>(workspace_items_.size())) {
    OnWorkspaceSelected(
        workspace_items_[selected_index_]->workspace_id());
  }
}

std::u16string AstraWorkspaceSwitcherBubble::GetWindowTitle() const {
  return u"Switch Workspace";
}

void AstraWorkspaceSwitcherBubble::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

bool AstraWorkspaceSwitcherBubble::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  // Handle up/down arrows and Enter.
  if (accelerator.key_code() == ui::VKEY_DOWN) {
    MoveSelection(1);
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_UP) {
    MoveSelection(-1);
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_RETURN) {
    ActivateSelected();
    return true;
  }
  return views::BubbleDialogDelegateView::AcceleratorPressed(accelerator);
}

}  // namespace astra
