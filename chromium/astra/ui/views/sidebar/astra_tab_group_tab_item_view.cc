#include "astra/ui/views/sidebar/astra_tab_group_tab_item_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kGroupTabItemHeight = 28;
constexpr int kGroupTabItemLeftIndent = 30;
constexpr int kGroupTabItemRightPadding = 8;
constexpr int kGroupTabItemFaviconSize = 16;
constexpr int kGroupTabItemFaviconSpacing = 8;
constexpr int kGroupTabItemCloseButtonSize = 16;
constexpr int kGroupTabItemCloseButtonSpacing = 4;
constexpr int kGroupTabItemCornerRadius = 4;
constexpr int kGroupTabItemAudioIndicatorSize = 16;
constexpr int kGroupTabItemAudioSpacing = 4;

// Astra color IDs for tab group tab item styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kGroupTabTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kGroupTabActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kGroupTabHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kGroupTabSelectedTextColorId =
    kColorAstraSidebarItemSelectedText;
constexpr ui::ColorId kGroupTabDragHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraTabGroupTabItemView::AstraTabGroupTabItemView(
    const std::u16string& title,
    int tab_index,
    PressedCallback activate_callback,
    CloseCallback close_callback)
    : LabelButton(std::move(activate_callback), title),
      tab_index_legacy_(tab_index),
      close_callback_(std::move(close_callback)) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kGroupTabItemCornerRadius));

  SetHorizontalAlignment(gfx::ALIGN_LEFT);
  SetFocusBehavior(FocusBehavior::ALWAYS);

  BuildLayout();
  SetTitle(title);
}

AstraTabGroupTabItemView::~AstraTabGroupTabItemView() = default;

void AstraTabGroupTabItemView::BuildLayout() {
  // We customize the label button's layout by adding padding.
  // LabelButton already handles image + label layout, but we need more
  // control over indentation and the close button.
  //
  // TODO(astra): Replace this with a proper views::View subclass that uses
  // BoxLayout for full control, instead of trying to customize LabelButton.
  // LabelButton is convenient but limiting for complex multi-element rows.
  //
  // For now, we use LabelButton's built-in image + label layout for the
  // favicon and title, and add the close button as a separate child.

  // Left indent for group nesting.
  SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kGroupTabItemLeftIndent)));

  // Set up image (favicon placeholder) and label spacing.
  SetImage(views::Button::STATE_NORMAL, gfx::ImageSkia());
  SetImageSize(gfx::Size(kGroupTabItemFaviconSize, kGroupTabItemFaviconSize));
  SetImageLabelSpacing(kGroupTabItemFaviconSpacing);

  // Close button — shown on hover.
  // TODO(astra): Use a proper close icon vector from ui::ResourceBundle
  // or chrome/app/theme. For now, use an ImageButton placeholder.
  // Chromium pattern: TabCloseButton (chrome/browser/ui/views/tabs/tab_close_button.h)
  close_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          [](AstraTabGroupTabItemView* item, const ui::Event& event) {
            if (item->close_callback_) {
              item->close_callback_.Run();
            }
            if (!item->tab_id_.empty() && item->tab_closed_callback_) {
              item->tab_closed_callback_.Run(item->tab_id_);
            }
          },
          base::Unretained(this))));
  close_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  close_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  close_button_->SetPreferredSize(
      gfx::Size(kGroupTabItemCloseButtonSize, kGroupTabItemCloseButtonSize));
  close_button_->SetVisible(false);
  close_button_->SetTooltipText(u"Close tab");
  close_button_->SetFocusBehavior(FocusBehavior::ALWAYS);

  // Audio indicator button.
  // TODO(astra): Use actual Chromium tab audio vector icons.
  audio_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          [](AstraTabGroupTabItemView* item, const ui::Event& event) {
            // Toggle mute state.
            // TODO(astra): Wire to actual WebContents::SetAudioMuted().
            // Chromium owner: content::WebContents (content/public/browser/web_contents.h)
            item->SetIsMuted(!item->is_muted_);
          },
          base::Unretained(this))));
  audio_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  audio_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  audio_button_->SetPreferredSize(
      gfx::Size(kGroupTabItemAudioIndicatorSize, kGroupTabItemAudioIndicatorSize));
  audio_button_->SetVisible(false);
  audio_button_->SetFocusBehavior(FocusBehavior::NEVER);
}

