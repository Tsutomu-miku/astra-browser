// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_history_helper.h"

#include <map>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "astra/browser/astra_history_helper_factory.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestHistoryObserver : public AstraHistoryObserver {
 public:
  void OnHistoryItemAdded(const GURL& url) override {
    item_added_count_++;
    last_added_url_ = url;
  }

  void OnHistoryItemRemoved(const GURL& url) override {
    item_removed_count_++;
    last_removed_url_ = url;
  }

  void OnHistoryItemsCleared() override {
    items_cleared_count_++;
  }

  void OnHistoryExpired() override {
    history_expired_count_++;
  }

  void OnHistorySettingsChanged() override {
    settings_changed_count_++;
  }

  void OnHistoryQueryCompleted(int query_id) override {
    query_completed_count_++;
    last_query_id_ = query_id;
  }

  // Counters
  int item_added_count_ = 0;
  int item_removed_count_ = 0;
  int items_cleared_count_ = 0;
  int history_expired_count_ = 0;
  int settings_changed_count_ = 0;
  int query_completed_count_ = 0;

  // Last recorded values
  GURL last_added_url_;
  GURL last_removed_url_;
  int last_query_id_ = 0;
};

}  // namespace

// Test fixture for AstraHistoryHelper tests.
class HistoryHelperTest : public testing::Test {
 protected:
  HistoryHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraHistoryHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~HistoryHelperTest() override = default;

  void SetUp() override {
    // Verify a few default presentation settings.
    ASSERT_TRUE(helper_->GetShowHistoryInSidebar());
    ASSERT_EQ(helper_->GetHistorySortOrder(), "time_desc");
    ASSERT_EQ(helper_->GetMaxHistoryResults(),
              AstraHistoryHelper::kDefaultMaxHistoryResults);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraHistoryHelper> helper_;
  std::vector<TestHistoryObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Construction and default state
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, DefaultState_ShowInSidebar) {
  EXPECT_TRUE(helper_->GetShowHistoryInSidebar());
}

TEST_F(HistoryHelperTest, DefaultState_SortOrder) {
  EXPECT_EQ(helper_->GetHistorySortOrder(), "time_desc");
}

TEST_F(HistoryHelperTest, DefaultState_MaxResults) {
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kDefaultMaxHistoryResults);
}

TEST_F(HistoryHelperTest, DefaultState_ShowFavicons) {
  EXPECT_TRUE(helper_->GetShowHistoryFavicons());
}

TEST_F(HistoryHelperTest, DefaultState_ShowVisitCount) {
  EXPECT_FALSE(helper_->GetShowVisitCount());
}

TEST_F(HistoryHelperTest, DefaultState_ShowVisitTime) {
  EXPECT_TRUE(helper_->GetShowVisitTime());
}

TEST_F(HistoryHelperTest, DefaultState_DisplayMode) {
  EXPECT_EQ(helper_->GetHistoryDisplayMode(), "list");
}

TEST_F(HistoryHelperTest, DefaultState_GroupByDate) {
  EXPECT_TRUE(helper_->GetGroupHistoryByDate());
}

TEST_F(HistoryHelperTest, DefaultState_ItemsPerDay) {
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(),
            AstraHistoryHelper::kDefaultHistoryItemsPerDay);
}

TEST_F(HistoryHelperTest, DefaultState_ShowTypedUrlsOnly) {
  EXPECT_FALSE(helper_->GetShowTypedUrlsOnly());
}

TEST_F(HistoryHelperTest, DefaultState_DeletionEnabled) {
  EXPECT_TRUE(helper_->GetHistoryDeletionEnabled());
}

TEST_F(HistoryHelperTest, DefaultState_AutoDelete) {
  EXPECT_FALSE(helper_->GetAutoDeleteHistory());
}

TEST_F(HistoryHelperTest, DefaultState_RetentionDays) {
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kDefaultHistoryRetentionDays);
}

TEST_F(HistoryHelperTest, DefaultState_HistoryItemCountZero) {
  // In the overlay, HistoryService is not available, so count is 0.
  EXPECT_EQ(helper_->GetHistoryItemCount(), 0);
}

TEST_F(HistoryHelperTest, DefaultState_VisitsTodayZero) {
  EXPECT_EQ(helper_->GetVisitsToday(), 0);
}

TEST_F(HistoryHelperTest, DefaultState_VisitsThisWeekZero) {
  EXPECT_EQ(helper_->GetVisitsThisWeek(), 0);
}

