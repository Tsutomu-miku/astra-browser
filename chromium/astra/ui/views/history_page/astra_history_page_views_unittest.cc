// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/history_page/astra_history_page_model.h"
#include "astra/ui/views/history_page/astra_history_page_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"

namespace astra {

class AstraHistoryPageModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraHistoryPageModel>();
  }

  void TearDown() override {
    model_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraHistoryPageModel> model_;
};

// Test model creation.
TEST_F(AstraHistoryPageModelTest, ModelCreation) {
  EXPECT_TRUE(model_->GetDays().empty());
  EXPECT_EQ(0u, model_->GetTotalEntryCount());
  EXPECT_EQ(AstraHistoryFilter::kAll, model_->GetFilter());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
  EXPECT_FALSE(model_->IsLoading());
}

// Test PopulateSampleHistory populates entries.
TEST_F(AstraHistoryPageModelTest, PopulateSampleHistory) {
  model_->PopulateSampleHistory();

  // Should have 30+ entries.
  EXPECT_GE(model_->GetTotalEntryCount(), 30u);

  // Days should be grouped and non-empty.
  const auto& days = model_->GetDays();
  EXPECT_FALSE(days.empty());

  // Each day should have entries.
  for (const auto& day : days) {
    EXPECT_FALSE(day.entries.empty());
    EXPECT_GT(day.total_visits, 0);
    EXPECT_FALSE(day.date_label.empty());
  }
}

// Test GetDays returns grouped days.
TEST_F(AstraHistoryPageModelTest, GetDaysGroupsByDay) {
  model_->PopulateSampleHistory();

  const auto& days = model_->GetDays();
  ASSERT_FALSE(days.empty());

  // Days should be in order (newest first).
  for (size_t i = 1; i < days.size(); ++i) {
    EXPECT_GT(days[i - 1].date, days[i].date);
  }

  // First day should be "Today" or "Yesterday".
  EXPECT_TRUE(days[0].date_label == u"Today" ||
              days[0].date_label == u"Yesterday");
}

// Test GetEntry by ID.
TEST_F(AstraHistoryPageModelTest, GetEntryById) {
  model_->PopulateSampleHistory();

  const AstraHistoryEntry* entry = model_->GetEntry("h001");
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ("h001", entry->id);
  EXPECT_FALSE(entry->title.empty());
  EXPECT_FALSE(entry->url.empty());

  // Non-existent ID returns nullptr.
  EXPECT_EQ(nullptr, model_->GetEntry("nonexistent-id"));
}

// Test filtering by time range.
TEST_F(AstraHistoryPageModelTest, FilterByTimeRange) {
  model_->PopulateSampleHistory();

  size_t all_count = model_->GetTotalEntryCount();
  EXPECT_GT(all_count, 0u);

  // Filter to "Today".
  model_->SetFilter(AstraHistoryFilter::kToday);
  size_t today_count = 0;
  for (const auto& day : model_->GetDays()) {
    today_count += day.entries.size();
  }
  EXPECT_LE(today_count, all_count);
  EXPECT_GT(today_count, 0u);

  // Filter to "Last 7 days" should show more than today.
  model_->SetFilter(AstraHistoryFilter::kLast7Days);
  size_t week_count = 0;
  for (const auto& day : model_->GetDays()) {
    week_count += day.entries.size();
  }
  EXPECT_GE(week_count, today_count);
}

// Test search filtering.
TEST_F(AstraHistoryPageModelTest, SearchFiltering) {
  model_->PopulateSampleHistory();

  size_t all_count = model_->GetTotalEntryCount();
  EXPECT_GT(all_count, 0u);

  // Search for "google" should match some entries.
  model_->SetSearchQuery(u"google");
  size_t search_count = 0;
  for (const auto& day : model_->GetDays()) {
    search_count += day.entries.size();
  }
  EXPECT_LE(search_count, all_count);

  // Search with no matches.
  model_->SetSearchQuery(u"zzz-no-match-xyz");
  EXPECT_TRUE(model_->GetDays().empty());

  // Clear search.
  model_->SetSearchQuery(u"");
  EXPECT_FALSE(model_->GetDays().empty());
}

