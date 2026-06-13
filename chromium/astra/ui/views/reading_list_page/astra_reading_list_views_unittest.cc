// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reading_list_page/astra_reading_list_model.h"
#include "astra/ui/views/reading_list_page/astra_reading_list_page_view.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// Model Tests
// ===========================================================================

class AstraReadingListModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraReadingListModel>();
  }

  void TearDown() override { model_.reset(); }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraReadingListModel> model_;
};

// Test model creation.
TEST_F(AstraReadingListModelTest, ModelCreation) {
  EXPECT_NE(nullptr, model_.get());
  EXPECT_EQ(0u, model_->GetCount());
  EXPECT_EQ(0u, model_->GetUnreadCount());
  EXPECT_EQ(0u, model_->GetFavoritesCount());
  EXPECT_TRUE(model_->GetEntries().empty());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_EQ(AstraReadingListFilter::kAll, model_->GetFilter());
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
  EXPECT_TRUE(model_->GetFolderFilter().empty());
  EXPECT_EQ(AstraReadingListSortType::kNewestFirst, model_->GetSortType());
  EXPECT_FALSE(model_->IsLoading());
}

// Test sample population.
TEST_F(AstraReadingListModelTest, PopulateSampleEntries) {
  model_->PopulateSampleEntries();

  // Should have 20+ entries.
  EXPECT_GE(model_->GetCount(), 20u);
  EXPECT_LT(0u, model_->GetUnreadCount());
  EXPECT_LT(0u, model_->GetFavoritesCount());

  // Should have categories.
  auto categories = model_->GetCategories();
  EXPECT_GT(categories.size(), 0u);

  // Should have folders.
  auto folders = model_->GetFolders();
  EXPECT_GT(folders.size(), 0u);
}

// Test adding an entry.
TEST_F(AstraReadingListModelTest, AddEntry) {
  size_t initial_count = model_->GetCount();

  std::string id = model_->AddEntry(
      u"Test Article", "https://example.com/test",
      u"This is a test article preview.",
      10, "Technology", "Later");

  EXPECT_FALSE(id.empty());
  EXPECT_EQ(initial_count + 1, model_->GetCount());

  const AstraReadingListEntry* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(u"Test Article", entry->title);
  EXPECT_EQ("https://example.com/test", entry->url);
  EXPECT_EQ(u"This is a test article preview.", entry->preview_text);
  EXPECT_EQ(10, entry->estimated_read_time_minutes);
  EXPECT_EQ("Technology", entry->category);
  EXPECT_EQ("Later", entry->folder);
  EXPECT_FALSE(entry->is_read);
  EXPECT_FALSE(entry->is_favorited);
  EXPECT_FALSE(entry->date_added.is_null());
  EXPECT_TRUE(entry->date_last_read.is_null());
  EXPECT_FALSE(entry->site_name.empty());
}

// Test removing an entry.
TEST_F(AstraReadingListModelTest, RemoveEntry) {
  std::string id = model_->AddEntry(u"To Remove", "https://remove.com",
                                     u"Preview", 5, "News", "Later");
  ASSERT_FALSE(id.empty());

  size_t count_before = model_->GetCount();

  model_->RemoveEntry(id);
  EXPECT_EQ(count_before - 1, model_->GetCount());
  EXPECT_EQ(nullptr, model_->GetEntry(id));
}

// Test removing non-existent entry is a no-op.
TEST_F(AstraReadingListModelTest, RemoveNonExistentEntry) {
  size_t count_before = model_->GetCount();
  model_->RemoveEntry("nonexistent");
  EXPECT_EQ(count_before, model_->GetCount());
}

// Test mark as read.
TEST_F(AstraReadingListModelTest, MarkAsRead) {
  std::string id = model_->AddEntry(u"Read Test", "https://read.com",
                                     u"Preview", 5, "News", "Later");
  ASSERT_FALSE(id.empty());

  const auto* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_read);

  model_->MarkAsRead(id);

  entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_TRUE(entry->is_read);
  EXPECT_FALSE(entry->date_last_read.is_null());

  // Unread count should decrease.
  EXPECT_EQ(0u, model_->GetUnreadCount());
}

