// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_recent_tabs_helper.h"

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestRecentTabsObserver : public AstraRecentTabsHelper::Observer {
 public:
  void OnRecentTabsChanged() override {
    recent_tabs_changed_count_++;
  }

  void OnTabClosedToRecent(const AstraRecentlyClosedTab& tab) override {
    tab_closed_count_++;
    last_closed_tab_id_ = tab.entry_id;
    last_closed_tab_title_ = tab.title;
  }

  void OnRecentTabRestored(int entry_id) override {
    tab_restored_count_++;
    last_restored_entry_id_ = entry_id;
  }

  void OnRecentTabsCleared() override {
    recent_tabs_cleared_count_++;
  }

  void OnRecentPresentationChanged() override {
    presentation_changed_count_++;
  }

  // Counters
  int recent_tabs_changed_count_ = 0;
  int tab_closed_count_ = 0;
  int tab_restored_count_ = 0;
  int recent_tabs_cleared_count_ = 0;
  int presentation_changed_count_ = 0;

  // Last recorded values
  int last_closed_tab_id_ = 0;
  std::u16string last_closed_tab_title_;
  int last_restored_entry_id_ = 0;
};

}  // namespace

// Test fixture for AstraRecentTabsHelper tests.
//
// Uses TestingProfile so the helper has a real Profile* with PrefService
// for presentation settings.
//
// TODO(astra): The underlying TabRestoreService is not available in the
// overlay test harness, so query methods return empty results.  Full
// integration tests require a browser test harness with
// InProcessBrowserTest.
// Chromium component: InProcessBrowserTest + TabRestoreService.
class RecentTabsHelperTest : public testing::Test {
 protected:
  RecentTabsHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register recent tabs prefs on the testing profile's pref service.
    // In production, this happens during profile initialization.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
  }

  ~RecentTabsHelperTest() override = default;

  void SetUp() override {
    // Verify prefs are at defaults.
    ASSERT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()),
              AstraRecentTabsHelper::kDefaultMaxRecentTabs);
    ASSERT_EQ(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()),
              AstraRecentTabsHelper::kDefaultShowInSidebar);
    ASSERT_EQ(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()),
              AstraRecentTabsHelper::kDefaultShowTimestamps);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      AstraRecentTabsHelper::RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;

  // Pool of test observers managed by the fixture.
  std::vector<std::unique_ptr<TestRecentTabsObserver>> test_observers_;

  // Helper to create and register a test observer.
  TestRecentTabsObserver* AddTestObserver() {
    auto observer = std::make_unique<TestRecentTabsObserver>();
    TestRecentTabsObserver* raw = observer.get();
    AstraRecentTabsHelper::AddObserver(raw);
    test_observers_.push_back(std::move(observer));
    return raw;
  }
};

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, DefaultMaxRecentTabs) {
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()),
            AstraRecentTabsHelper::kDefaultMaxRecentTabs);
  EXPECT_EQ(AstraRecentTabsHelper::kDefaultMaxRecentTabs, 10);
}

TEST_F(RecentTabsHelperTest, DefaultShowInSidebar) {
  EXPECT_EQ(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()),
            AstraRecentTabsHelper::kDefaultShowInSidebar);
  EXPECT_TRUE(AstraRecentTabsHelper::kDefaultShowInSidebar);
}

TEST_F(RecentTabsHelperTest, DefaultShowTimestamps) {
  EXPECT_EQ(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()),
            AstraRecentTabsHelper::kDefaultShowTimestamps);
  EXPECT_TRUE(AstraRecentTabsHelper::kDefaultShowTimestamps);
}

TEST_F(RecentTabsHelperTest, DefaultHasNoRecentTabs) {
  // In the overlay, TabRestoreService is not available, so there are
  // no recent tabs.  This tests the graceful degradation path.
  EXPECT_FALSE(AstraRecentTabsHelper::HasRecentlyClosedTabs(profile_.get()));
  EXPECT_EQ(AstraRecentTabsHelper::GetRecentTabCount(profile_.get()), 0u);
  EXPECT_TRUE(AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_.get()).empty());
  EXPECT_TRUE(AstraRecentTabsHelper::GetRecentTabs(profile_.get()).empty());
}

// ---------------------------------------------------------------------------
// Presentation settings: max recent tabs
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, SetMaxRecentTabs_ChangesValue) {
  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), 25);
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()), 25);
}

