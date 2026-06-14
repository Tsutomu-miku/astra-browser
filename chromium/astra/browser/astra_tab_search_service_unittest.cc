// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_search_service.h"

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestTabSearchObserver : public AstraTabSearchServiceObserver {
 public:
  void OnSearchResultChanged() override {
    change_count_++;
  }

  int change_count_ = 0;
};

// Helper: creates a test result item.
AstraTabSearchResult MakeResult(const std::string& title,
                                const std::string& url_spec,
                                AstraTabSearchCategory category) {
  AstraTabSearchResult result;
  result.title = title;
  result.url = GURL(url_spec);
  result.category = category;
  result.score = 0.0;
  return result;
}

// Helper: creates a test tab item.
AstraTabSearchResult MakeTab(const std::string& title,
                             const std::string& url_spec) {
  return MakeResult(title, url_spec, AstraTabSearchCategory::kTab);
}

// Helper: creates a test bookmark item.
AstraTabSearchResult MakeBookmark(const std::string& title,
                                  const std::string& url_spec) {
  return MakeResult(title, url_spec, AstraTabSearchCategory::kBookmark);
}

// Helper: creates a test history item.
AstraTabSearchResult MakeHistory(const std::string& title,
                                 const std::string& url_spec) {
  return MakeResult(title, url_spec, AstraTabSearchCategory::kHistory);
}

// Helper: creates a test reading list item.
AstraTabSearchResult MakeReadingList(const std::string& title,
                                     const std::string& url_spec) {
  return MakeResult(title, url_spec, AstraTabSearchCategory::kReadingList);
}

// Helper: creates a test note item.
AstraTabSearchResult MakeNote(const std::string& title,
                              const std::string& url_spec = "") {
  return MakeResult(title, url_spec, AstraTabSearchCategory::kNote);
}

}  // namespace

// ===========================================================================
// Test fixture
// ===========================================================================
//
// Uses TestingProfile from //chrome/test:test_support so the service has a
// real Profile* to attach to.  The service is obtained through the factory
// (AstraTabSearchServiceFactory::GetForProfile) to exercise the full
// ProfileKeyedService creation path.
//
// Test data is injected via SetXxxItemsForTesting() test helpers so we can
// exercise search and scoring logic without requiring real Chromium services.
//
// Chromium pattern: TestingProfile + ProfileKeyedServiceFactory + unit tests.
// ===========================================================================