// ---------------------------------------------------------------------------
// History queries (return empty when no history service)
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Query_MostVisitedEmpty) {
  auto items = helper_->GetMostVisited(10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_RecentHistoryEmpty) {
  auto items = helper_->GetRecentHistory(10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_SearchHistoryEmpty) {
  auto items = helper_->SearchHistory("test", 10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_SearchEmptyString) {
  // Empty search should return recent history (which is empty in overlay).
  auto items = helper_->SearchHistory("", 10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_HistoryForDayEmpty) {
  auto items = helper_->GetHistoryForDay(base::Time::Now());
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_HistoryForRangeEmpty) {
  base::Time now = base::Time::Now();
  auto items = helper_->GetHistoryForRange(now - base::Days(7), now, 10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_TypedUrlsEmpty) {
  auto items = helper_->GetTypedUrls(10);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, Query_GetLastVisitTimeNull) {
  GURL url("https://example.com");
  EXPECT_TRUE(helper_->GetLastVisitTime(url).is_null());
}

TEST_F(HistoryHelperTest, Query_GetVisitCountZero) {
  GURL url("https://example.com");
  EXPECT_EQ(helper_->GetVisitCount(url), 0);
}

TEST_F(HistoryHelperTest, Query_IsUrlInHistoryFalse) {
  GURL url("https://example.com");
  EXPECT_FALSE(helper_->IsUrlInHistory(url));
}

TEST_F(HistoryHelperTest, Query_InvalidURL) {
  GURL url("not-a-valid-url");
  EXPECT_TRUE(helper_->GetLastVisitTime(url).is_null());
  EXPECT_EQ(helper_->GetVisitCount(url), 0);
  EXPECT_FALSE(helper_->IsUrlInHistory(url));
}

TEST_F(HistoryHelperTest, Query_ZeroMaxResults) {
  // max_count = 0 should use the default from prefs.
  auto items = helper_->GetMostVisited(0);
  EXPECT_TRUE(items.empty());
}

// ---------------------------------------------------------------------------
// Presentation settings — show in sidebar
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ShowInSidebar_SetTrue) {
  helper_->SetShowHistoryInSidebar(true);
  EXPECT_TRUE(helper_->GetShowHistoryInSidebar());
}

TEST_F(HistoryHelperTest, Setting_ShowInSidebar_SetFalse) {
  helper_->SetShowHistoryInSidebar(false);
  EXPECT_FALSE(helper_->GetShowHistoryInSidebar());
}

TEST_F(HistoryHelperTest, Setting_ShowInSidebar_Toggle) {
  EXPECT_TRUE(helper_->GetShowHistoryInSidebar());
  bool new_state = helper_->ToggleShowHistoryInSidebar();
  EXPECT_FALSE(new_state);
  EXPECT_FALSE(helper_->GetShowHistoryInSidebar());
  new_state = helper_->ToggleShowHistoryInSidebar();
  EXPECT_TRUE(new_state);
  EXPECT_TRUE(helper_->GetShowHistoryInSidebar());
}

TEST_F(HistoryHelperTest, Setting_ShowInSidebar_Idempotent) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowHistoryInSidebar(true);  // Already true.
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->SetShowHistoryInSidebar(false);  // Changes.
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->SetShowHistoryInSidebar(false);  // Already false.
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — sort order
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_SortOrder_SetTimeDesc) {
  helper_->SetHistorySortOrder("time_desc");
  EXPECT_EQ(helper_->GetHistorySortOrder(), "time_desc");
}

TEST_F(HistoryHelperTest, Setting_SortOrder_SetTimeAsc) {
  helper_->SetHistorySortOrder("time_asc");
  EXPECT_EQ(helper_->GetHistorySortOrder(), "time_asc");
}

TEST_F(HistoryHelperTest, Setting_SortOrder_SetMostVisited) {
  helper_->SetHistorySortOrder("most_visited");
  EXPECT_EQ(helper_->GetHistorySortOrder(), "most_visited");
}

TEST_F(HistoryHelperTest, Setting_SortOrder_NotifiesOnChange) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistorySortOrder("most_visited");
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Setting_SortOrder_Idempotent) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistorySortOrder("time_desc");  // Already default.
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — max results with clamping
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_MaxResults_SetValid) {
  helper_->SetMaxHistoryResults(100);
  EXPECT_EQ(helper_->GetMaxHistoryResults(), 100);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_ClampMin) {
  helper_->SetMaxHistoryResults(0);
  // Should be clamped to minimum.
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kMinHistoryResults);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_ClampBelowMin) {
  helper_->SetMaxHistoryResults(-10);
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kMinHistoryResults);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_ClampMax) {
  helper_->SetMaxHistoryResults(1000);
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kMaxHistoryResults);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_AtMin) {
  helper_->SetMaxHistoryResults(AstraHistoryHelper::kMinHistoryResults);
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kMinHistoryResults);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_AtMax) {
  helper_->SetMaxHistoryResults(AstraHistoryHelper::kMaxHistoryResults);
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kMaxHistoryResults);
}

TEST_F(HistoryHelperTest, Setting_MaxResults_NotifiesOnChange) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMaxHistoryResults(100);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — show favicons
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ShowFavicons_SetFalse) {
  helper_->SetShowHistoryFavicons(false);
  EXPECT_FALSE(helper_->GetShowHistoryFavicons());
}

TEST_F(HistoryHelperTest, Setting_ShowFavicons_SetTrue) {
  helper_->SetShowHistoryFavicons(false);
  helper_->SetShowHistoryFavicons(true);
  EXPECT_TRUE(helper_->GetShowHistoryFavicons());
}

TEST_F(HistoryHelperTest, Setting_ShowFavicons_Toggle) {
  EXPECT_TRUE(helper_->GetShowHistoryFavicons());
  helper_->ToggleShowHistoryFavicons();
  EXPECT_FALSE(helper_->GetShowHistoryFavicons());
  helper_->ToggleShowHistoryFavicons();
  EXPECT_TRUE(helper_->GetShowHistoryFavicons());
}

// ---------------------------------------------------------------------------
// Presentation settings — show visit count
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ShowVisitCount_SetTrue) {
  helper_->SetShowVisitCount(true);
  EXPECT_TRUE(helper_->GetShowVisitCount());
}

TEST_F(HistoryHelperTest, Setting_ShowVisitCount_Toggle) {
  EXPECT_FALSE(helper_->GetShowVisitCount());
  helper_->ToggleShowVisitCount();
  EXPECT_TRUE(helper_->GetShowVisitCount());
  helper_->ToggleShowVisitCount();
  EXPECT_FALSE(helper_->GetShowVisitCount());
}

// ---------------------------------------------------------------------------
// Presentation settings — show visit time
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ShowVisitTime_SetFalse) {
  helper_->SetShowVisitTime(false);
  EXPECT_FALSE(helper_->GetShowVisitTime());
}

TEST_F(HistoryHelperTest, Setting_ShowVisitTime_Toggle) {
  EXPECT_TRUE(helper_->GetShowVisitTime());
  helper_->ToggleShowVisitTime();
  EXPECT_FALSE(helper_->GetShowVisitTime());
  helper_->ToggleShowVisitTime();
  EXPECT_TRUE(helper_->GetShowVisitTime());
}

// ---------------------------------------------------------------------------
// Presentation settings — display mode
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_DisplayMode_List) {
  helper_->SetHistoryDisplayMode("list");
  EXPECT_EQ(helper_->GetHistoryDisplayMode(), "list");
}

TEST_F(HistoryHelperTest, Setting_DisplayMode_Compact) {
  helper_->SetHistoryDisplayMode("compact");
  EXPECT_EQ(helper_->GetHistoryDisplayMode(), "compact");
}

TEST_F(HistoryHelperTest, Setting_DisplayMode_Card) {
  helper_->SetHistoryDisplayMode("card");
  EXPECT_EQ(helper_->GetHistoryDisplayMode(), "card");
}

TEST_F(HistoryHelperTest, Setting_DisplayMode_NotifiesOnChange) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistoryDisplayMode("compact");
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — group by date
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_GroupByDate_SetFalse) {
  helper_->SetGroupHistoryByDate(false);
  EXPECT_FALSE(helper_->GetGroupHistoryByDate());
}

