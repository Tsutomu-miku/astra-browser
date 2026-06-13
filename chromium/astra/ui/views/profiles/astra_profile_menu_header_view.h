// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Profile menu header view — top section of the profile menu showing the
// user's avatar, name, and email.
//
// Visual layout:
//   +----------------------------------+
//   |  [avatar]  User Name             |
//   |            user@example.com      |
//   +----------------------------------+
//
// This is a pure presentation view — it does not own profile state.
// The caller (AstraProfileMenuController) sets the display name and email
// from Chromium's Profile / IdentityManager.
//
// Chromium subsystems reused:
//   - Profile (chrome/browser/profiles/profile.h) for profile name/email
//   - IdentityManager (services/identity/) for signed-in user info
//   - ProfileAvatarIconUtil for avatar image generation
//
// Chromium owner: ProfileMenuView (chrome/browser/ui/views/profiles/)
//   The Chromium profile menu has its own header with avatar and sync info.
//   Astra embeds its own header or replaces the Chromium one via patch point.
//
// Patch point: ProfileMenuView::BuildBody() or BuildSyncInfo() —
//   insert or replace the header section with this view.
//
// TODO(astra): Wire up real profile info from Profile / IdentityManager.
//   Currently uses placeholder strings set by the controller.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_HEADER_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
class View;
}  // namespace views

namespace astra {

// Sync status displayed in the header view.
// Mirrors AstraSyncStatus from the model but kept separate here since
// the view should not depend on the model's enum directly.
enum class AstraHeaderSyncStatus {
  kNotSignedIn,
  kSyncing,
  kSynced,
  kError,
  kPaused,
};

// Header view for the profile menu.
//
// Shows:
//   - Profile avatar (circular, with initials as placeholder)
//   - Profile display name (bold, primary text)
//   - Profile email / account info (secondary text)
//   - Sync status indicator (optional, shown when sync status is available)
//   - Notification badge (optional, shown when there are unread notifications)
//
// The view is clickable — clicking may open profile settings or
// the account management page. The click action is delegated upward.
//
// Deepened features:
//   - Sync status indicator with icon and text
//   - Notification badge with count
//   - Full keyboard navigation support
//   - Comprehensive accessibility support
//   - Avatar visibility toggle (controlled by presentation settings)
//
// TODO(astra): Consider making this a Button subclass for proper
//   hover/press effects and accessibility. For now it's a plain View
//   with a click handler since the primary action opens another dialog.
class AstraProfileMenuHeaderView : public views::View {
 public:
  // Delegate for header click actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the user clicks on the header area.
    // Typically opens profile settings or account management.
    virtual void OnProfileHeaderClicked() = 0;

    // Called when the user clicks the sync status indicator.
    // Typically opens sync settings or sign-in page.
    virtual void OnSyncStatusClicked() {}
  };

  explicit AstraProfileMenuHeaderView(Delegate* delegate);
  ~AstraProfileMenuHeaderView() override;

  AstraProfileMenuHeaderView(const AstraProfileMenuHeaderView&) = delete;
  AstraProfileMenuHeaderView& operator=(const AstraProfileMenuHeaderView&) =
      delete;

  // -- Display data setters (called by controller) ------------------------

  // Sets the profile display name (shown as bold primary text).
  void SetProfileName(const std::u16string& name);

  // Sets the profile email or account info (shown as secondary text).
  void SetProfileEmail(const std::u16string& email);

  // Sets the initials shown in the avatar placeholder.
  // Usually the first letter(s) of the user's name.
  void SetAvatarInitials(const std::u16string& initials);

  // Sets the accent color used for the avatar background.
  // Uses the active workspace's accent color for visual consistency.
  void SetAccentColor(SkColor color);

  // -- Sync status --------------------------------------------------------

  // Sets the sync status displayed in the header.
  void SetSyncStatus(AstraHeaderSyncStatus status);
  AstraHeaderSyncStatus sync_status() const { return sync_status_; }

  // Sets whether the sync status indicator is visible.
  void SetSyncStatusVisible(bool visible);
  bool sync_status_visible() const { return sync_status_visible_; }

  // -- Notification badge -------------------------------------------------

  // Sets the notification count shown on the badge.
  // A count of 0 hides the badge.
  void SetNotificationCount(int count);
  int notification_count() const { return notification_count_; }

  // Sets whether the notification badge is visible.
  void SetNotificationBadgeVisible(bool visible);
  bool notification_badge_visible() const { return notification_badge_visible_; }

  // -- Workspace badge ----------------------------------------------------

  // Sets whether the workspace badge (color dot with name) is shown.
  void SetWorkspaceBadgeVisible(bool visible);
  bool workspace_badge_visible() const { return workspace_badge_visible_; }

  // Sets the workspace badge text and color.
  void SetWorkspaceBadge(const std::u16string& name, SkColor color);

  // -- Avatar visibility --------------------------------------------------

  // Sets whether the avatar is shown.
  void SetAvatarVisible(bool visible);
  bool avatar_visible() const { return avatar_visible_; }

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;

 private:
  // Updates the avatar view's background and initials.
  void UpdateAvatarVisuals();

  // Updates the hover state background.
  void UpdateHoverState();

  // Updates the sync status indicator visuals.
  void UpdateSyncStatusVisuals();

  // Updates the notification badge visibility and text.
  void UpdateNotificationBadge();

  // Updates all text colors from the color provider.
  void UpdateTextColors();

  // Updates the workspace badge visuals.
  void UpdateWorkspaceBadge();

  // Returns a human-readable sync status label for accessibility/tooltip.
  std::u16string GetSyncStatusLabel() const;

  raw_ptr<Delegate> delegate_;

  // Child views.
  raw_ptr<views::View> avatar_container_ = nullptr;
  raw_ptr<views::View> avatar_view_ = nullptr;
  raw_ptr<views::Label> avatar_initials_label_ = nullptr;
  raw_ptr<views::View> notification_badge_ = nullptr;
  raw_ptr<views::Label> notification_badge_label_ = nullptr;
  raw_ptr<views::View> workspace_badge_ = nullptr;
  raw_ptr<views::View> workspace_badge_dot_ = nullptr;
  raw_ptr<views::Label> workspace_badge_label_ = nullptr;

  raw_ptr<views::View> text_container_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> email_label_ = nullptr;

  raw_ptr<views::View> sync_status_container_ = nullptr;
  raw_ptr<views::View> sync_status_icon_ = nullptr;
  raw_ptr<views::Label> sync_status_label_ = nullptr;

  // Display state.
  std::u16string profile_name_;
  std::u16string profile_email_;
  std::u16string avatar_initials_;
  SkColor accent_color_ = SK_ColorBLUE;

  // Sync status state.
  AstraHeaderSyncStatus sync_status_ = AstraHeaderSyncStatus::kNotSignedIn;
  bool sync_status_visible_ = false;

  // Notification badge state.
  int notification_count_ = 0;
  bool notification_badge_visible_ = false;

  // Workspace badge state.
  bool workspace_badge_visible_ = false;
  std::u16string workspace_badge_name_;
  SkColor workspace_badge_color_ = SK_ColorBLUE;

  // Avatar visibility.
  bool avatar_visible_ = true;

  // Interaction state.
  bool is_hovered_ = false;
  bool is_pressed_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_HEADER_VIEW_H_
