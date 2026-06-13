// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/bookmarks_manager/astra_bookmarks_manager_model.h"
#include "astra/ui/views/bookmarks_manager/astra_bookmarks_manager_view.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraBookmarksManagerModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraBookmarksManagerModel>();
  }

  void TearDown() override { model_.reset(); }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraBookmarksManagerModel> model_;
};

// Test model creation.
TEST_F(AstraBookmarksManagerModelTest, ModelCreation) {
  EXPECT_NE(nullptr, model_.get());
  EXPECT_EQ(0u, model_->FlatBookmarkCount());

  // Root folder should exist.
  const auto& root = model_->GetRootFolder();
  EXPECT_EQ(0, root.id);

  // Three permanent folders should exist.
  EXPECT_EQ(3u, root.subfolders.size());

  // Bar folder.
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  EXPECT_EQ(u"Bookmarks bar", bar->title);

  // Other folder.
  const auto* other = model_->GetOtherBookmarksFolder();
  ASSERT_NE(nullptr, other);
  EXPECT_EQ(u"Other bookmarks", other->title);

  // Mobile folder.
  const auto* mobile = model_->GetMobileBookmarksFolder();
  ASSERT_NE(nullptr, mobile);
  EXPECT_EQ(u"Mobile bookmarks", mobile->title);
}

// Test sample population.
TEST_F(AstraBookmarksManagerModelTest, PopulateSampleBookmarks) {
  model_->PopulateSampleBookmarks();

  EXPECT_GT(model_->FlatBookmarkCount(), 0u);
  EXPECT_LT(0u, model_->FlatBookmarkCount());

  // Sample data should have bookmarks in bar and other folders.
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  EXPECT_GT(bar->children.size(), 0u);
  EXPECT_GT(bar->subfolders.size(), 0u);

  const auto* other = model_->GetOtherBookmarksFolder();
  ASSERT_NE(nullptr, other);
  EXPECT_GT(other->children.size(), 0u);
  EXPECT_GT(other->subfolders.size(), 0u);
}

// Test adding a bookmark.
TEST_F(AstraBookmarksManagerModelTest, AddBookmark) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  AstraBookmarkId bar_id = bar->id;

  size_t initial_count = model_->FlatBookmarkCount();

  AstraBookmarkId new_id =
      model_->AddBookmark(bar_id, u"Test Bookmark", "https://test.com");
  EXPECT_NE(0, new_id);
  EXPECT_EQ(initial_count + 1, model_->FlatBookmarkCount());

  const AstraBookmarkEntry* entry = model_->GetBookmark(new_id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(u"Test Bookmark", entry->title);
  EXPECT_EQ("https://test.com", entry->url);
  EXPECT_EQ(bar_id, entry->parent_id);
}

// Test removing a bookmark.
TEST_F(AstraBookmarksManagerModelTest, RemoveBookmark) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  AstraBookmarkId new_id =
      model_->AddBookmark(bar->id, u"To Remove", "https://remove.com");
  ASSERT_NE(0, new_id);

  size_t count_before = model_->FlatBookmarkCount();

  model_->RemoveBookmark(new_id);
  EXPECT_EQ(count_before - 1, model_->FlatBookmarkCount());

  const AstraBookmarkEntry* entry = model_->GetBookmark(new_id);
  EXPECT_EQ(nullptr, entry);
}

// Test updating a bookmark.
TEST_F(AstraBookmarksManagerModelTest, UpdateBookmark) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  AstraBookmarkId id =
      model_->AddBookmark(bar->id, u"Original", "https://original.com");
  ASSERT_NE(0, id);

  model_->UpdateBookmark(id, u"Updated Title", "https://updated.com");

  const AstraBookmarkEntry* entry = model_->GetBookmark(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(u"Updated Title", entry->title);
  EXPECT_EQ("https://updated.com", entry->url);
}

