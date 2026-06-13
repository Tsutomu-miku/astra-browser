#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

// A tab item displayed within a tab group in the sidebar.
//
// Similar to AstraSidebarItemView but with group-specific styling:
//   - Indented to show nested hierarchy under the group header
//   - Shows tab title and a favicon placeholder
//   - Has a close button that appears on hover
//   - Clicking the tab activates it in the tab strip
//
// This is a pure presentation view. Tab data comes from Chromium's
// TabStripModel and WebContents. The view never stores tab state.
//
// Chromium owner: Tab (chrome/browser/ui/tabs/tab.h),
//   TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
class AstraTabGroupTabItemView : public views::LabelButton {
 public:
  // Callback invoked when the close button is clicked.
  using CloseCallback = base::RepeatingClosure;

  AstraTabGroupTabItemView(const std::u16string& title,
                           int tab_index,
                           PressedCallback activate_callback,
                           CloseCallback close_callback);
  AstraTabGroupTabItemView(const AstraTabGroupTabItemView&) = delete;
  AstraTabGroupTabItemView& operator=(const AstraTabGroupTabItemView&) = delete;
  ~AstraTabGroupTabItemView() override;

  // Update the displayed tab title.
  void SetTitle(const std::u16string& title);

  // Set whether this tab is the active tab (highlighted).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // Get the TabStripModel index of the tab this item represents.
  int tab_index() const { return tab_index_; }
  void set_tab_index(int index) { tab_index_ = index; }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Update close button visibility based on hover state.
  void UpdateCloseButtonVisibility();

  // TabStripModel index of the tab this item represents.
  // This is a projection-only cache — the truth source is TabStripModel.
  int tab_index_ = -1;

  // Whether this is the active tab.
  bool is_active_ = false;

  // Callbacks for user actions.
  CloseCallback close_callback_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> favicon_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_
