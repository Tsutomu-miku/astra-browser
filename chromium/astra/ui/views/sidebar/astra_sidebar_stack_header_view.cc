#include "astra/ui/views/sidebar/astra_sidebar_stack_header_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_view.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
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
constexpr ui::ColorId kStackHeaderSelectedBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kStackHeaderHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kStackHeaderDragHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

// TODO(astra): Add Astra-specific tab count badge color IDs to
// astra_color_ids.h. For now, reuse Chromium's prominent button colors.
constexpr ui::ColorId kTabCountBadgeBgColor =
    ui::kColorButtonBackgroundProminent;
constexpr ui::ColorId kTabCountBadgeTextColor =
    ui::kColorButtonForegroundProminent;

// Unread indicator color.
// TODO(astra): Add to astra_color_ids.h.
constexpr SkColor kUnreadIndicatorColor = SK_ColorRED;

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
      base::BindRepeating(
          &AstraSidebarStackHeaderView::OnExpandButtonClicked,
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

  // Unread indicator dot.
  unread_indicator_ = AddChildView(std::make_unique<views::View>());
  unread_indicator_->SetPaintToLayer();
  unread_indicator_->layer()->SetFillsBoundsOpaquely(true);
  unread_indicator_->layer()->SetColor(kUnreadIndicatorColor);
  unread_indicator_->SetVisible(false);

  // Pin indicator icon.
  // TODO(astra): Use a real pin vector icon.
  //   Chromium resources: pin icon in ui/resources/vector_icons/
  pin_indicator_ = AddChildView(std::make_unique<views::ImageView>());
  pin_indicator_->SetVisible(false);
  pin_indicator_->SetTooltipText(u"Pinned stack");

  // Tab count badge on the trailing edge.
  tab_count_label_ = AddChildView(std::make_unique<views::Label>());
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetVisible(false);
  tab_count_label_->SetPaintToLayer();
  tab_count_label_->layer()->SetFillsBoundsOpaquely(false);
  tab_count_label_->layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kTabCountBadgeCornerRadius));

  // Menu button for stack actions (rename, delete, change color).
  menu_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraSidebarStackHeaderView::OnMenuButtonClicked,
                          base::Unretained(this))));
  menu_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  menu_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  menu_button_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  menu_button_->SetTooltipText(u"Stack actions");
  menu_button_->SetVisible(show_menu_button_);
}

AstraSidebarStackHeaderView::~AstraSidebarStackHeaderView() = default;

// =========================================================================
// Stack info
// =========================================================================

void AstraSidebarStackHeaderView::SetStackInfo(const AstraStackInfo& info) {
  stack_id_ = info.stack_id;
  color_ = info.color;
  tab_count_ = info.tab_count;
  is_expanded_ = info.is_expanded;
  is_pinned_ = info.is_pinned;
  has_unread_ = info.has_unread;

  // Update hex string for legacy getter.
  // TODO(astra): Remove when legacy accent_color() is removed.
  char hex_buf[8];
  base::snprintf(hex_buf, sizeof(hex_buf), "#%02X%02X%02X",
                 SkColorGetR(info.color),
                 SkColorGetG(info.color),
                 SkColorGetB(info.color));
  accent_color_hex_ = hex_buf;

  // Update all visual elements.
  if (title_label_) {
    title_label_->SetText(info.name);
  }
  UpdateExpandArrowVisuals();
  UpdateTabCountBadge();
  UpdateAccentColorBar();
  UpdateUnreadIndicator();
  UpdatePinIndicator();
  UpdateBackgroundColor();
  SchedulePaint();
}

// =========================================================================
// Name
// =========================================================================

void AstraSidebarStackHeaderView::SetName(const std::u16string& name) {
  if (title_label_) {
    title_label_->SetText(name);
  }
}

std::u16string AstraSidebarStackHeaderView::GetName() const {
  if (title_label_) {
    return title_label_->GetText();
  }
  return std::u16string();
}

// =========================================================================
// Color
// =========================================================================

void AstraSidebarStackHeaderView::SetColor(SkColor color) {
  if (color_ == color) {
    return;
  }
  color_ = color;
  UpdateAccentColorBar();

  // Update hex string for legacy getter.
  char hex_buf[8];
  base::snprintf(hex_buf, sizeof(hex_buf), "#%02X%02X%02X",
                 SkColorGetR(color),
                 SkColorGetG(color),
                 SkColorGetB(color));
  accent_color_hex_ = hex_buf;
}

// =========================================================================
// Tab count
// =========================================================================

void AstraSidebarStackHeaderView::SetTabCount(int count) {
  if (tab_count_ == count) {
    return;
  }
  tab_count_ = count;
  UpdateTabCountBadge();
}

// =========================================================================
// Expansion
// =========================================================================

