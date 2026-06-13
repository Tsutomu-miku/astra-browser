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

// Opacity values per extension state.
constexpr float kEnabledOpacity = 0.9f;
constexpr float kDisabledOpacity = 0.4f;
constexpr float kBlockedOpacity = 0.3f;
constexpr float kErrorOpacity = 0.7f;
constexpr float kUninstalledOpacity = 0.2f;

// Opacity override when popup is showing.
constexpr float kPopupShowingOpacity = 1.0f;

// Default badge colors.
constexpr SkColor kDefaultBadgeTextColor = SK_ColorWHITE;
constexpr SkColor kDefaultBadgeBackgroundColor = SK_ColorRED;

// Default icon size in dp.
constexpr int kDefaultIconSize = 24;

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
  SetOpacity(kEnabledOpacity);
}

AstraExtensionIconView::~AstraExtensionIconView() = default;

// =========================================================================
// Extension identity
// =========================================================================

const std::string& AstraExtensionIconView::GetExtensionId() const {
  return extension_id_;
}

// =========================================================================
// Extension info
// =========================================================================

void AstraExtensionIconView::SetExtensionInfo(const AstraExtensionInfo& info) {
  extension_id_ = info.extension_id;
  extension_name_ = info.name;
  icon_ = info.icon;
  has_icon_ = info.has_icon;
  extension_state_ = info.state;
  is_action_ = info.is_action;
  is_pinned_ = info.is_pinned;
  has_popup_ = info.has_popup;

  SetImage(views::ImageButton::ButtonState::STATE_NORMAL, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_HOVERED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_PRESSED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_DISABLED, icon_);

  UpdateTooltip();
  UpdateVisuals();
  SchedulePaint();
}

// =========================================================================
// Name / tooltip
// =========================================================================

void AstraExtensionIconView::SetName(const std::u16string& name) {
  if (extension_name_ == name) {
    return;
  }
  extension_name_ = name;
  UpdateTooltip();
}

const std::u16string& AstraExtensionIconView::GetName() const {
  return extension_name_;
}

// =========================================================================
// Icon
// =========================================================================

void AstraExtensionIconView::SetIcon(const gfx::ImageSkia& icon) {
  if (icon_.BackedBySameObjectAs(icon)) {
    return;
  }
  icon_ = icon;
  has_icon_ = !icon.isNull();
  SetImage(views::ImageButton::ButtonState::STATE_NORMAL, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_HOVERED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_PRESSED, icon_);
  SetImage(views::ImageButton::ButtonState::STATE_DISABLED, icon_);
  SchedulePaint();
}

const gfx::ImageSkia& AstraExtensionIconView::GetIcon() const {
  return icon_;
}

void AstraExtensionIconView::SetHasIcon(bool has_icon) {
  has_icon_ = has_icon;
  // TODO(astra): Show default placeholder icon when has_icon is false.
  //   Chromium pattern: ExtensionIconManager provides default icons.
  SchedulePaint();
}

bool AstraExtensionIconView::HasIcon() const {
  return has_icon_;
}

// =========================================================================
// Badge
// =========================================================================

void AstraExtensionIconView::SetBadgeText(const std::u16string& text) {
  if (badge_text_ == text) {
    return;
  }
  badge_text_ = text;
  UpdateTooltip();
  // TODO(astra): Render badge on top of the icon.
  //   For now, badge text is only shown in the tooltip.
  //   Chromium pattern: ToolbarActionView badge rendering.
  //   The badge is a small label in the top-right corner with a
  //   colored background and truncated text (max ~3 chars).
  SchedulePaint();
}

const std::u16string& AstraExtensionIconView::GetBadgeText() const {
  return badge_text_;
}

void AstraExtensionIconView::SetBadgeColor(SkColor color) {
  badge_text_color_ = color;
  // TODO(astra): Apply badge text color when badge rendering is implemented.
  SchedulePaint();
}

