#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Data structure describing a tab group for sidebar presentation.
//
// This is a projection struct — the truth source is Chromium's TabGroup model.
// The sidebar section reads group data from TabStripModel and projects it
// into these structs for display. Sidebar-only fields (is_expanded) are
// presentation state stored on the view layer.
//
// Chromium owner: TabGroup (chrome/browser/ui/tabs/tab_group.h)
// Chromium visuals: TabGroupVisualData (chrome/browser/ui/tabs/tab_group.h)
struct AstraTabGroupInfo {
  // Stable identifier for the group. Maps to tab_groups::TabGroupId.
  std::string group_id;

  // Display name of the group.
  std::u16string name;

  // Group color (resolved SkColor for rendering).
  SkColor color = SK_ColorGRAY;

  // Chromium color ID (tab_groups::TabGroupColorId as int).
  int color_id = 0;

  // Number of tabs in the group.
  int tab_count = 0;

  // Whether the group is expanded in the sidebar (tabs visible).
  // This is sidebar presentation state, independent of tab strip collapse.
  bool is_expanded = true;

  // Whether the group is collapsed in the main tab strip.
  // Mirrors TabGroup::IsCollapsed().
  bool is_collapsed_in_tabstrip = false;

  // Display order index (0-based, lower = higher in the list).
  int order_index = 0;

  // Last time a tab in the group was accessed.
  base::Time last_accessed;

  // Time the group was created.
  base::Time created_time;

  // Optional note attached to the group (Astra-only metadata).
  std::u16string note;

  // Whether the group is pinned (always visible, cannot be reordered).
  bool is_pinned = false;
};

// Sorting criteria for tab groups in the sidebar.
enum class AstraTabGroupSortBy {
  kManual,        // User-defined order (default).
  kName,          // Alphabetical by group name.
  kColor,         // By group color.
  kTabCount,      // By number of tabs (descending).
  kLastAccessed,  // By most recent access time (descending).
};

// Header row for a tab group in the sidebar.
//
// Shows a colored dot, expand/collapse chevron, group name, tab count badge,
// optional pin indicator, and optional menu button.
//
// Clicking the header toggles expansion. Clicking the menu button shows a
// context menu. The header is a pure presentation view — it does not own
// group state; data is projected via SetGroupInfo() and individual setters.
//
// Chromium owner: TabGroupHeader (chrome/browser/ui/views/tabs/tab_group_header.h)
// Chromium model: TabGroup (chrome/browser/ui/tabs/tab_group.h)
//
// TODO(astra): Add right-click context menu with rename, change color,
//   close group, etc. Chromium's tab group header already has this; we
//   should reuse the menu model from TabGroupController if possible rather
//   than reimplementing.
//   Chromium owner: TabGroupContextMenu (chrome/browser/ui/tabs/tab_group_context_menu.h)
class AstraTabGroupHeaderView : public views::Button {
 public:
  // Callback invoked when the header is clicked (expand/collapse toggle).
  using ToggleCallback = base::RepeatingClosure;

  // Callback invoked when the "new tab in group" button is clicked.
  using NewTabCallback = base::RepeatingClosure;

  // Callback invoked when the menu button is clicked.
  using MenuCallback = base::RepeatingClosure;

  AstraTabGroupHeaderView(const std::u16string& title,
                          SkColor color,
                          ToggleCallback toggle_callback,
                          NewTabCallback new_tab_callback);
  AstraTabGroupHeaderView(const AstraTabGroupHeaderView&) = delete;
  AstraTabGroupHeaderView& operator=(const AstraTabGroupHeaderView&) = delete;
  ~AstraTabGroupHeaderView() override;

  // -- Group info ----------------------------------------------------------

  // Set all group info at once. Convenience for bulk updates.
  void SetGroupInfo(const AstraTabGroupInfo& info);

  // Get the group ID.
  const std::string& GetGroupId() const { return group_id_; }

  // -- Name ----------------------------------------------------------------

  // Update the displayed group title.
  void SetName(const std::u16string& name);
  std::u16string GetName() const;

  // -- Color ---------------------------------------------------------------

  // Update the group color (colored dot on the left).
  void SetColor(SkColor color);
  SkColor GetColor() const { return group_color_; }

