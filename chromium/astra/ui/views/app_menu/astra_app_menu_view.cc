// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/app_menu/astra_app_menu_view.h"

#include <algorithm>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/bubble/bubble_frame_view.h"

namespace astra {

namespace {

// Get the default label for a menu item type.
std::u16string GetDefaultItemLabel(AstraAppMenuItemType type) {
  switch (type) {
    case AstraAppMenuItemType::kNewTab:
      return u"New tab";
    case AstraAppMenuItemType::kNewWindow:
      return u"New window";
    case AstraAppMenuItemType::kNewIncognitoWindow:
      return u"New incognito window";
    case AstraAppMenuItemType::kNewWorkspace:
      return u"New workspace";
    case AstraAppMenuItemType::kHistory:
      return u"History";
    case AstraAppMenuItemType::kDownloads:
      return u"Downloads";
    case AstraAppMenuItemType::kBookmarks:
      return u"Bookmarks";
    case AstraAppMenuItemType::kZoomIn:
      return u"Zoom in";
    case AstraAppMenuItemType::kZoomOut:
      return u"Zoom out";
    case AstraAppMenuItemType::kZoomReset:
      return u"Reset zoom";
    case AstraAppMenuItemType::kPrint:
      return u"Print";
    case AstraAppMenuItemType::kFind:
      return u"Find";
    case AstraAppMenuItemType::kMoreTools:
      return u"More tools";
    case AstraAppMenuItemType::kSettings:
      return u"Settings";
    case AstraAppMenuItemType::kHelp:
      return u"Help";
    case AstraAppMenuItemType::kAbout:
      return u"About Astra";
    case AstraAppMenuItemType::kExit:
      return u"Exit";
    case AstraAppMenuItemType::kFocusMode:
      return u"Focus mode";
    case AstraAppMenuItemType::kSplitView:
      return u"Split view";
    case AstraAppMenuItemType::kCommandPalette:
      return u"Command palette";
    case AstraAppMenuItemType::kWorkspaces:
      return u"Workspaces";
    case AstraAppMenuItemType::kSidebar:
      return u"Sidebar";
    case AstraAppMenuItemType::kScreenshot:
      return u"Screenshot";
  }
  return std::u16string();
}

// Get the default shortcut for a menu item type.
std::string GetDefaultShortcut(AstraAppMenuItemType type) {
  switch (type) {
    case AstraAppMenuItemType::kNewTab:
      return "Ctrl+T";
    case AstraAppMenuItemType::kNewWindow:
      return "Ctrl+N";
    case AstraAppMenuItemType::kNewIncognitoWindow:
      return "Ctrl+Shift+N";
    case AstraAppMenuItemType::kHistory:
      return "Ctrl+H";
    case AstraAppMenuItemType::kDownloads:
      return "Ctrl+J";
    case AstraAppMenuItemType::kBookmarks:
      return "Ctrl+Shift+B";
    case AstraAppMenuItemType::kZoomIn:
      return "Ctrl++";
    case AstraAppMenuItemType::kZoomOut:
      return "Ctrl+-";
    case AstraAppMenuItemType::kZoomReset:
      return "Ctrl+0";
    case AstraAppMenuItemType::kPrint:
      return "Ctrl+P";
    case AstraAppMenuItemType::kFind:
      return "Ctrl+F";
    case AstraAppMenuItemType::kSettings:
      return "Ctrl+,";
    case AstraAppMenuItemType::kCommandPalette:
      return "Ctrl+Shift+P";
    case AstraAppMenuItemType::kFocusMode:
      return "Ctrl+Shift+F";
    case AstraAppMenuItemType::kSidebar:
      return "Ctrl+\\\\";
    case AstraAppMenuItemType::kScreenshot:
      return "Ctrl+Shift+S";
    default:
      return "";
  }
}

// Draw a three-dot menu icon.
void DrawMenuIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int dot_size = std::min(bounds.width(), bounds.height()) / 8;
  int spacing = bounds.height() / 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx, cy - spacing), dot_size, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_size, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + spacing), dot_size, flags);
}

}  // namespace

// =========================================================================
// AstraAppMenuButton
// =========================================================================

