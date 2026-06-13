// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_workspace_avatar_button.h"

#include <algorithm>
#include <utility>

#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
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

// Button sizing — sized to match Chromium's toolbar avatar button.
// Chromium owner: AvatarToolbarButton
//   (chrome/browser/ui/views/toolbar/avatar_toolbar_button.h)
constexpr int kButtonHeightSmall = 28;
constexpr int kButtonHeightMedium = 32;
constexpr int kButtonHeightLarge = 40;

constexpr int kButtonMinWidthSmall = 28;
constexpr int kButtonMinWidthMedium = 32;
constexpr int kButtonMinWidthLarge = 40;

constexpr int kButtonHorizontalPaddingSmall = 6;
constexpr int kButtonHorizontalPaddingMedium = 8;
constexpr int kButtonHorizontalPaddingLarge = 12;

constexpr int kAvatarSizeSmall = 20;
constexpr int kAvatarSizeMedium = 24;
constexpr int kAvatarSizeLarge = 32;

constexpr int kAccentBadgeSizeSmall = 6;
constexpr int kAccentBadgeSizeMedium = 8;
constexpr int kAccentBadgeSizeLarge = 10;

constexpr int kNotificationBadgeSizeSmall = 14;
constexpr int kNotificationBadgeSizeMedium = 18;
constexpr int kNotificationBadgeSizeLarge = 22;

constexpr int kTextSpacingSmall = 4;
constexpr int kTextSpacingMedium = 6;
constexpr int kTextSpacingLarge = 8;

}  // namespace

// ---------------------------------------------------------------------------
// AstraWorkspaceAvatarButton
// ---------------------------------------------------------------------------

AstraWorkspaceAvatarButton::AstraWorkspaceAvatarButton(
    AstraWorkspaceService* workspace_service,
    Delegate* delegate)
    : Button(base::BindRepeating(&AstraWorkspaceAvatarButton::OnButtonClicked,
                                 base::Unretained(this))),
      workspace_service_(workspace_service),
      delegate_(delegate) {
  DCHECK(delegate_);

  // Button behavior.
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  SetFocusRingColorId(kColorAstraWorkspaceAccent);
  SetRequestFocusOnPress(false);
  SetTooltipText(u"Profile and workspaces");

  UpdateSizeVariant();
  UpdateFromService();
}

AstraWorkspaceAvatarButton::~AstraWorkspaceAvatarButton() = default;

// ---------------------------------------------------------------------------
// Display mode
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::SetDisplayMode(AstraAvatarButtonMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  UpdateNameLabelVisibility();
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Size variant
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::SetSizeVariant(AstraAvatarButtonSize size) {
  if (size_variant_ == size) {
    return;
  }
  size_variant_ = size;
  UpdateSizeVariant();
  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Data updates
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::UpdateFromService() {
  if (!workspace_service_) {
    if (workspace_name_label_) {
      workspace_name_label_->SetText(std::u16string());
    }
    accent_color_ = SK_ColorBLUE;
    UpdateTooltip();
    UpdateAccentBadge();
    return;
  }

  const auto& active_ws = workspace_service_->active_workspace();

  // Update workspace name.
  std::u16string ws_name = base::UTF8ToUTF16(active_ws.name);
  if (workspace_name_label_) {
    workspace_name_label_->SetText(ws_name);
  }

  // Update tooltip.
  UpdateTooltip();

  // Update accent badge.
  UpdateAccentBadge();

  // Trigger a repaint.
  SchedulePaint();
}

void AstraWorkspaceAvatarButton::SetProfileName(const std::u16string& name) {
  profile_name_ = name;
  // Update avatar initials from name.
  if (!name.empty()) {
    // Use first character as initial.
    // TODO(astra): Proper initials extraction (first + last name).
    SetAvatarInitials(std::u16string(1, name[0]));
  }
  UpdateTooltip();
}

void AstraWorkspaceAvatarButton::SetProfileEmail(const std::u16string& email) {
  profile_email_ = email;
  UpdateTooltip();
}

void AstraWorkspaceAvatarButton::SetAccentColor(SkColor color) {
  accent_color_ = color;
  UpdateAccentBadge();
  UpdateAvatarVisuals();
}

// ---------------------------------------------------------------------------
// Notification badge
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::SetNotificationCount(int count) {
  if (notification_count_ == count) {
    return;
  }
  notification_count_ = count;
  UpdateNotificationBadge();
}

void AstraWorkspaceAvatarButton::SetNotificationBadgeVisible(bool visible) {
  if (notification_badge_visible_ == visible) {
    return;
  }
  notification_badge_visible_ = visible;
  UpdateNotificationBadge();
}

// ---------------------------------------------------------------------------
// views::Button overrides
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::OnThemeChanged() {
  Button::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update workspace name text color.
  if (workspace_name_label_) {
    workspace_name_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForeground));
  }

  UpdateAvatarVisuals();
  UpdateAccentBadge();
  UpdateNotificationBadge();
}