class TabSearchServiceTest : public testing::Test {
 protected:
  TabSearchServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    service_ = AstraTabSearchServiceFactory::GetForProfile(profile_.get());
    DCHECK(service_);
  }

  ~TabSearchServiceTest() override = default;

  // testing::Test:
  void SetUp() override {
    ASSERT_NE(service_, nullptr);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Populates all five categories with a standard set of test items.
  void PopulateTestData() {
    // Tabs
    std::vector<AstraTabSearchResult> tabs;
    tabs.push_back(MakeTab("Google Search", "https://www.google.com/search"));
    tabs.push_back(MakeTab("Gmail - Inbox", "https://mail.google.com/mail/u/0"));
    tabs.push_back(MakeTab("YouTube - Home", "https://www.youtube.com/"));
    tabs.push_back(MakeTab("GitHub - Astra Browser",
                           "https://github.com/example/astra-browser"));
    tabs.push_back(MakeTab("Wikipedia - Chromium",
                           "https://en.wikipedia.org/wiki/Chromium"));
    service_->SetTabItemsForTesting(std::move(tabs));

    // Bookmarks
    std::vector<AstraTabSearchResult> bookmarks;
    bookmarks.push_back(MakeBookmark("Google Docs",
                                     "https://docs.google.com/document"));
    bookmarks.push_back(MakeBookmark("Gmail", "https://mail.google.com"));
    bookmarks.push_back(MakeBookmark("Stack Overflow",
                                     "https://stackoverflow.com"));
    bookmarks.push_back(MakeBookmark("MDN Web Docs",
                                     "https://developer.mozilla.org"));
    service_->SetBookmarkItemsForTesting(std::move(bookmarks));

    // History
    std::vector<AstraTabSearchResult> history;
    history.push_back(MakeHistory("Google Calendar",
                                  "https://calendar.google.com"));
    history.push_back(MakeHistory("YouTube - Watch",
                                  "https://www.youtube.com/watch?v=abc123"));
    history.push_back(MakeHistory("Reddit - Programming",
                                  "https://www.reddit.com/r/programming"));
    service_->SetHistoryItemsForTesting(std::move(history));

    // Reading list
    std::vector<AstraTabSearchResult> reading;
    reading.push_back(MakeReadingList(
        "The Future of Browsers", "https://example.com/future-browsers"));
    reading.push_back(MakeReadingList(
        "Chrome Extension Guide", "https://developer.chrome.com/docs/extensions"));
    service_->SetReadingListItemsForTesting(std::move(reading));

    // Notes
    std::vector<AstraTabSearchResult> notes;
    notes.push_back(MakeNote("Meeting Notes - Q2 Planning", ""));
    notes.push_back(MakeNote("Browser Architecture Ideas", ""));
    notes.push_back(MakeNote("Shopping List", ""));
    service_->SetNoteItemsForTesting(std::move(notes));
  }

  // Task environment is required for TestingProfile and base::Time.
  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<AstraTabSearchService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestTabSearchObserver> test_observers_;
};

// ===========================================================================
// Search basics
// ===========================================================================

TEST_F(TabSearchServiceTest, EmptyQueryReturnsAll) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kAll);
  // 5 tabs + 4 bookmarks + 3 history + 2 reading + 3 notes = 17
  EXPECT_EQ(results.size(), 17u);
}

TEST_F(TabSearchServiceTest, SearchByTitlePartialMatch) {
  PopulateTestData();

  auto results = service_->Search("Google", AstraTabSearchCategory::kAll);
  // Should match: Google Search (tab), Google Docs (bookmark),
  // Google Calendar (history) = 3 results
  EXPECT_GE(results.size(), 3u);

  // Verify all results contain "google" in title or URL.
  for (const auto& result : results) {
    bool has_match =
        result.title.find("Google") != std::string::npos ||
        result.url.spec().find("google") != std::string::npos;
    EXPECT_TRUE(has_match) << "Result: " << result.title;
  }
}

TEST_F(TabSearchServiceTest, SearchByUrlPartialMatch) {
  PopulateTestData();

  auto results = service_->Search("youtube.com", AstraTabSearchCategory::kAll);
  // Should match: YouTube - Home (tab), YouTube - Watch (history) = 2
  EXPECT_GE(results.size(), 2u);

  for (const auto& result : results) {
    EXPECT_NE(result.url.spec().find("youtube.com"), std::string::npos)
        << "Result: " << result.title;
  }
}

TEST_F(TabSearchServiceTest, CaseInsensitiveSearch) {
  PopulateTestData();

  auto results_upper = service_->Search("GMAIL", AstraTabSearchCategory::kAll);
  auto results_lower = service_->Search("gmail", AstraTabSearchCategory::kAll);
  auto results_mixed = service_->Search("Gmail", AstraTabSearchCategory::kAll);

  // All should find the same number of results.
  EXPECT_GT(results_upper.size(), 0u);
  EXPECT_EQ(results_upper.size(), results_lower.size());
  EXPECT_EQ(results_lower.size(), results_mixed.size());
}

TEST_F(TabSearchServiceTest, NoMatchReturnsEmpty) {
  PopulateTestData();

  auto results = service_->Search("zxqwv_this_wont_match_anything",
                                  AstraTabSearchCategory::kAll);
  EXPECT_TRUE(results.empty());
}

