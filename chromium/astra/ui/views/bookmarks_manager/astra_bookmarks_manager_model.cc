// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/bookmarks_manager/astra_bookmarks_manager_model.h"

#include <algorithm>

#include "base/i18n/case_conversion.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Helper to check if a string contains a query (case-insensitive).
bool StringContains(const std::u16string& haystack,
                    const std::u16string& needle_lower) {
  if (needle_lower.empty()) {
    return true;
  }
  std::u16string haystack_lower = base::i18n::ToLower(haystack);
  return haystack_lower.find(needle_lower) != std::u16string::npos;
}

}  // namespace

// ===========================================================================
// AstraBookmarksManagerModel
// ===========================================================================

AstraBookmarksManagerModel::AstraBookmarksManagerModel() {
  // Set up root folder.
  root_folder_.id = 0;
  root_folder_.title = u"Bookmarks";
  root_folder_.parent_id = -1;

  // Create the three permanent folders.
  AstraBookmarkFolder bar_folder;
  bar_folder.id = NextId();
  bar_folder.title = u"Bookmarks bar";
  bar_folder.parent_id = 0;
  bar_folder.total_bookmarks = 0;

  AstraBookmarkFolder other_folder;
  other_folder.id = NextId();
  other_folder.title = u"Other bookmarks";
  other_folder.parent_id = 0;
  other_folder.total_bookmarks = 0;

  AstraBookmarkFolder mobile_folder;
  mobile_folder.id = NextId();
  mobile_folder.title = u"Mobile bookmarks";
  mobile_folder.parent_id = 0;
  mobile_folder.total_bookmarks = 0;

  root_folder_.subfolders.push_back(std::move(bar_folder));
  root_folder_.subfolders.push_back(std::move(other_folder));
  root_folder_.subfolders.push_back(std::move(mobile_folder));

  // Bar folder is expanded by default.
  expanded_folders_.insert(root_folder_.subfolders[0].id);
  expanded_folders_.insert(root_folder_.subfolders[1].id);
}

AstraBookmarksManagerModel::~AstraBookmarksManagerModel() {
  for (auto& observer : observers_) {
    observer.OnBookmarksManagerModelShutdown();
  }
}

// -- Observer management ----------------------------------------------------