TEST_F(RecentTabsHelperTest, SetMaxRecentTabs_ClampsToMinimum) {
  // Setting to 0 or negative should clamp to 1.
  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), 0);
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()), 1);

  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), -5);
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()), 1);
}

TEST_F(RecentTabsHelperTest, SetMaxRecentTabs_ClampsToMaximum) {
  // Setting above the hard limit should clamp.
  int above_limit = AstraRecentTabsHelper::kMaxRecentTabsLimit + 50;
  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), above_limit);
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()),
            AstraRecentTabsHelper::kMaxRecentTabsLimit);
}

TEST_F(RecentTabsHelperTest, SetMaxRecentTabs_SameValueNoOp) {
  TestRecentTabsObserver* observer = AddTestObserver();

  // Set to the default value — should be a no-op.
  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(),
      AstraRecentTabsHelper::kDefaultMaxRecentTabs);

  // No observer notifications should have fired.
  EXPECT_EQ(observer->presentation_changed_count_, 0);
  EXPECT_EQ(observer->recent_tabs_changed_count_, 0);
}

TEST_F(RecentTabsHelperTest, SetMaxRecentTabs_NotifiesObservers) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), 20);

  // Both presentation changed and recent tabs changed should fire,
  // because changing the max count changes what's shown.
  EXPECT_GE(observer->presentation_changed_count_, 1);
  EXPECT_GE(observer->recent_tabs_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Presentation settings: show in sidebar
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, SetShowInSidebar_ChangesValue) {
  // Default is true.
  ASSERT_TRUE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));

  AstraRecentTabsHelper::SetShowInSidebar(profile_.get(), false);
  EXPECT_FALSE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));

  AstraRecentTabsHelper::SetShowInSidebar(profile_.get(), true);
  EXPECT_TRUE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));
}

TEST_F(RecentTabsHelperTest, SetShowInSidebar_SameValueNoOp) {
  TestRecentTabsObserver* observer = AddTestObserver();

  // Default is true; setting to true again should be a no-op.
  AstraRecentTabsHelper::SetShowInSidebar(profile_.get(), true);
  EXPECT_EQ(observer->presentation_changed_count_, 0);
}

TEST_F(RecentTabsHelperTest, SetShowInSidebar_NotifiesObserver) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::SetShowInSidebar(profile_.get(), false);
  EXPECT_EQ(observer->presentation_changed_count_, 1);

  // Should NOT fire recent tabs changed — sidebar visibility is purely
  // a presentation preference, not a data change.
}

TEST_F(RecentTabsHelperTest, ToggleShowInSidebar_TogglesState) {
  // Default is true.
  ASSERT_TRUE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));

  bool result = AstraRecentTabsHelper::ToggleShowInSidebar(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));

  result = AstraRecentTabsHelper::ToggleShowInSidebar(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));
}

// ---------------------------------------------------------------------------
// Presentation settings: show timestamps
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, SetShowTimestamps_ChangesValue) {
  // Default is true.
  ASSERT_TRUE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));

  AstraRecentTabsHelper::SetShowTimestamps(profile_.get(), false);
  EXPECT_FALSE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));

  AstraRecentTabsHelper::SetShowTimestamps(profile_.get(), true);
  EXPECT_TRUE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));
}

TEST_F(RecentTabsHelperTest, SetShowTimestamps_SameValueNoOp) {
  TestRecentTabsObserver* observer = AddTestObserver();

  // Default is true; setting to true again should be a no-op.
  AstraRecentTabsHelper::SetShowTimestamps(profile_.get(), true);
  EXPECT_EQ(observer->presentation_changed_count_, 0);
}

TEST_F(RecentTabsHelperTest, SetShowTimestamps_NotifiesObserver) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::SetShowTimestamps(profile_.get(), false);
  EXPECT_EQ(observer->presentation_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, ToggleShowTimestamps_TogglesState) {
  // Default is true.
  ASSERT_TRUE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));

  bool result = AstraRecentTabsHelper::ToggleShowTimestamps(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));

  result = AstraRecentTabsHelper::ToggleShowTimestamps(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));
}

// ---------------------------------------------------------------------------
// Query methods (projected data)
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, GetRecentlyClosedTabs_EmptyWhenNoService) {
  auto tabs = AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_.get());
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, GetRecentlyClosedTabs_RespectsMaxCount) {
  // Even though there are no tabs, the method should accept max_count.
  auto tabs = AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_.get(), 5);
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, GetRecentTabs_AliasForGetRecentlyClosedTabs) {
  // GetRecentTabs should behave the same as GetRecentlyClosedTabs.
  auto tabs1 = AstraRecentTabsHelper::GetRecentTabs(profile_.get());
  auto tabs2 = AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_.get());
  EXPECT_EQ(tabs1.size(), tabs2.size());
}

