#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_CARD_VIEW_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_CARD_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace astra {

// A single workspace card in the workspace overview.
//
// This is a presentation-only view.  It displays workspace metadata (name,
// accent color, tab count, created time, thumbnail placeholders) and invokes
// callbacks when clicked, double-clicked, or when the menu button is pressed.
// It never reads or writes workspace state directly — all data is pushed in
// by the parent view / controller.
//
// Two display modes are supported:
//   - Card mode: full card with thumbnail grid and info area (grid view).
//   - List mode: compact row with icon, name, and counts (list view).
//
// Visual layout (card mode):
//   +-------------------------------------+
//   |  accent color bar (top)             |
//   +-------------------------------------+
//   |  tab thumbnail grid (2x2)           |
//   |  (placeholder rectangles for now)   |
//   |                                     |
//   +-------------------------------------+
//   |  Workspace Name              [⋮]   |
//   |  N tabs · created X ago             |
//   +-------------------------------------+
//
// States:
//   - Default: normal card background, standard border.
//   - Hover: slightly lighter background, drop shadow.
//   - Selected: focus ring + subtle background tint (keyboard navigation).
//   - Active (current workspace): thicker accent border.
//   - Hibernated: desaturated, lower opacity, hibernation badge.
//
// TODO(astra): Replace placeholder thumbnail rectangles with real tab
// thumbnails from Chromium's thumbnail subsystem.
// Chromium subsystem: chrome/browser/ui/thumbnails/ or TabThumbnailTracker.
// Alternative: content::WebContents::GetCaptureUpdater().
// Patch point needed: thumbnail capture for Astra workspace overview.
class AstraWorkspaceCardView : public views::View {
 public:
  // Display mode for the card.
  enum class DisplayMode {
    kCard,  // Full card with thumbnails (grid view)
    kList,  // Compact list row (list view)
  };

  // Callback type for when the card is clicked (single-click, activates).
  using ClickCallback = base::RepeatingClosure;

  // Callback type for when the card is double-clicked (rename).
  using RenameCallback = base::RepeatingClosure;

  // Callback type for when the menu button (⋮) is pressed.
  using MenuActionCallback = base::RepeatingCallback<void(const gfx::Point&)>;

  // Callback type for when the delete action is triggered (keyboard or menu).
  using DeleteCallback = base::RepeatingClosure;

  AstraWorkspaceCardView();
  AstraWorkspaceCardView(const AstraWorkspaceCardView&) = delete;
  AstraWorkspaceCardView& operator=(const AstraWorkspaceCardView&) = delete;
  ~AstraWorkspaceCardView() override;

  // -- Data setters (called by parent / controller) -----------------------

  // Sets all workspace info at once from an AstraWorkspace struct.
  void SetWorkspaceInfo(const AstraWorkspace& workspace);

  // Returns the workspace info struct built from current card state.
  AstraWorkspace GetWorkspaceInfo() const;

  void SetWorkspaceName(const std::u16string& name);
  void SetAccentColor(const std::string& color_hex);

  // Sets the accent color as a SkColor value.
  void SetAccentColor(SkColor color);

  // Returns the accent color as a SkColor value.
  SkColor GetAccentColor() const;

  void SetTabCount(int tab_count);

  // Sets the number of windows in the workspace.  Shown in the card's
  // info area alongside the tab count (e.g. "2 windows · 8 tabs").
  void SetWindowCount(int window_count);

  // Sets the workspace creation time, shown as a relative timestamp
  // (e.g. "3 days ago") in the card's secondary info line.
  void SetCreatedTime(base::Time created_time);

  // Sets the last used time, shown alongside creation time when
  // show_statistics is enabled.
  void SetLastUsedTime(base::Time last_used_time);

  // Sets the number of thumbnail placeholders to show.  Currently up to 4
  // are shown in a 2x2 grid.
  void SetThumbnailCount(int count);

  // Sets the icon identifier for the workspace.
  void SetIcon(const std::optional<std::string>& icon);

  // Marks this card as representing the currently active workspace, which
  // affects the border / highlight styling.
  void SetIsActive(bool is_active);

  // Marks this card as selected (for keyboard navigation).  Shows a focus
  // ring and subtle background tint.
  void SetIsSelected(bool is_selected);

  // Marks this card as hovered (mouse is over the card).
  void SetHovered(bool is_hovered);

  // Returns whether the card is currently hovered.
  bool IsHovered() const { return is_hovered_; }

  // Marks this card's workspace as hibernated.  Hibernated cards have
  // reduced opacity and show a hibernation indicator badge.
  void SetIsHibernated(bool is_hibernated);