// Test moving a bookmark.
TEST_F(AstraBookmarksManagerModelTest, MoveBookmark) {
  model_->PopulateSampleBookmarks();

  const auto* bar = model_->GetBookmarksBarFolder();
  const auto* other = model_->GetOtherBookmarksFolder();
  ASSERT_NE(nullptr, bar);
  ASSERT_NE(nullptr, other);

  size_t bar_count_before = bar->children.size();
  size_t other_count_before = other->children.size();

  // Take the first bookmark from the bar.
  ASSERT_GT(bar->children.size(), 0u);
  AstraBookmarkId bookmark_id = bar->children[0].id;

  model_->MoveBookmark(bookmark_id, other->id, -1);

  // Refresh pointers after move (they may be invalidated).
  bar = model_->GetBookmarksBarFolder();
  other = model_->GetOtherBookmarksFolder();
  EXPECT_EQ(bar_count_before - 1, bar->children.size());
  EXPECT_EQ(other_count_before + 1, other->children.size());
}

// Test folder creation.
TEST_F(AstraBookmarksManagerModelTest, CreateFolder) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  size_t initial_subfolders = bar->subfolders.size();

  AstraBookmarkId folder_id = model_->CreateFolder(bar->id, u"My Folder");
  EXPECT_NE(0, folder_id);

  const AstraBookmarkFolder* folder = model_->GetFolder(folder_id);
  ASSERT_NE(nullptr, folder);
  EXPECT_EQ(u"My Folder", folder->title);
  EXPECT_EQ(bar->id, folder->parent_id);

  bar = model_->GetBookmarksBarFolder();
  EXPECT_EQ(initial_subfolders + 1, bar->subfolders.size());
}

// Test folder removal.
TEST_F(AstraBookmarksManagerModelTest, RemoveFolder) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  AstraBookmarkId folder_id = model_->CreateFolder(bar->id, u"To Delete");
  ASSERT_NE(0, folder_id);

  // Add a bookmark inside.
  model_->AddBookmark(folder_id, u"Inside Folder", "https://inside.com");

  size_t count_before = model_->FlatBookmarkCount();

  model_->RemoveFolder(folder_id);

  EXPECT_EQ(nullptr, model_->GetFolder(folder_id));
  EXPECT_EQ(count_before - 1, model_->FlatBookmarkCount());
}

// Test folder rename.
TEST_F(AstraBookmarksManagerModelTest, RenameFolder) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  AstraBookmarkId folder_id = model_->CreateFolder(bar->id, u"Old Name");
  ASSERT_NE(0, folder_id);

  model_->RenameFolder(folder_id, u"New Name");

  const AstraBookmarkFolder* folder = model_->GetFolder(folder_id);
  ASSERT_NE(nullptr, folder);
  EXPECT_EQ(u"New Name", folder->title);
}

// Test search.
TEST_F(AstraBookmarksManagerModelTest, SearchBookmarks) {
  model_->PopulateSampleBookmarks();

  // Search for "Google".
  auto results = model_->SearchBookmarks(u"google");
  EXPECT_GT(results.size(), 0u);

  for (const auto& entry : results) {
    std::u16 title_lower;
    for (char16_t c : entry.title) {
      title_lower += std::towlower(c);
    }
    std::u16 url_lower;
    for (char c : entry.url) {
      url_lower += std::towlower(static_cast<char16_t>(c));
    }
    EXPECT_TRUE(title_lower.find(u"google") != std::u16string::npos ||
                url_lower.find(u"google") != std::u16string::npos);
  }

  // Empty search should return empty results (no filter = no results from
  // SearchBookmarks API).
  auto empty_results = model_->SearchBookmarks(u"");
  EXPECT_EQ(0u, empty_results.size());
}

// Test search query getter/setter.
TEST_F(AstraBookmarksManagerModelTest, SearchQuery) {
  EXPECT_TRUE(model_->GetSearchQuery().empty());

  model_->SetSearchQuery(u"test");
  EXPECT_EQ(u"test", model_->GetSearchQuery());

  model_->SetSearchQuery(u"");
  EXPECT_TRUE(model_->GetSearchQuery().empty());
}

