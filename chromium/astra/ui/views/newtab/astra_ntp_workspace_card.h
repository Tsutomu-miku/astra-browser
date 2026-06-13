#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_WORKSPACE_CARD_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_WORKSPACE_CARD_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace astra {

// =========================================================================
// AstraNtpWorkspaceCard — workspace card on the new tab page
// =========================================================================
//
// A compact workspace card shown in the NTP workspace quick-access section.
// Shows an accent color header bar, workspace name, tab count badge, and a
// menu button (⋮) for workspace actions.  Clicking the card switches to
// that workspace.
//
// A special "New workspace" card can be shown at the end of the list that
// triggers creation of a new workspace.  This variant shows a plus icon
// and "New workspace" text.
//
// This is a presentation-only view.  All data is pushed in by the parent.
// It never reads directly from services.
//
// Visual layout (regular workspace card):
//   +---------------------------+
//   | [accent header bar]       |  ← full-width accent color bar
//   +---------------------------+
//   |  Workspace Name      [⋮] |  ← name + menu button
//   |  N tabs                  |  ← tab count badge
//   +---------------------------+
//
// Visual layout (new workspace card):
//   +---------------------------+
//   |  [+]                      |  ← plus icon (accent color)
//   |  New workspace            |  ← label
//   +---------------------------+
//
// States:
//   - Default: white background, subtle border.
//   - Hover: lighter background, subtle shadow.
//   - Active (current workspace): accent-colored border.
//   - Focus: focus ring around the card.
//
// Chromium owner: NTP workspace tiles (Chrome has no native NTP workspace
// concept — this is Astra-specific).
//
// Similar to: AstraWorkspaceCardView (workspace overview) but smaller and
// simpler — designed for the NTP sidebar/section, not the full overview.
//
// TODO(astra): Add proper tab count badge styling (pill-shaped badge
// with count inside).  Current version is just a text label.
// =========================================================================

class AstraNtpWorkspaceCard : public views::View {
 public:
  // Callback type for when the card is clicked.
  // Parameter: workspace id (empty string for "new workspace" card).
  using ClickCallback = base::RepeatingCallback<void(const std::string&)>;

  // Callback type for when the menu button (⋮) is pressed.
  // Parameters: workspace id, screen point for menu anchor.
  using MenuCallback = base::RepeatingCallback<void(const std::string&,
                                                    const gfx::Point&)>;

  // Callback type for when drag starts on the card.
  using DragStartCallback =
      base::RepeatingCallback<void(AstraNtpWorkspaceCard*)>;

  AstraNtpWorkspaceCard();
  AstraNtpWorkspaceCard(const AstraNtpWorkspaceCard&) = delete;
  AstraNtpWorkspaceCard& operator=(const AstraNtpWorkspaceCard&) = delete;
  ~AstraNtpWorkspaceCard() override;

  // -- Data setters (called by parent) -----------------------------------

  void SetWorkspaceId(const std::string& id);
  const std::string& workspace_id() const { return workspace_id_; }

  void SetWorkspaceName(const std::u16string& name);
  const std::u16string& workspace_name() const { return workspace_name_; }

  void SetAccentColor(const std::string& color_hex);
  const std::string& accent_color_hex() const { return accent_color_hex_; }

  void SetTabCount(int tab_count);
  int tab_count() const { return tab_count_; }

  void SetWindowCount(int window_count);
  int window_count() const { return window_count_; }

  void SetLastActiveTime(base::Time time);
  base::Time last_active_time() const { return last_active_time_; }

  void SetIsActive(bool is_active);
  bool is_active() const { return is_active_; }

  void SetIsSelected(bool is_selected);
  bool is_selected() const { return is_selected_; }

  // Marks this card as the "new workspace" action card (plus icon).
  void SetIsNewWorkspaceCard(bool is_new_card);
  bool is_new_workspace_card() const { return is_new_workspace_card_; }

  // Sets whether the drag handle is visible.
  void SetShowDragHandle(bool show);

  // Sets the card layout style (compact, full, grid).
  void SetCardStyle(AstraNtpWorkspaceCardStyle style);
  AstraNtpWorkspaceCardStyle card_style() const { return card_style_; }

  // -- Callbacks ---------------------------------------------------------

  void SetClickCallback(ClickCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void SetDragStartCallback(DragStartCallback callback);

  // -- views::View -------------------------------------------------------

  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;

 private:
  // Build the child views and layout.
  void BuildLayout();

  // Build the "new workspace" card variant layout.
  void BuildNewWorkspaceLayout();

  // Build the compact layout variant.
  void BuildCompactLayout();

  // Build the grid layout variant.
  void BuildGridLayout();

  // Updates visual state based on hover, active, and focus flags.
  void UpdateVisualState();

  // Updates the accent color of the header bar.
  void UpdateAccentColor();

  // Updates the visibility of the menu button and drag handle.
  void UpdateControlVisibility();

  // Updates the tab count badge display.
  void UpdateTabCountBadge();

  // Updates the last active time display.
  void UpdateLastActiveLabel();

  // Formats the last active time as a human-readable string.
  std::u16string FormatLastActiveTime() const;

  // Handler for the menu button press.
  void OnMenuButtonPressed();

  // Paints the accent color header bar.
  void PaintAccentHeader(gfx::Canvas* canvas);

  // Paints the focus ring.
  void PaintFocusRing(gfx::Canvas* canvas);

  // Paints the plus icon for the "new workspace" card.
  void PaintNewWorkspaceIcon(gfx::Canvas* canvas);

  // Paints the drag handle.
  void PaintDragHandle(gfx::Canvas* canvas);

  // Paints the selected state indicator.
  void PaintSelectionIndicator(gfx::Canvas* canvas);

  // Starts a drag operation.
  void StartDrag(const ui::LocatedEvent& event);

  // Data state.
  std::string workspace_id_;
  std::u16string workspace_name_;
  std::string accent_color_hex_;
  int tab_count_ = 0;
  int window_count_ = 0;
  base::Time last_active_time_;
  bool is_active_ = false;
  bool is_selected_ = false;
  bool is_hovered_ = false;
  bool is_pressed_ = false;
  bool is_focused_ = false;
  bool is_dragging_ = false;
  bool is_new_workspace_card_ = false;
  bool show_drag_handle_ = false;
  AstraNtpWorkspaceCardStyle card_style_ = AstraNtpWorkspaceCardStyle::kFull;

  // Point where drag started.
  gfx::Point drag_start_point_;

  // Parsed accent color as SkColor.
  SkColor accent_color_ = SK_ColorTRANSPARENT;

  ClickCallback click_callback_;
  MenuCallback menu_callback_;
  DragStartCallback drag_start_callback_;

  // Child views (regular card).
  raw_ptr<views::View> accent_header_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::View> tab_count_badge_ = nullptr;
  raw_ptr<views::Label> last_active_label_ = nullptr;
  raw_ptr<views::ImageButton> menu_button_ = nullptr;
  raw_ptr<views::View> drag_handle_ = nullptr;

  // Child views (new workspace card variant).
  raw_ptr<views::View> new_card_icon_ = nullptr;
  raw_ptr<views::Label> new_card_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_WORKSPACE_CARD_H_
