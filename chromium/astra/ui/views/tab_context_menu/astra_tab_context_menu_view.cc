// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_context_menu/astra_tab_context_menu_view.h"

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
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Get the default label for a menu item type.
std::u16string GetDefaultItemLabel(AstraTabContextMenuItemType type) {
  switch (type) {
    case AstraTabContextMenuItemType::kNewTab:
      return u"New tab";
    case AstraTabContextMenuItemType::kNewTabToRight:
      return u"New tab to the right";
    case AstraTabContextMenuItemType::kReload:
      return u"Reload";
    case AstraTabContextMenuItemType::kDuplicate:
      return u"Duplicate";
    case AstraTabContextMenuItemType::kPinTab:
      return u"Pin";
    case AstraTabContextMenuItemType::kUnpinTab:
      return u"Unpin";
    case AstraTabContextMenuItemType::kMuteTab:
      return u"Mute site";
    case AstraTabContextMenuItemType::kUnmuteTab:
      return u"Unmute site";
    case AstraTabContextMenuItemType::kCloseTab:
      return u"Close";
    case AstraTabContextMenuItemType::kCloseOtherTabs:
      return u"Close other tabs";
    case AstraTabContextMenuItemType::kCloseTabsToRight:
      return u"Close tabs to the right";
    case AstraTabContextMenuItemType::kCloseTabsToLeft:
      return u"Close tabs to the left";
    case AstraTabContextMenuItemType::kReopenClosedTab:
      return u"Reopen closed tab";
    case AstraTabContextMenuItemType::kMoveToNewWindow:
      return u"Move to new window";
    case AstraTabContextMenuItemType::kAddToBookmarks:
      return u"Add to bookmarks";
    case AstraTabContextMenuItemType::kMoveToWorkspace:
      return u"Move to workspace";
    case AstraTabContextMenuItemType::kAddToFavorites:
      return u"Add to favorites";
    case AstraTabContextMenuItemType::kSendToDevice:
      return u"Send to your devices";
    case AstraTabContextMenuItemType::kCopyURL:
      return u"Copy URL";
    case AstraTabContextMenuItemType::kPrint:
      return u"Print";
    case AstraTabContextMenuItemType::kBookmarkAllTabs:
      return u"Bookmark all tabs";
  }
  return std::u16string();
}

// Get the icon name for a menu item type.
const char* GetDefaultIconName(AstraTabContextMenuItemType type) {
  switch (type) {
    case AstraTabContextMenuItemType::kNewTab:
      return "new_tab";
    case AstraTabContextMenuItemType::kNewTabToRight:
      return "tab_plus_right";
    case AstraTabContextMenuItemType::kReload:
      return "refresh";
    case AstraTabContextMenuItemType::kDuplicate:
      return "content_copy";
    case AstraTabContextMenuItemType::kPinTab:
    case AstraTabContextMenuItemType::kUnpinTab:
      return "push_pin";
    case AstraTabContextMenuItemType::kMuteTab:
    case AstraTabContextMenuItemType::kUnmuteTab:
      return "volume_up";
    case AstraTabContextMenuItemType::kCloseTab:
      return "close";
    case AstraTabContextMenuItemType::kCloseOtherTabs:
      return "close_others";
    case AstraTabContextMenuItemType::kCloseTabsToRight:
      return "close_right";
    case AstraTabContextMenuItemType::kCloseTabsToLeft:
      return "close_left";
    case AstraTabContextMenuItemType::kReopenClosedTab:
      return "restore";
    case AstraTabContextMenuItemType::kMoveToNewWindow:
      return "open_in_new";
    case AstraTabContextMenuItemType::kAddToBookmarks:
      return "star";
    case AstraTabContextMenuItemType::kMoveToWorkspace:
      return "workspaces";
    case AstraTabContextMenuItemType::kAddToFavorites:
      return "favorite";
    case AstraTabContextMenuItemType::kSendToDevice:
      return "devices";
    case AstraTabContextMenuItemType::kCopyURL:
      return "link";
    case AstraTabContextMenuItemType::kPrint:
      return "print";
    case AstraTabContextMenuItemType::kBookmarkAllTabs:
      return "star_multiple";
  }
  return "";
}

// Draw a simple generic menu icon (colored circle with first letter).
void DrawMenuIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color,
                  const std::u16string& label) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int radius = std::min(bounds.width(), bounds.height()) / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx, cy), radius, flags);
}

}  // namespace

// =========================================================================
// AstraTabContextMenuView
// =========================================================================

AstraTabContextMenuView::AstraTabContextMenuView(views::View* anchor_view,
                                                 const TabInfo& tab_info)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT),
      tab_info_(tab_info) {
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  BuildLayout();
}

AstraTabContextMenuView::~AstraTabContextMenuView() = default;