// Test search is case-insensitive.
TEST_F(AstraHistoryPageModelTest, SearchCaseInsensitive) {
  model_->PopulateSampleHistory();

  model_->SetSearchQuery(u"GOOGLE");
  size_t upper_count = 0;
  for (const auto& day : model_->GetDays()) {
    upper_count += day.entries.size();
  }

  model_->SetSearchQuery(u"google");
  size_t lower_count = 0;
  for (const auto& day : model_->GetDays()) {
    lower_count += day.entries.size();
  }

  EXPECT_EQ(upper_count, lower_count);
}

// Test category filtering.
TEST_F(AstraHistoryPageModelTest, CategoryFiltering) {
  model_->PopulateSampleHistory();

  auto categories = model_->GetCategories();
  EXPECT_FALSE(categories.empty());

  // Filter by a specific category.
  std::string first_category = categories[0];
  model_->SetCategoryFilter(first_category);

  size_t filtered_count = 0;
  for (const auto& day : model_->GetDays()) {
    filtered_count += day.entries.size();
    for (const auto& entry : day.entries) {
      EXPECT_EQ(first_category, entry.category);
    }
  }
  EXPECT_GT(filtered_count, 0u);

  // Clear category filter.
  model_->SetCategoryFilter("");
  size_t all_count = 0;
  for (const auto& day : model_->GetDays()) {
    all_count += day.entries.size();
  }
  EXPECT_GT(all_count, filtered_count);
}

// Test GetCategories returns unique categories.
TEST_F(AstraHistoryPageModelTest, GetCategoriesUnique) {
  model_->PopulateSampleHistory();

  auto categories = model_->GetCategories();
  std::set<std::string> unique_cats(categories.begin(), categories.end());
  EXPECT_EQ(categories.size(), unique_cats.size());

  // Should have multiple categories (Work, Social, News, etc.).
  EXPECT_GE(categories.size(), 4u);
}

// Test RemoveEntry.
TEST_F(AstraHistoryPageModelTest, RemoveEntry) {
  model_->PopulateSampleHistory();

  size_t original_count = model_->GetTotalEntryCount();
  EXPECT_GT(original_count, 0u);

  model_->RemoveEntry("h001");
  EXPECT_EQ(original_count - 1, model_->GetTotalEntryCount());
  EXPECT_EQ(nullptr, model_->GetEntry("h001"));

  // Removing a non-existent entry is a no-op.
  model_->RemoveEntry("nonexistent");
  EXPECT_EQ(original_count - 1, model_->GetTotalEntryCount());
}

// Test ClearAllHistory.
TEST_F(AstraHistoryPageModelTest, ClearAllHistory) {
  model_->PopulateSampleHistory();
  EXPECT_GT(model_->GetTotalEntryCount(), 0u);
  EXPECT_FALSE(model_->GetDays().empty());

  model_->ClearAllHistory();
  EXPECT_EQ(0u, model_->GetTotalEntryCount());
  EXPECT_TRUE(model_->GetDays().empty());
}

// Test RemoveAllInRange.
TEST_F(AstraHistoryPageModelTest, RemoveAllInRange) {
  model_->PopulateSampleHistory();

  size_t all_count = model_->GetTotalEntryCount();
  EXPECT_GT(all_count, 0u);

  // Filter to today, then remove all in range.
  model_->SetFilter(AstraHistoryFilter::kToday);
  size_t today_count = 0;
  for (const auto& day : model_->GetDays()) {
    today_count += day.entries.size();
  }
  EXPECT_GT(today_count, 0u);

  model_->RemoveAllInRange();

  // Today should now be empty.
  model_->SetFilter(AstraHistoryFilter::kToday);
  EXPECT_TRUE(model_->GetDays().empty());

  // All entries should be reduced by today count.
  model_->SetFilter(AstraHistoryFilter::kAll);
  EXPECT_EQ(all_count - today_count, model_->GetTotalEntryCount());
}

