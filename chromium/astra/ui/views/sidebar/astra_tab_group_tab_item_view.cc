#include "astra/ui/views/sidebar/astra_tab_group_tab_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
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
constexpr int kGroupTabItemHeight = 28;
constexpr int kGroupTabItemLeftIndent = 30;
constexpr int kGroupTabItemRightPadding = 8;
constexpr int kGroupTabItemFaviconSize = 16;
constexpr int kGroupTabItemFaviconSpacing = 8;
constexpr int kGroupTabItemCloseButtonSize = 16;
constexpr int kGroupTabItemCloseButtonSpacing = 4;
constexpr int kGroupTabItemCornerRadius = 4;

// Astra color IDs for tab group tab item styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kGroupTabTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kGroupTabActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kGroupTabHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraTabGroupTabItemView::AstraTabGroupTabItemView(
    const std::u16string& title,
    int tab_index,
    PressedCallback activate_callback,
    CloseCallback close_callback)
    : LabelButton(std::move(activate_callback), title),
      tab_index_(tab_index),
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
  // We customize the label button's layout by adding a custom box layout.
  // LabelButton already handles label + image layout, but we need more
  // control over indentation and the close button.
  //
  // TODO(astra): Replace this with a proper views::View subclass that uses
  // BoxLayout for full control, instead of trying to customize LabelButton.
  // LabelButton is convenient but limiting for complex multi-element rows.
  //
  // For now, we use LabelButton's built-in image + label layout for the
  // favicon and title, and add the close button as a separate child that
  // is positioned manually or via a second layout pass.

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
          },
          base::Unretained(this))));
  close_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  close_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  close_button_->SetPreferredSize(
      gfx::Size(kGroupTabItemCloseButtonSize, kGroupTabItemCloseButtonSize));
  close_button_->SetVisible(false);
  close_button_->SetTooltipText(u"Close tab");

  // TODO(astra): Position the close button properly in the layout.
  // Currently it's added as a child but needs explicit positioning or
  // a proper BoxLayout that includes it in the flow.
}

void AstraTabGroupTabItemView::SetTitle(const std::u16string& title) {
  SetText(title);
}

void AstraTabGroupTabItemView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  OnThemeChanged();
}

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
  SetEnabledTextColors(text_color);

  // Update background color based on active/hover state.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kGroupTabActiveBgColorId);
  } else if (GetState() == STATE_HOVERED) {
    bg_color = color_provider->GetColor(kGroupTabHoverBgColorId);
  }
  if (layer()) {
    layer()->SetColor(bg_color);
  }
}

void AstraTabGroupTabItemView::OnMouseEntered(const ui::MouseEvent& event) {
  LabelButton::OnMouseEntered(event);
  UpdateCloseButtonVisibility();
}

void AstraTabGroupTabItemView::OnMouseExited(const ui::MouseEvent& event) {
  LabelButton::OnMouseExited(event);
  UpdateCloseButtonVisibility();
}

void AstraTabGroupTabItemView::UpdateCloseButtonVisibility() {
  if (!close_button_) {
    return;
  }
  // Show close button on hover.
  // TODO(astra): Also show when the tab is active or pinned, matching
  // Chromium's tab strip behavior (close button always visible on active tab).
  bool should_show = GetState() == STATE_HOVERED;
  close_button_->SetVisible(should_show);
}

}  // namespace astra