gfx::Size AstraWorkspaceAvatarButton::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = Button::CalculatePreferredSize(available_size);

  int min_height = GetButtonHeight();
  int min_width = GetButtonHeight();  // Square-ish minimum.

  size.set_height(std::max(size.height(), min_height));
  size.set_width(std::max(size.width(), min_width));

  return size;
}

void AstraWorkspaceAvatarButton::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  Button::GetAccessibleNodeData(node_data);

  // Accessible name.
  std::u16string name = u"Profile and workspaces";
  if (!profile_name_.empty()) {
    name = profile_name_;
  }
  node_data->SetName(name);

  node_data->role = ax::mojom::Role::kButton;
  node_data->SetHasPopup(ax::mojom::HasPopup::kMenu);

  // Description: current workspace + email.
  std::u16string description;
  if (workspace_name_label_ && !workspace_name_label_->GetText().empty()) {
    description = u"Current workspace: " + workspace_name_label_->GetText();
  }
  if (!profile_email_.empty()) {
    if (!description.empty()) {
      description += u", ";
    }
    description += profile_email_;
  }

  // Add notification count if badge is visible and count > 0.
  if (notification_badge_visible_ && notification_count_ > 0) {
    if (!description.empty()) {
      description += u", ";
    }
    description += base::NumberToString16(notification_count_) +
                    u" notifications";
  }

  if (!description.empty()) {
    node_data->SetDescription(description);
  }
}

void AstraWorkspaceAvatarButton::Layout() {
  Button::Layout();

  // Position the avatar initials label (centered in avatar container).
  if (avatar_initials_label_ && avatar_container_) {
    gfx::Rect bounds = avatar_container_->GetLocalBounds();
    avatar_initials_label_->SetBoundsRect(bounds);
  }

  // Position the accent badge at bottom-right of avatar container.
  if (accent_badge_ && avatar_container_) {
    gfx::Size badge_size = accent_badge_->GetPreferredSize();
    int x = avatar_container_->width() - badge_size.width();
    int y = avatar_container_->height() - badge_size.height();
    accent_badge_->SetBounds(x, y, badge_size.width(), badge_size.height());
  }

  // Position the notification badge at top-right of avatar container.
  if (notification_badge_ && avatar_container_) {
    gfx::Size badge_size = notification_badge_->GetPreferredSize();
    int x = avatar_container_->width() - badge_size.width() / 2;
    int y = -badge_size.height() / 4;
    notification_badge_->SetBounds(x, y, badge_size.width(),
                                    badge_size.height());
  }
}

void AstraWorkspaceAvatarButton::OnMouseEntered(const ui::MouseEvent& event) {
  Button::OnMouseEntered(event);
  is_hovered_ = true;
  SchedulePaint();
}

void AstraWorkspaceAvatarButton::OnMouseExited(const ui::MouseEvent& event) {
  Button::OnMouseExited(event);
  is_hovered_ = false;
  SchedulePaint();
}

void AstraWorkspaceAvatarButton::OnFocus() {
  Button::OnFocus();
  SchedulePaint();
}