// =========================================================================
// Tab info
// =========================================================================

void AstraTabGroupTabItemView::SetTabInfo(const AstraTabGroupTabInfo& info) {
  tab_id_ = info.tab_id;
  SetTitle(info.title);
  SetUrl(info.url);
  SetActive(info.is_active);
  SetPinned(info.is_pinned);
  SetIsAudible(info.is_audible);
  SetIsMuted(info.is_muted);
  SetIsLoading(info.is_loading);
  SetFavicon(info.favicon);
  SetHasFavicon(info.has_favicon);
  SetIndexInGroup(info.index_in_group);
  SetGroupId(info.group_id);
}

// =========================================================================
// Title
// =========================================================================

void AstraTabGroupTabItemView::SetTitle(const std::u16string& title) {
  SetText(title);
  SetAccessibleName(title);
}

std::u16string AstraTabGroupTabItemView::GetTitle() const {
  return GetText();
}

// =========================================================================
// URL
// =========================================================================

void AstraTabGroupTabItemView::SetUrl(const GURL& url) {
  url_ = url;
  // Update tooltip to show URL.
  if (url.is_valid()) {
    SetTooltipText(base::UTF8ToUTF16(url.spec()));
  } else {
    SetTooltipText(std::u16string());
  }
}

// =========================================================================
// Favicon
// =========================================================================

void AstraTabGroupTabItemView::SetFavicon(const gfx::ImageSkia& favicon) {
  favicon_ = favicon;
  has_favicon_ = !favicon.isNull();
  if (show_favicon_ && has_favicon_) {
    SetImage(views::Button::STATE_NORMAL, favicon);
  }
  UpdateFaviconVisibility();
}

void AstraTabGroupTabItemView::SetHasFavicon(bool has_favicon) {
  has_favicon_ = has_favicon;
  UpdateFaviconVisibility();
}

void AstraTabGroupTabItemView::UpdateFaviconVisibility() {
  bool should_show = show_favicon_ && has_favicon_;
  // The image visibility in LabelButton is controlled by the image itself.
  // When the image is null, the label takes the full width.
  if (!show_favicon_) {
    SetImage(views::Button::STATE_NORMAL, gfx::ImageSkia());
  } else if (has_favicon_ && !favicon_.isNull()) {
    SetImage(views::Button::STATE_NORMAL, favicon_);
  }
  InvalidateLayout();
}

// =========================================================================
// Active state
// =========================================================================

void AstraTabGroupTabItemView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  OnThemeChanged();
  // Active tabs always show the close button.
  UpdateCloseButtonVisibility();
}

// =========================================================================
// Close button
// =========================================================================

void AstraTabGroupTabItemView::SetCloseButtonVisible(bool visible) {
  if (!close_button_) {
    return;
  }
  if (!show_close_button_) {
    close_button_->SetVisible(false);
    return;
  }
  close_button_->SetVisible(visible);
}

bool AstraTabGroupTabItemView::IsCloseButtonVisible() const {
  return close_button_ && close_button_->GetVisible();
}

void AstraTabGroupTabItemView::SetShowCloseButton(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  UpdateCloseButtonVisibility();
}

void AstraTabGroupTabItemView::SetShowFavicon(bool show) {
  if (show_favicon_ == show) {
    return;
  }
  show_favicon_ = show;
  UpdateFaviconVisibility();
}

// =========================================================================
// Drag state
// =========================================================================

void AstraTabGroupTabItemView::SetIsDragging(bool dragging) {
  if (is_dragging_ == dragging) {
    return;
  }
  is_dragging_ = dragging;
  SetVisible(!dragging);
  SchedulePaint();
}

void AstraTabGroupTabItemView::SetDragHovered(bool hovered) {
  if (is_drag_hovered_ == hovered) {
    return;
  }
  is_drag_hovered_ = hovered;
  OnThemeChanged();
}

// =========================================================================
// Index in group
// =========================================================================

void AstraTabGroupTabItemView::SetIndexInGroup(int index) {
  index_in_group_ = index;
}

// =========================================================================
// Group ID
// =========================================================================

void AstraTabGroupTabItemView::SetGroupId(const std::string& group_id) {
  group_id_ = group_id;
}

// =========================================================================
// Pinned state
// =========================================================================

