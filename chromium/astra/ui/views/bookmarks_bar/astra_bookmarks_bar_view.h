#ifndef ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_VIEW_H_
#define ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class Label;
class MenuRunner;
class ScrollView;
}  // namespace views

namespace astra {

class AstraBookmarksBarModel;

// =========================================================================
// AstraBookmarksBarDelegate — delegate for bookmarks bar actions
// =========================================================================
//
// Delegate interface for actions that affect the browser or require
// navigation.  Implemented by browser-level code (e.g. AstraBrowserView).
class AstraBookmarksBarDelegate {
 public:
  virtual ~AstraBookmarksBarDelegate() = default;

  // Called to open a bookmark URL in the current tab.
  virtual void OpenBookmark(int64_t bookmark_id) = 0;

  // Called to open a bookmark URL in a new tab.
  virtual void OpenBookmarkInNewTab(int64_t bookmark_id) = 0;

  // Called to open all bookmarks in a folder as new tabs.
  virtual void OpenBookmarkFolderInNewTabs(int64_t folder_id) = 0;

  // Called when the "Other bookmarks" button is clicked.
  virtual void OnOtherBookmarksClicked(views::View* anchor) = 0;

  // Called when the "Add bookmark" button is clicked.
  virtual void OnAddBookmarkClicked() = 0;

  // Called when the bookmarks bar visibility should be toggled.
  virtual void OnToggleBookmarksBar() = 0;