TEST_F(HistoryHelperTest, Setting_GroupByDate_Toggle) {
  EXPECT_TRUE(helper_->GetGroupHistoryByDate());
  helper_->ToggleGroupHistoryByDate();
  EXPECT_FALSE(helper_->GetGroupHistoryByDate());
  helper_->ToggleGroupHistoryByDate();
  EXPECT_TRUE(helper_->GetGroupHistoryByDate());
}

// ---------------------------------------------------------------------------
// Presentation settings — items per day with clamping
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ItemsPerDay_SetValid) {
  helper_->SetHistoryItemsPerDay(50);
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(), 50);
}

TEST_F(HistoryHelperTest, Setting_ItemsPerDay_ClampMin) {
  helper_->SetHistoryItemsPerDay(0);
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(),
            AstraHistoryHelper::kMinHistoryItemsPerDay);
}

TEST_F(HistoryHelperTest, Setting_ItemsPerDay_ClampMax) {
  helper_->SetHistoryItemsPerDay(200);
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(),
            AstraHistoryHelper::kMaxHistoryItemsPerDay);
}

TEST_F(HistoryHelperTest, Setting_ItemsPerDay_AtMin) {
  helper_->SetHistoryItemsPerDay(AstraHistoryHelper::kMinHistoryItemsPerDay);
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(),
            AstraHistoryHelper::kMinHistoryItemsPerDay);
}

TEST_F(HistoryHelperTest, Setting_ItemsPerDay_AtMax) {
  helper_->SetHistoryItemsPerDay(AstraHistoryHelper::kMaxHistoryItemsPerDay);
  EXPECT_EQ(helper_->GetHistoryItemsPerDay(),
            AstraHistoryHelper::kMaxHistoryItemsPerDay);
}

// ---------------------------------------------------------------------------
// Presentation settings — show typed URLs only
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_ShowTypedUrlsOnly_SetTrue) {
  helper_->SetShowTypedUrlsOnly(true);
  EXPECT_TRUE(helper_->GetShowTypedUrlsOnly());
}

TEST_F(HistoryHelperTest, Setting_ShowTypedUrlsOnly_Toggle) {
  EXPECT_FALSE(helper_->GetShowTypedUrlsOnly());
  helper_->ToggleShowTypedUrlsOnly();
  EXPECT_TRUE(helper_->GetShowTypedUrlsOnly());
  helper_->ToggleShowTypedUrlsOnly();
  EXPECT_FALSE(helper_->GetShowTypedUrlsOnly());
}

// ---------------------------------------------------------------------------
// Presentation settings — deletion enabled
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_DeletionEnabled_SetFalse) {
  helper_->SetHistoryDeletionEnabled(false);
  EXPECT_FALSE(helper_->GetHistoryDeletionEnabled());
}

TEST_F(HistoryHelperTest, Setting_DeletionEnabled_Toggle) {
  EXPECT_TRUE(helper_->GetHistoryDeletionEnabled());
  helper_->ToggleHistoryDeletionEnabled();
  EXPECT_FALSE(helper_->GetHistoryDeletionEnabled());
  helper_->ToggleHistoryDeletionEnabled();
  EXPECT_TRUE(helper_->GetHistoryDeletionEnabled());
}

// ---------------------------------------------------------------------------
// Presentation settings — auto delete
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_AutoDelete_SetTrue) {
  helper_->SetAutoDeleteHistory(true);
  EXPECT_TRUE(helper_->GetAutoDeleteHistory());
}

TEST_F(HistoryHelperTest, Setting_AutoDelete_Toggle) {
  EXPECT_FALSE(helper_->GetAutoDeleteHistory());
  helper_->ToggleAutoDeleteHistory();
  EXPECT_TRUE(helper_->GetAutoDeleteHistory());
  helper_->ToggleAutoDeleteHistory();
  EXPECT_FALSE(helper_->GetAutoDeleteHistory());
}

// ---------------------------------------------------------------------------
// Presentation settings — retention days with clamping
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Setting_RetentionDays_SetValid) {
  helper_->SetHistoryRetentionDays(30);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(), 30);
}

TEST_F(HistoryHelperTest, Setting_RetentionDays_ClampMin) {
  helper_->SetHistoryRetentionDays(0);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kMinRetentionDays);
}

TEST_F(HistoryHelperTest, Setting_RetentionDays_ClampNegative) {
  helper_->SetHistoryRetentionDays(-5);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kMinRetentionDays);
}

TEST_F(HistoryHelperTest, Setting_RetentionDays_ClampMax) {
  helper_->SetHistoryRetentionDays(10000);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kMaxRetentionDays);
}

TEST_F(HistoryHelperTest, Setting_RetentionDays_AtMin) {
  helper_->SetHistoryRetentionDays(AstraHistoryHelper::kMinRetentionDays);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kMinRetentionDays);
}

TEST_F(HistoryHelperTest, Setting_RetentionDays_AtMax) {
  helper_->SetHistoryRetentionDays(AstraHistoryHelper::kMaxRetentionDays);
  EXPECT_EQ(helper_->GetHistoryRetentionDays(),
            AstraHistoryHelper::kMaxRetentionDays);
}

// ---------------------------------------------------------------------------
// Settings persistence round-trip via PrefService
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Persistence_ShowInSidebar_RoundTrip) {
  helper_->SetShowHistoryInSidebar(false);
  // Create a new helper reading from the same profile.
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowHistoryInSidebar());
}

TEST_F(HistoryHelperTest, Persistence_SortOrder_RoundTrip) {
  helper_->SetHistorySortOrder("most_visited");
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_EQ(helper2->GetHistorySortOrder(), "most_visited");
}

TEST_F(HistoryHelperTest, Persistence_MaxResults_RoundTrip) {
  helper_->SetMaxHistoryResults(200);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_EQ(helper2->GetMaxHistoryResults(), 200);
}

TEST_F(HistoryHelperTest, Persistence_ShowFavicons_RoundTrip) {
  helper_->SetShowHistoryFavicons(false);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowHistoryFavicons());
}

TEST_F(HistoryHelperTest, Persistence_ShowVisitCount_RoundTrip) {
  helper_->SetShowVisitCount(true);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetShowVisitCount());
}

