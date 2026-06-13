#include "astra/ui/views/sidebar/astra_tab_group_header_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kGroupHeaderHeight = 30;
constexpr int kGroupHeaderCompactHeight = 24;
constexpr int kGroupHeaderHorizontalPadding = 12;
constexpr int kGroupHeaderCompactHorizontalPadding = 8;
constexpr int kGroupHeaderDotSize = 10;
constexpr int kGroupHeaderDotSpacing = 8;
constexpr int kGroupHeaderChevronSize = 12;
constexpr int kGroupHeaderChevronSpacing = 4;
constexpr int kGroupHeaderTitleSpacing = 4;
constexpr int kGroupHeaderCornerRadius = 6;
constexpr int kNewTabButtonSize = 16;
constexpr int kNewTabButtonSpacing = 4;
constexpr int kMenuButtonSize = 16;
constexpr int kMenuButtonSpacing = 4;
constexpr int kPinIndicatorSize = 12;
constexpr int kPinIndicatorSpacing = 4;

// Astra color IDs for tab group header styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kGroupHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kGroupHeaderHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kGroupHeaderSelectedBgColorId =
    kColorAstraSidebarItemSelectedBackground;
constexpr ui::ColorId kGroupHeaderSelectedTextColorId =
    kColorAstraSidebarItemSelectedText;
constexpr ui::ColorId kGroupHeaderDragHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

// Placeholder color for the dot when no ColorProvider is available.
constexpr SkColor kDefaultColorDotColor = SK_ColorGRAY;

}  // namespace

AstraTabGroupHeaderView::AstraTabGroupHeaderView(
    const std::u16string& title,
    SkColor color,
    ToggleCallback toggle_callback,
    NewTabCallback new_tab_callback)
    : Button(base::BindRepeating(
          [](AstraTabGroupHeaderView* header, const ui::Event& event) {
            if (header->toggle_callback_) {
              header->toggle_callback_.Run();
            }
          },
          base::Unretained(this))),
      group_color_(color),
      toggle_callback_(std::move(toggle_callback)),
      new_tab_callback_(std::move(new_tab_callback)) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kGroupHeaderCornerRadius));

  BuildLayout();
  SetName(title);
  SetColor(color);
}

AstraTabGroupHeaderView::~AstraTabGroupHeaderView() = default;

void AstraTabGroupHeaderView::BuildLayout() {
  // Horizontal box layout: [dot] [chevron] [title...] [pin] [count] [+] [menu]
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kGroupHeaderHorizontalPadding), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Colored dot (left side).
  color_dot_ = AddChildView(std::make_unique<views::ImageView>());
  color_dot_->SetImageSize(
      gfx::Size(kGroupHeaderDotSize, kGroupHeaderDotSize));
  color_dot_->SetCanProcessEventsWithinSubtree(false);

  // Spacing after dot.
  auto* spacer_dot = AddChildView(std::make_unique<views::View>());
  spacer_dot->SetPreferredSize(gfx::Size(kGroupHeaderDotSpacing, 0));

  // Expand/collapse chevron.
  // TODO(astra): Use a real chevron icon from views::VectorIcon or
  // ui::ResourceBundle instead of a placeholder ImageView.
  // Chromium pattern: TabGroupHeader uses vector icons from tab_icons.h.
  chevron_ = AddChildView(std::make_unique<views::ImageView>());
  chevron_->SetImageSize(
      gfx::Size(kGroupHeaderChevronSize, kGroupHeaderChevronSize));
  chevron_->SetCanProcessEventsWithinSubtree(false);

  // Spacing between chevron and title.
  auto* spacer_chevron = AddChildView(std::make_unique<views::View>());
  spacer_chevron->SetPreferredSize(gfx::Size(kGroupHeaderChevronSpacing, 0));

  // Group title label.
  title_label_ = AddChildView(std::make_unique<views::Label>());
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  layout->SetFlexForView(title_label_, 1);

  // Spacing between title and pin indicator.
  auto* spacer_pin = AddChildView(std::make_unique<views::View>());
  spacer_pin->SetPreferredSize(gfx::Size(kGroupHeaderTitleSpacing, 0));

  // Pin indicator.
  pin_indicator_ = AddChildView(std::make_unique<views::ImageView>());
  pin_indicator_->SetImageSize(
      gfx::Size(kPinIndicatorSize, kPinIndicatorSize));
  pin_indicator_->SetCanProcessEventsWithinSubtree(false);
  pin_indicator_->SetVisible(false);

  // Spacing between pin and count.
  auto* spacer_count = AddChildView(std::make_unique<views::View>());
  spacer_count->SetPreferredSize(gfx::Size(kPinIndicatorSpacing, 0));

  // Tab count label (shown in parentheses).
  count_label_ = AddChildView(std::make_unique<views::Label>());
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);

  // Spacing before new tab button.
  auto* spacer_newtab = AddChildView(std::make_unique<views::View>());
  spacer_newtab->SetPreferredSize(gfx::Size(kNewTabButtonSpacing, 0));

  // "New tab in group" button (plus icon).
  // TODO(astra): Use a proper ImageButton with a vector icon (plus sign).
  // For now, use an ImageView placeholder with a click handler.
  // Chromium pattern: chrome/browser/ui/views/tabs/new_tab_button.h
  new_tab_button_ = AddChildView(std::make_unique<views::ImageView>());
  new_tab_button_->SetImageSize(
      gfx::Size(kNewTabButtonSize, kNewTabButtonSize));
  new_tab_button_->SetCanProcessEventsWithinSubtree(false);
  new_tab_button_->SetVisible(false);

  // Spacing before menu button.
  auto* spacer_menu = AddChildView(std::make_unique<views::View>());
  spacer_menu->SetPreferredSize(gfx::Size(kMenuButtonSpacing, 0));

  // Menu button (three-dot or chevron).
  // TODO(astra): Use a proper ImageButton with a vector icon.
  menu_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          [](AstraTabGroupHeaderView* header, const ui::Event& event) {
            if (header->menu_callback_) {
              header->menu_callback_.Run();
            }
          },
          base::Unretained(this))));
  menu_button_->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  menu_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  menu_button_->SetPreferredSize(
      gfx::Size(kMenuButtonSize, kMenuButtonSize));
  menu_button_->SetVisible(false);
  menu_button_->SetTooltipText(u"Group options");
  menu_button_->SetFocusBehavior(FocusBehavior::ALWAYS);
}

