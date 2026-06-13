// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_search_engine_helper.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestSearchEngineObserver : public AstraSearchEngineHelper::Observer {
 public:
  void OnDefaultSearchEngineChanged() override {
    default_engine_changed_count_++;
  }

  void OnSearchEngineAdded(const AstraSearchEngineInfo& engine) override {
    engine_added_count_++;
    last_added_engine_ = engine;
  }

  void OnSearchEngineRemoved(const std::string& engine_id) override {
    engine_removed_count_++;
    last_removed_engine_id_ = engine_id;
  }

  void OnSearchEnginesChanged() override {
    engines_changed_count_++;
  }

  void OnSearchPresentationChanged() override {
    presentation_changed_count_++;
  }

  void OnRecentQueriesChanged() override {
    recent_queries_changed_count_++;
  }

  // Counters
  int default_engine_changed_count_ = 0;
  int engine_added_count_ = 0;
  int engine_removed_count_ = 0;
  int engines_changed_count_ = 0;
  int presentation_changed_count_ = 0;
  int recent_queries_changed_count_ = 0;

  // Last recorded values
  AstraSearchEngineInfo last_added_engine_;
  std::string last_removed_engine_id_;
};

}  // namespace

// Test fixture for AstraSearchEngineHelper tests.
class SearchEngineHelperTest : public testing::Test {
 protected:
  SearchEngineHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
  }

  ~SearchEngineHelperTest() override = default;

  void SetUp() override {
    // Verify default state.
    ASSERT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
    ASSERT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
    ASSERT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      AstraSearchEngineHelper::RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::vector<TestSearchEngineObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, DefaultState_ShowDefaultEngine) {
  EXPECT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
}

TEST_F(SearchEngineHelperTest, DefaultState_ShowOtherEngines) {
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
}

TEST_F(SearchEngineHelperTest, DefaultState_SuggestionsEnabled) {
  EXPECT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
}

TEST_F(SearchEngineHelperTest, DefaultState_RecentQueriesEmpty) {
  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_TRUE(queries.empty());
}

TEST_F(SearchEngineHelperTest, DefaultState_MaxRecentQueries) {
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()),
            prefs::kDefaultSearchMaxRecentQueries);
}

TEST_F(SearchEngineHelperTest, DefaultState_SearchEngineCountZero) {
  // In the overlay, TemplateURLService is not available, so count is 0.
  EXPECT_EQ(AstraSearchEngineHelper::GetSearchEngineCount(profile_.get()), 0u);
}

TEST_F(SearchEngineHelperTest, DefaultState_GetSearchEnginesEmpty) {
  // In the overlay, TemplateURLService is not available.
  auto engines = AstraSearchEngineHelper::GetSearchEngines(profile_.get());
  EXPECT_TRUE(engines.empty());
}

TEST_F(SearchEngineHelperTest, DefaultState_GetSearchEnginesListEmpty) {
  // GetSearchEnginesList should be an alias for GetSearchEngines.
  auto engines = AstraSearchEngineHelper::GetSearchEnginesList(profile_.get());
  EXPECT_TRUE(engines.empty());
}

TEST_F(SearchEngineHelperTest, DefaultState_DefaultEngineNull) {
  // In the overlay, no default engine is available.
  EXPECT_EQ(AstraSearchEngineHelper::GetDefaultSearchEngine(profile_.get()),
            nullptr);
}

TEST_F(SearchEngineHelperTest, DefaultState_DefaultEngineInfoEmpty) {
  auto info = AstraSearchEngineHelper::GetDefaultSearchEngineInfo(
      profile_.get());
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.is_default);
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraSearchEngineHelper::Observer {};

  DefaultObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  // Trigger all observer paths via pref changes and manual notifications.
  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), false);
  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);
  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), false);
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "test query");

  AstraSearchEngineHelper::NotifySearchEnginesChanged();
  AstraSearchEngineHelper::NotifyDefaultSearchEngineChanged();
  AstraSearchEngineHelper::NotifySearchEngineAdded(AstraSearchEngineInfo());
  AstraSearchEngineHelper::NotifySearchEngineRemoved("test-id");

  AstraSearchEngineHelper::RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, AddRemoveObserver_NoCrash) {
  TestSearchEngineObserver observer;

  AstraSearchEngineHelper::AddObserver(&observer);
  AstraSearchEngineHelper::RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(SearchEngineHelperTest, RemoveNonexistentObserver_NoCrash) {
  TestSearchEngineObserver observer;

  AstraSearchEngineHelper::RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// Presentation settings — show default engine
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, SetShowDefaultEngine_ChangesValue) {
  ASSERT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));

  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), false);
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));

  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), true);
  EXPECT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
}

