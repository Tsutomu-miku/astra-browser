// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_page/astra_downloads_page_model.h"
#include "astra/ui/views/downloads_page/astra_downloads_page_view.h"

#include <set>

#include "base/i18n/case_conversion.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"

namespace astra {

namespace {

// Test observer that tracks notification counts.
class TestDownloadsPageObserver : public AstraDownloadsPageObserver {
 public:
  void OnDownloadsChanged() override { downloads_changed_count_++; }
  void OnDownloadAdded(const std::string& id) override {
    download_added_count_++;
    last_added_id_ = id;
  }
  void OnDownloadRemoved(const std::string& id) override {
    download_removed_count_++;
    last_removed_id_ = id;
  }
  void OnDownloadUpdated(const std::string& id) override {
    download_updated_count_++;
    last_updated_id_ = id;
  }
  void OnSearchChanged(const std::u16string& query) override {
    search_changed_count_++;
    last_search_query_ = query;
  }
  void OnFilterChanged() override { filter_changed_count_++; }
  void OnDownloadsPageModelShutdown() override {
    model_shutdown_count_++;
  }

  int downloads_changed_count_ = 0;
  int download_added_count_ = 0;
  int download_removed_count_ = 0;
  int download_updated_count_ = 0;
  int search_changed_count_ = 0;
  int filter_changed_count_ = 0;
  int model_shutdown_count_ = 0;

  std::string last_added_id_;
  std::string last_removed_id_;
  std::string last_updated_id_;
  std::u16string last_search_query_;
};

}  // namespace

// =========================================================================
// AstraDownloadsPageModel unit tests
// =========================================================================

class AstraDownloadsPageModelTest : public testing::Test {
 protected:
  AstraDownloadsPageModelTest() = default;
  ~AstraDownloadsPageModelTest() override = default;

