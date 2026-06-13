#include "astra/ui/views/sidebar/astra_sidebar_stack_tab_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Drag threshold (minimum mouse movement to start a drag).
constexpr int kDragThresholdDips = 8;

// Astra color IDs for stack tab item styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kTabItemTextColorId = kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kTabItemActiveTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kTabItemActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kTabItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kTabItemDragHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraSidebarStackTabItemView::AstraSidebarStackTabItemView(
    const std::u16string& title)
    : title_(title) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  UpdateCornerRadius();

  // Favicon view — placeholder for now.
  // TODO(astra): Wire up real favicon via chrome/browser/ui/views/tab_icon_view.h
  //   once the sidebar is connected to TabStripModel.
  favicon_view_ = AddChildView(std::make_unique<views::ImageView>());
  favicon_view_->SetVisible(false);

  // Title label.
  title_label_ = AddChildView(std::make_unique<views::Label>(title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Audio indicator button — shows on the trailing edge when audio is playing.
  audio_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          &AstraSidebarStackTabItemView::OnAudioButtonClicked,
          base::Unretained(this))));
  audio_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  audio_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  audio_button_->SetVisible(false);
  audio_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);

  // Close button — shows on hover.
  close_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          &AstraSidebarStackTabItemView::OnCloseButtonClicked,
          base::Unretained(this))));
  close_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  close_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  close_button_->SetVisible(false);
  close_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  close_button_->SetTooltipText(u"Close tab");
}

AstraSidebarStackTabItemView::~AstraSidebarStackTabItemView() = default;

// =========================================================================
// Tab info
// =========================================================================

void AstraSidebarStackTabItemView::SetTabInfo(const AstraStackTabInfo& info) {
  tab_id_ = info.tab_id;
  title_ = info.title;
  url_ = info.url;
  is_active_ = info.is_active;
  is_pinned_ = info.is_pinned;
  is_audible_ = info.is_audible;
  is_muted_ = info.is_muted;
  is_loading_ = info.is_loading;
  is_crashed_ = info.is_crashed;
  favicon_ = info.favicon;
  has_favicon_ = info.has_favicon;
  index_in_stack_ = info.index_in_stack;

  // Update derived audio state.
  if (is_muted_) {
    audio_state_ = AudioState::kMuted;
  } else if (is_audible_) {
    audio_state_ = AudioState::kPlaying;
  } else {
    audio_state_ = AudioState::kNone;
  }

  // Update all visual elements.
  if (title_label_) {
    title_label_->SetText(title_);
  }

  if (favicon_view_ && has_favicon_) {
    favicon_view_->SetImage(favicon_);
  }

  SetTooltipText(url_.spec());
  UpdateFaviconVisibility();
  UpdateAudioButtonVisuals();
  UpdateBackgroundColor();
  UpdateCornerRadius();

  // Update audio button visibility.
  if (audio_button_) {
    audio_button_->SetVisible(audio_state_ != AudioState::kNone);
  }

  InvalidateLayout();
}

// =========================================================================
// Title
// =========================================================================

void AstraSidebarStackTabItemView::SetTitle(const std::u16string& title) {
  title_ = title;
  if (title_label_) {
    title_label_->SetText(title);
  }
}

std::u16string AstraSidebarStackTabItemView::GetTitle() const {
  if (title_label_) {
    return title_label_->GetText();
  }
  return title_;
}

// =========================================================================
// URL
// =========================================================================

void AstraSidebarStackTabItemView::SetUrl(const GURL& url) {
  url_ = url;
  SetTooltipText(url.spec());
}

// =========================================================================
// Favicon
// =========================================================================

void AstraSidebarStackTabItemView::SetFavicon(const gfx::ImageSkia& favicon) {
  favicon_ = favicon;
  has_favicon_ = true;
  if (favicon_view_) {
    favicon_view_->SetImage(favicon);
  }
  UpdateFaviconVisibility();
  InvalidateLayout();
}

void AstraSidebarStackTabItemView::SetHasFavicon(bool has_favicon) {
  if (has_favicon_ == has_favicon) {
    return;
  }
  has_favicon_ = has_favicon;
  UpdateFaviconVisibility();
  InvalidateLayout();
}