TEST_F(HistoryHelperTest, Persistence_ShowVisitTime_RoundTrip) {
  helper_->SetShowVisitTime(false);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowVisitTime());
}

TEST_F(HistoryHelperTest, Persistence_DisplayMode_RoundTrip) {
  helper_->SetHistoryDisplayMode("card");
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_EQ(helper2->GetHistoryDisplayMode(), "card");
}

TEST_F(HistoryHelperTest, Persistence_GroupByDate_RoundTrip) {
  helper_->SetGroupHistoryByDate(false);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetGroupHistoryByDate());
}

TEST_F(HistoryHelperTest, Persistence_ItemsPerDay_RoundTrip) {
  helper_->SetHistoryItemsPerDay(50);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_EQ(helper2->GetHistoryItemsPerDay(), 50);
}

TEST_F(HistoryHelperTest, Persistence_ShowTypedUrlsOnly_RoundTrip) {
  helper_->SetShowTypedUrlsOnly(true);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetShowTypedUrlsOnly());
}

TEST_F(HistoryHelperTest, Persistence_DeletionEnabled_RoundTrip) {
  helper_->SetHistoryDeletionEnabled(false);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetHistoryDeletionEnabled());
}

TEST_F(HistoryHelperTest, Persistence_AutoDelete_RoundTrip) {
  helper_->SetAutoDeleteHistory(true);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetAutoDeleteHistory());
}

TEST_F(HistoryHelperTest, Persistence_RetentionDays_RoundTrip) {
  helper_->SetHistoryRetentionDays(30);
  auto helper2 = std::make_unique<AstraHistoryHelper>(profile_.get());
  EXPECT_EQ(helper2->GetHistoryRetentionDays(), 30);
}

// ---------------------------------------------------------------------------
// Observer notifications
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Observer_OnHistoryItemAdded) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  GURL url("https://example.com");
  helper_->NotifyHistoryItemAdded(url);

  EXPECT_EQ(observer.item_added_count_, 1);
  EXPECT_EQ(observer.last_added_url_, url);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_OnHistoryItemRemoved) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  GURL url("https://example.com");
  helper_->NotifyHistoryItemRemoved(url);

  EXPECT_EQ(observer.item_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_url_, url);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_OnHistoryItemsCleared) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyHistoryItemsCleared();

  EXPECT_EQ(observer.items_cleared_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_OnHistoryExpired) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyHistoryExpired();

  EXPECT_EQ(observer.history_expired_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_OnHistorySettingsChanged) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowHistoryInSidebar(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_OnHistoryQueryCompleted) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyHistoryQueryCompleted(42);

  EXPECT_EQ(observer.query_completed_count_, 1);
  EXPECT_EQ(observer.last_query_id_, 42);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_MultipleItemAddedNotifications) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyHistoryItemAdded(GURL("https://a.com"));
  helper_->NotifyHistoryItemAdded(GURL("https://b.com"));
  helper_->NotifyHistoryItemAdded(GURL("https://c.com"));

  EXPECT_EQ(observer.item_added_count_, 3);
  EXPECT_EQ(observer.last_added_url_, GURL("https://c.com"));

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer defaults — empty implementations don't crash
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraHistoryObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths.
  helper_->NotifyHistoryItemAdded(GURL("https://example.com"));
  helper_->NotifyHistoryItemRemoved(GURL("https://example.com"));
  helper_->NotifyHistoryItemsCleared();
  helper_->NotifyHistoryExpired();
  helper_->SetShowHistoryInSidebar(false);
  helper_->NotifyHistoryQueryCompleted(1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, MultipleObservers_AllNotified) {
  TestHistoryObserver observer1;
  TestHistoryObserver observer2;
  TestHistoryObserver observer3;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);
  helper_->AddObserver(&observer3);

  GURL url("https://example.com");
  helper_->NotifyHistoryItemAdded(url);

  EXPECT_EQ(observer1.item_added_count_, 1);
  EXPECT_EQ(observer2.item_added_count_, 1);
  EXPECT_EQ(observer3.item_added_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
  helper_->RemoveObserver(&observer3);
}

TEST_F(HistoryHelperTest, MultipleObservers_SettingsChange) {
  TestHistoryObserver observer1;
  TestHistoryObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->SetShowHistoryFavicons(false);

  EXPECT_EQ(observer1.settings_changed_count_, 1);
  EXPECT_EQ(observer2.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
}

TEST_F(HistoryHelperTest, RemoveObserver_StopsNotifications) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyHistoryItemsCleared();
  EXPECT_EQ(observer.items_cleared_count_, 1);

  helper_->RemoveObserver(&observer);

  helper_->NotifyHistoryItemsCleared();
  EXPECT_EQ(observer.items_cleared_count_, 1);  // Still 1.
}