TEST_F(SearchEngineHelperTest, SetShowDefaultEngine_SameValueNoOp) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  ASSERT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), true);

  EXPECT_EQ(observer.presentation_changed_count_, 0);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, SetShowDefaultEngine_FiresPresentationObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), false);

  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, SetShowDefaultEngine_NullProfileNoCrash) {
  AstraSearchEngineHelper::SetShowDefaultEngine(nullptr, false);
  // Should not crash.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Presentation settings — show other engines
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, SetShowOtherEngines_ChangesValue) {
  ASSERT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);
  EXPECT_TRUE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), false);
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
}

TEST_F(SearchEngineHelperTest, SetShowOtherEngines_SameValueNoOp) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  ASSERT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), false);

  EXPECT_EQ(observer.presentation_changed_count_, 0);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, SetShowOtherEngines_FiresPresentationObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);

  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, ToggleShowOtherEngines_FlipsValue) {
  ASSERT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));

  bool result = AstraSearchEngineHelper::ToggleShowOtherEngines(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));

  result = AstraSearchEngineHelper::ToggleShowOtherEngines(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
}

// ---------------------------------------------------------------------------
// Presentation settings — suggestions enabled
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, SetSuggestionsEnabled_ChangesValue) {
  ASSERT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));

  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), false);
  EXPECT_FALSE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));

  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), true);
  EXPECT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
}

TEST_F(SearchEngineHelperTest, SetSuggestionsEnabled_SameValueNoOp) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  ASSERT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), true);

  EXPECT_EQ(observer.presentation_changed_count_, 0);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, SetSuggestionsEnabled_FiresPresentationObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), false);

  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, ToggleSuggestionsEnabled_FlipsValue) {
  ASSERT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));

  bool result =
      AstraSearchEngineHelper::ToggleSuggestionsEnabled(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));

  result = AstraSearchEngineHelper::ToggleSuggestionsEnabled(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
}

// ---------------------------------------------------------------------------
// Recent search queries
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, AddRecentQuery_AddsToFront) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "first query");

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  ASSERT_EQ(queries.size(), 1u);
  EXPECT_EQ(queries[0], "first query");
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_MovesExistingToFront) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "first query");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "second query");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "first query");

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  ASSERT_EQ(queries.size(), 2u);
  EXPECT_EQ(queries[0], "first query");
  EXPECT_EQ(queries[1], "second query");
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_MostRecentFirst) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "first");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "second");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "third");

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  ASSERT_EQ(queries.size(), 3u);
  EXPECT_EQ(queries[0], "third");
  EXPECT_EQ(queries[1], "second");
  EXPECT_EQ(queries[2], "first");
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_EmptyQueryIgnored) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "");

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_TRUE(queries.empty());
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_NullProfileNoCrash) {
  AstraSearchEngineHelper::AddRecentQuery(nullptr, "test");
  SUCCEED();
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "test query");

  EXPECT_EQ(observer.recent_queries_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, ClearRecentQueries_EmptiesList) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "query 1");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "query 2");
  ASSERT_EQ(AstraSearchEngineHelper::GetRecentQueries(profile_.get()).size(),
            2u);

  AstraSearchEngineHelper::ClearRecentQueries(profile_.get());

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_TRUE(queries.empty());
}

TEST_F(SearchEngineHelperTest, ClearRecentQueries_FiresObserver) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "query 1");

  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::ClearRecentQueries(profile_.get());
  EXPECT_EQ(observer.recent_queries_changed_count_, 1);

  // Clear again — should not fire since it's already empty.
  AstraSearchEngineHelper::ClearRecentQueries(profile_.get());
  EXPECT_EQ(observer.recent_queries_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, ClearRecentQueries_NullProfileNoCrash) {
  AstraSearchEngineHelper::ClearRecentQueries(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Recent queries — max count and truncation
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, MaxRecentQueries_DefaultValue) {
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()),
            prefs::kDefaultSearchMaxRecentQueries);
}

TEST_F(SearchEngineHelperTest, SetMaxRecentQueries_ChangesValue) {
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 5);
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()), 5);

  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 20);
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()), 20);
}