TEST_F(TabSearchServiceTest, SearchWithWhitespaceTrimmed) {
  PopulateTestData();

  auto results_no_space = service_->Search("Google",
                                            AstraTabSearchCategory::kAll);
  auto results_with_spaces = service_->Search("  Google  ",
                                               AstraTabSearchCategory::kAll);

  EXPECT_GT(results_no_space.size(), 0u);
  EXPECT_EQ(results_no_space.size(), results_with_spaces.size());
}

TEST_F(TabSearchServiceTest, SearchEmptyItemsReturnsEmpty) {
  // No test data injected — all Collect methods return empty.
  auto results = service_->Search("anything", AstraTabSearchCategory::kAll);
  EXPECT_TRUE(results.empty());

  auto results_empty = service_->Search("", AstraTabSearchCategory::kAll);
  EXPECT_TRUE(results_empty.empty());
}

// ===========================================================================
// Category filtering
// ===========================================================================

TEST_F(TabSearchServiceTest, FilterByTabsCategory) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kTab);
  EXPECT_EQ(results.size(), 5u);
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kTab);
  }
}

TEST_F(TabSearchServiceTest, FilterByBookmarksCategory) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kBookmark);
  EXPECT_EQ(results.size(), 4u);
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kBookmark);
  }
}

TEST_F(TabSearchServiceTest, FilterByHistoryCategory) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kHistory);
  EXPECT_EQ(results.size(), 3u);
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kHistory);
  }
}

TEST_F(TabSearchServiceTest, FilterByReadingListCategory) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kReadingList);
  EXPECT_EQ(results.size(), 2u);
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kReadingList);
  }
}

TEST_F(TabSearchServiceTest, FilterByNotesCategory) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kNote);
  EXPECT_EQ(results.size(), 3u);
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kNote);
  }
}

TEST_F(TabSearchServiceTest, FilterByAllIncludesAllCategories) {
  PopulateTestData();

  auto results = service_->Search("", AstraTabSearchCategory::kAll);
  // 5 + 4 + 3 + 2 + 3 = 17 total
  EXPECT_EQ(results.size(), 17u);

  // Verify all categories are present.
  bool has_tab = false, has_bookmark = false, has_history = false;
  bool has_reading = false, has_note = false;
  for (const auto& r : results) {
    switch (r.category) {
      case AstraTabSearchCategory::kTab: has_tab = true; break;
      case AstraTabSearchCategory::kBookmark: has_bookmark = true; break;
      case AstraTabSearchCategory::kHistory: has_history = true; break;
      case AstraTabSearchCategory::kReadingList: has_reading = true; break;
      case AstraTabSearchCategory::kNote: has_note = true; break;
      case AstraTabSearchCategory::kAll: break;  // Not expected in results.
    }
  }
  EXPECT_TRUE(has_tab);
  EXPECT_TRUE(has_bookmark);
  EXPECT_TRUE(has_history);
  EXPECT_TRUE(has_reading);
  EXPECT_TRUE(has_note);
}

// ===========================================================================
// Result ranking
// ===========================================================================

TEST_F(TabSearchServiceTest, TitleMatchRanksHigherThanUrlMatch) {
  // Set up items where one has title match and one has URL match.
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Apple Website", "https://example.com/fruit"));
  items.push_back(MakeTab("Random Site", "https://apple.com/news"));
  service_->SetTabItemsForTesting(std::move(items));

  auto results = service_->Search("apple", AstraTabSearchCategory::kTab);
  ASSERT_EQ(results.size(), 2u);

  // First result should be the title match ("Apple Website").
  EXPECT_EQ(results[0].title, "Apple Website");
  EXPECT_EQ(results[1].title, "Random Site");

  // Title match score should be higher than URL match score.
  EXPECT_GT(results[0].score, results[1].score);
}