TEST_F(HistoryHelperTest, AddNullObserver_Safe) {
  // Adding a null observer should not crash.
  helper_->AddObserver(nullptr);
  // Removing a null observer should not crash.
  helper_->RemoveObserver(nullptr);
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Bulk_RemoveUrls_EmptyVector) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  std::vector<GURL> empty_urls;
  helper_->RemoveUrls(empty_urls);

  // Should not crash, and no notifications.
  EXPECT_EQ(observer.item_removed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_RemoveUrls_Multiple) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  std::vector<GURL> urls = {
    GURL("https://a.com"),
    GURL("https://b.com"),
    GURL("https://c.com"),
  };
  helper_->RemoveUrls(urls);

  // Each valid URL triggers a notification.
  EXPECT_EQ(observer.item_removed_count_, 3);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_RemoveUrls_WithInvalid) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  std::vector<GURL> urls = {
    GURL("https://valid.com"),
    GURL(""),  // Invalid
    GURL("https://valid2.com"),
  };
  helper_->RemoveUrls(urls);

  // Only valid URLs trigger notifications.
  EXPECT_EQ(observer.item_removed_count_, 2);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_ClearLastDays_ZeroDays) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearHistoryForLastDays(0);
  // Zero days should be a no-op.
  EXPECT_EQ(observer.history_expired_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_ClearLastDays_NegativeDays) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearHistoryForLastDays(-5);
  // Negative days should be a no-op.
  EXPECT_EQ(observer.history_expired_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_ClearLastDays_OneDay) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearHistoryForLastDays(1);
  EXPECT_EQ(observer.history_expired_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_DeleteOldHistory_Enabled) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->DeleteOldHistory();
  // Should fire expired notification.
  EXPECT_EQ(observer.history_expired_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_DeleteOldHistory_DeletionDisabled) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistoryDeletionEnabled(false);
  helper_->DeleteOldHistory();

  // No notification when deletion is disabled.
  EXPECT_EQ(observer.history_expired_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_ExpireByRetention_AutoDeleteOn) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetAutoDeleteHistory(true);
  helper_->ExpireHistoryByRetention();

  EXPECT_EQ(observer.history_expired_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Bulk_ExpireByRetention_AutoDeleteOff) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  // auto_delete is false by default.
  helper_->ExpireHistoryByRetention();

  // No notification when auto delete is off.
  EXPECT_EQ(observer.history_expired_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Utility methods — FormatVisitTime
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_FormatVisitTime_NullTime) {
  auto result = AstraHistoryHelper::FormatVisitTime(base::Time());
  EXPECT_EQ(result, u"Never");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_JustNow) {
  auto result = AstraHistoryHelper::FormatVisitTime(base::Time::Now());
  EXPECT_EQ(result, u"Just now");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_OneMinuteAgo) {
  base::Time time = base::Time::Now() - base::Seconds(45);
  auto result = AstraHistoryHelper::FormatVisitTime(time);
  // Less than 1 minute = "Just now"
  EXPECT_EQ(result, u"Just now");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_FiveMinutesAgo) {
  base::Time time = base::Time::Now() - base::Minutes(5);
  auto result = AstraHistoryHelper::FormatVisitTime(time);
  EXPECT_EQ(result, u"5 minutes ago");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_OneHourAgo) {
  base::Time time = base::Time::Now() - base::Minutes(60);
  auto result = AstraHistoryHelper::FormatVisitTime(time);
  EXPECT_EQ(result, u"1 hour ago");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_SeveralHoursAgo) {
  base::Time time = base::Time::Now() - base::Hours(3);
  auto result = AstraHistoryHelper::FormatVisitTime(time);
  EXPECT_EQ(result, u"3 hours ago");
}

TEST_F(HistoryHelperTest, Util_FormatVisitTime_FutureTime) {
  base::Time time = base::Time::Now() + base::Hours(1);
  auto result = AstraHistoryHelper::FormatVisitTime(time);
  EXPECT_EQ(result, u"Just now");
}

// ---------------------------------------------------------------------------
// Utility methods — FormatRelativeTime
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Zero) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::TimeDelta());
  EXPECT_EQ(result, u"0 seconds");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Negative) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Seconds(-10));
  EXPECT_EQ(result, u"0 seconds");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Seconds) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Seconds(45));
  EXPECT_EQ(result, u"45 seconds");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_OneSecond) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Seconds(1));
  EXPECT_EQ(result, u"1 second");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Minutes) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Minutes(10));
  EXPECT_EQ(result, u"10 minutes");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_OneMinute) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Minutes(1));
  EXPECT_EQ(result, u"1 minute");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Hours) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Hours(5));
  EXPECT_EQ(result, u"5 hours");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_OneHour) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Hours(1));
  EXPECT_EQ(result, u"1 hour");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Days) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Days(7));
  EXPECT_EQ(result, u"7 days");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_OneDay) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Days(1));
  EXPECT_EQ(result, u"1 day");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Months) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Days(90));
  EXPECT_EQ(result, u"3 months");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_Years) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Days(730));
  EXPECT_EQ(result, u"2 years");
}

TEST_F(HistoryHelperTest, Util_FormatRelativeTime_OneYear) {
  auto result = AstraHistoryHelper::FormatRelativeTime(base::Days(365));
  EXPECT_EQ(result, u"1 year");
}

// ---------------------------------------------------------------------------
// Utility methods — TruncateTitle
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_TruncateTitle_ShortTitle) {
  std::u16string title = u"Hello";
  auto result = AstraHistoryHelper::TruncateTitle(title, 20);
  EXPECT_EQ(result, u"Hello");
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_ExactLength) {
  std::u16string title = u"Hello";
  auto result = AstraHistoryHelper::TruncateTitle(title, 5);
  EXPECT_EQ(result, u"Hello");
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_LongTitle) {
  std::u16string title = u"Hello World";
  auto result = AstraHistoryHelper::TruncateTitle(title, 5);
  EXPECT_EQ(result, u"Hell…");
  EXPECT_EQ(result.size(), 5u);
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_ZeroMaxLength) {
  std::u16string title = u"Hello";
  auto result = AstraHistoryHelper::TruncateTitle(title, 0);
  EXPECT_EQ(result, u"");
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_NegativeMaxLength) {
  std::u16string title = u"Hello";
  auto result = AstraHistoryHelper::TruncateTitle(title, -5);
  EXPECT_EQ(result, u"");
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_MaxLengthOne) {
  std::u16string title = u"Hello";
  auto result = AstraHistoryHelper::TruncateTitle(title, 1);
  EXPECT_EQ(result, u"…");
}

TEST_F(HistoryHelperTest, Util_TruncateTitle_EmptyTitle) {
  std::u16string title;
  auto result = AstraHistoryHelper::TruncateTitle(title, 10);
  EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Utility methods — GetDomainName
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_GetDomainName_StandardUrl) {
  GURL url("https://www.example.com/path/to/page");
  auto result = AstraHistoryHelper::GetDomainName(url);
  EXPECT_EQ(result, "example.com");
}

TEST_F(HistoryHelperTest, Util_GetDomainName_NoWww) {
  GURL url("https://example.com/path");
  auto result = AstraHistoryHelper::GetDomainName(url);
  EXPECT_EQ(result, "example.com");
}

TEST_F(HistoryHelperTest, Util_GetDomainName_Subdomain) {
  GURL url("https://blog.example.com/article");
  auto result = AstraHistoryHelper::GetDomainName(url);
  // Note: simple implementation doesn't strip subdomains beyond "www."
  EXPECT_EQ(result, "blog.example.com");
}

TEST_F(HistoryHelperTest, Util_GetDomainName_InvalidUrl) {
  GURL url("");
  auto result = AstraHistoryHelper::GetDomainName(url);
  EXPECT_EQ(result, "");
}

