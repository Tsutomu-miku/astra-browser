#include "astra/ui/views/command_palette/astra_command_palette_section_header_view.h"

#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

#include "astra/ui/color/astra_color_ids.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSectionHeaderVerticalPadding = 8;
constexpr int kSectionHeaderHorizontalPadding = 16;
constexpr int kSectionHeaderTopMargin = 4;
constexpr int kSectionLabelFontSizeDelta = -1;
constexpr int kSectionIconSize = 14;
constexpr int kSectionIconSpacing = 8;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraCommandPaletteSectionHeaderView::AstraCommandPaletteSectionHeaderView(
    const std::u16string& label) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  BuildLayout();
  SetLabel(label);
}

AstraCommandPaletteSectionHeaderView::
    ~AstraCommandPaletteSectionHeaderView() = default;

// =========================================================================
// Layout construction
// =========================================================================

void AstraCommandPaletteSectionHeaderView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kSectionHeaderTopMargin, kSectionHeaderHorizontalPadding,
                      kSectionHeaderVerticalPadding,
                      kSectionHeaderHorizontalPadding),
      0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Row: icon + label.
  auto* row = AddChildView(std::make_unique<views::View>());
  row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kSectionIconSpacing));
  static_cast<views::BoxLayout*>(row->GetLayoutManager())
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon container.
  icon_container_ = row->AddChildView(std::make_unique<views::View>());
  icon_container_->SetPreferredSize(
      gfx::Size(kSectionIconSize, kSectionIconSize));
  icon_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  static_cast<views::BoxLayout*>(icon_container_->GetLayoutManager())
      ->set_main_axis_alignment(
          views::BoxLayout::MainAxisAlignment::kCenter);
  static_cast<views::BoxLayout*>(icon_container_->GetLayoutManager())
      ->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kCenter);

  icon_label_ = icon_container_->AddChildView(
      std::make_unique<views::Label>(std::u16string()));
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetAutoColorReadabilityEnabled(false);
  icon_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  // Category label.
  label_ = row->AddChildView(std::make_unique<views::Label>(std::u16string()));
  label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_->SetAutoColorReadabilityEnabled(false);
  label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kSectionLabelFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::SEMIBOLD));
  label_->SetElideBehavior(gfx::ELIDE_END);
  static_cast<views::BoxLayout*>(row->GetLayoutManager())
      ->SetFlexForView(label_, 1);
}

// =========================================================================
// Public API
// =========================================================================

void AstraCommandPaletteSectionHeaderView::SetLabel(
    const std::u16string& label) {
  label_->SetText(label);
}

void AstraCommandPaletteSectionHeaderView::SetIcon(
    const std::u16string& icon_text) {
  icon_text_ = icon_text;
  if (icon_label_) {
    icon_label_->SetText(icon_text);
  }
  if (!icon_text.empty()) {
    ShowIcon(true);
  }
}

void AstraCommandPaletteSectionHeaderView::ShowIcon(bool show) {
  if (show_icon_ == show) {
    return;
  }
  show_icon_ = show;
  if (icon_container_) {
    icon_container_->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraCommandPaletteSectionHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraCommandPaletteSectionHeaderView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateTextColors();
}

void AstraCommandPaletteSectionHeaderView::UpdateTextColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));

  if (icon_label_) {
    icon_label_->SetEnabledColor(
        color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
  }
}

}  // namespace astra
