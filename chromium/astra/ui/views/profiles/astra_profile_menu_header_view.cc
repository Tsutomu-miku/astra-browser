// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_profile_menu_header_view.h"

#include <algorithm>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kHeaderHeight = 72;          // Taller to accommodate sync status.
constexpr int kHeaderHorizontalPadding = 16;
constexpr int kHeaderVerticalPadding = 10;
constexpr int kAvatarSize = 40;
constexpr int kAvatarTextSpacing = 12;
constexpr int kTextSpacing = 2;
constexpr int kSyncStatusSpacing = 4;
constexpr int kSyncStatusIconSize = 12;

// Notification badge constants.
constexpr int kNotificationBadgeSize = 18;
constexpr int kNotificationBadgeOffset = 6;
constexpr int kNotificationBadgeFontSizeDelta = -2;

// Workspace badge constants.
constexpr int kWorkspaceBadgeHeight = 20;
constexpr int kWorkspaceBadgeDotSize = 8;
constexpr int kWorkspaceBadgeSpacing = 4;
constexpr int kWorkspaceBadgeHorizontalPadding = 6;
constexpr int kWorkspaceBadgeFontSizeDelta = -1;

// Font styles.
constexpr gfx::Font::Weight kNameFontWeight = gfx::Font::Weight::MEDIUM;
constexpr gfx::Font::Weight kSyncFontWeight = gfx::Font::Weight::NORMAL;

// Sync status colors.
constexpr SkColor kSyncStatusSyncedColor = SK_ColorGREEN;
constexpr SkColor kSyncStatusSyncingColor = SK_ColorBLUE;
constexpr SkColor kSyncStatusErrorColor = SK_ColorRED;
constexpr SkColor kSyncStatusPausedColor = SK_ColorGRAY;
constexpr SkColor kSyncStatusNotSignedInColor = SK_ColorLTGRAY;

}  // namespace

// ---------------------------------------------------------------------------
// AstraProfileMenuHeaderView
// ---------------------------------------------------------------------------