void AstraBookmarksManagerModel::AddObserver(
    AstraBookmarksManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraBookmarksManagerModel::RemoveObserver(
    AstraBookmarksManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Folder access ----------------------------------------------------------

const AstraBookmarkFolder& AstraBookmarksManagerModel::GetRootFolder() const {
  return root_folder_;
}

const AstraBookmarkFolder* AstraBookmarksManagerModel::GetBookmarksBarFolder()
    const {
  if (root_folder_.subfolders.empty()) {
    return nullptr;
  }
  return &root_folder_.subfolders[0];
}

const AstraBookmarkFolder* AstraBookmarksManagerModel::GetOtherBookmarksFolder()
    const {
  if (root_folder_.subfolders.size() < 2) {
    return nullptr;
  }
  return &root_folder_.subfolders[1];
}

const AstraBookmarkFolder* AstraBookmarksManagerModel::GetMobileBookmarksFolder()
    const {
  if (root_folder_.subfolders.size() < 3) {
    return nullptr;
  }
  return &root_folder_.subfolders[2];
}

const AstraBookmarkFolder* AstraBookmarksManagerModel::GetFolder(
    AstraBookmarkId id) const {
  if (id == 0) {
    return &root_folder_;
  }
  return FindFolder(root_folder_, id);
}

AstraBookmarkFolder* AstraBookmarksManagerModel::FindFolder(
    AstraBookmarkFolder& root,
    AstraBookmarkId id) {
  for (auto& subfolder : root.subfolders) {
    if (subfolder.id == id) {
      return &subfolder;
    }
    AstraBookmarkFolder* found = FindFolder(subfolder, id);
    if (found) {
      return found;
    }
  }
  return nullptr;
}

const AstraBookmarkFolder* AstraBookmarksManagerModel::FindFolder(
    const AstraBookmarkFolder& root,
    AstraBookmarkId id) const {
  for (const auto& subfolder : root.subfolders) {
    if (subfolder.id == id) {
      return &subfolder;
    }
    const AstraBookmarkFolder* found = FindFolder(subfolder, id);
    if (found) {
      return found;
    }
  }
  return nullptr;
}

// -- Bookmark access --------------------------------------------------------

const AstraBookmarkEntry* AstraBookmarksManagerModel::GetBookmark(
    AstraBookmarkId id) const {
  return FindBookmark(root_folder_, id);
}

AstraBookmarkEntry* AstraBookmarksManagerModel::FindBookmark(
    AstraBookmarkFolder& root,
    AstraBookmarkId id) {
  for (auto& entry : root.children) {
    if (entry.id == id) {
      return &entry;
    }
  }
  for (auto& subfolder : root.subfolders) {
    AstraBookmarkEntry* found = FindBookmark(subfolder, id);
    if (found) {
      return found;
    }
  }
  return nullptr;
}

const AstraBookmarkEntry* AstraBookmarksManagerModel::FindBookmark(
    const AstraBookmarkFolder& root,
    AstraBookmarkId id) const {
  for (const auto& entry : root.children) {
    if (entry.id == id) {
      return &entry;
    }
  }
  for (const auto& subfolder : root.subfolders) {
    const AstraBookmarkEntry* found = FindBookmark(subfolder, id);
    if (found) {
      return found;
    }
  }
  return nullptr;
}

size_t AstraBookmarksManagerModel::FlatBookmarkCount() const {
  return CountBookmarksRecursive(root_folder_);
}

int AstraBookmarksManagerModel::CountBookmarksRecursive(
    const AstraBookmarkFolder& folder) const {
  int count = static_cast<int>(folder.children.size());
  for (const auto& subfolder : folder.subfolders) {
    count += CountBookmarksRecursive(subfolder);
  }
  return count;
}

// -- Search -----------------------------------------------------------------

std::vector<AstraBookmarkEntry> AstraBookmarksManagerModel::SearchBookmarks(
    const std::u16string& query) const {
  std::vector<AstraBookmarkEntry> results;
  if (query.empty()) {
    return results;
  }
  std::u16string query_lower = base::i18n::ToLower(query);
  SearchRecursive(root_folder_, query_lower, results);
  return results;
}

void AstraBookmarksManagerModel::SearchRecursive(
    const AstraBookmarkFolder& folder,
    const std::u16string& query_lower,
    std::vector<AstraBookmarkEntry>& results) const {
  for (const auto& entry : folder.children) {
    if (StringContains(entry.title, query_lower) ||
        StringContains(base::UTF8ToUTF16(entry.url), query_lower)) {
      // Apply category filter if set.
      if (category_filter_.empty() || entry.category == category_filter_) {
        results.push_back(entry);
      }
    }
  }
  for (const auto& subfolder : folder.subfolders) {
    SearchRecursive(subfolder, query_lower, results);
  }
}

void AstraBookmarksManagerModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifySearchChanged();
}

// -- Bookmark manipulation --------------------------------------------------

AstraBookmarkId AstraBookmarksManagerModel::NextId() {
  return next_id_++;
}

AstraBookmarkId AstraBookmarksManagerModel::AddBookmark(
    AstraBookmarkId parent_id,
    const std::u16string& title,
    const std::string& url,
    int index) {
  AstraBookmarkFolder* parent = nullptr;
  if (parent_id == 0) {
    parent = &root_folder_;
  } else {
    parent = FindFolder(root_folder_, parent_id);
  }
  if (!parent) {
    return 0;
  }

  AstraBookmarkEntry entry;
  entry.id = NextId();
  entry.title = title;
  entry.url = url;
  entry.parent_id = parent_id;
  entry.is_folder = false;
  entry.date_added = base::Time::Now();

  if (index < 0 || index >= static_cast<int>(parent->children.size())) {
    parent->children.push_back(std::move(entry));
  } else {
    parent->children.insert(parent->children.begin() + index,
                            std::move(entry));
  }

  UpdateTotalBookmarks(*parent);
  NotifyBookmarkAdded(parent->children[index < 0 ?
      static_cast<int>(parent->children.size()) - 1 : index].id);
  NotifyBookmarksModelChanged();

  return parent->children.back().id;
}

void AstraBookmarksManagerModel::RemoveBookmark(AstraBookmarkId id) {
  if (RemoveBookmarkRecursive(root_folder_, id)) {
    NotifyBookmarkRemoved(id);
    NotifyBookmarksModelChanged();
  }
}

bool AstraBookmarksManagerModel::RemoveBookmarkRecursive(
    AstraBookmarkFolder& folder,
    AstraBookmarkId id) {
  for (auto it = folder.children.begin(); it != folder.children.end(); ++it) {
    if (it->id == id) {
      folder.children.erase(it);
      UpdateTotalBookmarks(folder);
      return true;
    }
  }
  for (auto& subfolder : folder.subfolders) {
    if (RemoveBookmarkRecursive(subfolder, id)) {
      UpdateTotalBookmarks(folder);
      return true;
    }
  }
  return false;
}

void AstraBookmarksManagerModel::UpdateBookmark(AstraBookmarkId id,
                                                const std::u16string& title,
                                                const std::string& url) {
  AstraBookmarkEntry* entry = FindBookmark(root_folder_, id);
  if (!entry) {
    return;
  }
  entry->title = title;
  entry->url = url;
  NotifyBookmarkChanged(id);
}

void AstraBookmarksManagerModel::MoveBookmark(AstraBookmarkId id,
                                              AstraBookmarkId new_parent_id,
                                              int index) {
  // Find the bookmark.
  AstraBookmarkEntry* entry = FindBookmark(root_folder_, id);
  if (!entry) {
    return;
  }

  // Copy the entry.
  AstraBookmarkEntry copied = *entry;
  copied.parent_id = new_parent_id;

  // Remove from old location.
  RemoveBookmarkRecursive(root_folder_, id);

  // Add to new parent.
  AstraBookmarkFolder* new_parent = nullptr;
  if (new_parent_id == 0) {
    new_parent = &root_folder_;
  } else {
    new_parent = FindFolder(root_folder_, new_parent_id);
  }
  if (!new_parent) {
    return;
  }

  copied.id = NextId();  // Reassign ID for simplicity in this scaffold.
  // TODO(astra): Preserve original IDs when wiring up to real
  // Chromium BookmarkModel.

  if (index < 0 || index >= static_cast<int>(new_parent->children.size())) {
    new_parent->children.push_back(std::move(copied));
  } else {
    new_parent->children.insert(new_parent->children.begin() + index,
                                std::move(copied));
  }

  UpdateTotalBookmarks(*new_parent);
  NotifyBookmarkChanged(new_parent->children.back().id);
  NotifyBookmarksModelChanged();
}

// -- Folder manipulation ----------------------------------------------------

AstraBookmarkId AstraBookmarksManagerModel::CreateFolder(
    AstraBookmarkId parent_id,
    const std::u16string& title,
    int index) {
  AstraBookmarkFolder* parent = nullptr;
  if (parent_id == 0) {
    parent = &root_folder_;
  } else {
    parent = FindFolder(root_folder_, parent_id);
  }
  if (!parent) {
    return 0;
  }

  AstraBookmarkFolder folder;
  folder.id = NextId();
  folder.title = title;
  folder.parent_id = parent_id;
  folder.total_bookmarks = 0;

  if (index < 0 || index >= static_cast<int>(parent->subfolders.size())) {
    parent->subfolders.push_back(std::move(folder));
  } else {
    parent->subfolders.insert(parent->subfolders.begin() + index,
                              std::move(folder));
  }

  UpdateTotalBookmarks(*parent);
  NotifyBookmarksModelChanged();

  return parent->subfolders.back().id;
}

void AstraBookmarksManagerModel::RemoveFolder(AstraBookmarkId id) {
  if (id <= 0) {
    return;  // Cannot remove root or permanent nodes in this scaffold.
  }
  if (RemoveFolderRecursive(root_folder_, id)) {
    expanded_folders_.erase(id);
    NotifyBookmarksModelChanged();
  }
}

bool AstraBookmarksManagerModel::RemoveFolderRecursive(
    AstraBookmarkFolder& parent,
    AstraBookmarkId id) {
  for (auto it = parent.subfolders.begin(); it != parent.subfolders.end();
       ++it) {
    if (it->id == id) {
      parent.subfolders.erase(it);
      UpdateTotalBookmarks(parent);
      return true;
    }
    if (RemoveFolderRecursive(*it, id)) {
      UpdateTotalBookmarks(parent);
      return true;
    }
  }
  return false;
}

void AstraBookmarksManagerModel::RenameFolder(AstraBookmarkId id,
                                              const std::u16string& title) {
  AstraBookmarkFolder* folder = FindFolder(root_folder_, id);
  if (!folder) {
    return;
  }
  folder->title = title;
  NotifyBookmarksModelChanged();
}

void AstraBookmarksManagerModel::UpdateTotalBookmarks(
    AstraBookmarkFolder& folder) {
  folder.total_bookmarks = CountBookmarksRecursive(folder);
}

// -- Expanded folders -------------------------------------------------------

void AstraBookmarksManagerModel::SetFolderExpanded(AstraBookmarkId folder_id,
                                                   bool expanded) {
  if (expanded) {
    if (expanded_folders_.insert(folder_id).second) {
      NotifyFolderExpanded(folder_id);
    }
  } else {
    if (expanded_folders_.erase(folder_id) > 0) {
      NotifyFolderExpanded(folder_id);
    }
  }
}

bool AstraBookmarksManagerModel::IsFolderExpanded(
    AstraBookmarkId folder_id) const {
  return expanded_folders_.count(folder_id) > 0;
}

// -- Sorting ----------------------------------------------------------------

void AstraBookmarksManagerModel::SortFolder(AstraBookmarkId folder_id,
                                            AstraBookmarkSortType sort_type) {
  AstraBookmarkFolder* folder = nullptr;
  if (folder_id == 0) {
    folder = &root_folder_;
  } else {
    folder = FindFolder(root_folder_, folder_id);
  }
  if (!folder) {
    return;
  }

  switch (sort_type) {
    case AstraBookmarkSortType::kName:
      std::sort(folder->children.begin(), folder->children.end(),
                [](const AstraBookmarkEntry& a, const AstraBookmarkEntry& b) {
                  return a.title < b.title;
                });
      std::sort(folder->subfolders.begin(), folder->subfolders.end(),
                [](const AstraBookmarkFolder& a, const AstraBookmarkFolder& b) {
                  return a.title < b.title;
                });
      break;
    case AstraBookmarkSortType::kDateAddedNewest:
      std::sort(folder->children.begin(), folder->children.end(),
                [](const AstraBookmarkEntry& a, const AstraBookmarkEntry& b) {
                  return a.date_added > b.date_added;
                });
      break;
    case AstraBookmarkSortType::kDateAddedOldest:
      std::sort(folder->children.begin(), folder->children.end(),
                [](const AstraBookmarkEntry& a, const AstraBookmarkEntry& b) {
                  return a.date_added < b.date_added;
                });
      break;
    case AstraBookmarkSortType::kUrl:
      std::sort(folder->children.begin(), folder->children.end(),
                [](const AstraBookmarkEntry& a, const AstraBookmarkEntry& b) {
                  return a.url < b.url;
                });
      break;
    case AstraBookmarkSortType::kMostVisited:
      std::sort(folder->children.begin(), folder->children.end(),
                [](const AstraBookmarkEntry& a, const AstraBookmarkEntry& b) {
                  return a.visit_count > b.visit_count;
                });
      break;
  }

  NotifyBookmarksModelChanged();
}

// -- Sample data ------------------------------------------------------------

void AstraBookmarksManagerModel::PopulateSampleBookmarks() {
  // Clear existing data first.
  root_folder_.subfolders.clear();
  root_folder_.children.clear();
  next_id_ = 1;
  expanded_folders_.clear();

  // Recreate permanent folders.
  AstraBookmarkFolder bar_folder;
  bar_folder.id = NextId();
  bar_folder.title = u"Bookmarks bar";
  bar_folder.parent_id = 0;
  AstraBookmarkId bar_id = bar_folder.id;

  AstraBookmarkFolder other_folder;
  other_folder.id = NextId();
  other_folder.title = u"Other bookmarks";
  other_folder.parent_id = 0;
  AstraBookmarkId other_id = other_folder.id;

  AstraBookmarkFolder mobile_folder;
  mobile_folder.id = NextId();
  mobile_folder.title = u"Mobile bookmarks";
  mobile_folder.parent_id = 0;

  // ---- Bookmarks bar contents ----
  std::vector<AstraBookmarkEntry> bar_bookmarks = {
      {0, u"Gmail", "https://mail.google.com", bar_id, false, {}, "",
       base::Time::Now() - base::Days(30), true, false, false, "Work",
       "Productivity", 120},
      {0, u"Google Calendar", "https://calendar.google.com", bar_id, false, {},
       "", base::Time::Now() - base::Days(25), true, false, false, "Work",
       "Productivity", 85},
      {0, u"Google Docs", "https://docs.google.com", bar_id, false, {}, "",
       base::Time::Now() - base::Days(20), true, false, false, "Work",
       "Productivity", 60},
      {0, u"GitHub", "https://github.com", bar_id, false, {}, "",
       base::Time::Now() - base::Days(15), true, false, false, "Work",
       "Development", 200},
      {0, u"Stack Overflow", "https://stackoverflow.com", bar_id, false, {},
       "", base::Time::Now() - base::Days(10), true, false, false, "Work",
       "Development", 150},
  };

  for (auto& bm : bar_bookmarks) {
    bm.id = NextId();
    bar_folder.children.push_back(bm);
  }

  // Create a "Design" folder in the bookmarks bar.
  AstraBookmarkFolder design_folder;
  design_folder.id = NextId();
  design_folder.title = u"Design";
  design_folder.parent_id = bar_id;

  std::vector<AstraBookmarkEntry> design_bookmarks = {
      {0, u"Figma", "https://figma.com", design_folder.id, false, {}, "",
       base::Time::Now() - base::Days(5), false, false, false, "Work",
       "Design", 90},
      {0, u"Dribbble", "https://dribbble.com", design_folder.id, false, {}, "",
       base::Time::Now() - base::Days(3), false, false, false, "Personal",
       "Design", 45},
      {0, u"Unsplash", "https://unsplash.com", design_folder.id, false, {}, "",
       base::Time::Now() - base::Days(7), false, false, false, "Personal",
       "Design", 30},
  };

  for (auto& bm : design_bookmarks) {
    bm.id = NextId();
    design_folder.children.push_back(bm);
  }
  design_folder.total_bookmarks =
      static_cast<int>(design_folder.children.size());
  bar_folder.subfolders.push_back(std::move(design_folder));

  // Create a "News" folder in the bookmarks bar.
  AstraBookmarkFolder news_folder;
  news_folder.id = NextId();
  news_folder.title = u"News";
  news_folder.parent_id = bar_id;

  std::vector<AstraBookmarkEntry> news_bookmarks = {
      {0, u"TechCrunch", "https://techcrunch.com", news_folder.id, false, {},
       "", base::Time::Now() - base::Days(2), false, false, false, "Personal",
       "News", 75},
      {0, u"The Verge", "https://theverge.com", news_folder.id, false, {}, "",
       base::Time::Now() - base::Days(4), false, false, false, "Personal",
       "News", 60},
      {0, u"Ars Technica", "https://arstechnica.com", news_folder.id, false,
       {}, "", base::Time::Now() - base::Days(6), false, false, false,
       "Personal", "News", 50},
      {0, u"Hacker News", "https://news.ycombinator.com", news_folder.id, false,
       {}, "", base::Time::Now() - base::Days(1), false, false, false, "Work",
       "News", 110},
  };

  for (auto& bm : news_bookmarks) {
    bm.id = NextId();
    news_folder.children.push_back(bm);
  }
  news_folder.total_bookmarks = static_cast<int>(news_folder.children.size());
  bar_folder.subfolders.push_back(std::move(news_folder));

  bar_folder.total_bookmarks =
      static_cast<int>(bar_bookmarks.size()) +
      static_cast<int>(bar_folder.subfolders[0].children.size()) +
      static_cast<int>(bar_folder.subfolders[1].children.size());

  // ---- Other bookmarks contents ----
  std::vector<AstraBookmarkEntry> other_bookmarks = {
      {0, u"Wikipedia", "https://en.wikipedia.org", other_id, false, {}, "",
       base::Time::Now() - base::Days(60), false, true, false, "Personal",
       "Reference", 40},
      {0, u"MDN Web Docs", "https://developer.mozilla.org", other_id, false,
       {}, "", base::Time::Now() - base::Days(45), false, true, false, "Work",
       "Development", 180},
      {0, u"Notion", "https://notion.so", other_id, false, {}, "",
       base::Time::Now() - base::Days(20), false, true, false, "Work",
       "Productivity", 55},
      {0, u"Spotify Web Player", "https://open.spotify.com", other_id, false,
       {}, "", base::Time::Now() - base::Days(12), false, true, false,
       "Personal", "Entertainment", 200},
      {0, u"YouTube", "https://youtube.com", other_id, false, {}, "",
       base::Time::Now() - base::Days(50), false, true, false, "Personal",
       "Entertainment", 500},
      {0, u"Reddit", "https://reddit.com", other_id, false, {}, "",
       base::Time::Now() - base::Days(35), false, true, false, "Personal",
       "Social", 300},
      {0, u"Twitter / X", "https://twitter.com", other_id, false, {}, "",
       base::Time::Now() - base::Days(18), false, true, false, "Personal",
       "Social", 250},
  };

  for (auto& bm : other_bookmarks) {
    bm.id = NextId();
    other_folder.children.push_back(bm);
  }

  // Create a "Learning" folder in other bookmarks.
  AstraBookmarkFolder learning_folder;
  learning_folder.id = NextId();
  learning_folder.title = u"Learning";
  learning_folder.parent_id = other_id;

  std::vector<AstraBookmarkEntry> learning_bookmarks = {
      {0, u"Coursera", "https://coursera.org", learning_folder.id, false, {},
       "", base::Time::Now() - base::Days(90), false, false, false, "Personal",
       "Learning", 20},
      {0, u"Khan Academy", "https://khanacademy.org", learning_folder.id, false,
       {}, "", base::Time::Now() - base::Days(80), false, false, false,
       "Personal", "Learning", 15},
      {0, u"freeCodeCamp", "https://freecodecamp.org", learning_folder.id,
       false, {}, "", base::Time::Now() - base::Days(70), false, false, false,
       "Work", "Learning", 35},
  };

  for (auto& bm : learning_bookmarks) {
    bm.id = NextId();
    learning_folder.children.push_back(bm);
  }
  learning_folder.total_bookmarks =
      static_cast<int>(learning_folder.children.size());
  other_folder.subfolders.push_back(std::move(learning_folder));

  // Create a "Finance" folder in other bookmarks.
  AstraBookmarkFolder finance_folder;
  finance_folder.id = NextId();
  finance_folder.title = u"Finance";
  finance_folder.parent_id = other_id;

  std::vector<AstraBookmarkEntry> finance_bookmarks = {
      {0, u"Robinhood", "https://robinhood.com", finance_folder.id, false, {},
       "", base::Time::Now() - base::Days(40), false, false, false,
       "Personal", "Finance", 10},
      {0, u"Coinbase", "https://coinbase.com", finance_folder.id, false, {},
       "", base::Time::Now() - base::Days(55), false, false, false,
       "Personal", "Finance", 8},
  };

  for (auto& bm : finance_bookmarks) {
    bm.id = NextId();
    finance_folder.children.push_back(bm);
  }
  finance_folder.total_bookmarks =
      static_cast<int>(finance_folder.children.size());
  other_folder.subfolders.push_back(std::move(finance_folder));

  other_folder.total_bookmarks =
      static_cast<int>(other_bookmarks.size()) +
      static_cast<int>(other_folder.subfolders[0].children.size()) +
      static_cast<int>(other_folder.subfolders[1].children.size());

  // ---- Mobile bookmarks ----
  std::vector<AstraBookmarkEntry> mobile_bookmarks = {
      {0, u"Google Maps", "https://maps.google.com", mobile_folder.id, false,
       {}, "", base::Time::Now() - base::Days(100), false, false, true,
       "Personal", "Utilities", 25},
      {0, u"Amazon", "https://amazon.com", mobile_folder.id, false, {}, "",
       base::Time::Now() - base::Days(75), false, false, true, "Personal",
       "Shopping", 100},
  };

  for (auto& bm : mobile_bookmarks) {
    bm.id = NextId();
    mobile_folder.children.push_back(bm);
  }
  mobile_folder.total_bookmarks =
      static_cast<int>(mobile_bookmarks.size());

  // Add all folders to root.
  root_folder_.subfolders.push_back(std::move(bar_folder));
  root_folder_.subfolders.push_back(std::move(other_folder));
  root_folder_.subfolders.push_back(std::move(mobile_folder));

  root_folder_.total_bookmarks = CountBookmarksRecursive(root_folder_);

  // Expand the main folders by default.
  expanded_folders_.insert(bar_id);
  expanded_folders_.insert(other_id);

  NotifyBookmarksModelChanged();
}

// -- Categories -------------------------------------------------------------

std::vector<std::string> AstraBookmarksManagerModel::GetCategories() const {
  std::set<std::string> categories_set;
  CollectCategoriesRecursive(root_folder_, categories_set);
  return std::vector<std::string>(categories_set.begin(),
                                  categories_set.end());
}

void AstraBookmarksManagerModel::CollectCategoriesRecursive(
    const AstraBookmarkFolder& folder,
    std::set<std::string>& categories) const {
  for (const auto& entry : folder.children) {
    if (!entry.category.empty()) {
      categories.insert(entry.category);
    }
  }
  for (const auto& subfolder : folder.subfolders) {
    CollectCategoriesRecursive(subfolder, categories);
  }
}

void AstraBookmarksManagerModel::SetCategoryFilter(
    const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  NotifyBookmarksModelChanged();
}

// -- Notification helpers ---------------------------------------------------

void AstraBookmarksManagerModel::NotifyBookmarksModelChanged() {
  for (auto& observer : observers_) {
    observer.OnBookmarksModelChanged();
  }
}

void AstraBookmarksManagerModel::NotifyBookmarkAdded(AstraBookmarkId id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkAdded(id);
  }
}

void AstraBookmarksManagerModel::NotifyBookmarkRemoved(AstraBookmarkId id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkRemoved(id);
  }
}

void AstraBookmarksManagerModel::NotifyBookmarkChanged(AstraBookmarkId id) {
  for (auto& observer : observers_) {
    observer.OnBookmarkChanged(id);
  }
}

void AstraBookmarksManagerModel::NotifyFolderExpanded(
    AstraBookmarkId folder_id) {
  for (auto& observer : observers_) {
    observer.OnFolderExpanded(folder_id);
  }
}

void AstraBookmarksManagerModel::NotifySearchChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchChanged(search_query_);
  }
}

}  // namespace astra