TEST_F(SearchEngineHelperTest, SetMaxRecentQueries_ClampsToLimit) {
  // Setting above the limit should be clamped.
  AstraSearchEngineHelper::SetMaxRecentQueries(
      profile_.get(), prefs::kMaxSearchRecentQueriesLimit + 50);
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()),
            prefs::kMaxSearchRecentQueriesLimit);
}

TEST_F(SearchEngineHelperTest, SetMaxRecentQueries_ClampsToZero) {
  // Setting below zero should be clamped to zero.
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), -5);
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()), 0);
}

TEST_F(SearchEngineHelperTest, SetMaxRecentQueries_TruncatesList) {
  // Add several queries.
  for (int i = 0; i < 10; ++i) {
    AstraSearchEngineHelper::AddRecentQuery(profile_.get(),
                                            "query " + std::to_string(i));
  }
  ASSERT_EQ(AstraSearchEngineHelper::GetRecentQueries(profile_.get()).size(),
            10u);

  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  // Reduce max below current count — should truncate.
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 3);

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_EQ(queries.size(), 3u);
  // Should keep the most recent ones.
  EXPECT_EQ(queries[0], "query 9");
  EXPECT_EQ(queries[1], "query 8");
  EXPECT_EQ(queries[2], "query 7");

  // Should have fired observer for truncation.
  EXPECT_EQ(observer.recent_queries_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, SetMaxRecentQueries_IncreaseNoTruncation) {
  // Add a few queries.
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "query 1");
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "query 2");
  ASSERT_EQ(AstraSearchEngineHelper::GetRecentQueries(profile_.get()).size(),
            2u);

  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  // Increase max — no truncation, no notification.
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 20);

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_EQ(queries.size(), 2u);
  EXPECT_EQ(observer.recent_queries_changed_count_, 0);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, AddRecentQuery_RespectsMax) {
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 3);

  // Add 5 queries — should be capped at 3.
  for (int i = 0; i < 5; ++i) {
    AstraSearchEngineHelper::AddRecentQuery(profile_.get(),
                                            "q" + std::to_string(i));
  }

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  EXPECT_EQ(queries.size(), 3u);
  // Should keep the 3 most recent.
  EXPECT_EQ(queries[0], "q4");
  EXPECT_EQ(queries[1], "q3");
  EXPECT_EQ(queries[2], "q2");
}

// ---------------------------------------------------------------------------
// Search engine query methods (stub behavior in overlay)
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, GetNonDefaultSearchEngines_EmptyInOverlay) {
  auto engines =
      AstraSearchEngineHelper::GetNonDefaultSearchEngines(profile_.get());
  EXPECT_TRUE(engines.empty());
}

TEST_F(SearchEngineHelperTest, IsDefaultSearchEngine_EmptyIdReturnsFalse) {
  EXPECT_FALSE(AstraSearchEngineHelper::IsDefaultSearchEngine(
      profile_.get(), std::string()));
}

TEST_F(SearchEngineHelperTest, IsDefaultSearchEngine_NullProfileNoCrash) {
  EXPECT_FALSE(
      AstraSearchEngineHelper::IsDefaultSearchEngine(nullptr, "test-id"));
}

TEST_F(SearchEngineHelperTest, GetSearchEngineByName_EmptyNameEmptyResult) {
  auto result = AstraSearchEngineHelper::GetSearchEngineByName(profile_.get(),
                                                               std::u16string());
  EXPECT_TRUE(result.id.empty());
}

TEST_F(SearchEngineHelperTest, GetSearchEngineByName_EmptyInOverlay) {
  auto result = AstraSearchEngineHelper::GetSearchEngineByName(
      profile_.get(), u"Google");
  EXPECT_TRUE(result.id.empty());
}

TEST_F(SearchEngineHelperTest, GetSearchEngineByKeyword_EmptyKeywordEmptyResult) {
  auto result = AstraSearchEngineHelper::GetSearchEngineByKeyword(
      profile_.get(), std::u16string());
  EXPECT_TRUE(result.id.empty());
}

TEST_F(SearchEngineHelperTest, GetSearchEngineByKeyword_EmptyInOverlay) {
  auto result = AstraSearchEngineHelper::GetSearchEngineByKeyword(
      profile_.get(), u"google.com");
  EXPECT_TRUE(result.id.empty());
}

// ---------------------------------------------------------------------------
// Search engine operations (stub behavior in overlay)
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, SetDefaultSearchEngine_NullReturnsFalse) {
  EXPECT_FALSE(AstraSearchEngineHelper::SetDefaultSearchEngine(
      profile_.get(), nullptr));
}

