// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_MODEL_H_
#define ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_MODEL_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

using AstraBookmarkId = int64_t;

// Sort type for folder contents.
enum class AstraBookmarkSortType {
  kName,
  kDateAddedNewest,
  kDateAddedOldest,
  kUrl,
  kMostVisited,
};

// A single bookmark entry.
struct AstraBookmarkEntry {
  AstraBookmarkId id = 0;
  std::u16string title;
  std::string url;
  AstraBookmarkId parent_id = 0;
  bool is_folder = false;
  gfx::ImageSkia favicon;
  std::string favicon_url;
  base::Time date_added;
  bool is_bookmarked_bar = false;
  bool is_other = false;
  bool is_mobile = false;
  std::string workspace;
  std::string category;
  int visit_count = 0;
};

// A bookmark folder containing children and subfolders.
struct AstraBookmarkFolder {
  AstraBookmarkId id = 0;
  std::u16string title;
  AstraBookmarkId parent_id = 0;
  std::vector<AstraBookmarkEntry> children;
  std::vector<AstraBookmarkFolder> subfolders;
  int total_bookmarks = 0;
};

// Observer for AstraBookmarksManagerModel.
class AstraBookmarksManagerObserver : public base::CheckedObserver {
 public:
  // Called when the entire bookmarks model changes.
  virtual void OnBookmarksModelChanged() {}

  // Called when a bookmark is added.
  virtual void OnBookmarkAdded(AstraBookmarkId id) {}

  // Called when a bookmark is removed.
  virtual void OnBookmarkRemoved(AstraBookmarkId id) {}

  // Called when a bookmark's properties change.
  virtual void OnBookmarkChanged(AstraBookmarkId id) {}

  // Called when a folder is expanded or collapsed.
  virtual void OnFolderExpanded(AstraBookmarkId folder_id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(const std::u16string& query) {}

  // Called when the model is about to be destroyed.
  virtual void OnBookmarksManagerModelShutdown() {}

 protected:
  ~AstraBookmarksManagerObserver() override = default;
};

// Model for the bookmarks manager page.
//
// Owns bookmark and folder data, with search, sorting, and category
// filtering.  Bookmark data comes from Chromium's BookmarkModel — this
// model projects and augments it with Astra-specific workspace and
// category metadata.
//
// Chromium owner: BookmarkModel / BookmarkNode
//   (components/bookmarks/browser/bookmark_model.h)
//
// TODO(astra): Wire up to Chromium's BookmarkModel via a KeyedService
// wrapper.  Patch point: chrome/browser/bookmarks/bookmark_model_factory.cc
// or chrome/browser/ui/webui/bookmarks/bookmarks_ui.cc.
class AstraBookmarksManagerModel {
 public:
  AstraBookmarksManagerModel();
  ~AstraBookmarksManagerModel();

  AstraBookmarksManagerModel(const AstraBookmarksManagerModel&) = delete;
  AstraBookmarksManagerModel& operator=(const AstraBookmarksManagerModel&) =
      delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraBookmarksManagerObserver* observer);
  void RemoveObserver(AstraBookmarksManagerObserver* observer);

  // -- Folder access --------------------------------------------------------

  // Get the root folder (contains the three permanent nodes).
  const AstraBookmarkFolder& GetRootFolder() const;

  // Get the Bookmarks bar folder.
  const AstraBookmarkFolder* GetBookmarksBarFolder() const;

  // Get the "Other bookmarks" folder.
  const AstraBookmarkFolder* GetOtherBookmarksFolder() const;

  // Get the "Mobile bookmarks" folder.
  const AstraBookmarkFolder* GetMobileBookmarksFolder() const;

  // Get a specific folder by ID. Returns nullptr if not found.
  const AstraBookmarkFolder* GetFolder(AstraBookmarkId id) const;

  // -- Bookmark access ------------------------------------------------------

  // Get a specific bookmark by ID. Returns nullptr if not found.
  const AstraBookmarkEntry* GetBookmark(AstraBookmarkId id) const;

  // Get the total number of bookmarks (flat, across all folders).
  size_t FlatBookmarkCount() const;

  // -- Search ---------------------------------------------------------------

  // Search bookmarks by title or URL. Returns matching entries.
  std::vector<AstraBookmarkEntry> SearchBookmarks(
      const std::u16string& query) const;

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Bookmark manipulation ------------------------------------------------

  // Add a new bookmark. Returns the ID of the new bookmark, or 0 on failure.
  AstraBookmarkId AddBookmark(AstraBookmarkId parent_id,
                              const std::u16string& title,
                              const std::string& url,
                              int index = -1);