AstraAppMenuButton::AstraAppMenuButton()
    : views::ImageButton(base::BindRepeating(
          &AstraAppMenuButton::HandleButtonPress, base::Unretained(this))) {
  SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  SetTooltipText(u"Customize and control Astra");
}

AstraAppMenuButton::~AstraAppMenuButton() {
  if (menu_bubble_ && menu_bubble_->GetWidget())
    menu_bubble_->GetWidget()->Close();
}

void AstraAppMenuButton::ShowMenu() {
  if (menu_bubble_ && menu_bubble_->GetWidget()) {
    menu_bubble_->GetWidget()->Show();
    return;
  }

  // Create menu bubble.
  auto menu = std::make_unique<AstraAppMenuView>(this);
  menu->PopulateDefaultItems();

  menu_bubble_ = menu.get();
  views::BubbleDialogDelegateView::CreateBubble(std::move(menu))->Show();
}

void AstraAppMenuButton::HideMenu() {
  if (menu_bubble_ && menu_bubble_->GetWidget())
    menu_bubble_->GetWidget()->Close();
}

bool AstraAppMenuButton::IsMenuShowing() const {
  return menu_bubble_ && menu_bubble_->GetWidget() &&
         menu_bubble_->GetWidget()->IsVisible();
}

void AstraAppMenuButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);

  gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                     /*image_scale=*/1.0f, false);
  DrawMenuIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize), text_color);
  SetImage(views::ImageButton::STATE_NORMAL,
           gfx::ImageSkia(canvas.GetBitmap(),
                          gfx::Size(kButtonSize, kButtonSize)));
}

bool AstraAppMenuButton::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    HandleButtonPress();
    return true;
  }
  return views::ImageButton::OnMousePressed(event);
}

void AstraAppMenuButton::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    HandleButtonPress();
    event->SetHandled();
  }
}

void AstraAppMenuButton::HandleButtonPress() {
  if (IsMenuShowing()) {
    HideMenu();
  } else {
    ShowMenu();
  }
}

// =========================================================================
// AstraAppMenuView
// =========================================================================

AstraAppMenuView::AstraAppMenuView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  BuildLayout();
}

AstraAppMenuView::~AstraAppMenuView() = default;

// -- Menu items -------------------------------------------------------------

void AstraAppMenuView::AddMenuItem(const AstraAppMenuItem& item) {
  items_.push_back(item);
  RebuildItems();
}

void AstraAppMenuView::AddSeparator() {
  AstraAppMenuItem item;
  item.is_separator = true;
  items_.push_back(item);
  RebuildItems();
}

void AstraAppMenuView::AddSectionHeader(const std::u16string& title) {
  AstraAppMenuItem item;
  item.label = title;
  item.is_section_header = true;
  items_.push_back(item);
  RebuildItems();
}

void AstraAppMenuView::ClearItems() {
  items_.clear();
  RebuildItems();
}

void AstraAppMenuView::PopulateDefaultItems() {
  items_.clear();

  // New section.
  AddSectionHeader(u"New");
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kNewTab;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kNewWindow;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kNewIncognitoWindow;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kNewWorkspace;
    item.label = GetDefaultItemLabel(item.type);
    items_.push_back(item);
  }

  AddSeparator();

  // Astra section.
  AddSectionHeader(u"Astra");
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kWorkspaces;
    item.label = GetDefaultItemLabel(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kSidebar;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kFocusMode;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    item.is_checked = false;
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kSplitView;
    item.label = GetDefaultItemLabel(item.type);
    item.is_checked = false;
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kCommandPalette;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kScreenshot;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }

  AddSeparator();

  // Tools section.
  AddSectionHeader(u"Tools");
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kHistory;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kDownloads;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kBookmarks;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kFind;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kPrint;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kZoomIn;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kZoomOut;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kZoomReset;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }

  AddSeparator();

  // Bottom section.
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kSettings;
    item.label = GetDefaultItemLabel(item.type);
    item.shortcut = GetDefaultShortcut(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kHelp;
    item.label = GetDefaultItemLabel(item.type);
    items_.push_back(item);
  }
  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kAbout;
    item.label = GetDefaultItemLabel(item.type);
    items_.push_back(item);
  }

  AddSeparator();

  {
    AstraAppMenuItem item;
    item.type = AstraAppMenuItemType::kExit;
    item.label = GetDefaultItemLabel(item.type);
    items_.push_back(item);
  }

  RebuildItems();
}

