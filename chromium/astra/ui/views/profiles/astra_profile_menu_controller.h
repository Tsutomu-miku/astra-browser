// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_CONTROLLER_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/memory/weak_ptr.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/browser/astra_theme_service.h"
#include "astra/ui/views/profiles/astra_profile_menu_header_view.h"
#include "astra/ui/views/profiles/astra_profile_menu_footer_view.h"
#include "astra/ui/views/profiles/astra_profile_menu_model.h"
#include "astra/ui/views/profiles/astra_profile_menu_workspaces.h"
#include "astra/ui/views/profiles/astra_workspace_avatar_button.h"
#include "ui/views/widget/widget_observer.h"

class Browser;
class PrefService;
class Profile;

namespace views {
class Widget;
class View;
class BubbleDialogDelegateView;
}  // namespace views

namespace astra {

class AstraProfileMenuHeaderView;
class AstraProfileMenuFooterView;
class AstraProfileMenuModel;

// =========================================================================
// Profile menu controller
// =========================================================================
//
// AstraProfileMenuController manages the profile menu and workspace
// integration.  It is the bridge between Astra services (truth sources),
// the model (state/logic/presentation), and the UI views.
//
// MVC architecture:
//
//   Model:  AstraProfileMenuModel — owns state, settings, persistence
//   View:   HeaderView, WorkspacesView, FooterView, AvatarButton
//   Controller: this class — bridges model and views, handles actions
//
//   PrefService <--> Model <--> Controller <--> Views
//   (persistence)    (state)    (bridge)    (presentation)
//
// Responsibilities:
//   - Creates and owns the profile menu model (AstraProfileMenuModel).
//   - Creates and owns the profile menu views (header, workspaces, footer).
//   - Shows/hides the profile menu bubble/widget.
//   - Observes AstraWorkspaceService for workspace changes.
//   - Observes AstraThemeService for accent color and theme changes.
//   - Observes the model and updates views accordingly.
//   - Updates the model when user actions happen.
//   - Handles user actions by delegating upward to the Delegate.
//   - Notifies observers of menu open/close events.
//   - Manages persistence via the model + PrefService.
//
// Implements:
//   - AstraProfileMenuModelObserver: reacts to model state changes.
//   - AstraWorkspaceServiceObserver: reacts to workspace state changes.
//   - AstraThemeServiceObserver: reacts to theme/accent color changes.
//   - AstraProfileMenuWorkspaces::Delegate: handles menu user actions.
//   - AstraWorkspaceAvatarButton::Delegate: handles avatar button clicks.
//   - AstraProfileMenuHeaderView::Delegate: handles header clicks.
//   - AstraProfileMenuFooterView::Delegate: handles footer action clicks.
//   - views::WidgetObserver: tracks menu widget lifecycle.
//
// Chromium owner: ProfileMenuViewController
//   (chrome/browser/ui/views/profiles/profile_menu_view_controller.h)
//   This controller mirrors the pattern of ProfileMenuViewController but
//   extends it with Astra-specific workspace functionality.
//
// Patch point: ProfileMenuView::BuildBody() — insert the workspace section
//   by calling into this controller's GetWorkspacesView().
//   Or replace the entire profile menu with this controller's content view.
// =========================================================================

class AstraProfileMenuController
    : public AstraProfileMenuModelObserver,
      public AstraWorkspaceServiceObserver,
      public AstraThemeServiceObserver,
      public AstraProfileMenuWorkspaces::Delegate,
      public AstraWorkspaceAvatarButton::Delegate,
      public AstraProfileMenuHeaderView::Delegate,
      public AstraProfileMenuFooterView::Delegate,
      public views::WidgetObserver {
 public:
  // Observer interface for menu open/close events.
  //
  // Other UI surfaces or controllers can observe the profile menu to react
  // to its visibility state (e.g., pause animations when menu opens).
  class Observer : public base::CheckedObserver {
   public:
    // Called when the profile menu is about to be shown.
    virtual void OnProfileMenuWillShow() {}

    // Called when the profile menu has been shown and is visible.
    virtual void OnProfileMenuShown() {}

    // Called when the profile menu is about to be hidden.
    virtual void OnProfileMenuWillHide() {}

    // Called when the profile menu has been hidden / closed.
    virtual void OnProfileMenuHidden() {}

   protected:
    ~Observer() override = default;
  };

  // Delegate interface for browser-level actions.
  //
  // The controller delegates all "real" actions (opening settings,
  // signing out, closing the browser, etc.) to this delegate.
  // The controller itself does not own any browser-level logic — it is
  // purely a UI coordinator.
  //
  // Typically implemented by Browser or an AstraBrowserView-level coordinator.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the user clicks "Settings" in the profile menu footer.
    // The delegate should open the settings page / dialog.
    virtual void OnOpenSettings() = 0;

    // Called when the user clicks "Help" in the profile menu footer.
    virtual void OnOpenHelp() = 0;

    // Called when the user clicks "Manage workspaces" link.
    // The delegate should open workspace management UI or navigate to it.
    virtual void OnManageWorkspaces() = 0;

    // Called when the user clicks "Exit" / "Close" in the footer.
    // The delegate should close the menu or the browser (delegate decides).
    virtual void OnExitClicked() = 0;

    // Called when the user clicks the profile header (avatar/name).
    // Typically opens profile settings or account management.
    virtual void OnProfileHeaderClicked() = 0;

    // Called when a workspace is selected / activated.
    // The delegate may want to close the menu, log metrics, etc.
    // The actual workspace switching is done by the controller via
    // AstraWorkspaceService — this is a notification, not a request.
    virtual void OnWorkspaceSwitched(const std::string& workspace_id) {}
  };