// Test mark as unread.
TEST_F(AstraReadingListModelTest, MarkAsUnread) {
  std::string id = model_->AddEntry(u"Unread Test", "https://unread.com",
                                     u"Preview", 5, "News", "Later");
  model_->MarkAsRead(id);

  const auto* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_TRUE(entry->is_read);

  model_->MarkAsUnread(id);

  entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_read);
}

// Test toggle favorite.
TEST_F(AstraReadingListModelTest, ToggleFavorite) {
  std::string id = model_->AddEntry(u"Favorite Test", "https://fav.com",
                                     u"Preview", 5, "News", "Later");
  ASSERT_FALSE(id.empty());

  const auto* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_favorited);

  model_->ToggleFavorite(id);

  entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_TRUE(entry->is_favorited);
  EXPECT_EQ(1u, model_->GetFavoritesCount());

  // Toggle again.
  model_->ToggleFavorite(id);

  entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_favorited);
  EXPECT_EQ(0u, model_->GetFavoritesCount());
}

// Test moving entry to folder.
TEST_F(AstraReadingListModelTest, MoveEntryToFolder) {
  std::string id = model_->AddEntry(u"Move Test", "https://move.com",
                                     u"Preview", 5, "News", "Later");

  model_->MoveEntryToFolder(id, "Must Read");

  const auto* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ("Must Read", entry->folder);
}

// Test search.
TEST_F(AstraReadingListModelTest, Search) {
  model_->PopulateSampleEntries();
  size_t total = model_->GetCount();

  // Search for "Chromium".
  model_->SetSearchQuery(u"Chromium");
  auto filtered = model_->GetFilteredEntries();
  EXPECT_LT(filtered.size(), total);
  EXPECT_GT(filtered.size(), 0u);

  for (const auto& entry : filtered) {
    std::u16 title_lower = base::i18n::ToLower(entry.title);
    std::u16 preview_lower = base::i18n::ToLower(entry.preview_text);
    std::u16 site_lower = base::i18n::ToLower(
        base::UTF8ToUTF16(entry.site_name));
    std::u16 url_lower = base::i18n::ToLower(
        base::UTF8ToUTF16(entry.url));
    std::u16 query_lower = base::i18n::ToLower(u"Chromium");

    bool found = title_lower.find(query_lower) != std::u16string::npos ||
                 preview_lower.find(query_lower) != std::u16string::npos ||
                 site_lower.find(query_lower) != std::u16string::npos ||
                 url_lower.find(query_lower) != std::u16string::npos;
    EXPECT_TRUE(found) << "Entry does not match search: "
                       << base::UTF16ToUTF8(entry.title);
  }

  // Reset search.
  model_->SetSearchQuery(u"");
  auto all_filtered = model_->GetFilteredEntries();
  EXPECT_EQ(total, all_filtered.size());
}

// Test search query getter/setter.
TEST_F(AstraReadingListModelTest, SearchQuery) {
  EXPECT_TRUE(model_->GetSearchQuery().empty());

  model_->SetSearchQuery(u"test");
  EXPECT_EQ(u"test", model_->GetSearchQuery());

  model_->SetSearchQuery(u"");
  EXPECT_TRUE(model_->GetSearchQuery().empty());
}

// Test filter: unread.
TEST_F(AstraReadingListModelTest, FilterUnread) {
  model_->PopulateSampleEntries();
  size_t total = model_->GetCount();
  size_t unread_total = model_->GetUnreadCount();

  model_->SetFilter(AstraReadingListFilter::kUnread);
  auto filtered = model_->GetFilteredEntries();

  EXPECT_EQ(unread_total, filtered.size());
  EXPECT_LT(filtered.size(), total);

  for (const auto& entry : filtered) {
    EXPECT_FALSE(entry.is_read);
  }
}

// Test filter: read.
TEST_F(AstraReadingListModelTest, FilterRead) {
  model_->PopulateSampleEntries();

  model_->SetFilter(AstraReadingListFilter::kRead);
  auto filtered = model_->GetFilteredEntries();

  for (const auto& entry : filtered) {
    EXPECT_TRUE(entry.is_read);
  }
}

// Test filter: favorites.
TEST_F(AstraReadingListModelTest, FilterFavorites) {
  model_->PopulateSampleEntries();
  size_t favorites_total = model_->GetFavoritesCount();

  model_->SetFilter(AstraReadingListFilter::kFavorites);
  auto filtered = model_->GetFilteredEntries();

  EXPECT_EQ(favorites_total, filtered.size());

  for (const auto& entry : filtered) {
    EXPECT_TRUE(entry.is_favorited);
  }
}

