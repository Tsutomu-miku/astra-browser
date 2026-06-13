#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Header row for a tab group in the sidebar.
//
// Shows a colored dot, group name, tab count, and an expand/collapse chevron.
// Clicking the header toggles whether the group's tabs are visible in the
// sidebar (sidebar-only collapse state, independent of tab strip collapse).
//
// This is a pure presentation view. Group data (name, color, tab count) is
// projected from Chromium's TabGroup by the parent sidebar section. The view
// does not own or cache group state beyond what is needed for rendering.
//
// Chromium owner: TabGroupHeader (chrome/browser/ui/views/tabs/tab_group_header.h)
// Chromium model: TabGroup (chrome/browser/ui/tabs/tab_group.h),
//                 TabGroupColorId (chrome/browser/ui/tabs/tab_group_color.h)
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

  AstraTabGroupHeaderView(const std::u16string& title,
                          tab_groups::TabGroupColorId color,
                          ToggleCallback toggle_callback,
                          NewTabCallback new_tab_callback);
  AstraTabGroupHeaderView(const AstraTabGroupHeaderView&) = delete;
  AstraTabGroupHeaderView& operator=(const AstraTabGroupHeaderView&) = delete;
  ~AstraTabGroupHeaderView() override;

  // Update the displayed group title.
  void SetTitle(const std::u16string& title);

  // Update the displayed tab count (shown in parentheses next to the title).
  void SetTabCount(int count);

  // Update the group color (colored dot on the left).
  void SetGroupColor(tab_groups::TabGroupColorId color);

  // Set whether the group is expanded (tabs visible below the header).
  void SetExpanded(bool expanded);
  bool expanded() const { return expanded_; }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Compute the background color based on hover/active state.
  SkColor GetBackgroundColor() const;

  // The color of this tab group (from Chromium's TabGroupColorId enum).
  tab_groups::TabGroupColorId group_color_;

  // Whether the group is expanded in the sidebar (tabs visible below).
  // This is sidebar presentation state, independent of the tab strip's
  // own collapsed state.
  bool expanded_ = true;

  // Callbacks for user actions. Not owned.
  ToggleCallback toggle_callback_;
  NewTabCallback new_tab_callback_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> color_dot_ = nullptr;
  raw_ptr<views::ImageView> chevron_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ImageView> new_tab_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_HEADER_VIEW_H_