TEST_F(SearchEngineHelperTest, SetDefaultSearchEngineById_EmptyIdReturnsFalse) {
  EXPECT_FALSE(AstraSearchEngineHelper::SetDefaultSearchEngineById(
      profile_.get(), std::string()));
}

TEST_F(SearchEngineHelperTest, AddSearchEngine_EmptyNameReturnsNull) {
  EXPECT_EQ(AstraSearchEngineHelper::AddSearchEngine(
                profile_.get(), std::u16string(), u"keyword", "http://example.com"),
            nullptr);
}

TEST_F(SearchEngineHelperTest, AddSearchEngine_EmptyUrlReturnsNull) {
  EXPECT_EQ(AstraSearchEngineHelper::AddSearchEngine(
                profile_.get(), u"Name", u"keyword", std::string()),
            nullptr);
}

TEST_F(SearchEngineHelperTest, RemoveSearchEngine_NullReturnsFalse) {
  EXPECT_FALSE(AstraSearchEngineHelper::RemoveSearchEngine(
      profile_.get(), nullptr));
}

TEST_F(SearchEngineHelperTest, RemoveSearchEngineById_EmptyIdReturnsFalse) {
  EXPECT_FALSE(AstraSearchEngineHelper::RemoveSearchEngineById(
      profile_.get(), std::string()));
}

// ---------------------------------------------------------------------------
// Search shortcut helpers
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, BuildSearchUrl_EmptyKeywordEmptyResult) {
  EXPECT_TRUE(AstraSearchEngineHelper::BuildSearchUrl(
                  profile_.get(), std::u16string(), u"query")
                  .empty());
}

TEST_F(SearchEngineHelperTest, BuildSearchUrl_EmptyQueryEmptyResult) {
  EXPECT_TRUE(AstraSearchEngineHelper::BuildSearchUrl(
                  profile_.get(), u"keyword", std::u16string())
                  .empty());
}

TEST_F(SearchEngineHelperTest, BuildSearchUrl_NullProfileEmptyResult) {
  EXPECT_TRUE(AstraSearchEngineHelper::BuildSearchUrl(
                  nullptr, u"keyword", u"query")
                  .empty());
}

// ---------------------------------------------------------------------------
// Manual observer notifications
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, NotifySearchEnginesChanged_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::NotifySearchEnginesChanged();
  EXPECT_EQ(observer.engines_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, NotifyDefaultSearchEngineChanged_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::NotifyDefaultSearchEngineChanged();
  EXPECT_EQ(observer.default_engine_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, NotifySearchEngineAdded_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineInfo info;
  info.id = "test-id";
  info.name = u"Test Engine";
  AstraSearchEngineHelper::NotifySearchEngineAdded(info);

  EXPECT_EQ(observer.engine_added_count_, 1);
  EXPECT_EQ(observer.last_added_engine_.id, "test-id");
  EXPECT_EQ(observer.last_added_engine_.name, u"Test Engine");

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, NotifySearchEngineRemoved_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::NotifySearchEngineRemoved("engine-123");

  EXPECT_EQ(observer.engine_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_engine_id_, "engine-123");

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, NotifySearchPresentationChanged_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::NotifySearchPresentationChanged();
  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

TEST_F(SearchEngineHelperTest, NotifyRecentQueriesChanged_FiresObserver) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::NotifyRecentQueriesChanged();
  EXPECT_EQ(observer.recent_queries_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, MultipleObservers_AllNotified) {
  TestSearchEngineObserver observer1;
  TestSearchEngineObserver observer2;

  AstraSearchEngineHelper::AddObserver(&observer1);
  AstraSearchEngineHelper::AddObserver(&observer2);

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);

  EXPECT_EQ(observer1.presentation_changed_count_, 1);
  EXPECT_EQ(observer2.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer1);
  AstraSearchEngineHelper::RemoveObserver(&observer2);
}

TEST_F(SearchEngineHelperTest, RemoveObserver_StopsNotifications) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);
  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::RemoveObserver(&observer);

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), false);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.presentation_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Null profile edge cases
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, GetShowDefaultEngine_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraSearchEngineHelper::GetShowDefaultEngine(nullptr),
            prefs::kDefaultSearchShowDefaultEngine);
}

TEST_F(SearchEngineHelperTest, GetShowOtherEngines_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraSearchEngineHelper::GetShowOtherEngines(nullptr),
            prefs::kDefaultSearchShowOtherEngines);
}