void AstraWorkspaceAvatarButton::OnBlur() {
  Button::OnBlur();
  SchedulePaint();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AstraWorkspaceAvatarButton::OnButtonClicked() {
  if (delegate_) {
    delegate_->OnWorkspaceAvatarButtonClicked(this);
  }
}

void AstraWorkspaceAvatarButton::UpdateTooltip() {
  // Build tooltip text.
  std::u16string tooltip;

  if (!profile_name_.empty()) {
    tooltip = profile_name_;
    if (!profile_email_.empty()) {
      tooltip += u"\n" + profile_email_;
    }
  }

  if (workspace_service_) {
    const auto& active_ws = workspace_service_->active_workspace();
    std::u16string ws_name = base::UTF8ToUTF16(active_ws.name);
    if (!tooltip.empty()) {
      tooltip += u"\n";
    }
    tooltip += u"Workspace: " + ws_name;
  }

  if (notification_badge_visible_ && notification_count_ > 0) {
    if (!tooltip.empty()) {
      tooltip += u"\n";
    }
    tooltip += base::NumberToString16(notification_count_) +
               u" notifications";
  }

  if (!tooltip.empty()) {
    SetTooltipText(tooltip);
  }
}

void AstraWorkspaceAvatarButton::UpdateAccentBadge() {
  if (!accent_badge_) {
    return;
  }

  int badge_size = GetAccentBadgeSize();
  accent_badge_->SetPreferredSize(gfx::Size(badge_size, badge_size));

  // Round the accent badge.
  accent_badge_->SetBackground(
      views::CreateRoundedRectBackground(accent_color_, badge_size / 2));

  SchedulePaint();
}

void AstraWorkspaceAvatarButton::UpdateAvatarVisuals() {
  if (!avatar_container_ || !avatar_initials_label_) {
    return;
  }

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  int avatar_size = GetAvatarSize();
  avatar_container_->SetPreferredSize(gfx::Size(avatar_size, avatar_size));

  // Circular avatar background.
  // TODO(astra): Use a real profile avatar image when available.
  //   Chromium owner: ProfileAvatarIconUtil
  //   (chrome/browser/profiles/profile_avatar_icon_util.h)
  //   For now we use a subtle background with initials.
  avatar_container_->SetBackground(
      views::CreateRoundedRectBackground(
          color_provider->GetColor(ui::kColorToolbarButtonIcon),
          avatar_size / 2));

  // Initials text color.
  avatar_initials_label_->SetEnabledColor(
      color_provider->GetColor(ui::kColorToolbarButtonIcon));
  // TODO(astra): If we use the accent color for the avatar background,
  //   the text should be white for contrast. For now we keep it subtle.
}

void AstraWorkspaceAvatarButton::SetAvatarInitials(
    const std::u16string& initials) {
  if (avatar_initials_label_) {
    avatar_initials_label_->SetText(initials);
  }
}

void AstraWorkspaceAvatarButton::UpdateNameLabelVisibility() {
  if (!workspace_name_label_) {
    return;
  }
  bool visible = (display_mode_ == AstraAvatarButtonMode::kExpanded);
  workspace_name_label_->SetVisible(visible);
}

void AstraWorkspaceAvatarButton::UpdateNotificationBadge() {
  if (!notification_badge_ || !notification_badge_label_) {
    return;
  }

  bool show_badge = notification_badge_visible_ && notification_count_ > 0;
  notification_badge_->SetVisible(show_badge);

  if (show_badge) {
    int badge_size = GetNotificationBadgeSize();
    notification_badge_->SetPreferredSize(
        gfx::Size(badge_size, badge_size));

    // Red background for notification badge.
    notification_badge_->SetBackground(
        views::CreateRoundedRectBackground(SK_ColorRED, badge_size / 2));

    // Show count, or "+" if > 99.
    std::u16string badge_text;
    if (notification_count_ > 99) {
      badge_text = u"99+";
    } else {
      badge_text = base::NumberToString16(notification_count_);
    }
    notification_badge_label_->SetText(badge_text);
    notification_badge_label_->SetEnabledColor(SK_ColorWHITE);
  }

  SchedulePaint();
}

int AstraWorkspaceAvatarButton::GetAvatarSize() const {
  switch (size_variant_) {
    case AstraAvatarButtonSize::kSmall:
      return kAvatarSizeSmall;
    case AstraAvatarButtonSize::kMedium:
      return kAvatarSizeMedium;
    case AstraAvatarButtonSize::kLarge:
      return kAvatarSizeLarge;
  }
  return kAvatarSizeMedium;
}

int AstraWorkspaceAvatarButton::GetAccentBadgeSize() const {
  switch (size_variant_) {
    case AstraAvatarButtonSize::kSmall:
      return kAccentBadgeSizeSmall;
    case AstraAvatarButtonSize::kMedium:
      return kAccentBadgeSizeMedium;
    case AstraAvatarButtonSize::kLarge:
      return kAccentBadgeSizeLarge;
  }
  return kAccentBadgeSizeMedium;
}

int AstraWorkspaceAvatarButton::GetNotificationBadgeSize() const {
  switch (size_variant_) {
    case AstraAvatarButtonSize::kSmall:
      return kNotificationBadgeSizeSmall;
    case AstraAvatarButtonSize::kMedium:
      return kNotificationBadgeSizeMedium;
    case AstraAvatarButtonSize::kLarge:
      return kNotificationBadgeSizeLarge;
  }
  return kNotificationBadgeSizeMedium;
}

int AstraWorkspaceAvatarButton::GetButtonHeight() const {
  switch (size_variant_) {
    case AstraAvatarButtonSize::kSmall:
      return kButtonHeightSmall;
    case AstraAvatarButtonSize::kMedium:
      return kButtonHeightMedium;
    case AstraAvatarButtonSize::kLarge:
      return kButtonHeightLarge;
  }
  return kButtonHeightMedium;
}

void AstraWorkspaceAvatarButton::UpdateSizeVariant() {
  // Rebuild child views with the current size variant.
  RemoveAllChildViews();

  int button_height = GetButtonHeight();
  int horizontal_padding = [&]() {
    switch (size_variant_) {
      case AstraAvatarButtonSize::kSmall:
        return kButtonHorizontalPaddingSmall;
      case AstraAvatarButtonSize::kMedium:
        return kButtonHorizontalPaddingMedium;
      case AstraAvatarButtonSize::kLarge:
        return kButtonHorizontalPaddingLarge;
    }
    return kButtonHorizontalPaddingMedium;
  }();

  int text_spacing = [&]() {
    switch (size_variant_) {
      case AstraAvatarButtonSize::kSmall:
        return kTextSpacingSmall;
      case AstraAvatarButtonSize::kMedium:
        return kTextSpacingMedium;
      case AstraAvatarButtonSize::kLarge:
        return kTextSpacingLarge;
    }
    return kTextSpacingMedium;
  }();

  // Horizontal layout: [avatar+badges] | [workspace name]
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, horizontal_padding), text_spacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_minimum_cross_axis_size(button_height);

  // --- Avatar container (holds avatar + badge overlays) ---
  auto avatar_container = std::make_unique<views::View>();
  int avatar_size = GetAvatarSize();
  avatar_container->SetPreferredSize(gfx::Size(avatar_size, avatar_size));

  // Initials label inside the avatar.
  auto initials_label = std::make_unique<views::Label>();
  initials_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  initials_label->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  initials_label->SetAutoColorReadabilityEnabled(false);
  initials_label->SetFontList(
      initials_label->font_list().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  avatar_initials_label_ = initials_label.get();
  avatar_container->AddChildView(std::move(initials_label));

  // Accent badge (small colored dot at bottom-right of avatar).
  auto accent_badge = std::make_unique<views::View>();
  int accent_badge_size = GetAccentBadgeSize();
  accent_badge->SetPreferredSize(
      gfx::Size(accent_badge_size, accent_badge_size));
  accent_badge_ = accent_badge.get();
  avatar_container->AddChildView(std::move(accent_badge));

  // Notification badge (top-right of avatar).
  auto notification_badge = std::make_unique<views::View>();
  int notif_badge_size = GetNotificationBadgeSize();
  notification_badge->SetPreferredSize(
      gfx::Size(notif_badge_size, notif_badge_size));
  notification_badge->SetLayoutManager(
      std::make_unique<views::FillLayout>());

  auto badge_label = std::make_unique<views::Label>();
  badge_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  badge_label->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  badge_label->SetAutoColorReadabilityEnabled(false);
  badge_label->SetFontList(
      badge_label->font_list().Derive(
          -2, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
  notification_badge_label_ = badge_label.get();
  notification_badge->AddChildView(std::move(badge_label));

  notification_badge_ = notification_badge.get();
  avatar_container->AddChildView(std::move(notification_badge));

  avatar_container_ = AddChildView(std::move(avatar_container));

  // --- Workspace name label (shown in expanded mode) ---
  auto name_label = std::make_unique<views::Label>();
  name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label->SetAutoColorReadabilityEnabled(false);
  name_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label->SetFontList(
      name_label->font_list().Derive(0, gfx::Font::NORMAL,
                                     gfx::Font::Weight::NORMAL));
  workspace_name_label_ = AddChildView(std::move(name_label));

  // Start in compact mode.
  UpdateNameLabelVisibility();

  // Initialize badge visibility.
  UpdateNotificationBadge();

  // Apply theme.
  if (GetColorProvider()) {
    OnThemeChanged();
  }
}

}  // namespace astra
