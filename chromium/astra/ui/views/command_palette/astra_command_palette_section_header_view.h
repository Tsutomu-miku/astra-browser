#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
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
// (e.g. "Tabs", "Workspaces") with an optional icon and a subtle
// horizontal divider line.
//
// Layout:
//   +---------------------------------------------------------------+
//   |  [ICON]  Category Name                                        |
//   +---------------------------------------------------------------+
//
// TODO(astra): Replace text-based icon with a proper vector icon using
// gfx::VectorIcon and views::ImageView.
//   Chromium component: ui/gfx/vector_icon_types.h
//   Patch point: //chrome/app/theme vector icons
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

  // Set the icon text (short string displayed next to the label).
  // Pass empty string to hide the icon.
  void SetIcon(const std::u16string& icon_text);

  // Show or hide the icon.
  void ShowIcon(bool show);
  bool show_icon() const { return show_icon_; }

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  // Build the child views and layout.
  void BuildLayout();

  // Update text colors from the color provider.
  void UpdateTextColors();

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> icon_container_ = nullptr;
  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> label_ = nullptr;
  raw_ptr<views::View> divider_ = nullptr;

  // Whether the icon is visible.
  bool show_icon_ = true;

  // The current icon text.
  std::u16string icon_text_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_SECTION_HEADER_VIEW_H_