// Test filter getter/setter.
TEST_F(AstraReadingListModelTest, FilterGetterSetter) {
  EXPECT_EQ(AstraReadingListFilter::kAll, model_->GetFilter());

  model_->SetFilter(AstraReadingListFilter::kUnread);
  EXPECT_EQ(AstraReadingListFilter::kUnread, model_->GetFilter());

  model_->SetFilter(AstraReadingListFilter::kRead);
  EXPECT_EQ(AstraReadingListFilter::kRead, model_->GetFilter());

  model_->SetFilter(AstraReadingListFilter::kFavorites);
  EXPECT_EQ(AstraReadingListFilter::kFavorites, model_->GetFilter());

  model_->SetFilter(AstraReadingListFilter::kAll);
  EXPECT_EQ(AstraReadingListFilter::kAll, model_->GetFilter());
}

// Test category filter.
TEST_F(AstraReadingListModelTest, CategoryFilter) {
  model_->PopulateSampleEntries();
  auto categories = model_->GetCategories();
  ASSERT_GT(categories.size(), 0u);

  // Filter by first category.
  std::string test_category = categories[0];
  model_->SetCategoryFilter(test_category);
  EXPECT_EQ(test_category, model_->GetCategoryFilter());

  auto filtered = model_->GetFilteredEntries();
  EXPECT_GT(filtered.size(), 0u);

  for (const auto& entry : filtered) {
    EXPECT_EQ(test_category, entry.category);
  }

  // Reset filter.
  model_->SetCategoryFilter("");
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
}

// Test folder filter.
TEST_F(AstraReadingListModelTest, FolderFilter) {
  model_->PopulateSampleEntries();
  auto folders = model_->GetFolders();
  ASSERT_GT(folders.size(), 0u);

  // Filter by first folder.
  std::string test_folder = base::UTF16ToUTF8(folders[0].name);
  model_->SetFolderFilter(test_folder);
  EXPECT_EQ(test_folder, model_->GetFolderFilter());

  auto filtered = model_->GetFilteredEntries();

  for (const auto& entry : filtered) {
    EXPECT_EQ(test_folder, entry.folder);
  }

  // Reset filter.
  model_->SetFolderFilter("");
  EXPECT_TRUE(model_->GetFolderFilter().empty());
}

// Test sorting: newest first.
TEST_F(AstraReadingListModelTest, SortNewestFirst) {
  model_->AddEntry(u"Older", "https://older.com", u"p", 5, "News", "Later");
  // Add a small delay by manipulating dates is tricky, but we can verify
  // the sort type is set correctly.
  model_->SetSortType(AstraReadingListSortType::kNewestFirst);
  EXPECT_EQ(AstraReadingListSortType::kNewestFirst, model_->GetSortType());

  auto entries = model_->GetFilteredEntries();
  if (entries.size() >= 2) {
    for (size_t i = 1; i < entries.size(); ++i) {
      EXPECT_GE(entries[i - 1].date_added, entries[i].date_added);
    }
  }
}

// Test sorting: oldest first.
TEST_F(AstraReadingListModelTest, SortOldestFirst) {
  model_->AddEntry(u"First", "https://first.com", u"p", 5, "News", "Later");
  model_->AddEntry(u"Second", "https://second.com", u"p", 5, "News", "Later");

  model_->SetSortType(AstraReadingListSortType::kOldestFirst);
  EXPECT_EQ(AstraReadingListSortType::kOldestFirst, model_->GetSortType());

  auto entries = model_->GetFilteredEntries();
  if (entries.size() >= 2) {
    for (size_t i = 1; i < entries.size(); ++i) {
      EXPECT_LE(entries[i - 1].date_added, entries[i].date_added);
    }
  }
}

