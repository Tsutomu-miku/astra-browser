#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view.h"

#include "astra/ui/views/sidebar/astra_sidebar_drag_types.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSidebarItemHeight = 32;
constexpr int kSidebarItemHorizontalPadding = 12;
constexpr int kSidebarItemVerticalPadding = 4;
constexpr int kSidebarItemIconSpacing = 8;
constexpr int kSidebarItemIconSize = 16;
constexpr int kSidebarItemDragHandleSize = 10;
constexpr int kSidebarItemCornerRadius = 6;
constexpr int kSidebarItemTrailingSpacing = 6;
constexpr int kSidebarBadgeMinWidth = 16;
constexpr int kSidebarBadgeHorizontalPadding = 4;

// Audio indicator layout.
constexpr int kAudioIndicatorSize = 16;

// Astra color IDs for sidebar items.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kSidebarItemTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kSidebarItemSelectedTextColorId =
    kColorAstraSidebarItemSelectedText;
constexpr ui::ColorId kSidebarItemActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kSidebarItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kSidebarItemSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kSidebarItemSelectedBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kSidebarDropTargetBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kSidebarBadgeBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kSidebarBadgeTextColorId =
    kColorAstraSidebarItemSelectedText;

// Minimum distance (in DIPs) the mouse must move after press before a
// drag gesture is recognized.
// Chromium DnD subsystem: ui/base/dragdrop/drag_utils.h
constexpr int kDragThresholdDips = 8;

}  // namespace

AstraSidebarItemView::AstraSidebarItemView() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kSidebarItemCornerRadius));

  SetFocusBehavior(FocusBehavior::ALWAYS);

  BuildLayout();
}

AstraSidebarItemView::AstraSidebarItemView(const std::u16string& title,
                                           Type type)
    : type_(type) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kSidebarItemCornerRadius));

  SetFocusBehavior(FocusBehavior::ALWAYS);

  BuildLayout();
  SetTitle(title);
}

AstraSidebarItemView::~AstraSidebarItemView() = default;

// =========================================================================
// Type
// =========================================================================

void AstraSidebarItemView::SetType(Type type) {
  if (type_ == type) {
    return;
  }
  type_ = type;
  UpdateVisuals();
  InvalidateLayout();
}

void AstraSidebarItemView::BuildLayout() {
  // Main horizontal layout: [drag_handle] [icon] [text_container] [trailing_container]
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(kSidebarItemVerticalPadding,
                      kSidebarItemHorizontalPadding),
      kSidebarItemIconSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Drag handle (initially hidden).
  drag_handle_view_ = AddChildView(std::make_unique<views::ImageView>());
  drag_handle_view_->SetPreferredSize(
      gfx::Size(kSidebarItemDragHandleSize, kSidebarItemDragHandleSize));
  drag_handle_view_->SetCanProcessEventsWithinSubtree(false);
  drag_handle_view_->SetVisible(false);
  drag_handle_view_->SetAccessibleName(u"Drag to reorder");

  // Leading icon.
  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetPreferredSize(
      gfx::Size(kSidebarItemIconSize, kSidebarItemIconSize));
  icon_view_->SetCanProcessEventsWithinSubtree(false);
  icon_view_->SetVisible(false);  // Hidden by default; shown when icon is set.

  // Text container: title + secondary (vertical stack).
  text_container_ = AddChildView(std::make_unique<views::View>());
  auto* text_layout = text_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  text_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->SetFlexForView(text_container_, 1);

  // Title label.
  title_label_ = text_container_->AddChildView(
      std::make_unique<views::Label>());
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Secondary label (subtitle). Hidden by default.
  secondary_label_ = text_container_->AddChildView(
      std::make_unique<views::Label>());
  secondary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  secondary_label_->SetAutoColorReadabilityEnabled(false);
  secondary_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  secondary_label_->SetFontList(
      secondary_label_->font_list().DeriveWithSizeDelta(-1));
  secondary_label_->SetVisible(false);

  // Trailing container: trailing icon, badge, chevron (horizontal).
  trailing_container_ = AddChildView(std::make_unique<views::View>());
  auto* trailing_layout = trailing_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kSidebarItemTrailingSpacing));
  trailing_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Trailing icon. Hidden by default.
  trailing_icon_view_ = trailing_container_->AddChildView(
      std::make_unique<views::ImageView>());
  trailing_icon_view_->SetPreferredSize(
      gfx::Size(kSidebarItemIconSize, kSidebarItemIconSize));
  trailing_icon_view_->SetCanProcessEventsWithinSubtree(false);
  trailing_icon_view_->SetVisible(false);

  // Badge label. Hidden by default.
  badge_label_ = trailing_container_->AddChildView(
      std::make_unique<views::Label>());
  badge_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  badge_label_->SetAutoColorReadabilityEnabled(false);
  badge_label_->SetFontList(
      badge_label_->font_list().DeriveWithSizeDelta(-1));
  badge_label_->SetVisible(false);
  badge_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kSidebarBadgeHorizontalPadding)));

  // Chevron view. Hidden by default.
  chevron_view_ = trailing_container_->AddChildView(
      std::make_unique<views::ImageView>());
  chevron_view_->SetPreferredSize(
      gfx::Size(kSidebarItemIconSize, kSidebarItemIconSize));
  chevron_view_->SetCanProcessEventsWithinSubtree(false);
  chevron_view_->SetVisible(false);

  // Audio indicator button. Hidden by default.
  audio_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarItemView::OnAudioButtonClicked,
                          base::Unretained(this))));
  audio_button_->SetImageHorizontalAlignment(
      views::ImageButton::ALIGN_CENTER);
  audio_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  audio_button_->SetVisible(false);
  audio_button_->SetFocusBehavior(FocusBehavior::NEVER);
  audio_button_->SetPreferredSize(
      gfx::Size(kAudioIndicatorSize, kAudioIndicatorSize));
}

