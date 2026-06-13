#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"

class Browser;
class Profile;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace astra {

class AstraBookmarkItemView;

// =========================================================================
// AstraBookmarkItemInfo — presentation data for a bookmark item
// =========================================================================
//
// A lightweight data struct representing a bookmark item projected from
// Chromium's BookmarkModel. This is presentation-facing — it carries only
// the fields needed for display and interaction.
//
// Truth source: bookmarks::BookmarkNode (components/bookmarks/browser/)
//
// TODO(astra): Wire to BookmarkModel for real bookmark data.
//   Chromium owner: BookmarkModel (components/bookmarks/browser/bookmark_model.h)
struct AstraBookmarkItemInfo {
  // Unique identifier for the bookmark.
  std::string id;

  // Display title of the bookmark or folder.
  std::u16string title;

  // URL of the bookmark (empty for folders).
  GURL url;

  // Whether this is a folder.
  bool is_folder = false;

  // ID of the parent folder (empty for top-level items).
  std::string parent_id;

  // Date the bookmark was added.
  base::Time date_added;

  // Date the bookmark was last modified.
  base::Time date_modified;

  // Whether this is the bookmark bar node / its children.
  bool is_bookmark_bar = false;

  // Whether this is under "Other Bookmarks".
  bool is_other = false;

  // Whether this is under "Mobile Bookmarks".
  bool is_mobile = false;

  // Number of child bookmarks (for folders).
  int child_count = 0;

  // Whether the bookmark has a custom favicon.
  bool has_favicon = false;
};

// =========================================================================
// AstraSidebarBookmarksDelegate — action callbacks for bookmarks section
// =========================================================================
//
// Delegate interface for user actions originating in the bookmarks section.
// Implemented by the parent sidebar view or controller to handle browser-
// level operations (opening URLs, modifying bookmarks, etc.).
//
// The bookmarks section view is pure presentation — it never mutates
// Chromium's BookmarkModel directly. All mutations go through this
// delegate or through BookmarkModel API calls on the service layer.
class AstraSidebarBookmarksDelegate {
 public:
  virtual ~AstraSidebarBookmarksDelegate() = default;

  // Called when a bookmark item is clicked (primary action).
  // |bookmark_id| identifies the bookmark that was clicked.
  virtual void OnBookmarkClicked(const std::string& bookmark_id) = 0;

  // Called when a bookmark item is middle-clicked (open in new tab).
  virtual void OnBookmarkMiddleClicked(const std::string& bookmark_id) = 0;

  // Called when a bookmark item is right-clicked (context menu).
  // |point| is in screen coordinates.
  virtual void OnBookmarkRightClicked(const std::string& bookmark_id,
                                      const gfx::Point& point) = 0;

  // Called when a folder is opened / expanded.
  virtual void OnFolderOpened(const std::string& folder_id) = 0;

  // Called when the user requests a new folder.
  virtual void OnNewFolderRequested() = 0;

  // Called when the user requests adding a new bookmark.
  virtual void OnAddBookmarkRequested() = 0;

  // Called when a bookmark is dragged.
  // |point| is the current drag position in screen coordinates.
  virtual void OnBookmarkDragged(const std::string& bookmark_id,
                                 const gfx::Point& point) = 0;