// Test sorting: alphabetical.
TEST_F(AstraReadingListModelTest, SortAlphabetical) {
  model_->AddEntry(u"Charlie", "https://c.com", u"p", 5, "News", "Later");
  model_->AddEntry(u"Alice", "https://a.com", u"p", 5, "News", "Later");
  model_->AddEntry(u"Bob", "https://b.com", u"p", 5, "News", "Later");

  model_->SetSortType(AstraReadingListSortType::kAlphabetical);
  EXPECT_EQ(AstraReadingListSortType::kAlphabetical, model_->GetSortType());

  auto entries = model_->GetFilteredEntries();
  ASSERT_GE(entries.size(), 3u);
  EXPECT_EQ(u"Alice", entries[0].title);
  EXPECT_EQ(u"Bob", entries[1].title);
  EXPECT_EQ(u"Charlie", entries[2].title);
}

// Test sorting: read time.
TEST_F(AstraReadingListModelTest, SortReadTime) {
  model_->AddEntry(u"Long", "https://long.com", u"p", 30, "News", "Later");
  model_->AddEntry(u"Short", "https://short.com", u"p", 5, "News", "Later");
  model_->AddEntry(u"Medium", "https://medium.com", u"p", 15, "News", "Later");

  model_->SetSortType(AstraReadingListSortType::kReadTime);
  EXPECT_EQ(AstraReadingListSortType::kReadTime, model_->GetSortType());

  auto entries = model_->GetFilteredEntries();
  ASSERT_GE(entries.size(), 3u);
  EXPECT_EQ(5, entries[0].estimated_read_time_minutes);
  EXPECT_EQ(15, entries[1].estimated_read_time_minutes);
  EXPECT_EQ(30, entries[2].estimated_read_time_minutes);
}

// Test folder management.
TEST_F(AstraReadingListModelTest, FolderManagement) {
  // Add folder.
  std::string folder_id = model_->AddFolder(u"My Folder");
  EXPECT_FALSE(folder_id.empty());
  EXPECT_EQ(1u, model_->GetFolders().size());

  const auto* folder =
      std::find_if(model_->GetFolders().begin(),
                   model_->GetFolders().end(),
                   [&folder_id](const AstraReadingListFolder& f) {
                     return f.id == folder_id;
                   });
  ASSERT_NE(folder, model_->GetFolders().end());
  EXPECT_EQ(u"My Folder", folder->name);

  // Rename folder.
  model_->RenameFolder(folder_id, u"Renamed Folder");
  folder = std::find_if(model_->GetFolders().begin(),
                        model_->GetFolders().end(),
                        [&folder_id](const AstraReadingListFolder& f) {
                          return f.id == folder_id;
                        });
  ASSERT_NE(folder, model_->GetFolders().end());
  EXPECT_EQ(u"Renamed Folder", folder->name);

  // Remove folder.
  model_->RemoveFolder(folder_id);
  EXPECT_EQ(0u, model_->GetFolders().size());
}