SkColor AstraExtensionIconView::GetBadgeColor() const {
  return badge_text_color_;
}

void AstraExtensionIconView::SetBadgeBackgroundColor(SkColor color) {
  badge_background_color_ = color;
  // TODO(astra): Apply badge background color when badge rendering is
  //   implemented. Chromium pattern: ExtensionBadgeBackgroundColor.
  SchedulePaint();
}

SkColor AstraExtensionIconView::GetBadgeBackgroundColor() const {
  return badge_background_color_;
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

AstraExtensionState AstraExtensionIconView::GetExtensionState() const {
  return extension_state_;
}

void AstraExtensionIconView::SetExtensionEnabled(bool enabled) {
  SetExtensionState(enabled ? AstraExtensionState::kEnabled
                            : AstraExtensionState::kDisabled);
}

bool AstraExtensionIconView::IsExtensionEnabled() const {
  return extension_state_ == AstraExtensionState::kEnabled;
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
  //   extensions. Could include different background, border, or icon size.
  SchedulePaint();
}

bool AstraExtensionIconView::IsAction() const {
  return is_action_;
}

// =========================================================================
// Pinned state
// =========================================================================

void AstraExtensionIconView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  // TODO(astra): Show pin indicator (e.g., a small pin icon in the corner).
  //   Chromium pattern: ExtensionsToolbarContainer pin indicator.
  SchedulePaint();
}

bool AstraExtensionIconView::IsPinned() const {
  return is_pinned_;
}

// =========================================================================
// Tooltip
// =========================================================================

void AstraExtensionIconView::SetShowTooltip(bool show) {
  if (show_tooltip_ == show) {
    return;
  }
  show_tooltip_ = show;
  if (!show_tooltip_) {
    views::ImageButton::SetTooltipText(std::u16string());
  } else {
    UpdateTooltip();
  }
}

bool AstraExtensionIconView::GetShowTooltip() const {
  return show_tooltip_;
}

// =========================================================================
// Context menu
// =========================================================================

void AstraExtensionIconView::SetShowContextMenu(bool show) {
  show_context_menu_ = show;
}

bool AstraExtensionIconView::GetShowContextMenu() const {
  return show_context_menu_;
}

// =========================================================================
// Icon size
// =========================================================================

void AstraExtensionIconView::SetIconSize(int size_px) {
  if (icon_size_ == size_px) {
    return;
  }
  icon_size_ = size_px;
  InvalidateLayout();
  SchedulePaint();
}

int AstraExtensionIconView::GetIconSize() const {
  return icon_size_;
}

// =========================================================================
// Popup
// =========================================================================

void AstraExtensionIconView::SetHasPopup(bool has_popup) {
  has_popup_ = has_popup;
}

bool AstraExtensionIconView::HasPopup() const {
  return has_popup_;
}

void AstraExtensionIconView::SetPopupShowing(bool showing) {
  if (popup_showing_ == showing) {
    return;
  }
  popup_showing_ = showing;
  UpdateVisuals();
}

bool AstraExtensionIconView::popup_showing() const {
  return popup_showing_;
}

// =========================================================================
// Current / selected
// =========================================================================

void AstraExtensionIconView::SetIsCurrent(bool current) {
  if (is_current_ == current) {
    return;
  }
  is_current_ = current;
  // TODO(astra): Show selected/active background for current extension.
  SchedulePaint();
}

bool AstraExtensionIconView::IsCurrent() const {
  return is_current_;
}

// =========================================================================
// Notification badge
// =========================================================================

void AstraExtensionIconView::SetNotificationCount(int count) {
  if (notification_count_ == count) {
    return;
  }
  notification_count_ = count;
  // Update badge text to show the count.
  if (count > 0) {
    // Truncate large counts (e.g., "99+" for counts over 99).
    if (count > 99) {
      SetBadgeText(u"99+");
    } else {
      SetBadgeText(base::NumberToString16(count));
    }
  } else {
    SetBadgeText(std::u16string());
  }
}