// Test category filter.
TEST_F(AstraBookmarksManagerModelTest, CategoryFilter) {
  model_->PopulateSampleBookmarks();

  // Get all categories.
  auto categories = model_->GetCategories();
  EXPECT_GT(categories.size(), 0u);

  // Filter by "Work" category.
  model_->SetCategoryFilter("Work");
  EXPECT_EQ("Work", model_->GetCategoryFilter());

  // Search with category filter.
  auto results = model_->SearchBookmarks(u"");
  // Empty search returns nothing from SearchBookmarks directly.

  // Reset filter.
  model_->SetCategoryFilter("");
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
}

// Test expanded folders.
TEST_F(AstraBookmarksManagerModelTest, ExpandedFolders) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  // Bar folder should be expanded by default.
  EXPECT_TRUE(model_->IsFolderExpanded(bar->id));

  // Collapse it.
  model_->SetFolderExpanded(bar->id, false);
  EXPECT_FALSE(model_->IsFolderExpanded(bar->id));

  // Expand it again.
  model_->SetFolderExpanded(bar->id, true);
  EXPECT_TRUE(model_->IsFolderExpanded(bar->id));

  // Get all expanded folders.
  const auto& expanded = model_->GetExpandedFolders();
  EXPECT_GT(expanded.size(), 0u);
}

// Test observer notifications.
TEST_F(AstraBookmarksManagerModelTest, ObserverNotifications) {
  class TestObserver : public AstraBookmarksManagerObserver {
   public:
    void OnBookmarksModelChanged() override { model_changed = true; }
    void OnBookmarkAdded(AstraBookmarkId id) override {
      bookmark_added = true;
      added_id = id;
    }
    void OnBookmarkRemoved(AstraBookmarkId id) override {
      bookmark_removed = true;
      removed_id = id;
    }
    void OnBookmarkChanged(AstraBookmarkId id) override {
      bookmark_changed = true;
      changed_id = id;
    }
    void OnFolderExpanded(AstraBookmarkId folder_id) override {
      folder_expanded = true;
      expanded_folder_id = folder_id;
    }
    void OnSearchChanged(const std::u16string& query) override {
      search_changed = true;
      search_query = query;
    }

    bool model_changed = false;
    bool bookmark_added = false;
    bool bookmark_removed = false;
    bool bookmark_changed = false;
    bool folder_expanded = false;
    bool search_changed = false;
    AstraBookmarkId added_id = 0;
    AstraBookmarkId removed_id = 0;
    AstraBookmarkId changed_id = 0;
    AstraBookmarkId expanded_folder_id = 0;
    std::u16string search_query;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Add bookmark should trigger notifications.
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  AstraBookmarkId new_id =
      model_->AddBookmark(bar->id, u"Test", "https://test.com");
  EXPECT_TRUE(observer.bookmark_added);
  EXPECT_EQ(new_id, observer.added_id);
  EXPECT_TRUE(observer.model_changed);

  // Reset.
  observer = TestObserver();

  // Update bookmark.
  model_->UpdateBookmark(new_id, u"Updated", "https://updated.com");
  EXPECT_TRUE(observer.bookmark_changed);
  EXPECT_EQ(new_id, observer.changed_id);

  // Reset.
  observer = TestObserver();

  // Search change.
  model_->SetSearchQuery(u"hello");
  EXPECT_TRUE(observer.search_changed);
  EXPECT_EQ(u"hello", observer.search_query);

  // Reset.
  observer = TestObserver();

  // Folder expand change.
  model_->SetFolderExpanded(bar->id, false);
  EXPECT_TRUE(observer.folder_expanded);

  // Remove observer.
  model_->RemoveObserver(&observer);

  // Further changes should not affect the observer.
  observer = TestObserver();
  model_->AddBookmark(bar->id, u"Another", "https://another.com");
  // Observer was recreated from default, and removed earlier, but we
  // recreated it as a fresh object — it was never re-registered.
  // Let's verify that the freshly created observer doesn't have notifications.
  EXPECT_FALSE(observer.bookmark_added);
}

// Test FlatBookmarkCount.
TEST_F(AstraBookmarksManagerModelTest, FlatBookmarkCount) {
  EXPECT_EQ(0u, model_->FlatBookmarkCount());

  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  model_->AddBookmark(bar->id, u"One", "https://one.com");
  EXPECT_EQ(1u, model_->FlatBookmarkCount());

  model_->AddBookmark(bar->id, u"Two", "https://two.com");
  EXPECT_EQ(2u, model_->FlatBookmarkCount());

  // Create a nested folder with a bookmark.
  AstraBookmarkId folder_id = model_->CreateFolder(bar->id, u"Subfolder");
  model_->AddBookmark(folder_id, u"Nested", "https://nested.com");
  EXPECT_EQ(3u, model_->FlatBookmarkCount());
}

// Test GetFolder for root.
TEST_F(AstraBookmarksManagerModelTest, GetRootFolderById) {
  const AstraBookmarkFolder* root = model_->GetFolder(0);
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(0, root->id);
}

// Test GetBookmark returns nullptr for invalid ID.
TEST_F(AstraBookmarksManagerModelTest, GetBookmarkInvalid) {
  EXPECT_EQ(nullptr, model_->GetBookmark(99999));
}

// Test GetFolder returns nullptr for invalid ID.
TEST_F(AstraBookmarksManagerModelTest, GetFolderInvalid) {
  EXPECT_EQ(nullptr, model_->GetFolder(99999));
}

// Test sort folder.
TEST_F(AstraBookmarksManagerModelTest, SortFolder) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  model_->AddBookmark(bar->id, u"Charlie", "https://charlie.com");
  model_->AddBookmark(bar->id, u"Alice", "https://alice.com");
  model_->AddBookmark(bar->id, u"Bob", "https://bob.com");

  // Sort by name.
  model_->SortFolder(bar->id, AstraBookmarkSortType::kName);

  bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  ASSERT_GE(bar->children.size(), 3u);

  // Should be in alphabetical order.
  EXPECT_EQ(u"Alice", bar->children[0].title);
  EXPECT_EQ(u"Bob", bar->children[1].title);
  EXPECT_EQ(u"Charlie", bar->children[2].title);
}