TEST_F(RecentTabsHelperTest, GetRecentTabCount_ZeroWhenNoService) {
  EXPECT_EQ(AstraRecentTabsHelper::GetRecentTabCount(profile_.get()), 0u);
}

TEST_F(RecentTabsHelperTest, HasRecentlyClosedTabs_FalseWhenNoService) {
  EXPECT_FALSE(AstraRecentTabsHelper::HasRecentlyClosedTabs(profile_.get()));
}

// ---------------------------------------------------------------------------
// Filtering methods
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, GetRecentTabsForWorkspace_EmptyWhenNoTabs) {
  auto tabs = AstraRecentTabsHelper::GetRecentTabsForWorkspace(
      profile_.get(), "workspace-1");
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, GetRecentTabsForWorkspace_EmptyWorkspaceId) {
  auto tabs = AstraRecentTabsHelper::GetRecentTabsForWorkspace(
      profile_.get(), "");
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, GetRecentTabsInTimeRange_EmptyWhenNoTabs) {
  auto tabs = AstraRecentTabsHelper::GetRecentTabsInTimeRange(
      profile_.get(), base::Time::Now() - base::Hours(1));
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, GetRecentTabsInTimeRange_WithUntil) {
  auto tabs = AstraRecentTabsHelper::GetRecentTabsInTimeRange(
      profile_.get(),
      base::Time::Now() - base::Hours(1),
      base::Time::Now());
  EXPECT_TRUE(tabs.empty());
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, SearchRecentTabs_EmptyWhenNoTabs) {
  auto results = AstraRecentTabsHelper::SearchRecentTabs(
      profile_.get(), u"anything");
  EXPECT_TRUE(results.empty());
}

TEST_F(RecentTabsHelperTest, SearchRecentTabs_EmptyQueryReturnsAll) {
  // With no tabs, empty query should also return empty.
  auto results = AstraRecentTabsHelper::SearchRecentTabs(
      profile_.get(), std::u16string());
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// Restore operations
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, RestoreMostRecentTab_NullWhenNoService) {
  content::WebContents* contents =
      AstraRecentTabsHelper::RestoreMostRecentTab(profile_.get());
  EXPECT_EQ(contents, nullptr);
}

TEST_F(RecentTabsHelperTest, RestoreTabById_NullWhenNoService) {
  content::WebContents* contents =
      AstraRecentTabsHelper::RestoreTabById(profile_.get(), 42);
  EXPECT_EQ(contents, nullptr);
}

TEST_F(RecentTabsHelperTest, RestoreAll_ZeroWhenNoService) {
  size_t restored = AstraRecentTabsHelper::RestoreAll(profile_.get());
  EXPECT_EQ(restored, 0u);
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, ClearAllRecentTabs_NoCrashWhenNoService) {
  // Should not crash even when there's no TabRestoreService.
  AstraRecentTabsHelper::ClearAllRecentTabs(profile_.get());
  // Still no tabs.
  EXPECT_FALSE(AstraRecentTabsHelper::HasRecentlyClosedTabs(profile_.get()));
}

TEST_F(RecentTabsHelperTest, ClearAllRecentTabs_NotifiesObservers) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::ClearAllRecentTabs(profile_.get());

  EXPECT_GE(observer->recent_tabs_cleared_count_, 1);
  EXPECT_GE(observer->recent_tabs_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, GetTotalClosedCount_ZeroWhenNoService) {
  EXPECT_EQ(AstraRecentTabsHelper::GetTotalClosedCount(profile_.get()), 0u);
}

TEST_F(RecentTabsHelperTest, GetSessionCount_ZeroWhenNoService) {
  EXPECT_EQ(AstraRecentTabsHelper::GetSessionCount(profile_.get()), 0u);
}

