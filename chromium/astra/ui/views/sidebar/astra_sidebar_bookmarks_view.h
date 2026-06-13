#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "ui/views/view.h"

class Browser;
class Profile;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace views {
class ImageButton;
class Label;
class ScrollView;
}  // namespace views

namespace astra {

class AstraBookmarkItemView;

// A sidebar section that displays Chrome bookmarks as an expandable tree.
//
// This is a projection-only view — it reads bookmark state from Chromium's
// BookmarkModel and renders it into the sidebar. It never stores bookmark
// data locally; all mutations (add, remove, reorder, rename) are delegated
// to BookmarkModel methods.
//
// The section has a collapsible header ("Bookmarks") and a scrollable tree
// of bookmark items. Folders can be expanded/collapsed to show/hide their
// children.
//
// Implements bookmarks::BookmarkModelObserver to receive live updates from
// Chromium's bookmark system. When the model changes, the tree is rebuilt
// to reflect the new state.
//
// Chromium subsystems reused:
//   - BookmarkModel (components/bookmarks/browser/bookmark_model.h)
//   - BookmarkModelObserver (components/bookmarks/browser/bookmark_model_observer.h)
//   - BookmarkNode (components/bookmarks/browser/bookmark_node.h)
//
// Chromium owner/reference: Bookmarks Side Panel
//   (chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_view.h)
//
// TODO(astra): Implement incremental UI updates (add/remove/move individual
// items) instead of full rebuilds on every model change. Full rebuilds are
// correct but O(n) per change; incremental updates via the observer methods
// would be O(1) for most operations.
// Chromium owner: bookmarks::BookmarkModelObserver
class AstraSidebarBookmarksView : public views::View,
                                  public bookmarks::BookmarkModelObserver {
 public:
  explicit AstraSidebarBookmarksView(Browser* browser);
  AstraSidebarBookmarksView(const AstraSidebarBookmarksView&) = delete;
  AstraSidebarBookmarksView& operator=(const AstraSidebarBookmarksView&) =
      delete;
  ~AstraSidebarBookmarksView() override;

  // Set whether the entire bookmarks section is collapsed (only the header
  // shows). The section starts expanded by default.
  void SetSectionCollapsed(bool collapsed);
  bool IsSectionCollapsed() const { return is_collapsed_; }

  // Toggle section collapsed/expanded.
  void ToggleSectionCollapsed();

  // Rebuild the entire bookmark tree from the model.
  // Called on model load and after structural changes.
  // TODO(astra): Replace full rebuilds with incremental updates for better
  // performance with large bookmark trees.
  void RebuildFromModel();

  // -- bookmarks::BookmarkModelObserver ----------------------------------

  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         size_t index) override;
  void BookmarkNodeRemoved(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* parent,
      size_t index,
      const bookmarks::BookmarkNode* node,
      const std::set<GURL>& removed_urls) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         size_t old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         size_t new_index) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;
  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override;
  void BookmarkModelBeingDeleted(
      bookmarks::BookmarkModel* model) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views (header + scrollable tree). Called once from
  // constructor.
  void BuildLayout();

  // Start observing the BookmarkModel for the current profile.
  // Safe to call multiple times — resets observation.
  void StartObservingModel();

  // Stop observing the current BookmarkModel. No-op if not observing.
  void StopObservingModel();

  // Recursively add bookmark nodes to the tree view.
  // |parent| is the parent bookmark node whose children to add.
  // |depth| is the current indentation level.
  void AddChildrenOf(const bookmarks::BookmarkNode* parent, int depth);

  // Create a bookmark item view for the given node.
  std::unique_ptr<AstraBookmarkItemView> CreateItemView(
      const bookmarks::BookmarkNode* node,
      int depth);

  // Handle a click on a bookmark item.
  // |node| is the bookmark node that was clicked.
  // |open_in_new_tab| is true for middle-click or Ctrl+click.
  void OnBookmarkClicked(const bookmarks::BookmarkNode* node,
                         bool open_in_new_tab);

  // Handle a folder expand/collapse toggle.
  void OnFolderToggled(const bookmarks::BookmarkNode* folder_node);

  // Open a bookmark URL in the browser.
  // |node| must be a URL bookmark node.
  // |open_in_new_tab| controls whether a new tab is created.
  void OpenBookmark(const bookmarks::BookmarkNode* node,
                    bool open_in_new_tab);

  // Returns true if a folder should be shown as expanded.
  // TODO(astra): Persist expanded state per folder. Currently all folders
  // default to expanded.
  bool IsFolderExpanded(const bookmarks::BookmarkNode* folder_node) const;

  // Set a folder's expanded state.
  void SetFolderExpanded(const bookmarks::BookmarkNode* folder_node,
                         bool expanded);

  raw_ptr<Browser> browser_;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_ = nullptr;

  // Observation of Chromium's BookmarkModel for reactive tree updates.
  base::ScopedObservation<bookmarks::BookmarkModel,
                          bookmarks::BookmarkModelObserver>
      bookmark_model_observation_{this};

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_view_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::ImageButton> collapse_button_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tree_container_ = nullptr;

  // Whether the section is collapsed (only header visible).
  bool is_collapsed_ = false;

  // Map from BookmarkNode pointer to its item view. Used for incremental
  // updates and folder expansion state lookup.
  // TODO(astra): Use this map for incremental updates once we switch from
  // full rebuilds to per-change updates.
  base::flat_map<const bookmarks::BookmarkNode*, AstraBookmarkItemView*>
      node_to_item_;

  // Set of folder node ids that are currently collapsed. Folders are
  // expanded by default; only collapsed folders are stored here.
  // TODO(astra): Persist this state via PrefService.
  std::set<int64_t> collapsed_folders_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_