  // Remove a bookmark by ID.
  void RemoveBookmark(AstraBookmarkId id);

  // Update a bookmark's title and URL.
  void UpdateBookmark(AstraBookmarkId id,
                      const std::u16string& title,
                      const std::string& url);

  // Move a bookmark to a new parent at the given index.
  void MoveBookmark(AstraBookmarkId id,
                    AstraBookmarkId new_parent_id,
                    int index);

  // -- Folder manipulation --------------------------------------------------

  // Create a new folder. Returns the ID of the new folder, or 0 on failure.
  AstraBookmarkId CreateFolder(AstraBookmarkId parent_id,
                               const std::u16string& title,
                               int index = -1);

  // Remove a folder and all its contents.
  void RemoveFolder(AstraBookmarkId id);

  // Rename a folder.
  void RenameFolder(AstraBookmarkId id, const std::u16string& title);

  // -- Expanded folders -----------------------------------------------------

  void SetFolderExpanded(AstraBookmarkId folder_id, bool expanded);
  bool IsFolderExpanded(AstraBookmarkId folder_id) const;
  const std::set<AstraBookmarkId>& GetExpandedFolders() const {
    return expanded_folders_;
  }

  // -- Sorting --------------------------------------------------------------

  // Sort the contents of a folder by the given sort type.
  void SortFolder(AstraBookmarkId folder_id, AstraBookmarkSortType sort_type);

  // -- Sample data ----------------------------------------------------------

  // Populate with rich sample bookmark data for development/testing.
  void PopulateSampleBookmarks();

  // -- Categories -----------------------------------------------------------

  // Get unique category names from all bookmarks.
  std::vector<std::string> GetCategories() const;

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

 private:
  // -- Notification helpers -------------------------------------------------

  void NotifyBookmarksModelChanged();
  void NotifyBookmarkAdded(AstraBookmarkId id);
  void NotifyBookmarkRemoved(AstraBookmarkId id);
  void NotifyBookmarkChanged(AstraBookmarkId id);
  void NotifyFolderExpanded(AstraBookmarkId folder_id);
  void NotifySearchChanged();

  // -- Recursive helpers ----------------------------------------------------

  // Find a folder by ID within a folder tree.
  AstraBookmarkFolder* FindFolder(AstraBookmarkFolder& root,
                                  AstraBookmarkId id);
  const AstraBookmarkFolder* FindFolder(const AstraBookmarkFolder& root,
                                        AstraBookmarkId id) const;

  // Find a bookmark by ID within a folder tree.
  AstraBookmarkEntry* FindBookmark(AstraBookmarkFolder& root,
                                   AstraBookmarkId id);
  const AstraBookmarkEntry* FindBookmark(const AstraBookmarkFolder& root,
                                         AstraBookmarkId id) const;

  // Recursively count all bookmarks in a folder tree.
  int CountBookmarksRecursive(const AstraBookmarkFolder& folder) const;

  // Recursively search for bookmarks matching a query.
  void SearchRecursive(const AstraBookmarkFolder& folder,
                       const std::u16string& query_lower,
                       std::vector<AstraBookmarkEntry>& results) const;

  // Recursively collect unique categories.
  void CollectCategoriesRecursive(const AstraBookmarkFolder& folder,
                                  std::set<std::string>& categories) const;

  // Recursively remove a bookmark by ID. Returns true if found and removed.
  bool RemoveBookmarkRecursive(AstraBookmarkFolder& folder,
                               AstraBookmarkId id);

  // Recursively remove a folder by ID. Returns true if found and removed.
  bool RemoveFolderRecursive(AstraBookmarkFolder& parent,
                             AstraBookmarkId id);

  // Update total_bookmarks for a folder and all its ancestors.
  void UpdateTotalBookmarks(AstraBookmarkFolder& folder);

  // Generate the next available ID.
  AstraBookmarkId NextId();

  // -- State ----------------------------------------------------------------

  // Root folder containing the three permanent nodes (bar, other, mobile).
  AstraBookmarkFolder root_folder_;

  // Current search query.
  std::u16string search_query_;

  // Current category filter (empty = all categories).
  std::string category_filter_;

  // Set of expanded folder IDs.
  std::set<AstraBookmarkId> expanded_folders_;

  // Next ID to assign.
  AstraBookmarkId next_id_ = 1;

  // Observers.
  base::ObserverList<AstraBookmarksManagerObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_MODEL_H_