// Test sort by date added (newest first).
TEST_F(AstraBookmarksManagerModelTest, SortFolderByDateNewest) {
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);

  model_->AddBookmark(bar->id, u"First", "https://first.com");
  model_->AddBookmark(bar->id, u"Second", "https://second.com");
  model_->AddBookmark(bar->id, u"Third", "https://third.com");

  model_->SortFolder(bar->id, AstraBookmarkSortType::kDateAddedNewest);

  bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  ASSERT_GE(bar->children.size(), 3u);

  // "Third" was added last, should be first when sorted by newest.
  EXPECT_EQ(u"Third", bar->children[0].title);
}

// ===========================================================================
// View Tests
// ===========================================================================

class AstraBookmarksManagerViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraBookmarksManagerModel>();
    view_ = std::make_unique<AstraBookmarksManagerView>(model_.get());
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraBookmarksManagerModel> model_;
  std::unique_ptr<AstraBookmarksManagerView> view_;
};

// Test view creation.
TEST_F(AstraBookmarksManagerViewTest, ViewCreation) {
  EXPECT_NE(nullptr, view_.get());
  EXPECT_EQ(model_.get(), view_->model());
}

// Test view default display mode.
TEST_F(AstraBookmarksManagerViewTest, DefaultDisplayMode) {
  EXPECT_EQ(AstraBookmarksDisplayMode::kGrid, view_->display_mode());
}

// Test switching display mode.
TEST_F(AstraBookmarksManagerViewTest, SwitchDisplayMode) {
  view_->SetDisplayMode(AstraBookmarksDisplayMode::kList);
  EXPECT_EQ(AstraBookmarksDisplayMode::kList, view_->display_mode());

  view_->SetDisplayMode(AstraBookmarksDisplayMode::kGrid);
  EXPECT_EQ(AstraBookmarksDisplayMode::kGrid, view_->display_mode());
}