  explicit AstraProfileMenuController(Browser* browser);
  ~AstraProfileMenuController() override;

  AstraProfileMenuController(const AstraProfileMenuController&) = delete;
  AstraProfileMenuController& operator=(const AstraProfileMenuController&) =
      delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Delegate -----------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() const { return delegate_; }

  // -- Model access -------------------------------------------------------

  // Returns the profile menu model.  The model owns all menu state
  // and presentation settings.
  AstraProfileMenuModel* model() { return model_.get(); }
  const AstraProfileMenuModel* model() const { return model_.get(); }

  // -- Menu show/hide -----------------------------------------------------

  // Shows the profile menu as a bubble anchored to |anchor_view|.
  // If the menu is already open, it is re-anchored and focused.
  void ShowProfileMenu(views::View* anchor_view);

  // Hides the profile menu if it's open.
  void HideProfileMenu();

  // Returns true if the profile menu bubble / widget is currently shown.
  bool IsProfileMenuShowing() const;

  // -- Avatar button ------------------------------------------------------

  // Creates and returns the workspace avatar button.
  // The caller takes ownership of the returned view.
  // The controller observes the button via the Delegate interface and
  // updates it when workspace or theme state changes.
  std::unique_ptr<AstraWorkspaceAvatarButton> CreateAvatarButton();

  // Returns the avatar button if one has been created and is still alive,
  // otherwise nullptr.
  AstraWorkspaceAvatarButton* avatar_button() { return avatar_button_; }

  // -- Menu content views -------------------------------------------------

  // Returns the workspace section view, creating it if needed.
  // Owned by the controller.
  AstraProfileMenuWorkspaces* GetWorkspacesView();

  // Returns the profile menu header view, creating it if needed.
  // Owned by the controller.
  AstraProfileMenuHeaderView* GetHeaderView();

  // Returns the profile menu footer view, creating it if needed.
  // Owned by the controller.
  AstraProfileMenuFooterView* GetFooterView();

  // -- Persistence --------------------------------------------------------

  // Loads presentation settings from the profile's PrefService.
  // Call this after construction when a profile is available.
  void LoadFromPrefs();

  // Saves presentation settings to the profile's PrefService.
  // Call this before destruction or when settings change.
  void SaveToPrefs();

  // -- AstraProfileMenuModelObserver --------------------------------------

  void OnProfileMenuOpened() override;
  void OnProfileMenuClosed() override;
  void OnProfileSelected(int profile_index) override;
  void OnWorkspaceSelected(const std::string& workspace_id) override;
  void OnMenuSettingsChanged() override;
  void OnWorkspacesChanged() override;
  void OnActiveWorkspaceChanged(const std::string& workspace_id) override;
  void OnSyncStatusChanged(AstraSyncStatus status) override;

  // -- AstraWorkspaceServiceObserver --------------------------------------