TEST_F(HistoryHelperTest, Util_GetDomainName_FileUrl) {
  GURL url("file:///path/to/file.html");
  auto result = AstraHistoryHelper::GetDomainName(url);
  EXPECT_EQ(result, "");
}

TEST_F(HistoryHelperTest, Util_GetDomainName_WwwCaseInsensitive) {
  GURL url("https://WWW.Example.COM/");
  auto result = AstraHistoryHelper::GetDomainName(url);
  EXPECT_EQ(result, "Example.COM");
}

// ---------------------------------------------------------------------------
// Utility methods — IsSameDay
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_IsSameDay_SameTime) {
  base::Time now = base::Time::Now();
  EXPECT_TRUE(AstraHistoryHelper::IsSameDay(now, now));
}

TEST_F(HistoryHelperTest, Util_IsSameDay_SameDayDifferentTime) {
  base::Time morning = base::Time::Now().LocalMidnight() + base::Hours(8);
  base::Time evening = base::Time::Now().LocalMidnight() + base::Hours(20);
  EXPECT_TRUE(AstraHistoryHelper::IsSameDay(morning, evening));
}

TEST_F(HistoryHelperTest, Util_IsSameDay_DifferentDays) {
  base::Time today = base::Time::Now();
  base::Time yesterday = today - base::Days(1);
  EXPECT_FALSE(AstraHistoryHelper::IsSameDay(today, yesterday));
}

TEST_F(HistoryHelperTest, Util_IsSameDay_NullTime) {
  base::Time now = base::Time::Now();
  EXPECT_FALSE(AstraHistoryHelper::IsSameDay(now, base::Time()));
  EXPECT_FALSE(AstraHistoryHelper::IsSameDay(base::Time(), now));
  EXPECT_FALSE(AstraHistoryHelper::IsSameDay(base::Time(), base::Time()));
}

TEST_F(HistoryHelperTest, Util_IsSameDay_ExactlyMidnight) {
  base::Time day1_midnight = base::Time::Now().LocalMidnight();
  base::Time day1_end = day1_midnight + base::Hours(23) + base::Minutes(59);
  EXPECT_TRUE(AstraHistoryHelper::IsSameDay(day1_midnight, day1_end));

  base::Time day2_midnight = day1_midnight + base::Days(1);
  EXPECT_FALSE(AstraHistoryHelper::IsSameDay(day1_midnight, day2_midnight));
}

// ---------------------------------------------------------------------------
// Utility methods — GroupByDate
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Util_GroupByDate_EmptyVector) {
  std::vector<AstraHistoryItem> items;
  auto groups = AstraHistoryHelper::GroupByDate(items);
  EXPECT_TRUE(groups.empty());
}

TEST_F(HistoryHelperTest, Util_GroupByDate_SingleItem) {
  std::vector<AstraHistoryItem> items;
  AstraHistoryItem item;
  item.url = GURL("https://example.com");
  item.title = u"Example";
  item.visit_time = base::Time::Now();
  items.push_back(item);

  auto groups = AstraHistoryHelper::GroupByDate(items);
  EXPECT_EQ(groups.size(), 1u);
  EXPECT_EQ(groups.begin()->second.size(), 1u);
  EXPECT_EQ(groups.begin()->second[0].url, GURL("https://example.com"));
}

TEST_F(HistoryHelperTest, Util_GroupByDate_SameDay) {
  std::vector<AstraHistoryItem> items;
  base::Time base_time = base::Time::Now().LocalMidnight();

  AstraHistoryItem item1;
  item1.url = GURL("https://a.com");
  item1.visit_time = base_time + base::Hours(10);
  items.push_back(item1);

  AstraHistoryItem item2;
  item2.url = GURL("https://b.com");
  item2.visit_time = base_time + base::Hours(14);
  items.push_back(item2);

  auto groups = AstraHistoryHelper::GroupByDate(items);
  EXPECT_EQ(groups.size(), 1u);
  EXPECT_EQ(groups.begin()->second.size(), 2u);
  // Should be sorted with most recent first.
  EXPECT_EQ(groups.begin()->second[0].url, GURL("https://b.com"));
  EXPECT_EQ(groups.begin()->second[1].url, GURL("https://a.com"));
}

TEST_F(HistoryHelperTest, Util_GroupByDate_MultipleDays) {
  std::vector<AstraHistoryItem> items;
  base::Time today = base::Time::Now().LocalMidnight();
  base::Time yesterday = today - base::Days(1);
  base::Time two_days_ago = today - base::Days(2);

  AstraHistoryItem item1;
  item1.url = GURL("https://today.com");
  item1.visit_time = today + base::Hours(10);
  items.push_back(item1);

  AstraHistoryItem item2;
  item2.url = GURL("https://yesterday.com");
  item2.visit_time = yesterday + base::Hours(10);
  items.push_back(item2);

  AstraHistoryItem item3;
  item3.url = GURL("https://older.com");
  item3.visit_time = two_days_ago + base::Hours(10);
  items.push_back(item3);

  auto groups = AstraHistoryHelper::GroupByDate(items);
  EXPECT_EQ(groups.size(), 3u);
}

TEST_F(HistoryHelperTest, Util_GroupByDate_NullVisitTimeSkipped) {
  std::vector<AstraHistoryItem> items;

  AstraHistoryItem item1;
  item1.url = GURL("https://valid.com");
  item1.visit_time = base::Time::Now();
  items.push_back(item1);

  AstraHistoryItem item2;
  item2.url = GURL("https://null-time.com");
  item2.visit_time = base::Time();  // Null time
  items.push_back(item2);

  auto groups = AstraHistoryHelper::GroupByDate(items);
  EXPECT_EQ(groups.size(), 1u);  // Only the valid one
  EXPECT_EQ(groups.begin()->second.size(), 1u);
  EXPECT_EQ(groups.begin()->second[0].url, GURL("https://valid.com"));
}

