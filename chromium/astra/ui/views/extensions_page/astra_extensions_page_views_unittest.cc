// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_page/astra_extensions_page_model.h"
#include "astra/ui/views/extensions_page/astra_extensions_page_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraExtensionsPageModelTest
// ===========================================================================

class AstraExtensionsPageModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraExtensionsPageModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraExtensionsPageModel> model_;
};

// Test model creation.
TEST_F(AstraExtensionsPageModelTest, Creation) {
  EXPECT_EQ(0u, model_->GetTotalCount());
  EXPECT_EQ(0u, model_->GetEnabledCount());
  EXPECT_TRUE(model_->GetAllExtensions().empty());
  EXPECT_EQ(AstraExtensionFilter::kAll, model_->GetFilter());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
  EXPECT_FALSE(model_->IsLoading());
}

// Test populate sample extensions.
TEST_F(AstraExtensionsPageModelTest, PopulateSampleExtensions) {
  model_->PopulateSampleExtensions();
  EXPECT_GT(model_->GetTotalCount(), 15u);
  EXPECT_GT(model_->GetEnabledCount(), 0u);
  EXPECT_LT(model_->GetEnabledCount(), model_->GetTotalCount());
}

// Test get extension by ID.
TEST_F(AstraExtensionsPageModelTest, GetExtension) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  const auto& first = all[0];
  const auto* found = model_->GetExtension(first.id);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(first.id, found->id);
  EXPECT_EQ(first.name, found->name);

  // Non-existent ID.
  EXPECT_EQ(nullptr, model_->GetExtension("nonexistent"));
}

// Test search filtering.
TEST_F(AstraExtensionsPageModelTest, FilterEnabled) {
  model_->PopulateSampleExtensions();
  size_t total = model_->GetTotalCount();
  size_t enabled = model_->GetEnabledCount();

  model_->SetFilter(AstraExtensionFilter::kEnabled);
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(enabled, filtered.size());

  model_->SetFilter(AstraExtensionFilter::kDisabled);
  filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(total - enabled, filtered.size());
}

// Test themes filter.
TEST_F(AstraExtensionsPageModelTest, FilterThemes) {
  model_->PopulateSampleExtensions();
  model_->SetFilter(AstraExtensionFilter::kThemes);
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_GT(filtered.size(), 0u);
  for (const auto& ext : filtered) {
    EXPECT_EQ(AstraExtensionType::kTheme, ext.type);
  }
}

// Test apps filter.
TEST_F(AstraExtensionsPageModelTest, FilterApps) {
  model_->PopulateSampleExtensions();
  model_->SetFilter(AstraExtensionFilter::kApps);
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_GT(filtered.size(), 0u);
  for (const auto& ext : filtered) {
    EXPECT_TRUE(ext.type == AstraExtensionType::kHostedApp ||
                 ext.type == AstraExtensionType::kPackagedApp);
  }
}

// Test pinned filter.
TEST_F(AstraExtensionsPageModelTest, FilterPinned) {
  model_->PopulateSampleExtensions();
  model_->SetFilter(AstraExtensionFilter::kPinned);
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_GT(filtered.size(), 0u);
  for (const auto& ext : filtered) {
    EXPECT_TRUE(ext.is_pinned);
  }
}

// Test sidebar filter.
TEST_F(AstraExtensionsPageModelTest, FilterSidebar) {
  model_->PopulateSampleExtensions();
  model_->SetFilter(AstraExtensionFilter::kSidebar);
  auto filtered = model_->GetFilteredExtensions();
  for (const auto& ext : filtered) {
    EXPECT_TRUE(ext.is_in_sidebar);
  }
}

// Test search.
TEST_F(AstraExtensionsPageModelTest, Search) {
  model_->PopulateSampleExtensions();
  size_t total = model_->GetTotalCount();

  // Search for "Chrome" — should match something.
  model_->SetSearchQuery(u"Chrome");
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_LT(filtered.size(), total);

  // Search for something that should match everything.
  model_->SetSearchQuery(u"xyz123nonexistent");
  filtered = model_->GetFilteredExtensions();
  EXPECT_TRUE(filtered.empty());

  // Reset.
  model_->SetSearchQuery(std::u16string());
  filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(total, filtered.size());
}

// Test category filter.
TEST_F(AstraExtensionsPageModelTest, CategoryFilter) {
  model_->PopulateSampleExtensions();
  auto categories = model_->GetCategories();
  EXPECT_FALSE(categories.empty());

  // Filter by first category.
  model_->SetCategoryFilter(categories[0].id);
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(categories[0].count, filtered.size());

  // Reset.
  model_->SetCategoryFilter(std::string());
  EXPECT_EQ(model_->GetTotalCount(),
            model_->GetFilteredExtensions().size());
}