AstraProfileMenuHeaderView::AstraProfileMenuHeaderView(Delegate* delegate)
    : delegate_(delegate) {
  // Make focusable for keyboard accessibility.
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  SetFocusRingColorId(kColorAstraWorkspaceAccent);

  // Horizontal layout: [avatar+badge] | [text stack] | [workspace badge]
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(kHeaderVerticalPadding, kHeaderHorizontalPadding),
      kAvatarTextSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // --- Avatar container (avatar + notification badge overlay) ---
  auto avatar_container = std::make_unique<views::View>();
  avatar_container->SetPreferredSize(gfx::Size(kAvatarSize, kAvatarSize));

  // Avatar view (circular background with initials).
  auto avatar_view = std::make_unique<views::View>();
  avatar_view->SetPreferredSize(gfx::Size(kAvatarSize, kAvatarSize));
  avatar_view->SetLayoutManager(std::make_unique<views::FillLayout>());

  auto initials_label = std::make_unique<views::Label>();
  initials_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  initials_label->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  initials_label->SetAutoColorReadabilityEnabled(false);
  initials_label->SetFontList(
      initials_label->font_list().Derive(
          2, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  avatar_initials_label_ = initials_label.get();
  avatar_view->AddChildView(std::move(initials_label));

  avatar_view_ = avatar_view.get();
  avatar_container->AddChildView(std::move(avatar_view));

  // Notification badge (top-right corner of avatar).
  auto notification_badge = std::make_unique<views::View>();
  notification_badge->SetPreferredSize(
      gfx::Size(kNotificationBadgeSize, kNotificationBadgeSize));
  notification_badge->SetLayoutManager(std::make_unique<views::FillLayout>());

  auto badge_label = std::make_unique<views::Label>();
  badge_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  badge_label->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  badge_label->SetAutoColorReadabilityEnabled(false);
  badge_label->SetFontList(
      badge_label->font_list().Derive(
          kNotificationBadgeFontSizeDelta, gfx::Font::NORMAL,
          gfx::Font::Weight::BOLD));
  badge_label->SetEnabledColor(SK_ColorWHITE);
  notification_badge_label_ = badge_label.get();
  notification_badge->AddChildView(std::move(badge_label));

  notification_badge_ = notification_badge.get();
  avatar_container->AddChildView(std::move(notification_badge));

  avatar_container_ = AddChildView(std::move(avatar_container));

  // --- Text stack (vertical: name + email) ---
  auto text_container = std::make_unique<views::View>();
  auto* text_layout =
      text_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kTextSpacing));
  text_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);
  text_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  text_layout->SetFlexForView(text_container, 1);

  // Name label (bold, primary text).
  auto name_label = std::make_unique<views::Label>();
  name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label->SetAutoColorReadabilityEnabled(false);
  name_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label->SetFontList(
      name_label->font_list().Derive(0, gfx::Font::NORMAL, kNameFontWeight));
  name_label_ = text_container->AddChildView(std::move(name_label));

  // Email label (secondary text, smaller).
  auto email_label = std::make_unique<views::Label>();
  email_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  email_label->SetAutoColorReadabilityEnabled(false);
  email_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  email_label->SetFontList(
      email_label->font_list().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  email_label_ = text_container->AddChildView(std::move(email_label));

  // Sync status row (icon + label).
  auto sync_container = std::make_unique<views::View>();
  auto* sync_layout =
      sync_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kSyncStatusSpacing));
  sync_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto sync_icon = std::make_unique<views::View>();
  sync_icon->SetPreferredSize(
      gfx::Size(kSyncStatusIconSize, kSyncStatusIconSize));
  sync_status_icon_ = sync_icon.get();
  sync_container->AddChildView(std::move(sync_icon));

  auto sync_label = std::make_unique<views::Label>();
  sync_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  sync_label->SetAutoColorReadabilityEnabled(false);
  sync_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  sync_label->SetFontList(
      sync_label->font_list().Derive(
          -1, gfx::Font::NORMAL, kSyncFontWeight));
  sync_status_label_ = sync_label.get();
  sync_container->AddChildView(std::move(sync_label));

  sync_status_container_ = text_container->AddChildView(std::move(sync_container));

  text_container_ = AddChildView(std::move(text_container));

  // --- Workspace badge (right side, color dot + name) ---
  auto workspace_badge = std::make_unique<views::View>();
  workspace_badge->SetVisible(false);  // Hidden by default.
  auto* badge_layout = workspace_badge->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kWorkspaceBadgeHorizontalPadding),
          kWorkspaceBadgeSpacing));
  badge_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  badge_layout->set_minimum_cross_axis_size(kWorkspaceBadgeHeight);

  auto badge_dot = std::make_unique<views::View>();
  badge_dot->SetPreferredSize(
      gfx::Size(kWorkspaceBadgeDotSize, kWorkspaceBadgeDotSize));
  workspace_badge_dot_ = badge_dot.get();
  workspace_badge->AddChildView(std::move(badge_dot));

  auto badge_label = std::make_unique<views::Label>();
  badge_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  badge_label->SetAutoColorReadabilityEnabled(false);
  badge_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  badge_label->SetFontList(
      badge_label->font_list().Derive(
          kWorkspaceBadgeFontSizeDelta, gfx::Font::NORMAL,
          gfx::Font::Weight::MEDIUM));
  workspace_badge_label_ = badge_label.get();
  workspace_badge->AddChildView(std::move(badge_label));

  workspace_badge_ = AddChildView(std::move(workspace_badge));

  // Initialize visibility states.
  SetSyncStatusVisible(false);
  SetNotificationBadgeVisible(false);

  // Initialize sync status visuals.
  UpdateSyncStatusVisuals();
}

AstraProfileMenuHeaderView::~AstraProfileMenuHeaderView() = default;

// ---------------------------------------------------------------------------
// Display data setters
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::SetProfileName(const std::u16string& name) {
  profile_name_ = name;
  name_label_->SetText(name);
  // If initials are not set yet, derive them from the name.
  if (avatar_initials_.empty() && !name.empty()) {
    // Use first character as initials placeholder.
    // TODO(astra): Proper initials extraction (first letter of first name +
    //   first letter of last name). For now just use first character.
    SetAvatarInitials(std::u16string(1, name[0]));
  }
  OnPropertyChanged(&profile_name_, views::kPropertyEffectsLayout);
}

void AstraProfileMenuHeaderView::SetProfileEmail(const std::u16string& email) {
  profile_email_ = email;
  email_label_->SetText(email);
  OnPropertyChanged(&profile_email_, views::kPropertyEffectsLayout);
}

void AstraProfileMenuHeaderView::SetAvatarInitials(
    const std::u16string& initials) {
  avatar_initials_ = initials;
  avatar_initials_label_->SetText(initials);
  UpdateAvatarVisuals();
}

void AstraProfileMenuHeaderView::SetAccentColor(SkColor color) {
  accent_color_ = color;
  UpdateAvatarVisuals();
}

// ---------------------------------------------------------------------------
// Sync status
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::SetSyncStatus(AstraHeaderSyncStatus status) {
  if (sync_status_ == status) {
    return;
  }
  sync_status_ = status;
  UpdateSyncStatusVisuals();
  SchedulePaint();
}