void AstraSidebarStackTabItemView::UpdateFaviconVisibility() {
  if (!favicon_view_) {
    return;
  }
  bool visible = show_favicon_ && has_favicon_;
  favicon_view_->SetVisible(visible);
}

// =========================================================================
// Active state
// =========================================================================

void AstraSidebarStackTabItemView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  UpdateBackgroundColor();
}

// =========================================================================
// Close button
// =========================================================================

void AstraSidebarStackTabItemView::SetCloseButtonVisible(bool visible) {
  if (!close_button_) {
    return;
  }
  if (!show_close_button_) {
    close_button_->SetVisible(false);
    return;
  }
  close_button_->SetVisible(visible);
}

bool AstraSidebarStackTabItemView::IsCloseButtonVisible() const {
  return close_button_ && close_button_->GetVisible();
}

void AstraSidebarStackTabItemView::SetShowCloseButton(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  if (!show && close_button_) {
    close_button_->SetVisible(false);
  }
  InvalidateLayout();
}

// =========================================================================
// Favicon visibility
// =========================================================================

void AstraSidebarStackTabItemView::SetShowFavicon(bool show) {
  if (show_favicon_ == show) {
    return;
  }
  show_favicon_ = show;
  UpdateFaviconVisibility();
  InvalidateLayout();
}

// =========================================================================
// Drag state
// =========================================================================

void AstraSidebarStackTabItemView::SetIsDragging(bool dragging) {
  if (is_dragging_ == dragging) {
    return;
  }
  is_dragging_ = dragging;
  SchedulePaint();
}

void AstraSidebarStackTabItemView::SetDragHovered(bool hovered) {
  if (is_drag_hovered_ == hovered) {
    return;
  }
  is_drag_hovered_ = hovered;
  UpdateBackgroundColor();
}

// =========================================================================
// Index
// =========================================================================

void AstraSidebarStackTabItemView::SetIndex(int index) {
  index_in_stack_ = index;
}

// =========================================================================
// Stack ID
// =========================================================================

void AstraSidebarStackTabItemView::SetStackId(const std::string& stack_id) {
  stack_id_ = stack_id;
}

// =========================================================================
// First/last in stack
// =========================================================================

void AstraSidebarStackTabItemView::SetIsFirst(bool first) {
  if (is_first_ == first) {
    return;
  }
  is_first_ = first;
  UpdateCornerRadius();
}

void AstraSidebarStackTabItemView::SetIsLast(bool last) {
  if (is_last_ == last) {
    return;
  }
  is_last_ = last;
  UpdateCornerRadius();
}

void AstraSidebarStackTabItemView::UpdateCornerRadius() {
  if (!layer()) {
    return;
  }

  float tl = 0, tr = 0, bl = 0, br = 0;
  if (is_first_) {
    tl = kTabItemCornerRadius;
    tr = kTabItemCornerRadius;
  }
  if (is_last_) {
    bl = kTabItemCornerRadius;
    br = kTabItemCornerRadius;
  }
  // If neither first nor last, no rounded corners.
  // If both, all corners rounded.
  layer()->SetRoundedCornerRadius(gfx::RoundedCornersF(tl, tr, br, bl));
}

// =========================================================================
// Pinned
// =========================================================================

void AstraSidebarStackTabItemView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  // TODO(astra): Update visual indicator for pinned tabs.
  SchedulePaint();
}

// =========================================================================
// Audio state
// =========================================================================

void AstraSidebarStackTabItemView::SetIsAudible(bool audible) {
  if (is_audible_ == audible) {
    return;
  }
  is_audible_ = audible;

  // Update derived audio state.
  if (is_muted_) {
    audio_state_ = AudioState::kMuted;
  } else if (is_audible_) {
    audio_state_ = AudioState::kPlaying;
  } else {
    audio_state_ = AudioState::kNone;
  }

  if (audio_button_) {
    audio_button_->SetVisible(audio_state_ != AudioState::kNone);
  }
  UpdateAudioButtonVisuals();
  InvalidateLayout();
}

