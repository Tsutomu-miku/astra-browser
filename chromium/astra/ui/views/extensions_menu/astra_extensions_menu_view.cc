// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_menu/astra_extensions_menu_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "skia/ext/image_operations.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Helper to get the category title.
std::u16string GetCategoryTitle(AstraExtensionCategory category) {
  switch (category) {
    case AstraExtensionCategory::kPinned:
      return u"Pinned";
    case AstraExtensionCategory::kActive:
      return u"Active on this site";
    case AstraExtensionCategory::kInactive:
      return u"Inactive";
    case AstraExtensionCategory::kBlocked:
      return u"Blocked on this site";
  }
  return std::u16string();
}

}  // namespace

// ===========================================================================
// AstraExtensionsMenuItemView
// ===========================================================================

AstraExtensionsMenuItemView::AstraExtensionsMenuItemView(
    const AstraExtensionMenuEntry& entry,
    AstraExtensionsMenuDelegate* delegate)
    : extension_id_(entry.extension_id),
      name_(entry.name),
      description_(entry.description),
      icon_(entry.icon),
      state_(entry.state),
      pinned_(entry.pinned_to_toolbar),
      has_badge_(entry.has_badge),
      badge_text_(entry.badge_text),
      badge_color_(entry.badge_color),
      delegate_(delegate) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Icon view.
  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetImageSize(gfx::Size(kIconSize, kIconSize));

  // Name label.
  name_label_ = AddChildView(std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorId(ui::kColorLabelForeground);

  // Description label.
  description_label_ = AddChildView(std::make_unique<views::Label>(description_));
  description_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  description_label_->SetAutoColorId(ui::kColorLabelForegroundSecondary);
  description_label_->SetElideBehavior(gfx::ELIDE_TAIL);

  // Pin button.
  pin_button_ = AddChildView(std::make_unique<views::ImageButton>());
  pin_button_->SetTooltipText(pinned_ ? u"Unpin from toolbar" : u"Pin to toolbar");
  pin_button_->SetAccessibleName(pinned_ ? u"Unpin from toolbar" : u"Pin to toolbar");
  pin_button_->SetCallback(base::BindRepeating(
      &AstraExtensionsMenuItemView::TogglePinned, base::Unretained(this)));

  UpdateVisuals();
}

AstraExtensionsMenuItemView::~AstraExtensionsMenuItemView() = default;

void AstraExtensionsMenuItemView::UpdateFromEntry(
    const AstraExtensionMenuEntry& entry) {
  DCHECK_EQ(extension_id_, entry.extension_id);
  name_ = entry.name;
  description_ = entry.description;
  icon_ = entry.icon;
  state_ = entry.state;
  pinned_ = entry.pinned_to_toolbar;
  has_badge_ = entry.has_badge;
  badge_text_ = entry.badge_text;
  badge_color_ = entry.badge_color;

  name_label_->SetText(name_);
  description_label_->SetText(description_);
  UpdateVisuals();
}

gfx::Size AstraExtensionsMenuItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size size;
  size.set_height(kItemHeight);
  // Width is determined by parent.
  return size;
}

void AstraExtensionsMenuItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  // Icon on the left.
  int icon_x = bounds.x() + kItemPadding;
  int icon_y = bounds.y() + (bounds.height() - kIconSize) / 2;
  icon_view_->SetBounds(icon_x, icon_y, kIconSize, kIconSize);

  // Pin button on the right.
  int pin_size = 20;
  int pin_x = bounds.right() - kItemPadding - pin_size;
  int pin_y = bounds.y() + (bounds.height() - pin_size) / 2;
  pin_button_->SetBounds(pin_x, pin_y, pin_size, pin_size);

  // Text area in the middle.
  int text_x = icon_x + kIconSize + kItemPadding;
  int text_width = pin_x - text_x - kItemPadding / 2;
  int text_y = bounds.y();

  // Name on top.
  name_label_->SetBounds(text_x, text_y + 8, text_width, 16);

  // Description on bottom.
  description_label_->SetBounds(text_x, text_y + 26, text_width, 14);
}

void AstraExtensionsMenuItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
}

void AstraExtensionsMenuItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->SetName(name_);
  node_data->role = ax::mojom::Role::kButton;
}

bool AstraExtensionsMenuItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton()) {
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraExtensionsMenuItemView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton() && HitTestPoint(event.location())) {
    HandlePrimaryClick();
    return;
  }
  views::View::OnMouseReleased(event);
}

void AstraExtensionsMenuItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraExtensionsMenuItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

void AstraExtensionsMenuItemView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    HandlePrimaryClick();
    event->SetHandled();
    return;
  }
  views::View::OnGestureEvent(event);
}

void AstraExtensionsMenuItemView::HandlePrimaryClick() {
  if (delegate_) {
    delegate_->OnExtensionClicked(extension_id_);
  }
}

void AstraExtensionsMenuItemView::TogglePinned() {
  if (delegate_) {
    delegate_->OnExtensionPinToggled(extension_id_, !pinned_);
  }
}

void AstraExtensionsMenuItemView::UpdateVisuals() {
  // Update icon opacity based on state.
  UpdateIcon();

  // Update name label.
  name_label_->SetEnabled(state_ != AstraExtensionState::kDisabled &&
                          state_ != AstraExtensionState::kBlocked);

  // Update pin button tooltip.
  pin_button_->SetTooltipText(pinned_ ? u"Unpin from toolbar"
                                   : u"Pin to toolbar");

  SchedulePaint();
}

void AstraExtensionsMenuItemView::UpdateIcon() {
  // If we have a real icon, use it.
  if (!icon_.isNull()) {
    icon_view_->SetImage(ui::ImageModel::FromImageSkia(icon_));
    return;
  }

  // Create a placeholder colored circle with first letter.
  // TODO(astra): Use proper default extension icon from Chromium resources.
}

// ===========================================================================
// AstraExtensionsMenuView
// ===========================================================================

AstraExtensionsMenuView::AstraExtensionsMenuView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                        views::BubbleBorder::TOP_RIGHT) {
  set_close_on_deactivate(true);
  set_close_on_esc(true);
  set_margins(gfx::Insets());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);

  Build();
}

AstraExtensionsMenuView::~AstraExtensionsMenuView() = default;

void AstraExtensionsMenuView::SetModel(AstraExtensionsMenuModel* model) {
  if (model_ == model) {
    return;
  }

  model_observation_.Reset();
  model_ = model;

  if (model_) {
    model_observation_.Observe(model_);
    RebuildItems();
  } else {
    item_views_.clear();
      if (content_container_) {
      // Remove all children except section headers and items.
      content_container_->RemoveAllChildViews();
    }
  }
}

void AstraExtensionsMenuView::SetSearchQuery(const std::u16string& query) {
  if (search_box_) {
    search_box_->SetText(query);
  }
}

std::u16string AstraExtensionsMenuView::GetSearchQuery() const {
  return search_box_ ? search_box_->GetText();
}

std::u16string AstraExtensionsMenuView::GetWindowTitle() const {
  return u"Extensions";
}

gfx::Size AstraExtensionsMenuView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kBubbleWidth, kBubbleMaxHeight);
}

void AstraExtensionsMenuView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  if (search_box_) {
    // Update search box styling.
  }
}

void AstraExtensionsMenuView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (model_) {
    model_->SetSearchQuery(new_contents);
  }
}

bool AstraExtensionsMenuView::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& key_event) {
  if (key_event.key_code() == ui::VKEY_ESCAPE) {
    GetWidget()->Close();
    return true;
  }
  return false;
}

void AstraExtensionsMenuView::OnExtensionsChanged(
    AstraExtensionsMenuModel* model) {
  DCHECK_EQ(model_, model);
  RebuildItems();
}

