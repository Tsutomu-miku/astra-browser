#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_BOOKMARK_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_BOOKMARK_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace views {
class ImageView;
}  // namespace views

namespace astra {

// Delegate interface for AstraBookmarkItemView actions.
// Implemented by the parent bookmarks section view.
class AstraBookmarkItemDelegate {
 public:
  virtual ~AstraBookmarkItemDelegate() = default;

  // Called when a bookmark item is clicked (opens the URL).
  // |node| is the bookmark node; |open_in_new_tab| indicates middle/ctrl click.
  virtual void OnBookmarkItemClicked(
      const bookmarks::BookmarkNode* node,
      bool open_in_new_tab) = 0;

  // Called when a folder's expand arrow is clicked.
  virtual void OnBookmarkFolderExpandedToggled(
      const bookmarks::BookmarkNode* folder_node) = 0;
};

// An individual bookmark item row in the sidebar bookmarks tree.
//
// Represents either a bookmark folder or a bookmark URL node.
// Folders have an expand/collapse chevron and show a folder icon.
// URL bookmarks show a page icon and navigate when clicked.
//
// This is a pure presentation view — it does not own bookmark state.
// Data is projected from Chromium's BookmarkModel by the parent
// AstraSidebarBookmarksView.
//
// TODO(astra): Replace icon placeholders with real favicons via
//   chrome/browser/ui/views/bookmarks/bookmark_utils.h and
//   components/bookmarks/browser/bookmark_model.h (GetFavicon).
// Chromium owner: BookmarkModel (components/bookmarks/browser/bookmark_model.h)
class AstraBookmarkItemView : public AstraSidebarItemView {
 public:
  // Type of bookmark node this item represents.
  enum class Type {
    kFolder,
    kUrl,
  };

  AstraBookmarkItemView(const bookmarks::BookmarkNode* node,
                        Type type,
                        int depth);
  AstraBookmarkItemView(const AstraBookmarkItemView&) = delete;
  AstraBookmarkItemView& operator=(const AstraBookmarkItemView&) = delete;
  ~AstraBookmarkItemView() override;

  // -- Bookmark info ------------------------------------------------------

  // Set all bookmark info at once.
  void SetBookmarkInfo(const GURL& url,
                       const std::u16string& title,
                       bool is_folder);

  // Get the URL of the bookmark (empty for folders).
  const GURL& GetUrl() const { return url_; }

  // Whether this item represents a folder.
  bool IsFolder() const { return type_ == Type::kFolder; }

  // -- Folder state -------------------------------------------------------

  // Set whether this folder is expanded/collapsed.
  void SetFolderExpanded(bool expanded);
  bool IsFolderExpanded() const { return is_folder_expanded_; }

  // -- Bookmark count (folders) ------------------------------------------

  // Set the number of bookmarks inside a folder (shown as badge).
  void SetBookmarkCount(int count);
  // Show or hide the bookmark count badge.
  void ShowBookmarkCount(bool show);

  // -- Bookmark bar styling -----------------------------------------------

  // Set whether this is the special "bookmark bar" item (has special styling).
  void SetIsBookmarkBar(bool is_bar);
  bool IsBookmarkBar() const { return is_bookmark_bar_; }

  // -- Delegates ----------------------------------------------------------

  void set_delegate(AstraBookmarkItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- Node access --------------------------------------------------------

  const bookmarks::BookmarkNode* bookmark_node() const { return node_; }
  Type type() const { return type_; }
  int depth() const { return depth_; }

  // -- AstraSidebarItemView overrides ------------------------------------

  void SetTitle(const std::u16string& title) override;
  void SetActive(bool active) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;

  // Returns true if |point| is over the expand chevron (for folders).
  bool IsPointInExpandChevron(const gfx::Point& point) const;

  // Update the expand chevron appearance based on is_folder_expanded_.
  void UpdateExpandChevron();

  // Update the icon based on folder/URL type.
  void UpdateIcon();

  // Apply depth-based indentation to the item.
  void ApplyDepthIndentation();

  // The bookmark node this item represents. Not owned.
  raw_ptr<const bookmarks::BookmarkNode> node_ = nullptr;

  // Type of bookmark (folder or URL).
  Type type_;

  // Indentation depth (0 = top level).
  int depth_;

  // URL of the bookmark (only valid for URL items).
  GURL url_;

  // Whether folder is expanded.
  bool is_folder_expanded_ = true;

  // Whether this is the special bookmark bar item.
  bool is_bookmark_bar_ = false;

  // Bookmark count (for folders).
  int bookmark_count_ = 0;
  bool show_bookmark_count_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraBookmarkItemDelegate> delegate_ = nullptr;

  // Hover state tracking.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_BOOKMARK_ITEM_VIEW_H_
