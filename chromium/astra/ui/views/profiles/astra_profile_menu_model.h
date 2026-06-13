// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Profile menu model — state, settings, and persistence for the profile menu.
//
// AstraProfileMenuModel owns the menu's state and presentation settings.
// It is the single source of truth for:
//   - Menu open/closed state
//   - Selected profile index (for multi-profile menus)
//   - Active workspace ID
//   - Presentation settings (what to show, how to show it)
//   - Workspace list ordering and visibility
//
// The model does NOT own profile data or workspace data — those come from
// Chromium's Profile and AstraWorkspaceService respectively.  The model
// stores only presentation metadata and menu UI state.
//
// Architecture:
//
//   PrefService  -->  AstraProfileMenuModel  -->  Views (via observers)
//   (persistence)       (state + logic)           (presentation)
//
//   WorkspaceService  --(observed by)-->  Controller  -->  Model
//   (truth source)                           (bridge)     (menu state)
//
// Chromium subsystems reused:
//   - PrefService (persistence)
//   - Profile (profile identity)
//   - IdentityManager (sign-in / sync status)
//
// Chromium owner: ProfileMenuViewController
//   (chrome/browser/ui/views/profiles/profile_menu_view_controller.h)
// Patch point: The model is created and owned by AstraProfileMenuController,
//   which plugs into Chromium's profile menu as an augmented section.
//
// TODO(astra): Consider making this a ProfileKeyedService if the model
//   needs to outlive the menu widget (e.g., for pre-loading workspace data).
//   For now it's a regular object owned by the controller.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_MODEL_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/skia/include/core/SkColor.h"

class PrefService;

namespace astra {

// Workspace display mode — controls how workspaces appear in the menu.
enum class AstraWorkspaceDisplayMode {
  kIconsOnly = 0,       // Only color dots, no names.
  kNamesOnly = 1,       // Only names, no color dots.
  kIconsAndNames = 2,   // Both color dots and names (default).
};

// Menu position — which side of the anchor the menu appears on.
enum class AstraProfileMenuPosition {
  kLeft,   // Menu appears to the left of the anchor.
  kRight,  // Menu appears to the right of the anchor (default).
};

// Sync status state — projected from Chromium's SyncService.
enum class AstraSyncStatus {
  kNotSignedIn,   // User is not signed in.
  kSyncing,       // Sync is actively running.
  kSynced,        // Sync is up to date.
  kError,         // Sync has an error.
  kPaused,        // Sync is paused.
};

// Profile info struct — lightweight projection of Chromium Profile data
// for menu display.  The model stores a copy so views can query it
// without hitting the Profile directly.
struct AstraMenuProfileInfo {
  std::u16string name;
  std::u16string email;
  std::string avatar_url;
  bool is_guest = false;
  bool is_managed = false;
};

// Workspace info struct — lightweight projection of workspace data
// for menu display.  The model stores a copy so it can answer questions
// about workspace ordering and visibility without hitting the service.
struct AstraMenuWorkspaceInfo {
  std::string id;
  std::u16string name;
  SkColor accent_color = SK_ColorBLUE;
  int tab_count = 0;
  int window_count = 0;
  bool is_active = false;
  int order_index = 0;
};

// Multi-profile entry — represents a profile in the multi-profile switcher.
struct AstraMenuProfileEntry {
  std::string profile_path;   // Profile path / identifier.
  std::u16string name;        // Profile display name.
  std::u16string email;       // Profile email / subtitle.
  std::string avatar_url;     // Avatar image URL (optional).
  bool is_guest = false;      // Whether this is a guest profile.
  bool is_managed = false;    // Whether this is a managed profile.
  bool is_current = false;    // Whether this is the currently active profile.
};

// Menu size variant — controls overall menu dimensions.
enum class AstraProfileMenuSize {
  kCompact,   // Narrower menu, smaller items.
  kNormal,    // Standard size (default).
  kLarge,     // Wider menu, larger items.
};

// =========================================================================
// Observer interface
// =========================================================================
//
// Views and controllers observe the model to stay in sync with menu state
// and settings changes.  All observer methods have empty default
// implementations — subclasses override only the methods they care about.
//
// Observer notification granularity:
//   - OnProfileMenuOpened / OnProfileMenuClosed: menu visibility
//   - OnProfileSelected: profile switch within the menu
//   - OnWorkspaceSelected: workspace switch via the menu
//   - OnMenuSettingsChanged: any presentation setting changed
//   - OnWorkspacesChanged: workspace list changed (add/remove/reorder)
// =========================================================================

class AstraProfileMenuModelObserver : public base::CheckedObserver {
 public:
  // Called when the profile menu is opened (becomes visible).
  virtual void OnProfileMenuOpened() {}