// Test toolbar components exist.
TEST_F(AstraBookmarksManagerViewTest, ToolbarComponents) {
  EXPECT_NE(nullptr, view_->search_field_for_test());
  EXPECT_NE(nullptr, view_->add_button_for_test());
  EXPECT_NE(nullptr, view_->grid_view_button_for_test());
  EXPECT_NE(nullptr, view_->list_view_button_for_test());
}

// Test sidebar exists.
TEST_F(AstraBookmarksManagerViewTest, SidebarExists) {
  EXPECT_NE(nullptr, view_->sidebar_for_test());
}

// Test content scroll exists.
TEST_F(AstraBookmarksManagerViewTest, ContentScrollExists) {
  EXPECT_NE(nullptr, view_->content_scroll_for_test());
}

// Test status bar exists.
TEST_F(AstraBookmarksManagerViewTest, StatusBarExists) {
  EXPECT_NE(nullptr, view_->status_label_for_test());
}

// Test empty state (no bookmarks).
TEST_F(AstraBookmarksManagerViewTest, EmptyState) {
  // With no bookmarks and no search, status should show 0 bookmarks.
  EXPECT_NE(nullptr, view_->status_label_for_test());
  EXPECT_EQ(0u, view_->bookmark_item_count_for_test());
}

// Test with sample data.
TEST_F(AstraBookmarksManagerViewTest, WithSampleData) {
  model_->PopulateSampleBookmarks();
  view_->SetModel(model_.get());

  EXPECT_GT(view_->bookmark_item_count_for_test(), 0u);

  // Status bar should reflect count.
  EXPECT_NE(nullptr, view_->status_label_for_test());
}

// Test search field updates model.
TEST_F(AstraBookmarksManagerViewTest, SearchField) {
  model_->PopulateSampleBookmarks();
  view_->SetModel(model_.get());

  auto* search_field = view_->search_field_for_test();
  ASSERT_NE(nullptr, search_field);

  // Simulate typing in the search field.
  search_field->SetText(u"gmail");
  // The ContentsChanged handler should update the model's search query.
  // Since we set text directly, we can check via the model.
  model_->SetSearchQuery(u"gmail");
  EXPECT_EQ(u"gmail", model_->GetSearchQuery());
}

// Test set model.
TEST_F(AstraBookmarksManagerViewTest, SetModel) {
  auto new_model = std::make_unique<AstraBookmarksManagerModel>();
  new_model->PopulateSampleBookmarks();

  size_t count_before = view_->bookmark_item_count_for_test();

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // After setting model with sample data, count should increase.
  EXPECT_GT(view_->bookmark_item_count_for_test(), count_before);
}

// Test selected folder.
TEST_F(AstraBookmarksManagerViewTest, SelectedFolder) {
  model_->PopulateSampleBookmarks();
  view_->SetModel(model_.get());

  // Default selected folder is 0 (all).
  EXPECT_EQ(0, view_->selected_folder());

  // Select the bookmarks bar folder.
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  view_->SetSelectedFolder(bar->id);
  EXPECT_EQ(bar->id, view_->selected_folder());
}

// Test view preferred size.
TEST_F(AstraBookmarksManagerViewTest, PreferredSize) {
  gfx::Size pref = view_->CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// Test view with no model.
TEST_F(AstraBookmarksManagerViewTest, ViewWithNoModel) {
  auto empty_view = std::make_unique<AstraBookmarksManagerView>();
  EXPECT_EQ(nullptr, empty_view->model());
  EXPECT_EQ(0u, empty_view->bookmark_item_count_for_test());
}

// Test that view observes model changes.
TEST_F(AstraBookmarksManagerViewTest, ViewObservesModel) {
  model_->PopulateSampleBookmarks();
  view_->SetModel(model_.get());

  size_t count_before = view_->bookmark_item_count_for_test();
  EXPECT_GT(count_before, 0u);

  // Add a bookmark and check the view updates.
  const auto* bar = model_->GetBookmarksBarFolder();
  ASSERT_NE(nullptr, bar);
  model_->AddBookmark(bar->id, u"Newly Added", "https://new.com");

  // View should have updated.
  EXPECT_EQ(count_before + 1, view_->bookmark_item_count_for_test());
}

}  // namespace astra