void AstraProfileMenuHeaderView::SetSyncStatusVisible(bool visible) {
  if (sync_status_visible_ == visible) {
    return;
  }
  sync_status_visible_ = visible;
  if (sync_status_container_) {
    sync_status_container_->SetVisible(visible);
  }
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Notification badge
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::SetNotificationCount(int count) {
  if (notification_count_ == count) {
    return;
  }
  notification_count_ = count;
  UpdateNotificationBadge();
}

void AstraProfileMenuHeaderView::SetNotificationBadgeVisible(bool visible) {
  if (notification_badge_visible_ == visible) {
    return;
  }
  notification_badge_visible_ = visible;
  UpdateNotificationBadge();
}

// ---------------------------------------------------------------------------
// Avatar visibility
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::SetAvatarVisible(bool visible) {
  if (avatar_visible_ == visible) {
    return;
  }
  avatar_visible_ = visible;
  if (avatar_container_) {
    avatar_container_->SetVisible(visible);
  }
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Workspace badge
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::SetWorkspaceBadgeVisible(bool visible) {
  if (workspace_badge_visible_ == visible) {
    return;
  }
  workspace_badge_visible_ = visible;
  if (workspace_badge_) {
    workspace_badge_->SetVisible(visible);
  }
  PreferredSizeChanged();
}

void AstraProfileMenuHeaderView::SetWorkspaceBadge(const std::u16string& name,
                                                  SkColor color) {
  workspace_badge_name_ = name;
  workspace_badge_color_ = color;
  UpdateWorkspaceBadge();
}

// ---------------------------------------------------------------------------
// views::View overrides
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::OnThemeChanged() {
  View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  UpdateTextColors();
  UpdateAvatarVisuals();
  UpdateHoverState();
  UpdateSyncStatusVisuals();
  UpdateNotificationBadge();
  UpdateWorkspaceBadge();
}

gfx::Size AstraProfileMenuHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = View::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kHeaderHeight));
  return size;
}

bool AstraProfileMenuHeaderView::OnMousePressed(const ui::MouseEvent& event) {
  is_pressed_ = true;
  return true;
}

void AstraProfileMenuHeaderView::OnMouseReleased(const ui::MouseEvent& event) {
  bool was_pressed = is_pressed_;
  is_pressed_ = false;

  if (was_pressed && event.IsOnlyLeftMouseButton() &&
      HitTestPoint(event.location())) {
    if (delegate_) {
      // Check if click was on the sync status area.
      if (sync_status_visible_ && sync_status_container_ &&
          sync_status_container_->HitTestPoint(event.location())) {
        delegate_->OnSyncStatusClicked();
      } else {
        delegate_->OnProfileHeaderClicked();
      }
    }
  }
  SchedulePaint();
}

void AstraProfileMenuHeaderView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateHoverState();
}

void AstraProfileMenuHeaderView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  is_pressed_ = false;
  UpdateHoverState();
}

void AstraProfileMenuHeaderView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  View::GetAccessibleNodeData(node_data);

  // Build accessible name: "User Name, user@example.com".
  std::u16string accessible_name;
  if (!profile_name_.empty()) {
    accessible_name = profile_name_;
    if (!profile_email_.empty()) {
      accessible_name += u", " + profile_email_;
    }
  } else if (!profile_email_.empty()) {
    accessible_name = profile_email_;
  }
  node_data->SetName(accessible_name);

  node_data->role = ax::mojom::Role::kButton;
  node_data->SetDescription(u"Manage profile");

  // Add sync status to description if visible.
  if (sync_status_visible_) {
    std::u16string sync_desc = GetSyncStatusLabel();
    if (!sync_desc.empty()) {
      std::u16string desc = node_data->GetDescription();
      if (!desc.empty()) {
        desc += u", ";
      }
      desc += sync_desc;
      node_data->SetDescription(desc);
    }
  }

  // Add notification count if badge is visible and count > 0.
  if (notification_badge_visible_ && notification_count_ > 0) {
    std::u16string desc = node_data->GetDescription();
    if (!desc.empty()) {
      desc += u", ";
    }
    desc += base::NumberToString16(notification_count_) + u" notifications";
    node_data->SetDescription(desc);
  }

  // Add workspace info if badge is visible.
  if (workspace_badge_visible_ && !workspace_badge_name_.empty()) {
    std::u16string desc = node_data->GetDescription();
    if (!desc.empty()) {
      desc += u", ";
    }
    desc += u"Workspace: " + workspace_badge_name_;
    node_data->SetDescription(desc);
  }
}

bool AstraProfileMenuHeaderView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.key_code() == ui::VKEY_RETURN ||
      event.key_code() == ui::VKEY_SPACE) {
    if (delegate_) {
      delegate_->OnProfileHeaderClicked();
    }
    return true;
  }
  return View::OnKeyPressed(event);
}

bool AstraProfileMenuHeaderView::OnKeyReleased(const ui::KeyEvent& event) {
  if (event.key_code() == ui::VKEY_RETURN ||
      event.key_code() == ui::VKEY_SPACE) {
    return true;
  }
  return View::OnKeyReleased(event);
}