  // Sets the display mode (card or list).
  void SetDisplayMode(DisplayMode mode);

  // Sets whether to show statistics (last used time, etc.) on the card.
  void SetShowStatistics(bool show);

  // Sets the size variant (affects preferred size in card mode).
  void SetSizeVariant(AstraWorkspaceOverviewCardSize size);

  // -- Tab / window count visibility ---------------------------------------

  // Shows or hides the tab count on the card.
  void ShowTabCount(bool show);

  // Returns whether the tab count is visible.
  bool IsTabCountVisible() const;

  // Shows or hides the window count on the card.
  void ShowWindowCount(bool show);

  // Returns whether the window count is visible.
  bool IsWindowCountVisible() const;

  // -- Menu button visibility ---------------------------------------------

  // Shows or hides the menu button (⋮) on the card.
  void ShowMenuButton(bool show);

  // Returns whether the menu button is visible.
  bool IsMenuButtonVisible() const;

  // -- Pinned state --------------------------------------------------------

  // Sets whether this workspace is pinned.
  // Pinned workspaces stay at the beginning of the list.
  void SetPinned(bool pinned);

  // Returns whether this workspace is pinned.
  bool IsPinned() const { return is_pinned_; }

  // -- Callbacks ----------------------------------------------------------

  void SetClickCallback(ClickCallback callback);
  void SetRenameCallback(RenameCallback callback);
  void SetMenuActionCallback(MenuActionCallback callback);
  void SetDeleteCallback(DeleteCallback callback);

  // -- Query helpers ------------------------------------------------------

  const std::u16string& workspace_name() const { return workspace_name_; }
  bool is_active() const { return is_active_; }
  bool is_selected() const { return is_selected_; }
  bool is_hibernated() const { return is_hibernated_; }
  DisplayMode display_mode() const { return display_mode_; }

  // -- views::View --------------------------------------------------------

  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnMouseDoubleClicked(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void OnThemeChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaintBorder(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  // Build the child views and layout.  Called once from constructor.
  void BuildLayout();

  // Updates the accent color bar's background color.
  void UpdateAccentColorBar();

  // Updates the card's visual state based on hover, active, and selected.
  // Triggers a repaint and updates the shadow layer.
  void UpdateVisualState();

  // Updates the created time label with a human-readable relative timestamp.
  void UpdateCreatedTimeLabel();

  // Updates the last used time label.
  void UpdateLastUsedTimeLabel();

  // Updates shadow based on hover / selected state.
  void UpdateShadow();

  // Updates the visibility of statistics-related elements.
  void UpdateStatisticsVisibility();

  // Updates the hibernation badge visibility and styling.
  void UpdateHibernationBadge();

  // Gets the card dimensions based on current size variant.
  int GetCardWidth() const;
  int GetCardHeight() const;
  int GetListRowHeight() const;

  // Data state (pushed in by parent).
  std::u16string workspace_name_;
  std::string accent_color_hex_;
  SkColor accent_color_ = gfx::kPlaceholderColor;
  int tab_count_ = 0;
  int window_count_ = 1;
  base::Time created_time_;
  base::Time last_used_time_;
  int thumbnail_count_ = 0;
  std::optional<std::string> icon_;
  bool is_active_ = false;
  bool is_selected_ = false;
  bool is_hovered_ = false;
  bool is_pressed_ = false;
  bool is_hibernated_ = false;
  bool is_pinned_ = false;
  DisplayMode display_mode_ = DisplayMode::kCard;
  bool show_statistics_ = true;
  bool show_tab_count_ = true;
  bool show_window_count_ = true;
  bool show_menu_button_ = true;
  AstraWorkspaceOverviewCardSize size_variant_ =
      AstraWorkspaceOverviewCardSize::kMedium;

  // Callbacks.
  ClickCallback click_callback_;
  RenameCallback rename_callback_;
  MenuActionCallback menu_action_callback_;
  DeleteCallback delete_callback_;

  // Child views (owned by view hierarchy).
  raw_ptr<views::View> accent_bar_ = nullptr;
  raw_ptr<views::View> thumbnail_grid_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> window_count_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::Label> created_time_label_ = nullptr;
  raw_ptr<views::Label> last_used_time_label_ = nullptr;
  raw_ptr<views::Label> hibernation_badge_ = nullptr;
  raw_ptr<views::ImageButton> menu_button_ = nullptr;

  // Thumbnail placeholder views (owned by thumbnail_grid_).
  std::vector<raw_ptr<views::View>> thumbnail_placeholders_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_CARD_VIEW_H_