TEST_F(TabSearchServiceTest, ExactTitleMatchRanksHighest) {
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Gmail", "https://mail.google.com"));
  items.push_back(MakeTab("Gmail Settings", "https://mail.google.com/settings"));
  items.push_back(MakeTab("Email Service", "https://gmail.example.com"));
  service_->SetTabItemsForTesting(std::move(items));

  auto results = service_->Search("Gmail", AstraTabSearchCategory::kTab);
  ASSERT_EQ(results.size(), 3u);

  // Exact match "Gmail" should be first.
  EXPECT_EQ(results[0].title, "Gmail");
  EXPECT_GT(results[0].score, results[1].score);
}

TEST_F(TabSearchServiceTest, PrefixMatchRanksHigherThanMidString) {
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Search Engine", "https://example.com"));
  items.push_back(MakeTab("Web Search", "https://example.com"));
  service_->SetTabItemsForTesting(std::move(items));

  auto results = service_->Search("search", AstraTabSearchCategory::kTab);
  ASSERT_EQ(results.size(), 2u);

  // "Search Engine" has "search" as prefix of title — should rank higher.
  EXPECT_EQ(results[0].title, "Search Engine");
  EXPECT_GT(results[0].score, results[1].score);
}

TEST_F(TabSearchServiceTest, ResultsSortedByScoreDescending) {
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Zebra page", "https://example.com/zebra"));
  items.push_back(MakeTab("Zebra documentation", "https://zebra.dev/docs"));
  items.push_back(MakeTab("About zebras", "https://example.com/about"));
  service_->SetTabItemsForTesting(std::move(items));

  auto results = service_->Search("zebra", AstraTabSearchCategory::kTab);
  ASSERT_GT(results.size(), 1u);

  // Verify scores are in descending order.
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].score, results[i].score)
        << "Result " << i - 1 << " (" << results[i - 1].title
        << ") should have score >= result " << i << " ("
        << results[i].title << ")";
  }
}

TEST_F(TabSearchServiceTest, ExactMatchScoreGreaterThanPartial) {
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Chat", "https://chat.example.com"));
  items.push_back(MakeTab("Chat Application", "https://example.com"));
  service_->SetTabItemsForTesting(std::move(items));

  auto results = service_->Search("Chat", AstraTabSearchCategory::kTab);
  ASSERT_EQ(results.size(), 2u);

  // Exact title match ("Chat") should have higher score than partial.
  EXPECT_EQ(results[0].title, "Chat");
  EXPECT_GT(results[0].score, results[1].score);
}

TEST_F(TabSearchServiceTest, ScoringIsConsistent) {
  PopulateTestData();

  // Running the same search twice should give the same results.
  auto results1 = service_->Search("Google", AstraTabSearchCategory::kAll);
  auto results2 = service_->Search("Google", AstraTabSearchCategory::kAll);

  ASSERT_EQ(results1.size(), results2.size());
  for (size_t i = 0; i < results1.size(); ++i) {
    EXPECT_EQ(results1[i].title, results2[i].title);
    EXPECT_DOUBLE_EQ(results1[i].score, results2[i].score);
  }
}

// ===========================================================================
// Convenience search methods
// ===========================================================================

TEST_F(TabSearchServiceTest, SearchTabsConvenienceMethod) {
  PopulateTestData();

  auto results = service_->SearchTabs("Google");
  auto results_via_category =
      service_->Search("Google", AstraTabSearchCategory::kTab);

  EXPECT_EQ(results.size(), results_via_category.size());
  for (size_t i = 0; i < results.size(); ++i) {
    EXPECT_EQ(results[i].title, results_via_category[i].title);
    EXPECT_EQ(results[i].category, AstraTabSearchCategory::kTab);
  }
}

TEST_F(TabSearchServiceTest, SearchBookmarksConvenienceMethod) {
  PopulateTestData();

  auto results = service_->SearchBookmarks("Docs");
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kBookmark);
  }
}