// ---------------------------------------------------------------------------
// Observer notifications (manual triggers)
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, ObserverFiresOnRecentTabsChanged) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::NotifyRecentTabsChanged();

  EXPECT_EQ(observer->recent_tabs_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, ObserverFiresOnTabClosed) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentlyClosedTab tab;
  tab.entry_id = 42;
  tab.title = u"Example Page";
  tab.url = GURL("https://example.com");

  AstraRecentTabsHelper::NotifyTabClosedToRecent(tab);

  EXPECT_EQ(observer->tab_closed_count_, 1);
  EXPECT_EQ(observer->last_closed_tab_id_, 42);
  EXPECT_EQ(observer->last_closed_tab_title_, u"Example Page");
  // Tab closed should also trigger the catch-all recent tabs changed.
  EXPECT_GE(observer->recent_tabs_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, ObserverFiresOnTabRestored) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::NotifyRecentTabRestored(7);

  EXPECT_EQ(observer->tab_restored_count_, 1);
  EXPECT_EQ(observer->last_restored_entry_id_, 7);
  // Restored should also trigger the catch-all recent tabs changed.
  EXPECT_GE(observer->recent_tabs_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, ObserverFiresOnRecentTabsCleared) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::NotifyRecentTabsCleared();

  EXPECT_EQ(observer->recent_tabs_cleared_count_, 1);
  // Cleared should also trigger the catch-all recent tabs changed.
  EXPECT_GE(observer->recent_tabs_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, ObserverFiresOnPresentationChanged) {
  TestRecentTabsObserver* observer = AddTestObserver();

  AstraRecentTabsHelper::NotifyRecentPresentationChanged();

  EXPECT_EQ(observer->presentation_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, MultipleObservers_AllNotified) {
  TestRecentTabsObserver* observer1 = AddTestObserver();
  TestRecentTabsObserver* observer2 = AddTestObserver();

  AstraRecentTabsHelper::NotifyRecentTabsChanged();

  EXPECT_EQ(observer1->recent_tabs_changed_count_, 1);
  EXPECT_EQ(observer2->recent_tabs_changed_count_, 1);
}

TEST_F(RecentTabsHelperTest, RemoveObserver_StopsNotifications) {
  TestRecentTabsObserver observer;
  AstraRecentTabsHelper::AddObserver(&observer);

  AstraRecentTabsHelper::NotifyRecentTabsChanged();
  EXPECT_EQ(observer.recent_tabs_changed_count_, 1);

  AstraRecentTabsHelper::RemoveObserver(&observer);

  AstraRecentTabsHelper::NotifyRecentTabsChanged();
  // Still 1, not incremented after removal.
  EXPECT_EQ(observer.recent_tabs_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

TEST(AstraRecentTabsObserverTest, DefaultImplementationsAreNoOps) {
  // Observer has default empty implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraRecentTabsHelper::Observer {};

  TestObserver observer;
  AstraRecentlyClosedTab tab;

  observer.OnRecentTabsChanged();
  observer.OnTabClosedToRecent(tab);
  observer.OnRecentTabRestored(0);
  observer.OnRecentTabsCleared();
  observer.OnRecentPresentationChanged();
  // No crash = success (default implementations are no-ops).
}

// ---------------------------------------------------------------------------
// Partial observer (only overrides one method)
// ---------------------------------------------------------------------------

namespace {

// Observer that overrides only OnRecentTabsChanged to verify defaults work.
class PartialRecentTabsObserver : public AstraRecentTabsHelper::Observer {
 public:
  void OnRecentTabsChanged() override {
    changed_count_++;
  }

  int changed_count_ = 0;
};

}  // namespace

TEST_F(RecentTabsHelperTest, PartialObserver_DefaultImplementationsWork) {
  PartialRecentTabsObserver partial_observer;
  AstraRecentTabsHelper::AddObserver(&partial_observer);

  // Trigger OnRecentTabsChanged — should be counted.
  AstraRecentTabsHelper::NotifyRecentTabsChanged();
  EXPECT_EQ(partial_observer.changed_count_, 1);

  // These should all compile and not crash (default implementations).
  AstraRecentlyClosedTab tab;
  AstraRecentTabsHelper::NotifyTabClosedToRecent(tab);
  AstraRecentTabsHelper::NotifyRecentTabRestored(1);
  AstraRecentTabsHelper::NotifyRecentTabsCleared();
  AstraRecentTabsHelper::NotifyRecentPresentationChanged();

  // Only OnRecentTabsChanged was overridden and should have fired more.
  // TabClosedToRecent, RecentTabsCleared also trigger OnRecentTabsChanged.
  // So: 1 (explicit) + 1 (TabClosedToRecent) + 1 (RecentTabsCleared) = 3.
  EXPECT_GE(partial_observer.changed_count_, 3);

  AstraRecentTabsHelper::RemoveObserver(&partial_observer);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, NullProfile_AllMethodsReturnDefaults) {
  // All methods should handle null profile gracefully.
  EXPECT_TRUE(AstraRecentTabsHelper::GetRecentlyClosedTabs(nullptr).empty());
  EXPECT_TRUE(AstraRecentTabsHelper::GetRecentTabs(nullptr).empty());
  EXPECT_EQ(AstraRecentTabsHelper::GetRecentTabCount(nullptr), 0u);
  EXPECT_FALSE(AstraRecentTabsHelper::HasRecentlyClosedTabs(nullptr));
  EXPECT_EQ(AstraRecentTabsHelper::RestoreMostRecentTab(nullptr), nullptr);
  EXPECT_EQ(AstraRecentTabsHelper::RestoreTabById(nullptr, 1), nullptr);
  EXPECT_EQ(AstraRecentTabsHelper::RestoreAll(nullptr), 0u);
  EXPECT_EQ(AstraRecentTabsHelper::GetTotalClosedCount(nullptr), 0u);
  EXPECT_EQ(AstraRecentTabsHelper::GetSessionCount(nullptr), 0u);

  // Presentation settings should return defaults for null profile.
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(nullptr),
            AstraRecentTabsHelper::kDefaultMaxRecentTabs);
  EXPECT_EQ(AstraRecentTabsHelper::GetShowInSidebar(nullptr),
            AstraRecentTabsHelper::kDefaultShowInSidebar);
  EXPECT_EQ(AstraRecentTabsHelper::GetShowTimestamps(nullptr),
            AstraRecentTabsHelper::kDefaultShowTimestamps);

  // These should not crash with null profile.
  AstraRecentTabsHelper::SetMaxRecentTabs(nullptr, 20);
  AstraRecentTabsHelper::SetShowInSidebar(nullptr, false);
  AstraRecentTabsHelper::SetShowTimestamps(nullptr, false);
  AstraRecentTabsHelper::ClearAllRecentTabs(nullptr);

  // Filter and search methods should not crash.
  EXPECT_TRUE(
      AstraRecentTabsHelper::GetRecentTabsForWorkspace(nullptr, "ws1").empty());
  EXPECT_TRUE(AstraRecentTabsHelper::GetRecentTabsInTimeRange(
      nullptr, base::Time::Now()).empty());
  EXPECT_TRUE(
      AstraRecentTabsHelper::SearchRecentTabs(nullptr, u"query").empty());
}

TEST_F(RecentTabsHelperTest, NullObserver_NoCrash) {
  // Adding or removing a null observer should not crash.
  AstraRecentTabsHelper::AddObserver(nullptr);
  AstraRecentTabsHelper::RemoveObserver(nullptr);
  // No crash = success.
}

TEST_F(RecentTabsHelperTest, MaxCountLimitConstant) {
  // The hard limit should be greater than the default.
  EXPECT_GT(AstraRecentTabsHelper::kMaxRecentTabsLimit,
            AstraRecentTabsHelper::kDefaultMaxRecentTabs);
  EXPECT_GT(AstraRecentTabsHelper::kMaxRecentTabsLimit, 0);
}

TEST_F(RecentTabsHelperTest, AstraRecentlyClosedTab_DefaultValues) {
  AstraRecentlyClosedTab tab;
  EXPECT_EQ(tab.entry_id, 0);
  EXPECT_TRUE(tab.title.empty());
  EXPECT_TRUE(tab.url.is_empty());
  EXPECT_TRUE(tab.close_time.is_null());
  EXPECT_EQ(tab.list_index, 0);
  EXPECT_TRUE(tab.workspace_id.empty());
  EXPECT_FALSE(tab.has_favicon);
}

TEST_F(RecentTabsHelperTest, GetRecentTabsForWorkspace_RespectsMaxCount) {
  // Should accept max_count parameter and not crash.
  auto tabs = AstraRecentTabsHelper::GetRecentTabsForWorkspace(
      profile_.get(), "ws1", 5);
  EXPECT_TRUE(tabs.empty());
}

TEST_F(RecentTabsHelperTest, SearchRecentTabs_RespectsMaxCount) {
  // Should accept max_count parameter and not crash.
  auto tabs = AstraRecentTabsHelper::SearchRecentTabs(
      profile_.get(), u"query", 5);
  EXPECT_TRUE(tabs.empty());
}

// ---------------------------------------------------------------------------
// Persistence round-trip (presentation settings)
// ---------------------------------------------------------------------------

TEST_F(RecentTabsHelperTest, Persistence_MaxRecentTabsSurvivesProfileReuse) {
  // Set a non-default value.
  AstraRecentTabsHelper::SetMaxRecentTabs(profile_.get(), 42);
  ASSERT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()), 42);

  // The TestingProfile's PrefService persists across reads.
  // Verify the value is still there.
  EXPECT_EQ(AstraRecentTabsHelper::GetMaxRecentTabs(profile_.get()), 42);
}

