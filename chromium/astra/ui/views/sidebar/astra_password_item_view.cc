#include "astra/ui/views/sidebar/astra_password_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
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
constexpr int kPasswordItemHeight = 36;
constexpr int kPasswordItemHorizontalPadding = 12;
constexpr int kPasswordItemIconSize = 16;
constexpr int kPasswordItemIconSpacing = 8;
constexpr int kPasswordItemCopyButtonSize = 16;
constexpr int kPasswordItemCopyButtonSpacing = 8;

// Font sizing.
constexpr int kPasswordSiteFontSizeDelta = 0;
constexpr int kPasswordUsernameFontSizeDelta = -1;

// Astra color IDs for password item styling.
constexpr ui::ColorId kPasswordSiteTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kPasswordUsernameTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kPasswordCompromisedColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kPasswordBlockedTextColorId =
    kColorAstraSidebarItemSecondaryText;

}  // namespace

AstraPasswordItemView::AstraPasswordItemView(
    const std::u16string& site,
    const std::u16string& username,
    bool is_compromised)
    : site_(site),
      username_(username),
      is_compromised_(is_compromised) {
  SetTitle(site);
  SetSecondaryText(username);

  // Show icon placeholder.
  icon_view()->SetVisible(true);
  UpdateIcon();
}

AstraPasswordItemView::~AstraPasswordItemView() = default;

void AstraPasswordItemView::BuildLayout() {
  AstraSidebarItemView::BuildLayout();

  // Copy button (right side) — add to trailing container.
  copy_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraPasswordItemView::OnCopyButtonPressed,
                              base::Unretained(this))));
  copy_button_->SetPreferredSize(
      gfx::Size(kPasswordItemCopyButtonSize,
                kPasswordItemCopyButtonSize));
  copy_button_->SetTooltipText(u"Copy password");
  copy_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  copy_button_->SetVisible(false);

  // Initially hide the copy button — it appears on hover.
  UpdateCopyButtonVisibility();
}

// =========================================================================
// Password info
// =========================================================================

void AstraPasswordItemView::SetPasswordInfo(const std::u16string& site,
                                            const std::u16string& username,
                                            bool is_compromised) {
  site_ = site;
  username_ = username;
  is_compromised_ = is_compromised;

  UpdateLabels();
  UpdateIcon();
  SchedulePaint();
}

// =========================================================================
// Compromised state
// =========================================================================

void AstraPasswordItemView::SetCompromised(bool compromised) {
  if (is_compromised_ == compromised) {
    return;
  }
  is_compromised_ = compromised;
  UpdateIcon();
  OnThemeChanged();
}

// =========================================================================
// Blocked state
// =========================================================================

void AstraPasswordItemView::SetIsBlocked(bool blocked) {
  if (is_blocked_ == blocked) {
    return;
  }
  is_blocked_ = blocked;
  SetEnabled(!blocked);
  UpdateIcon();
}

// =========================================================================
// Labels
// =========================================================================

void AstraPasswordItemView::UpdateLabels() {
  if (title_label()) {
    title_label()->SetText(site_);
  }
  if (secondary_label()) {
    secondary_label()->SetText(username_);
  }
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
// Copy button
// =========================================================================

void AstraPasswordItemView::UpdateCopyButtonVisibility() {
  if (copy_button_) {
    copy_button_->SetVisible(is_hovered_internal_ || HasFocus());
  }
}

void AstraPasswordItemView::OnCopyButtonPressed() {
  if (delegate_) {
    delegate_->OnPasswordCopyRequested(site_, username_);
  }
}

// =========================================================================
// Click handling
// =========================================================================

void AstraPasswordItemView::OnItemClicked() {
  if (delegate_ && !is_blocked_) {
    delegate_->OnPasswordItemClicked(site_, username_);
  }
}

// =========================================================================
// Hover handling
// =========================================================================

void AstraPasswordItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  UpdateCopyButtonVisibility();
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraPasswordItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  UpdateCopyButtonVisibility();
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
    if (is_compromised_) {
      text_color = color_provider->GetColor(kPasswordCompromisedColorId);
    }

    // Blocked passwords get dimmed text.
    if (is_blocked_) {
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
}

}  // namespace astra