// -- views::BubbleDialogDelegateView ----------------------------------------

gfx::Size AstraAppMenuView::CalculatePreferredSize() const {
  return gfx::Size(kMenuWidth, 0);
}

void AstraAppMenuView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

void AstraAppMenuView::WindowClosing() {
  if (delegate_)
    delegate_->OnMenuClosed();
}

// -- Private methods --------------------------------------------------------

void AstraAppMenuView::BuildLayout() {
  auto items_container = std::make_unique<views::View>();
  items_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(4, 0), 0));
  items_container_ = AddChildView(std::move(items_container));

  UpdateColors();
}

void AstraAppMenuView::RebuildItems() {
  if (!items_container_)
    return;

  items_container_->RemoveAllChildViews();
  item_views_.clear();

  for (const auto& item : items_) {
    if (item.is_separator) {
      items_container_->AddChildView(std::make_unique<views::Separator>());
      continue;
    }

    if (item.is_section_header) {
      auto header = std::make_unique<views::Label>(item.label);
      header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      header->SetAutoColorReadabilityEnabled(false);
      header->SetFontList(views::Label::GetDefaultFontList().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
      header->SetBorder(
          views::CreateEmptyBorder(gfx::Insets::VH(6, kHorizontalPadding)));
      items_container_->AddChildView(std::move(header));
      continue;
    }

    // Regular menu item with label and shortcut.
    auto item_view = std::make_unique<views::View>();
    item_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        gfx::Insets::VH(0, kHorizontalPadding), kIconTextSpacing));
    item_view->SetPreferredSize(gfx::Size(kMenuWidth, kMenuItemHeight));

    // Icon placeholder.
    auto icon = std::make_unique<views::ImageView>();
    icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
    // TODO(astra): Set proper icons for each menu item.
    item_view->AddChildView(std::move(icon));

    // Label.
    auto label = std::make_unique<views::Label>(item.label);
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetAutoColorReadabilityEnabled(false);
    label->SetElideBehavior(gfx::ELIDE_TAIL);
    auto* label_ptr = item_view->AddChildView(std::move(label));
    static_cast<views::BoxLayout*>(item_view->GetLayoutManager())
        ->SetFlexForView(label_ptr, 1);

    // Shortcut.
    if (!item.shortcut.empty()) {
      auto shortcut = std::make_unique<views::Label>(
          base::UTF8ToUTF16(item.shortcut));
      shortcut->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
      shortcut->SetAutoColorReadabilityEnabled(false);
      shortcut->SetFontList(views::Label::GetDefaultFontList().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
      item_view->AddChildView(std::move(shortcut));
    }

    // Check mark.
    if (item.is_checked) {
      // TODO(astra): Add checkmark indicator.
    }

    // Make the whole item clickable by using a LabelButton approach.
    // For simplicity, we make the item view handle clicks.
    // TODO(astra): Use a proper button or InkDrop for hover/press effects.

    auto* item_ptr = items_container_->AddChildView(std::move(item_view));
    item_views_.push_back(item_ptr);
  }

  if (GetWidget())
    SizeToContents();
}

void AstraAppMenuView::UpdateColors() {
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);
  SkColor secondary_color =
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelSecondaryColor);
  SkColor bg_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_WindowBackground);

  SetBackground(views::CreateSolidBackground(bg_color));

  // Update all item labels.
  for (auto* item_view : item_views_) {
    // First child is icon, second is label, third is shortcut.
    if (item_view->children().size() >= 2) {
      auto* label = static_cast<views::Label*>(item_view->children()[1]);
      label->SetEnabledColor(text_color);
    }
    if (item_view->children().size() >= 3) {
      auto* shortcut = static_cast<views::Label*>(item_view->children()[2]);
      shortcut->SetEnabledColor(secondary_color);
    }
  }
}

// -- Button handlers --------------------------------------------------------

void AstraAppMenuView::OnItemClicked(AstraAppMenuItemType type) {
  if (delegate_)
    delegate_->OnMenuItemSelected(type);

  // Close menu after selection.
  if (GetWidget())
    GetWidget()->Close();
}

}  // namespace astra