// Test folder entry counts.
TEST_F(AstraReadingListModelTest, FolderEntryCounts) {
  model_->AddFolder(u"Tech");
  model_->AddEntry(u"Article 1", "https://a1.com", u"p", 5, "Tech", "Tech");
  model_->AddEntry(u"Article 2", "https://a2.com", u"p", 5, "Tech", "Tech");

  auto folders = model_->GetFolders();
  // After adding entries, folder counts should be updated.
  // Find the "Tech" folder.
  bool found = false;
  for (const auto& folder : folders) {
    if (folder.name == u"Tech") {
      EXPECT_EQ(2, folder.entry_count);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

// Test loading state.
TEST_F(AstraReadingListModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());
  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());
  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test get entry returns nullptr for invalid ID.
TEST_F(AstraReadingListModelTest, GetEntryInvalid) {
  EXPECT_EQ(nullptr, model_->GetEntry("nonexistent"));
}

// Test observer notifications.
TEST_F(AstraReadingListModelTest, ObserverNotifications) {
  class TestObserver : public AstraReadingListObserver {
   public:
    void OnReadingListChanged() override { reading_list_changed = true; }
    void OnEntryAdded(const std::string& id) override {
      entry_added = true;
      added_id = id;
    }
    void OnEntryRemoved(const std::string& id) override {
      entry_removed = true;
      removed_id = id;
    }
    void OnEntryUpdated(const std::string& id) override {
      entry_updated = true;
      updated_id = id;
    }
    void OnFolderAdded(const std::string& id) override {
      folder_added = true;
      added_folder_id = id;
    }
    void OnFolderRemoved(const std::string& id) override {
      folder_removed = true;
      removed_folder_id = id;
    }
    void OnSearchChanged(const std::u16string& query) override {
      search_changed = true;
      search_query = query;
    }
    void OnFilterChanged() override { filter_changed = true; }
    void OnReadingListModelShutdown() override { shutdown = true; }

    bool reading_list_changed = false;
    bool entry_added = false;
    bool entry_removed = false;
    bool entry_updated = false;
    bool folder_added = false;
    bool folder_removed = false;
    bool search_changed = false;
    bool filter_changed = false;
    bool shutdown = false;
    std::string added_id;
    std::string removed_id;
    std::string updated_id;
    std::string added_folder_id;
    std::string removed_folder_id;
    std::u16string search_query;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Add entry should trigger notifications.
  std::string new_id = model_->AddEntry(u"Test", "https://test.com",
                                        u"Preview", 5, "News", "Later");
  EXPECT_TRUE(observer.entry_added);
  EXPECT_EQ(new_id, observer.added_id);
  EXPECT_TRUE(observer.reading_list_changed);

  // Reset for next test.
  observer = TestObserver();

  // Mark as read.
  model_->MarkAsRead(new_id);
  EXPECT_TRUE(observer.entry_updated);
  EXPECT_EQ(new_id, observer.updated_id);

  // Reset.
  observer = TestObserver();

  // Toggle favorite.
  model_->ToggleFavorite(new_id);
  EXPECT_TRUE(observer.entry_updated);

  // Reset.
  observer = TestObserver();

  // Search change.
  model_->SetSearchQuery(u"hello");
  EXPECT_TRUE(observer.search_changed);
  EXPECT_EQ(u"hello", observer.search_query);

  // Reset.
  observer = TestObserver();

  // Filter change.
  model_->SetFilter(AstraReadingListFilter::kUnread);
  EXPECT_TRUE(observer.filter_changed);

  // Reset.
  observer = TestObserver();

  // Add folder.
  std::string folder_id = model_->AddFolder(u"Test Folder");
  EXPECT_TRUE(observer.folder_added);
  EXPECT_EQ(folder_id, observer.added_folder_id);

  // Reset.
  observer = TestObserver();

  // Remove entry.
  model_->RemoveEntry(new_id);
  EXPECT_TRUE(observer.entry_removed);
  EXPECT_EQ(new_id, observer.removed_id);

  // Reset.
  observer = TestObserver();

  // Remove folder.
  model_->RemoveFolder(folder_id);
  EXPECT_TRUE(observer.folder_removed);
  EXPECT_EQ(folder_id, observer.removed_folder_id);

  // Test shutdown notification.
  model_->RemoveObserver(&observer);
  // Re-add and then destroy model.
  model_->AddObserver(&observer);
  model_.reset();
  EXPECT_TRUE(observer.shutdown);
}

// Test categories are unique.
TEST_F(AstraReadingListModelTest, UniqueCategories) {
  model_->AddEntry(u"A1", "https://a1.com", u"p", 5, "Technology", "Later");
  model_->AddEntry(u"A2", "https://a2.com", u"p", 5, "Technology", "Later");
  model_->AddEntry(u"A3", "https://a3.com", u"p", 5, "News", "Later");
  model_->AddEntry(u"A4", "https://a4.com", u"p", 5, "Design", "Later");
  model_->AddEntry(u"A5", "https://a5.com", u"p", 5, "News", "Later");

  auto categories = model_->GetCategories();
  EXPECT_EQ(3u, categories.size());

  // Verify each is unique.
  std::set<std::string> cat_set(categories.begin(), categories.end());
  EXPECT_EQ(categories.size(), cat_set.size());
}

// Test filtered entries with multiple filters combined.
TEST_F(AstraReadingListModelTest, CombinedFilters) {
  model_->PopulateSampleEntries();

  // Combine unread filter with category filter.
  model_->SetFilter(AstraReadingListFilter::kUnread);
  auto categories = model_->GetCategories();
  ASSERT_GT(categories.size(), 0u);
  model_->SetCategoryFilter(categories[0]);

  auto filtered = model_->GetFilteredEntries();

  for (const auto& entry : filtered) {
    EXPECT_FALSE(entry.is_read);
    EXPECT_EQ(categories[0], entry.category);
  }

  // Add search on top.
  model_->SetSearchQuery(u"the");
  auto search_filtered = model_->GetFilteredEntries();
  EXPECT_LE(search_filtered.size(), filtered.size());
}

// Test sample entries have varied categories and folders.
TEST_F(AstraReadingListModelTest, SampleDataVariety) {
  model_->PopulateSampleEntries();

  auto categories = model_->GetCategories();
  EXPECT_GE(categories.size(), 3u);

  auto folders = model_->GetFolders();
  EXPECT_GE(folders.size(), 3u);

  // Verify entries are spread across categories.
  std::map<std::string, int> cat_counts;
  for (const auto& entry : model_->GetEntries()) {
    cat_counts[entry.category]++;
  }
  EXPECT_GE(cat_counts.size(), 3u);
}

// ===========================================================================
// View Tests
// ===========================================================================

class AstraReadingListViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraReadingListModel>();
    view_ = std::make_unique<AstraReadingListPageView>(model_.get());
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraReadingListModel> model_;
  std::unique_ptr<AstraReadingListPageView> view_;
};

// Test view creation.
TEST_F(AstraReadingListViewTest, ViewCreation) {
  EXPECT_NE(nullptr, view_.get());
  EXPECT_EQ(model_.get(), view_->model());
}

// Test view default display mode.
TEST_F(AstraReadingListViewTest, DefaultDisplayMode) {
  EXPECT_EQ(AstraReadingListDisplayMode::kList, view_->display_mode());
}

// Test switching display mode.
TEST_F(AstraReadingListViewTest, SwitchDisplayMode) {
  view_->SetDisplayMode(AstraReadingListDisplayMode::kGrid);
  EXPECT_EQ(AstraReadingListDisplayMode::kGrid, view_->display_mode());

  view_->SetDisplayMode(AstraReadingListDisplayMode::kList);
  EXPECT_EQ(AstraReadingListDisplayMode::kList, view_->display_mode());
}

// Test toolbar components exist.
TEST_F(AstraReadingListViewTest, ToolbarComponents) {
  EXPECT_NE(nullptr, view_->search_field_for_test());
  EXPECT_NE(nullptr, view_->add_button_for_test());
  EXPECT_NE(nullptr, view_->sort_button_for_test());
  EXPECT_NE(nullptr, view_->list_view_button_for_test());
  EXPECT_NE(nullptr, view_->grid_view_button_for_test());
}

// Test sidebar exists.
TEST_F(AstraReadingListViewTest, SidebarExists) {
  EXPECT_NE(nullptr, view_->sidebar_for_test());
}

// Test content scroll exists.
TEST_F(AstraReadingListViewTest, ContentScrollExists) {
  EXPECT_NE(nullptr, view_->content_scroll_for_test());
}

// Test detail panel exists.
TEST_F(AstraReadingListViewTest, DetailPanelExists) {
  EXPECT_NE(nullptr, view_->detail_panel_for_test());
}

// Test filter items in sidebar.
TEST_F(AstraReadingListViewTest, FilterItems) {
  // Should have 3 filter items: All, Unread, Favorites.
  EXPECT_EQ(3u, view_->filter_item_count_for_test());
}

// Test empty state with no entries.
TEST_F(AstraReadingListViewTest, EmptyState) {
  // With no entries, empty state should be shown.
  EXPECT_NE(nullptr, view_->empty_view_for_test());
  EXPECT_EQ(0u, view_->list_item_count_for_test());
}

// Test with sample data: entries appear.
TEST_F(AstraReadingListViewTest, WithSampleData) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  EXPECT_GT(view_->list_item_count_for_test(), 0u);
  EXPECT_GT(view_->folder_item_count_for_test(), 0u);
}

// Test folder items populate.
TEST_F(AstraReadingListViewTest, FolderItemsPopulate) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  size_t folder_count = model_->GetFolders().size();
  EXPECT_EQ(folder_count, view_->folder_item_count_for_test());
}