int AstraExtensionIconView::GetNotificationCount() const {
  return notification_count_;
}

void AstraExtensionIconView::SetShowNotificationBadge(bool show) {
  show_notification_badge_ = show;
  // TODO(astra): Show/hide notification badge based on this flag.
  SchedulePaint();
}

bool AstraExtensionIconView::GetShowNotificationBadge() const {
  return show_notification_badge_;
}

// =========================================================================
// Loading state
// =========================================================================

void AstraExtensionIconView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  // TODO(astra): Show loading spinner animation over the icon.
  //   Chromium pattern: Throbber or Spinner views.
  SchedulePaint();
}

bool AstraExtensionIconView::IsLoading() const {
  return is_loading_;
}

// =========================================================================
// Error state
// =========================================================================

void AstraExtensionIconView::SetInErrorState(bool error) {
  if (is_in_error_state_ == error) {
    return;
  }
  is_in_error_state_ = error;
  // TODO(astra): Show error indicator (e.g., warning icon overlay).
  //   Chromium pattern: Extension error badging in toolbar.
  SchedulePaint();
}

bool AstraExtensionIconView::IsInErrorState() const {
  return is_in_error_state_;
}

// =========================================================================
// Visuals
// =========================================================================

void AstraExtensionIconView::UpdateVisuals() {
  SetOpacity(GetEffectiveOpacity());

  // Enable/disable interaction based on state.
  bool interactive = extension_state_ == AstraExtensionState::kEnabled ||
                     extension_state_ == AstraExtensionState::kError;
  SetEnabled(interactive);

  SchedulePaint();
}

float AstraExtensionIconView::GetEffectiveOpacity() const {
  // Popup showing overrides with full opacity.
  if (popup_showing_) {
    return kPopupShowingOpacity;
  }

  switch (extension_state_) {
    case AstraExtensionState::kEnabled:
      return kEnabledOpacity;
    case AstraExtensionState::kDisabled:
      return kDisabledOpacity;
    case AstraExtensionState::kBlocked:
      return kBlockedOpacity;
    case AstraExtensionState::kError:
      return kErrorOpacity;
    case AstraExtensionState::kUninstalled:
      return kUninstalledOpacity;
  }
  return kEnabledOpacity;
}

void AstraExtensionIconView::UpdateTooltip() {
  if (!show_tooltip_) {
    return;
  }
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
  int size = icon_size_ + 2 * kIconPadding;
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
  if (event.IsMiddleMouseButton()) {
    // Middle click: handled on release.
    return true;
  }
  return views::ImageButton::OnMousePressed(event);
}

void AstraExtensionIconView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsRightMouseButton() && HitTestPoint(event.location())) {
    if (show_context_menu_) {
      gfx::Point screen_point = event.location();
      ConvertPointToScreen(this, &screen_point);
      ShowContextMenu(screen_point);
    }
    return;
  }
  if (event.IsMiddleMouseButton() && HitTestPoint(event.location())) {
    HandleMiddleClickAction();
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
  // Fire the legacy callback for backward compatibility.
  delegate_->OnExtensionIconClicked(extension_id_, this);
  // Fire the new unified callback.
  delegate_->OnExtensionClicked(extension_id_);
}

void AstraExtensionIconView::HandleMiddleClickAction() {
  if (!delegate_) {
    return;
  }
  delegate_->OnExtensionMiddleClicked(extension_id_);
}

void AstraExtensionIconView::ShowContextMenu(const gfx::Point& screen_point) {
  if (!delegate_) {
    return;
  }
  // Fire the legacy callback for backward compatibility.
  delegate_->OnExtensionIconContextMenu(extension_id_, screen_point);
  // Fire the new unified callback.
  delegate_->OnExtensionRightClicked(extension_id_, screen_point);
}

// =========================================================================
// Metadata
// =========================================================================

BEGIN_METADATA(AstraExtensionIconView)
END_METADATA

}  // namespace astra
