#include "astra/ui/views/sidebar/astra_password_item_view.h"

#include <string>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kPasswordItemHeight = 56;  // Increased to fit two lines + strength
constexpr int kPasswordItemHorizontalPadding = 12;
constexpr int kPasswordItemIconSize = 16;
constexpr int kPasswordItemIconSpacing = 8;
constexpr int kPasswordItemCopyButtonSize = 16;
constexpr int kPasswordItemCopyButtonSpacing = 4;
constexpr int kPasswordStrengthBarHeight = 3;
constexpr int kPasswordWarningBadgeHeight = 14;
constexpr int kPasswordLastUsedFontDelta = -2;

// Font sizing.
constexpr int kPasswordSiteFontSizeDelta = 0;
constexpr int kPasswordUsernameFontSizeDelta = -1;
constexpr int kPasswordLabelFontSizeDelta = -1;

// Number of dot characters to show for hidden password display.
constexpr int kHiddenPasswordDotCount = 8;

// Astra color IDs for password item styling.
constexpr ui::ColorId kPasswordSiteTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kPasswordUsernameTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kPasswordCompromisedColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kPasswordBlockedTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kPasswordWeakColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kPasswordReusedColorId = kColorAstraSidebarItemText;

}  // namespace

AstraPasswordItemView::AstraPasswordItemView(
    const AstraPasswordEntry& entry)
    : entry_(entry),
      is_password_revealed_(!(entry.strength == AstraPasswordStrength::kVeryWeak)) {
  // Determine the primary warning type.
  if (entry.is_compromised) {
    warning_type_ = AstraPasswordWarningType::kCompromised;
  } else if (entry.is_weak) {
    warning_type_ = AstraPasswordWarningType::kWeak;
  } else if (entry.is_reused) {
    warning_type_ = AstraPasswordWarningType::kReused;
  }

  SetTitle(entry.site_display_name);
  SetSecondaryText(entry.username);
  icon_view()->SetVisible(true);
  UpdateIcon();
}

AstraPasswordItemView::AstraPasswordItemView(
    const std::u16string& site,
    const std::u16string& username,
    bool is_compromised) {
  entry_.site_display_name = site;
  entry_.username = username;
  entry_.is_compromised = is_compromised;
  if (is_compromised) {
    warning_type_ = AstraPasswordWarningType::kCompromised;
  }

  SetTitle(site);
  SetSecondaryText(username);
  icon_view()->SetVisible(true);
  UpdateIcon();
}

AstraPasswordItemView::~AstraPasswordItemView() = default;

void AstraPasswordItemView::BuildLayout() {
  AstraSidebarItemView::BuildLayout();

  // ---- Password display label (below username) ----
  // Shows dots or "Reveal" hint text.
  password_label_ = text_container()->AddChildView(
      std::make_unique<views::Label>(std::u16string()));
  password_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  password_label_->SetAutoColorReadabilityEnabled(false);
  password_label_->SetFontList(
      password_label_->font_list().DeriveWithSizeDelta(kPasswordLabelFontSizeDelta));
  UpdatePasswordDisplay();

  // ---- Strength indicator bar (in the text container, below password) ----
  strength_bar_container_ = text_container()->AddChildView(
      std::make_unique<views::View>());
  strength_bar_container_->SetPreferredSize(
      gfx::Size(0, kPasswordStrengthBarHeight));
  UpdateStrengthIndicator();

  // ---- Warning badge (in trailing container) ----
  warning_badge_ = trailing_container()->AddChildView(
      std::make_unique<views::Label>());
  warning_badge_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  warning_badge_->SetAutoColorReadabilityEnabled(false);
  warning_badge_->SetVisible(false);
  warning_badge_->SetFontList(
      warning_badge_->font_list().DeriveWithSizeDelta(-2));
  UpdateWarningBadge();

  // ---- Last used label (in trailing area, shown below badge) ----
  // For simplicity, we use the secondary text area for last-used info
  // when compact mode is enabled. For now, show in the trailing area
  // when not hovered.
  // TODO(astra): Implement last_used_label_view_ properly in the layout.
  // For now, we just store the text and show it via tooltip.

  // ---- Action buttons (in trailing container) ----
  // These appear on hover.

  // Copy username button.
  copy_username_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPasswordItemView::OnCopyUsernameButtonPressed,
              base::Unretained(this))));
  copy_username_button_->SetPreferredSize(
      gfx::Size(kPasswordItemCopyButtonSize,
                kPasswordItemCopyButtonSize));
  copy_username_button_->SetTooltipText(u"Copy username");
  copy_username_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  copy_username_button_->SetAccessibleName(u"Copy username");

  // Copy password button.
  copy_password_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPasswordItemView::OnCopyPasswordButtonPressed,
              base::Unretained(this))));
  copy_password_button_->SetPreferredSize(
      gfx::Size(kPasswordItemCopyButtonSize,
                kPasswordItemCopyButtonSize));
  copy_password_button_->SetTooltipText(u"Copy password");
  copy_password_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  copy_password_button_->SetAccessibleName(u"Copy password");

  // Reveal/hide password button.
  reveal_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPasswordItemView::OnRevealButtonPressed,
              base::Unretained(this))));
  reveal_button_->SetPreferredSize(
      gfx::Size(kPasswordItemCopyButtonSize,
                kPasswordItemCopyButtonSize));
  reveal_button_->SetTooltipText(u"Show password");
  reveal_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  reveal_button_->SetAccessibleName(u"Show password");

  // Open in new tab button.
  open_in_new_tab_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPasswordItemView::OnOpenInNewTabButtonPressed,
              base::Unretained(this))));
  open_in_new_tab_button_->SetPreferredSize(
      gfx::Size(kPasswordItemCopyButtonSize,
                kPasswordItemCopyButtonSize));
  open_in_new_tab_button_->SetTooltipText(u"Open site in new tab");
  open_in_new_tab_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  open_in_new_tab_button_->SetAccessibleName(u"Open site in new tab");

  // Initially hide action buttons — they appear on hover.
  UpdateActionButtonsVisibility();

  // Accessibility.
  SetAccessibleName(entry_.site_display_name);
}