TEST_F(RecentTabsHelperTest, Persistence_ShowInSidebarSurvivesProfileReuse) {
  AstraRecentTabsHelper::SetShowInSidebar(profile_.get(), false);
  ASSERT_FALSE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));

  EXPECT_FALSE(AstraRecentTabsHelper::GetShowInSidebar(profile_.get()));
}

TEST_F(RecentTabsHelperTest, Persistence_ShowTimestampsSurvivesProfileReuse) {
  AstraRecentTabsHelper::SetShowTimestamps(profile_.get(), false);
  ASSERT_FALSE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));

  EXPECT_FALSE(AstraRecentTabsHelper::GetShowTimestamps(profile_.get()));
}

TEST_F(RecentTabsHelperTest, Persistence_PrefsRegisteredInRegistry) {
  // Verify all recent tabs pref keys are registered in the pref service.
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(prefs, nullptr);

  // Each pref should be findable and have its default value.
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefRecentTabsMaxCount),
            prefs::kDefaultRecentTabsMaxCount);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefRecentTabsShowInSidebar),
            prefs::kDefaultRecentTabsShowInSidebar);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefRecentTabsShowTimestamps),
            prefs::kDefaultRecentTabsShowTimestamps);
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

TEST(AstraRecentTabsConstantsTest, DefaultMaxRecentTabsIsPositive) {
  EXPECT_GT(AstraRecentTabsHelper::kDefaultMaxRecentTabs, 0);
}

