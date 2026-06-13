#ifndef ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_MODEL_H_
#define ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_MODEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace astra {

// =========================================================================
// AstraBookmarksBarItem — data for a single bookmarks bar item
// =========================================================================
//
// Projection of bookmarks::BookmarkNode for display in the bookmarks bar.
// Contains only the fields needed by the bookmarks bar UI.
//
// Truth source: Chromium's BookmarkModel.
//   (components/bookmarks/browser/bookmark_model.h)
struct AstraBookmarksBarItem {
  // Unique identifier (matches BookmarkNode id).
  int64_t id = -1;

  // Display title of the bookmark or folder.
  std::u16string title;

  // URL of the bookmark (empty for folders).
  GURL url;

  // Whether this item is a folder.
  bool is_folder = false;

  // Whether this item is the "Bookmarks Bar" root (not shown as a button).
  bool is_bookmarks_bar_folder = false;

  // Favicon (for URL bookmarks).
  gfx::Image favicon;

  // Index of the item within its parent folder.
  int index = 0;

  // Number of children (for folders).
  int child_count = 0;
};

// =========================================================================
// AstraBookmarksBarObserver — observer interface
// =========================================================================
//
// Observer for bookmarks bar model changes.  All methods have empty
// default implementations.
//
// Chromium pattern: BookmarkModelObserver
//   (components/bookmarks/browser/bookmark_model_observer.h)
class AstraBookmarksBarObserver : public base::CheckedObserver {
 public:
  // Called when the bookmarks bar model is loaded/changed.
  virtual void OnBookmarksBarLoaded(AstraBookmarksBarModel* model) {}

  // Called when a bookmark is added to the bar.
  virtual void OnBookmarkAdded(AstraBookmarksBarModel* model,
                               int64_t bookmark_id) {}

  // Called when a bookmark is removed from the bar.
  virtual void OnBookmarkRemoved(AstraBookmarksBarModel* model,
                                 int64_t bookmark_id) {}

  // Called when a bookmark's title or URL changes.
  virtual void OnBookmarkChanged(AstraBookmarksBarModel* model,
                                 int64_t bookmark_id) {}

  // Called when bookmarks are reordered.
  virtual void OnBookmarksReordered(AstraBookmarksBarModel* model) {}

  // Called when the bookmarks bar visibility preference changes.
  virtual void OnBookmarksBarVisibilityChanged(
      AstraBookmarksBarModel* model,
      bool visible) {}

  // Called when the model is shutting down.
  virtual void OnBookmarksBarModelShutdown(
      AstraBookmarksBarModel* model) {}

 protected:
  ~AstraBookmarksBarObserver() override = default;
};

// =========================================================================
// AstraBookmarksBarModel — model for the bookmarks bar
// =========================================================================
//
// Model that manages bookmark data for the bookmarks bar.
// Projects Chromium's BookmarkModel state into a format suitable for
// the bookmarks bar view.
//
// The model is a projection layer — it never owns bookmark data.
// All truth comes from Chromium's BookmarkModel.
//
// Responsibilities:
//   - Projects the "Bookmarks Bar" root folder from BookmarkModel
//   - Observes BookmarkModel for changes and notifies observers
//   - Manages bookmarks bar visibility preferences
//   - Provides sorted list of bar items
//   - Handles bookmark operations (add, remove, edit, reorder)
//
// Chromium owner: BookmarkModel
//   (components/bookmarks/browser/bookmark_model.h)
// Chromium observer: BookmarkModelObserver
//   (components/bookmarks/browser/bookmark_model_observer.h)
//
// TODO(astra): Wire to BookmarkModelObserver for real-time updates.
//   Patch point: None needed — public observer interface.
// =========================================================================

class AstraBookmarksBarModel {
 public:
  explicit AstraBookmarksBarModel(Profile* profile);
  ~AstraBookmarksBarModel();

  AstraBookmarksBarModel(const AstraBookmarksBarModel&) = delete;
  AstraBookmarksBarModel& operator=(const AstraBookmarksBarModel&) =
      delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(AstraBookmarksBarObserver* observer);
  void RemoveObserver(AstraBookmarksBarObserver* observer);

  // -- Item access --------------------------------------------------------

  // Get all items on the bookmarks bar (top-level children of the
  // "Bookmarks Bar" folder).
  const std::vector<AstraBookmarksBarItem>& GetItems() const { return items_; }

  // Get the number of items on the bar.
  size_t GetItemCount() const { return items_.size(); }

  // Get a specific item by ID.  Returns nullptr if not found.
  const AstraBookmarksBarItem* GetItem(int64_t id) const;

  // Get a specific item by index.  Returns nullptr if out of range.
  const AstraBookmarksBarItem* GetItemAtIndex(int index) const;

  // Get all children of a folder (for folder menus).
  std::vector<AstraBookmarksBarItem> GetFolderChildren(
      int64_t folder_id) const;