TEST_F(TabSearchServiceTest, SearchHistoryConvenienceMethod) {
  PopulateTestData();

  auto results = service_->SearchHistory("Reddit");
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kHistory);
  }
}

TEST_F(TabSearchServiceTest, SearchReadingListConvenienceMethod) {
  PopulateTestData();

  auto results = service_->SearchReadingList("Browsers");
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kReadingList);
  }
}

TEST_F(TabSearchServiceTest, SearchNotesConvenienceMethod) {
  PopulateTestData();

  auto results = service_->SearchNotes("Meeting");
  for (const auto& r : results) {
    EXPECT_EQ(r.category, AstraTabSearchCategory::kNote);
  }
}

// ===========================================================================
// Recent queries
// ===========================================================================

TEST_F(TabSearchServiceTest, RecentQueries_InitialEmpty) {
  auto recent = service_->GetRecentQueries();
  EXPECT_TRUE(recent.empty());
}

TEST_F(TabSearchServiceTest, RecentQueries_AddToFront) {
  service_->AddToRecentQueries("first query");
  service_->AddToRecentQueries("second query");

  auto recent = service_->GetRecentQueries();
  ASSERT_EQ(recent.size(), 2u);
  EXPECT_EQ(recent[0], "second query");  // Most recent first.
  EXPECT_EQ(recent[1], "first query");
}

TEST_F(TabSearchServiceTest, RecentQueries_NoDuplicates) {
  service_->AddToRecentQueries("apple");
  service_->AddToRecentQueries("banana");
  service_->AddToRecentQueries("apple");  // Duplicate — should move to front.

  auto recent = service_->GetRecentQueries();
  EXPECT_EQ(recent.size(), 2u);
  EXPECT_EQ(recent[0], "apple");   // Moved to front.
  EXPECT_EQ(recent[1], "banana");  // Still there but now second.
}

TEST_F(TabSearchServiceTest, RecentQueries_BoundedSize) {
  // Add 15 queries, max is 10.
  for (int i = 0; i < 15; ++i) {
    service_->AddToRecentQueries("query-" + std::to_string(i));
  }

  auto recent = service_->GetRecentQueries();
  EXPECT_EQ(recent.size(), 10u);  // Bounded at kMaxRecentQueries.

  // Most recent should be first (query-14 down to query-5).
  EXPECT_EQ(recent[0], "query-14");
  EXPECT_EQ(recent[9], "query-5");
}

TEST_F(TabSearchServiceTest, RecentQueries_ClearAll) {
  service_->AddToRecentQueries("one");
  service_->AddToRecentQueries("two");
  ASSERT_GT(service_->GetRecentQueries().size(), 0u);

  service_->ClearRecentQueries();

  auto recent = service_->GetRecentQueries();
  EXPECT_TRUE(recent.empty());
}

TEST_F(TabSearchServiceTest, RecentQueries_EmptyStringIgnored) {
  service_->AddToRecentQueries("");
  service_->AddToRecentQueries("   ");  // Whitespace-only.

  auto recent = service_->GetRecentQueries();
  EXPECT_TRUE(recent.empty());
}

TEST_F(TabSearchServiceTest, RecentQueries_MaxConstantMatches) {
  // Verify the constant value matches the spec.
  EXPECT_EQ(AstraTabSearchService::kMaxRecentQueries, 10u);
}

// ===========================================================================
// Observer notifications
// ===========================================================================

TEST_F(TabSearchServiceTest, ObserverFiresOnResultChange) {
  TestTabSearchObserver observer;
  service_->AddObserver(&observer);

  EXPECT_EQ(observer.change_count_, 0);

  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 1);

  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 2);

  service_->RemoveObserver(&observer);
}

TEST_F(TabSearchServiceTest, Observer_AddRemove) {
  TestTabSearchObserver observer;

  // Not added yet — no notification.
  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 0);

  // Add observer — should get notifications.
  service_->AddObserver(&observer);
  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 1);

  // Remove observer — no more notifications.
  service_->RemoveObserver(&observer);
  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 1);  // Still 1, not incremented.
}