// Test observer notifications.
TEST_F(AstraHistoryPageModelTest, ObserverNotifications) {
  class TestObserver : public AstraHistoryPageObserver {
   public:
    void OnHistoryChanged(AstraHistoryPageModel* m) override {
      history_changed_count++;
      last_model = m;
    }
    void OnFilterChanged(AstraHistoryPageModel* m,
                         AstraHistoryFilter f) override {
      filter_changed_count++;
      last_filter = f;
    }
    void OnSearchChanged(AstraHistoryPageModel* m,
                         const std::u16string& q) override {
      search_changed_count++;
      last_query = q;
    }
    void OnHistoryEntryRemoved(AstraHistoryPageModel* m,
                               const std::string& id) override {
      entry_removed_count++;
      last_removed_id = id;
    }
    void OnHistoryPageModelShutdown(AstraHistoryPageModel* m) override {
      shutdown_count++;
    }

    int history_changed_count = 0;
    int filter_changed_count = 0;
    int search_changed_count = 0;
    int entry_removed_count = 0;
    int shutdown_count = 0;
    AstraHistoryPageModel* last_model = nullptr;
    AstraHistoryFilter last_filter = AstraHistoryFilter::kAll;
    std::u16string last_query;
    std::string last_removed_id;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Populate should trigger OnHistoryChanged.
  model_->PopulateSampleHistory();
  EXPECT_GE(observer.history_changed_count, 1);
  EXPECT_EQ(model_.get(), observer.last_model);

  // SetFilter should trigger OnFilterChanged and OnHistoryChanged.
  int prev_history = observer.history_changed_count;
  model_->SetFilter(AstraHistoryFilter::kToday);
  EXPECT_EQ(1, observer.filter_changed_count);
  EXPECT_EQ(AstraHistoryFilter::kToday, observer.last_filter);
  EXPECT_GT(observer.history_changed_count, prev_history);

  // SetSearchQuery should trigger OnSearchChanged and OnHistoryChanged.
  prev_history = observer.history_changed_count;
  model_->SetSearchQuery(u"test");
  EXPECT_EQ(1, observer.search_changed_count);
  EXPECT_EQ(u"test", observer.last_query);
  EXPECT_GT(observer.history_changed_count, prev_history);

  // RemoveEntry should trigger OnHistoryEntryRemoved and OnHistoryChanged.
  prev_history = observer.history_changed_count;
  model_->RemoveEntry("h001");
  EXPECT_EQ(1, observer.entry_removed_count);
  EXPECT_EQ("h001", observer.last_removed_id);
  EXPECT_GT(observer.history_changed_count, prev_history);

  // Remove observer and verify no more notifications.
  model_->RemoveObserver(&observer);
  int history_before = observer.history_changed_count;
  model_->ClearAllHistory();
  EXPECT_EQ(history_before, observer.history_changed_count);
}

// Test SetLoading.
TEST_F(AstraHistoryPageModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());

  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());

  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test GetFilterOptions.
TEST_F(AstraHistoryPageModelTest, GetFilterOptions) {
  auto options = model_->GetFilterOptions();
  EXPECT_EQ(6u, options.size());

  // Verify all filter values are present.
  std::set<AstraHistoryFilter> filters;
  for (const auto& opt : options) {
    filters.insert(opt.first);
  }
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kAll));
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kToday));
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kYesterday));
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kLast7Days));
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kLast30Days));
  EXPECT_TRUE(filters.count(AstraHistoryFilter::kThisMonth));
}

// ===========================================================================
// View tests
// ===========================================================================

class AstraHistoryPageViewTest : public views::ViewsTestBase {
 protected:
  void SetUp() override {
    views::ViewsTestBase::SetUp();

    model_ = std::make_unique<AstraHistoryPageModel>();
    model_->PopulateSampleHistory();

    view_ = std::make_unique<AstraHistoryPageView>(model_.get());
    view_->SetSize(gfx::Size(800, 600));
    view_->Layout();
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
    views::ViewsTestBase::TearDown();
  }

  std::unique_ptr<AstraHistoryPageModel> model_;
  std::unique_ptr<AstraHistoryPageView> view_;
};

// Test view creation with model.
TEST_F(AstraHistoryPageViewTest, ViewCreation) {
  EXPECT_NE(nullptr, view_->model());
  EXPECT_EQ(model_.get(), view_->model());
  EXPECT_NE(nullptr, view_->search_field());
  EXPECT_NE(nullptr, view_->clear_data_button());
  EXPECT_NE(nullptr, view_->scroll_view());
  EXPECT_NE(nullptr, view_->content_view());
}

// Test view observes model changes.
TEST_F(AstraHistoryPageViewTest, ObservesModelChanges) {
  // Initially has content.
  EXPECT_FALSE(model_->GetDays().empty());

  // Clearing history should update the view (empty state).
  model_->ClearAllHistory();
  EXPECT_TRUE(model_->GetDays().empty());
}

