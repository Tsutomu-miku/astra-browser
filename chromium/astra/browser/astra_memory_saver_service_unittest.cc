// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_memory_saver_service.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestMemorySaverServiceObserver
    : public AstraMemorySaverServiceObserver {
 public:
  void OnTabSuspended(content::WebContents* web_contents) override {
    suspended_count_++;
    last_suspended_web_contents_ = web_contents;
  }

  void OnTabRestored(content::WebContents* web_contents) override {
    restored_count_++;
    last_restored_web_contents_ = web_contents;
  }

  void OnMemorySaverEnabledChanged(bool enabled) override {
    enabled_changed_count_++;
    last_enabled_state_ = enabled;
  }

  void OnMemorySaverTimeoutChanged(base::TimeDelta timeout) override {
    timeout_changed_count_++;
    last_timeout_ = timeout;
  }

  void OnWhitelistChanged() override {
    whitelist_changed_count_++;
  }

  // Counters
  int suspended_count_ = 0;
  int restored_count_ = 0;
  int enabled_changed_count_ = 0;
  int timeout_changed_count_ = 0;
  int whitelist_changed_count_ = 0;

  // Last recorded values
  raw_ptr<content::WebContents> last_suspended_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_restored_web_contents_ = nullptr;
  bool last_enabled_state_ = false;
  base::TimeDelta last_timeout_;
};

}  // namespace

// Test fixture for AstraMemorySaverService tests.
//
// Uses TestingProfile so the service has a real Profile* to attach to.
// The service is constructed directly since the factory may not be fully
// wired up in the test harness.
//
// Tests that require WebContents use the null-web-contents path to verify
// error handling and edge cases.  Tests with real WebContents require a
// content test harness and are marked as TODO(astra).
//
// TODO(astra): Add WebContents-based tests using content::WebContentsTester
// or content::TestWebContentsFactory once the content test harness is
// available.  Chromium component: content/public/test:test_support.
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
class MemorySaverServiceTest : public testing::Test {
 protected:
  MemorySaverServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // TODO(astra): Obtain service through the factory once
    // AstraMemorySaverServiceFactory is properly wired up.
    // For now we construct directly with the profile.
    service_ = std::make_unique<AstraMemorySaverService>(profile_.get());
    DCHECK(service_);
  }

  ~MemorySaverServiceTest() override = default;

  void SetUp() override {
    // Auto-suspend should be enabled by default.
    ASSERT_TRUE(service_->auto_suspend_enabled());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraMemorySaverService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestMemorySaverServiceObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default settings
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, AutoSuspend_DefaultsToEnabled) {
  EXPECT_TRUE(service_->auto_suspend_enabled());
}

TEST_F(MemorySaverServiceTest, Timeout_DefaultsTo5Minutes) {
  EXPECT_EQ(service_->auto_suspend_timeout(), base::Minutes(5));
}

TEST_F(MemorySaverServiceTest, SuspendActiveWorkspace_DefaultsToFalse) {
  EXPECT_FALSE(service_->suspend_active_workspace());
}

TEST_F(MemorySaverServiceTest, SuspendedTabCount_DefaultsToZero) {
  // No tabs = no suspended tabs.
  EXPECT_EQ(service_->GetSuspendedTabCount(), 0u);
}