// =========================================================================
// Title
// =========================================================================

void AstraSidebarItemView::SetTitle(const std::u16string& title) {
  if (title_label_) {
    title_label_->SetText(title);
  }
  // Also update tooltip if not explicitly set?
  // For now, we don't auto-update tooltip from title since tooltip may be
  // set independently.
}

std::u16string AstraSidebarItemView::GetTitle() const {
  if (title_label_) {
    return title_label_->GetText();
  }
  return std::u16string();
}

// =========================================================================
// Tooltip
// =========================================================================

void AstraSidebarItemView::SetTooltip(const std::u16string& tooltip) {
  views::View::SetTooltipText(tooltip);
}

std::u16string AstraSidebarItemView::GetTooltip() const {
  return views::View::GetTooltipText(gfx::Point());
}

// =========================================================================
// Selection state
// =========================================================================

void AstraSidebarItemView::SetSelected(bool selected) {
  if (is_selected_ == selected) {
    return;
  }
  is_selected_ = selected;
  UpdateVisuals();
}

// =========================================================================
// Hover state
// =========================================================================

void AstraSidebarItemView::SetHovered(bool hovered) {
  if (is_hovered_ == hovered) {
    return;
  }
  is_hovered_ = hovered;
  UpdateVisuals();
}

// =========================================================================
// Active state
// =========================================================================

void AstraSidebarItemView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  UpdateVisuals();
}

// =========================================================================
// Enabled state
// =========================================================================

void AstraSidebarItemView::SetEnabled(bool enabled) {
  if (is_enabled_ == enabled) {
    return;
  }
  is_enabled_ = enabled;
  views::View::SetEnabled(enabled);
  UpdateVisuals();
}

// =========================================================================
// Context menu
// =========================================================================

