// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_profile_menu_controller.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/prefs/pref_service.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_workspace_service.h"
#include "astra/browser/astra_workspace_service_factory.h"
#include "astra/browser/astra_theme_service.h"
#include "astra/browser/astra_theme_service_factory.h"
#include "astra/ui/views/profiles/astra_profile_menu_header_view.h"
#include "astra/ui/views/profiles/astra_profile_menu_footer_view.h"
#include "astra/ui/views/profiles/astra_profile_menu_model.h"
#include "astra/ui/views/profiles/astra_profile_menu_workspaces.h"
#include "astra/ui/views/profiles/astra_workspace_avatar_button.h"
#include "astra/ui/color/astra_color_ids.h"

namespace astra {

namespace {

// Default menu bubble width.
constexpr int kMenuBubbleMinWidth = 280;

// Helper: parse hex color string to SkColor.
// TODO(astra): Use a proper color parsing utility.
SkColor HexToSkColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') {
    return SK_ColorBLUE;
  }
  std::string cleaned = hex.substr(1);
  if (cleaned.size() != 6) {
    return SK_ColorBLUE;
  }
  unsigned int r, g, b;
  if (sscanf(cleaned.c_str(), "%02x%02x%02x", &r, &g, &b) != 3) {
    return SK_ColorBLUE;
  }
  return SkColorSetRGB(r, g, b);
}

// Convert model display mode to workspaces view display mode.
AstraWorkspaceDisplayMode ModelToViewDisplayMode(
    AstraWorkspaceDisplayMode mode) {
  switch (mode) {
    case AstraWorkspaceDisplayMode::kIconsOnly:
      return AstraWorkspaceDisplayMode::kIconsOnly;
    case AstraWorkspaceDisplayMode::kNamesOnly:
      return AstraWorkspaceDisplayMode::kNamesOnly;
    case AstraWorkspaceDisplayMode::kIconsAndNames:
      return AstraWorkspaceDisplayMode::kIconsAndNames;
  }
  return AstraWorkspaceDisplayMode::kIconsAndNames;
}

// Convert model menu position to bubble anchor position.
views::BubbleBorder::Arrow PositionToArrow(
    AstraProfileMenuPosition position) {
  switch (position) {
    case AstraProfileMenuPosition::kLeft:
      return views::BubbleBorder::TOP_LEFT;
    case AstraProfileMenuPosition::kRight:
      return views::BubbleBorder::TOP_RIGHT;
  }
  return views::BubbleBorder::TOP_RIGHT;
}