// =========================================================================
// Password entry
// =========================================================================

void AstraPasswordItemView::SetPasswordEntry(const AstraPasswordEntry& entry) {
  entry_ = entry;

  // Update warning type.
  if (entry.is_compromised) {
    warning_type_ = AstraPasswordWarningType::kCompromised;
  } else if (entry.is_weak) {
    warning_type_ = AstraPasswordWarningType::kWeak;
  } else if (entry.is_reused) {
    warning_type_ = AstraPasswordWarningType::kReused;
  } else {
    warning_type_ = AstraPasswordWarningType::kNone;
  }

  UpdateLabels();
  UpdateIcon();
  UpdateStrengthIndicator();
  UpdateWarningBadge();
  UpdatePasswordDisplay();
  SchedulePaint();
}

// =========================================================================
// Compromised / warning state
// =========================================================================

void AstraPasswordItemView::SetCompromised(bool compromised) {
  if (entry_.is_compromised == compromised) {
    return;
  }
  entry_.is_compromised = compromised;
  if (compromised) {
    warning_type_ = AstraPasswordWarningType::kCompromised;
  } else if (warning_type_ == AstraPasswordWarningType::kCompromised) {
    warning_type_ = AstraPasswordWarningType::kNone;
  }
  UpdateIcon();
  UpdateWarningBadge();
  OnThemeChanged();
}

void AstraPasswordItemView::SetWarningType(AstraPasswordWarningType type) {
  if (warning_type_ == type) {
    return;
  }
  warning_type_ = type;
  UpdateWarningBadge();
  UpdateIcon();
  OnThemeChanged();
}

// =========================================================================
// Blocked state
// =========================================================================

void AstraPasswordItemView::SetIsBlocked(bool blocked) {
  if (entry_.is_blocked == blocked) {
    return;
  }
  entry_.is_blocked = blocked;
  SetEnabled(!blocked);
  UpdateIcon();
}

// =========================================================================
// Password visibility (reveal)
// =========================================================================

void AstraPasswordItemView::SetPasswordRevealed(bool revealed) {
  if (is_password_revealed_ == revealed) {
    return;
  }
  is_password_revealed_ = revealed;
  UpdatePasswordDisplay();
  if (reveal_button_) {
    reveal_button_->SetTooltipText(revealed ? u"Hide password"
                                            : u"Show password");
    reveal_button_->SetAccessibleName(revealed ? u"Hide password"
                                               : u"Show password");
  }
}

void AstraPasswordItemView::TogglePasswordRevealed() {
  SetPasswordRevealed(!is_password_revealed_);
}

// =========================================================================
// Strength indicator
// =========================================================================

void AstraPasswordItemView::SetStrength(AstraPasswordStrength strength) {
  if (entry_.strength == strength) {
    return;
  }
  entry_.strength = strength;
  UpdateStrengthIndicator();
}