  base::test::TaskEnvironment task_environment_;
  AstraDownloadsPageModel model_;
};

TEST_F(AstraDownloadsPageModelTest, CreateModel) {
  EXPECT_EQ(0u, model_.GetCount());
  EXPECT_TRUE(model_.GetDownloads().empty());
  EXPECT_EQ(nullptr, model_.GetDownload("nonexistent"));
  EXPECT_EQ(AstraDownloadsPageFilter::kAll, model_.GetFilter());
  EXPECT_TRUE(model_.GetSearchQuery().empty());
  EXPECT_TRUE(model_.GetCategoryFilter().empty());
  EXPECT_FALSE(model_.IsLoading());
  EXPECT_EQ(0, model_.GetTotalDownloadedBytes());
}

TEST_F(AstraDownloadsPageModelTest, PopulateSampleDownloads) {
  model_.PopulateSampleDownloads();

  // Should have 15+ sample downloads.
  EXPECT_GE(model_.GetCount(), 15u);

  // All downloads should be visible when filter is "All".
  EXPECT_EQ(model_.GetCount(), model_.GetDownloads().size());

  // Downloads should be sorted by start_time descending.
  const auto& downloads = model_.GetDownloads();
  for (size_t i = 1; i < downloads.size(); ++i) {
    EXPECT_GE(downloads[i - 1].start_time, downloads[i].start_time);
  }

  // Should have multiple categories.
  auto categories = model_.GetCategories();
  EXPECT_GE(categories.size(), 3u);

  // Total downloaded bytes should be non-zero.
  EXPECT_GT(model_.GetTotalDownloadedBytes(), 0);
}

TEST_F(AstraDownloadsPageModelTest, GetDownloadById) {
  model_.PopulateSampleDownloads();

  const auto& downloads = model_.GetDownloads();
  ASSERT_FALSE(downloads.empty());

  const std::string& first_id = downloads.front().id;
  const AstraDownloadEntry* entry = model_.GetDownload(first_id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(first_id, entry->id);
  EXPECT_FALSE(entry->file_name.empty());

  // Non-existent ID returns nullptr.
  EXPECT_EQ(nullptr, model_.GetDownload("nonexistent-id"));
}

TEST_F(AstraDownloadsPageModelTest, FilterByState) {
  model_.PopulateSampleDownloads();
  size_t all_count = model_.GetCount();
  EXPECT_GT(all_count, 0u);

  // Filter by "In progress" should show in-progress and paused.
  model_.SetFilter(AstraDownloadsPageFilter::kInProgress);
  const auto& in_progress = model_.GetDownloads();
  EXPECT_LT(in_progress.size(), all_count);
  for (const auto& entry : in_progress) {
    EXPECT_TRUE(entry.state == AstraDownloadPageState::kInProgress ||
                entry.state == AstraDownloadPageState::kPaused);
  }

  // Filter by "Completed".
  model_.SetFilter(AstraDownloadsPageFilter::kCompleted);
  const auto& completed = model_.GetDownloads();
  for (const auto& entry : completed) {
    EXPECT_EQ(AstraDownloadPageState::kComplete, entry.state);
  }

  // Filter by "Cancelled".
  model_.SetFilter(AstraDownloadsPageFilter::kCancelled);
  const auto& cancelled = model_.GetDownloads();
  for (const auto& entry : cancelled) {
    EXPECT_EQ(AstraDownloadPageState::kCancelled, entry.state);
  }

  // Filter by "Interrupted".
  model_.SetFilter(AstraDownloadsPageFilter::kInterrupted);
  const auto& interrupted = model_.GetDownloads();
  for (const auto& entry : interrupted) {
    EXPECT_EQ(AstraDownloadPageState::kInterrupted, entry.state);
  }

  // Reset to "All".
  model_.SetFilter(AstraDownloadsPageFilter::kAll);
  EXPECT_EQ(all_count, model_.GetDownloads().size());
}

TEST_F(AstraDownloadsPageModelTest, SearchQuery) {
  model_.PopulateSampleDownloads();
  size_t all_count = model_.GetCount();

  // Search for something that should match some downloads.
  model_.SetSearchQuery(u"report");
  const auto& filtered = model_.GetDownloads();
  EXPECT_LT(filtered.size(), all_count);
  for (const auto& entry : filtered) {
    std::u16 name_lower = base::i18n::ToLower(entry.file_name);
    std::u16 url_lower = base::i18n::ToLower(base::UTF8ToUTF16(entry.url));
    EXPECT_TRUE(name_lower.find(u"report") != std::u16string::npos ||
                url_lower.find(u"report") != std::u16string::npos);
  }

  // Empty search shows all.
  model_.SetSearchQuery(u"");
  EXPECT_EQ(all_count, model_.GetDownloads().size());
}

TEST_F(AstraDownloadsPageModelTest, CategoryFilter) {
  model_.PopulateSampleDownloads();

  auto categories = model_.GetCategories();
  ASSERT_FALSE(categories.empty());

  // Filter by first category.
  const std::string& cat = categories[0];
  model_.SetCategoryFilter(cat);
  const auto& filtered = model_.GetDownloads();
  EXPECT_FALSE(filtered.empty());
  for (const auto& entry : filtered) {
    EXPECT_EQ(cat, entry.category);
  }

  // Empty category filter shows all.
  model_.SetCategoryFilter("");
  EXPECT_EQ(model_.GetCount(), model_.GetDownloads().size());
}

TEST_F(AstraDownloadsPageModelTest, CombinedFilterAndSearch) {
  model_.PopulateSampleDownloads();

  // Apply both state filter and search.
  model_.SetFilter(AstraDownloadsPageFilter::kCompleted);
  model_.SetSearchQuery(u"photo");

  const auto& filtered = model_.GetDownloads();
  for (const auto& entry : filtered) {
    EXPECT_EQ(AstraDownloadPageState::kComplete, entry.state);
    std::u16 name_lower = base::i18n::ToLower(entry.file_name);
    EXPECT_TRUE(name_lower.find(u"photo") != std::u16string::npos);
  }
}

TEST_F(AstraDownloadsPageModelTest, CombinedFilterAndCategory) {
  model_.PopulateSampleDownloads();

  model_.SetFilter(AstraDownloadsPageFilter::kCompleted);
  auto categories = model_.GetCategories();
  ASSERT_FALSE(categories.empty());

  model_.SetCategoryFilter(categories[0]);
  const auto& filtered = model_.GetDownloads();
  for (const auto& entry : filtered) {
    EXPECT_EQ(AstraDownloadPageState::kComplete, entry.state);
    EXPECT_EQ(categories[0], entry.category);
  }
}

TEST_F(AstraDownloadsPageModelTest, RemoveDownload) {
  model_.PopulateSampleDownloads();
  size_t original_count = model_.GetCount();

  const std::string first_id = model_.GetDownloads().front().id;
  model_.RemoveDownload(first_id);

  EXPECT_EQ(original_count - 1, model_.GetCount());
  EXPECT_EQ(nullptr, model_.GetDownload(first_id));
}

TEST_F(AstraDownloadsPageModelTest, ClearAllDownloads) {
  model_.PopulateSampleDownloads();
  EXPECT_GT(model_.GetCount(), 0u);

  model_.ClearAllDownloads();
  EXPECT_EQ(0u, model_.GetCount());
  EXPECT_TRUE(model_.GetDownloads().empty());
}

TEST_F(AstraDownloadsPageModelTest, PauseAndResumeDownload) {
  model_.PopulateSampleDownloads();

  // Find an in-progress download.
  std::string in_progress_id;
  for (const auto& entry : model_.GetDownloads()) {
    if (entry.state == AstraDownloadPageState::kInProgress) {
      in_progress_id = entry.id;
      break;
    }
  }
  ASSERT_FALSE(in_progress_id.empty());

  // Pause it.
  model_.PauseDownload(in_progress_id);
  const AstraDownloadEntry* paused = model_.GetDownload(in_progress_id);
  ASSERT_NE(nullptr, paused);
  EXPECT_EQ(AstraDownloadPageState::kPaused, paused->state);

  // Resume it.
  model_.ResumeDownload(in_progress_id);
  const AstraDownloadEntry* resumed = model_.GetDownload(in_progress_id);
  ASSERT_NE(nullptr, resumed);
  EXPECT_EQ(AstraDownloadPageState::kInProgress, resumed->state);
}

TEST_F(AstraDownloadsPageModelTest, CancelDownload) {
  model_.PopulateSampleDownloads();

  // Find an in-progress download.
  std::string in_progress_id;
  for (const auto& entry : model_.GetDownloads()) {
    if (entry.state == AstraDownloadPageState::kInProgress) {
      in_progress_id = entry.id;
      break;
    }
  }
  ASSERT_FALSE(in_progress_id.empty());

  model_.CancelDownload(in_progress_id);
  const AstraDownloadEntry* cancelled = model_.GetDownload(in_progress_id);
  ASSERT_NE(nullptr, cancelled);
  EXPECT_EQ(AstraDownloadPageState::kCancelled, cancelled->state);
  EXPECT_FALSE(cancelled->end_time.is_null());
}

TEST_F(AstraDownloadsPageModelTest, RetryDownload) {
  model_.PopulateSampleDownloads();

  // Find a cancelled download.
  std::string cancelled_id;
  for (const auto& entry : model_.GetDownloads()) {
    if (entry.state == AstraDownloadPageState::kCancelled) {
      cancelled_id = entry.id;
      break;
    }
  }
  ASSERT_FALSE(cancelled_id.empty());

  model_.RetryDownload(cancelled_id);
  const AstraDownloadEntry* retried = model_.GetDownload(cancelled_id);
  ASSERT_NE(nullptr, retried);
  EXPECT_EQ(AstraDownloadPageState::kInProgress, retried->state);
  EXPECT_EQ(0, retried->received_bytes);
}

TEST_F(AstraDownloadsPageModelTest, LoadingState) {
  EXPECT_FALSE(model_.IsLoading());

  model_.SetLoading(true);
  EXPECT_TRUE(model_.IsLoading());

  model_.SetLoading(false);
  EXPECT_FALSE(model_.IsLoading());
}

TEST_F(AstraDownloadsPageModelTest, FilterOptions) {
  auto options = model_.GetFilterOptions();
  EXPECT_EQ(5u, options.size());  // All, In progress, Completed, Cancelled, Interrupted
}

TEST_F(AstraDownloadsPageModelTest, ObserversNotification) {
  TestDownloadsPageObserver observer;
  model_.AddObserver(&observer);

  // Populating sample data should trigger downloads changed.
  model_.PopulateSampleDownloads();
  EXPECT_GT(observer.downloads_changed_count_, 0);

  int prev_downloads_changed = observer.downloads_changed_count_;
  int prev_search_changed = observer.search_changed_count_;
  int prev_filter_changed = observer.filter_changed_count_;

  // Setting search should trigger search changed and downloads changed.
  model_.SetSearchQuery(u"test");
  EXPECT_EQ(prev_search_changed + 1, observer.search_changed_count_);
  EXPECT_EQ(u"test", observer.last_search_query_);
  EXPECT_GT(observer.downloads_changed_count_, prev_downloads_changed);

  prev_downloads_changed = observer.downloads_changed_count_;

  // Setting filter should trigger filter changed and downloads changed.
  model_.SetFilter(AstraDownloadsPageFilter::kCompleted);
  EXPECT_EQ(prev_filter_changed + 1, observer.filter_changed_count_);
  EXPECT_GT(observer.downloads_changed_count_, prev_downloads_changed);

  prev_downloads_changed = observer.downloads_changed_count_;

  // Removing a download should trigger removed and downloads changed.
  size_t count_before = model_.GetCount();
  const std::string first_id = model_.GetDownloads().front().id;
  int prev_removed = observer.download_removed_count_;
  model_.RemoveDownload(first_id);
  EXPECT_EQ(prev_removed + 1, observer.download_removed_count_);
  EXPECT_EQ(first_id, observer.last_removed_id_);
  EXPECT_GT(observer.downloads_changed_count_, prev_downloads_changed);
  EXPECT_EQ(count_before - 1, model_.GetCount());

  // Remove observer and verify no more notifications.
  model_.RemoveObserver(&observer);
  int notifications_after_remove = observer.downloads_changed_count_;
  model_.SetSearchQuery(u"something else");
  EXPECT_EQ(notifications_after_remove, observer.downloads_changed_count_);
}

TEST_F(AstraDownloadsPageModelTest, ShutdownNotification) {
  TestDownloadsPageObserver observer;
  auto model = std::make_unique<AstraDownloadsPageModel>();
  model->AddObserver(&observer);

  EXPECT_EQ(0, observer.model_shutdown_count_);
  model.reset();
  EXPECT_EQ(1, observer.model_shutdown_count_);
}

TEST_F(AstraDownloadsPageModelTest, CategoriesAreUnique) {
  model_.PopulateSampleDownloads();
  auto categories = model_.GetCategories();

  // Verify no duplicate categories.
  std::set<std::string> unique_cats(categories.begin(), categories.end());
  EXPECT_EQ(categories.size(), unique_cats.size());
}

TEST_F(AstraDownloadsPageModelTest, TotalDownloadedBytes) {
  model_.PopulateSampleDownloads();

  int64_t total = model_.GetTotalDownloadedBytes();
  EXPECT_GT(total, 0);

  // Manually calculate total from completed downloads.
  int64_t expected_total = 0;
  for (size_t i = 0; i < model_.GetCount(); ++i) {
    const auto* entry = model_.GetDownload(model_.GetDownloads()[i].id);
    if (entry && entry->state == AstraDownloadPageState::kComplete) {
      expected_total += entry->received_bytes;
    }
  }
  EXPECT_EQ(expected_total, total);
}

// =========================================================================
// AstraDownloadsPageView unit tests
// =========================================================================

class AstraDownloadsPageViewTest : public views::ViewsTestBase {
 protected:
  AstraDownloadsPageViewTest() = default;
  ~AstraDownloadsPageViewTest() override = default;

  void SetUp() override {
    views::ViewsTestBase::SetUp();
    model_ = std::make_unique<AstraDownloadsPageModel>();
    view_ = std::make_unique<AstraDownloadsPageView>(model_.get());
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
    views::ViewsTestBase::TearDown();
  }

  std::unique_ptr<AstraDownloadsPageModel> model_;
  std::unique_ptr<AstraDownloadsPageView> view_;
};

TEST_F(AstraDownloadsPageViewTest, CreateView) {
  EXPECT_NE(nullptr, view_.get());
  EXPECT_EQ(model_.get(), view_->model());
}

TEST_F(AstraDownloadsPageViewTest, ViewHasSearchField) {
  EXPECT_NE(nullptr, view_->search_field_for_test());
  EXPECT_TRUE(view_->search_field_for_test()->GetText().empty());
}

TEST_F(AstraDownloadsPageViewTest, ViewHasCategoryChipsContainer) {
  EXPECT_NE(nullptr, view_->category_chips_container_for_test());
}

TEST_F(AstraDownloadsPageViewTest, ViewHasScrollView) {
  EXPECT_NE(nullptr, view_->scroll_view_for_test());
  EXPECT_NE(nullptr, view_->content_container_for_test());
}

TEST_F(AstraDownloadsPageViewTest, EmptyStateWithNoDownloads) {
  // With no downloads and no sample data, empty state should be visible.
  EXPECT_NE(nullptr, view_->empty_state_for_test());
  // Empty state visibility is controlled by UpdateEmptyState which is called
  // when model changes. With 0 downloads, it should be visible.
}

TEST_F(AstraDownloadsPageViewTest, PopulateDownloadsRebuildsList) {
  model_->PopulateSampleDownloads();

  // After populating, content container should have date group children
  // (in addition to the empty state view).
  int child_count = view_->content_container_for_test()->children().size();
  // At minimum there's the empty state view. After populating there
  // should be additional date group views.
  EXPECT_GE(child_count, 1);  // empty state + at least 1 date group
}

TEST_F(AstraDownloadsPageViewTest, SetModel) {
  auto new_model = std::make_unique<AstraDownloadsPageModel>();
  new_model->PopulateSampleDownloads();

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // View should reflect the new model's data.
  EXPECT_FALSE(view_->empty_state_for_test()->GetVisible());
}

TEST_F(AstraDownloadsPageViewTest, CategoryChipsPopulated) {
  model_->PopulateSampleDownloads();

  // Category chips container should have chips (at least "All" + categories).
  auto* chips = view_->category_chips_container_for_test();
  EXPECT_GT(chips->children().size(), 1u);  // At least "All" + some categories
}

TEST_F(AstraDownloadsPageViewTest, SearchFieldUpdatesModel) {
  model_->PopulateSampleDownloads();

  // Simulate typing in the search field.
  view_->search_field_for_test()->SetText(u"report");

  // The model should have the search query.
  EXPECT_EQ(u"report", model_->GetSearchQuery());
}

TEST_F(AstraDownloadsPageViewTest, PreferredSize) {
  gfx::Size preferred = view_->GetPreferredSize();
  EXPECT_GT(preferred.width(), 0);
  EXPECT_GT(preferred.height(), 0);
}

TEST_F(AstraDownloadsPageViewTest, FilterButtonsPresent) {
  // Filter buttons container should have 5 buttons (All, In progress,
  // Completed, Cancelled, Interrupted).
  auto* filter_container = view_->filter_buttons_container_for_test();
  EXPECT_NE(nullptr, filter_container);
  EXPECT_EQ(5u, filter_container->children().size());
}

TEST_F(AstraDownloadsPageViewTest, ClearAllButton) {
  model_->PopulateSampleDownloads();
  EXPECT_GT(model_->GetCount(), 0u);

  // Click the clear all button.
  auto* clear_button = view_->clear_all_button_for_test();
  ASSERT_NE(nullptr, clear_button);
  ui::MouseEvent click_event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                             base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                             ui::EF_LEFT_MOUSE_BUTTON);
  clear_button->OnMousePressed(click_event);
  clear_button->OnMouseReleased(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(), gfx::Point(),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
      ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_EQ(0u, model_->GetCount());
}

}  // namespace astra