void AstraSidebarItemView::SetContextMenuEnabled(bool enabled) {
  context_menu_enabled_ = enabled;
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarItemView::SetDragEnabled(bool enabled) {
  drag_enabled_ = enabled;
}

void AstraSidebarItemView::SetDropTarget(bool is_target) {
  if (is_drop_target_ == is_target) {
    return;
  }
  is_drop_target_ = is_target;
  UpdateVisuals();
}

// =========================================================================
// Drag handle
// =========================================================================

void AstraSidebarItemView::SetShowDragHandle(bool show) {
  if (show_drag_handle_ == show) {
    return;
  }
  show_drag_handle_ = show;
  if (drag_handle_view_) {
    drag_handle_view_->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// Leading icon
// =========================================================================

void AstraSidebarItemView::SetIcon(const gfx::ImageSkia& icon) {
  if (icon_view_) {
    icon_view_->SetImage(icon);
    icon_view_->SetVisible(!icon.isNull());
    InvalidateLayout();
  }
}

void AstraSidebarItemView::ClearIcon() {
  if (icon_view_) {
    icon_view_->SetImage(gfx::ImageSkia());
    icon_view_->SetVisible(false);
    InvalidateLayout();
  }
}

// =========================================================================
// Trailing icon
// =========================================================================

void AstraSidebarItemView::SetTrailingIcon(const gfx::ImageSkia& icon) {
  if (trailing_icon_view_) {
    trailing_icon_view_->SetImage(icon);
    if (!icon.isNull()) {
      trailing_icon_view_->SetVisible(true);
    }
    InvalidateLayout();
  }
}

void AstraSidebarItemView::ShowTrailingIcon(bool show) {
  if (trailing_icon_view_) {
    trailing_icon_view_->SetVisible(show);
    InvalidateLayout();
  }
}

// =========================================================================
// Secondary text
// =========================================================================

void AstraSidebarItemView::SetSecondaryText(const std::u16string& text) {
  if (secondary_label_) {
    secondary_label_->SetText(text);
    if (!text.empty()) {
      secondary_label_->SetVisible(true);
    }
    InvalidateLayout();
  }
}

void AstraSidebarItemView::ShowSecondaryText(bool show) {
  if (secondary_label_) {
    secondary_label_->SetVisible(show);
    InvalidateLayout();
  }
}

// =========================================================================
// Badge
// =========================================================================

void AstraSidebarItemView::SetBadgeText(const std::u16string& text) {
  if (badge_label_) {
    badge_label_->SetText(text);
    if (!text.empty()) {
      badge_label_->SetVisible(true);
    }
    InvalidateLayout();
  }
}

void AstraSidebarItemView::ShowBadge(bool show) {
  if (badge_label_) {
    badge_label_->SetVisible(show);
    InvalidateLayout();
  }
}

// =========================================================================
// Chevron
// =========================================================================

void AstraSidebarItemView::SetChevronVisible(bool visible) {
  if (chevron_visible_ == visible && chevron_view_->GetVisible() == visible) {
    return;
  }
  chevron_visible_ = visible;
  if (chevron_view_) {
    chevron_view_->SetVisible(visible);
  }
  InvalidateLayout();
}

void AstraSidebarItemView::SetChevronRotated(bool rotated) {
  if (chevron_rotated_ == rotated) {
    return;
  }
  chevron_rotated_ = rotated;
  // TODO(astra): Rotate the chevron icon image.
  //   For now, we just update the state; visual rotation will be
  //   implemented with proper vector icons.
  //   Chromium pattern: gfx::Transform for rotation.
  if (chevron_view_) {
    // Placeholder: update visual state.
    chevron_view_->SchedulePaint();
  }
}

// =========================================================================
// Compact mode
// =========================================================================

void AstraSidebarItemView::SetCompactMode(bool compact) {
  if (is_compact_ == compact) {
    return;
  }
  is_compact_ = compact;

  // In compact mode, hide text elements and show only the icon.
  if (text_container_) {
    text_container_->SetVisible(!compact);
  }
  if (trailing_container_) {
    // In compact mode, hide most trailing elements except badges.
    // Badges can still be shown as small overlays.
    if (chevron_view_) {
      chevron_view_->SetVisible(chevron_visible_ && !compact);
    }
  }
  if (secondary_label_) {
    secondary_label_->SetVisible(!compact);
  }
  if (drag_handle_view_) {
    drag_handle_view_->SetVisible(show_drag_handle_ && !compact);
  }

  UpdateVisuals();
  InvalidateLayout();
}

// =========================================================================
// Tooltip preview
// =========================================================================

void AstraSidebarItemView::SetDetailedTooltip(const std::u16string& title,
                                              const std::u16string& subtitle) {
  // Build a detailed tooltip with title and optional subtitle.
  std::u16string tooltip_text = title;
  if (!subtitle.empty()) {
    tooltip_text += u"\n" + subtitle;
  }
  SetTooltip(tooltip_text);
}

void AstraSidebarItemView::SetShowTooltipPreview(bool show) {
  show_tooltip_preview_ = show;
  // TODO(astra): When tooltip preview is enabled, show a rich preview
  //   bubble on hover (similar to tab hover cards). When disabled, show
  //   a standard tooltip only. The rich preview would include the full
  //   title, URL, favicon, and action buttons.
  //   Chromium pattern: TabHoverCardController
  //   (chrome/browser/ui/tabs/tab_hover_card_controller.h)
}

// =========================================================================
// Audio indicator
// =========================================================================

void AstraSidebarItemView::SetAudioState(AudioState state) {
  if (audio_state_ == state) {
    return;
  }
  audio_state_ = state;

  const bool has_audio = state != AudioState::kNone;
  if (audio_button_) {
    audio_button_->SetVisible(has_audio);
  }

  UpdateAudioButtonVisuals();
  InvalidateLayout();
}

void AstraSidebarItemView::UpdateAudioButtonVisuals() {
  if (!audio_button_) {
    return;
  }

  // TODO(astra): Use actual Chromium tab audio vector icons from
  //   ui/resources/vector_icons/.
  //   Chromium owner: TabRenderer (chrome/browser/ui/views/tabs/tab_renderer.h)

  // Set tooltip text based on audio state.
  switch (audio_state_) {
    case AudioState::kPlaying:
      audio_button_->SetTooltipText(u"Mute tab");
      break;
    case AudioState::kMuted:
      audio_button_->SetTooltipText(u"Unmute tab");
      break;
    case AudioState::kNone:
      audio_button_->SetTooltipText(std::u16string());
      break;
  }
}

void AstraSidebarItemView::OnAudioButtonClicked() {
  if (audio_toggle_callback_) {
    audio_toggle_callback_.Run();
  }
}

// =========================================================================
// Suspended state
// =========================================================================

void AstraSidebarItemView::SetSuspendedState(SuspendedState state) {
  if (suspended_state_ == state) {
    return;
  }
  suspended_state_ = state;

  // Update tooltip to indicate suspended state.
  if (state == SuspendedState::kSuspended) {
    SetTooltip(u"Suspended — click to reload");
  }

  UpdateVisuals();
  InvalidateLayout();
}

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarItemView::Layout() {
  views::View::Layout();

  // Position the audio button on the trailing edge if visible.
  if (audio_button_ && audio_button_->GetVisible()) {
    int x = width() - kSidebarItemHorizontalPadding - kAudioIndicatorSize;
    int y = (height() - kAudioIndicatorSize) / 2;
    audio_button_->SetBounds(x, y, kAudioIndicatorSize, kAudioIndicatorSize);
  }
}

gfx::Size AstraSidebarItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::View::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kSidebarItemHeight));
  return size;
}