  // Called when a bookmark is dropped onto a target folder.
  // |target_folder_id| is the folder receiving the drop.
  // |dragged_bookmark_id| is the bookmark being moved.
  virtual void OnBookmarkDropped(const std::string& target_folder_id,
                                 const std::string& dragged_bookmark_id) = 0;
};

// =========================================================================
// AstraSidebarBookmarksView — bookmarks sidebar section
// =========================================================================
//
// A sidebar section that displays Chrome bookmarks as an expandable tree.
// Extends AstraSidebarSectionView for common section chrome (header with
// title, chevron, search, add button, etc.) and adds bookmark-specific
// presentation logic.
//
// This is a projection-only view — it reads bookmark state from Chromium's
// BookmarkModel and renders it into the sidebar. It never stores bookmark
// data locally; all mutations (add, remove, reorder, rename) are delegated
// to BookmarkModel methods.
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
class AstraSidebarBookmarksView
    : public AstraSidebarSectionView,
      public bookmarks::BookmarkModelObserver {
 public:
  explicit AstraSidebarBookmarksView(Browser* browser);
  ~AstraSidebarBookmarksView() override;

  AstraSidebarBookmarksView(const AstraSidebarBookmarksView&) = delete;
  AstraSidebarBookmarksView& operator=(const AstraSidebarBookmarksView&) =
      delete;

  // -- Delegate ------------------------------------------------------------

  // Set the action delegate.  Not owned.
  void set_delegate(AstraSidebarBookmarksDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarBookmarksDelegate* delegate() const { return delegate_; }

  // -- Bookmark data projection -------------------------------------------

  // Set the full list of visible bookmarks (current folder's children).
  // Replaces all existing bookmark items.
  void SetBookmarks(const std::vector<AstraBookmarkItemInfo>& bookmarks);

  // Get the total number of bookmarks in the current view.
  int GetBookmarkCount() const;

  // Get bookmark info at the given index.
  // Returns an empty/default struct if index is out of bounds.
  AstraBookmarkItemInfo GetBookmarkAt(int index) const;

  // Add a bookmark to the end of the current list.
  void AddBookmark(const AstraBookmarkItemInfo& bookmark);

  // Remove the bookmark at the given index.
  void RemoveBookmark(int index);

  // Update the bookmark at the given index with new info.
  void UpdateBookmark(int index, const AstraBookmarkItemInfo& bookmark);

  // -- Selection -----------------------------------------------------------

  // Set the selected bookmark by index. -1 clears selection.
  void SetSelectedBookmark(int index);
  int GetSelectedBookmarkIndex() const { return selected_index_; }
  void ClearSelection();

  // -- Folders first ordering ----------------------------------------------

  // Set whether folders are shown before URL bookmarks.
  void SetShowFoldersFirst(bool show_first);
  bool GetShowFoldersFirst() const { return show_folders_first_; }

  // -- Folder navigation ---------------------------------------------------

  // Set the currently displayed folder ID.
  // "root" or empty means the top-level bookmark bar.
  void SetCurrentFolder(const std::string& folder_id);
  const std::string& GetCurrentFolder() const { return current_folder_id_; }

  // Get the breadcrumb path from root to current folder.
  // Returns a list of folder IDs from root to current.
  std::vector<std::string> GetFolderPath() const;

  // Navigate into a subfolder.
  void NavigateToFolder(const std::string& folder_id);

  // Navigate up to the parent folder.
  void NavigateUp();

  // Whether it's possible to navigate up (not at root).
  bool CanNavigateUp() const;

  // -- Folder tree display -------------------------------------------------

  // Set whether to show the full folder tree or flat list.
  void SetShowFolderTree(bool show);
  bool GetShowFolderTree() const { return show_folder_tree_; }

  // -- Folder operations (presentation-only, delegates actual work) --------

  // Request creation of a new folder in the current directory.
  // Actual creation is delegated; the view updates via observer.
  void NewFolder(const std::u16string& name);

  // Request deletion of a folder.
  void DeleteFolder(const std::string& folder_id);

  // Request rename of a folder.
  void RenameFolder(const std::string& folder_id, const std::u16string& new_name);

  // -- Sorting and filtering -----------------------------------------------

  // Sort bookmarks by the given order.
  void SortBookmarks(AstraSidebarSortOrder order);

  // Filter bookmarks by the given filter.
  void FilterBookmarks(AstraSidebarFilter filter);

  // Search bookmarks by query string.
  void SearchBookmarks(const std::u16string& query);

  // Get the number of visible (non-filtered) bookmarks.
  int GetVisibleBookmarkCount() const;

  // -- Bookmark bar filter -------------------------------------------------

  // Set whether to show only bookmark bar items (hide other/mobile).
  void SetShowOnlyBookmarksBar(bool show);
  bool GetShowOnlyBookmarksBar() const { return show_only_bookmarks_bar_; }

  // -- Model integration ---------------------------------------------------

  // Rebuild the entire bookmark tree from the model.
  // Called on model load and after structural changes.
  // TODO(astra): Replace full rebuilds with incremental updates for better
  // performance with large bookmark trees.
  void RebuildFromModel();

  // -- bookmarks::BookmarkModelObserver ------------------------------------

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

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 protected:
  // AstraSidebarSectionView overrides.
  void OnAddButtonClicked() override;
  void OnSearchQueryChanged(const std::u16string& query) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildBookmarksLayout();

  // Start observing the BookmarkModel for the current profile.
  void StartObservingModel();

  // Stop observing the current BookmarkModel.
  void StopObservingModel();

  // Get HistoryService from the profile.
  // TODO(astra): Use BookmarkModelFactory::GetForBrowserContext.
  bookmarks::BookmarkModel* GetBookmarkModel();

  // Recursively add bookmark nodes to the tree view.
  void AddChildrenOf(const bookmarks::BookmarkNode* parent, int depth);

  // Create a bookmark item view for the given node.
  std::unique_ptr<AstraBookmarkItemView> CreateItemView(
      const bookmarks::BookmarkNode* node,
      int depth);

  // Handle a click on a bookmark item.
  void OnBookmarkItemClicked(const bookmarks::BookmarkNode* node,
                             bool open_in_new_tab);

  // Handle a folder expand/collapse toggle.
  void OnFolderToggled(const bookmarks::BookmarkNode* folder_node);

  // Open a bookmark URL in the browser.
  void OpenBookmark(const bookmarks::BookmarkNode* node,
                    bool open_in_new_tab);

  // Returns true if a folder should be shown as expanded.
  bool IsFolderExpanded(const bookmarks::BookmarkNode* folder_node) const;

  // Set a folder's expanded state.
  void SetFolderExpanded(const bookmarks::BookmarkNode* folder_node,
                         bool expanded);

  // Rebuild the bookmarks_ vector from current item views.
  void RebuildBookmarkInfoFromViews();

  // Apply current sort order to bookmark items.
  void ApplySortOrder();

  // Apply current filter to bookmark items.
  void ApplyFilter();

  // Compare function for sort order.
  static bool CompareBookmarks(const AstraBookmarkItemInfo& a,
                               const AstraBookmarkItemInfo& b,
                               AstraSidebarSortOrder order,
                               bool folders_first);

  // The browser associated with this sidebar section.
  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_ = nullptr;

  // Observation of Chromium's BookmarkModel.
  base::ScopedObservation<bookmarks::BookmarkModel,
                          bookmarks::BookmarkModelObserver>
      bookmark_model_observation_{this};

  // Action delegate for user-initiated operations.
  raw_ptr<AstraSidebarBookmarksDelegate> delegate_ = nullptr;

  // Cached bookmark data for the current view (projection from model).
  std::vector<AstraBookmarkItemInfo> bookmarks_;

  // Currently selected bookmark index. -1 means no selection.
  int selected_index_ = -1;

  // Whether folders are displayed before URLs.
  bool show_folders_first_ = true;

  // Currently displayed folder ID (for flat/folder navigation mode).
  std::string current_folder_id_;

  // Whether to show the hierarchical tree or flat list.
  bool show_folder_tree_ = true;

  // Whether to show only bookmark bar items.
  bool show_only_bookmarks_bar_ = false;

  // Map from BookmarkNode pointer to its item view.
  base::flat_map<const bookmarks::BookmarkNode*, AstraBookmarkItemView*>
      node_to_item_;

  // Set of collapsed folder node IDs.
  std::set<int64_t> collapsed_folders_;

  // Breadcrumb path (stack of folder IDs for navigation).
  std::vector<std::string> folder_path_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_BOOKMARKS_VIEW_H_