TEST_F(SearchEngineHelperTest, GetSuggestionsEnabled_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraSearchEngineHelper::GetSuggestionsEnabled(nullptr),
            prefs::kDefaultSearchSuggestionsEnabled);
}

TEST_F(SearchEngineHelperTest, GetMaxRecentQueries_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(nullptr),
            prefs::kDefaultSearchMaxRecentQueries);
}

TEST_F(SearchEngineHelperTest, GetRecentQueries_NullProfileEmpty) {
  auto queries = AstraSearchEngineHelper::GetRecentQueries(nullptr);
  EXPECT_TRUE(queries.empty());
}

TEST_F(SearchEngineHelperTest, GetDefaultSearchEngine_NullProfileNull) {
  EXPECT_EQ(AstraSearchEngineHelper::GetDefaultSearchEngine(nullptr), nullptr);
}

TEST_F(SearchEngineHelperTest, GetSearchEngines_NullProfileEmpty) {
  auto engines = AstraSearchEngineHelper::GetSearchEngines(nullptr);
  EXPECT_TRUE(engines.empty());
}

TEST_F(SearchEngineHelperTest, GetSearchEngineCount_NullProfileZero) {
  EXPECT_EQ(AstraSearchEngineHelper::GetSearchEngineCount(nullptr), 0u);
}

TEST_F(SearchEngineHelperTest, OpenChromeSearchSettings_NullProfileNoCrash) {
  AstraSearchEngineHelper::OpenChromeSearchSettings(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, PrefsPersist_ShowDefaultEngine) {
  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), false);

  // Read from the same profile's prefs — value should persist.
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
}

TEST_F(SearchEngineHelperTest, PrefsPersist_ShowOtherEngines) {
  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);

  EXPECT_TRUE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
}

TEST_F(SearchEngineHelperTest, PrefsPersist_SuggestionsEnabled) {
  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), false);

  EXPECT_FALSE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
}

TEST_F(SearchEngineHelperTest, PrefsPersist_RecentQueries) {
  AstraSearchEngineHelper::AddRecentQuery(profile_.get(), "persisted query");

  auto queries = AstraSearchEngineHelper::GetRecentQueries(profile_.get());
  ASSERT_EQ(queries.size(), 1u);
  EXPECT_EQ(queries[0], "persisted query");
}

TEST_F(SearchEngineHelperTest, PrefsPersist_MaxRecentQueries) {
  AstraSearchEngineHelper::SetMaxRecentQueries(profile_.get(), 5);

  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()), 5);
}

TEST_F(SearchEngineHelperTest, PrefsPersist_DefaultValues) {
  // All values should start at their defaults.
  EXPECT_TRUE(AstraSearchEngineHelper::GetShowDefaultEngine(profile_.get()));
  EXPECT_FALSE(AstraSearchEngineHelper::GetShowOtherEngines(profile_.get()));
  EXPECT_TRUE(AstraSearchEngineHelper::GetSuggestionsEnabled(profile_.get()));
  EXPECT_EQ(AstraSearchEngineHelper::GetMaxRecentQueries(profile_.get()),
            prefs::kDefaultSearchMaxRecentQueries);
  EXPECT_TRUE(
      AstraSearchEngineHelper::GetRecentQueries(profile_.get()).empty());
}

// ---------------------------------------------------------------------------
// Combined presentation settings
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, MultiplePresentationChanges_AllNotify) {
  TestSearchEngineObserver observer;
  AstraSearchEngineHelper::AddObserver(&observer);

  // Change each setting — each should fire a presentation notification.
  AstraSearchEngineHelper::SetShowDefaultEngine(profile_.get(), false);
  EXPECT_EQ(observer.presentation_changed_count_, 1);

  AstraSearchEngineHelper::SetShowOtherEngines(profile_.get(), true);
  EXPECT_EQ(observer.presentation_changed_count_, 2);

  AstraSearchEngineHelper::SetSuggestionsEnabled(profile_.get(), false);
  EXPECT_EQ(observer.presentation_changed_count_, 3);

  AstraSearchEngineHelper::RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraSearchEngineInfo struct
// ---------------------------------------------------------------------------

TEST_F(SearchEngineHelperTest, SearchEngineInfo_DefaultConstructed) {
  AstraSearchEngineInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.keyword.empty());
  EXPECT_TRUE(info.url.empty());
  EXPECT_FALSE(info.is_default);
  EXPECT_FALSE(info.is_builtin);
  EXPECT_FALSE(info.is_editable);
}

}  // namespace astra