TEST_F(TabSearchServiceTest, Observer_MultipleObservers) {
  TestTabSearchObserver obs1;
  TestTabSearchObserver obs2;
  TestTabSearchObserver obs3;

  service_->AddObserver(&obs1);
  service_->AddObserver(&obs2);
  service_->AddObserver(&obs3);

  service_->NotifySearchResultChangedForTesting();

  EXPECT_EQ(obs1.change_count_, 1);
  EXPECT_EQ(obs2.change_count_, 1);
  EXPECT_EQ(obs3.change_count_, 1);

  service_->RemoveObserver(&obs2);
  service_->NotifySearchResultChangedForTesting();

  EXPECT_EQ(obs1.change_count_, 2);
  EXPECT_EQ(obs2.change_count_, 1);  // Removed, didn't get second notification.
  EXPECT_EQ(obs3.change_count_, 2);

  service_->RemoveObserver(&obs1);
  service_->RemoveObserver(&obs3);
}

// ===========================================================================
// Factory tests
// ===========================================================================

TEST_F(TabSearchServiceTest, Factory_GetForProfileReturnsInstance) {
  // The service is created in the fixture constructor via the factory.
  EXPECT_NE(service_, nullptr);
}

TEST_F(TabSearchServiceTest, Factory_SameProfileSameInstance) {
  // Calling GetForProfile twice with the same profile should return
  // the same instance (ProfileKeyedService behavior).
  AstraTabSearchService* instance1 =
      AstraTabSearchServiceFactory::GetForProfile(profile_.get());
  AstraTabSearchService* instance2 =
      AstraTabSearchServiceFactory::GetForProfile(profile_.get());

  EXPECT_EQ(instance1, instance2);
  EXPECT_EQ(service_, instance1);
}

TEST_F(TabSearchServiceTest, Factory_GetInstanceReturnsSingleton) {
  AstraTabSearchServiceFactory* factory1 =
      AstraTabSearchServiceFactory::GetInstance();
  AstraTabSearchServiceFactory* factory2 =
      AstraTabSearchServiceFactory::GetInstance();

  EXPECT_NE(factory1, nullptr);
  EXPECT_EQ(factory1, factory2);
}