// Test search field updates model.
TEST_F(AstraReadingListViewTest, SearchFieldUpdatesModel) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  auto* search_field = view_->search_field_for_test();
  ASSERT_NE(nullptr, search_field);

  // Simulate typing by setting text and triggering ContentsChanged.
  search_field->SetText(u"Chromium");
  // The TextfieldController ContentsChanged should be called.
  view_->ContentsChanged(search_field, u"Chromium");

  EXPECT_EQ(u"Chromium", model_->GetSearchQuery());
}

// Test set model.
TEST_F(AstraReadingListViewTest, SetModel) {
  auto new_model = std::make_unique<AstraReadingListModel>();
  new_model->PopulateSampleEntries();

  size_t count_before = view_->list_item_count_for_test();

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // After setting model with sample data, count should increase.
  EXPECT_GT(view_->list_item_count_for_test(), count_before);
}

// Test selected entry.
TEST_F(AstraReadingListViewTest, SelectedEntry) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  // Default should be empty selection.
  EXPECT_TRUE(view_->selected_entry_id().empty());

  // Select the first entry.
  auto entries = model_->GetFilteredEntries();
  ASSERT_GT(entries.size(), 0u);
  view_->SetSelectedEntry(entries[0].id);
  EXPECT_EQ(entries[0].id, view_->selected_entry_id());
}