void AstraSidebarStackTabItemView::SetIsMuted(bool muted) {
  if (is_muted_ == muted) {
    return;
  }
  is_muted_ = muted;

  // Update derived audio state.
  if (is_muted_) {
    audio_state_ = AudioState::kMuted;
  } else if (is_audible_) {
    audio_state_ = AudioState::kPlaying;
  } else {
    audio_state_ = AudioState::kNone;
  }

  if (audio_button_) {
    audio_button_->SetVisible(audio_state_ != AudioState::kNone);
  }
  UpdateAudioButtonVisuals();
  InvalidateLayout();
}

void AstraSidebarStackTabItemView::SetAudioState(AudioState state) {
  if (audio_state_ == state) {
    return;
  }
  audio_state_ = state;

  // Sync boolean flags.
  is_audible_ = (state == AudioState::kPlaying);
  is_muted_ = (state == AudioState::kMuted);

  if (audio_button_) {
    audio_button_->SetVisible(state != AudioState::kNone);
  }

  UpdateAudioButtonVisuals();
  InvalidateLayout();
}

// =========================================================================
// Loading
// =========================================================================

void AstraSidebarStackTabItemView::SetIsLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  // TODO(astra): Show loading spinner/throbber.
  //   Chromium owner: TabThrobber (chrome/browser/ui/views/tabs/tab_throbber.h)
  SchedulePaint();
}

// =========================================================================
// Crashed
// =========================================================================

void AstraSidebarStackTabItemView::SetIsCrashed(bool crashed) {
  if (is_crashed_ == crashed) {
    return;
  }
  is_crashed_ = crashed;
  // TODO(astra): Show sad tab / crash indicator.
  //   Chromium owner: SadTabView (chrome/browser/ui/views/sad_tab_view.h)
  SchedulePaint();
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarStackTabItemView::SetDraggable(bool draggable) {
  draggable_ = draggable;
}

// =========================================================================
// Layout
// =========================================================================

gfx::Size AstraSidebarStackTabItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = 0;
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  return gfx::Size(width, kTabItemHeight);
}

void AstraSidebarStackTabItemView::Layout() {
  views::View::Layout();

  // Start after the indent + padding.
  int x = kTabItemIndent + kHorizontalPadding;
  int y_center = height() / 2;

  // Favicon (if visible).
  if (favicon_view_ && favicon_view_->GetVisible()) {
    int icon_y = y_center - kIconSize / 2;
    favicon_view_->SetBounds(x, icon_y, kIconSize, kIconSize);
    x += kIconSize + kIconSpacing;
  }

  // Title fills the available space.
  if (title_label_) {
    int title_x = x;
    int title_width = width() - title_x - kHorizontalPadding;

    // Reserve space for audio button if visible.
    if (audio_button_ && audio_button_->GetVisible()) {
      title_width -= kCloseButtonSize + kIconSpacing;
    }

    // Reserve space for close button if visible.
    if (close_button_ && close_button_->GetVisible()) {
      title_width -= kCloseButtonSize + kIconSpacing;
    }

    title_label_->SetBounds(title_x, 0, title_width, height());
  }

  // Audio button on the trailing edge.
  if (audio_button_ && audio_button_->GetVisible()) {
    int btn_x = width() - kHorizontalPadding - kCloseButtonSize;
    // Adjust if close button is also visible.
    if (close_button_ && close_button_->GetVisible()) {
      btn_x -= kCloseButtonSize + kIconSpacing;
    }
    int btn_y = y_center - kCloseButtonSize / 2;
    audio_button_->SetBounds(btn_x, btn_y, kCloseButtonSize, kCloseButtonSize);
  }

  // Close button on the trailing edge.
  if (close_button_ && close_button_->GetVisible()) {
    int btn_x = width() - kHorizontalPadding - kCloseButtonSize;
    int btn_y = y_center - kCloseButtonSize / 2;
    close_button_->SetBounds(btn_x, btn_y, kCloseButtonSize, kCloseButtonSize);
  }
}

// =========================================================================
// Theme
// =========================================================================