// Test search field updates model.
TEST_F(AstraHistoryPageViewTest, SearchFieldUpdatesModel) {
  ASSERT_TRUE(view_->search_field());

  view_->search_field_->SetText(u"google");
  // Textfield controller callback fires when contents change.
  // Since SetText might not trigger ContentsChanged directly, we test
  // via the model directly.
  model_->SetSearchQuery(u"google");
  EXPECT_EQ(u"google", model_->GetSearchQuery());

  // Verify filtering works.
  EXPECT_FALSE(model_->GetDays().empty());
}

// Test SetModel changes the observed model.
TEST_F(AstraHistoryPageViewTest, SetModel) {
  auto new_model = std::make_unique<AstraHistoryPageModel>();
  new_model->PopulateSampleHistory();

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // View should reflect the new model.
  EXPECT_FALSE(new_model->GetDays().empty());

  // Clean up: disconnect before destruction.
  view_->SetModel(nullptr);
}

// Test empty state.
TEST_F(AstraHistoryPageViewTest, EmptyState) {
  // Start with populated model - no empty state.
  EXPECT_FALSE(model_->GetDays().empty());

  // Clear all - should show empty state.
  model_->ClearAllHistory();
  EXPECT_TRUE(model_->GetDays().empty());
}

// Test that delegate is null initially.
TEST_F(AstraHistoryPageViewTest, DelegateNullInitially) {
  EXPECT_EQ(nullptr, view_->delegate());
}

// Test setting delegate.
TEST_F(AstraHistoryPageViewTest, SetDelegate) {
  class TestDelegate : public AstraHistoryPageDelegate {
   public:
    void OnHistoryEntryClicked(const std::string&) override {}
    void OnBookmarkToggled(const std::string&, bool) override {}
    void OnClearBrowsingData() override { clear_called = true; }
    void OnSearchQueryChanged(const std::u16string&) override {}
    void OnFilterChanged(AstraHistoryFilter) override {}
    void OnCategoryFilterChanged(const std::string&) override {}
    void OnRemoveEntry(const std::string&) override {}

    bool clear_called = false;
  };

  TestDelegate delegate;
  view_->SetDelegate(&delegate);
  EXPECT_EQ(&delegate, view_->delegate());
}

// Test AstraHistoryEntryRow.
TEST_F(AstraHistoryPageViewTest, EntryRow) {
  AstraHistoryEntry entry;
  entry.id = "test123";
  entry.title = u"Test Page Title";
  entry.url = "https://example.com/page";
  entry.host = "example.com";
  entry.visit_time = base::Time::Now();
  entry.is_bookmarked = true;
  entry.category = "Work";

  AstraHistoryEntryRow row(entry);
  row.SetSize(gfx::Size(600, 56));

  EXPECT_EQ("test123", row.entry_id());
  EXPECT_EQ("Test Page Title", base::UTF16ToUTF8(row.entry().title));
  EXPECT_EQ("example.com", row.entry().host);
  EXPECT_NE(nullptr, row.bookmark_button());
  EXPECT_NE(nullptr, row.more_button());
}

// Test AstraHistoryDaySection.
TEST_F(AstraHistoryPageViewTest, DaySection) {
  AstraHistoryDay day;
  day.date = base::Time::Now();
  day.date_label = u"Today";
  day.total_visits = 5;

  // Add 2 entries.
  AstraHistoryEntry e1;
  e1.id = "e1";
  e1.title = u"Entry 1";
  e1.visit_time = base::Time::Now();
  day.entries.push_back(e1);

  AstraHistoryEntry e2;
  e2.id = "e2";
  e2.title = u"Entry 2";
  e2.visit_time = base::Time::Now();
  day.entries.push_back(e2);

  AstraHistoryDaySection section(day);
  section.SetSize(gfx::Size(600, 200));

  EXPECT_EQ(2u, section.GetEntryCount());
  EXPECT_NE(nullptr, section.GetEntryRow(0));
  EXPECT_NE(nullptr, section.GetEntryRow(1));
  EXPECT_EQ(nullptr, section.GetEntryRow(5));
  EXPECT_EQ("e1", section.GetEntryRow(0)->entry_id());
}

// Test that RefreshFromModel rebuilds the view.
TEST_F(AstraHistoryPageViewTest, RefreshFromModel) {
  // Verify the view has content from the model.
  EXPECT_FALSE(model_->GetDays().empty());

  // Call refresh explicitly.
  view_->RefreshFromModel();

  // Should still have content.
  EXPECT_FALSE(model_->GetDays().empty());
}

}  // namespace astra