TEST(AstraRecentTabsConstantsTest, MaxLimitGreaterThanDefault) {
  EXPECT_GT(AstraRecentTabsHelper::kMaxRecentTabsLimit,
            AstraRecentTabsHelper::kDefaultMaxRecentTabs);
}

TEST(AstraRecentTabsConstantsTest, DefaultShowInSidebar) {
  EXPECT_TRUE(AstraRecentTabsHelper::kDefaultShowInSidebar);
}

TEST(AstraRecentTabsConstantsTest, DefaultShowTimestamps) {
  EXPECT_TRUE(AstraRecentTabsHelper::kDefaultShowTimestamps);
}

// ---------------------------------------------------------------------------
// Pref key constants
// ---------------------------------------------------------------------------

TEST(AstraRecentTabsPrefsTest, PrefKeysHaveCorrectFormat) {
  // All recent tabs pref keys should start with "astra.recent_tabs."
  EXPECT_EQ(std::string(prefs::kPrefRecentTabsMaxCount).substr(0, 18),
            "astra.recent_tabs.");
  EXPECT_EQ(std::string(prefs::kPrefRecentTabsShowInSidebar).substr(0, 18),
            "astra.recent_tabs.");
  EXPECT_EQ(std::string(prefs::kPrefRecentTabsShowTimestamps).substr(0, 18),
            "astra.recent_tabs.");
}

TEST(AstraRecentTabsPrefsTest, DefaultsAreValid) {
  EXPECT_GT(prefs::kDefaultRecentTabsMaxCount, 0);
  // Default should be <= the hard limit defined in the helper.
  EXPECT_LE(prefs::kDefaultRecentTabsMaxCount,
            AstraRecentTabsHelper::kMaxRecentTabsLimit);
}

// ---------------------------------------------------------------------------
// TODO(astra): Additional tests needed
// ---------------------------------------------------------------------------
//
// Browser tests (require full Chromium browser test harness):
//   - TabCloseAddsToRecentTabs
//   - RestoreMostRecentTab_OpensTab
//   - RestoreTabById_OpensCorrectTab
//   - ClearAllRecentTabs_ClearsTabRestoreService
//   - GetRecentTabsForWorkspace_FiltersCorrectly
//   - SearchRecentTabs_FindsByTitleAndUrl
//   - TabRestoreServiceObserverBridge_NotifiesOnChanges
//   - WorkspaceMetadata_PreservedAcrossCloseAndRestore
//
// TODO(astra): Add browser_tests for recent tabs helper integration with real
// TabRestoreService and Profile.
// Chromium component: InProcessBrowserTest + TabRestoreService.

}  // namespace astra