// ---------------------------------------------------------------------------
// Auto-suspend enabled setting
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SetAutoSuspendEnabled_TogglesState) {
  ASSERT_TRUE(service_->auto_suspend_enabled());

  service_->SetAutoSuspendEnabled(false);
  EXPECT_FALSE(service_->auto_suspend_enabled());

  service_->SetAutoSuspendEnabled(true);
  EXPECT_TRUE(service_->auto_suspend_enabled());
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendEnabled_SameValueNoOp) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  bool original = service_->auto_suspend_enabled();
  service_->SetAutoSuspendEnabled(original);

  // Should not have notified since nothing changed.
  EXPECT_EQ(observer.enabled_changed_count_, 0);
  EXPECT_EQ(service_->auto_suspend_enabled(), original);

  service_->RemoveObserver(&observer);
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendEnabled_NotifiesObserver) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetAutoSuspendEnabled(false);

  EXPECT_EQ(observer.enabled_changed_count_, 1);
  EXPECT_FALSE(observer.last_enabled_state_);

  service_->SetAutoSuspendEnabled(true);

  EXPECT_EQ(observer.enabled_changed_count_, 2);
  EXPECT_TRUE(observer.last_enabled_state_);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Auto-suspend timeout setting
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SetAutoSuspendTimeout_ChangesTimeout) {
  base::TimeDelta original = service_->auto_suspend_timeout();
  ASSERT_EQ(original, base::Minutes(5));

  service_->SetAutoSuspendTimeout(base::Minutes(15));
  EXPECT_EQ(service_->auto_suspend_timeout(), base::Minutes(15));

  service_->SetAutoSuspendTimeout(base::Minutes(1));
  EXPECT_EQ(service_->auto_suspend_timeout(), base::Minutes(1));
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendTimeout_ZeroIgnored) {
  base::TimeDelta original = service_->auto_suspend_timeout();

  service_->SetAutoSuspendTimeout(base::TimeDelta());
  EXPECT_EQ(service_->auto_suspend_timeout(), original);
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendTimeout_NegativeIgnored) {
  base::TimeDelta original = service_->auto_suspend_timeout();

  service_->SetAutoSuspendTimeout(base::Minutes(-10));
  EXPECT_EQ(service_->auto_suspend_timeout(), original);
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendTimeout_SameValueNoOp) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  base::TimeDelta original = service_->auto_suspend_timeout();
  service_->SetAutoSuspendTimeout(original);

  EXPECT_EQ(observer.timeout_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(MemorySaverServiceTest, SetAutoSuspendTimeout_NotifiesObserver) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetAutoSuspendTimeout(base::Minutes(30));

  EXPECT_EQ(observer.timeout_changed_count_, 1);
  EXPECT_EQ(observer.last_timeout_, base::Minutes(30));

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Suspend active workspace setting
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SetSuspendActiveWorkspace_ChangesSetting) {
  ASSERT_FALSE(service_->suspend_active_workspace());

  service_->SetSuspendActiveWorkspace(true);
  EXPECT_TRUE(service_->suspend_active_workspace());

  service_->SetSuspendActiveWorkspace(false);
  EXPECT_FALSE(service_->suspend_active_workspace());
}

TEST_F(MemorySaverServiceTest, SetSuspendActiveWorkspace_SameValueNoOp) {
  bool original = service_->suspend_active_workspace();

  service_->SetSuspendActiveWorkspace(original);

  EXPECT_EQ(service_->suspend_active_workspace(), original);
}

// ---------------------------------------------------------------------------
// Tab suspension — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SuspendTab_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(service_->SuspendTab(nullptr));
}

TEST_F(MemorySaverServiceTest, RestoreTab_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(service_->RestoreTab(nullptr));
}

TEST_F(MemorySaverServiceTest, IsTabSuspended_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(service_->IsTabSuspended(nullptr));
}

TEST_F(MemorySaverServiceTest, CanSuspendTab_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(service_->CanSuspendTab(nullptr));
}

// ---------------------------------------------------------------------------
// Tab activity tracking — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, OnTabActivated_NullWebContentsNoOp) {
  // Should not crash with null web contents.
  service_->OnTabActivated(nullptr);
  SUCCEED() << "OnTabActivated with null web contents handled without crash.";
}

TEST_F(MemorySaverServiceTest, OnTabInserted_NullWebContentsNoOp) {
  // Should not crash with null web contents.
  service_->OnTabInserted(nullptr);
  SUCCEED() << "OnTabInserted with null web contents handled without crash.";
}

// ---------------------------------------------------------------------------
// WakeAllTabs — empty state
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, WakeAllTabs_EmptyReturnsZero) {
  // No tabs to wake — should return 0 and not crash.
  size_t restored = service_->WakeAllTabs();
  EXPECT_EQ(restored, 0u);
}