void AstraPasswordItemView::SetStrengthPercent(int percent) {
  entry_.strength_percent = percent;
  // Recompute strength level from percent.
  if (percent <= 20) {
    entry_.strength = AstraPasswordStrength::kVeryWeak;
  } else if (percent <= 40) {
    entry_.strength = AstraPasswordStrength::kWeak;
  } else if (percent <= 60) {
    entry_.strength = AstraPasswordStrength::kMedium;
  } else if (percent <= 80) {
    entry_.strength = AstraPasswordStrength::kStrong;
  } else {
    entry_.strength = AstraPasswordStrength::kVeryStrong;
  }
  UpdateStrengthIndicator();
}

void AstraPasswordItemView::SetStrengthIndicatorVisible(bool visible) {
  if (strength_visible_ == visible) {
    return;
  }
  strength_visible_ = visible;
  if (strength_bar_container_) {
    strength_bar_container_->SetVisible(visible);
  }
  InvalidateLayout();
}

// =========================================================================
// Last used time
// =========================================================================

void AstraPasswordItemView::SetLastUsedLabel(const std::u16string& label) {
  last_used_label_ = label;
  if (last_used_label_view_) {
    last_used_label_view_->SetText(label);
  }
  UpdateTooltip();
}

void AstraPasswordItemView::SetLastUsedVisible(bool visible) {
  if (last_used_visible_ == visible) {
    return;
  }
  last_used_visible_ = visible;
  if (last_used_label_view_) {
    last_used_label_view_->SetVisible(visible);
  }
}

// =========================================================================
// Labels / display updates
// =========================================================================

void AstraPasswordItemView::UpdateLabels() {
  if (title_label()) {
    title_label()->SetText(entry_.site_display_name);
  }
  if (secondary_label()) {
    secondary_label()->SetText(entry_.username);
  }
  SetAccessibleName(entry_.site_display_name);
}

void AstraPasswordItemView::UpdatePasswordDisplay() {
  if (!password_label_) {
    return;
  }

  if (is_password_revealed_) {
    // We don't actually store the password — show a placeholder "revealed" indicator.
    // In a real implementation, the password would be fetched on-demand.
    password_label_->SetText(u"*****");  // Placeholder for revealed state
    password_label_->SetTooltipText(u"Password revealed");
  } else {
    // Show dots for hidden password.
    std::u16string dots;
    for (int i = 0; i < kHiddenPasswordDotCount; ++i) {
      dots += u'\u2022';  // Bullet character
    }
    password_label_->SetText(dots);
    password_label_->SetTooltipText(u"Password hidden");
  }
}

void AstraPasswordItemView::UpdateStrengthIndicator() {
  if (!strength_bar_container_) {
    return;
  }
  strength_bar_container_->SetVisible(strength_visible_);
  // TODO(astra): Implement actual strength bar visualization with colored
  //   segments. For now, we just show/hide the container.
  // The strength bar would typically have 4 segments that light up
  // progressively based on strength level.
}

void AstraPasswordItemView::UpdateWarningBadge() {
  if (!warning_badge_) {
    return;
  }

  if (warning_type_ == AstraPasswordWarningType::kNone) {
    warning_badge_->SetVisible(false);
    return;
  }

  warning_badge_->SetVisible(true);
  switch (warning_type_) {
    case AstraPasswordWarningType::kCompromised:
      warning_badge_->SetText(u"!");
      warning_badge_->SetTooltipText(u"Compromised password");
      break;
    case AstraPasswordWarningType::kWeak:
      warning_badge_->SetText(u"W");
      warning_badge_->SetTooltipText(u"Weak password");
      break;
    case AstraPasswordWarningType::kReused:
      warning_badge_->SetText(u"R");
      warning_badge_->SetTooltipText(u"Reused password");
      break;
    default:
      warning_badge_->SetVisible(false);
      break;
  }
}

void AstraPasswordItemView::UpdateLastUsedDisplay() {
  // The last used info is shown in the tooltip for now.
  UpdateTooltip();
}

// =========================================================================
// Icon
// =========================================================================

void AstraPasswordItemView::UpdateIcon() {
  // TODO(astra): Use real icons:
  //   - Normal: key icon
  //   - Compromised: warning icon
  //   - Blocked: blocked/block icon
  //   Chromium resources:
  //     - Key: ui/resources/vector_icons/key_icon
  //     - Warning: ui/resources/vector_icons/warning_icon
  //
  // For now, we just show the default icon placeholder.
  if (icon_view()) {
    icon_view()->SetVisible(true);
  }
}