void AstraExtensionsMenuView::OnExtensionChanged(
    AstraExtensionsMenuModel* model,
    const std::string& extension_id) {
  DCHECK_EQ(model_, model);
  UpdateExtensionItem(extension_id);
}

void AstraExtensionsMenuView::OnExtensionsMenuModelShutdown(
    AstraExtensionsMenuModel* model) {
  DCHECK_EQ(model_, model);
  model_observation_.Reset();
  model_ = nullptr;
}

void AstraExtensionsMenuView::OnExtensionClicked(
    const std::string& extension_id) {
  if (outer_delegate_) {
    outer_delegate_->OnExtensionClicked(extension_id);
  }
  // Close the menu after clicking an extension.
  GetWidget()->Close();
}

void AstraExtensionsMenuView::OnExtensionPinToggled(
    const std::string& extension_id,
    bool pinned) {
  if (outer_delegate_) {
    outer_delegate_->OnExtensionPinToggled(extension_id, pinned);
  }
}

void AstraExtensionsMenuView::OnManageExtension(
    const std::string& extension_id) {
  if (outer_delegate_) {
    outer_delegate_->OnManageExtension(extension_id);
  }
  GetWidget()->Close();
}

void AstraExtensionsMenuView::OnRemoveExtension(
    const std::string& extension_id) {
  if (outer_delegate_) {
    outer_delegate_->OnRemoveExtension(extension_id);
  }
  GetWidget()->Close();
}

void AstraExtensionsMenuView::OnVisitChromeWebStore() {
  if (outer_delegate_) {
    outer_delegate_->OnVisitChromeWebStore();
  }
  GetWidget()->Close();
}

void AstraExtensionsMenuView::OnManageExtensionsPage() {
  if (outer_delegate_) {
    outer_delegate_->OnManageExtensionsPage();
  }
  GetWidget()->Close();
}

size_t AstraExtensionsMenuView::GetItemViewCountForTest() const {
  return item_views_.size();
}

void AstraExtensionsMenuView::Build() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Search box at top.
  auto search_container = std::make_unique<views::View>();
  search_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8),
      8, 8, 8)));
  search_container->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x1A);

  auto search_box = std::make_unique<views::Textfield>();
  search_box->SetPlaceholderText(u"Search extensions");
  search_box->SetAccessibleName(u"Search extensions");
  search_box->SetController(this);
  search_box->SetBorder(views::CreateRoundedRectBorder(
      1, 4, SkColorSetA(SK_ColorBLACK, 0x33)));
  search_box_ = search_container->AddChildView(std::move(search_box));
  search_box_->SetProperty(views::kFlexBehaviorKey,
                      views::FlexSpecification(
                          views::MinimumFlexSizeRule::kScaleToMinimum,
                          views::MaximumFlexSizeRule::kUnbounded));

  AddChildView(std::move(search_container));

  // Scroll view for extension list.
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->ClipHeightTo(0, kBubbleMaxHeight - kSearchBoxHeight - kFooterHeight - 24);
  scroll_view_ = scroll_view.get();

  content_container_ = scroll_view->SetContents(std::make_unique<views::View>());
  content_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  AddChildView(std::move(scroll_view));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));

  // Footer.
  auto footer = std::make_unique<views::View>();
  footer->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8),
      8, 8)));
  footer->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetA(SK_ColorBLACK, 0x1A)));

  auto manage_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraExtensionsMenuView::OnManageExtensionsClicked,
                          base::Unretained(this)),
      u"Manage extensions");
  manage_button->SetUnderlineColorId(ui::kColorLabelForeground);
  footer->AddChildView(std::move(manage_button));

  auto store_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraExtensionsMenuView::OnChromeWebStoreClicked,
                          base::Unretained(this)),
      u"Chrome Web Store");
  store_button->SetUnderlineColorId(ui::kColorLabelForeground);
  footer->AddChildView(std::move(store_button));

  footer_ = AddChildView(std::move(footer));
  footer_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::MinimumFlexSizeRule::kPreferred,
          views::MaximumFlexSizeRule::kPreferred));
}