// Test enable/disable extension.
TEST_F(AstraExtensionsPageModelTest, EnableDisable) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  bool was_enabled = all[0].is_enabled;

  model_->ToggleExtensionEnabled(id);
  const auto* ext = model_->GetExtension(id);
  EXPECT_NE(was_enabled, ext->is_enabled);

  model_->ToggleExtensionEnabled(id);
  ext = model_->GetExtension(id);
  EXPECT_EQ(was_enabled, ext->is_enabled);

  model_->SetExtensionEnabled(id, true);
  ext = model_->GetExtension(id);
  EXPECT_TRUE(ext->is_enabled);

  model_->SetExtensionEnabled(id, false);
  ext = model_->GetExtension(id);
  EXPECT_FALSE(ext->is_enabled);
}

// Test remove extension.
TEST_F(AstraExtensionsPageModelTest, RemoveExtension) {
  model_->PopulateSampleExtensions();
  size_t count = model_->GetTotalCount();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  model_->RemoveExtension(id);

  EXPECT_EQ(count - 1, model_->GetTotalCount());
  EXPECT_EQ(nullptr, model_->GetExtension(id));

  // Remove non-existent should do nothing.
  model_->RemoveExtension("nonexistent");
  EXPECT_EQ(count - 1, model_->GetTotalCount());
}

// Test pin toggle.
TEST_F(AstraExtensionsPageModelTest, PinToggle) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  bool was_pinned = all[0].is_pinned;

  model_->ToggleExtensionPinned(id);
  const auto* ext = model_->GetExtension(id);
  EXPECT_NE(was_pinned, ext->is_pinned);
}

// Test sidebar toggle.
TEST_F(AstraExtensionsPageModelTest, SidebarToggle) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  bool was_in_sidebar = all[0].is_in_sidebar;

  model_->ToggleExtensionInSidebar(id);
  const auto* ext = model_->GetExtension(id);
  EXPECT_NE(was_in_sidebar, ext->is_in_sidebar);
}

// Test incognito toggle.
TEST_F(AstraExtensionsPageModelTest, IncognitoToggle) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  bool was_incognito = all[0].allows_in_incognito;

  model_->ToggleExtensionIncognito(id);
  const auto* ext = model_->GetExtension(id);
  EXPECT_NE(was_incognito, ext->allows_in_incognito);
}

// Test set category.
TEST_F(AstraExtensionsPageModelTest, SetCategory) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  model_->SetExtensionCategory(id, "Test Category");
  const auto* ext = model_->GetExtension(id);
  EXPECT_EQ("Test Category", ext->category);
}

// Test set workspace.
TEST_F(AstraExtensionsPageModelTest, SetWorkspace) {
  model_->PopulateSampleExtensions();
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  model_->SetExtensionWorkspace(id, "Test Workspace");
  const auto* ext = model_->GetExtension(id);
  EXPECT_EQ("Test Workspace", ext->workspace);
}

// Test sort by install date sort.
TEST_F(AstraExtensionsPageModelTest, SortByInstallDate) {
  model_->PopulateSampleExtensions();
  model_->SetSortType(AstraExtensionSortType::kInstallDate);
  auto filtered = model_->GetFilteredExtensions();
  ASSERT_GT(filtered.size(), 1u);

  // Should be sorted newest first.
  for (size_t i = 1; i < filtered.size(); ++i) {
    EXPECT_GE(filtered[i - 1].install_time, filtered[i].install_time ||
              filtered[i - 1].install_time == filtered[i].install_time);
  }
}

// Test sort by recent usage.
TEST_F(AstraExtensionsPageModelTest, SortByRecentUsage) {
  model_->PopulateSampleExtensions();
  model_->SetSortType(AstraExtensionSortType::kRecentUsage);
  auto filtered = model_->GetFilteredExtensions();
  ASSERT_GT(filtered.size(), 1u);

  // Should be sorted highest usage first.
  for (size_t i = 1; i < filtered.size(); ++i) {
    EXPECT_GE(filtered[i - 1].recent_usage_count,
              filtered[i].recent_usage_count ||
              filtered[i - 1].recent_usage_count ==
                  filtered[i].recent_usage_count);
  }
}

// Test sort by name.
TEST_F(AstraExtensionsPageModelTest, SortByName) {
  model_->PopulateSampleExtensions();
  model_->SetSortType(AstraExtensionSortType::kName);
  auto filtered = model_->GetFilteredExtensions();
  ASSERT_GT(filtered.size(), 1u);

  // Should be alphabetical.
  for (size_t i = 1; i < filtered.size(); ++i) {
    EXPECT_LE(filtered[i - 1].name <= filtered[i].name);
  }
}