  void OnWorkspaceAdded(const AstraWorkspace& workspace) override;
  void OnWorkspaceRemoved(const std::string& workspace_id) override;
  void OnWorkspaceRenamed(const std::string& workspace_id,
                          const std::string& new_name) override;
  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override;
  void OnWorkspacesReordered() override;

  // -- AstraThemeServiceObserver ------------------------------------------

  void OnAccentColorChanged(SkColor new_accent_color) override;
  void OnThemeChanged() override;

  // -- AstraProfileMenuWorkspaces::Delegate -------------------------------

  void OnWorkspaceSelected(const std::string& workspace_id) override;
  void OnNewWorkspace() override;
  void OnManageWorkspaces() override;
  void OnWorkspaceReordered(const std::string& workspace_id,
                            int direction) override;

  // -- AstraWorkspaceAvatarButton::Delegate -------------------------------

  void OnWorkspaceAvatarButtonClicked(
      AstraWorkspaceAvatarButton* button) override;

  // -- AstraProfileMenuHeaderView::Delegate -------------------------------

  void OnProfileHeaderClicked() override;
  void OnSyncStatusClicked() override;

  // -- AstraProfileMenuFooterView::Delegate -------------------------------

  void OnSettingsClicked() override;
  void OnHelpClicked() override;
  void OnManageWorkspacesClicked() override;
  void OnExitClicked() override;

  // -- views::WidgetObserver ----------------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

 private:
  // Returns the workspace service for the browser's profile.
  AstraWorkspaceService* GetWorkspaceService();

  // Returns the theme service for the browser's profile.
  AstraThemeService* GetThemeService();

  // Returns the profile from the browser.
  Profile* GetProfile();

  // Returns the pref service for the profile.
  PrefService* GetPrefService();

  // Updates all UI (menu views and avatar button) from services and model.
  void UpdateAllFromServices();

  // Updates just the avatar button from services.
  void UpdateAvatarButtonFromServices();

  // Updates the profile header info from the profile.
  void UpdateHeaderFromProfile();

  // Updates the workspaces view from the model.
  void UpdateWorkspacesViewFromModel();

  // Updates views to reflect current model settings.
  void ApplyModelSettingsToViews();

  // Builds the full menu content view (header + workspaces + footer).
  // Ownership is transferred to the caller.
  std::unique_ptr<views::View> BuildMenuContentView();

  // Syncs workspace data from the service to the model.
  void SyncWorkspacesToModel();

  // Notifies observers that the menu is about to show.
  void NotifyMenuWillShow();

  // Notifies observers that the menu has been shown.
  void NotifyMenuShown();

  // Notifies observers that the menu is about to hide.
  void NotifyMenuWillHide();

  // Notifies observers that the menu has been hidden.
  void NotifyMenuHidden();

  raw_ptr<Browser> browser_;
  raw_ptr<Profile> profile_;

  // Delegate for browser-level actions. Not owned.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Observers of menu state. Not owned.
  base::ObserverList<Observer> observers_;

  // The model — owns all menu state and presentation settings.
  std::unique_ptr<AstraProfileMenuModel> model_;

  // Menu content views — owned by the controller or by the widget hierarchy.
  std::unique_ptr<AstraProfileMenuHeaderView> header_view_;
  std::unique_ptr<AstraProfileMenuWorkspaces> workspaces_view_;
  std::unique_ptr<AstraProfileMenuFooterView> footer_view_;

  // The menu bubble widget.  Null when the menu is closed.
  raw_ptr<views::Widget> menu_widget_ = nullptr;

  // The bubble delegate associated with the menu widget.
  raw_ptr<views::BubbleDialogDelegateView> bubble_delegate_ = nullptr;

  // Weak reference to the avatar button.  Owned by its parent view.
  raw_ptr<AstraWorkspaceAvatarButton> avatar_button_ = nullptr;

  // Tracks whether we're currently observing the workspace service.
  bool observing_workspace_service_ = false;

  // Tracks whether we're currently observing the theme service.
  bool observing_theme_service_ = false;

  // Workspace counter for generating new workspace IDs.
  // TODO(astra): Replace with a proper ID generation strategy.
  int workspace_counter_ = 0;

  base::WeakPtrFactory<AstraProfileMenuController> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_CONTROLLER_H_
