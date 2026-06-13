#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_

#include <string>

#include "ui/views/view.h"

namespace views {
class Label;
}  // namespace views

namespace astra {

// =========================================================================
// Section header for command palette category groups
// =========================================================================
//
// A lightweight section header view that appears between groups of
// command results in the command palette.  Shows a category label
// (e.g. "Tabs", "Workspaces") with a subtle horizontal divider line.
//
// Layout:
//   +---------------------------------------------------------------+
//   |  Category Name                                                |
//   +---------------------------------------------------------------+
//
// TODO(astra): Consider adding a small category icon next to the label
// for faster visual scanning.  Chromium component: ui/views/controls/image_view.h
// =========================================================================

class AstraCommandPaletteSectionHeaderView : public views::View {
 public:
  explicit AstraCommandPaletteSectionHeaderView(const std::u16string& label);
  ~AstraCommandPaletteSectionHeaderView() override;

  AstraCommandPaletteSectionHeaderView(
      const AstraCommandPaletteSectionHeaderView&) = delete;
  AstraCommandPaletteSectionHeaderView& operator=(
      const AstraCommandPaletteSectionHeaderView&) = delete;

  // Update the section header label text.
  void SetLabel(const std::u16string& label);

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  // Child views (owned by the view hierarchy).
  raw_ptr<views::Label> label_ = nullptr;
  raw_ptr<views::View> divider_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_