void AstraProfileMenuHeaderView::OnFocus() {
  View::OnFocus();
  SchedulePaint();
}

void AstraProfileMenuHeaderView::OnBlur() {
  View::OnBlur();
  is_pressed_ = false;
  SchedulePaint();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AstraProfileMenuHeaderView::UpdateAvatarVisuals() {
  if (!avatar_view_ || !avatar_initials_label_) {
    return;
  }

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Circular avatar background with accent color.
  // TODO(astra): Use a real profile avatar image when available.
  //   Chromium owner: ProfileAvatarIconUtil
  //   (chrome/browser/profiles/profile_avatar_icon_util.h)
  avatar_view_->SetBackground(views::CreateRoundedRectBackground(
      accent_color_, kAvatarSize / 2));

  // Initials text color — use white for contrast on accent background.
  // TODO(astra): Compute proper contrast ratio against accent_color_.
  //   For now assume accent colors are dark enough for white text.
  avatar_initials_label_->SetEnabledColor(SK_ColorWHITE);
}

void AstraProfileMenuHeaderView::UpdateHoverState() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (is_hovered_ || HasFocus()) {
    SetBackground(views::CreateSolidBackground(
        color_provider->GetColor(ui::kColorHoverButtonBackground)));
  } else {
    SetBackground(nullptr);
  }
}

void AstraProfileMenuHeaderView::UpdateSyncStatusVisuals() {
  if (!sync_status_icon_ || !sync_status_label_) {
    return;
  }

  const auto* color_provider = GetColorProvider();

  SkColor icon_color;
  std::u16string label_text;

  switch (sync_status_) {
    case AstraHeaderSyncStatus::kNotSignedIn:
      icon_color = kSyncStatusNotSignedInColor;
      label_text = u"Not signed in";
      break;
    case AstraHeaderSyncStatus::kSyncing:
      icon_color = kSyncStatusSyncingColor;
      label_text = u"Syncing...";
      break;
    case AstraHeaderSyncStatus::kSynced:
      icon_color = kSyncStatusSyncedColor;
      label_text = u"Sync is on";
      break;
    case AstraHeaderSyncStatus::kError:
      icon_color = kSyncStatusErrorColor;
      label_text = u"Sync error";
      break;
    case AstraHeaderSyncStatus::kPaused:
      icon_color = kSyncStatusPausedColor;
      label_text = u"Sync paused";
      break;
  }

  sync_status_icon_->SetBackground(views::CreateRoundedRectBackground(
      icon_color, kSyncStatusIconSize / 2));
  sync_status_label_->SetText(label_text);

  if (color_provider) {
    sync_status_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

void AstraProfileMenuHeaderView::UpdateNotificationBadge() {
  if (!notification_badge_ || !notification_badge_label_) {
    return;
  }

  bool show_badge = notification_badge_visible_ && notification_count_ > 0;
  notification_badge_->SetVisible(show_badge);

  if (show_badge) {
    // Red background for notification badge.
    notification_badge_->SetBackground(views::CreateRoundedRectBackground(
        SK_ColorRED, kNotificationBadgeSize / 2));

    // Show count, or "+" if > 99.
    std::u16string badge_text;
    if (notification_count_ > 99) {
      badge_text = u"99+";
    } else {
      badge_text = base::NumberToString16(notification_count_);
    }
    notification_badge_label_->SetText(badge_text);
  }
}

void AstraProfileMenuHeaderView::UpdateWorkspaceBadge() {
  if (!workspace_badge_dot_ || !workspace_badge_label_) {
    return;
  }

  // Color dot.
  workspace_badge_dot_->SetBackground(views::CreateRoundedRectBackground(
      workspace_badge_color_, kWorkspaceBadgeDotSize / 2));

  // Label.
  workspace_badge_label_->SetText(workspace_badge_name_);

  const auto* color_provider = GetColorProvider();
  if (color_provider) {
    workspace_badge_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

void AstraProfileMenuHeaderView::UpdateTextColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  name_label_->SetEnabledColor(
      color_provider->GetColor(ui::kColorLabelForeground));
  email_label_->SetEnabledColor(
      color_provider->GetColor(ui::kColorLabelForegroundSecondary));
}

std::u16string AstraProfileMenuHeaderView::GetSyncStatusLabel() const {
  switch (sync_status_) {
    case AstraHeaderSyncStatus::kNotSignedIn:
      return u"Not signed in";
    case AstraHeaderSyncStatus::kSyncing:
      return u"Syncing";
    case AstraHeaderSyncStatus::kSynced:
      return u"Sync is on";
    case AstraHeaderSyncStatus::kError:
      return u"Sync error";
    case AstraHeaderSyncStatus::kPaused:
      return u"Sync paused";
  }
  return u"";
}

}  // namespace astra