TEST_F(HistoryHelperTest, Util_GroupByDate_ItemsSortedWithinDay) {
  std::vector<AstraHistoryItem> items;
  base::Time day_start = base::Time::Now().LocalMidnight();

  // Add in reverse chronological order.
  AstraHistoryItem item1;
  item1.url = GURL("https://earlier.com");
  item1.visit_time = day_start + base::Hours(9);
  items.push_back(item1);

  AstraHistoryItem item2;
  item2.url = GURL("https://later.com");
  item2.visit_time = day_start + base::Hours(15);
  items.push_back(item2);

  AstraHistoryItem item3;
  item3.url = GURL("https://midday.com");
  item3.visit_time = day_start + base::Hours(12);
  items.push_back(item3);

  auto groups = AstraHistoryHelper::GroupByDate(items);
  ASSERT_EQ(groups.size(), 1u);
  auto& day_items = groups.begin()->second;
  ASSERT_EQ(day_items.size(), 3u);
  // Should be sorted most recent first.
  EXPECT_EQ(day_items[0].url, GURL("https://later.com"));
  EXPECT_EQ(day_items[1].url, GURL("https://midday.com"));
  EXPECT_EQ(day_items[2].url, GURL("https://earlier.com"));
}

// ---------------------------------------------------------------------------
// History item operations
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Operation_RemoveHistoryItem_ValidUrl) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  GURL url("https://example.com");
  helper_->RemoveHistoryItem(url);

  EXPECT_EQ(observer.item_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_url_, url);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_RemoveHistoryItem_InvalidUrl) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  GURL url("");
  helper_->RemoveHistoryItem(url);

  // Invalid URL should not trigger notification.
  EXPECT_EQ(observer.item_removed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_RemoveHistoryItem_DeletionDisabled) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistoryDeletionEnabled(false);
  GURL url("https://example.com");
  helper_->RemoveHistoryItem(url);

  EXPECT_EQ(observer.item_removed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_ClearAllHistory) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearAllHistory();

  EXPECT_EQ(observer.items_cleared_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_ClearAllHistory_DeletionDisabled) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistoryDeletionEnabled(false);
  helper_->ClearAllHistory();

  EXPECT_EQ(observer.items_cleared_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_RemoveRange_Valid) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  base::Time now = base::Time::Now();
  helper_->RemoveHistoryForRange(now - base::Days(7), now);

  EXPECT_EQ(observer.history_expired_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Operation_RemoveRange_DeletionDisabled) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHistoryDeletionEnabled(false);
  base::Time now = base::Time::Now();
  helper_->RemoveHistoryForRange(now - base::Days(7), now);

  EXPECT_EQ(observer.history_expired_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, EdgeCase_EmptyHistory_AllQueries) {
  // With no history service, all queries should return empty/zero/null
  // without crashing.
  EXPECT_EQ(helper_->GetHistoryItemCount(), 0);
  EXPECT_EQ(helper_->GetVisitsToday(), 0);
  EXPECT_EQ(helper_->GetVisitsThisWeek(), 0);
  EXPECT_TRUE(helper_->GetMostVisited(10).empty());
  EXPECT_TRUE(helper_->GetRecentHistory(10).empty());
  EXPECT_TRUE(helper_->SearchHistory("anything", 10).empty());
  EXPECT_TRUE(helper_->GetTypedUrls(10).empty());
}

TEST_F(HistoryHelperTest, EdgeCase_InvalidUrlQueries) {
  GURL invalid_url("not a url");
  EXPECT_TRUE(helper_->GetLastVisitTime(invalid_url).is_null());
  EXPECT_EQ(helper_->GetVisitCount(invalid_url), 0);
  EXPECT_FALSE(helper_->IsUrlInHistory(invalid_url));
}

TEST_F(HistoryHelperTest, EdgeCase_NullTime) {
  // GetHistoryForDay with null time should not crash.
  auto items = helper_->GetHistoryForDay(base::Time());
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, EdgeCase_ZeroMaxResultsInQuery) {
  // max_results = 0 should not crash.
  auto items = helper_->GetRecentHistory(0);
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, EdgeCase_NegativeMaxResults) {
  // Negative max results should not crash.
  auto items = helper_->GetMostVisited(-5);
  // They get clamped to min.
  EXPECT_TRUE(items.empty());
}

TEST_F(HistoryHelperTest, EdgeCase_OutOfRangeDates) {
  base::Time min_time = base::Time::Min();
  base::Time max_time = base::Time::Max();
  auto items = helper_->GetHistoryForRange(min_time, max_time, 10);
  EXPECT_TRUE(items.empty());
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Factory_GetForProfile) {
  // Using TestingProfile, the factory should return a valid instance.
  AstraHistoryHelper* helper =
      AstraHistoryHelperFactory::GetForProfile(profile_.get());
  EXPECT_NE(helper, nullptr);
}

TEST_F(HistoryHelperTest, Factory_GetForProfile_NullProfile) {
  AstraHistoryHelper* helper =
      AstraHistoryHelperFactory::GetForProfile(nullptr);
  EXPECT_EQ(helper, nullptr);
}

TEST_F(HistoryHelperTest, Factory_GetInstance) {
  // The factory singleton should exist.
  auto* factory = AstraHistoryHelperFactory::GetInstance();
  EXPECT_NE(factory, nullptr);
  // Calling again should return the same instance.
  auto* factory2 = AstraHistoryHelperFactory::GetInstance();
  EXPECT_EQ(factory, factory2);
}

TEST_F(HistoryHelperTest, Factory_RegisterProfilePrefs) {
  // Create a fresh profile and register prefs via the factory.
  TestingProfile profile;
  PrefRegistrySimple* registry = profile.GetTestingPrefService()->registry();
  AstraHistoryHelperFactory::RegisterProfilePrefs(registry);

  // Check that prefs are registered with correct defaults.
  EXPECT_TRUE(profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryShowInSidebar));
  EXPECT_EQ(profile.GetPrefs()->GetString(prefs::kPrefHistorySortOrder),
            "time_desc");
  EXPECT_EQ(profile.GetPrefs()->GetInteger(prefs::kPrefHistoryMaxResults),
            50);
  EXPECT_TRUE(profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryShowFavicons));
  EXPECT_FALSE(
      profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryShowVisitCount));
  EXPECT_TRUE(profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryShowVisitTime));
  EXPECT_EQ(profile.GetPrefs()->GetString(prefs::kPrefHistoryDisplayMode),
            "list");
  EXPECT_TRUE(
      profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryGroupByDate));
  EXPECT_EQ(profile.GetPrefs()->GetInteger(prefs::kPrefHistoryItemsPerDay),
            20);
  EXPECT_FALSE(
      profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryShowTypedUrlsOnly));
  EXPECT_TRUE(
      profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryDeletionEnabled));
  EXPECT_FALSE(profile.GetPrefs()->GetBoolean(prefs::kPrefHistoryAutoDelete));
  EXPECT_EQ(
      profile.GetPrefs()->GetInteger(prefs::kPrefHistoryRetentionDays), 90);
}

