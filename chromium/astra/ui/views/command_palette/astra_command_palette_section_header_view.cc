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

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraCommandPaletteSectionHeaderView::AstraCommandPaletteSectionHeaderView(
    const std::u16string& label) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kSectionHeaderTopMargin, kSectionHeaderHorizontalPadding,
                      kSectionHeaderVerticalPadding,
                      kSectionHeaderHorizontalPadding),
      0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Category label.
  label_ = AddChildView(std::make_unique<views::Label>(label));
  label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_->SetAutoColorReadabilityEnabled(false);
  label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kSectionLabelFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::SEMIBOLD));
  label_->SetElideBehavior(gfx::ELIDE_END);
}

AstraCommandPaletteSectionHeaderView::
    ~AstraCommandPaletteSectionHeaderView() = default;

// =========================================================================
// Public API
// =========================================================================

void AstraCommandPaletteSectionHeaderView::SetLabel(
    const std::u16string& label) {
  label_->SetText(label);
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
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
}

}  // namespace astra