  // Called to show the bookmark manager.
  virtual void OnShowBookmarkManager() = 0;
};

// =========================================================================
// AstraBookmarksBarView — the bookmarks bar view
// =========================================================================
//
// The bookmarks bar that appears below the omnibox (or above the web
// contents, depending on configuration).  Shows bookmark items and
// folders that live in the "Bookmarks Bar" root folder.
//
// Layout (left to right):
//   1. App launcher / apps button (optional)
//   2. Bookmark items and folders
//   3. Overflow button (shows when items don't fit)
//   4. "Other bookmarks" button
//   5. "Add bookmark" button
//
// The view observes AstraBookmarksBarModel for data changes.
//
// Chromium pattern: BookmarkBarView
//   (chrome/browser/ui/views/bookmarks/bookmark_bar_view.h)
//
// TODO(astra): Integrate with BrowserView via a patch.
//   Chromium owner: BrowserView (chrome/browser/ui/views/frame/browser_view.h)
//   Patch point: BrowserView::InitViews() — insert the bookmarks bar
//   between the toolbar and the contents area.
// =========================================================================

class AstraBookmarksBarView
    : public views::View,
      public AstraBookmarksBarObserver,
      public AstraBookmarksBarItemDelegate {
 public:
  explicit AstraBookmarksBarView(AstraBookmarksBarModel* model);
  ~AstraBookmarksBarView() override;

  AstraBookmarksBarView(const AstraBookmarksBarView&) = delete;
  AstraBookmarksBarView& operator=(const AstraBookmarksBarView&) = delete;

  // -- Model binding ------------------------------------------------------

  void SetModel(AstraBookmarksBarModel* model);
  AstraBookmarksBarModel* model() { return model_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraBookmarksBarDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- View state ---------------------------------------------------------

  // Show or hide the bookmarks bar (animated).
  void SetVisible(bool visible);

  // Whether the bar is currently visible.
  bool IsBarVisible() const { return bar_visible_; }

  // -- Item access --------------------------------------------------------

  // Get the item view at the given index.
  class AstraBookmarksBarItemView* GetItemAt(int index) const;

  // Get the number of items shown on the bar.
  int GetItemCount() const;

  // -- Display options ----------------------------------------------------

  void SetShowFavicons(bool show);
  void SetShowText(bool show);
  void SetMaxItemWidth(int width);
  void SetShowOtherBookmarksButton(bool show);

  // -- AstraBookmarksBarObserver ------------------------------------------

  void OnBookmarksBarLoaded(AstraBookmarksBarModel* model) override;
  void OnBookmarkAdded(AstraBookmarksBarModel* model,
                       int64_t bookmark_id) override;
  void OnBookmarkRemoved(AstraBookmarksBarModel* model,
                         int64_t bookmark_id) override;
  void OnBookmarkChanged(AstraBookmarksBarModel* model,
                         int64_t bookmark_id) override;
  void OnBookmarksReordered(AstraBookmarksBarModel* model) override;
  void OnBookmarksBarVisibilityChanged(AstraBookmarksBarModel* model,
                                        bool visible) override;
  void OnBookmarksBarModelShutdown(AstraBookmarksBarModel* model) override;

  // -- AstraBookmarksBarItemDelegate --------------------------------------

  void OnBookmarkClicked(int64_t bookmark_id,
                          bool is_middle_click,
                          bool is_shift_click) override;
  void OnFolderClicked(int64_t folder_id,
                        views::View* anchor_view) override;
  void OnBookmarkRightClicked(int64_t bookmark_id,
                               const gfx::Point& point) override;
  void OnBookmarkDragStarted(int64_t bookmark_id,
                              const ui::MouseEvent& event) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build all child views of the bookmarks bar.
  void BuildLayout();

  // Build the leading (left-side) buttons.
  void BuildLeadingButtons();

  // Build the scrollable items container.
  void BuildItemsContainer();

  // Build the trailing (right-side) buttons.
  void BuildTrailingButtons();

  // Rebuild all bookmark items from the model.
  void RebuildItems();

  // Update all items to reflect current model state.
  void UpdateItems();

  // Create a new item view from a model item.
  std::unique_ptr<AstraBookmarksBarItemView> CreateItemView(
      const struct AstraBookmarksBarItem& item);

  // Find an item view by bookmark ID.
  AstraBookmarksBarItemView* FindItemView(int64_t bookmark_id) const;

  // Update colors from the color provider.
  void UpdateColors();

  // Button handlers.
  void OnOtherBookmarksClicked();
  void OnAddBookmarkClicked();
  void OnOverflowButtonClicked();

  // Show the context menu for a bookmark item.
  void ShowContextMenu(int64_t bookmark_id, const gfx::Point& point);

  // Handle drag and drop reordering.
  void HandleDragReorder(int64_t dragged_id, int target_index);

  // The model providing bookmark data.  Not owned.
  raw_ptr<AstraBookmarksBarModel> model_ = nullptr;

  // Delegate for browser-level actions.  Not owned.
  raw_ptr<AstraBookmarksBarDelegate> delegate_ = nullptr;

  // Whether the bar is currently visible.
  bool bar_visible_ = true;

  // Display options.
  bool show_favicons_ = true;
  bool show_text_ = true;
  int max_item_width_ = 150;
  bool show_other_bookmarks_button_ = true;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> leading_container_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::View> trailing_container_ = nullptr;
  raw_ptr<views::ImageButton> overflow_button_ = nullptr;
  raw_ptr<views::ImageButton> other_bookmarks_button_ = nullptr;
  raw_ptr<views::ImageButton> add_bookmark_button_ = nullptr;

  // Item views in display order (owned by items_container_).
  std::vector<raw_ptr<AstraBookmarksBarItemView>> item_views_;

  // Context menu runner.
  std::unique_ptr<views::MenuRunner> context_menu_runner_;

  // Scoped observation of the model.
  base::ScopedObservation<AstraBookmarksBarModel,
                          AstraBookmarksBarObserver>
      model_observation_{this};

  // -- Constants ---------------------------------------------------------

  static constexpr int kBarHeight = 32;
  static constexpr int kLeadingPadding = 8;
  static constexpr int kTrailingPadding = 8;
  static constexpr int kItemSpacing = 2;
  static constexpr int kTrailingButtonSpacing = 4;
  static constexpr int kButtonSize = 28;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_VIEW_H_
