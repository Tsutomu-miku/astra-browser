// Copyright 2026 The Astra Authors. All rights reserved.
// Use Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_profile_menu_model.h"

#include <algorithm>

#include "astra/browser/astra_prefs.h"
#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Helper: convert a string menu position to the enum.
AstraProfileMenuPosition PositionFromString(const std::string& str) {
  if (str == "left") {
    return AstraProfileMenuPosition::kLeft;
  }
  return AstraProfileMenuPosition::kRight;
}

// Helper: convert a menu position enum to string.
std::string PositionToString(AstraProfileMenuPosition position) {
  switch (position) {
    case AstraProfileMenuPosition::kLeft:
      return "left";
    case AstraProfileMenuPosition::kRight:
      return "right";
  }
  return "right";
}

// Helper: convert int to display mode enum.
AstraWorkspaceDisplayMode DisplayModeFromInt(int value) {
  switch (value) {
    case 0:
      return AstraWorkspaceDisplayMode::kIconsOnly;
    case 1:
      return AstraWorkspaceDisplayMode::kNamesOnly;
    case 2:
    default:
      return AstraWorkspaceDisplayMode::kIconsAndNames;
  }
}

// Helper: display mode enum to int.
int DisplayModeToInt(AstraWorkspaceDisplayMode mode) {
  return static_cast<int>(mode);
}

// Helper: convert int to menu size enum.
AstraProfileMenuSize MenuSizeFromInt(int value) {
  switch (value) {
    case 0:
      return AstraProfileMenuSize::kCompact;
    case 1:
    default:
      return AstraProfileMenuSize::kNormal;
    case 2:
      return AstraProfileMenuSize::kLarge;
  }
}

