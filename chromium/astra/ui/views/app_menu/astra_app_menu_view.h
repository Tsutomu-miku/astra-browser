// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_APP_MENU_ASTRA_APP_MENU_VIEW_H_
#define ASTRA_UI_VIEWS_APP_MENU_ASTRA_APP_MENU_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
class LabelButton;
class Separator;
}  // namespace views

namespace astra {

// App menu item type.
enum class AstraAppMenuItemType {
  kNewTab,
  kNewWindow,
  kNewIncognitoWindow,
  kNewWorkspace,
  kHistory,
  kDownloads,
  kBookmarks,
  kZoomIn,
  kZoomOut,
  kZoomReset,
  kPrint,
  kFind,
  kMoreTools,
  kSettings,
  kHelp,
  kAbout,
  kExit,
  kFocusMode,       // Astra-specific
  kSplitView,       // Astra-specific
  kCommandPalette,  // Astra-specific
  kWorkspaces,      // Astra-specific
  kSidebar,         // Astra-specific
  kScreenshot,      // Astra-specific
};

// App menu item.
struct AstraAppMenuItem {
  AstraAppMenuItemType type;
  std::u16string label;
  std::string shortcut;
  std::string icon_name;
  bool enabled = true;
  bool is_separator = false;
  bool is_section_header = false;
  bool is_submenu = false;
  bool is_checked = false;
};

// =========================================================================
// AstraAppMenuButton — toolbar app menu button
// =========================================================================
//
// The three-dot menu button in the toolbar.  Shows the app menu bubble
// when clicked.
//
// TODO(astra): Integrate with BrowserView toolbar via a patch.
//   Chromium owner: AppMenuButton (chrome/browser/ui/views/toolbar/)
// =========================================================================

class AstraAppMenuButton : public views::ImageButton {
 public:
  AstraAppMenuButton();
  ~AstraAppMenuButton() override;

  AstraAppMenuButton(const AstraAppMenuButton&) = delete;
  AstraAppMenuButton& operator=(const AstraAppMenuButton&) = delete;

  // Show or hide the app menu.
  void ShowMenu();
  void HideMenu();
  bool IsMenuShowing() const;

  // -- views::ImageButton ---------------------------------------------------

  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Handle button press.
  void HandleButtonPress();

  // Bubble reference (not owned).
  raw_ptr<views::BubbleDialogDelegateView> menu_bubble_ = nullptr;

  static constexpr int kButtonSize = 28;
};

// =========================================================================
// AstraAppMenuView — app menu (three-dot menu) bubble
// =========================================================================
//
// The main application menu that appears when clicking the three-dot
// button in the toolbar.  Contains standard browser commands plus
// Astra-specific features like Focus Mode, Split View, and Workspaces.
//
// Layout (vertical sections):
//   - Top section: New tab, New window, New incognito
//   - Astra section: Workspaces, Focus mode, Split view, Sidebar
//   - Tools section: History, Downloads, Bookmarks, Zoom, Print, Find
//   - More section: Settings, Help, About, Exit
//
// Chromium pattern: AppMenu / AppMenuModel
//   (chrome/browser/ui/menu/app_menu_model.h)
//
// TODO(astra): Integrate with BrowserView toolbar via a patch.
//   Chromium owner: AppMenu (chrome/browser/ui/views/app_menu_button.h)
//   Patch point: ToolbarView — replace or augment app menu button
// =========================================================================

class AstraAppMenuView : public views::BubbleDialogDelegateView {
 public:
  // Delegate for app menu actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when a menu item is selected.
    virtual void OnMenuItemSelected(AstraAppMenuItemType type) = 0;

    // Called when the menu is closed.
    virtual void OnMenuClosed() = 0;
  };

  explicit AstraAppMenuView(views::View* anchor_view);
  ~AstraAppMenuView() override;

  AstraAppMenuView(const AstraAppMenuView&) = delete;
  AstraAppMenuView& operator=(const AstraAppMenuView&) = delete;

  // -- Menu items -----------------------------------------------------------

  // Add a menu item.
  void AddMenuItem(const AstraAppMenuItem& item);

  // Add a separator.
  void AddSeparator();

  // Add a section header.
  void AddSectionHeader(const std::u16string& title);

  // Clear all items.
  void ClearItems();

  // Populate with default menu items.
  void PopulateDefaultItems();

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::BubbleDialogDelegateView --------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void WindowClosing() override;

 private:
  // Build the menu layout.
  void BuildLayout();

  // Rebuild all menu items.
  void RebuildItems();

  // Update colors from theme.
  void UpdateColors();

  // Handle menu item click.
  void OnItemClicked(AstraAppMenuItemType type);

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Menu items data.
  std::vector<AstraAppMenuItem> items_;

  // Child views.
  raw_ptr<views::View> items_container_ = nullptr;
  std::vector<raw_ptr<views::View>> item_views_;

  // -- Constants ------------------------------------------------------------

  static constexpr int kMenuWidth = 260;
  static constexpr int kMenuItemHeight = 32;
  static constexpr int kSectionHeaderHeight = 24;
  static constexpr int kIconSize = 16;
  static constexpr int kHorizontalPadding = 12;
  static constexpr int kIconTextSpacing = 10;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_APP_MENU_ASTRA_APP_MENU_VIEW_H_
