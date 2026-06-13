#include "astra/ui/views/command_palette/astra_command_palette_item_view.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"

#include "astra/ui/color/astra_color_ids.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kItemVerticalPadding = 10;
constexpr int kItemHorizontalPadding = 12;
constexpr int kIconSize = 20;
constexpr int kIconSpacing = 12;
constexpr int kNameShortcutSpacing = 8;
constexpr int kDescriptionLineHeight = 18;
constexpr int kDescriptionTopMargin = 2;
constexpr int kTypeBadgeLeftMargin = 8;
constexpr int kTypeBadgePaddingH = 6;
constexpr int kTypeBadgePaddingV = 2;

// Font sizes relative to default.
constexpr int kTitleFontSizeDelta = 1;
constexpr int kDescriptionFontSizeDelta = -1;
constexpr int kShortcutFontSizeDelta = -1;
constexpr int kTypeBadgeFontSizeDelta = -2;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraCommandPaletteItemView::AstraCommandPaletteItemView(
    const std::u16string& title,
    const std::u16string& description,
    const std::u16string& shortcut,
    const std::string& icon_name,
    bool is_astra) {
  command_.title = title;
  command_.description = description;
  command_.shortcut_text = shortcut;
  command_.icon_name = icon_name;
  command_.is_astra = is_astra;

  BuildLayout();
}

AstraCommandPaletteItemView::AstraCommandPaletteItemView(
    const AstraCommandItem& command)
    : command_(command) {
  BuildLayout();
}

AstraCommandPaletteItemView::~AstraCommandPaletteItemView() = default;

// =========================================================================
// Layout construction
// =========================================================================

void AstraCommandPaletteItemView::BuildLayout() {
  // Paint background for selection/hover highlighting.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Horizontal layout: icon + content area.
  auto* main_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(kItemVerticalPadding, kItemHorizontalPadding),
      kIconSpacing));
  main_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon placeholder (left side).
  // TODO(astra): Replace with a proper vector icon using views::ImageView
  // and gfx::VectorIcon.  Chromium component: ui/views/controls/image_view.h
  icon_container_ = AddChildView(std::make_unique<views::View>());
  icon_container_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  icon_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  static_cast<views::BoxLayout*>(icon_container_->GetLayoutManager())
      ->set_main_axis_alignment(
          views::BoxLayout::MainAxisAlignment::kCenter);
  static_cast<views::BoxLayout*>(icon_container_->GetLayoutManager())
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kCenter);

  std::u16string icon_label = base::UTF8ToUTF16(command_.icon_name.empty()
                                                    ? "◆"
                                                    : command_.icon_name.substr(
                                                          0, 2));
  icon_label_ = icon_container_->AddChildView(
      std::make_unique<views::Label>(icon_label));
  icon_label_->SetAutoColorReadabilityEnabled(false);
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  // Content area (title + shortcut on top, description + type badge below).
  auto* content = AddChildView(std::make_unique<views::View>());
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kDescriptionTopMargin));
  static_cast<views::BoxLayout*>(content->GetLayoutManager())
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kStretch);
  main_layout->SetFlexForView(content, 1);

  // Top row: title (flex) + shortcut (fixed).
  auto* top_row = content->AddChildView(std::make_unique<views::View>());
  auto* top_layout = top_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  top_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  top_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  top_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // Title label (bold, primary text).
  title_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(command_.title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kTitleFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::BOLD));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  top_layout->SetFlexForView(title_label_, 1);

  // Shortcut label (right-aligned, secondary text).
  shortcut_label_ =
      top_row->AddChildView(std::make_unique<views::Label>(
          command_.shortcut_text));
  shortcut_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  shortcut_label_->SetAutoColorReadabilityEnabled(false);
  shortcut_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kShortcutFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));

  // Bottom row: description (flex) + type badge (optional).
  auto* bottom_row = content->AddChildView(std::make_unique<views::View>());
  auto* bottom_layout = bottom_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  bottom_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  bottom_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  bottom_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // Description label (secondary text).
  description_label_ =
      bottom_row->AddChildView(
          std::make_unique<views::Label>(command_.description));
  description_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  description_label_->SetAutoColorReadabilityEnabled(false);
  description_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kDescriptionFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));
  description_label_->SetElideBehavior(gfx::ELIDE_END);
  bottom_layout->SetFlexForView(description_label_, 1);

  // Type badge label (small, pill-style indicator of command type).
  type_badge_label_ = bottom_row->AddChildView(
      std::make_unique<views::Label>(GetCommandTypeName(command_.type)));
  type_badge_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  type_badge_label_->SetAutoColorReadabilityEnabled(false);
  type_badge_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kTypeBadgeFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::BOLD));
  type_badge_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kTypeBadgePaddingV, kTypeBadgePaddingH)));

  // Recent badge (clock icon indicator for recently used commands).
  recent_badge_container_ = bottom_row->AddChildView(std::make_unique<views::View>());
  recent_badge_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kTypeBadgeLeftMargin), 0));
  static_cast<views::BoxLayout*>(recent_badge_container_->GetLayoutManager())
      ->set_main_axis_alignment(
          views::BoxLayout::MainAxisAlignment::kCenter);
  static_cast<views::BoxLayout*>(recent_badge_container_->GetLayoutManager())
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kCenter);

  recent_badge_label_ = recent_badge_container_->AddChildView(
      std::make_unique<views::Label>(u"◷"));
  recent_badge_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  recent_badge_label_->SetAutoColorReadabilityEnabled(false);
  recent_badge_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kTypeBadgeFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));
  recent_badge_container_->SetVisible(false);  // Hidden by default.

  // Set initial accessibility info.
  SetAccessibleName(command_.title);
  SetAccessibleDescription(command_.description);
}