// Helper: menu size enum to int.
int MenuSizeToInt(AstraProfileMenuSize size) {
  return static_cast<int>(size);
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraProfileMenuModel
// ---------------------------------------------------------------------------

AstraProfileMenuModel::AstraProfileMenuModel() = default;

AstraProfileMenuModel::~AstraProfileMenuModel() = default;

// ---------------------------------------------------------------------------
// Menu lifecycle
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::OpenMenu() {
  if (is_open_) {
    return;
  }
  is_open_ = true;
  for (auto& observer : observers_) {
    observer.OnProfileMenuOpened();
  }
}

void AstraProfileMenuModel::CloseMenu() {
  if (!is_open_) {
    return;
  }
  is_open_ = false;
  for (auto& observer : observers_) {
    observer.OnProfileMenuClosed();
  }
}

// ---------------------------------------------------------------------------
// Profile selection
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetSelectedProfileIndex(int index) {
  int clamped = std::clamp(index, 0, std::max(0, profile_count_ - 1));
  if (selected_profile_index_ == clamped) {
    return;
  }
  selected_profile_index_ = clamped;
  for (auto& observer : observers_) {
    observer.OnProfileSelected(selected_profile_index_);
  }
}

void AstraProfileMenuModel::SetProfileCount(int count) {
  profile_count_ = std::max(1, count);
  // Clamp selected index to new range.
  if (selected_profile_index_ >= profile_count_) {
    selected_profile_index_ = profile_count_ - 1;
  }
}

// ---------------------------------------------------------------------------
// Workspace management
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetActiveWorkspaceId(
    const std::string& workspace_id) {
  if (active_workspace_id_ == workspace_id) {
    return;
  }
  active_workspace_id_ = workspace_id;

  // Update is_active flags in workspace list.
  bool changed = false;
  for (auto& ws : workspaces_) {
    bool was_active = ws.is_active;
    ws.is_active = (ws.id == workspace_id);
    if (was_active != ws.is_active) {
      changed = true;
    }
  }

  if (changed) {
    for (auto& observer : observers_) {
      observer.OnActiveWorkspaceChanged(active_workspace_id_);
    }
  }
}

const AstraMenuWorkspaceInfo* AstraProfileMenuModel::GetWorkspaceAt(
    size_t index) const {
  if (index >= workspaces_.size()) {
    return nullptr;
  }
  return &workspaces_[index];
}

const AstraMenuWorkspaceInfo* AstraProfileMenuModel::GetWorkspaceById(
    const std::string& id) const {
  for (const auto& ws : workspaces_) {
    if (ws.id == id) {
      return &ws;
    }
  }
  return nullptr;
}

int AstraProfileMenuModel::GetActiveWorkspaceIndex() const {
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (workspaces_[i].is_active) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void AstraProfileMenuModel::SetWorkspaces(
    const std::vector<AstraMenuWorkspaceInfo>& workspaces) {
  workspaces_ = workspaces;

  // Update active state based on active_workspace_id_.
  for (auto& ws : workspaces_) {
    ws.is_active = (ws.id == active_workspace_id_);
  }

  // Sort by order_index.
  std::sort(workspaces_.begin(), workspaces_.end(),
            [](const AstraMenuWorkspaceInfo& a,
               const AstraMenuWorkspaceInfo& b) {
              return a.order_index < b.order_index;
            });

  for (auto& observer : observers_) {
    observer.OnWorkspacesChanged();
  }
}

void AstraProfileMenuModel::AddWorkspace(
    const AstraMenuWorkspaceInfo& workspace) {
  AstraMenuWorkspaceInfo ws = workspace;
  ws.is_active = (ws.id == active_workspace_id_);
  ws.order_index = static_cast<int>(workspaces_.size());
  workspaces_.push_back(std::move(ws));
  for (auto& observer : observers_) {
    observer.OnWorkspacesChanged();
  }
}

bool AstraProfileMenuModel::RemoveWorkspace(const std::string& workspace_id) {
  auto it = std::find_if(workspaces_.begin(), workspaces_.end(),
                       [&workspace_id](const AstraMenuWorkspaceInfo& ws) {
                         return ws.id == workspace_id;
                       });
  if (it == workspaces_.end()) {
    return false;
  }

  // If we're removing the active workspace, clear active ID.
  bool was_active = it->is_active;
  workspaces_.erase(it);

  if (was_active) {
    active_workspace_id_.clear();
  }

  for (auto& observer : observers_) {
    observer.OnWorkspacesChanged();
  }
  return true;
}

bool AstraProfileMenuModel::RenameWorkspace(const std::string& workspace_id,
                                           const std::u16string& new_name) {
  for (auto& ws : workspaces_) {
    if (ws.id == workspace_id) {
      if (ws.name == new_name) {
        return false;  // No change.
      }
      ws.name = new_name;
      for (auto& observer : observers_) {
        observer.OnWorkspacesChanged();
      }
      return true;
    }
  }
  return false;
}

bool AstraProfileMenuModel::ReorderWorkspace(size_t from_index,
                                            size_t to_index) {
  if (from_index >= workspaces_.size() ||
      to_index >= workspaces_.size() ||
      from_index == to_index) {
    return false;
  }

  // Move the element.
  AstraMenuWorkspaceInfo item = std::move(workspaces_[from_index]);
  workspaces_.erase(workspaces_.begin() + from_index);
  workspaces_.insert(workspaces_.begin() + to_index, std::move(item));

  // Update order_index values.
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    workspaces_[i].order_index = static_cast<int>(i);
  }

  for (auto& observer : observers_) {
    observer.OnWorkspacesChanged();
  }
  return true;
}

bool AstraProfileMenuModel::SelectWorkspaceByIndex(size_t index) {
  if (index >= workspaces_.size()) {
    return false;
  }
  const std::string& id = workspaces_[index].id;
  SetActiveWorkspaceId(id);
  for (auto& observer : observers_) {
    observer.OnWorkspaceSelected(id);
  }
  return true;
}

bool AstraProfileMenuModel::SelectWorkspaceById(const std::string& workspace_id) {
  // Find the workspace.
  bool found = false;
  for (const auto& ws : workspaces_) {
    if (ws.id == workspace_id) {
      found = true;
      break;
    }
  }
  if (!found) {
    return false;
  }
  SetActiveWorkspaceId(workspace_id);
  for (auto& observer : observers_) {
    observer.OnWorkspaceSelected(workspace_id);
  }
  return true;
}

// ---------------------------------------------------------------------------
// Presentation settings
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::set_show_workspaces(bool show) {
  if (show_workspaces_ == show) {
    return;
  }
  show_workspaces_ = show;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_max_workspaces_shown(int max) {
  int clamped = ClampMaxWorkspaces(max);
  if (max_workspaces_shown_ == clamped) {
    return;
  }
  max_workspaces_shown_ = clamped;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_show_avatar(bool show) {
  if (show_avatar_ == show) {
    return;
  }
  show_avatar_ = show;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_show_sync_status(bool show) {
  if (show_sync_status_ == show) {
    return;
  }
  show_sync_status_ = show;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_workspace_display_mode(
    AstraWorkspaceDisplayMode mode) {
  if (workspace_display_mode_ == mode) {
    return;
  }
  workspace_display_mode_ = mode;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_menu_position(
    AstraProfileMenuPosition position) {
  if (menu_position_ == position) {
    return;
  }
  menu_position_ = position;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_show_recently_closed(bool show) {
  if (show_recently_closed_ == show) {
    return;
  }
  show_recently_closed_ = show;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_show_sign_in_promo(bool show) {
  if (show_sign_in_promo_ == show) {
    return;
  }
  show_sign_in_promo_ = show;
  NotifySettingsChanged();
}

// ---------------------------------------------------------------------------
// Profile info
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetProfileInfo(const AstraMenuProfileInfo& info) {
  // Compare relevant fields to detect change.
  bool changed = (profile_info_.name != info.name) ||
                 (profile_info_.email != info.email) ||
                 (profile_info_.avatar_url != info.avatar_url) ||
                 (profile_info_.is_guest != info.is_guest) ||
                 (profile_info_.is_managed != info.is_managed);

  if (!changed) {
    return;
  }

  profile_info_ = info;

  for (auto& observer : observers_) {
    observer.OnProfileInfoSet(profile_info_);
  }
}

// ---------------------------------------------------------------------------
// Multi-profile list
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetProfileList(
    const std::vector<AstraMenuProfileEntry>& profiles) {
  profile_list_ = profiles;
  // Update profile count.
  profile_count_ = static_cast<int>(profiles.size());
  // Clamp selected index.
  if (selected_profile_index_ >= profile_count_) {
    selected_profile_index_ = std::max(0, profile_count_ - 1);
  }
  // Update current profile flag and profile info.
  for (size_t i = 0; i < profile_list_.size(); ++i) {
    if (profile_list_[i].is_current) {
      selected_profile_index_ = static_cast<int>(i);
      // Sync profile info from the current entry.
      profile_info_.name = profile_list_[i].name;
      profile_info_.email = profile_list_[i].email;
      profile_info_.avatar_url = profile_list_[i].avatar_url;
      profile_info_.is_guest = profile_list_[i].is_guest;
      profile_info_.is_managed = profile_list_[i].is_managed;
      break;
    }
  }

  for (auto& observer : observers_) {
    observer.OnProfileListChanged();
  }
}

const AstraMenuProfileEntry* AstraProfileMenuModel::GetProfileEntryAt(
    size_t index) const {
  if (index >= profile_list_.size()) {
    return nullptr;
  }
  return &profile_list_[index];
}

const AstraMenuProfileEntry* AstraProfileMenuModel::GetCurrentProfileEntry()
    const {
  for (const auto& entry : profile_list_) {
    if (entry.is_current) {
      return &entry;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Guest mode
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetGuestMode(bool enabled) {
  if (is_guest_mode_ == enabled) {
    return;
  }
  is_guest_mode_ = enabled;
  profile_info_.is_guest = enabled;

  for (auto& observer : observers_) {
    observer.OnGuestModeToggled(is_guest_mode_);
  }
}

// ---------------------------------------------------------------------------
// Workspace color customization
// ---------------------------------------------------------------------------

bool AstraProfileMenuModel::SetWorkspaceColor(const std::string& workspace_id,
                                              SkColor color) {
  for (auto& ws : workspaces_) {
    if (ws.id == workspace_id) {
      if (ws.accent_color == color) {
        return false;  // No change.
      }
      ws.accent_color = color;
      for (auto& observer : observers_) {
        observer.OnWorkspaceColorChanged(workspace_id, color);
      }
      return true;
    }
  }
  return false;  // Not found.
}

// ---------------------------------------------------------------------------
// Menu size variant
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::set_menu_size(AstraProfileMenuSize size) {
  if (menu_size_ == size) {
    return;
  }
  menu_size_ = size;
  NotifySettingsChanged();
  for (auto& observer : observers_) {
    observer.OnMenuSizeChanged();
  }
}

// ---------------------------------------------------------------------------
// Menu width
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::set_custom_menu_width(int width) {
  int clamped = std::clamp(width, 0, kMaxMenuWidth);
  if (custom_menu_width_ == clamped) {
    return;
  }
  custom_menu_width_ = clamped;
  NotifySettingsChanged();
}

int AstraProfileMenuModel::GetEffectiveMenuWidth() const {
  if (custom_menu_width_ > 0) {
    return custom_menu_width_;
  }
  switch (menu_size_) {
    case AstraProfileMenuSize::kCompact:
      return kMenuWidthCompact;
    case AstraProfileMenuSize::kNormal:
      return kMenuWidthNormal;
    case AstraProfileMenuSize::kLarge:
      return kMenuWidthLarge;
  }
  return kDefaultMenuWidth;
}

// ---------------------------------------------------------------------------
// Additional presentation settings
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::set_show_dividers(bool show) {
  if (show_dividers_ == show) {
    return;
  }
  show_dividers_ = show;
  NotifySettingsChanged();
  for (auto& observer : observers_) {
    observer.OnShowDividersChanged(show_dividers_);
  }
}

void AstraProfileMenuModel::set_show_tab_counts(bool show) {
  if (show_tab_counts_ == show) {
    return;
  }
  show_tab_counts_ = show;
  NotifySettingsChanged();
  for (auto& observer : observers_) {
    observer.OnShowTabCountsChanged(show_tab_counts_);
  }
}

void AstraProfileMenuModel::set_compact_mode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;
  NotifySettingsChanged();
  for (auto& observer : observers_) {
    observer.OnCompactModeChanged(compact_mode_);
  }
}

void AstraProfileMenuModel::set_show_manage_workspaces(bool show) {
  if (show_manage_workspaces_ == show) {
    return;
  }
  show_manage_workspaces_ = show;
  NotifySettingsChanged();
}

void AstraProfileMenuModel::set_show_new_workspace_button(bool show) {
  if (show_new_workspace_button_ == show) {
    return;
  }
  show_new_workspace_button_ = show;
  NotifySettingsChanged();
}

// ---------------------------------------------------------------------------
// Sync status
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::SetSyncStatus(AstraSyncStatus status) {
  if (sync_status_ == status) {
    return;
  }
  sync_status_ = status;
  for (auto& observer : observers_) {
    observer.OnSyncStatusChanged(sync_status_);
  }
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  show_workspaces_ = prefs->GetBoolean(prefs::kPrefProfileMenuShowWorkspaces);
  max_workspaces_shown_ = ClampMaxWorkspaces(
      prefs->GetInteger(prefs::kPrefProfileMenuMaxWorkspaces));
  show_avatar_ = prefs->GetBoolean(prefs::kPrefProfileMenuShowAvatar);
  show_sync_status_ = prefs->GetBoolean(prefs::kPrefProfileMenuShowSyncStatus);
  workspace_display_mode_ = DisplayModeFromInt(
      prefs->GetInteger(prefs::kPrefProfileMenuWorkspaceDisplayMode));
  menu_position_ = PositionFromString(
      prefs->GetString(prefs::kPrefProfileMenuPosition));
  show_recently_closed_ =
      prefs->GetBoolean(prefs::kPrefProfileMenuShowRecentlyClosed);
  show_sign_in_promo_ =
      prefs->GetBoolean(prefs::kPrefProfileMenuShowSignInPromo);
  show_dividers_ = prefs->GetBoolean(prefs::kPrefProfileMenuShowDividers);
  show_tab_counts_ = prefs->GetBoolean(prefs::kPrefProfileMenuShowTabCounts);
  compact_mode_ = prefs->GetBoolean(prefs::kPrefProfileMenuCompactMode);
  menu_size_ = MenuSizeFromInt(prefs->GetInteger(prefs::kPrefProfileMenuSize));
  custom_menu_width_ =
      std::clamp(prefs->GetInteger(prefs::kPrefProfileMenuCustomWidth),
                 0, kMaxMenuWidth);
  is_guest_mode_ = prefs->GetBoolean(prefs::kPrefProfileMenuGuestMode);
  show_manage_workspaces_ =
      prefs->GetBoolean(prefs::kPrefProfileMenuShowManageWorkspaces);
  show_new_workspace_button_ =
      prefs->GetBoolean(prefs::kPrefProfileMenuShowNewWorkspaceButton);
}

void AstraProfileMenuModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefProfileMenuShowWorkspaces, show_workspaces_);
  prefs->SetInteger(prefs::kPrefProfileMenuMaxWorkspaces,
                    max_workspaces_shown_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowAvatar, show_avatar_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowSyncStatus, show_sync_status_);
  prefs->SetInteger(prefs::kPrefProfileMenuWorkspaceDisplayMode,
                    DisplayModeToInt(workspace_display_mode_));
  prefs->SetString(prefs::kPrefProfileMenuPosition,
                   PositionToString(menu_position_));
  prefs->SetBoolean(prefs::kPrefProfileMenuShowRecentlyClosed,
                    show_recently_closed_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowSignInPromo,
                    show_sign_in_promo_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowDividers, show_dividers_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowTabCounts, show_tab_counts_);
  prefs->SetBoolean(prefs::kPrefProfileMenuCompactMode, compact_mode_);
  prefs->SetInteger(prefs::kPrefProfileMenuSize, MenuSizeToInt(menu_size_));
  prefs->SetInteger(prefs::kPrefProfileMenuCustomWidth, custom_menu_width_);
  prefs->SetBoolean(prefs::kPrefProfileMenuGuestMode, is_guest_mode_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowManageWorkspaces,
                    show_manage_workspaces_);
  prefs->SetBoolean(prefs::kPrefProfileMenuShowNewWorkspaceButton,
                    show_new_workspace_button_);
}

void AstraProfileMenuModel::ResetToDefaults() {
  show_workspaces_ = true;
  max_workspaces_shown_ = kDefaultMaxWorkspaces;
  show_avatar_ = true;
  show_sync_status_ = true;
  workspace_display_mode_ = AstraWorkspaceDisplayMode::kIconsAndNames;
  menu_position_ = AstraProfileMenuPosition::kRight;
  show_recently_closed_ = true;
  show_sign_in_promo_ = true;
  show_dividers_ = true;
  show_tab_counts_ = true;
  compact_mode_ = false;
  menu_size_ = AstraProfileMenuSize::kNormal;
  custom_menu_width_ = 0;
  is_guest_mode_ = false;
  show_manage_workspaces_ = true;
  show_new_workspace_button_ = true;
  NotifySettingsChanged();
}

// ---------------------------------------------------------------------------
// Utility methods
// ---------------------------------------------------------------------------

int AstraProfileMenuModel::GetVisibleWorkspaceCount() const {
  return std::min(static_cast<int>(workspaces_.size()), max_workspaces_shown_);
}

bool AstraProfileMenuModel::IsWorkspaceVisible(size_t index) const {
  return index < static_cast<size_t>(max_workspaces_shown_) &&
         index < workspaces_.size();
}

std::u16string AstraProfileMenuModel::GetSyncStatusLabel() const {
  switch (sync_status_) {
    case AstraSyncStatus::kNotSignedIn:
      return u"Not signed in";
    case AstraSyncStatus::kSyncing:
      return u"Syncing...";
    case AstraSyncStatus::kSynced:
      return u"Sync is on";
    case AstraSyncStatus::kError:
      return u"Sync error";
    case AstraSyncStatus::kPaused:
      return u"Sync paused";
  }
  return u"";
}

bool AstraProfileMenuModel::HasSyncError() const {
  return sync_status_ == AstraSyncStatus::kError ||
         sync_status_ == AstraSyncStatus::kPaused;
}

// ---------------------------------------------------------------------------
// Observers
// ---------------------------------------------------------------------------

void AstraProfileMenuModel::AddObserver(
    AstraProfileMenuModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraProfileMenuModel::RemoveObserver(
    AstraProfileMenuModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int AstraProfileMenuModel::ClampMaxWorkspaces(int value) {
  return std::clamp(value, kMinMaxWorkspaces, kMaxMaxWorkspaces);
}

void AstraProfileMenuModel::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnMenuSettingsChanged();
  }
}

}  // namespace astra