// -- Menu items -------------------------------------------------------------

void AstraTabContextMenuView::AddMenuItem(
    const AstraTabContextMenuItem& item) {
  items_.push_back(item);
  RebuildItems();
}

void AstraTabContextMenuView::AddSeparator() {
  AstraTabContextMenuItem item;
  item.is_separator = true;
  items_.push_back(item);
  RebuildItems();
}

void AstraTabContextMenuView::AddSectionHeader(const std::u16string& title) {
  AstraTabContextMenuItem item;
  item.label = title;
  item.is_header = true;
  items_.push_back(item);
  RebuildItems();
}

void AstraTabContextMenuView::ClearItems() {
  items_.clear();
  RebuildItems();
}

// -- Tab info ---------------------------------------------------------------

void AstraTabContextMenuView::SetTabInfo(const TabInfo& tab_info) {
  tab_info_ = tab_info;
  if (title_label_)
    title_label_->SetText(tab_info_.title);
  if (url_label_)
    url_label_->SetText(base::UTF8ToUTF16(tab_info_.url.spec()));
  SchedulePaint();
}

// -- views::BubbleDialogDelegateView ----------------------------------------

gfx::Size AstraTabContextMenuView::CalculatePreferredSize() const {
  return gfx::Size(kMenuWidth, 0);
}

void AstraTabContextMenuView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

void AstraTabContextMenuView::WindowClosing() {
  if (delegate_)
    delegate_->OnMenuClosed();
}

// -- Private methods --------------------------------------------------------

void AstraTabContextMenuView::BuildLayout() {
  auto* container = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));

  // Header.
  BuildHeader();

  // Separator after header.
  AddChildView(std::make_unique<views::Separator>());

  // Items container.
  auto items_container = std::make_unique<views::View>();
  items_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  items_container_ = AddChildView(std::move(items_container));

  UpdateColors();
  RebuildItems();
}

void AstraTabContextMenuView::BuildHeader() {
  auto header = std::make_unique<views::View>();
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, kHorizontalPadding), kIconTextSpacing));
  header_view_ = AddChildView(std::move(header));

  // Favicon.
  auto favicon = std::make_unique<views::ImageView>();
  favicon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  favicon_view_ = header_view_->AddChildView(std::move(favicon));

  // Title + URL container.
  auto title_container = std::make_unique<views::View>();
  title_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 2));
  title_container_ = header_view_->AddChildView(std::move(title_container));
  static_cast<views::BoxLayout*>(
      header_view_->GetLayoutManager())->SetFlexForView(title_container_, 1);

  // Title label.
  auto title_label = std::make_unique<views::Label>(tab_info_.title);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  title_label_ = title_container_->AddChildView(std::move(title_label));

  // URL label.
  std::u16string url_text = base::UTF8ToUTF16(tab_info_.url.spec());
  auto url_label = std::make_unique<views::Label>(url_text);
  url_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label->SetAutoColorReadabilityEnabled(false);
  url_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  url_label_ = title_container_->AddChildView(std::move(url_label));
}

void AstraTabContextMenuView::RebuildItems() {
  if (!items_container_)
    return;

  items_container_->RemoveAllChildViews();
  item_views_.clear();

  for (const auto& item : items_) {
    if (item.is_separator) {
      items_container_->AddChildView(std::make_unique<views::Separator>());
      continue;
    }

    if (item.is_header) {
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

    // Regular menu item.
    auto* button = items_container_->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(&AstraTabContextMenuView::OnItemClicked,
                                base::Unretained(this), item.type),
            item.label));
    button->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    button->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(0, kHorizontalPadding)));
    button->SetSize(gfx::Size(kMenuWidth, kMenuItemHeight));
    button->SetEnabled(item.enabled);

    if (item.is_dangerous) {
      // TODO(astra): Set dangerous text color.
    }

    item_views_.push_back(button);
  }

  if (GetWidget())
    SizeToContents();
}

void AstraTabContextMenuView::UpdateColors() {
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

  if (title_label_)
    title_label_->SetEnabledColor(text_color);
  if (url_label_)
    url_label_->SetEnabledColor(secondary_color);

  // Draw favicon placeholder.
  if (favicon_view_) {
    gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), /*image_scale=*/1.0f,
                       false);
    DrawMenuIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize),
                 SkColorSetRGB(0x5C, 0x6B, 0xC0), u"");
    favicon_view_->SetImage(
        gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(kIconSize, kIconSize)));
  }
}

// -- Button handlers --------------------------------------------------------

void AstraTabContextMenuView::OnItemClicked(AstraTabContextMenuItemType type) {
  if (delegate_)
    delegate_->OnMenuItemSelected(type);

  // Close the menu after selection.
  if (GetWidget())
    GetWidget()->Close();
}

}  // namespace astra
