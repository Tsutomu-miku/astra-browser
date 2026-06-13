// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_MODEL_H_
#define ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// A single reading list entry.
struct AstraReadingListEntry {
  std::string id;
  std::u16string title;
  std::string url;
  gfx::ImageSkia favicon;
  std::string favicon_url;
  std::u16string preview_text;
  int estimated_read_time_minutes = 0;
  base::Time date_added;
  base::Time date_last_read;
  bool is_read = false;
  bool is_favorited = false;
  std::string category;
  std::string workspace;
  std::string folder;
  std::string site_name;
};

// A reading list folder.
struct AstraReadingListFolder {
  std::string id;
  std::u16string name;
  int entry_count = 0;
};

// Filter options for reading list.
enum class AstraReadingListFilter {
  kAll,
  kUnread,
  kRead,
  kFavorites,
};

// Sort type for reading list entries.
enum class AstraReadingListSortType {
  kNewestFirst,
  kOldestFirst,
  kAlphabetical,
  kReadTime,
};

// Observer for AstraReadingListModel.
class AstraReadingListObserver : public base::CheckedObserver {
 public:
  // Called when the reading list changes in any way.
  virtual void OnReadingListChanged() {}

  // Called when an entry is added.
  virtual void OnEntryAdded(const std::string& id) {}

  // Called when an entry is removed.
  virtual void OnEntryRemoved(const std::string& id) {}

  // Called when an entry's properties change.
  virtual void OnEntryUpdated(const std::string& id) {}

  // Called when a folder is added.
  virtual void OnFolderAdded(const std::string& id) {}

  // Called when a folder is removed.
  virtual void OnFolderRemoved(const std::string& id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(const std::u16string& query) {}

  // Called when the filter changes.
  virtual void OnFilterChanged() {}

  // Called when the model is about to be destroyed.
  virtual void OnReadingListModelShutdown() {}

 protected:
  ~AstraReadingListObserver() override = default;
};

// Model for the reading list page.
//
// Owns reading list entries and folders, with search, filtering, sorting,
// and category/folder filtering.  Reading list data comes from Chromium's
// ReadingListModel — this model projects and augments it with Astra-specific
// folder, category, and workspace metadata.
//
// Chromium owner: ReadingListModel / ReadingListEntry
//   (components/reading_list/core/reading_list_model.h)
//
// TODO(astra): Wire up to Chromium's ReadingListModel via a KeyedService
// wrapper.  Patch point:
// chrome/browser/reading_list/reading_list_model_factory.cc or
// chrome/browser/ui/webui/side_panel/reading_list/reading_list_page_handler.cc.
class AstraReadingListModel {
 public:
  AstraReadingListModel();
  ~AstraReadingListModel();

  AstraReadingListModel(const AstraReadingListModel&) = delete;
  AstraReadingListModel& operator=(const AstraReadingListModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraReadingListObserver* observer);
  void RemoveObserver(AstraReadingListObserver* observer);

  // -- Entry access ---------------------------------------------------------

  // Get all entries (unfiltered).
  const std::vector<AstraReadingListEntry>& GetEntries() const;

  // Get a specific entry by ID. Returns nullptr if not found.
  const AstraReadingListEntry* GetEntry(const std::string& id) const;

  // Get total count of entries.
  size_t GetCount() const;

  // Get count of unread entries.
  size_t GetUnreadCount() const;

  // Get count of favorited entries.
  size_t GetFavoritesCount() const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Filtering ------------------------------------------------------------

  void SetFilter(AstraReadingListFilter filter);
  AstraReadingListFilter GetFilter() const { return filter_; }

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

  void SetFolderFilter(const std::string& folder);
  const std::string& GetFolderFilter() const { return folder_filter_; }

  // -- Sorting --------------------------------------------------------------

  void SetSortType(AstraReadingListSortType sort_type);
  AstraReadingListSortType GetSortType() const { return sort_type_; }

  // -- Categories -----------------------------------------------------------