// ---------------------------------------------------------------------------
// Shutdown cleanup
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Shutdown_ClearsObservers) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  helper_->Shutdown();

  // After shutdown, notification should not reach the observer.
  helper_->NotifyHistorySettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 0);
}

TEST_F(HistoryHelperTest, Shutdown_CalledMultipleTimes) {
  // Shutdown should be idempotent.
  helper_->Shutdown();
  helper_->Shutdown();
  // Should not crash.
  SUCCEED();
}

TEST_F(HistoryHelperTest, Shutdown_ProfileCleared) {
  helper_->Shutdown();
  // After shutdown, operations that need profile/prefs should not crash.
  // They should return default values or empty results.
  EXPECT_TRUE(helper_->GetShowHistoryInSidebar());  // Uses fallback default
  EXPECT_EQ(helper_->GetMaxHistoryResults(),
            AstraHistoryHelper::kDefaultMaxHistoryResults);
}

// ---------------------------------------------------------------------------
// AstraHistoryItem struct
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, HistoryItem_DefaultValues) {
  AstraHistoryItem item;
  EXPECT_FALSE(item.url.is_valid());
  EXPECT_TRUE(item.title.empty());
  EXPECT_TRUE(item.visit_time.is_null());
  EXPECT_EQ(item.visit_count, 0);
  EXPECT_FALSE(item.is_bookmarked);
  EXPECT_FALSE(item.is_typed);
  EXPECT_EQ(item.transition_type, 0);
  EXPECT_FALSE(item.favicon_url.is_valid());
}

TEST_F(HistoryHelperTest, HistoryQuery_DefaultValues) {
  AstraHistoryQuery query;
  EXPECT_TRUE(query.search_text.empty());
  EXPECT_TRUE(query.begin_time.is_min());
  EXPECT_TRUE(query.end_time.is_max());
  EXPECT_EQ(query.max_results, 0);
  EXPECT_FALSE(query.only_typed);
  EXPECT_EQ(query.sort_by, AstraHistoryQuery::kByTime);
}

// ---------------------------------------------------------------------------
// Additional setting test — toggles return correct value
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, ToggleShowHistoryInSidebar_ReturnsNewState) {
  bool result = helper_->ToggleShowHistoryInSidebar();
  EXPECT_FALSE(result);
  result = helper_->ToggleShowHistoryInSidebar();
  EXPECT_TRUE(result);
}

TEST_F(HistoryHelperTest, ToggleShowHistoryFavicons_ReturnsNewState) {
  bool result = helper_->ToggleShowHistoryFavicons();
  EXPECT_FALSE(result);
  result = helper_->ToggleShowHistoryFavicons();
  EXPECT_TRUE(result);
}

TEST_F(HistoryHelperTest, ToggleShowVisitCount_ReturnsNewState) {
  bool result = helper_->ToggleShowVisitCount();
  EXPECT_TRUE(result);
  result = helper_->ToggleShowVisitCount();
  EXPECT_FALSE(result);
}

TEST_F(HistoryHelperTest, ToggleShowVisitTime_ReturnsNewState) {
  bool result = helper_->ToggleShowVisitTime();
  EXPECT_FALSE(result);
  result = helper_->ToggleShowVisitTime();
  EXPECT_TRUE(result);
}

TEST_F(HistoryHelperTest, ToggleGroupByDate_ReturnsNewState) {
  bool result = helper_->ToggleGroupHistoryByDate();
  EXPECT_FALSE(result);
  result = helper_->ToggleGroupHistoryByDate();
  EXPECT_TRUE(result);
}

TEST_F(HistoryHelperTest, ToggleShowTypedUrlsOnly_ReturnsNewState) {
  bool result = helper_->ToggleShowTypedUrlsOnly();
  EXPECT_TRUE(result);
  result = helper_->ToggleShowTypedUrlsOnly();
  EXPECT_FALSE(result);
}

TEST_F(HistoryHelperTest, ToggleDeletionEnabled_ReturnsNewState) {
  bool result = helper_->ToggleHistoryDeletionEnabled();
  EXPECT_FALSE(result);
  result = helper_->ToggleHistoryDeletionEnabled();
  EXPECT_TRUE(result);
}

TEST_F(HistoryHelperTest, ToggleAutoDelete_ReturnsNewState) {
  bool result = helper_->ToggleAutoDeleteHistory();
  EXPECT_TRUE(result);
  result = helper_->ToggleAutoDeleteHistory();
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Additional observer tests
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, Observer_SettingsChangedFiresOncePerChange) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  // Change multiple settings — each should fire once.
  helper_->SetShowHistoryInSidebar(false);
  helper_->SetShowHistoryFavicons(false);
  helper_->SetShowVisitTime(false);

  EXPECT_EQ(observer.settings_changed_count_, 3);

  helper_->RemoveObserver(&observer);
}

TEST_F(HistoryHelperTest, Observer_NoSettingsChangeWhenValueSame) {
  TestHistoryObserver observer;
  helper_->AddObserver(&observer);

  // Set to the same value — should not fire.
  helper_->SetShowHistoryInSidebar(true);  // Already true.
  helper_->SetHistorySortOrder("time_desc");  // Already "time_desc".
  helper_->SetMaxHistoryResults(50);  // Already 50.

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraHistoryQuery enum values
// ---------------------------------------------------------------------------

TEST_F(HistoryHelperTest, QuerySortBy_EnumValues) {
  AstraHistoryQuery query;
  query.sort_by = AstraHistoryQuery::kByTime;
  EXPECT_EQ(query.sort_by, AstraHistoryQuery::SortBy::kByTime);

  query.sort_by = AstraHistoryQuery::kByVisitCount;
  EXPECT_EQ(query.sort_by, AstraHistoryQuery::SortBy::kByVisitCount);

  query.sort_by = AstraHistoryQuery::kByTitle;
  EXPECT_EQ(query.sort_by, AstraHistoryQuery::SortBy::kByTitle);
}

}  // namespace astra