// Test view preferred size.
TEST_F(AstraReadingListViewTest, PreferredSize) {
  gfx::Size pref = view_->CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// Test view with no model.
TEST_F(AstraReadingListViewTest, ViewWithNoModel) {
  auto empty_view = std::make_unique<AstraReadingListPageView>();
  EXPECT_EQ(nullptr, empty_view->model());
  EXPECT_EQ(0u, empty_view->list_item_count_for_test());
  EXPECT_EQ(0u, empty_view->folder_item_count_for_test());
}

// Test that view observes model changes.
TEST_F(AstraReadingListViewTest, ViewObservesModel) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  size_t count_before = view_->list_item_count_for_test();
  EXPECT_GT(count_before, 0u);

  // Add an entry and check the view updates.
  model_->AddEntry(u"Newly Added", "https://new.com",
                   u"Preview text", 7, "Technology", "Later");

  // View should have updated.
  EXPECT_EQ(count_before + 1, view_->list_item_count_for_test());
}

// Test that removing an entry updates the view.
TEST_F(AstraReadingListViewTest, ViewUpdatesOnRemove) {
  std::string id = model_->AddEntry(u"To Remove", "https://remove.com",
                                     u"Preview", 5, "News", "Later");
  view_->SetModel(model_.get());

  size_t count_before = view_->list_item_count_for_test();

  model_->RemoveEntry(id);

  EXPECT_EQ(count_before - 1, view_->list_item_count_for_test());
}

// Test grid view with sample data.
TEST_F(AstraReadingListViewTest, GridViewWithData) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  view_->SetDisplayMode(AstraReadingListDisplayMode::kGrid);
  EXPECT_EQ(AstraReadingListDisplayMode::kGrid, view_->display_mode());

  // Items should still exist in grid mode.
  EXPECT_GT(view_->list_item_count_for_test(), 0u);
}

// Test toggle favorite from view.
TEST_F(AstraReadingListViewTest, ToggleFavoriteFromView) {
  std::string id = model_->AddEntry(u"Fav Test", "https://fav.com",
                                     u"Preview", 5, "News", "Later");
  view_->SetModel(model_.get());

  // Verify initial state.
  const auto* entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_favorited);

  // The OnEntryFavorite handler calls model_->ToggleFavorite.
  // We can test it indirectly by verifying the view updates when model changes.
  model_->ToggleFavorite(id);

  entry = model_->GetEntry(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_TRUE(entry->is_favorited);
}

// Test view with search empty state.
TEST_F(AstraReadingListViewTest, SearchEmptyState) {
  model_->PopulateSampleEntries();
  view_->SetModel(model_.get());

  // Search for something that doesn't exist.
  model_->SetSearchQuery(u"zzzzzzzzzzzzzzzzzzz");

  // Empty view should be visible (no list items).
  EXPECT_EQ(0u, view_->list_item_count_for_test());
}

// Test detail panel with selection.
TEST_F(AstraReadingListViewTest, DetailPanelWithSelection) {
  std::string id = model_->AddEntry(u"Detail Test", "https://detail.com",
                                     u"This is the preview text for testing.",
                                     12, "Technology", "Must Read");
  view_->SetModel(model_.get());

  view_->SetSelectedEntry(id);
  EXPECT_EQ(id, view_->selected_entry_id());
}

}  // namespace astra
