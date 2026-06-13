#include "astra/ui/views/sidebar/astra_extension_icon_view.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kIconSize = 24;
constexpr int kIconPadding = 4;

// Opacity values.
constexpr float kDisabledOpacity = 0.4f;
constexpr float kBlockedOpacity = 0.3f;
constexpr float kPopupShowingOpacity = 1.0f;
constexpr float kDefaultOpacity = 0.9f;

}  // namespace

AstraExtensionIconView::AstraExtensionIconView(
    const std::string& extension_id,
    AstraExtensionIconDelegate* delegate)
    : views::ImageButton(base::BindRepeating(
          &AstraExtensionIconView::HandlePrimaryAction,
          base::Unretained(this))),
      extension_id_(extension_id),
      delegate_(delegate) {
  // Set up the image button with proper sizing.
  SetImageHorizontalAlignment(views::ImageButton::Alignment::kCenter);
  SetImageVerticalAlignment(views::ImageButton::Alignment::kCenter);

  // Configure tooltip behavior.
  SetHasInkDrop(true);
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Default opacity.
  SetOpacity(kDefaultOpacity);
}

AstraExtensionIconView::~AstraExtensionIconView() = default;

// =========================================================================
// Extension info
// =========================================================================

void AstraExtensionIconView::SetExtensionInfo(const std::string& extension_id,
                                              const std::u16string& name,
                                              const gfx::ImageSkia& icon) {
  extension_id_ = extension_id;
  extension_name_ = name;
  icon_ = icon;

  SetExtensionIcon(icon);
  UpdateTooltip();
  SchedulePaint();
}

// =========================================================================
// Icon
// =========================================================================

void AstraExtensionIconView::SetExtensionIcon(const gfx::ImageSkia& icon) {
  icon_ = icon;
  // Set icon for all states.
  SetImage(views::ImageButton::ButtonState::STATE_NORMAL, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_HOVERED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_PRESSED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_DISABLED, icon_);
  SchedulePaint();
}

// =========================================================================
// Badge
// =========================================================================

void AstraExtensionIconView::SetBadgeText(const std::u16string& text) {
  badge_text_ = text;
  UpdateTooltip();
  // TODO(astra): Render badge on top of the icon.
  //   For now, badge text is only shown in the tooltip.
  //   Chromium pattern: ToolbarActionView badge rendering.
}

// =========================================================================
// Action vs extension
// =========================================================================

void AstraExtensionIconView::SetIsAction(bool is_action) {
  if (is_action_ == is_action) {
    return;
  }
  is_action_ = is_action;
  // TODO(astra): Apply different styling for browser actions vs. sidebar
  //   extensions. Could include different background, border, etc.
  SchedulePaint();
}

// =========================================================================
// Extension state
// =========================================================================

void AstraExtensionIconView::SetExtensionState(AstraExtensionState state) {
  if (extension_state_ == state) {
    return;
  }
  extension_state_ = state;
  UpdateVisuals();
}

void AstraExtensionIconView::SetExtensionEnabled(bool enabled) {
  SetExtensionState(enabled ? AstraExtensionState::kEnabled
                            : AstraExtensionState::kDisabled);
}

// =========================================================================
// Popup state
// =========================================================================

void AstraExtensionIconView::SetPopupShowing(bool showing) {
  if (popup_showing_ == showing) {
    return;
  }
  popup_showing_ = showing;
  UpdateVisuals();
}

// =========================================================================
// Visuals
// =========================================================================

void AstraExtensionIconView::UpdateVisuals() {
  // Opacity based on state.
  float opacity = kDefaultOpacity;

  switch (extension_state_) {
    case AstraExtensionState::kEnabled:
      opacity = kDefaultOpacity;
      break;
    case AstraExtensionState::kDisabled:
      opacity = kDisabledOpacity;
      break;
    case AstraExtensionState::kBlocked:
      opacity = kBlockedOpacity;
      break;
  }

  // Popup showing overrides with higher opacity.
  if (popup_showing_) {
    opacity = kPopupShowingOpacity;
  }

  SetOpacity(opacity);

  // Enable/disable based on state.
  bool enabled = extension_state_ == AstraExtensionState::kEnabled;
  SetEnabled(enabled);

  SchedulePaint();
}

void AstraExtensionIconView::UpdateTooltip() {
  std::u16string tooltip = extension_name_;
  if (!badge_text_.empty()) {
    tooltip += u" (" + badge_text_ + u")";
  }
  views::ImageButton::SetTooltipText(tooltip);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraExtensionIconView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Square icon button with padding.
  int size = kIconSize + 2 * kIconPadding;
  return gfx::Size(size, size);
}

void AstraExtensionIconView::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update ink drop colors based on the current theme.
  // TODO(astra): Use Astra-specific color tokens once we have a dedicated
  //   color provider mixin at astra/ui/color/.
}

bool AstraExtensionIconView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsRightMouseButton()) {
    // Right click: will show context menu on release.
    return true;
  }
  return views::ImageButton::OnMousePressed(event);
}

void AstraExtensionIconView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsRightMouseButton() && HitTestPoint(event.location())) {
    gfx::Point screen_point = event.location();
    ConvertPointToScreen(this, &screen_point);
    ShowContextMenu(screen_point);
    return;
  }
  views::ImageButton::OnMouseReleased(event);
}

void AstraExtensionIconView::OnGestureEvent(ui::GestureEvent* event) {
  // TODO(astra): Handle long-press for context menu on touch.
  //   Chromium pattern: ExtensionToolbarButton uses gesture events for
  //   touch support.
  views::ImageButton::OnGestureEvent(event);
}

// =========================================================================
// Action handlers
// =========================================================================

void AstraExtensionIconView::HandlePrimaryAction(const ui::Event& event) {
  if (!delegate_) {
    return;
  }
  delegate_->OnExtensionIconClicked(extension_id_, this);
}

void AstraExtensionIconView::ShowContextMenu(const gfx::Point& screen_point) {
  if (!delegate_) {
    return;
  }
  delegate_->OnExtensionIconContextMenu(extension_id_, screen_point);
}

// =========================================================================
// Metadata
// =========================================================================

BEGIN_METADATA(AstraExtensionIconView)
END_METADATA

}  // namespace astra