  // Set the Chromium color ID for the group.
  void SetColorId(int color_id);
  int GetColorId() const { return color_id_; }

  // -- Tab count -----------------------------------------------------------

  // Update the displayed tab count (shown in parentheses next to the title).
  void SetTabCount(int count);
  int GetTabCount() const { return tab_count_; }

  // -- Expansion -----------------------------------------------------------

  // Set whether the group is expanded (tabs visible below the header).
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return expanded_; }

  // -- Selection -----------------------------------------------------------

  // Set whether this group header is selected (keyboard navigation state).
  void SetSelected(bool selected);
  bool IsSelected() const { return is_selected_; }

  // -- Pinned state --------------------------------------------------------

  // Set whether this group is pinned.
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }

  // -- Chevron visibility --------------------------------------------------

  // Set whether the expand/collapse chevron is shown.
  void SetShowChevron(bool show);
  bool GetShowChevron() const { return show_chevron_; }

  // -- Tab count visibility ------------------------------------------------

  // Set whether the tab count badge is shown.
  void SetShowTabCount(bool show);
  bool GetShowTabCount() const { return show_tab_count_; }

  // -- Color dot visibility ------------------------------------------------

  // Set whether the colored dot is shown.
  void SetShowColorDot(bool show);
  bool GetShowColorDot() const { return show_color_dot_; }

  // -- Menu button visibility ----------------------------------------------

  // Set whether the menu button is shown.
  void SetShowMenuButton(bool show);
  bool GetShowMenuButton() const { return show_menu_button_; }

  // -- Compact mode --------------------------------------------------------

  // Set whether compact mode is enabled (reduced height/padding).
  void SetCompact(bool compact);
  bool IsCompact() const { return is_compact_; }

  // -- Tabstrip collapsed state --------------------------------------------

  // Set whether the group is collapsed in the main tab strip.
  void SetCollapsedInTabstrip(bool collapsed);
  bool IsCollapsedInTabstrip() const { return is_collapsed_in_tabstrip_; }

  // -- Drag hover state ----------------------------------------------------

  // Set whether a drag operation is hovering over this header.
  void SetIsDragHovered(bool hovered);
  bool IsDragHovered() const { return is_drag_hovered_; }

  // -- Menu callback -------------------------------------------------------

  void set_menu_callback(MenuCallback callback) {
    menu_callback_ = std::move(callback);
  }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Compute the background color based on hover/active/selected state.
  SkColor GetBackgroundColor() const;

  // Update visibility of child views based on show flags.
  void UpdateChildVisibility();

  // Update the color dot's visual appearance.
  void UpdateColorDot();

  // Update the chevron's visual appearance (rotation for expanded state).
  void UpdateChevron();

  // The stable identifier for this group.
  std::string group_id_;

  // The color of this tab group.
  SkColor group_color_ = SK_ColorGRAY;

  // The Chromium color ID (tab_groups::TabGroupColorId as int).
  int color_id_ = 0;

  // Number of tabs in the group.
  int tab_count_ = 0;

  // Whether the group is expanded in the sidebar (tabs visible below).
  // This is sidebar presentation state, independent of the tab strip's
  // own collapsed state.
  bool expanded_ = true;

  // Whether this header is selected (keyboard navigation highlight).
  bool is_selected_ = false;

  // Whether this group is pinned.
  bool is_pinned_ = false;

  // Whether compact mode is enabled.
  bool is_compact_ = false;

  // Whether the group is collapsed in the main tab strip.
  bool is_collapsed_in_tabstrip_ = false;

  // Whether a drag is hovering over this header.
  bool is_drag_hovered_ = false;

  // Visibility flags for child elements.
  bool show_chevron_ = true;
  bool show_tab_count_ = true;
  bool show_color_dot_ = true;
  bool show_menu_button_ = false;

  // Callbacks for user actions. Not owned.
  ToggleCallback toggle_callback_;
  NewTabCallback new_tab_callback_;
  MenuCallback menu_callback_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> color_dot_ = nullptr;
  raw_ptr<views::ImageView> chevron_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ImageView> new_tab_button_ = nullptr;
  raw_ptr<views::ImageButton> menu_button_ = nullptr;
  raw_ptr<views::ImageView> pin_indicator_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_