void AstraSidebarStackTabItemView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update text color. Active tabs use primary text color; inactive use
  // secondary to visually de-emphasize child tabs.
  if (title_label_) {
    ui::ColorId text_color_id =
        is_active_ ? kTabItemActiveTextColorId : kTabItemTextColorId;
    title_label_->SetEnabledColor(color_provider->GetColor(text_color_id));
  }

  UpdateBackgroundColor();
}

void AstraSidebarStackTabItemView::UpdateBackgroundColor() {
  if (!layer()) {
    return;
  }

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kTabItemActiveBgColorId);
  } else if (is_drag_hovered_) {
    bg_color = color_provider->GetColor(kTabItemDragHoverBgColorId);
  } else if (is_hovered_) {
    bg_color = color_provider->GetColor(kTabItemHoverBgColorId);
  }
  layer()->SetColor(bg_color);
}

// =========================================================================
// Mouse events
// =========================================================================

bool AstraSidebarStackTabItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (!draggable_ || event.flags() & ui::EF_RIGHT_MOUSE_BUTTON) {
    return views::View::OnMousePressed(event);
  }

  // Only left button starts a potential drag.
  if (event.IsOnlyLeftMouseButton()) {
    drag_start_point_ = event.location();
    is_dragging_ = false;
  }

  return true;
}

bool AstraSidebarStackTabItemView::OnMouseDragged(
    const ui::MouseEvent& event) {
  if (!draggable_ || is_dragging_) {
    return views::View::OnMouseDragged(event);
  }

  // Check if the mouse has moved beyond the drag threshold.
  gfx::Vector2d delta = event.location() - drag_start_point_;
  if (abs(delta.x()) >= kDragThresholdDips ||
      abs(delta.y()) >= kDragThresholdDips) {
    is_dragging_ = true;

    // Notify the delegate.
    if (delegate_ && web_contents_) {
      delegate_->OnStackTabDragStarted(web_contents_, event.location());
    }
  }

  return true;
}

void AstraSidebarStackTabItemView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  // Click — activate the tab.
  if (event.IsOnlyLeftMouseButton() && delegate_ && web_contents_) {
    delegate_->OnStackTabClicked(web_contents_);
  }

  views::View::OnMouseReleased(event);
}

void AstraSidebarStackTabItemView::OnMouseCaptureLost() {
  is_dragging_ = false;
  views::View::OnMouseCaptureLost();
}

void AstraSidebarStackTabItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  if (show_close_button_) {
    SetCloseButtonVisible(true);
  }
  UpdateBackgroundColor();
  views::View::OnMouseEntered(event);
}

void AstraSidebarStackTabItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  SetCloseButtonVisible(false);
  UpdateBackgroundColor();
  views::View::OnMouseExited(event);
}

// =========================================================================
// Accessibility
// =========================================================================

void AstraSidebarStackTabItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;
  if (title_label_) {
    node_data->SetName(title_label_->GetText());
  }
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraSidebarStackTabItemView::OnCloseButtonClicked() {
  if (delegate_ && web_contents_) {
    delegate_->OnStackTabClosed(web_contents_);
  }
}

void AstraSidebarStackTabItemView::OnAudioButtonClicked() {
  // TODO(astra): Implement audio toggle for stack tab items.
  // This should route through WebContents::SetAudioMuted() via the delegate.
  // For now, this is a placeholder — the audio button visibility and
  // state projection work, but clicking does not yet toggle mute.
  //
  // Chromium owner: content::WebContents::SetAudioMuted()
  //   (content/public/browser/web_contents.h)
}

// =========================================================================
// Visual updates
// =========================================================================

void AstraSidebarStackTabItemView::UpdateAudioButtonVisuals() {
  if (!audio_button_) {
    return;
  }

  // TODO(astra): Use real Chromium tab audio vector icons.
  //   - kTabAudioPlayingIcon for AudioState::kPlaying
  //   - kTabAudioMutedIcon for AudioState::kMuted
  //
  // Chromium owner: TabRenderer (chrome/browser/ui/views/tabs/tab_renderer.h)
  // Chromium vector icons: tab_audio_indicator_*.icon in
  //   ui/resources/vector_icons/

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

}  // namespace astra
