// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_CONTEXT_MENU_ASTRA_TAB_CONTEXT_MENU_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_CONTEXT_MENU_ASTRA_TAB_CONTEXT_MENU_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
class LabelButton;
class Separator;
}  // namespace views

namespace astra {

// Tab context menu item type.
enum class AstraTabContextMenuItemType {
  kNewTab,
  kNewTabToRight,
  kReload,
  kDuplicate,
  kPinTab,
  kUnpinTab,
  kMuteTab,
  kUnmuteTab,
  kCloseTab,
  kCloseOtherTabs,
  kCloseTabsToRight,
  kCloseTabsToLeft,
  kReopenClosedTab,
  kMoveToNewWindow,
  kAddToBookmarks,
  kMoveToWorkspace,  // Astra-specific
  kAddToFavorites,    // Astra-specific
  kSendToDevice,
  kCopyURL,
  kPrint,
  kBookmarkAllTabs,
};

// Tab context menu item.
struct AstraTabContextMenuItem {
  AstraTabContextMenuItemType type;
  std::u16string label;
  std::string icon_name;
  bool enabled = true;
  bool is_header = false;
  bool is_separator = false;
  bool is_dangerous = false;
};

// =========================================================================
// AstraTabContextMenuView — tab right-click context menu
// =========================================================================
//
// A context menu that appears when right-clicking a tab.  Includes standard
// tab operations plus Astra-specific features like "Move to workspace"
// and "Add to favorites".
//
// Layout (vertical list):
//   - Tab info header (title, URL, favicon)
//   - Separator
//   - Menu items (with icons and keyboard shortcuts)
//   - Separator
//   - Astra section (workspace, favorites)
//
// Chromium pattern: TabContextMenu / TabMenuModel
//   (chrome/browser/ui/tabs/tab_menu_model.h)
//
// TODO(astra): Integrate with TabStrip via a patch.
//   Chromium owner: TabContextMenuContents
//   (chrome/browser/ui/views/tabs/tab_context_menu_contents.h)
//   Patch point: Tab::OnMousePressed() — show Astra context menu instead
// =========================================================================

class AstraTabContextMenuView : public views::BubbleDialogDelegateView {
 public:
  // Delegate for context menu actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when a menu item is selected.
    virtual void OnMenuItemSelected(AstraTabContextMenuItemType type) = 0;

    // Called when "Move to workspace" submenu item is selected.
    virtual void OnMoveToWorkspace(int workspace_id) = 0;

    // Called to get the list of workspaces for the submenu.
    virtual std::vector<std::u16string> GetWorkspaceList() = 0;

    // Called when the menu is closed.
    virtual void OnMenuClosed() = 0;
  };

  // Tab info for the menu header.
  struct TabInfo {
    std::u16string title;
    GURL url;
    bool is_pinned = false;
    bool is_muted = false;
    bool is_audible = false;
    bool is_discarded = false;
  };

  explicit AstraTabContextMenuView(views::View* anchor_view,
                                   const TabInfo& tab_info);
  ~AstraTabContextMenuView() override;

  AstraTabContextMenuView(const AstraTabContextMenuView&) = delete;
  AstraTabContextMenuView& operator=(const AstraTabContextMenuView&) = delete;

  // -- Menu items -----------------------------------------------------------

  // Add a menu item.
  void AddMenuItem(const AstraTabContextMenuItem& item);

  // Add a separator.
  void AddSeparator();

  // Add a section header.
  void AddSectionHeader(const std::u16string& title);

  // Clear all items.
  void ClearItems();

  // -- Tab info -------------------------------------------------------------

  void SetTabInfo(const TabInfo& tab_info);
  const TabInfo& tab_info() const { return tab_info_; }

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::BubbleDialogDelegateView --------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void WindowClosing() override;

 private:
  // Build the menu layout.
  void BuildLayout();

  // Build the tab info header.
  void BuildHeader();

  // Rebuild all menu items.
  void RebuildItems();

  // Update colors from theme.
  void UpdateColors();

  // Handle menu item click.
  void OnItemClicked(AstraTabContextMenuItemType type);

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Tab info.
  TabInfo tab_info_;

  // Menu items data.
  std::vector<AstraTabContextMenuItem> items_;

  // Child views.
  raw_ptr<views::View> header_view_ = nullptr;
  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::View> title_container_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  std::vector<raw_ptr<views::View>> item_views_;

  // -- Constants ------------------------------------------------------------

  static constexpr int kMenuWidth = 240;
  static constexpr int kMenuItemHeight = 32;
  static constexpr int kHeaderHeight = 48;
  static constexpr int kIconSize = 16;
  static constexpr int kHorizontalPadding = 12;
  static constexpr int kIconTextSpacing = 10;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_CONTEXT_MENU_ASTRA_TAB_CONTEXT_MENU_VIEW_H_
