#include "astra/ui/views/sidebar/astra_sidebar_stack_header_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Astra color IDs for stack header styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kStackHeaderTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kStackHeaderActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kStackHeaderHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

// TODO(astra): Add Astra-specific child count badge color IDs to
// astra_color_ids.h. For now, reuse Chromium's prominent button colors.
constexpr ui::ColorId kChildCountBadgeBgColor =
    ui::kColorButtonBackgroundProminent;
constexpr ui::ColorId kChildCountBadgeTextColor =
    ui::kColorButtonForegroundProminent;

}  // namespace

AstraSidebarStackHeaderView::AstraSidebarStackHeaderView(
    const std::u16string& title) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kStackHeaderCornerRadius));

  // Color accent bar on the left edge.
  accent_color_bar_ = AddChildView(std::make_unique<views::View>());
  accent_color_bar_->SetPaintToLayer();
  accent_color_bar_->layer()->SetFillsBoundsOpaquely(false);

  // Expand/collapse arrow button on the leading edge.
  expand_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarStackHeaderView::OnExpandButtonClicked,
                          base::Unretained(this))));
  expand_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  expand_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  expand_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  expand_button_->SetTooltipText(u"Expand/collapse stack");

  // Title label.
  title_label_ = AddChildView(std::make_unique<views::Label>(title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Child count badge on the trailing edge.
  child_count_label_ = AddChildView(std::make_unique<views::Label>());
  child_count_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  child_count_label_->SetAutoColorReadabilityEnabled(false);
  child_count_label_->SetVisible(false);
  child_count_label_->SetPaintToLayer();
  child_count_label_->layer()->SetFillsBoundsOpaquely(false);
  child_count_label_->layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kChildCountBadgeCornerRadius));

  // Menu button for stack actions (rename, delete, change color).
  menu_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarStackHeaderView::OnMenuButtonClicked,
                          base::Unretained(this))));
  menu_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  menu_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  menu_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  menu_button_->SetTooltipText(u"Stack actions");
  menu_button_->SetVisible(false);  // Shown on hover.
}

AstraSidebarStackHeaderView::~AstraSidebarStackHeaderView() = default;

void AstraSidebarStackHeaderView::SetTitle(const std::u16string& title) {
  if (title_label_) {
    title_label_->SetText(title);
  }
}

void AstraSidebarStackHeaderView::SetExpanded(bool expanded) {
  if (is_expanded_ == expanded) {
    return;
  }
  is_expanded_ = expanded;
  UpdateExpandArrowVisuals();
  SchedulePaint();
}

void AstraSidebarStackHeaderView::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;
  OnThemeChanged();
}

void AstraSidebarStackHeaderView::SetChildCount(size_t count) {
  if (child_count_ == count) {
    return;
  }
  child_count_ = count;
  UpdateChildCountBadge();
}

void AstraSidebarStackHeaderView::SetAccentColor(
    const std::string& color_hex) {
  if (accent_color_ == color_hex) {
    return;
  }
  accent_color_ = color_hex;
  UpdateAccentColorBar();
}

gfx::Size AstraSidebarStackHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Fixed height, width follows available space.
  int width = 0;
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  return gfx::Size(width, kStackHeaderHeight);
}

void AstraSidebarStackHeaderView::Layout() {
  views::View::Layout();

  int x = kStackHeaderHorizontalPadding;
  int y_center = height() / 2;

  // Color accent bar on the far left edge.
  if (accent_color_bar_) {
    int bar_y = (height() - kStackHeaderHeight) / 2 + 4;
    int bar_height = kStackHeaderHeight - 8;
    accent_color_bar_->SetBounds(2, bar_y, kAccentColorBarWidth, bar_height);
  }

  // Expand arrow on the left.
  if (expand_button_) {
    int arrow_y = y_center - kExpandArrowSize / 2;
    expand_button_->SetBounds(x, arrow_y, kExpandArrowSize, kExpandArrowSize);
    x += kExpandArrowSize + kStackHeaderIconSpacing;
  }

  // Menu button on the right (shown on hover).
  int right_x = width() - kStackHeaderHorizontalPadding;
  if (menu_button_ && menu_button_->GetVisible()) {
    right_x -= kMenuButtonSize;
    int btn_y = y_center - kMenuButtonSize / 2;
    menu_button_->SetBounds(right_x, btn_y, kMenuButtonSize, kMenuButtonSize);
    right_x -= kStackHeaderIconSpacing;
  }

  // Child count badge on the right (before menu button).
  if (child_count_label_ && child_count_label_->GetVisible()) {
    // Measure the badge text to get the width.
    int badge_width = kChildCountBadgeMinWidth;
    gfx::Size text_size = child_count_label_->GetPreferredSize();
    badge_width = std::max(badge_width, text_size.width() + 8);

    right_x -= badge_width;
    int badge_y = y_center - kChildCountBadgeHeight / 2;
    child_count_label_->SetBounds(right_x, badge_y, badge_width,
                                  kChildCountBadgeHeight);
    right_x -= kChildCountBadgeSpacing;
  }

  // Title fills the remaining space.
  if (title_label_) {
    int title_width = right_x - x;
    if (title_width < 0) title_width = 0;
    title_label_->SetBounds(x, 0, title_width, height());
  }
}