void AstraSidebarStackHeaderView::SetExpanded(bool expanded) {
  if (is_expanded_ == expanded) {
    return;
  }
  is_expanded_ = expanded;
  UpdateExpandArrowVisuals();
  SchedulePaint();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarStackHeaderView::SetSelected(bool selected) {
  if (is_selected_ == selected) {
    return;
  }
  is_selected_ = selected;
  UpdateBackgroundColor();
}

// =========================================================================
// Pinned
// =========================================================================

void AstraSidebarStackHeaderView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  UpdatePinIndicator();
  InvalidateLayout();
}

// =========================================================================
// Chevron
// =========================================================================

void AstraSidebarStackHeaderView::SetShowChevron(bool show) {
  if (show_chevron_ == show) {
    return;
  }
  show_chevron_ = show;
  if (expand_button_) {
    expand_button_->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// Tab count display
// =========================================================================

void AstraSidebarStackHeaderView::SetShowTabCount(bool show) {
  if (show_tab_count_ == show) {
    return;
  }
  show_tab_count_ = show;
  UpdateTabCountBadge();
  InvalidateLayout();
}

// =========================================================================
// Color indicator
// =========================================================================

void AstraSidebarStackHeaderView::SetShowColorIndicator(bool show) {
  if (show_color_indicator_ == show) {
    return;
  }
  show_color_indicator_ = show;
  if (accent_color_bar_) {
    accent_color_bar_->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// Menu button
// =========================================================================

void AstraSidebarStackHeaderView::SetShowMenuButton(bool show) {
  if (show_menu_button_ == show) {
    return;
  }
  show_menu_button_ = show;
  if (menu_button_) {
    menu_button_->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// Unread
// =========================================================================

void AstraSidebarStackHeaderView::SetHasUnread(bool has_unread) {
  if (has_unread_ == has_unread) {
    return;
  }
  has_unread_ = has_unread;
  UpdateUnreadIndicator();
  InvalidateLayout();
}

// =========================================================================
// Compact mode
// =========================================================================

void AstraSidebarStackHeaderView::SetCompact(bool compact) {
  if (is_compact_ == compact) {
    return;
  }
  is_compact_ = compact;
  // TODO(astra): Adjust font size for compact mode.
  InvalidateLayout();
}

// =========================================================================
// Drag hovered
// =========================================================================

void AstraSidebarStackHeaderView::SetDragHovered(bool hovered) {
  if (is_drag_hovered_ == hovered) {
    return;
  }
  is_drag_hovered_ = hovered;
  UpdateBackgroundColor();
}

// =========================================================================
// Legacy / compatibility
// =========================================================================

void AstraSidebarStackHeaderView::SetAccentColor(const std::string& color_hex) {
  accent_color_hex_ = color_hex;
  SkColor color = ParseHexColor(color_hex);
  SetColor(color);
}

// =========================================================================
// Layout
// =========================================================================

int AstraSidebarStackHeaderView::GetHeaderHeight() const {
  return is_compact_ ? kStackHeaderCompactHeight : kStackHeaderHeight;
}

gfx::Size AstraSidebarStackHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Fixed height, width follows available space.
  int width = 0;
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  return gfx::Size(width, GetHeaderHeight());
}

void AstraSidebarStackHeaderView::Layout() {
  views::View::Layout();

  int x = kStackHeaderHorizontalPadding;
  int y_center = height() / 2;
  int header_height = GetHeaderHeight();

  // Color accent bar on the far left edge.
  if (accent_color_bar_ && accent_color_bar_->GetVisible()) {
    int bar_y = (height() - header_height) / 2 + 4;
    int bar_height = header_height - 8;
    accent_color_bar_->SetBounds(2, bar_y, kAccentColorBarWidth, bar_height);
  }

  // Expand arrow on the left.
  if (expand_button_ && expand_button_->GetVisible()) {
    int arrow_y = y_center - kExpandArrowSize / 2;
    expand_button_->SetBounds(x, arrow_y, kExpandArrowSize, kExpandArrowSize);
    x += kExpandArrowSize + kStackHeaderIconSpacing;
  }

  // Pin indicator (after expand arrow).
  if (pin_indicator_ && pin_indicator_->GetVisible()) {
    int pin_y = y_center - kPinIndicatorSize / 2;
    pin_indicator_->SetBounds(x, pin_y, kPinIndicatorSize, kPinIndicatorSize);
    x += kPinIndicatorSize + kStackHeaderIconSpacing;
  }

  // Right edge tracking for trailing elements.
  int right_x = width() - kStackHeaderHorizontalPadding;

  // Menu button on the right.
  if (menu_button_ && menu_button_->GetVisible()) {
    right_x -= kMenuButtonSize;
    int btn_y = y_center - kMenuButtonSize / 2;
    menu_button_->SetBounds(right_x, btn_y, kMenuButtonSize, kMenuButtonSize);
    right_x -= kStackHeaderIconSpacing;
  }

  // Tab count badge on the right (before menu button).
  if (tab_count_label_ && tab_count_label_->GetVisible()) {
    // Measure the badge text to get the width.
    int badge_width = kTabCountBadgeMinWidth;
    gfx::Size text_size = tab_count_label_->GetPreferredSize();
    badge_width = std::max(badge_width, text_size.width() + 8);

    right_x -= badge_width;
    int badge_y = y_center - kTabCountBadgeHeight / 2;
    tab_count_label_->SetBounds(right_x, badge_y, badge_width,
                                 kTabCountBadgeHeight);
    right_x -= kTabCountBadgeSpacing;
  }

  // Unread indicator (before tab count badge).
  if (unread_indicator_ && unread_indicator_->GetVisible()) {
    right_x -= kUnreadIndicatorSize;
    int dot_y = y_center - kUnreadIndicatorSize / 2;
    unread_indicator_->SetBounds(right_x, dot_y, kUnreadIndicatorSize,
                                  kUnreadIndicatorSize);
    right_x -= kStackHeaderIconSpacing;
  }

  // Title fills the remaining space.
  if (title_label_) {
    int title_width = right_x - x;
    if (title_width < 0) title_width = 0;
    title_label_->SetBounds(x, 0, title_width, height());
  }
}

// =========================================================================
// Theme
// =========================================================================

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

  // Update tab count badge colors.
  if (tab_count_label_) {
    tab_count_label_->SetEnabledColor(
        color_provider->GetColor(kTabCountBadgeTextColor));
    if (tab_count_label_->layer()) {
      tab_count_label_->layer()->SetColor(
          color_provider->GetColor(kTabCountBadgeBgColor));
    }
  }

  UpdateBackgroundColor();
  UpdateExpandArrowVisuals();
  UpdateAccentColorBar();
}

// =========================================================================
// Mouse events
// =========================================================================

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
  // Show menu button on hover if it's not always shown.
  if (menu_button_ && !show_menu_button_) {
    menu_button_->SetVisible(true);
  }
  UpdateBackgroundColor();
  views::View::OnMouseEntered(event);
}

void AstraSidebarStackHeaderView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  // Hide menu button on exit if it's not always shown.
  if (menu_button_ && !show_menu_button_) {
    menu_button_->SetVisible(false);
  }
  UpdateBackgroundColor();
  views::View::OnMouseExited(event);
}

// =========================================================================
// Accessibility
// =========================================================================

void AstraSidebarStackHeaderView::GetAccessibleNodeData(
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

// =========================================================================
// Visual updates
// =========================================================================

void AstraSidebarStackHeaderView::UpdateExpandArrowVisuals() {
  if (!expand_button_) {
    return;
  }

  // TODO(astra): Use real Chromium vector icons for the expand/collapse
  // arrow.  For now, we use the tooltip text and click handling to convey
  // state; the visual icon will be wired up when building against the full
  // Chromium tree with vector icon support.
  //
  // Chromium resources:
  //   - kTreeViewExpandedIcon / kTreeViewCollapsedIcon
  //   - ui/resources/vector_icons/arrow_down.icon / arrow_right.icon
  //
  // Use ui::ImageModel::FromVectorIcon() with the appropriate vector icon
  // and color from the ColorProvider.
  //
  // Chromium owner: views::TreeView (ui/views/controls/tree/tree_view.h)

  // Update tooltip.
  if (is_expanded_) {
    expand_button_->SetTooltipText(u"Collapse stack");
  } else {
    expand_button_->SetTooltipText(u"Expand stack");
  }
}

void AstraSidebarStackHeaderView::UpdateTabCountBadge() {
  if (!tab_count_label_) {
    return;
  }

  if (show_tab_count_ && tab_count_ > 0) {
    tab_count_label_->SetText(base::NumberToString16(tab_count_));
    tab_count_label_->SetVisible(true);
  } else {
    tab_count_label_->SetVisible(false);
  }

  InvalidateLayout();
}

void AstraSidebarStackHeaderView::UpdateAccentColorBar() {
  if (!accent_color_bar_ || !accent_color_bar_->layer()) {
    return;
  }
  accent_color_bar_->layer()->SetColor(color_);
}

void AstraSidebarStackHeaderView::UpdateUnreadIndicator() {
  if (!unread_indicator_) {
    return;
  }
  unread_indicator_->SetVisible(has_unread_);
}

void AstraSidebarStackHeaderView::UpdatePinIndicator() {
  if (!pin_indicator_) {
    return;
  }
  pin_indicator_->SetVisible(is_pinned_);
}

void AstraSidebarStackHeaderView::UpdateBackgroundColor() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider || !layer()) {
    return;
  }

  SkColor bg_color = SK_ColorTRANSPARENT;
  if (is_selected_) {
    bg_color = color_provider->GetColor(kStackHeaderSelectedBgColorId);
  } else if (is_drag_hovered_) {
    bg_color = color_provider->GetColor(kStackHeaderDragHoverBgColorId);
  } else if (is_hovered_) {
    bg_color = color_provider->GetColor(kStackHeaderHoverBgColorId);
  }
  layer()->SetColor(bg_color);
}

// =========================================================================
// Static helpers
// =========================================================================

SkColor AstraSidebarStackHeaderView::ParseHexColor(const std::string& hex) {
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