// Convert header sync status to model sync status.
AstraHeaderSyncStatus ModelToHeaderSyncStatus(AstraSyncStatus status) {
  switch (status) {
    case AstraSyncStatus::kNotSignedIn:
      return AstraHeaderSyncStatus::kNotSignedIn;
    case AstraSyncStatus::kSyncing:
      return AstraHeaderSyncStatus::kSyncing;
    case AstraSyncStatus::kSynced:
      return AstraHeaderSyncStatus::kSynced;
    case AstraSyncStatus::kError:
      return AstraHeaderSyncStatus::kError;
    case AstraSyncStatus::kPaused:
      return AstraHeaderSyncStatus::kPaused;
  }
  return AstraHeaderSyncStatus::kNotSignedIn;
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraProfileMenuController
// ---------------------------------------------------------------------------

AstraProfileMenuController::AstraProfileMenuController(Browser* browser)
    : browser_(browser), profile_(browser ? browser->profile() : nullptr) {
  DCHECK(browser_);
  DCHECK(profile_);

  // Create the model.
  model_ = std::make_unique<AstraProfileMenuModel>();

  // Observe the model for state changes.
  model_->AddObserver(this);

  // Load settings from prefs if available.
  LoadFromPrefs();

  // Sync workspace data from the service to the model.
  SyncWorkspacesToModel();

  // Start observing the workspace service for changes.
  AstraWorkspaceService* ws_service = GetWorkspaceService();
  if (ws_service) {
    ws_service->AddObserver(this);
    observing_workspace_service_ = true;
  }

  // Start observing the theme service for changes.
  AstraThemeService* theme_service = GetThemeService();
  if (theme_service) {
    theme_service->AddObserver(this);
    observing_theme_service_ = true;
  }
}

AstraProfileMenuController::~AstraProfileMenuController() {
  // Save settings to prefs.
  SaveToPrefs();

  // Stop observing the model.
  if (model_) {
    model_->RemoveObserver(this);
  }

  // Stop observing the workspace service.
  if (observing_workspace_service_) {
    AstraWorkspaceService* ws_service = GetWorkspaceService();
    if (ws_service) {
      ws_service->RemoveObserver(this);
    }
    observing_workspace_service_ = false;
  }

  // Stop observing the theme service.
  if (observing_theme_service_) {
    AstraThemeService* theme_service = GetThemeService();
    if (theme_service) {
      theme_service->RemoveObserver(this);
    }
    observing_theme_service_ = false;
  }

  // Stop observing the menu widget.
  if (menu_widget_) {
    menu_widget_->RemoveObserver(this);
    menu_widget_->Close();
    menu_widget_ = nullptr;
  }
}

// ---------------------------------------------------------------------------
// Observer management
// ---------------------------------------------------------------------------

void AstraProfileMenuController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraProfileMenuController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// ---------------------------------------------------------------------------
// Menu show/hide
// ---------------------------------------------------------------------------

void AstraProfileMenuController::ShowProfileMenu(views::View* anchor_view) {
  if (!anchor_view) {
    return;
  }

  // If already showing, just focus it.
  if (menu_widget_) {
    menu_widget_->Show();
    menu_widget_->Activate();
    return;
  }

  // Notify observers that menu is about to show.
  NotifyMenuWillShow();

  // Update model state.
  model_->OpenMenu();

  // Ensure views are up to date before showing.
  UpdateAllFromServices();

  // Build the menu content view.
  auto content_view = BuildMenuContentView();

  // Create a bubble delegate and widget.
  auto bubble = std::make_unique<views::BubbleDialogDelegateView>(
      anchor_view, PositionToArrow(model_->menu_position()),
      views::BubbleBorder::STANDARD_SHADOW);

  bubble->SetButtons(ui::DIALOG_BUTTON_NONE);
  bubble->SetShowCloseButton(false);
  bubble->set_fixed_width(kMenuBubbleMinWidth);
  bubble->SetAnchorView(anchor_view);
  bubble->set_close_on_deactivate(true);
  bubble->set_close_on_esc(true);

  // Add the content view to the bubble.
  bubble->AddChildView(std::move(content_view));

  // Store a raw pointer to the delegate before we release ownership.
  bubble_delegate_ = bubble.get();

  // Create the widget. Ownership transfers to the widget system.
  menu_widget_ =
      views::BubbleDialogDelegateView::CreateBubble(std::move(bubble));

  // Observe the widget for destruction and visibility changes.
  menu_widget_->AddObserver(this);

  // Show the widget.
  menu_widget_->Show();

  // Notify observers that menu has been shown.
  NotifyMenuShown();
}

void AstraProfileMenuController::HideProfileMenu() {
  if (!menu_widget_) {
    return;
  }

  // Notify observers that menu is about to hide.
  NotifyMenuWillHide();

  // Update model state.
  model_->CloseMenu();

  // Remove our observer before closing to avoid reentrancy.
  menu_widget_->RemoveObserver(this);

  // Close the widget.
  menu_widget_->Close();
  menu_widget_ = nullptr;
  bubble_delegate_ = nullptr;

  // Notify observers that menu has been hidden.
  NotifyMenuHidden();
}

bool AstraProfileMenuController::IsProfileMenuShowing() const {
  return menu_widget_ != nullptr;
}

// ---------------------------------------------------------------------------
// Avatar button
// ---------------------------------------------------------------------------

std::unique_ptr<AstraWorkspaceAvatarButton>
AstraProfileMenuController::CreateAvatarButton() {
  auto button = std::make_unique<AstraWorkspaceAvatarButton>(
      GetWorkspaceService(), this);
  avatar_button_ = button.get();

  // Initialize the button state from services and model.
  UpdateAvatarButtonFromServices();

  return button;
}

// ---------------------------------------------------------------------------
// Menu content views
// ---------------------------------------------------------------------------

AstraProfileMenuWorkspaces*
AstraProfileMenuController::GetWorkspacesView() {
  if (!workspaces_view_) {
    workspaces_view_ = std::make_unique<AstraProfileMenuWorkspaces>(
        GetWorkspaceService(), this);
    // Apply model settings to the new view.
    ApplyModelSettingsToViews();
  }
  return workspaces_view_.get();
}

AstraProfileMenuHeaderView*
AstraProfileMenuController::GetHeaderView() {
  if (!header_view_) {
    header_view_ = std::make_unique<AstraProfileMenuHeaderView>(this);
    UpdateHeaderFromProfile();
    // Apply model settings.
    header_view_->SetAvatarVisible(model_->show_avatar());
    header_view_->SetSyncStatusVisible(model_->show_sync_status());
    header_view_->SetSyncStatus(ModelToHeaderSyncStatus(model_->sync_status()));
  }
  return header_view_.get();
}

AstraProfileMenuFooterView*
AstraProfileMenuController::GetFooterView() {
  if (!footer_view_) {
    footer_view_ = std::make_unique<AstraProfileMenuFooterView>(this);
  }
  return footer_view_.get();
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void AstraProfileMenuController::LoadFromPrefs() {
  PrefService* prefs = GetPrefService();
  if (prefs && model_) {
    model_->LoadFromPrefs(prefs);
    ApplyModelSettingsToViews();
  }
}

void AstraProfileMenuController::SaveToPrefs() {
  PrefService* prefs = GetPrefService();
  if (prefs && model_) {
    model_->SaveToPrefs(prefs);
  }
}

// ---------------------------------------------------------------------------
// AstraProfileMenuModelObserver
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnProfileMenuOpened() {
  // Model reports menu opened — views already updated by ShowProfileMenu.
}

void AstraProfileMenuController::OnProfileMenuClosed() {
  // Model reports menu closed — views already updated by HideProfileMenu.
}

void AstraProfileMenuController::OnProfileSelected(int profile_index) {
  // TODO(astra): Handle profile switching in multi-profile menus.
  //   For now we just observe the change.
}

void AstraProfileMenuController::OnWorkspaceSelected(
    const std::string& workspace_id) {
  // Model reports workspace selected — already handled by delegate methods.
}

void AstraProfileMenuController::OnMenuSettingsChanged() {
  // Presentation settings changed — update all views.
  ApplyModelSettingsToViews();
  // Save to prefs.
  SaveToPrefs();
}

void AstraProfileMenuController::OnWorkspacesChanged() {
  // Workspace list in model changed — update workspaces view.
  if (workspaces_view_) {
    workspaces_view_->UpdateFromService();
  }
}

void AstraProfileMenuController::OnActiveWorkspaceChanged(
    const std::string& workspace_id) {
  // Active workspace changed in model — update views.
  if (workspaces_view_) {
    workspaces_view_->UpdateFromService();
  }
  if (avatar_button_) {
    avatar_button_->UpdateFromService();
  }
}

void AstraProfileMenuController::OnSyncStatusChanged(AstraSyncStatus status) {
  // Sync status changed — update header view.
  if (header_view_) {
    header_view_->SetSyncStatus(ModelToHeaderSyncStatus(status));
  }
}

// ---------------------------------------------------------------------------
// AstraWorkspaceServiceObserver
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnWorkspaceAdded(
    const AstraWorkspace& workspace) {
  SyncWorkspacesToModel();
  UpdateAllFromServices();
}

void AstraProfileMenuController::OnWorkspaceRemoved(
    const std::string& workspace_id) {
  SyncWorkspacesToModel();
  UpdateAllFromServices();
}

void AstraProfileMenuController::OnWorkspaceRenamed(
    const std::string& workspace_id,
    const std::string& new_name) {
  SyncWorkspacesToModel();
  UpdateAllFromServices();
}

void AstraProfileMenuController::OnActiveWorkspaceChanged(
    const std::string& old_id,
    const std::string& new_id) {
  // Update model's active workspace.
  model_->SetActiveWorkspaceId(new_id);

  // Update theme service accent color to match the new workspace.
  AstraThemeService* theme_service = GetThemeService();
  if (theme_service) {
    theme_service->SetActiveWorkspace(new_id);
  }

  // Notify delegate of workspace switch.
  if (delegate_) {
    delegate_->OnWorkspaceSwitched(new_id);
  }

  UpdateAllFromServices();
}

void AstraProfileMenuController::OnWorkspacesReordered() {
  SyncWorkspacesToModel();
  UpdateAllFromServices();
}

// ---------------------------------------------------------------------------
// AstraThemeServiceObserver
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnAccentColorChanged(SkColor new_accent_color) {
  // Update the avatar button's accent color.
  if (avatar_button_) {
    avatar_button_->SetAccentColor(new_accent_color);
  }

  // Update the header view's accent color.
  if (header_view_) {
    header_view_->SetAccentColor(new_accent_color);
  }
}

void AstraProfileMenuController::OnThemeChanged() {
  // Views handle their own OnThemeChanged via the View hierarchy.
}

// ---------------------------------------------------------------------------
// AstraProfileMenuWorkspaces::Delegate
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnWorkspaceSelected(
    const std::string& workspace_id) {
  AstraWorkspaceService* service = GetWorkspaceService();
  if (!service) {
    return;
  }

  // Update model state.
  model_->SelectWorkspaceById(workspace_id);

  // Switch the active workspace in the service.
  service->ActivateWorkspace(workspace_id);

  // Close the menu after selection.
  HideProfileMenu();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnWorkspaceSwitched(workspace_id);
  }
}

void AstraProfileMenuController::OnNewWorkspace() {
  AstraWorkspaceService* service = GetWorkspaceService();
  if (!service) {
    return;
  }

  ++workspace_counter_;
  std::string new_id = "workspace-" + base::NumberToString(workspace_counter_);

  AstraWorkspace workspace;
  workspace.id = new_id;
  workspace.name = "Workspace " + base::NumberToString(workspace_counter_);
  workspace.accent_color = "#5AD8A6";
  service->AddWorkspace(std::move(workspace));
  service->ActivateWorkspace(new_id);

  // Update model.
  model_->SetActiveWorkspaceId(new_id);

  // Close the menu after creating the new workspace.
  HideProfileMenu();

  // Notify delegate of workspace switch.
  if (delegate_) {
    delegate_->OnWorkspaceSwitched(new_id);
  }
}

void AstraProfileMenuController::OnManageWorkspaces() {
  if (delegate_) {
    delegate_->OnManageWorkspaces();
  }
  HideProfileMenu();
}

void AstraProfileMenuController::OnWorkspaceReordered(
    const std::string& workspace_id,
    int direction) {
  // TODO(astra): Implement workspace reordering in the service.
  //   For now we just notify the delegate.
  if (delegate_) {
    // The delegate can handle the reorder if needed.
  }
}

// ---------------------------------------------------------------------------
// AstraWorkspaceAvatarButton::Delegate
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnWorkspaceAvatarButtonClicked(
    AstraWorkspaceAvatarButton* button) {
  // Toggle the profile menu.
  if (IsProfileMenuShowing()) {
    HideProfileMenu();
  } else {
    ShowProfileMenu(button);
  }
}

// ---------------------------------------------------------------------------
// AstraProfileMenuHeaderView::Delegate
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnProfileHeaderClicked() {
  if (delegate_) {
    delegate_->OnProfileHeaderClicked();
  }
}

void AstraProfileMenuController::OnSyncStatusClicked() {
  // TODO(astra): Open sync settings or sign-in page.
  //   For now just close the menu.
  HideProfileMenu();
}

// ---------------------------------------------------------------------------
// AstraProfileMenuFooterView::Delegate
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnSettingsClicked() {
  if (delegate_) {
    delegate_->OnOpenSettings();
  }
  HideProfileMenu();
}

void AstraProfileMenuController::OnHelpClicked() {
  if (delegate_) {
    delegate_->OnOpenHelp();
  }
  HideProfileMenu();
}

void AstraProfileMenuController::OnManageWorkspacesClicked() {
  if (delegate_) {
    delegate_->OnManageWorkspaces();
  }
  HideProfileMenu();
}

void AstraProfileMenuController::OnExitClicked() {
  if (delegate_) {
    delegate_->OnExitClicked();
  }
  HideProfileMenu();
}

// ---------------------------------------------------------------------------
// views::WidgetObserver
// ---------------------------------------------------------------------------

void AstraProfileMenuController::OnWidgetDestroying(views::Widget* widget) {
  if (widget != menu_widget_) {
    return;
  }

  // Widget is being destroyed — clear our references.
  widget->RemoveObserver(this);
  menu_widget_ = nullptr;
  bubble_delegate_ = nullptr;

  // Update model.
  if (model_ && model_->is_open()) {
    model_->CloseMenu();
  }

  // Notify observers.
  NotifyMenuHidden();
}

void AstraProfileMenuController::OnWidgetVisibilityChanged(views::Widget* widget,
                                                           bool visible) {
  if (widget != menu_widget_) {
    return;
  }

  // Widget visibility changed — update model state if needed.
  if (!visible && model_ && model_->is_open()) {
    model_->CloseMenu();
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

AstraWorkspaceService* AstraProfileMenuController::GetWorkspaceService() {
  if (!profile_) {
    return nullptr;
  }
  return AstraWorkspaceServiceFactory::GetForProfile(profile_);
}

AstraThemeService* AstraProfileMenuController::GetThemeService() {
  if (!profile_) {
    return nullptr;
  }
  return AstraThemeServiceFactory::GetForProfile(profile_);
}

Profile* AstraProfileMenuController::GetProfile() {
  return profile_;
}

PrefService* AstraProfileMenuController::GetPrefService() {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

void AstraProfileMenuController::UpdateAllFromServices() {
  // Sync workspace data to model.
  SyncWorkspacesToModel();

  // Update workspace menu view.
  if (workspaces_view_) {
    workspaces_view_->UpdateFromService();
  }

  // Update avatar button.
  UpdateAvatarButtonFromServices();

  // Update header from profile.
  UpdateHeaderFromProfile();

  // Apply model settings.
  ApplyModelSettingsToViews();
}

void AstraProfileMenuController::UpdateAvatarButtonFromServices() {
  if (!avatar_button_) {
    return;
  }

  avatar_button_->UpdateFromService();

  // Set accent color from theme service.
  AstraThemeService* theme_service = GetThemeService();
  if (theme_service) {
    avatar_button_->SetAccentColor(theme_service->GetAccentColor());
  }

  // Set profile info.
  if (profile_) {
    std::u16string profile_name = base::UTF8ToUTF16(profile_->GetProfileUserName());
    if (profile_name.empty()) {
      profile_name = u"User";
    }
    avatar_button_->SetProfileName(profile_name);
  }
}

void AstraProfileMenuController::UpdateHeaderFromProfile() {
  if (!header_view_ || !profile_) {
    return;
  }

  // Set profile name and email from Profile.
  std::u16string profile_name = base::UTF8ToUTF16(profile_->GetProfileUserName());
  if (profile_name.empty()) {
    profile_name = u"User";
  }
  header_view_->SetProfileName(profile_name);
  header_view_->SetProfileEmail(u"user@astra.local");

  // Set accent color from theme service.
  AstraThemeService* theme_service = GetThemeService();
  if (theme_service) {
    header_view_->SetAccentColor(theme_service->GetAccentColor());
  }

  // Set sync status from model.
  header_view_->SetSyncStatus(ModelToHeaderSyncStatus(model_->sync_status()));
}

void AstraProfileMenuController::UpdateWorkspacesViewFromModel() {
  if (!workspaces_view_) {
    return;
  }
  workspaces_view_->SetDisplayMode(
      ModelToViewDisplayMode(model_->workspace_display_mode()));
  workspaces_view_->SetMaxListHeight(
      model_->max_workspaces_shown() * 40);  // 40px per item (medium size)
}

void AstraProfileMenuController::ApplyModelSettingsToViews() {
  if (!model_) {
    return;
  }

  // Header view.
  if (header_view_) {
    header_view_->SetAvatarVisible(model_->show_avatar());
    header_view_->SetSyncStatusVisible(model_->show_sync_status());
    header_view_->SetSyncStatus(ModelToHeaderSyncStatus(model_->sync_status()));
  }

  // Workspaces view.
  if (workspaces_view_) {
    workspaces_view_->SetDisplayMode(
        ModelToViewDisplayMode(model_->workspace_display_mode()));
    workspaces_view_->SetVisible(model_->show_workspaces());
    // Approximate: max_workspaces_shown * 40px per row.
    workspaces_view_->SetMaxListHeight(model_->max_workspaces_shown() * 40);
  }

  // Avatar button.
  if (avatar_button_) {
    // Avatar button doesn't directly use all model settings, but some
    // settings could affect it in the future.
  }
}

std::unique_ptr<views::View>
AstraProfileMenuController::BuildMenuContentView() {
  auto container = std::make_unique<views::View>();
  auto* layout = container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Header view — transfer ownership to the container.
  auto* header = GetHeaderView();
  container->AddChildView(
      std::unique_ptr<views::View>(header_view_.release()));

  // Workspaces view — transfer ownership to the container.
  if (model_->show_workspaces()) {
    auto* workspaces = GetWorkspacesView();
    container->AddChildView(
        std::unique_ptr<views::View>(workspaces_view_.release()));
  }

  // Footer view — transfer ownership to the container.
  auto* footer = GetFooterView();
  container->AddChildView(
      std::unique_ptr<views::View>(footer_view_.release()));

  return container;
}

void AstraProfileMenuController::SyncWorkspacesToModel() {
  if (!model_ || !GetWorkspaceService()) {
    return;
  }

  AstraWorkspaceService* service = GetWorkspaceService();
  const auto& workspaces = service->workspaces();

  std::vector<AstraMenuWorkspaceInfo> workspace_infos;
  workspace_infos.reserve(workspaces.size());

  for (size_t i = 0; i < workspaces.size(); ++i) {
    const auto& ws = workspaces[i];
    AstraMenuWorkspaceInfo info;
    info.id = ws.id;
    info.name = base::UTF8ToUTF16(ws.name);
    info.accent_color = HexToSkColor(ws.accent_color);
    info.tab_count = static_cast<int>(service->GetTabCount(ws.id));
    info.is_active = (ws.id == service->active_workspace_id());
    info.order_index = static_cast<int>(i);
    workspace_infos.push_back(std::move(info));
  }

  model_->SetWorkspaces(workspace_infos);
  model_->SetActiveWorkspaceId(service->active_workspace_id());
}

// ---------------------------------------------------------------------------
// Observer notification helpers
// ---------------------------------------------------------------------------

void AstraProfileMenuController::NotifyMenuWillShow() {
  for (auto& observer : observers_) {
    observer.OnProfileMenuWillShow();
  }
}

void AstraProfileMenuController::NotifyMenuShown() {
  for (auto& observer : observers_) {
    observer.OnProfileMenuShown();
  }
}

void AstraProfileMenuController::NotifyMenuWillHide() {
  for (auto& observer : observers_) {
    observer.OnProfileMenuWillHide();
  }
}

void AstraProfileMenuController::NotifyMenuHidden() {
  for (auto& observer : observers_) {
    observer.OnProfileMenuHidden();
  }
}

}  // namespace astra