  // Called when the profile menu is closed (no longer visible).
  virtual void OnProfileMenuClosed() {}

  // Called when a different profile is selected in the menu.
  // |profile_index| is the 0-based index of the selected profile.
  virtual void OnProfileSelected(int profile_index) {}

  // Called when a workspace is selected from the menu.
  // |workspace_id| is the ID of the selected workspace.
  virtual void OnWorkspaceSelected(const std::string& workspace_id) {}

  // Called when any menu presentation setting changes.
  // Views should rebuild or update their layout to reflect new settings.
  virtual void OnMenuSettingsChanged() {}

  // Called when the workspace list changes (added, removed, reordered).
  virtual void OnWorkspacesChanged() {}

  // Called when the active workspace changes (via any source).
  virtual void OnActiveWorkspaceChanged(const std::string& workspace_id) {}

  // Called when sync status changes.
  virtual void OnSyncStatusChanged(AstraSyncStatus status) {}

  // Called when profile info (name, email, etc.) changes.
  virtual void OnProfileInfoChanged() {}

  // Called when a workspace color changes.
  virtual void OnWorkspaceColorChanged(const std::string& workspace_id) {}

  // Called when the menu size variant changes.
  virtual void OnMenuSizeChanged() {}

  // Called when guest mode is toggled.
  virtual void OnGuestModeToggled(bool is_guest) {}

  // Called when profile info (name, email, etc.) is set / updated.
  virtual void OnProfileInfoSet(const AstraMenuProfileInfo& info) {}

  // Called when the profile list changes (add/remove/reorder).
  virtual void OnProfileListChanged() {}

  // Called when workspace color changes.
  virtual void OnWorkspaceColorChanged(const std::string& workspace_id,
                                       SkColor new_color) {}

  // Called when dividers visibility changes.
  virtual void OnShowDividersChanged(bool show) {}

  // Called when tab counts visibility changes.
  virtual void OnShowTabCountsChanged(bool show) {}

  // Called when compact mode changes.
  virtual void OnCompactModeChanged(bool compact) {}

 protected:
  ~AstraProfileMenuModelObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================

class AstraProfileMenuModel {
 public:
  // Constants for setting bounds.
  static constexpr int kMinMaxWorkspaces = 3;
  static constexpr int kMaxMaxWorkspaces = 10;
  static constexpr int kDefaultMaxWorkspaces = 6;

  // Menu width constants (in DIPs).
  static constexpr int kMenuWidthCompact = 240;
  static constexpr int kMenuWidthNormal = 280;
  static constexpr int kMenuWidthLarge = 340;
  static constexpr int kMinMenuWidth = 200;
  static constexpr int kMaxMenuWidth = 500;
  static constexpr int kDefaultMenuWidth = kMenuWidthNormal;

  AstraProfileMenuModel();
  ~AstraProfileMenuModel();

  AstraProfileMenuModel(const AstraProfileMenuModel&) = delete;
  AstraProfileMenuModel& operator=(const AstraProfileMenuModel&) = delete;

  // -- Menu lifecycle ------------------------------------------------------

  // Opens the menu.  Notifies observers with OnProfileMenuOpened.
  // No-op if already open.
  void OpenMenu();

  // Closes the menu.  Notifies observers with OnProfileMenuClosed.
  // No-op if already closed.
  void CloseMenu();

  // Returns whether the menu is currently open.
  bool is_open() const { return is_open_; }

  // -- Profile selection ---------------------------------------------------

  // Sets the selected profile index.  Clamps to valid range.
  // Notifies observers with OnProfileSelected if the value changed.
  void SetSelectedProfileIndex(int index);

  // Returns the 0-based index of the currently selected profile.
  int selected_profile_index() const { return selected_profile_index_; }

  // Sets the total number of available profiles.
  // Used to clamp the selected index.
  void SetProfileCount(int count);
  int profile_count() const { return profile_count_; }

  // -- Workspace management ------------------------------------------------

  // Sets the active workspace ID.  Notifies observers with
  // OnActiveWorkspaceChanged if the ID changed.
  void SetActiveWorkspaceId(const std::string& workspace_id);

  // Returns the ID of the currently active workspace.
  const std::string& active_workspace_id() const {
    return active_workspace_id_;
  }

  // Returns the full list of workspaces for menu display.
  const std::vector<AstraMenuWorkspaceInfo>& workspaces() const {
    return workspaces_;
  }

  // Returns the number of workspaces.
  size_t GetWorkspaceCount() const { return workspaces_.size(); }