// Test loading state.
TEST_F(AstraExtensionsPageModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());
  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());
  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test filter options.
TEST_F(AstraExtensionsPageModelTest, FilterOptions) {
  auto options = model_->GetFilterOptions();
  EXPECT_GT(options.size(), 5u);
}

// Test sort options.
TEST_F(AstraExtensionsPageModelTest, SortOptions) {
  auto options = model_->GetSortOptions();
  EXPECT_EQ(4u, options.size());
}

// Test observer notifications.
TEST_F(AstraExtensionsPageModelTest, ObserverNotifications) {
  class TestObserver : public AstraExtensionsPageObserver {
   public:
    void OnExtensionsChanged(AstraExtensionsPageModel* m) override {
      changed_count++;
      last_model = m;
    }
    void OnFilterChanged(AstraExtensionsPageModel* m) override {
      filter_changed_count++;
    }
    void OnSearchChanged(AstraExtensionsPageModel* m,
                         const std::u16string& q) override {
      search_changed_count++;
      last_query = q;
    }
    void OnExtensionAdded(AstraExtensionsPageModel* m,
                          const std::string& id) override {
      added_count++;
      last_added_id = id;
    }
    void OnExtensionRemoved(AstraExtensionsPageModel* m,
                            const std::string& id) override {
      removed_count++;
      last_removed_id = id;
    }

    int changed_count = 0;
    int filter_changed_count = 0;
    int search_changed_count = 0;
    int added_count = 0;
    int removed_count = 0;
    AstraExtensionsPageModel* last_model = nullptr;
    std::u16string last_query;
    std::string last_added_id;
    std::string last_removed_id;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Populate should trigger change.
  model_->PopulateSampleExtensions();
  EXPECT_GT(observer.changed_count, 0);

  int prev_changed = observer.changed_count;
  int prev_filter = observer.filter_changed_count;

  // Change filter.
  model_->SetFilter(AstraExtensionFilter::kEnabled);
  EXPECT_GT(observer.filter_changed_count, prev_filter);
  EXPECT_GT(observer.changed_count, prev_changed);

  prev_changed = observer.changed_count;
  int prev_search = observer.search_changed_count;

  // Change search.
  model_->SetSearchQuery(u"test");
  EXPECT_GT(observer.search_changed_count, prev_search);
  EXPECT_GT(observer.changed_count, prev_changed);
  EXPECT_EQ(u"test", observer.last_query);

  // Remove should trigger removed + changed.
  auto all = model_->GetAllExtensions();
  ASSERT_FALSE(all.empty());
  prev_changed = observer.changed_count;
  int prev_removed = observer.removed_count;
  model_->RemoveExtension(all[0].id);
  EXPECT_GT(observer.removed_count, prev_removed);
  EXPECT_GT(observer.changed_count, prev_changed);
  EXPECT_EQ(all[0].id, observer.last_removed_id);

  // TestObserver observer2;
  model_->RemoveObserver(&observer2);

  // Remove and add observer — observer2 should observer removal.
  model_->RemoveObserver(&observer);

  // After removal, no more notifications.
  int after_remove = observer.changed_count;
  model_->SetSearchQuery(u"another");
  EXPECT_EQ(after_remove, observer.changed_count);
}

// Test shutdown notification.
TEST_F(AstraExtensionsPageModelTest, ShutdownNotification) {
  class TestObserver : public AstraExtensionsPageObserver {
   public:
    void OnExtensionsPageModelShutdown(
        AstraExtensionsPageModel* m) override {
      shutdown_called = true;
      last_model = m;
    }
    bool shutdown_called = false;
    AstraExtensionsPageModel* last_model = nullptr;
  };

  TestObserver observer;
  {
    AstraExtensionsPageModel m;
    m.AddObserver(&observer);
    EXPECT_FALSE(observer.shutdown_called);
  }
  EXPECT_TRUE(observer.shutdown_called);
}

// ===========================================================================
// AstraExtensionsPageViewTest
// ===========================================================================

class AstraExtensionsPageViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraExtensionsPageModel>();
    view_ = std::make_unique<AstraExtensionsPageView>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraExtensionsPageModel> model_;
  std::unique_ptr<AstraExtensionsPageView> view_;
};

// Test view creation.
TEST_F(AstraExtensionsPageViewTest, Creation) {
  EXPECT_NE(nullptr, view_->search_field());
  EXPECT_NE(nullptr, view_->sort_combobox());
  EXPECT_NE(nullptr, view_->categories_sidebar());
  EXPECT_NE(nullptr, view_->content_scroll_view());
  EXPECT_NE(nullptr, view_->extensions_container());
  EXPECT_NE(nullptr, view_->empty_state());
  EXPECT_NE(nullptr, view_->add_button());
  EXPECT_EQ(nullptr, view_->model());
  EXPECT_EQ(nullptr, view_->delegate());
}