// =========================================================================
// Group info
// =========================================================================

void AstraTabGroupHeaderView::SetGroupInfo(const AstraTabGroupInfo& info) {
  group_id_ = info.group_id;
  SetName(info.name);
  SetColor(info.color);
  SetColorId(info.color_id);
  SetTabCount(info.tab_count);
  SetExpanded(info.is_expanded);
  SetCollapsedInTabstrip(info.is_collapsed_in_tabstrip);
  SetPinned(info.is_pinned);
  // Note: last_accessed, created_time, order_index, note are metadata
  // used by the parent view for sorting/filtering, not rendered here.
}

// =========================================================================
// Name
// =========================================================================

void AstraTabGroupHeaderView::SetName(const std::u16string& name) {
  title_label_->SetText(name);
  SetAccessibleName(name);
}

std::u16string AstraTabGroupHeaderView::GetName() const {
  return title_label_ ? title_label_->GetText() : std::u16string();
}

// =========================================================================
// Color
// =========================================================================

void AstraTabGroupHeaderView::SetColor(SkColor color) {
  group_color_ = color;
  UpdateColorDot();
  OnThemeChanged();
}

void AstraTabGroupHeaderView::SetColorId(int color_id) {
  color_id_ = color_id;
  // Note: color_id_ is primarily used for mapping back to Chromium's
  // tab_groups::TabGroupColorId. The actual rendered color is set via
  // SetColor() which takes an SkColor.
  // TODO(astra): When ColorProvider is available, resolve the color ID
  // to an SkColor through the color provider system instead of
  // relying on the caller to pass the resolved color.
}

void AstraTabGroupHeaderView::UpdateColorDot() {
  if (!color_dot_) {
    return;
  }
  // TODO(astra): Draw a proper colored circle using Skia or a vector icon.
  // For now, we rely on the background color of the image view or a
  // placeholder color. The real implementation would use:
  //   gfx::ImageSkiaOperations::CreateColorMask() or a solid color image.
  //
  // Chromium pattern: TabGroupHeader uses a vector icon colored via
  // the color provider. See chrome/browser/ui/views/tabs/tab_group_header.cc
  //
  // For unit test purposes, the color is stored in group_color_ and can
  // be queried via GetColor().
  color_dot_->SchedulePaint();
}

// =========================================================================
// Tab count
// =========================================================================

void AstraTabGroupHeaderView::SetTabCount(int count) {
  tab_count_ = count;
  count_label_->SetText(u"(" + base::NumberToString16(count) + u")");
  UpdateChildVisibility();
}

// =========================================================================
// Expansion
// =========================================================================

void AstraTabGroupHeaderView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;
  UpdateChevron();
  SchedulePaint();
}

void AstraTabGroupHeaderView::UpdateChevron() {
  if (!chevron_) {
    return;
  }
  // TODO(astra): Rotate the chevron icon to reflect expanded state.
  // In Chromium, the chevron points right when collapsed and down when expanded.
  // For now we just update the state; icon rotation will be added when real
  // vector icons are wired in.
  //
  // Chromium pattern: views::ImageView::SetImage with rotated transform.
  chevron_->SchedulePaint();
}

// =========================================================================
// Selection
// =========================================================================

void AstraTabGroupHeaderView::SetSelected(bool selected) {
  if (is_selected_ == selected) {
    return;
  }
  is_selected_ = selected;
  OnThemeChanged();
}

// =========================================================================
// Pinned state
// =========================================================================

