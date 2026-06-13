// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_AVATAR_BUTTON_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_AVATAR_BUTTON_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class Label;
class ImageView;
}  // namespace views

namespace astra {

class AstraWorkspaceService;

// Size variant for the avatar button.
enum class AstraAvatarButtonSize {
  kSmall,    // Small icon (e.g., for compact toolbars).
  kMedium,   // Standard size (default).
  kLarge,    // Larger size (e.g., for profile menu headers).
};

// =========================================================================
// Workspace avatar button — profile + workspace indicator in the toolbar
// =========================================================================
//
// AstraWorkspaceAvatarButton is a toolbar button that shows both the user's
// profile avatar and the current workspace indicator.  It is designed to
// replace or augment Chromium's AvatarToolbarButton in Astra-branded builds.
//
// Visual design:
//
//   Compact mode (icon only):
//     [avatar circle]
//       [accent badge]    <- small accent color dot at bottom-right
//       [notification badge] <- optional badge with count
//
//   Expanded mode (icon + name):
//     [avatar circle]  Workspace Name
//       [accent badge]
//
// Deepened features:
//   - Multiple size variants (small, medium, large)
//   - Notification badge with count
//   - Multiple badge types (accent, notification, sync)
//   - Enhanced accessibility
//   - Full keyboard support
//   - Hover and pressed visual states
//   - Tooltip with profile and workspace info
//
// Behavior:
//   - Click: opens the profile menu (delegated to the delegate)
//   - Hover: highlight background, pointer cursor
//   - Keyboard: focus ring, Enter/Space to activate
//   - Accessibility: proper role, name, description
//
// This is a projection-only view: it reads workspace state from
// AstraWorkspaceService but never stores or mutates it.
//
// Chromium owner: AvatarToolbarButton
//   (chrome/browser/ui/views/toolbar/avatar_toolbar_button.h)
// Patch point: ToolbarView::Init() or the avatar button construction site.
//   Replace or augment the avatar button with this one.
//
// TODO(astra): This should ideally be composited with Chromium's
//   AvatarToolbarButton so we get all the profile menu functionality
//   plus the workspace indicator.  For now it's a standalone button
//   button that can be placed alongside the avatar.
// =========================================================================

// Display mode for the avatar button.
enum class AstraAvatarButtonMode {
  kCompact,   // Icon only — avatar with badge.
  kExpanded,   // Icon + workspace name label.
};

class AstraWorkspaceAvatarButton : public views::Button {
 public:
  // Delegate interface for button actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the button is clicked.  The delegate should show the
    // profile menu (e.g., a bubble anchored to this button).
    virtual void OnWorkspaceAvatarButtonClicked(
        AstraWorkspaceAvatarButton* button) = 0;
  };

  AstraWorkspaceAvatarButton(AstraWorkspaceService* workspace_service,
                             Delegate* delegate);
  ~AstraWorkspaceAvatarButton() override;

  AstraWorkspaceAvatarButton(const AstraWorkspaceAvatarButton&) = delete;
  AstraWorkspaceAvatarButton& operator=(const AstraWorkspaceAvatarButton&) =
      delete;

  // -- Display mode -------------------------------------------------------

  // Sets the display mode: compact (icon only) or expanded (icon + name).
  void SetDisplayMode(AstraAvatarButtonMode mode);
  AstraAvatarButtonMode display_mode() const { return display_mode_; }

  // -- Size variant -------------------------------------------------------

  void SetSizeVariant(AstraAvatarButtonSize size);
  AstraAvatarButtonSize size_variant() const { return size_variant_; }

  // -- Data updates (called by controller) ------------------------------------------

  // Refreshes the button's visual state (accent color, workspace name,
  // tooltip) from the underlying workspace service.
  void UpdateFromService();

  // Sets the profile display name (used for initials and tooltip).
  void SetProfileName(const std::u16string& name);

  // Sets the profile email (used for accessibility and tooltip).
  void SetProfileEmail(const std::u16string& email);

  // Sets the accent color for the workspace badge and avatar background.
  void SetAccentColor(SkColor color);

  // -- Notification badge -------------------------------------------------

  // Sets the notification count shown on the badge.
  // A count of 0 hides the badge.
  void SetNotificationCount(int count);
  int notification_count() const { return notification_count_; }

  // Sets whether the notification badge is visible.
  void SetNotificationBadgeVisible(bool visible);
  bool notification_badge_visible() const { return notification_badge_visible_; }

  // -- views::Button ------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;

 private:
  // Handles the button click — forwards to delegate.
  void OnButtonClicked();

  // Updates the tooltip text based on current state.
  void UpdateTooltip();

  // Updates the accent color badge visualization.
  void UpdateAccentBadge();

  // Updates the avatar view (background color, initials).
  void UpdateAvatarVisuals();

  // Updates the avatar initials text.
  void SetAvatarInitials(const std::u16string& initials);

  // Updates visibility of the name label based on display mode.
  void UpdateNameLabelVisibility();

  // Updates the notification badge.
  void UpdateNotificationBadge();

  // Returns the avatar size based on size variant.
  int GetAvatarSize() const;

  // Returns the accent badge size based on size variant.
  int GetAccentBadgeSize() const;

  // Returns the notification badge size based on size variant.
  int GetNotificationBadgeSize() const;

  // Returns the button height based on size variant.
  int GetButtonHeight() const;

  raw_ptr<AstraWorkspaceService> workspace_service_;
  raw_ptr<Delegate> delegate_;

  // Display mode: compact or expanded.
  AstraAvatarButtonMode display_mode_ = AstraAvatarButtonMode::kCompact;

  // Size variant: small, medium, or large.
  AstraAvatarButtonSize size_variant_ = AstraAvatarButtonSize::kMedium;

  // Profile display state.
  std::u16string profile_name_;
  std::u16string profile_email_;

  // The accent color of the current workspace.
  // Cached from the service so we don't need to look it up on every paint.
  SkColor accent_color_ = SK_ColorBLUE;

  // Notification badge state.
  int notification_count_ = 0;
  bool notification_badge_visible_ = false;

  // Hover state.
  bool is_hovered_ = false;

  // Child views (owned by view hierarchy).
  raw_ptr<views::View> avatar_container_ = nullptr;
  raw_ptr<views::Label> avatar_initials_label_ = nullptr;
  raw_ptr<views::View> accent_badge_ = nullptr;
  raw_ptr<views::View> notification_badge_ = nullptr;
  raw_ptr<views::Label> notification_badge_label_ = nullptr;
  raw_ptr<views::Label> workspace_name_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_AVATAR_BUTTON_H_