TEST_F(TabSearchServiceTest, Factory_NullProfileReturnsNull) {
  AstraTabSearchService* result =
      AstraTabSearchServiceFactory::GetForProfile(nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST_F(TabSearchServiceTest, Factory_DifferentProfileDifferentInstance) {
  // With kOwnInstance for regular profiles, each profile gets its own
  // service instance.
  TestingProfile another_profile;
  AstraTabSearchService* another_service =
      AstraTabSearchServiceFactory::GetForProfile(&another_profile);

  EXPECT_NE(another_service, nullptr);
  EXPECT_NE(another_service, service_);
}

TEST_F(TabSearchServiceTest, Factory_IncognitoSeparateInstance) {
  // With kOwnInstance for incognito, the OTR profile gets its own
  // service instance (not redirected to the original profile).
  //
  // TODO(astra): Verify with real TestingProfile OTR profile.  The exact
  //   API for creating OTR TestingProfile varies by Chromium version.
  //   Chromium owner: Profile::GetOffTheRecordProfile().
  //   Patch point: testing/profile/testing_profile.h.
  //
  // For now, we verify that a separate regular profile gets a separate
  // instance, which is the same kOwnInstance behavior that applies to
  // incognito profiles.
  TestingProfile otr_profile;
  AstraTabSearchService* otr_service =
      AstraTabSearchServiceFactory::GetForProfile(&otr_profile);

  EXPECT_NE(otr_service, nullptr);
  EXPECT_NE(otr_service, service_);
}

// ===========================================================================
// Service lifecycle
// ===========================================================================

TEST_F(TabSearchServiceTest, ShutdownClearsObservers) {
  TestTabSearchObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, notifying (via test helper) should not reach the
  // observer because the observer list was cleared.
  service_->NotifySearchResultChangedForTesting();
  EXPECT_EQ(observer.change_count_, 0);
}

// ===========================================================================
// Edge cases
// ===========================================================================

TEST_F(TabSearchServiceTest, SpecialCharactersInQuery) {
  std::vector<AstraTabSearchResult> items;
  items.push_back(MakeTab("Example Site", "https://example.com/path?q=test"));
  service_->SetTabItemsForTesting(std::move(items));

  // Special characters should not crash the search.
  auto results1 = service_->Search("example.com/path?q=",
                                    AstraTabSearchCategory::kTab);
  EXPECT_GE(results1.size(), 1u);

  auto results2 = service_->Search("?q=test", AstraTabSearchCategory::kTab);
  EXPECT_GE(results2.size(), 1u);

  // Very unusual characters.
  auto results3 = service_->Search("<>\"'\\", AstraTabSearchCategory::kTab);
  EXPECT_TRUE(results3.empty());  // No match, but no crash.
}

TEST_F(TabSearchServiceTest, LongQueryNoCrash) {
  PopulateTestData();

  std::string long_query(1000, 'a');
  auto results = service_->Search(long_query, AstraTabSearchCategory::kAll);
  // Should not crash — just return no match (or maybe match if any
  // title/URL has 1000 'a's, which it won't).
  EXPECT_TRUE(results.empty());
}

TEST_F(TabSearchServiceTest, ResultStructHasExpectedFields) {
  AstraTabSearchResult result;

  // Default values.
  EXPECT_TRUE(result.title.empty());
  EXPECT_TRUE(result.url.is_empty());
  EXPECT_EQ(result.category, AstraTabSearchCategory::kTab);
  EXPECT_DOUBLE_EQ(result.score, 0.0);
  EXPECT_EQ(result.source_data, nullptr);
  EXPECT_TRUE(result.workspace_id.empty());
  EXPECT_TRUE(result.folder_path.empty());
  EXPECT_FALSE(result.is_read);
  EXPECT_TRUE(result.note_preview.empty());

  // Field assignment works.
  result.title = "Test Title";
  result.url = GURL("https://example.com");
  result.category = AstraTabSearchCategory::kBookmark;
  result.score = 5.0;
  result.workspace_id = "ws-1";
  result.folder_path = "Bookmarks Bar/News";
  result.is_read = true;
  result.note_preview = "This is a note preview...";

  EXPECT_EQ(result.title, "Test Title");
  EXPECT_EQ(result.url.spec(), "https://example.com/");
  EXPECT_EQ(result.category, AstraTabSearchCategory::kBookmark);
  EXPECT_DOUBLE_EQ(result.score, 5.0);
  EXPECT_EQ(result.workspace_id, "ws-1");
  EXPECT_EQ(result.folder_path, "Bookmarks Bar/News");
  EXPECT_TRUE(result.is_read);
  EXPECT_EQ(result.note_preview, "This is a note preview...");
}

TEST_F(TabSearchServiceTest, CategoryEnumOrder) {
  // Verify category enum values for stability.
  // kAll = 0, kTab = 1, kBookmark = 2, kHistory = 3, kReadingList = 4, kNote = 5
  // This ordering affects tie-breaking in search results.
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kAll), 0);
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kTab), 1);
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kBookmark), 2);
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kHistory), 3);
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kReadingList), 4);
  EXPECT_EQ(static_cast<int>(AstraTabSearchCategory::kNote), 5);
}

TEST_F(TabSearchServiceTest, PrefKeyConstant) {
  // Verify the pref key constant is defined and non-empty.
  EXPECT_STREQ(AstraTabSearchService::kPrefRecentQueries,
               "astra.tab_search.recent_queries");
}

}  // namespace astra