  // Returns workspace info by index, or nullptr if out of range.
  const AstraMenuWorkspaceInfo* GetWorkspaceAt(size_t index) const;

  // Returns workspace info by ID, or nullptr if not found.
  const AstraMenuWorkspaceInfo* GetWorkspaceById(
      const std::string& id) const;

  // Returns the index of the active workspace, or -1 if none.
  int GetActiveWorkspaceIndex() const;

  // Sets the full workspace list.  Notifies observers with OnWorkspacesChanged.
  void SetWorkspaces(const std::vector<AstraMenuWorkspaceInfo>& workspaces);

  // Adds a workspace to the list.  Notifies OnWorkspacesChanged.
  void AddWorkspace(const AstraMenuWorkspaceInfo& workspace);

  // Removes a workspace by ID.  Returns true if found and removed.
  // Notifies OnWorkspacesChanged if removed.
  bool RemoveWorkspace(const std::string& workspace_id);

  // Renames a workspace by ID.  Returns true if found and renamed.
  // Notifies OnWorkspacesChanged if renamed.
  bool RenameWorkspace(const std::string& workspace_id,
                       const std::u16string& new_name);

  // Reorders workspaces: moves the workspace at |from_index| to |to_index|.
  // Returns true if the move was valid and performed.
  // Notifies OnWorkspacesChanged if reordering occurred.
  bool ReorderWorkspace(size_t from_index, size_t to_index);

  // Selects (activates) a workspace by index.  Returns true if the
  // index was valid.  Notifies OnWorkspaceSelected and updates
  // active_workspace_id_.
  bool SelectWorkspaceByIndex(size_t index);

  // Selects (activates) a workspace by ID.  Returns true if the
  // workspace was found.  Notifies OnWorkspaceSelected.
  bool SelectWorkspaceById(const std::string& workspace_id);

  // -- Presentation settings ----------------------------------------------

  // Whether to show the workspace list section in the menu.
  bool show_workspaces() const { return show_workspaces_; }
  void set_show_workspaces(bool show);

  // Maximum number of workspaces shown before scrolling.
  // Clamped to [kMinMaxWorkspaces, kMaxMaxWorkspaces].
  int max_workspaces_shown() const { return max_workspaces_shown_; }
  void set_max_workspaces_shown(int max);

  // Whether to show the profile avatar in the menu header.
  bool show_avatar() const { return show_avatar_; }
  void set_show_avatar(bool show);

  // Whether to show sync status in the menu.
  bool show_sync_status() const { return show_sync_status_; }
  void set_show_sync_status(bool show);

  // Workspace display mode (icons, names, or both).
  AstraWorkspaceDisplayMode workspace_display_mode() const {
    return workspace_display_mode_;
  }
  void set_workspace_display_mode(AstraWorkspaceDisplayMode mode);

  // Menu position (left or right side of anchor).
  AstraProfileMenuPosition menu_position() const { return menu_position_; }
  void set_menu_position(AstraProfileMenuPosition position);

  // Whether to show the recently closed section.
  bool show_recently_closed() const { return show_recently_closed_; }
  void set_show_recently_closed(bool show);

  // Whether to show the sign-in promo.
  bool show_sign_in_promo() const { return show_sign_in_promo_; }
  void set_show_sign_in_promo(bool show);

  // -- Profile info --------------------------------------------------------

  // Sets the current profile info (name, email, avatar, etc.).
  // Notifies observers with OnProfileInfoSet if the info changed.
  void SetProfileInfo(const AstraMenuProfileInfo& info);
  const AstraMenuProfileInfo& profile_info() const { return profile_info_; }

  // -- Multi-profile list --------------------------------------------------

  // Sets the full list of profiles for multi-profile switching.
  void SetProfileList(const std::vector<AstraMenuProfileEntry>& profiles);
  const std::vector<AstraMenuProfileEntry>& profile_list() const {
    return profile_list_;
  }
  size_t GetProfileListSize() const { return profile_list_.size(); }
  const AstraMenuProfileEntry* GetProfileEntryAt(size_t index) const;
  const AstraMenuProfileEntry* GetCurrentProfileEntry() const;

  // -- Guest mode ----------------------------------------------------------

  // Enables or disables guest mode.
  void SetGuestMode(bool enabled);
  bool is_guest_mode() const { return is_guest_mode_; }

  // -- Workspace color customization --------------------------------------

  // Sets the accent color of a workspace by ID.
  // Returns true if the workspace was found and color changed.
  bool SetWorkspaceColor(const std::string& workspace_id, SkColor color);

  // -- Menu size variant ---------------------------------------------------

  // Sets the menu size variant (compact, normal, large).
  void set_menu_size(AstraProfileMenuSize size);
  AstraProfileMenuSize menu_size() const { return menu_size_; }