void AstraTabGroupTabItemView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  // Pinned tabs might have different styling or close button behavior.
  // TODO(astra): Visual distinction for pinned tabs in groups.
  UpdateCloseButtonVisibility();
  SchedulePaint();
}

// =========================================================================
// Audio state
// =========================================================================

void AstraTabGroupTabItemView::SetIsAudible(bool audible) {
  if (is_audible_ == audible) {
    return;
  }
  is_audible_ = audible;
  UpdateAudioIndicator();
}

void AstraTabGroupTabItemView::SetIsMuted(bool muted) {
  if (is_muted_ == muted) {
    return;
  }
  is_muted_ = muted;
  // If muted, the tab isn't "audible" in the sense of producing sound.
  // But we still show the muted indicator.
  UpdateAudioIndicator();
}

void AstraTabGroupTabItemView::UpdateAudioIndicator() {
  if (!audio_button_) {
    return;
  }

  bool has_audio = is_audible_ || is_muted_;
  audio_button_->SetVisible(has_audio);

  if (has_audio) {
    if (is_muted_) {
      audio_button_->SetTooltipText(u"Unmute tab");
    } else {
      audio_button_->SetTooltipText(u"Mute tab");
    }
  }

  // TODO(astra): Update the audio button icon based on state.
  // Chromium provides vector icons for tab audio:
  //   - tab_audio_playing_icon
  //   - tab_audio_muting_icon
  //   - tab_audio_recording_icon
  // Chromium owner: chrome/browser/ui/views/tabs/tab_renderer.h
}

// =========================================================================
// Loading state
// =========================================================================

void AstraTabGroupTabItemView::SetIsLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  // TODO(astra): Show a loading spinner/throbber when loading.
  // Chromium pattern: TabLoadingSpinner / ThrobberView
  // Chromium owner: ui/views/controls/throbber.h
  SchedulePaint();
}

// =========================================================================
// Close button visibility
// =========================================================================

void AstraTabGroupTabItemView::UpdateCloseButtonVisibility() {
  if (!close_button_ || !show_close_button_) {
    if (close_button_) {
      close_button_->SetVisible(false);
    }
    return;
  }
  // Show close button on hover or when tab is active.
  // This matches Chromium's tab strip behavior.
  bool should_show =
      GetState() == STATE_HOVERED || is_active_ || is_drag_hovered_;
  close_button_->SetVisible(should_show);
}

// =========================================================================
// Mouse events
// =========================================================================

void AstraTabGroupTabItemView::OnMouseEntered(const ui::MouseEvent& event) {
  LabelButton::OnMouseEntered(event);
  UpdateCloseButtonVisibility();
}

void AstraTabGroupTabItemView::OnMouseExited(const ui::MouseEvent& event) {
  LabelButton::OnMouseExited(event);
  UpdateCloseButtonVisibility();
}

bool AstraTabGroupTabItemView::OnMousePressed(const ui::MouseEvent& event) {
  // Handle middle-click.
  if (event.IsMiddleMouseButton()) {
    HandleMiddleClick();
    return true;
  }
  return LabelButton::OnMousePressed(event);
}

void AstraTabGroupTabItemView::HandleMiddleClick() {
  // Middle-click on a tab closes it (Chromium behavior).
  if (close_callback_) {
    close_callback_.Run();
  }
  if (!tab_id_.empty() && tab_middle_clicked_callback_) {
    tab_middle_clicked_callback_.Run(tab_id_);
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraTabGroupTabItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = LabelButton::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kGroupTabItemHeight));
  return size;
}

void AstraTabGroupTabItemView::OnThemeChanged() {
  LabelButton::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update text color.
  SkColor text_color = color_provider->GetColor(kGroupTabTextColorId);
  if (is_active_) {
    text_color = color_provider->GetColor(kGroupTabSelectedTextColorId);
  }
  SetEnabledTextColors(text_color);

  // Update background color based on active/hover/drag state.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kGroupTabActiveBgColorId);
  } else if (is_drag_hovered_) {
    bg_color = color_provider->GetColor(kGroupTabDragHoverBgColorId);
  } else if (GetState() == STATE_HOVERED) {
    bg_color = color_provider->GetColor(kGroupTabHoverBgColorId);
  }
  if (layer()) {
    layer()->SetColor(bg_color);
  }
}

}  // namespace astra