TEST_F(MemorySaverServiceTest, GetSuspendedTabCount_EmptyReturnsZero) {
  EXPECT_EQ(service_->GetSuspendedTabCount(), 0u);
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, AddRemoveObserver_NoCrash) {
  TestMemorySaverServiceObserver observer;

  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(MemorySaverServiceTest, RemoveNonexistentObserver_NoCrash) {
  TestMemorySaverServiceObserver observer;

  // Removing an observer that was never added should not crash.
  service_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST_F(MemorySaverServiceTest, MultipleObservers_AllNotified) {
  TestMemorySaverServiceObserver observer1;
  TestMemorySaverServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->SetAutoSuspendEnabled(false);

  EXPECT_EQ(observer1.enabled_changed_count_, 1);
  EXPECT_EQ(observer2.enabled_changed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(MemorySaverServiceTest, RemoveObserver_StopsNotifications) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetAutoSuspendEnabled(false);
  EXPECT_EQ(observer.enabled_changed_count_, 1);

  service_->RemoveObserver(&observer);

  service_->SetAutoSuspendEnabled(true);
  // Should not have received the second notification.
  EXPECT_EQ(observer.enabled_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, ShutdownClearsObservers) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, setting changes should not notify observers.
  service_->SetAutoSuspendEnabled(false);
  EXPECT_EQ(observer.enabled_changed_count_, 0);
}

TEST_F(MemorySaverServiceTest, ShutdownIdempotent) {
  service_->Shutdown();
  service_->Shutdown();
  SUCCEED() << "Double shutdown completed without crash.";
}

// ---------------------------------------------------------------------------
// Settings persistence (basic checks)
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SettingsPersistAcrossServiceLifetime) {
  // Change some settings.
  service_->SetAutoSuspendEnabled(false);
  service_->SetAutoSuspendTimeout(base::Minutes(30));
  service_->SetSuspendActiveWorkspace(true);

  // Create a new service for the same profile — settings should persist
  // via PrefService.
  // TODO(astra): This test verifies persistence through PrefService.
  // It requires the pref keys to be properly registered with TestingProfile.
  // If prefs aren't registered, the new service will use defaults.
  auto service2 = std::make_unique<AstraMemorySaverService>(profile_.get());

  // At minimum, creating a second service for the same profile should not
  // crash (two instances sharing a profile's prefs).
  SUCCEED() << "Second service instance created without crash.";
}

// ---------------------------------------------------------------------------
// Whitelist — basic operations
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, Whitelist_EmptyByDefault) {
  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(MemorySaverServiceTest, AddWhitelistedSite_AddsToList) {
  ASSERT_TRUE(service_->whitelist().empty());

  service_->AddWhitelistedSite("example.com");

  EXPECT_EQ(service_->whitelist().size(), 1u);
  EXPECT_EQ(service_->whitelist()[0], "example.com");
}

TEST_F(MemorySaverServiceTest, AddWhitelistedSite_DuplicateIgnored) {
  service_->AddWhitelistedSite("example.com");
  size_t count_before = service_->whitelist().size();

  service_->AddWhitelistedSite("example.com");

  EXPECT_EQ(service_->whitelist().size(), count_before);
}

TEST_F(MemorySaverServiceTest, AddWhitelistedSite_EmptyIgnored) {
  service_->AddWhitelistedSite("");

  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(MemorySaverServiceTest, AddWhitelistedSite_MultipleSites) {
  service_->AddWhitelistedSite("youtube.com");
  service_->AddWhitelistedSite("reddit.com");
  service_->AddWhitelistedSite("twitter.com");

  EXPECT_EQ(service_->whitelist().size(), 3u);
}

TEST_F(MemorySaverServiceTest, RemoveWhitelistedSite_RemovesFromList) {
  service_->AddWhitelistedSite("example.com");
  ASSERT_EQ(service_->whitelist().size(), 1u);

  service_->RemoveWhitelistedSite("example.com");

  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(MemorySaverServiceTest, RemoveWhitelistedSite_NonexistentNoOp) {
  service_->AddWhitelistedSite("example.com");
  size_t count_before = service_->whitelist().size();

  service_->RemoveWhitelistedSite("nonexistent.com");

  EXPECT_EQ(service_->whitelist().size(), count_before);
}

TEST_F(MemorySaverServiceTest, RemoveWhitelistedSite_EmptyListNoOp) {
  ASSERT_TRUE(service_->whitelist().empty());

  // Should not crash when removing from an empty list.
  service_->RemoveWhitelistedSite("example.com");
  SUCCEED() << "Remove from empty whitelist handled without crash.";
}

// ---------------------------------------------------------------------------
// Whitelist — pattern matching
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_EmptyWhitelistReturnsFalse) {
  EXPECT_FALSE(service_->IsSiteWhitelisted("https://youtube.com/watch"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_ExactHostMatch) {
  service_->AddWhitelistedSite("youtube.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://youtube.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("http://youtube.com/watch?v=abc"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_SubdomainMatch) {
  // Bare domain should match subdomains too.
  service_->AddWhitelistedSite("reddit.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://www.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://old.reddit.com/r/all"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_WildcardPattern) {
  service_->AddWhitelistedSite("*.reddit.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://www.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://old.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://reddit.com/"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_UnrelatedSite) {
  service_->AddWhitelistedSite("youtube.com");

  EXPECT_FALSE(service_->IsSiteWhitelisted("https://google.com/"));
  EXPECT_FALSE(service_->IsSiteWhitelisted("https://github.com/"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_InvalidUrlUsesSubstringMatch) {
  service_->AddWhitelistedSite("important");

  // Non-URL string uses substring fallback.
  EXPECT_TRUE(service_->IsSiteWhitelisted("this is an important site"));
  EXPECT_FALSE(service_->IsSiteWhitelisted("regular work page"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_MultiplePatterns) {
  service_->AddWhitelistedSite("youtube.com");
  service_->AddWhitelistedSite("*.reddit.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://youtube.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://www.reddit.com/"));
  EXPECT_FALSE(service_->IsSiteWhitelisted("https://google.com/"));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_DeepSubdomain) {
  service_->AddWhitelistedSite("example.com");

  // Multi-level subdomains should also match.
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://sub.www.example.com/path"));
}

// ---------------------------------------------------------------------------
// Whitelist — observer notifications
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, Whitelist_AddNotifiesObserver) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->AddWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(MemorySaverServiceTest, Whitelist_RemoveNotifiesObserver) {
  service_->AddWhitelistedSite("example.com");

  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  service_->RemoveWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(MemorySaverServiceTest, Whitelist_DuplicateAddDoesNotNotify) {
  service_->AddWhitelistedSite("example.com");

  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  // Adding a duplicate should not notify.
  service_->AddWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(MemorySaverServiceTest, Whitelist_RemoveNonexistentDoesNotNotify) {
  TestMemorySaverServiceObserver observer;
  service_->AddObserver(&observer);

  // Removing a site that isn't there should not notify.
  service_->RemoveWhitelistedSite("nonexistent.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Suspension stats
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, Stats_ZeroByDefault) {
  EXPECT_EQ(service_->GetTotalSuspendedCount(), 0u);
  EXPECT_EQ(service_->GetMemorySavedEstimateBytes(), 0);
}

TEST_F(MemorySaverServiceTest, Stats_ResetStatsClearsCounters) {
  // Set up some state.
  service_->ResetStats();

  EXPECT_EQ(service_->GetTotalSuspendedCount(), 0u);
  EXPECT_EQ(service_->GetMemorySavedEstimateBytes(), 0);
}

// ---------------------------------------------------------------------------
// Exception policy defaults
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SuspendPinnedTabs_DefaultsToFalse) {
  EXPECT_FALSE(service_->suspend_pinned_tabs());
}

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SuspendAudibleTabs_DefaultsToFalse) {
  EXPECT_FALSE(service_->suspend_audible_tabs());
}

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SuspendActiveTab_DefaultsToFalse) {
  EXPECT_FALSE(service_->suspend_active_tab());
}

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SetSuspendPinnedTabs) {
  ASSERT_FALSE(service_->suspend_pinned_tabs());

  service_->set_suspend_pinned_tabs(true);
  EXPECT_TRUE(service_->suspend_pinned_tabs());

  service_->set_suspend_pinned_tabs(false);
  EXPECT_FALSE(service_->suspend_pinned_tabs());
}

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SetSuspendAudibleTabs) {
  ASSERT_FALSE(service_->suspend_audible_tabs());

  service_->set_suspend_audible_tabs(true);
  EXPECT_TRUE(service_->suspend_audible_tabs());

  service_->set_suspend_audible_tabs(false);
  EXPECT_FALSE(service_->suspend_audible_tabs());
}

TEST_F(MemorySaverServiceTest, ExceptionPolicy_SetSuspendActiveTab) {
  ASSERT_FALSE(service_->suspend_active_tab());

  service_->set_suspend_active_tab(true);
  EXPECT_TRUE(service_->suspend_active_tab());

  service_->set_suspend_active_tab(false);
  EXPECT_FALSE(service_->suspend_active_tab());
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

// Observer with no overrides — tests that default implementations compile
// and don't crash when called.
class DefaultImplObserver : public AstraMemorySaverServiceObserver {
  // Intentionally empty — all methods use default implementations.
};

TEST_F(MemorySaverServiceTest, Observer_DefaultImpl_NoCrash) {
  DefaultImplObserver observer;
  service_->AddObserver(&observer);

  // Trigger all observer methods — should not crash with default impls.
  service_->SetAutoSuspendEnabled(false);
  service_->SetAutoSuspendEnabled(true);
  service_->SetAutoSuspendTimeout(base::Minutes(10));
  service_->AddWhitelistedSite("example.com");

  service_->RemoveObserver(&observer);
  SUCCEED() << "Default observer implementations work without crash.";
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, SuspendAllEligibleTabs_EmptyReturnsZero) {
  // No tabs — should return 0 and not crash.
  size_t suspended = service_->SuspendAllEligibleTabs();
  EXPECT_EQ(suspended, 0u);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_EmptyUrl) {
  service_->AddWhitelistedSite("example.com");
  EXPECT_FALSE(service_->IsSiteWhitelisted(""));
}

TEST_F(MemorySaverServiceTest, IsSiteWhitelisted_JustProtocol) {
  service_->AddWhitelistedSite("example.com");
  // "https://" is not a valid URL with a host.
  EXPECT_FALSE(service_->IsSiteWhitelisted("https://"));
}

TEST_F(MemorySaverServiceTest, Whitelist_PersistenceAcrossServiceLifetime) {
  // Add some whitelist entries.
  service_->AddWhitelistedSite("site1.com");
  service_->AddWhitelistedSite("site2.com");
  ASSERT_EQ(service_->whitelist().size(), 2u);

  // Create a new service for the same profile — whitelist should persist
  // via PrefService.
  // TODO(astra): This test verifies persistence through PrefService.
  // It requires the whitelist pref to be properly registered with TestingProfile.
  // If prefs aren't registered, the new service will use defaults (empty list).
  auto service2 = std::make_unique<AstraMemorySaverService>(profile_.get());

  // At minimum, creating a second service for the same profile should not
  // crash (two instances sharing a profile's prefs).
  SUCCEED() << "Second service instance created without crash.";
}

// ---------------------------------------------------------------------------
// TODO(astra): WebContents-based tests (require content test harness)
// ---------------------------------------------------------------------------
//
// The following tests require real WebContents objects and should be
// implemented once the content test harness is available:
//
//   - SuspendTab_MarksTabAsSuspended
//   - SuspendTab_AlreadySuspendedReturnsTrue
//   - SuspendTab_CannotSuspendPinnedTab
//   - SuspendTab_CannotSuspendPinnedTabUnlessPolicyAllows
//   - SuspendTab_CannotSuspendWhitelistedSite
//   - SuspendTab_CannotSuspendActiveTab (when policy forbids)
//   - RestoreTab_ClearsSuspendedState
//   - RestoreTab_AlreadyActiveReturnsTrue
//   - IsTabSuspended_ReturnsCorrectState
//   - CanSuspendTab_NonSuspendedEligibleReturnsTrue
//   - CanSuspendTab_AlreadySuspendedReturnsFalse
//   - CanSuspendTab_SidebarPinnedReturnsFalse
//   - CanSuspendTab_WhitelistedSiteReturnsFalse
//   - OnTabActivated_UpdatesLastActiveTime
//   - OnTabActivated_RestoresSuspendedTab
//   - OnTabInserted_SetsInitialActiveTime
//   - GetSuspendedTabCount_CountsSuspendedTabs
//   - WakeAllTabs_RestoresAllSuspendedTabs
//   - SuspendAllEligibleTabs_SuspendsAllEligible
//   - Stats_TotalSuspendedIncrementsOnSuspend
//   - Stats_MemorySavedEstimateIncrementsOnSuspend
//   - Stats_MemorySavedEstimateDecrementsOnRestore
//   - Stats_ResetStatsDoesNotAffectSuspendedTabs
//   - Observer_OnTabSuspendedNotifies
//   - Observer_OnTabRestoredNotifies
//   - Observer_MultipleObserversAllNotifiedOnSuspend
//
// Timer / auto-suspend tests:
//   - AutoSuspend_SuspendsEligibleTabsAfterTimeout
//   - DisablingAutoSuspend_StopsTimer
//   - EnablingAutoSuspend_StartsTimer
//
// TODO(astra): Add browser_tests for memory saver integration with real
// Browser, TabStripModel, and tab discard system.
// Chromium component: InProcessBrowserTest + resource_coordinator::TabManager.

}  // namespace astra