// Test set model.
TEST_F(AstraExtensionsPageViewTest, SetModel) {
  view_->SetModel(model_.get());
  EXPECT_EQ(model_.get(), view_->model());

  // Setting same model again should be safe.
  view_->SetModel(model_.get());
  EXPECT_EQ(model_.get(), view_->model());

  // Setting null should work.
  view_->SetModel(nullptr);
  EXPECT_EQ(nullptr, view_->model());
}

// Test view with model and sample data.
TEST_F(AstraExtensionsPageViewTest, WithSampleData) {
  model_->PopulateSampleExtensions();
  view_->SetModel(model_.get());

  EXPECT_GT(view_->GetExtensionCardCount(), 0);
  EXPECT_FALSE(view_->empty_state()->GetVisible());
}

// Test empty state.
TEST_F(AstraExtensionsPageViewTest, EmptyState) {
  view_->SetModel(model_.get());
  EXPECT_TRUE(view_->empty_state()->GetVisible());
}

// Test search field changes trigger search updates cards.
TEST_F(AstraExtensionsPageViewTest, SearchUpdatesView) {
  model_->PopulateSampleExtensions();
  view_->SetModel(model_.get());

  int initial_count = view_->GetExtensionCardCount();
  EXPECT_GT(initial_count, 0);

  model_->SetSearchQuery(u"Chrome");
  // After search, should have fewer or equal cards.
  EXPECT_LE(view_->GetExtensionCardCount(), initial_count);

  model_->SetSearchQuery(u"xyznonexistent123");
  EXPECT_EQ(0, view_->GetExtensionCardCount());
  EXPECT_TRUE(view_->empty_state()->GetVisible());
}

// Test display mode toggle.
TEST_F(AstraExtensionsPageViewTest, DisplayModeToggle) {
  model_->PopulateSampleExtensions();
  view_->SetModel(model_.get());

  EXPECT_FALSE(view_->IsCompactMode());

  view_->SetDisplayMode(true);
  EXPECT_TRUE(view_->IsCompactMode());

  view_->SetDisplayMode(false);
  EXPECT_FALSE(view_->IsCompactMode());
}

// Test set delegate.
TEST_F(AstraExtensionsPageViewTest, SetDelegate) {
  class TestDelegate : public AstraExtensionsPageDelegate {
   public:
    void OnOpenExtension(const std::string& id) override { open_called++; }
    void OnExtensionOptions(const std::string& id) override {
      options_called++;
    }
    void OnOpenChromeWebStore() override { webstore_called++; }

    int open_called = 0;
    int options_called = 0;
    int webstore_called = 0;
  };

  TestDelegate delegate;
  view_->SetDelegate(&delegate);
  EXPECT_EQ(&delegate, view_->delegate());
}

// Test extension card view.
TEST_F(AstraExtensionsPageViewTest, ExtensionCard) {
  model_->PopulateSampleExtensions();
  view_->SetModel(model_.get());

  auto* card = view_->GetExtensionCardAt(0);
  ASSERT_NE(nullptr, card);
  EXPECT_NE(std::string(), card->extension_id());
  EXPECT_NE(nullptr, card->enabled_toggle());
  EXPECT_NE(nullptr, card->pin_button());
  EXPECT_NE(nullptr, card->details_button());
  EXPECT_NE(nullptr, card->name_label());
  EXPECT_NE(nullptr, card->desc_label());
}

// Test category sidebar has cards are visible with filter.
TEST_F(AstraExtensionsPageViewTest, FilterUpdatesView) {
  model_->PopulateSampleExtensions();
  view_->SetModel(model_.get());

  int all_count = view_->GetExtensionCardCount();

  model_->SetFilter(AstraExtensionFilter::kEnabled);
  int enabled_count = view_->GetExtensionCardCount();
  EXPECT_LE(enabled_count, all_count);

  model_->SetFilter(AstraExtensionFilter::kThemes);
  int themes_count = view_->GetExtensionCardCount();
  EXPECT_LT(themes_count, all_count);
}

// Test preferred size.
TEST_F(AstraExtensionsPageViewTest, PreferredSize) {
  gfx::Size pref = view_->CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// Test search field updates model.
TEST_F(AstraExtensionsPageViewTest, SearchFieldUpdatesModel) {
  view_->SetModel(model_.get());
  model_->PopulateSampleExtensions();

  // Simulate typing in the search field.
  view_->search_field()->SetText(u"test");
  // The TextfieldController::SetText calls ContentsChanged when text change
  // actually ContentsChanged text field.
  EXPECT_EQ(u"test", model_->GetSearchQuery());
}

}  // namespace astra
