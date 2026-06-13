#include "astra/ui/views/sidebar/astra_sidebar_stack_child_view.h"

#include "astra/ui/color/astra_color_ids.h"
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

// Corner radius for the child item background.
constexpr int kChildCornerRadius = 4;

// Drag threshold (minimum mouse movement to start a drag).
constexpr int kDragThresholdDips = 8;

// Astra color IDs for stack child item styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kChildTextColorId = kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kChildActiveTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kChildActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kChildHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraSidebarStackChildView::AstraSidebarStackChildView(
    const std::u16string& title) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(gfx::RoundedCornersF(kChildCornerRadius));

  // Title label.
  title_label_ = AddChildView(std::make_unique<views::Label>(title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Audio indicator button — shows on the trailing edge when audio is playing.
  audio_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarStackChildView::OnAudioButtonClicked,
                          base::Unretained(this))));
  audio_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  audio_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  audio_button_->SetVisible(false);
  audio_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);

  // Close button — shows on hover.
  close_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarStackChildView::OnCloseButtonClicked,
                          base::Unretained(this))));
  close_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  close_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  close_button_->SetVisible(false);
  close_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  close_button_->SetTooltipText(u"Close tab");
}

AstraSidebarStackChildView::~AstraSidebarStackChildView() = default;

void AstraSidebarStackChildView::SetTitle(const std::u16string& title) {
  if (title_label_) {
    title_label_->SetText(title);
  }
}

void AstraSidebarStackChildView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  OnThemeChanged();
}

void AstraSidebarStackChildView::SetAudioState(AudioState state) {
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

void AstraSidebarStackChildView::SetCloseButtonVisible(bool visible) {
  if (close_button_) {
    close_button_->SetVisible(visible);
  }
}

gfx::Size AstraSidebarStackChildView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = 0;
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  return gfx::Size(width, kChildItemHeight);
}

void AstraSidebarStackChildView::Layout() {
  views::View::Layout();

  // Start after the indent + padding.
  int x = kChildIndent + kChildHorizontalPadding;
  int y_center = height() / 2;

  // Title fills the available space.
  if (title_label_) {
    int title_x = x;
    int title_width = width() - title_x - kChildHorizontalPadding;

    // Reserve space for audio button if visible.
    if (audio_button_ && audio_button_->GetVisible()) {
      title_width -= kCloseButtonSize + kChildIconSpacing;
    }

    // Reserve space for close button if visible.
    if (close_button_ && close_button_->GetVisible()) {
      title_width -= kCloseButtonSize + kChildIconSpacing;
    }

    title_label_->SetBounds(title_x, 0, title_width, height());
  }

  // Audio button on the trailing edge.
  if (audio_button_ && audio_button_->GetVisible()) {
    int btn_x = width() - kChildHorizontalPadding - kCloseButtonSize;
    // Adjust if close button is also visible.
    if (close_button_ && close_button_->GetVisible()) {
      btn_x -= kCloseButtonSize + kChildIconSpacing;
    }
    int btn_y = y_center - kCloseButtonSize / 2;
    audio_button_->SetBounds(btn_x, btn_y, kCloseButtonSize, kCloseButtonSize);
  }

  // Close button on the trailing edge.
  if (close_button_ && close_button_->GetVisible()) {
    int btn_x = width() - kChildHorizontalPadding - kCloseButtonSize;
    int btn_y = y_center - kCloseButtonSize / 2;
    close_button_->SetBounds(btn_x, btn_y, kCloseButtonSize, kCloseButtonSize);
  }
}

void AstraSidebarStackChildView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update text color. Active tabs use primary text color; inactive use
  // secondary to visually de-emphasize child tabs.
  if (title_label_) {
    ui::ColorId text_color_id =
        is_active_ ? kChildActiveTextColorId : kChildTextColorId;
    title_label_->SetEnabledColor(color_provider->GetColor(text_color_id));
  }

  // Update background color.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kChildActiveBgColorId);
  } else if (is_hovered_) {
    bg_color = color_provider->GetColor(kChildHoverBgColorId);
  }
  if (layer()) {
    layer()->SetColor(bg_color);
  }
}

bool AstraSidebarStackChildView::OnMousePressed(
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

bool AstraSidebarStackChildView::OnMouseDragged(
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
    if (delegate_) {
      delegate_->OnStackChildDragStarted(tab_index_, event.location());
    }
  }

  return true;
}

void AstraSidebarStackChildView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  // Click — activate the tab.
  if (event.IsOnlyLeftMouseButton() && delegate_) {
    delegate_->OnStackChildClicked(tab_index_);
  }

  views::View::OnMouseReleased(event);
}

void AstraSidebarStackChildView::OnMouseCaptureLost() {
  is_dragging_ = false;
  views::View::OnMouseCaptureLost();
}

void AstraSidebarStackChildView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  SetCloseButtonVisible(true);
  OnThemeChanged();
  views::View::OnMouseEntered(event);
}

void AstraSidebarStackChildView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  SetCloseButtonVisible(false);
  OnThemeChanged();
  views::View::OnMouseExited(event);
}

void AstraSidebarStackChildView::OnCloseButtonClicked() {
  if (delegate_) {
    delegate_->OnStackChildClosed(tab_index_);
  }
}

void AstraSidebarStackChildView::OnAudioButtonClicked() {
  // TODO(astra): Implement audio toggle for stack child tabs.
  // This should route through WebContents::SetAudioMuted() via the delegate.
  // For now, this is a placeholder — the audio button visibility and
  // state projection work, but clicking does not yet toggle mute.
  //
  // Chromium owner: content::WebContents::SetAudioMuted()
  //   (content/public/browser/web_contents.h)
}

void AstraSidebarStackChildView::UpdateAudioButtonVisuals() {
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

void AstraSidebarStackChildView::SetDraggable(bool draggable) {
  draggable_ = draggable;
}

}  // namespace astra
