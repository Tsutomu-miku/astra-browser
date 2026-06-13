#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_ITEM_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/range/range.h"
#include "ui/views/view.h"

#include "astra/ui/views/command_palette/astra_command_palette_model.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace astra {

// =========================================================================
// A single command result item in the command palette list.
// =========================================================================
//
// Layout:
//   +---------------------------------------------------------------+
//   |  [ICON]  Command Title (with match highlights)  Shortcut     |
//   |          Description / type indicator (secondary)            |
//   +---------------------------------------------------------------+
//
// Shows:
//   - Icon (left side).
//   - Command title (primary label, with match highlighting).
//   - Keyboard shortcut hint (right-aligned, secondary color).
//   - Description / category label (below name, lighter color).
//   - Type indicator badge (optional, next to description).
//   - Selected / hover / highlighted state highlighting.
//
// This is a pure presentation view — it does not own product state.
// Data is projected from the command palette model via AstraCommandItem.
//
// Accessibility:
//   - Accessible role: kListItem
//   - Accessible name: command title
//   - Accessible description: command description
//
// TODO(astra): Add matched text highlighting.  The title and description
// labels should bold or color the characters that matched the search query.
// This can be done with views::StyledLabel or by manually setting text
// styles on ranges.  Chromium component: ui/views/controls/styled_label.
//
// TODO(astra): Use a proper button base class (views::Button or
// views::LabelButton) so the item has proper hover/active states and
// accessibility.  Currently uses a plain View with click handlers.
//
// TODO(astra): Replace icon placeholder with a real vector icon.
// Chromium component: ui/gfx/vector_icon_types.h
// Patch point: //chrome/app/theme vector icons, or a custom Astra icon set.
// =========================================================================

class AstraCommandPaletteItemView : public views::View {
 public:
  // Construct with all display data.
  AstraCommandPaletteItemView(const std::u16string& title,
                              const std::u16string& description,
                              const std::u16string& shortcut,
                              const std::string& icon_name,
                              bool is_astra);

  // Construct from an AstraCommandItem.
  explicit AstraCommandPaletteItemView(const AstraCommandItem& command);

  ~AstraCommandPaletteItemView() override;

  AstraCommandPaletteItemView(const AstraCommandPaletteItemView&) = delete;
  AstraCommandPaletteItemView& operator=(
      const AstraCommandPaletteItemView&) = delete;

  // -- Command data ------------------------------------------------------

  // Set the full command data from an AstraCommandItem.
  void SetCommand(const AstraCommandItem& command);

  // Get the current command data.
  const AstraCommandItem& GetCommand() const { return command_; }

  // -- State -------------------------------------------------------------

  // Set the item as selected (highlighted background).
  void SetSelected(bool selected);
  bool IsSelected() const { return selected_; }

  // Set the item as highlighted (e.g. for keyboard focus indication).
  // Highlighted is a visual state distinct from selection — a hovered
  // or keyboard-focused item can be highlighted without being selected.
  void SetHighlighted(bool highlighted);
  bool IsHighlighted() const { return highlighted_; }

  // -- Match highlighting ------------------------------------------------

  // Set the ranges of text that matched the search query.
  // These ranges are used to visually highlight matched characters in
  // the title and description labels.
  void SetMatchRanges(const std::vector<gfx::Range>& ranges);
  const std::vector<gfx::Range>& match_ranges() const {
    return match_ranges_;
  }

  // -- Visibility toggles ------------------------------------------------

  // Show or hide the icon.
  void ShowIcon(bool show);
  bool show_icon() const { return show_icon_; }

  // Show or hide the shortcut text.
  void ShowShortcut(bool show);
  bool show_shortcut() const { return show_shortcut_; }

  // Show or hide the description.
  void ShowDescription(bool show);
  bool show_description() const { return show_description_; }

  // Show or hide the category / type badge.
  void ShowCategoryBadge(bool show);
  bool show_category_badge() const { return show_category_badge_; }

  // Show or hide the "recent" indicator badge.
  void ShowRecentBadge(bool show);
  bool show_recent_badge() const { return show_recent_badge_; }

  // -- Legacy API (kept for backward compatibility) ----------------------

  // Update the display data (used when refreshing results).
  void UpdateContent(const std::u16string& title,
                     const std::u16string& description,
                     const std::u16string& shortcut,
                     const std::u16string& icon_label);

  // Show or hide the description label (legacy name).
  void SetShowDescription(bool show) { ShowDescription(show); }
  bool show_description() const { return show_shortcut_; }

  // Show or hide the shortcut label (legacy name).
  void SetShowShortcut(bool show) { ShowShortcut(show); }
  bool show_shortcut() const { return show_shortcut_; }

  // -- Callback ----------------------------------------------------------

  // Callback invoked when the item is clicked or activated via keyboard.
  using ActivatedCallback = base::RepeatingClosure;
  void SetActivatedCallback(ActivatedCallback callback) {
    activated_callback_ = std::move(callback);
  }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout.  Called from constructor.
  void BuildLayout();

  // Update the background color based on selection, highlight, and hover.
  void UpdateBackground();

  // Update text colors from the color provider.
  void UpdateTextColors();

  // Update all text content from command_.
  void UpdateTextContent();

  // Apply match highlighting to the title label.
  void ApplyMatchHighlighting();

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> icon_container_ = nullptr;
  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> shortcut_label_ = nullptr;
  raw_ptr<views::Label> description_label_ = nullptr;
  raw_ptr<views::Label> type_badge_label_ = nullptr;
  raw_ptr<views::View> recent_badge_container_ = nullptr;
  raw_ptr<views::Label> recent_badge_label_ = nullptr;

  // The current command data being displayed.
  AstraCommandItem command_;

  // Visual state flags.
  bool selected_ = false;
  bool highlighted_ = false;
  bool hovered_ = false;

  // Visibility toggles.
  bool show_icon_ = true;
  bool show_shortcut_ = true;
  bool show_description_ = true;
  bool show_category_badge_ = true;
  bool show_recent_badge_ = false;

  // Match ranges for search highlighting.
  std::vector<gfx::Range> match_ranges_;

  // Callback.
  ActivatedCallback activated_callback_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_ITEM_VIEW_H_