// =========================================================================
// Action buttons visibility
// =========================================================================

void AstraPasswordItemView::UpdateActionButtonsVisibility() {
  bool show_buttons = is_hovered_internal_ || HasFocus();
  if (copy_password_button_) {
    copy_password_button_->SetVisible(show_buttons && !entry_.is_blocked);
  }
  if (copy_username_button_) {
    copy_username_button_->SetVisible(show_buttons && !entry_.is_blocked);
  }
  if (reveal_button_) {
    reveal_button_->SetVisible(show_buttons && !entry_.is_blocked);
  }
  if (open_in_new_tab_button_) {
    open_in_new_tab_button_->SetVisible(show_buttons && !entry_.is_blocked);
  }
  if (warning_badge_) {
    // Warning badge is always visible when there's a warning,
    // not just on hover.
    warning_badge_->SetVisible(warning_type_ != AstraPasswordWarningType::kNone);
  }
}

// =========================================================================
// Button action handlers
// =========================================================================

void AstraPasswordItemView::OnCopyPasswordButtonPressed() {
  if (delegate_ && !entry_.is_blocked) {
    delegate_->OnPasswordCopyRequested(entry_);
  }
}

void AstraPasswordItemView::OnCopyUsernameButtonPressed() {
  if (delegate_ && !entry_.is_blocked) {
    delegate_->OnUsernameCopyRequested(entry_);
  }
}

void AstraPasswordItemView::OnRevealButtonPressed() {
  TogglePasswordRevealed();
  if (delegate_ && !entry_.is_blocked) {
    delegate_->OnPasswordRevealToggled(entry_, is_password_revealed_);
  }
}

void AstraPasswordItemView::OnOpenInNewTabButtonPressed() {
  if (delegate_ && !entry_.is_blocked) {
    delegate_->OnPasswordOpenInNewTab(entry_);
  }
}

// =========================================================================
// Click handling
// =========================================================================

void AstraPasswordItemView::OnItemClicked() {
  if (delegate_ && !entry_.is_blocked) {
    delegate_->OnPasswordItemClicked(entry_);
  }
}

// =========================================================================
// Hover handling
// =========================================================================

void AstraPasswordItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraPasswordItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseExited(event);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraPasswordItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kPasswordItemHeight));
  return size;
}

void AstraPasswordItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label()) {
    SkColor text_color = color_provider->GetColor(kPasswordSiteTextColorId);

    // Compromised passwords get warning color.
    if (entry_.is_compromised) {
      text_color = color_provider->GetColor(kPasswordCompromisedColorId);
    }

    // Blocked passwords get dimmed text.
    if (entry_.is_blocked) {
      text_color = color_provider->GetColor(kPasswordBlockedTextColorId);
    }

    title_label()->SetEnabledColor(text_color);
  }

  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kPasswordUsernameTextColorId));
    secondary_label()->SetFontList(
        secondary_label()->font_list().DeriveWithSizeDelta(
            kPasswordUsernameFontSizeDelta));
  }

  if (password_label_) {
    password_label_->SetEnabledColor(
        color_provider->GetColor(kPasswordUsernameTextColorId));
  }

  if (warning_badge_ && warning_badge_->GetVisible()) {
    SkColor badge_color = color_provider->GetColor(kPasswordSiteTextColorId);
    switch (warning_type_) {
      case AstraPasswordWarningType::kCompromised:
        badge_color = color_provider->GetColor(kPasswordCompromisedColorId);
        break;
      case AstraPasswordWarningType::kWeak:
        badge_color = color_provider->GetColor(kPasswordWeakColorId);
        break;
      case AstraPasswordWarningType::kReused:
        badge_color = color_provider->GetColor(kPasswordReusedColorId);
        break;
      default:
        break;
    }
    warning_badge_->SetEnabledColor(badge_color);
  }
}

void AstraPasswordItemView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;
  std::u16string name = entry_.site_display_name;
  if (!entry_.username.empty()) {
    name += u" (" + entry_.username + u")";
  }
  if (entry_.is_compromised) {
    name += u" - compromised";
  }
  if (entry_.is_weak) {
    name += u" - weak";
  }
  node_data->SetName(name);
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kDescription,
      base::UTF16ToUTF8(
          AstraPasswordHelper::GetPasswordStrengthLabel(entry_.strength)));
}

}  // namespace astra