void AstraSidebarStackHeaderView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update text colors.
  if (title_label_) {
    title_label_->SetEnabledColor(
        color_provider->GetColor(kStackHeaderTextColorId));
  }

  // Update child count badge colors.
  if (child_count_label_) {
    child_count_label_->SetEnabledColor(
        color_provider->GetColor(kChildCountBadgeTextColor));
    if (child_count_label_->layer()) {
      child_count_label_->layer()->SetColor(
          color_provider->GetColor(kChildCountBadgeBgColor));
    }
  }

  // Update background color based on active/hover state.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_active_) {
    bg_color = color_provider->GetColor(kStackHeaderActiveBgColorId);
  } else if (is_hovered_) {
    bg_color = color_provider->GetColor(kStackHeaderHoverBgColorId);
  }
  if (layer()) {
    layer()->SetColor(bg_color);
  }

  UpdateExpandArrowVisuals();
  UpdateAccentColorBar();
}

bool AstraSidebarStackHeaderView::OnMousePressed(
    const ui::MouseEvent& event) {
  // Only handle left button clicks on the header body (not the expand
  // button or menu button — those handle their own events).
  if (!event.IsOnlyLeftMouseButton()) {
    return views::View::OnMousePressed(event);
  }

  // Check if the click is on the expand button — the button handles its own.
  // Check if the click is on the menu button — the button handles its own.
  // If it's on the header body, notify the delegate.
  if (delegate_ && !stack_id_.empty()) {
    delegate_->OnStackHeaderClicked(stack_id_);
  }

  return true;
}

void AstraSidebarStackHeaderView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  if (menu_button_) {
    menu_button_->SetVisible(true);
  }
  OnThemeChanged();
  views::View::OnMouseEntered(event);
}

void AstraSidebarStackHeaderView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  if (menu_button_) {
    menu_button_->SetVisible(false);
  }
  OnThemeChanged();
  views::View::OnMouseExited(event);
}

void AstraSidebarStackHeaderView::OnExpandButtonClicked() {
  if (delegate_ && !stack_id_.empty()) {
    delegate_->OnStackToggleExpanded(stack_id_);
  }
}

void AstraSidebarStackHeaderView::OnMenuButtonClicked() {
  if (delegate_ && !stack_id_.empty() && menu_button_) {
    gfx::Point anchor = menu_button_->GetBoundsInScreen().bottom_left();
    delegate_->OnStackMenuClicked(stack_id_, anchor);
  }
}

void AstraSidebarStackHeaderView::UpdateExpandArrowVisuals() {
  if (!expand_button_) {
    return;
  }

  // TODO(astra): Use real Chromium vector icons for the expand/collapse
  // arrow.  For now, we use a simple text symbol as a visual placeholder
  // so the layout and state machine work correctly.
  //
  // Chromium resources:
  //   - kTreeViewExpandedIcon / kTreeViewCollapsedIcon
  //   - ui/resources/vector_icons/arrow_down.icon / arrow_right.icon
  //
  // Use ui::ImageModel::FromVectorIcon() with the appropriate vector icon
  // and color from the ColorProvider.
  //
  // Chromium owner: views::TreeView (ui/views/controls/tree/tree_view.h)

  // For the skeleton implementation, we rely on the tooltip text and
  // click handling to convey state; the visual icon will be wired up
  // when building against the full Chromium tree with vector icon support.
}

void AstraSidebarStackHeaderView::UpdateChildCountBadge() {
  if (!child_count_label_) {
    return;
  }

  if (child_count_ > 0) {
    child_count_label_->SetText(
        base::NumberToString16(static_cast<int>(child_count_)));
    child_count_label_->SetVisible(true);
  } else {
    child_count_label_->SetVisible(false);
  }

  InvalidateLayout();
}

void AstraSidebarStackHeaderView::UpdateAccentColorBar() {
  if (!accent_color_bar_ || !accent_color_bar_->layer()) {
    return;
  }

  SkColor color = ParseHexColor(accent_color_);
  accent_color_bar_->layer()->SetColor(color);
}

SkColor AstraSidebarStackHeaderView::ParseHexColor(
    const std::string& hex) {
  // Parse a hex color string like "#RRGGBB" or "RRGGBB".
  // Returns SK_ColorGRAY on failure (safe default).
  //
  // TODO(astra): Use a proper color parsing utility.
  //   Chromium owner: SkColorParse / css_color_parser
  //   (third_party/skia/include/core/SkColor.h)
  //
  // For the skeleton implementation, we do a simple hex-to-SkColor conversion.

  if (hex.empty()) {
    return SK_ColorGRAY;
  }

  std::string color_str = hex;
  if (color_str[0] == '#') {
    color_str = color_str.substr(1);
  }

  if (color_str.length() != 6) {
    return SK_ColorGRAY;
  }

  // Convert hex string to integer.
  unsigned int color_val = 0;
  if (!base::HexStringToUInt(color_str, &color_val)) {
    return SK_ColorGRAY;
  }

  return SkColorSetRGB(
      static_cast<unsigned char>((color_val >> 16) & 0xFF),
      static_cast<unsigned char>((color_val >> 8) & 0xFF),
      static_cast<unsigned char>(color_val & 0xFF));
}

}  // namespace astra