void AstraTabGroupHeaderView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  if (pin_indicator_) {
    pin_indicator_->SetVisible(pinned);
  }
  UpdateChildVisibility();
  InvalidateLayout();
}

// =========================================================================
// Visibility toggles for child elements
// =========================================================================

void AstraTabGroupHeaderView::SetShowChevron(bool show) {
  if (show_chevron_ == show) {
    return;
  }
  show_chevron_ = show;
  UpdateChildVisibility();
}

void AstraTabGroupHeaderView::SetShowTabCount(bool show) {
  if (show_tab_count_ == show) {
    return;
  }
  show_tab_count_ = show;
  UpdateChildVisibility();
}

void AstraTabGroupHeaderView::SetShowColorDot(bool show) {
  if (show_color_dot_ == show) {
    return;
  }
  show_color_dot_ = show;
  UpdateChildVisibility();
}

void AstraTabGroupHeaderView::SetShowMenuButton(bool show) {
  if (show_menu_button_ == show) {
    return;
  }
  show_menu_button_ = show;
  UpdateChildVisibility();
}

void AstraTabGroupHeaderView::UpdateChildVisibility() {
  if (color_dot_) {
    color_dot_->SetVisible(show_color_dot_);
  }
  if (chevron_) {
    chevron_->SetVisible(show_chevron_);
  }
  if (count_label_) {
    count_label_->SetVisible(show_tab_count_);
  }
  if (menu_button_) {
    menu_button_->SetVisible(show_menu_button_);
  }
  // Pin indicator visibility is controlled by SetPinned(), not a show flag,
  // since it reflects actual group state rather than a display preference.
  InvalidateLayout();
}

// =========================================================================
// Compact mode
// =========================================================================

void AstraTabGroupHeaderView::SetCompact(bool compact) {
  if (is_compact_ == compact) {
    return;
  }
  is_compact_ = compact;
  // Update padding based on compact mode.
  if (auto* layout = GetLayoutManager()) {
    // TODO(astra): Update the BoxLayout insets directly.
    // For now, we invalidate layout and rely on preferred size changes.
  }
  // Update font size for compact mode.
  if (title_label_) {
    // TODO(astra): Adjust font size for compact mode.
  }
  InvalidateLayout();
}

// =========================================================================
// Tabstrip collapsed state
// =========================================================================

void AstraTabGroupHeaderView::SetCollapsedInTabstrip(bool collapsed) {
  if (is_collapsed_in_tabstrip_ == collapsed) {
    return;
  }
  is_collapsed_in_tabstrip_ = collapsed;
  // TODO(astra): Update visual indicator for tabstrip-collapsed state.
  // Could dim the text or show a different icon.
  SchedulePaint();
}

// =========================================================================
// Drag hover state
// =========================================================================

void AstraTabGroupHeaderView::SetIsDragHovered(bool hovered) {
  if (is_drag_hovered_ == hovered) {
    return;
  }
  is_drag_hovered_ = hovered;
  OnThemeChanged();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraTabGroupHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::Button::CalculatePreferredSize(available_size);
  int height = is_compact_ ? kGroupHeaderCompactHeight : kGroupHeaderHeight;
  size.set_height(std::max(size.height(), height));
  return size;
}

void AstraTabGroupHeaderView::OnThemeChanged() {
  views::Button::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update text colors.
  SkColor text_color = color_provider->GetColor(kGroupHeaderTextColorId);
  if (is_selected_) {
    text_color = color_provider->GetColor(kGroupHeaderSelectedTextColorId);
  }
  title_label_->SetEnabledColor(text_color);
  count_label_->SetEnabledColor(text_color);

  // Update background color based on state.
  SkColor bg_color = GetBackgroundColor();
  if (layer()) {
    layer()->SetColor(bg_color);
  }

  // Update the color dot if it's visible.
  if (show_color_dot_ && color_dot_) {
    // TODO(astra): Create a proper colored circle image.
    // For now, the color is stored and accessible via GetColor().
  }
}

SkColor AstraTabGroupHeaderView::GetBackgroundColor() const {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return SK_ColorTRANSPARENT;
  }

  if (is_selected_) {
    return color_provider->GetColor(kGroupHeaderSelectedBgColorId);
  }
  if (is_drag_hovered_) {
    return color_provider->GetColor(kGroupHeaderDragHoverBgColorId);
  }
  if (GetState() == STATE_HOVERED) {
    return color_provider->GetColor(kGroupHeaderHoverBgColorId);
  }
  return SK_ColorTRANSPARENT;
}

void AstraTabGroupHeaderView::OnMouseEntered(const ui::MouseEvent& event) {
  views::Button::OnMouseEntered(event);
  // Show the new tab button on hover.
  if (new_tab_button_) {
    new_tab_button_->SetVisible(true);
  }
}

void AstraTabGroupHeaderView::OnMouseExited(const ui::MouseEvent& event) {
  views::Button::OnMouseExited(event);
  // Hide the new tab button when not hovering.
  if (new_tab_button_) {
    new_tab_button_->SetVisible(false);
  }
}

}  // namespace astra