// =========================================================================
// Command data
// =========================================================================

void AstraCommandPaletteItemView::SetCommand(const AstraCommandItem& command) {
  command_ = command;
  UpdateTextContent();
  ApplyMatchHighlighting();
  ShowRecentBadge(command.is_recent);
  SetAccessibleName(command_.title);
  SetAccessibleDescription(command_.description);
}

// =========================================================================
// Visual state
// =========================================================================

void AstraCommandPaletteItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  UpdateBackground();
  NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
}

void AstraCommandPaletteItemView::SetHighlighted(bool highlighted) {
  if (highlighted_ == highlighted) {
    return;
  }
  highlighted_ = highlighted;
  UpdateBackground();
}

// =========================================================================
// Match highlighting
// =========================================================================

void AstraCommandPaletteItemView::SetMatchRanges(
    const std::vector<gfx::Range>& ranges) {
  match_ranges_ = ranges;
  ApplyMatchHighlighting();
}

void AstraCommandPaletteItemView::ApplyMatchHighlighting() {
  // TODO(astra): Apply actual text styling for matched ranges.
  // This requires views::StyledLabel or a custom label implementation.
  // For now, match ranges are stored but not visually applied.
  // Chromium component: ui/views/controls/styled_label/styled_label.h
}

// =========================================================================
// Visibility toggles
// =========================================================================

void AstraCommandPaletteItemView::ShowIcon(bool show) {
  if (show_icon_ == show) {
    return;
  }
  show_icon_ = show;
  icon_container_->SetVisible(show);
  InvalidateLayout();
}

void AstraCommandPaletteItemView::ShowShortcut(bool show) {
  if (show_shortcut_ == show) {
    return;
  }
  show_shortcut_ = show;
  shortcut_label_->SetVisible(show);
  InvalidateLayout();
}

void AstraCommandPaletteItemView::ShowDescription(bool show) {
  if (show_description_ == show) {
    return;
  }
  show_description_ = show;
  description_label_->parent()->SetVisible(show);
  InvalidateLayout();
}

void AstraCommandPaletteItemView::ShowCategoryBadge(bool show) {
  if (show_category_badge_ == show) {
    return;
  }
  show_category_badge_ = show;
  type_badge_label_->SetVisible(show);
  InvalidateLayout();
}

void AstraCommandPaletteItemView::ShowRecentBadge(bool show) {
  if (show_recent_badge_ == show) {
    return;
  }
  show_recent_badge_ = show;
  recent_badge_container_->SetVisible(show);
  InvalidateLayout();
}

// =========================================================================
// Legacy content update
// =========================================================================

void AstraCommandPaletteItemView::UpdateContent(
    const std::u16string& title,
    const std::u16string& description,
    const std::u16string& shortcut,
    const std::u16string& icon_label) {
  command_.title = title;
  command_.description = description;
  command_.shortcut_text = shortcut;
  command_.icon_name = base::UTF16ToUTF8(icon_label);

  UpdateTextContent();
  SetAccessibleName(title);
  SetAccessibleDescription(description);
}

void AstraCommandPaletteItemView::UpdateTextContent() {
  title_label_->SetText(command_.title);
  description_label_->SetText(command_.description);
  shortcut_label_->SetText(command_.shortcut_text);

  std::u16string icon_label =
      base::UTF8ToUTF16(command_.icon_name.empty()
                            ? "◆"
                            : command_.icon_name.substr(0, 2));
  icon_label_->SetText(icon_label);

  type_badge_label_->SetText(GetCommandTypeName(command_.type));
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraCommandPaletteItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Let the layout manager compute the preferred size.
  return views::View::CalculatePreferredSize(available_size);
}

void AstraCommandPaletteItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateTextColors();
  UpdateBackground();
}

bool AstraCommandPaletteItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (activated_callback_) {
    activated_callback_.Run();
  }
  return true;
}

void AstraCommandPaletteItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  hovered_ = true;
  UpdateBackground();
}

void AstraCommandPaletteItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  hovered_ = false;
  UpdateBackground();
}

void AstraCommandPaletteItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;
  if (title_label_) {
    node_data->SetName(title_label_->GetText());
  }
  if (description_label_) {
    node_data->SetDescription(description_label_->GetText());
  }
  node_data->AddState(ax::mojom::State::kSelectable);
  if (selected_) {
    node_data->AddState(ax::mojom::State::kSelected);
  }
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraCommandPaletteItemView::UpdateBackground() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider || !layer()) {
    return;
  }

  SkColor color = SK_ColorTRANSPARENT;
  if (selected_) {
    color =
        color_provider->GetColor(kColorAstraCommandPaletteSelectedBackground);
  } else if (highlighted_) {
    // Highlighted state uses a slightly different shade than selection.
    // TODO(astra): Add a dedicated highlight color to Astra color IDs.
    color = color_provider->GetColor(kColorAstraSidebarItemHoverBackground);
  } else if (hovered_) {
    // Use a subtle hover color.
    // TODO(astra): Add a dedicated hover color to Astra color IDs.
    color = color_provider->GetColor(kColorAstraSidebarItemHoverBackground);
  }
  layer()->SetColor(color);
}

void AstraCommandPaletteItemView::UpdateTextColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  title_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteText));
  description_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
  shortcut_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteShortcutText));
  icon_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteText));

  // Type badge uses a muted color.
  type_badge_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));

  // Recent badge uses a subtle accent-like color.
  recent_badge_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteShortcutText));
}

}  // namespace astra
