// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Profile menu footer view — bottom section of the profile menu with
// action links: Settings, Help, and Exit.
//
// Visual layout:
//   +-------------------------------+
//   |  [separator]                  |
//   |  Settings          Help  Exit |
//   +-------------------------------+
//
// This is a pure presentation view — all actions delegate upward.
//
// Chromium owner: ProfileMenuView (chrome/browser/ui/views/profiles/)
//   Chromium's profile menu has a "Manage your Google Account" link and
//   a "Customize your Chromebook" link at the bottom.
//
// Patch point: ProfileMenuView::BuildBody() — insert this view as the
//   bottom section of the profile menu.
//
// TODO(astra): Add more actions as needed (e.g., "Sign out", "Guest mode").
//   Chromium's profile menu has many more options in the footer area.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_FOOTER_VIEW_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_FOOTER_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class Separator;
}  // namespace views

namespace astra {

// Footer view for the profile menu.
//
// Contains action buttons that are common to the profile menu:
//   - Settings: opens Astra settings
//   - Help: opens help documentation
//   - Exit / Close: closes the menu or browser (delegate decides)
//
// All actions are delegated to the Delegate interface — the view itself
// does not know how to perform these actions.
class AstraProfileMenuFooterView : public views::View {
 public:
  // Delegate for footer action buttons.
  // Implemented by the controller or browser-level coordinator.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the user clicks "Settings".
    virtual void OnSettingsClicked() = 0;

    // Called when the user clicks "Help".
    virtual void OnHelpClicked() = 0;

    // Called when the user clicks "Manage workspaces" link.
    // TODO(astra): Move this to the workspace section or make it
    //   a separate view. For now it lives in the footer as a convenience.
    virtual void OnManageWorkspacesClicked() = 0;

    // Called when the user clicks "Exit" or "Close".
    virtual void OnExitClicked() = 0;
  };

  explicit AstraProfileMenuFooterView(Delegate* delegate);
  ~AstraProfileMenuFooterView() override;

  AstraProfileMenuFooterView(const AstraProfileMenuFooterView&) = delete;
  AstraProfileMenuFooterView& operator=(const AstraProfileMenuFooterView&) =
      delete;

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  // Button click handlers — forward to delegate.
  void OnSettingsButtonClicked();
  void OnHelpButtonClicked();
  void OnManageWorkspacesClicked();
  void OnExitButtonClicked();

  raw_ptr<Delegate> delegate_;

  // Child views.
  raw_ptr<views::Separator> separator_ = nullptr;
  raw_ptr<views::LabelButton> manage_workspaces_button_ = nullptr;
  raw_ptr<views::LabelButton> settings_button_ = nullptr;
  raw_ptr<views::LabelButton> help_button_ = nullptr;
  raw_ptr<views::LabelButton> exit_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_FOOTER_VIEW_H_