void AstraExtensionsMenuView::RebuildItems() {
  if (!content_container_ || !model_) {
    return;
  }

  item_views_.clear();
  content_container_->RemoveAllChildViews();

  auto filtered = model_->GetFilteredExtensions();
  if (filtered.empty()) {
    auto empty_label = std::make_unique<views::Label>(u"No extensions found");
    empty_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    empty_label->SetAutoColorId(ui::kColorLabelForegroundSecondary);
    empty_label->SetBorder(gfx::Insets(24));
    content_container_->AddChildView(std::move(empty_label));
    return;
  }

  // Group by category.
  BuildCategoryItems(AstraExtensionCategory::kPinned, u"Pinned",
                    content_container_);
  BuildCategoryItems(AstraExtensionCategory::kActive, u"Active on this site",
                    content_container_);
  BuildCategoryItems(AstraExtensionCategory::kInactive, u"Inactive",
                    content_container_);
  BuildCategoryItems(AstraExtensionCategory::kBlocked, u"Blocked on this site",
                    content_container_);

  content_container_->InvalidateLayout();
  if (scroll_view_) {
    scroll_view_->Layout();
  }
}

void AstraExtensionsMenuView::UpdateItems() {
  if (!model_) {
    return;
  }
  for (auto* item : item_views_) {
    const auto* entry = model_->GetExtension(item->extension_id());
    if (entry) {
      item->UpdateFromEntry(*entry);
    }
  }
}

void AstraExtensionsMenuView::UpdateExtensionItem(
    const std::string& extension_id) {
  AstraExtensionsMenuItemView* item = FindItemView(extension_id);
  if (!item || !model_) {
    return;
  }
  const auto* entry = model_->GetExtension(extension_id);
  if (entry) {
    item->UpdateFromEntry(*entry);
  }
}

std::unique_ptr<views::View> AstraExtensionsMenuView::CreateSectionHeader(
    const std::u16string& title,
    size_t count) {
  auto header = std::make_unique<views::View>();
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8, 12, 4, 12)));
  header->SetPreferredSize(gfx::Size(0, kSectionHeaderHeight));

  auto title_label = std::make_unique<views::Label>(title));
  title_label->SetAutoColorId(ui::kColorLabelForegroundSecondary);
  title_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
      views::style::STYLE_PRIMARY));
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->AddChildView(std::move(title_label));

  return header;
}

void AstraExtensionsMenuView::BuildCategoryItems(
    AstraExtensionCategory category,
    const std::u16string& title,
    views::View* container) {
  if (!model_) {
    return;
  }

  auto items = model_->GetExtensionsByCategory(category);
  // Also consider filtered set, filtered also filtered;
  // Apply search filter.
  if (!model_->GetSearchQuery().empty()) {
    auto filtered = model_->GetFilteredExtensions();
    items.clear();
    for (const auto& entry : filtered) {
      if (entry.category == category) {
        items.push_back(entry);
      }
    }
  }

  if (items.empty()) {
    return;
  }

  container->AddChildView(CreateSectionHeader(title, items.size()));

  for (const auto& entry : items) {
    auto item_view = std::make_unique<AstraExtensionsMenuItemView>(entry, this);
    AstraExtensionsMenuItemView* raw_item = item_view.get();
    container->AddChildView(std::move(item_view));
    item_views_.push_back(raw_item);
  }
}

AstraExtensionsMenuItemView* AstraExtensionsMenuView::FindItemView(
    const std::string& extension_id) const {
  for (auto* item : item_views_) {
    if (item->extension_id() == extension_id) {
      return item;
    }
  }
  return nullptr;
}

void AstraExtensionsMenuView::OnManageExtensionsClicked() {
  OnManageExtensionsPage();
}

void AstraExtensionsMenuView::OnChromeWebStoreClicked() {
  OnVisitChromeWebStore();
}

}  // namespace astra