  // -- Menu width ----------------------------------------------------------

  // Sets a custom menu width in DIPs.  0 means use the size variant default.
  void set_custom_menu_width(int width);
  int custom_menu_width() const { return custom_menu_width_; }

  // Returns the effective menu width, considering custom width and size variant.
  int GetEffectiveMenuWidth() const;

  // -- Additional presentation settings ------------------------------------

  // Whether to show dividers between menu sections.
  bool show_dividers() const { return show_dividers_; }
  void set_show_dividers(bool show);

  // Whether to show tab counts on workspace items.
  bool show_tab_counts() const { return show_tab_counts_; }
  void set_show_tab_counts(bool show);

  // Whether the menu uses compact layout.
  bool compact_mode() const { return compact_mode_; }
  void set_compact_mode(bool compact);

  // Whether to show the "Manage workspaces" link.
  bool show_manage_workspaces() const { return show_manage_workspaces_; }
  void set_show_manage_workspaces(bool show);

  // Whether to show the "New workspace" button.
  bool show_new_workspace_button() const {
    return show_new_workspace_button_;
  }
  void set_show_new_workspace_button(bool show);

  // -- Sync status ---------------------------------------------------------

  // Sets the current sync status.  Notifies OnSyncStatusChanged if changed.
  void SetSyncStatus(AstraSyncStatus status);
  AstraSyncStatus sync_status() const { return sync_status_; }

  // -- Persistence ---------------------------------------------------------

  // Loads presentation settings from PrefService.
  // Call this after construction when a profile / pref service is available.
  void LoadFromPrefs(PrefService* prefs);

  // Saves presentation settings to PrefService.
  // Call this when settings change or before destruction.
  void SaveToPrefs(PrefService* prefs) const;

  // Resets all presentation settings to their default values.
  // Notifies OnMenuSettingsChanged.
  void ResetToDefaults();

  // -- Utility methods -----------------------------------------------------

  // Returns the number of visible workspaces (limited by max_workspaces_shown).
  int GetVisibleWorkspaceCount() const;

  // Returns true if the workspace at |index| is visible within the
  // max_workspaces_shown limit.
  bool IsWorkspaceVisible(size_t index) const;

  // Returns a display label for the current sync status.
  // Used by views for accessibility and tooltips.
  std::u16string GetSyncStatusLabel() const;

  // Returns true if the sync status indicates an error or warning state.
  bool HasSyncError() const;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraProfileMenuModelObserver* observer);
  void RemoveObserver(AstraProfileMenuModelObserver* observer);

 private:
  // Helper: clamps max_workspaces_shown to valid range.
  static int ClampMaxWorkspaces(int value);

  // Helper: notifies observers with OnMenuSettingsChanged.
  void NotifySettingsChanged();

  // -- Menu state ----------------------------------------------------------

  bool is_open_ = false;
  int selected_profile_index_ = 0;
  int profile_count_ = 1;
  std::string active_workspace_id_;

  // -- Workspace list ------------------------------------------------------

  std::vector<AstraMenuWorkspaceInfo> workspaces_;

  // -- Presentation settings ----------------------------------------------

  bool show_workspaces_ = true;
  int max_workspaces_shown_ = kDefaultMaxWorkspaces;
  bool show_avatar_ = true;
  bool show_sync_status_ = true;
  AstraWorkspaceDisplayMode workspace_display_mode_ =
      AstraWorkspaceDisplayMode::kIconsAndNames;
  AstraProfileMenuPosition menu_position_ = AstraProfileMenuPosition::kRight;
  bool show_recently_closed_ = true;
  bool show_sign_in_promo_ = true;

  // -- Additional presentation settings ------------------------------------

  bool show_dividers_ = true;
  bool show_tab_counts_ = true;
  bool compact_mode_ = false;
  AstraProfileMenuSize menu_size_ = AstraProfileMenuSize::kNormal;
  int custom_menu_width_ = 0;
  bool show_manage_workspaces_ = true;
  bool show_new_workspace_button_ = true;

  // -- Profile info --------------------------------------------------------

  AstraMenuProfileInfo profile_info_;

  // -- Multi-profile list --------------------------------------------------

  std::vector<AstraMenuProfileEntry> profile_list_;

  // -- Guest mode ----------------------------------------------------------

  bool is_guest_mode_ = false;

  // -- Sync status ---------------------------------------------------------

  AstraSyncStatus sync_status_ = AstraSyncStatus::kNotSignedIn;

  // -- Observers -----------------------------------------------------------

  base::ObserverList<AstraProfileMenuModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_MODEL_H_