// =========================================================================
// Visuals
// =========================================================================

void AstraSidebarItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
}

void AstraSidebarItemView::UpdateVisuals() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Text colors.
  SkColor text_color = color_provider->GetColor(kSidebarItemTextColorId);

  // Selected/active items use selected text color.
  if (is_selected_ || is_active_) {
    text_color = color_provider->GetColor(kSidebarItemSelectedTextColorId);
  }

  // Disabled items have dimmed text.
  if (!is_enabled_) {
    text_color = SkColorSetA(text_color, SkColorGetA(text_color) / 2);
  }

  // Suspended tabs get faded text.
  if (suspended_state_ == SuspendedState::kSuspended) {
    text_color = SkColorSetA(text_color, SkColorGetA(text_color) / 2);
  }

  if (title_label_) {
    title_label_->SetEnabledColor(text_color);
  }

  if (secondary_label_) {
    SkColor secondary_color =
        color_provider->GetColor(kSidebarItemSecondaryTextColorId);
    if (!is_enabled_) {
      secondary_color = SkColorSetA(secondary_color,
                                    SkColorGetA(secondary_color) / 2);
    }
    secondary_label_->SetEnabledColor(secondary_color);
  }

  // Badge colors.
  if (badge_label_) {
    badge_label_->SetEnabledColor(
        color_provider->GetColor(kSidebarBadgeTextColorId));
    // TODO(astra): Add background to badge label.
    //   For now, just text color.
  }

  // Background color.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kSidebarItemActiveBgColorId);
  } else if (is_selected_) {
    bg_color = color_provider->GetColor(kSidebarItemSelectedBgColorId);
  } else if (is_hovered_) {
    bg_color = color_provider->GetColor(kSidebarItemHoverBgColorId);
  } else if (is_drop_target_) {
    bg_color = color_provider->GetColor(kSidebarDropTargetBgColorId);
  }

  if (layer()) {
    layer()->SetColor(bg_color);
  }

  // Refresh audio indicator visuals.
  UpdateAudioButtonVisuals();
}