  // -- Visibility ---------------------------------------------------------

  // Whether the bookmarks bar is currently visible.
  bool IsVisible() const { return visible_; }

  // Set the bookmarks bar visibility.
  void SetVisible(bool visible);

  // Toggle the bookmarks bar visibility.
  void ToggleVisible();

  // Whether to show the bookmarks bar on the new tab page only.
  bool show_on_ntp_only() const { return show_on_ntp_only_; }
  void set_show_on_ntp_only(bool show);

  // -- Bookmark operations ------------------------------------------------
  //
  // These delegate to Chromium's BookmarkModel.

  // Add a new bookmark to the bookmarks bar.
  void AddBookmark(const std::u16string& title, const GURL& url);

  // Add a new folder to the bookmarks bar.
  void AddFolder(const std::u16string& title);

  // Remove a bookmark or folder from the bar.
  void RemoveBookmark(int64_t id);

  // Rename a bookmark or folder.
  void RenameBookmark(int64_t id, const std::u16string& new_title);

  // Change a bookmark's URL.
  void ChangeBookmarkURL(int64_t id, const GURL& new_url);

  // Move a bookmark to a new position within the bar.
  void MoveBookmark(int64_t id, int new_index);

  // Open a bookmark in the current tab.
  void OpenBookmark(int64_t id);

  // Open a bookmark in a new tab.
  void OpenBookmarkInNewTab(int64_t id);

  // Open all bookmarks in a folder as new tabs.
  void OpenFolderInNewTabs(int64_t folder_id);

  // -- Display settings ---------------------------------------------------

  // Whether to show favicons on bookmark items.
  bool show_favicons() const { return show_favicons_; }
  void set_show_favicons(bool show);

  // Whether to show text labels on bookmark items.
  bool show_text() const { return show_text_; }
  void set_show_text(bool show);

  // Maximum width of a bookmark item (in pixels, 0 = no limit).
  int max_item_width() const { return max_item_width_; }
  void set_max_item_width(int width);

  // Whether to show the "Applications" or "Other bookmarks" folder.
  bool show_other_bookmarks() const { return show_other_bookmarks_; }
  void set_show_other_bookmarks(bool show);

  // -- Refresh ------------------------------------------------------------

  // Refresh all bookmark data from the underlying BookmarkModel.
  void Refresh();

  // -- Utility ------------------------------------------------------------

  // Format a bookmark title for display (eliding if necessary).
  static std::u16 FormatTitle(const std::u16string& title,
                               int max_width_pixels,
                               const gfx::FontList& font_list);

  // -- Constants ----------------------------------------------------------

  static constexpr int kDefaultMaxItemWidth = 150;
  static constexpr int kMinItemWidth = 48;
  static constexpr int kMaxItemWidthLimit = 400;

 private:
  // Get the BookmarkModel for the associated profile.
  bookmarks::BookmarkModel* GetBookmarkModel() const;

  // Project a BookmarkNode into a bar item.
  static AstraBookmarksBarItem ProjectNode(
      const bookmarks::BookmarkNode* node);

  // Project all children of a node.
  std::vector<AstraBookmarksBarItem> ProjectChildren(
      const bookmarks::BookmarkNode* parent) const;

  // Load visibility preferences from PrefService.
  void LoadPrefs();

  // Save visibility preferences to PrefService.
  void SavePrefs();

  // Notify observers that the bookmarks bar changed.
  void NotifyLoaded();

  // Notify observers that a bookmark was added.
  void NotifyBookmarkAdded(int64_t id);

  // Notify observers that a bookmark was removed.
  void NotifyBookmarkRemoved(int64_t id);

  // Notify observers that a bookmark was changed.
  void NotifyBookmarkChanged(int64_t id);

  // Notify observers that bookmarks were reordered.
  void NotifyReordered();

  // Notify observers that visibility changed.
  void NotifyVisibilityChanged();

  // Find the index of an item by ID.  Returns -1 if not found.
  int FindItemIndex(int64_t id) const;

  // The profile associated with this bookmarks bar.  Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // The pref service for persistence.  Not owned.
  raw_ptr<PrefService> pref_service_ = nullptr;

  // Observers.
  base::ObserverList<AstraBookmarksBarObserver> observers_;

  // Cached bar items (projected from BookmarkModel).
  std::vector<AstraBookmarksBarItem> items_;

  // Visibility state.
  bool visible_ = true;
  bool show_on_ntp_only_ = false;

  // Display settings.
  bool show_favicons_ = true;
  bool show_text_ = true;
  int max_item_width_ = kDefaultMaxItemWidth;
  bool show_other_bookmarks_ = true;

  // Whether we're currently observing the BookmarkModel.
  // TODO(astra): Set to true once BookmarkModelObserver is wired up.
  bool observing_model_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_MODEL_H_
