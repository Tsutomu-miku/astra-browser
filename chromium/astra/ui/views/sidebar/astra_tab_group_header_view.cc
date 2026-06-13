#include "astra/ui/views/sidebar/astra_tab_group_header_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/i18n/number_formatting.h"
#include "chrome/browser/ui/tabs/tab_group_color.h"
#include "chrome/browser/ui/tabs/tab_group_theme.h"
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
#include "ui/views/view_utils.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kGroupHeaderHeight = 30;
constexpr int kGroupHeaderHorizontalPadding = 12;
constexpr int kGroupHeaderDotSize = 10;
constexpr int kGroupHeaderDotSpacing = 8;
constexpr int kGroupHeaderChevronSize = 12;
constexpr int kGroupHeaderChevronSpacing = 4;
constexpr int kGroupHeaderTitleSpacing = 4;
constexpr int kGroupHeaderCornerRadius = 6;
constexpr int kNewTabButtonSize = 16;
constexpr int kNewTabButtonSpacing = 4;

// Astra color IDs for tab group header styling.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kGroupHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kGroupHeaderHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraTabGroupHeaderView::AstraTabGroupHeaderView(
    const std::u16string& title,
    tab_groups::TabGroupColorId color,
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
  SetTitle(title);
  SetGroupColor(color);
}

AstraTabGroupHeaderView::~AstraTabGroupHeaderView() = default;

void AstraTabGroupHeaderView::BuildLayout() {
  // Horizontal box layout: [dot] [chevron] [title...] [count] [+button]
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

  layout->SetFlexForView(AddChildView(std::make_unique<views::View>()), 0);
  // Add spacing after dot.
  auto* spacer1 = AddChildView(std::make_unique<views::View>());
  spacer1->SetPreferredSize(gfx::Size(kGroupHeaderDotSpacing, 0));

  // Expand/collapse chevron.
  // TODO(astra): Use a real chevron icon from views::VectorIcon or
  // ui::ResourceBundle instead of a placeholder ImageView.
  // Chromium pattern: TabGroupHeader uses vector icons from tab_icons.h.
  chevron_ = AddChildView(std::make_unique<views::ImageView>());
  chevron_->SetImageSize(
      gfx::Size(kGroupHeaderChevronSize, kGroupHeaderChevronSize));
  chevron_->SetCanProcessEventsWithinSubtree(false);

  // Spacing between chevron and title.
  auto* spacer2 = AddChildView(std::make_unique<views::View>());
  spacer2->SetPreferredSize(gfx::Size(kGroupHeaderChevronSpacing, 0));

  // Group title label.
  title_label_ = AddChildView(std::make_unique<views::Label>());
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  layout->SetFlexForView(title_label_, 1);

  // Spacing between title and count.
  auto* spacer3 = AddChildView(std::make_unique<views::View>());
  spacer3->SetPreferredSize(gfx::Size(kGroupHeaderTitleSpacing, 0));

  // Tab count label (shown in parentheses).
  count_label_ = AddChildView(std::make_unique<views::Label>());
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);

  // Spacing before new tab button.
  auto* spacer4 = AddChildView(std::make_unique<views::View>());
  spacer4->SetPreferredSize(gfx::Size(kNewTabButtonSpacing, 0));

  // "New tab in group" button (plus icon).
  // TODO(astra): Use a proper ImageButton with a vector icon (plus sign).
  // For now, use an ImageView placeholder with a click handler.
  // Chromium pattern: chrome/browser/ui/views/tabs/new_tab_button.h
  new_tab_button_ = AddChildView(std::make_unique<views::ImageView>());
  new_tab_button_->SetImageSize(
      gfx::Size(kNewTabButtonSize, kNewTabButtonSize));
  new_tab_button_->SetCanProcessEventsWithinSubtree(false);

  // Make the "new tab" button clickable without triggering the header toggle.
  // We install a mouse event handler on the button that stops propagation.
  // TODO(astra): Replace with a proper ImageButton that handles its own press.
}

void AstraTabGroupHeaderView::SetTitle(const std::u16string& title) {
  title_label_->SetText(title);
}

void AstraTabGroupHeaderView::SetTabCount(int count) {
  count_label_->SetText(u"(" + base::NumberToString16(count) + u")");
}

void AstraTabGroupHeaderView::SetGroupColor(tab_groups::TabGroupColorId color) {
  group_color_ = color;
  // TODO(astra): Set the color dot's actual color using TabGroupColorUtils
  // or the ColorProvider. For now, we defer to OnThemeChanged which reads
  // the color from the color provider.
  //
  // Chromium's tab group colors are defined in:
  //   chrome/browser/ui/tabs/tab_group_color.h
  //   chrome/browser/ui/tabs/tab_group_theme.h
  // The proper approach is to use GetTabGroupSystemColor() or similar.
  OnThemeChanged();
}

void AstraTabGroupHeaderView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;
  // TODO(astra): Rotate the chevron icon to reflect expanded state.
  // In Chromium, the chevron points right when collapsed and down when expanded.
  // For now we just update the state; icon rotation will be added when real
  // vector icons are wired in.
  OnThemeChanged();
}

gfx::Size AstraTabGroupHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::Button::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kGroupHeaderHeight));
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
  title_label_->SetEnabledColor(text_color);
  count_label_->SetEnabledColor(text_color);

  // Update background color based on hover state.
  SkColor bg_color = SK_ColorTRANSPARENT;
  if (GetState() == STATE_HOVERED) {
    bg_color = color_provider->GetColor(kGroupHeaderHoverBgColorId);
  }
  if (layer()) {
    layer()->SetColor(bg_color);
  }

  // TODO(astra): Set the color dot's background color from the tab group
  // color palette. Chromium provides GetTabGroupForegroundColor() and
  // GetTabGroupSystemColor() in tab_group_theme.h. We should use those
  // via the ColorProvider or directly from the TabGroup model.
  //
  // For now, paint the dot with a placeholder color. The real implementation
  // would use something like:
  //   SkColor dot_color = color_provider->GetColor(
  //       GetTabGroupSystemColorID(group_color_));
  //
  // Chromium owner: tab_groups::GetTabGroupSystemColorID()
  // (chrome/browser/ui/tabs/tab_group_theme.h)
  if (color_dot_) {
    // Placeholder: draw a simple colored dot.
    // TODO(astra): Replace with a proper colored circle via Skia or a vector.
    // Using the ImageView with a solid color image is a stopgap.
  }
}

}  // namespace astra