void AstraSidebarItemView::UpdateTooltip() {
  // Base class has no special tooltip logic beyond what's set via SetTooltip.
  // Subclasses may override to compose tooltips from item data.
}

// =========================================================================
// Mouse events
// =========================================================================

bool AstraSidebarItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (!is_enabled_) {
    return false;
  }

  // Right-click: context menu.
  if (event.IsRightMouseButton() && context_menu_enabled_) {
    return true;  // Will show context menu on release.
  }

  // Drag initiation (left button only).
  if (drag_enabled_ && event.IsOnlyLeftMouseButton()) {
    drag_start_point_ = event.location();
    is_dragging_ = false;
  }

  return true;
}

bool AstraSidebarItemView::OnMouseDragged(const ui::MouseEvent& event) {
  if (!drag_enabled_ || is_dragging_ || !is_enabled_) {
    return views::View::OnMouseDragged(event);
  }

  // Check if the mouse has moved beyond the drag threshold.
  gfx::Vector2d delta = event.location() - drag_start_point_;
  if (abs(delta.x()) >= kDragThresholdDips ||
      abs(delta.y()) >= kDragThresholdDips) {
    is_dragging_ = true;

    if (drag_delegate_) {
      drag_delegate_->OnItemDragStarted(this, event.location());
    }
  }

  return true;
}

void AstraSidebarItemView::OnMouseReleased(const ui::MouseEvent& event) {
  // Right-click: show context menu.
  if (event.IsRightMouseButton() && context_menu_enabled_) {
    if (HitTestPoint(event.location())) {
      gfx::Point screen_point = event.location();
      ConvertPointToScreen(this, &screen_point);
      ShowContextMenu(screen_point);
    }
    return;
  }

  // If we were dragging, don't trigger click.
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  // Left click: primary action.
  if (event.IsOnlyLeftMouseButton() &&
      GetLocalBounds().Contains(event.location())) {
    OnItemClicked();
  }
}

void AstraSidebarItemView::OnMouseCaptureLost() {
  is_dragging_ = false;
  views::View::OnMouseCaptureLost();
}

void AstraSidebarItemView::OnMouseEntered(const ui::MouseEvent& event) {
  if (!is_enabled_) {
    return;
  }

  is_hovered_ = true;
  UpdateVisuals();

  if (hover_delegate_ && !is_dragging_) {
    hover_delegate_->OnItemHoverStarted(this, event.location());
  }
}

void AstraSidebarItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  UpdateVisuals();

  if (hover_delegate_) {
    hover_delegate_->OnItemHoverEnded(this);
  }
}

bool AstraSidebarItemView::OnMouseMoved(const ui::MouseEvent& event) {
  if (is_hovered_ && hover_delegate_ && !is_dragging_) {
    hover_delegate_->OnItemHoverMoved(this, event.location());
  }
  return views::View::OnMouseMoved(event);
}

bool AstraSidebarItemView::OnKeyPressed(const ui::KeyEvent& event) {
  if (!is_enabled_) {
    return false;
  }

  // Space or Enter: activate the item.
  if (event.key_code() == ui::VKEY_SPACE ||
      event.key_code() == ui::VKEY_RETURN) {
    OnItemClicked();
    return true;
  }

  // Menu key or Shift+F10: context menu.
  if ((event.key_code() == ui::VKEY_APPS ||
       (event.key_code() == ui::VKEY_F10 &&
        event.IsShiftDown())) &&
      context_menu_enabled_) {
    gfx::Point screen_point;
    ConvertPointToScreen(this, &screen_point);
    screen_point.Offset(0, height() / 2);
    ShowContextMenu(screen_point);
    return true;
  }

  return views::View::OnKeyPressed(event);
}

// =========================================================================
// Action handlers (virtual, can be overridden by subclasses)
// =========================================================================

void AstraSidebarItemView::OnItemClicked() {
  // Base class has no default click action.
  // Subclasses should override to handle clicks.
  // If a click callback is set, invoke it.
  if (click_callback_) {
    click_callback_.Run();
  }
}

void AstraSidebarItemView::ShowContextMenu(const gfx::Point& screen_point) {
  if (context_menu_delegate_) {
    context_menu_delegate_->OnItemContextMenuRequested(this, screen_point);
  }
}

}  // namespace astra