  // Get unique category names from all entries.
  std::vector<std::string> GetCategories() const;

  // -- Folders --------------------------------------------------------------

  // Get the list of reading list folders.
  const std::vector<AstraReadingListFolder>& GetFolders() const;

  // -- Entry manipulation ---------------------------------------------------

  // Add a new entry. Returns the ID of the new entry.
  std::string AddEntry(const std::u16string& title,
                       const std::string& url,
                       const std::u16string& preview_text,
                       int estimated_read_time,
                       const std::string& category,
                       const std::string& folder);

  // Remove an entry by ID.
  void RemoveEntry(const std::string& id);

  // Mark an entry as read.
  void MarkAsRead(const std::string& id);

  // Mark an entry as unread.
  void MarkAsUnread(const std::string& id);

  // Toggle the favorite status of an entry.
  void ToggleFavorite(const std::string& id);

  // Move an entry to a different folder.
  void MoveEntryToFolder(const std::string& id, const std::string& folder);

  // -- Folder manipulation --------------------------------------------------

  // Add a new folder. Returns the ID of the new folder.
  std::string AddFolder(const std::u16string& name);

  // Remove a folder.
  void RemoveFolder(const std::string& id);

  // Rename a folder.
  void RenameFolder(const std::string& id, const std::u16string& name);

  // -- Sample data ----------------------------------------------------------

  // Populate with 20+ sample reading list entries across categories and folders.
  void PopulateSampleEntries();

  // -- State ----------------------------------------------------------------

  // Whether the reading list is currently loading.
  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

  // Get filtered and sorted entries for display.
  std::vector<AstraReadingListEntry> GetFilteredEntries() const;

 private:
  // -- Filtering helpers ----------------------------------------------------

  bool MatchesSearch(const AstraReadingListEntry& entry) const;
  bool MatchesFilter(const AstraReadingListEntry& entry) const;
  bool MatchesCategory(const AstraReadingListEntry& entry) const;
  bool MatchesFolder(const AstraReadingListEntry& entry) const;

  // -- Sorting helpers ------------------------------------------------------

  void SortEntries(std::vector<AstraReadingListEntry>& entries) const;

  // -- Apply filters --------------------------------------------------------

  void ApplyFilters();

  // -- Folder helpers -------------------------------------------------------

  void UpdateFolderEntryCounts();
  AstraReadingListFolder* FindFolder(const std::string& id);
  const AstraReadingListFolder* FindFolder(const std::string& id) const;

  // -- Notification helpers -------------------------------------------------

  void NotifyReadingListChanged();
  void NotifyEntryAdded(const std::string& id);
  void NotifyEntryRemoved(const std::string& id);
  void NotifyEntryUpdated(const std::string& id);
  void NotifyFolderAdded(const std::string& id);
  void NotifyFolderRemoved(const std::string& id);
  void NotifySearchChanged();
  void NotifyFilterChanged();

  // Generate a unique ID.
  std::string GenerateId() const;

  // -- State ----------------------------------------------------------------

  // All reading list entries (unfiltered).
  std::vector<AstraReadingListEntry> all_entries_;

  // Filtered entries (cached).
  mutable std::vector<AstraReadingListEntry> filtered_entries_;
  mutable bool filtered_dirty_ = true;

  // Folders.
  std::vector<AstraReadingListFolder> folders_;

  // Current search query.
  std::u16string search_query_;

  // Current filter.
  AstraReadingListFilter filter_ = AstraReadingListFilter::kAll;

  // Current category filter (empty = all).
  std::string category_filter_;

  // Current folder filter (empty = all).
  std::string folder_filter_;

  // Current sort type.
  AstraReadingListSortType sort_type_ = AstraReadingListSortType::kNewestFirst;

  // Loading state.
  bool loading_ = false;

  // Next ID counter for generating unique IDs.
  mutable int next_id_ = 1;

  // Observers.
  base::ObserverList<AstraReadingListObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_MODEL_H_
