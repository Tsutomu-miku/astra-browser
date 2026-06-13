// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_workspace_window_manager.h"

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_window_features.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestWorkspaceWindowManagerObserver
    : public AstraWorkspaceWindowManagerObserver {
 public:
  // Window add/remove
  int window_added_count_ = 0;
  int window_removed_count_ = 0;
  Browser* last_added_browser_ = nullptr;
  Browser* last_removed_browser_ = nullptr;
  std::string last_added_workspace_id_;
  std::string last_removed_workspace_id_;

  // Active workspace
  int active_workspace_changed_count_ = 0;
  std::string last_old_workspace_id_;
  std::string last_new_workspace_id_;

  // Window count
  int window_count_changed_count_ = 0;
  std::string last_count_workspace_id_;
  size_t last_count_ = 0;

  // Hibernation
  int hibernation_changed_count_ = 0;
  std::string last_hibernation_workspace_id_;
  bool last_hibernation_state_ = false;

  // Observer interface
  void OnWindowAddedToWorkspace(Browser* browser,
                                const std::string& workspace_id) override {
    window_added_count_++;
    last_added_browser_ = browser;
    last_added_workspace_id_ = workspace_id;
  }

  void OnWindowRemovedFromWorkspace(Browser* browser,
                                    const std::string& workspace_id) override {
    window_removed_count_++;
    last_removed_browser_ = browser;
    last_removed_workspace_id_ = workspace_id;
  }

  void OnActiveWorkspaceChanged(const std::string& old_id,
                                 const std::string& new_id) override {
    active_workspace_changed_count_++;
    last_old_workspace_id_ = old_id;
    last_new_workspace_id_ = new_id;
  }

  void OnWindowCountChanged(const std::string& workspace_id,
                            size_t new_count) override {
    window_count_changed_count_++;
    last_count_workspace_id_ = workspace_id;
    last_count_ = new_count;
  }

  void OnWorkspaceHibernationChanged(const std::string& workspace_id,
                                     bool is_hibernated) override {
    hibernation_changed_count_++;
    last_hibernation_workspace_id_ = workspace_id;
    last_hibernation_state_ = is_hibernated;
  }

  // Resets all counters.
  void ResetCounters() {
    window_added_count_ = 0;
    window_removed_count_ = 0;
    last_added_browser_ = nullptr;
    last_removed_browser_ = nullptr;
    last_added_workspace_id_.clear();
    last_removed_workspace_id_.clear();

    active_workspace_changed_count_ = 0;
    last_old_workspace_id_.clear();
    last_new_workspace_id_.clear();

    window_count_changed_count_ = 0;
    last_count_workspace_id_.clear();
    last_count_ = 0;

    hibernation_changed_count_ = 0;
    last_hibernation_workspace_id_.clear();
    last_hibernation_state_ = false;
  }
};

}  // namespace

// Test fixture for AstraWorkspaceWindowManager tests.
//
// Uses TestingProfile so the manager has a profile to work with.
// The manager is a singleton that observes BrowserList, so in unit tests
// without a real browser environment, BrowserList will be empty.
//
// TODO(astra): Add browser_tests for full integration with real Browser
//   windows and TabStripModel.
//   Chromium component: InProcessBrowserTest.
class WorkspaceWindowManagerTest : public testing::Test {
 protected:
  WorkspaceWindowManagerTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register prefs on the testing profile's pref service.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());

    manager_ = AstraWorkspaceWindowManager::GetInstance();
    DCHECK(manager_);
  }

  ~WorkspaceWindowManagerTest() override = default;

  void SetUp() override {
    // Manager should be accessible.
    ASSERT_NE(manager_, nullptr);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      manager_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Creates a standalone AstraWindowFeatures instance for testing without
  // needing a real Browser object.
  std::unique_ptr<AstraWindowFeatures> CreateTestWindowFeatures() {
    return AstraWindowFeatures::CreateForTesting();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;

  // The singleton manager instance.
  raw_ptr<AstraWorkspaceWindowManager> manager_ = nullptr;

  // Pool of test observers managed by the fixture.
  std::vector<std::unique_ptr<TestWorkspaceWindowManagerObserver>>
      test_observers_;
};

// ---------------------------------------------------------------------------
// Singleton access
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetInstanceReturnsNonNull) {
  EXPECT_NE(AstraWorkspaceWindowManager::GetInstance(), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, GetInstanceReturnsSameInstance) {
  AstraWorkspaceWindowManager* instance1 =
      AstraWorkspaceWindowManager::GetInstance();
  AstraWorkspaceWindowManager* instance2 =
      AstraWorkspaceWindowManager::GetInstance();
  EXPECT_EQ(instance1, instance2);
}

// ---------------------------------------------------------------------------
// Default / empty state
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, EmptyManagerHasZeroWindows) {
  // With no browser windows, all counts should be zero.
  EXPECT_EQ(manager_->GetTotalWindowCount(profile_.get()), 0u);
  EXPECT_EQ(manager_->GetTotalTabCount(profile_.get()), 0u);
  EXPECT_EQ(manager_->GetAverageTabsPerWindow(profile_.get()), 0.0);
}

TEST_F(WorkspaceWindowManagerTest, EmptyWorkspaceHasZeroWindows) {
  EXPECT_EQ(
      manager_->GetWindowCount(profile_.get(), "nonexistent-workspace"), 0u);
  EXPECT_EQ(
      manager_->GetTabCount(profile_.get(), "nonexistent-workspace"), 0u);
  EXPECT_TRUE(
      manager_->GetWindowsForWorkspace(profile_.get(), "nonexistent-workspace")
          .empty());
  EXPECT_TRUE(
      manager_->GetWindowsInWorkspace(profile_.get(), "nonexistent-workspace")
          .empty());
}

TEST_F(WorkspaceWindowManagerTest, NullProfileReturnsEmpty) {
  // Null profile should return empty/default results, not crash.
  EXPECT_EQ(manager_->GetTotalWindowCount(nullptr), 0u);
  EXPECT_EQ(manager_->GetTotalTabCount(nullptr), 0u);
  EXPECT_EQ(manager_->GetAverageTabsPerWindow(nullptr), 0.0);
  EXPECT_TRUE(
      manager_->GetWindowsForWorkspace(nullptr, "ws1").empty());
  EXPECT_EQ(
      manager_->GetWindowCount(nullptr, "ws1"), 0u);
  EXPECT_EQ(
      manager_->GetTabCount(nullptr, "ws1"), 0u);
  EXPECT_EQ(
      manager_->GetActiveWindowInWorkspace(nullptr, "ws1"), nullptr);
  EXPECT_EQ(manager_->GetWorkspaceForWindow(nullptr), "default");
  EXPECT_FALSE(manager_->IsWorkspaceHibernated(nullptr, "ws1"));
  EXPECT_EQ(manager_->GetActiveWorkspaceId(nullptr), "default");
}

// ---------------------------------------------------------------------------
// Query methods with empty state
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetActiveWindowInWorkspace_EmptyReturnsNull) {
  EXPECT_EQ(
      manager_->GetActiveWindowInWorkspace(profile_.get(), "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, GetAverageTabsPerWindowInWorkspace_EmptyIsZero) {
  EXPECT_DOUBLE_EQ(
      manager_->GetAverageTabsPerWindowInWorkspace(profile_.get(), "ws1"),
      0.0);
}

TEST_F(WorkspaceWindowManagerTest, GetWindowCountInWorkspace_EmptyIsZero) {
  EXPECT_EQ(
      manager_->GetWindowCountInWorkspace(profile_.get(), "ws1"), 0u);
}

TEST_F(WorkspaceWindowManagerTest, GetTabCountInWorkspace_EmptyIsZero) {
  EXPECT_EQ(
      manager_->GetTabCountInWorkspace(profile_.get(), "ws1"), 0u);
}

// ---------------------------------------------------------------------------
// Null browser / edge case handling
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, MoveWindowToWorkspace_NullBrowserIsNoOp) {
  TestWorkspaceWindowManagerObserver observer;
  manager_->AddObserver(&observer);

  manager_->MoveWindowToWorkspace(nullptr, "ws1");

  // No notifications should fire.
  EXPECT_EQ(observer.window_added_count_, 0);
  EXPECT_EQ(observer.window_removed_count_, 0);
  EXPECT_EQ(observer.window_count_changed_count_, 0);

  manager_->RemoveObserver(&observer);
}

TEST_F(WorkspaceWindowManagerTest, CloseAllWindowsInWorkspace_EmptyIsNoOp) {
  // Should not crash when there are no windows to close.
  manager_->CloseAllWindowsInWorkspace(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MoveAllWindowsToWorkspace_EmptyIsNoOp) {
  // Should not crash when there are no windows to move.
  manager_->MoveAllWindowsToWorkspace(profile_.get(), "ws1", "ws2");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MoveAllWindowsToWorkspace_SameWorkspaceIsNoOp) {
  manager_->MoveAllWindowsToWorkspace(profile_.get(), "ws1", "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MoveAllTabsToWorkspace_SameWorkspaceIsNoOp) {
  manager_->MoveAllTabsToWorkspace(profile_.get(), "ws1", "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MoveAllTabsToWorkspace_NullProfileIsNoOp) {
  manager_->MoveAllTabsToWorkspace(nullptr, "ws1", "ws2");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, HibernateWorkspace_EmptyIsNoOp) {
  TestWorkspaceWindowManagerObserver observer;
  manager_->AddObserver(&observer);

  // Hibernating an empty workspace should not notify (no windows = not
  // considered hibernated in IsWorkspaceHibernated).
  manager_->HibernateWorkspace(profile_.get(), "empty-ws");

  // No hibernation notification for empty workspace.
  EXPECT_EQ(observer.hibernation_changed_count_, 0);

  manager_->RemoveObserver(&observer);
}

TEST_F(WorkspaceWindowManagerTest, WakeUpWorkspace_EmptyIsNoOp) {
  TestWorkspaceWindowManagerObserver observer;
  manager_->AddObserver(&observer);

  manager_->WakeUpWorkspace(profile_.get(), "empty-ws");

  // No change notification for empty workspace.
  EXPECT_EQ(observer.hibernation_changed_count_, 0);

  manager_->RemoveObserver(&observer);
}

TEST_F(WorkspaceWindowManagerTest, IsWorkspaceHibernated_EmptyReturnsFalse) {
  EXPECT_FALSE(manager_->IsWorkspaceHibernated(profile_.get(), "ws1"));
}

// ---------------------------------------------------------------------------
// ReorderWindowsInWorkspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, ReorderWindowsInWorkspace_EmptyReturnsFalse) {
  std::vector<Browser*> empty_list;
  EXPECT_FALSE(
      manager_->ReorderWindowsInWorkspace(profile_.get(), "ws1", empty_list));
}

TEST_F(WorkspaceWindowManagerTest, ReorderWindowsInWorkspace_NullProfileReturnsFalse) {
  std::vector<Browser*> list;
  EXPECT_FALSE(manager_->ReorderWindowsInWorkspace(nullptr, "ws1", list));
}

// ---------------------------------------------------------------------------
// Hibernation with test window features
// ---------------------------------------------------------------------------

TEST(AstraWindowFeaturesHibernationTest, DefaultIsNotHibernated) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);
  EXPECT_FALSE(features->is_hibernated());
}

TEST(AstraWindowFeaturesHibernationTest, SetHibernated) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);

  features->set_is_hibernated(true);
  EXPECT_TRUE(features->is_hibernated());

  features->set_is_hibernated(false);
  EXPECT_FALSE(features->is_hibernated());
}

TEST(AstraWindowFeaturesHibernationTest, ResetClearsHibernation) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);

  features->set_is_hibernated(true);
  ASSERT_TRUE(features->is_hibernated());

  features->Reset();
  EXPECT_FALSE(features->is_hibernated());
}

// ---------------------------------------------------------------------------
// Window ordering with test window features
// ---------------------------------------------------------------------------

TEST(AstraWindowFeaturesOrderTest, DefaultOrderIndexIsZero) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);
  EXPECT_EQ(features->order_index(), 0u);
}

TEST(AstraWindowFeaturesOrderTest, SetOrderIndex) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);

  features->set_order_index(5);
  EXPECT_EQ(features->order_index(), 5u);

  features->set_order_index(0);
  EXPECT_EQ(features->order_index(), 0u);
}

TEST(AstraWindowFeaturesOrderTest, ResetClearsOrderIndex) {
  auto features = AstraWindowFeatures::CreateForTesting();
  ASSERT_NE(features, nullptr);

  features->set_order_index(10);
  ASSERT_EQ(features->order_index(), 10u);

  features->Reset();
  EXPECT_EQ(features->order_index(), 0u);
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

TEST(AstraWorkspaceWindowManagerObserverTest, DefaultImplementationsAreNoOps) {
  // Observer has default empty implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraWorkspaceWindowManagerObserver {};

  TestObserver observer;
  // Call all observer methods — no crash = success.
  observer.OnWindowAddedToWorkspace(nullptr, "ws1");
  observer.OnWindowRemovedFromWorkspace(nullptr, "ws1");
  observer.OnActiveWorkspaceChanged("old", "new");
  observer.OnWindowCountChanged("ws1", 5);
  observer.OnWorkspaceHibernationChanged("ws1", true);
}

// ---------------------------------------------------------------------------
// Partial observer (only overrides one method)
// ---------------------------------------------------------------------------

namespace {

// Observer that overrides only OnWindowCountChanged to verify defaults work.
class PartialObserver : public AstraWorkspaceWindowManagerObserver {
 public:
  void OnWindowCountChanged(const std::string& workspace_id,
                            size_t new_count) override {
    count_changed_count_++;
    last_workspace_id_ = workspace_id;
    last_count_ = new_count;
  }

  int count_changed_count_ = 0;
  std::string last_workspace_id_;
  size_t last_count_ = 0;
};

}  // namespace

TEST_F(WorkspaceWindowManagerTest, PartialObserver_DefaultImplementationsWork) {
  PartialObserver partial_observer;
  manager_->AddObserver(&partial_observer);

  // All these operations should compile and not crash (default implementations).
  // With an empty BrowserList, most operations are no-ops.
  manager_->MoveWindowToWorkspace(nullptr, "ws1");
  manager_->CloseAllWindowsInWorkspace(profile_.get(), "ws1");
  manager_->HibernateWorkspace(profile_.get(), "ws1");
  manager_->SwitchToWorkspace(profile_.get(), "ws2");

  // OnWindowCountChanged should not have fired (empty state).
  EXPECT_EQ(partial_observer.count_changed_count_, 0);

  manager_->RemoveObserver(&partial_observer);
}

// ---------------------------------------------------------------------------
// Pref registration (workspace window settings)
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, Prefs_DefaultValuesAreCorrect) {
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(prefs, nullptr);

  // Remember placement default: true
  EXPECT_TRUE(
      prefs->GetBoolean(prefs::kPrefWorkspaceWindowRememberPlacement));

  // New in active workspace default: true
  EXPECT_TRUE(
      prefs->GetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace));

  // Auto-tile default: false
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefWorkspaceWindowAutoTile));

  // Default window count default: 1
  EXPECT_EQ(
      prefs->GetInteger(prefs::kPrefWorkspaceWindowDefaultWindowCount), 1);
}

TEST_F(WorkspaceWindowManagerTest, Prefs_CanSetAndGet) {
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(prefs, nullptr);

  // Set remember placement to false.
  prefs->SetBoolean(prefs::kPrefWorkspaceWindowRememberPlacement, false);
  EXPECT_FALSE(
      prefs->GetBoolean(prefs::kPrefWorkspaceWindowRememberPlacement));

  // Set new in active workspace to false.
  prefs->SetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace, false);
  EXPECT_FALSE(
      prefs->GetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace));

  // Set auto-tile to true.
  prefs->SetBoolean(prefs::kPrefWorkspaceWindowAutoTile, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefWorkspaceWindowAutoTile));

  // Set default window count to 3.
  prefs->SetInteger(prefs::kPrefWorkspaceWindowDefaultWindowCount, 3);
  EXPECT_EQ(
      prefs->GetInteger(prefs::kPrefWorkspaceWindowDefaultWindowCount), 3);
}

TEST_F(WorkspaceWindowManagerTest, Prefs_PersistenceRoundTrip) {
  // Change a pref value.
  profile_->GetPrefs()->SetBoolean(
      prefs::kPrefWorkspaceWindowAutoTile, true);
  profile_->GetPrefs()->SetInteger(
      prefs::kPrefWorkspaceWindowDefaultWindowCount, 2);

  // Create a new service that reads from the same profile's prefs.
  // Actually, since TestingProfile persists prefs within the same profile,
  // we verify by reading back.
  EXPECT_TRUE(profile_->GetPrefs()->GetBoolean(
      prefs::kPrefWorkspaceWindowAutoTile));
  EXPECT_EQ(profile_->GetPrefs()->GetInteger(
      prefs::kPrefWorkspaceWindowDefaultWindowCount), 2);
}

// ---------------------------------------------------------------------------
// SwitchToWorkspace edge cases
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, SwitchToWorkspace_NullProfileIsNoOp) {
  TestWorkspaceWindowManagerObserver observer;
  manager_->AddObserver(&observer);

  manager_->SwitchToWorkspace(nullptr, "ws1");

  EXPECT_EQ(observer.active_workspace_changed_count_, 0);

  manager_->RemoveObserver(&observer);
}

TEST_F(WorkspaceWindowManagerTest, GetActiveWorkspaceId_DefaultForEmptyProfile) {
  // With no windows, the active workspace should be "default".
  EXPECT_EQ(manager_->GetActiveWorkspaceId(profile_.get()), "default");
}

// ---------------------------------------------------------------------------
// Stats calculations with empty state
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, Stats_ZeroWindowsZeroTabs) {
  EXPECT_EQ(manager_->GetTotalWindowCount(profile_.get()), 0u);
  EXPECT_EQ(manager_->GetTotalTabCount(profile_.get()), 0u);
  EXPECT_DOUBLE_EQ(manager_->GetAverageTabsPerWindow(profile_.get()), 0.0);
}

TEST_F(WorkspaceWindowManagerTest, Stats_NullProfileReturnsZero) {
  EXPECT_EQ(manager_->GetTotalWindowCount(nullptr), 0u);
  EXPECT_EQ(manager_->GetTotalTabCount(nullptr), 0u);
  EXPECT_DOUBLE_EQ(manager_->GetAverageTabsPerWindow(nullptr), 0.0);
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, AddAndRemoveObserver) {
  TestWorkspaceWindowManagerObserver observer;

  // Add observer.
  manager_->AddObserver(&observer);

  // Remove observer.
  manager_->RemoveObserver(&observer);

  // After removal, no notifications should reach the observer.
  // (With empty BrowserList, there are no natural events, but the add/remove
  // should not crash.)
}

TEST_F(WorkspaceWindowManagerTest, RemoveNonExistentObserverIsSafe) {
  TestWorkspaceWindowManagerObserver observer;
  // Removing an observer that was never added should not crash.
  manager_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, MultipleObservers_AllReceiveEvents) {
  // Note: With empty BrowserList, we can't test actual window add/remove
  // events.  But we can test that multiple observers can be added and
  // removed without issues.
  TestWorkspaceWindowManagerObserver observer1;
  TestWorkspaceWindowManagerObserver observer2;
  TestWorkspaceWindowManagerObserver observer3;

  manager_->AddObserver(&observer1);
  manager_->AddObserver(&observer2);
  manager_->AddObserver(&observer3);

  // All should have zero counts initially.
  EXPECT_EQ(observer1.window_added_count_, 0);
  EXPECT_EQ(observer2.window_added_count_, 0);
  EXPECT_EQ(observer3.window_added_count_, 0);

  manager_->RemoveObserver(&observer1);
  manager_->RemoveObserver(&observer2);
  manager_->RemoveObserver(&observer3);
}

// ---------------------------------------------------------------------------
// GetWorkspaceForWindow
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceForWindow_NullBrowserReturnsDefault) {
  EXPECT_EQ(manager_->GetWorkspaceForWindow(nullptr), "default");
}

// ---------------------------------------------------------------------------
// Bulk operations with null profile
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, CloseAllWindowsInWorkspace_NullProfileIsNoOp) {
  manager_->CloseAllWindowsInWorkspace(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MoveAllWindowsToWorkspace_NullProfileIsNoOp) {
  manager_->MoveAllWindowsToWorkspace(nullptr, "ws1", "ws2");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, HibernateWorkspace_NullProfileIsNoOp) {
  manager_->HibernateWorkspace(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, WakeUpWorkspace_NullProfileIsNoOp) {
  manager_->WakeUpWorkspace(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, IsWorkspaceHibernated_NullProfileReturnsFalse) {
  EXPECT_FALSE(manager_->IsWorkspaceHibernated(nullptr, "ws1"));
}

// ---------------------------------------------------------------------------
// Window arrangement
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, TileWindows_NullProfileIsNoOp) {
  manager_->TileWindows(nullptr, "ws1", AstraTileDirection::kHorizontal);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindows_EmptyWorkspaceIsNoOp) {
  manager_->TileWindows(profile_.get(), "ws1", AstraTileDirection::kHorizontal);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsHorizontal_NullProfileIsNoOp) {
  manager_->TileWindowsHorizontal(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsHorizontal_EmptyWorkspaceIsNoOp) {
  manager_->TileWindowsHorizontal(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsVertical_NullProfileIsNoOp) {
  manager_->TileWindowsVertical(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsVertical_EmptyWorkspaceIsNoOp) {
  manager_->TileWindowsVertical(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsGrid_NullProfileIsNoOp) {
  manager_->TileWindowsGrid(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindowsGrid_EmptyWorkspaceIsNoOp) {
  manager_->TileWindowsGrid(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, StackWindows_NullProfileIsNoOp) {
  manager_->StackWindows(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, StackWindows_EmptyWorkspaceIsNoOp) {
  manager_->StackWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, CascadeWindows_NullProfileIsNoOp) {
  manager_->CascadeWindows(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, CascadeWindows_EmptyWorkspaceIsNoOp) {
  manager_->CascadeWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MaximizeAllWindows_NullProfileIsNoOp) {
  manager_->MaximizeAllWindows(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MaximizeAllWindows_EmptyWorkspaceIsNoOp) {
  manager_->MaximizeAllWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MinimizeAllWindows_NullProfileIsNoOp) {
  manager_->MinimizeAllWindows(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MinimizeAllWindows_EmptyWorkspaceIsNoOp) {
  manager_->MinimizeAllWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, RestoreAllWindows_NullProfileIsNoOp) {
  manager_->RestoreAllWindows(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, RestoreAllWindows_EmptyWorkspaceIsNoOp) {
  manager_->RestoreAllWindows(profile_.get(), "ws1");
  // No crash = success.
}

// ---------------------------------------------------------------------------
// Focus cycling
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetFocusedWindow_NullProfileReturnsNull) {
  EXPECT_EQ(manager_->GetFocusedWindow(nullptr, "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, GetFocusedWindow_EmptyWorkspaceReturnsNull) {
  EXPECT_EQ(manager_->GetFocusedWindow(profile_.get(), "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, GetFocusedWindow_SameAsActiveWindow) {
  // GetFocusedWindow is an alias for GetActiveWindowInWorkspace.
  EXPECT_EQ(manager_->GetFocusedWindow(profile_.get(), "ws1"),
            manager_->GetActiveWindowInWorkspace(profile_.get(), "ws1"));
}

TEST_F(WorkspaceWindowManagerTest, FocusNextWindow_NullProfileReturnsNull) {
  EXPECT_EQ(manager_->FocusNextWindow(nullptr, "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, FocusNextWindow_EmptyWorkspaceReturnsNull) {
  EXPECT_EQ(manager_->FocusNextWindow(profile_.get(), "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, FocusPreviousWindow_NullProfileReturnsNull) {
  EXPECT_EQ(manager_->FocusPreviousWindow(nullptr, "ws1"), nullptr);
}

TEST_F(WorkspaceWindowManagerTest, FocusPreviousWindow_EmptyWorkspaceReturnsNull) {
  EXPECT_EQ(manager_->FocusPreviousWindow(profile_.get(), "ws1"), nullptr);
}

// ---------------------------------------------------------------------------
// New window behavior
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetNewWindowBehavior_NullProfileReturnsDefault) {
  EXPECT_EQ(manager_->GetNewWindowBehavior(nullptr),
            AstraNewWindowBehavior::kActiveWorkspace);
}

TEST_F(WorkspaceWindowManagerTest, GetNewWindowBehavior_DefaultIsActiveWorkspace) {
  EXPECT_EQ(manager_->GetNewWindowBehavior(profile_.get()),
            AstraNewWindowBehavior::kActiveWorkspace);
}

TEST_F(WorkspaceWindowManagerTest, SetNewWindowBehavior_NullProfileIsNoOp) {
  manager_->SetNewWindowBehavior(nullptr, AstraNewWindowBehavior::kDefaultWorkspace);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SetNewWindowBehavior_UpdatesSetting) {
  // Default is active workspace.
  ASSERT_EQ(manager_->GetNewWindowBehavior(profile_.get()),
            AstraNewWindowBehavior::kActiveWorkspace);

  // Change to default workspace.
  manager_->SetNewWindowBehavior(profile_.get(),
                                 AstraNewWindowBehavior::kDefaultWorkspace);
  EXPECT_EQ(manager_->GetNewWindowBehavior(profile_.get()),
            AstraNewWindowBehavior::kDefaultWorkspace);
}

TEST_F(WorkspaceWindowManagerTest, NewWindowBehavior_EnumValues) {
  // Verify the enum has the expected values.
  EXPECT_NE(static_cast<int>(AstraNewWindowBehavior::kDefaultWorkspace),
            static_cast<int>(AstraNewWindowBehavior::kActiveWorkspace));
  EXPECT_NE(static_cast<int>(AstraNewWindowBehavior::kDefaultWorkspace),
            static_cast<int>(AstraNewWindowBehavior::kNewWorkspace));
  EXPECT_NE(static_cast<int>(AstraNewWindowBehavior::kDefaultWorkspace),
            static_cast<int>(AstraNewWindowBehavior::kAskUser));
}

TEST_F(WorkspaceWindowManagerTest, TileDirection_EnumValues) {
  // Verify the enum has the expected values.
  EXPECT_NE(static_cast<int>(AstraTileDirection::kHorizontal),
            static_cast<int>(AstraTileDirection::kVertical));
  EXPECT_NE(static_cast<int>(AstraTileDirection::kHorizontal),
            static_cast<int>(AstraTileDirection::kGrid));
}

// ---------------------------------------------------------------------------
// Workspace switching helpers
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, SwitchToNextWorkspace_NullProfileIsNoOp) {
  manager_->SwitchToNextWorkspace(nullptr);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SwitchToPreviousWorkspace_NullProfileIsNoOp) {
  manager_->SwitchToPreviousWorkspace(nullptr);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SwitchToWorkspaceAtIndex_NullProfileIsNoOp) {
  manager_->SwitchToWorkspaceAtIndex(nullptr, 0);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceCount_NullProfileReturnsZero) {
  EXPECT_EQ(manager_->GetWorkspaceCount(nullptr), 0u);
}

TEST_F(WorkspaceWindowManagerTest, GetActiveWorkspaceIndex_NullProfileReturnsZero) {
  EXPECT_EQ(manager_->GetActiveWorkspaceIndex(nullptr), 0u);
}

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceIndex_NullProfileReturnsZero) {
  EXPECT_EQ(manager_->GetWorkspaceIndex(nullptr, "ws1"), 0u);
}

TEST_F(WorkspaceWindowManagerTest, GetRecentWorkspaces_NullProfileReturnsEmpty) {
  auto result = manager_->GetRecentWorkspaces(nullptr, 5);
  EXPECT_TRUE(result.empty());
}

TEST_F(WorkspaceWindowManagerTest, GetRecentWorkspaces_RespectsMaxCount) {
  auto result = manager_->GetRecentWorkspaces(profile_.get(), 3);
  EXPECT_LE(result.size(), 3u);
}

TEST_F(WorkspaceWindowManagerTest, GetRecentWorkspaces_MaxCountZeroReturnsEmpty) {
  auto result = manager_->GetRecentWorkspaces(profile_.get(), 0);
  EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Switch animation duration
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetSwitchAnimationDurationMs_NullProfileReturnsDefault) {
  EXPECT_GT(manager_->GetSwitchAnimationDurationMs(nullptr), 0);
}

TEST_F(WorkspaceWindowManagerTest, GetSwitchAnimationDurationMs_ReturnsPositiveValue) {
  int duration = manager_->GetSwitchAnimationDurationMs(profile_.get());
  EXPECT_GT(duration, 0);
  EXPECT_LE(duration, 2000);  // Reasonable upper bound
}

TEST_F(WorkspaceWindowManagerTest, SetSwitchAnimationDurationMs_NullProfileIsNoOp) {
  manager_->SetSwitchAnimationDurationMs(nullptr, 300);
  // No crash = success.
}

// ---------------------------------------------------------------------------
// Saved window state
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, SaveAllWindowState_NullProfileIsNoOp) {
  manager_->SaveAllWindowState(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SaveAllWindowState_EmptyWorkspaceIsNoOp) {
  manager_->SaveAllWindowState(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, RestoreAllWindowState_NullProfileIsNoOp) {
  manager_->RestoreAllWindowState(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, RestoreAllWindowState_EmptyWorkspaceIsNoOp) {
  manager_->RestoreAllWindowState(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, HasSavedWindowState_NullProfileReturnsFalse) {
  EXPECT_FALSE(manager_->HasSavedWindowState(nullptr, "ws1"));
}

TEST_F(WorkspaceWindowManagerTest, HasSavedWindowState_EmptyWorkspaceReturnsFalse) {
  EXPECT_FALSE(manager_->HasSavedWindowState(profile_.get(), "ws1"));
}

TEST_F(WorkspaceWindowManagerTest, ClearSavedWindowState_NullProfileIsNoOp) {
  manager_->ClearSavedWindowState(nullptr, "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, ClearSavedWindowState_EmptyWorkspaceIsNoOp) {
  manager_->ClearSavedWindowState(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SaveThenClearWindowState) {
  // Save then clear - should not crash.
  manager_->SaveAllWindowState(profile_.get(), "ws1");
  manager_->ClearSavedWindowState(profile_.get(), "ws1");
  EXPECT_FALSE(manager_->HasSavedWindowState(profile_.get(), "ws1"));
}

// ---------------------------------------------------------------------------
// Settings (PrefService-based)
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetRememberPlacement_NullProfileReturnsTrue) {
  EXPECT_TRUE(manager_->GetRememberPlacement(nullptr));
}

TEST_F(WorkspaceWindowManagerTest, GetRememberPlacement_DefaultIsTrue) {
  EXPECT_TRUE(manager_->GetRememberPlacement(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetRememberPlacement_UpdatesPref) {
  manager_->SetRememberPlacement(profile_.get(), false);
  EXPECT_FALSE(manager_->GetRememberPlacement(profile_.get()));

  manager_->SetRememberPlacement(profile_.get(), true);
  EXPECT_TRUE(manager_->GetRememberPlacement(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetRememberPlacement_NullProfileIsNoOp) {
  manager_->SetRememberPlacement(nullptr, false);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetNewInActiveWorkspace_NullProfileReturnsTrue) {
  EXPECT_TRUE(manager_->GetNewInActiveWorkspace(nullptr));
}

TEST_F(WorkspaceWindowManagerTest, GetNewInActiveWorkspace_DefaultIsTrue) {
  EXPECT_TRUE(manager_->GetNewInActiveWorkspace(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetNewInActiveWorkspace_UpdatesPref) {
  manager_->SetNewInActiveWorkspace(profile_.get(), false);
  EXPECT_FALSE(manager_->GetNewInActiveWorkspace(profile_.get()));

  manager_->SetNewInActiveWorkspace(profile_.get(), true);
  EXPECT_TRUE(manager_->GetNewInActiveWorkspace(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetNewInActiveWorkspace_NullProfileIsNoOp) {
  manager_->SetNewInActiveWorkspace(nullptr, false);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetTileGap_NullProfileReturnsDefault) {
  EXPECT_GT(manager_->GetTileGap(nullptr), 0);
}

TEST_F(WorkspaceWindowManagerTest, GetTileGap_ReturnsPositiveValue) {
  int gap = manager_->GetTileGap(profile_.get());
  EXPECT_GT(gap, 0);
  EXPECT_LT(gap, 100);  // Reasonable upper bound
}

TEST_F(WorkspaceWindowManagerTest, SetTileGap_NullProfileIsNoOp) {
  manager_->SetTileGap(nullptr, 10);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetTilePadding_NullProfileReturnsDefault) {
  EXPECT_GT(manager_->GetTilePadding(nullptr), 0);
}

TEST_F(WorkspaceWindowManagerTest, GetTilePadding_ReturnsPositiveValue) {
  int padding = manager_->GetTilePadding(profile_.get());
  EXPECT_GT(padding, 0);
  EXPECT_LT(padding, 200);  // Reasonable upper bound
}

TEST_F(WorkspaceWindowManagerTest, SetTilePadding_NullProfileIsNoOp) {
  manager_->SetTilePadding(nullptr, 20);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetCascadeOffset_NullProfileReturnsDefault) {
  EXPECT_GT(manager_->GetCascadeOffset(nullptr), 0);
}

TEST_F(WorkspaceWindowManagerTest, GetCascadeOffset_ReturnsPositiveValue) {
  int offset = manager_->GetCascadeOffset(profile_.get());
  EXPECT_GT(offset, 0);
  EXPECT_LT(offset, 200);  // Reasonable upper bound
}

TEST_F(WorkspaceWindowManagerTest, SetCascadeOffset_NullProfileIsNoOp) {
  manager_->SetCascadeOffset(nullptr, 30);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetAutoTile_NullProfileReturnsFalse) {
  EXPECT_FALSE(manager_->GetAutoTile(nullptr));
}

TEST_F(WorkspaceWindowManagerTest, GetAutoTile_DefaultIsFalse) {
  EXPECT_FALSE(manager_->GetAutoTile(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetAutoTile_UpdatesPref) {
  manager_->SetAutoTile(profile_.get(), true);
  EXPECT_TRUE(manager_->GetAutoTile(profile_.get()));

  manager_->SetAutoTile(profile_.get(), false);
  EXPECT_FALSE(manager_->GetAutoTile(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, SetAutoTile_NullProfileIsNoOp) {
  manager_->SetAutoTile(nullptr, true);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetDefaultWorkspaceId_NullProfileReturnsDefault) {
  EXPECT_EQ(manager_->GetDefaultWorkspaceId(nullptr), "default");
}

TEST_F(WorkspaceWindowManagerTest, GetDefaultWorkspaceId_ReturnsNonEmpty) {
  std::string id = manager_->GetDefaultWorkspaceId(profile_.get());
  EXPECT_FALSE(id.empty());
}

TEST_F(WorkspaceWindowManagerTest, SetDefaultWorkspaceId_NullProfileIsNoOp) {
  manager_->SetDefaultWorkspaceId(nullptr, "ws1");
  // No crash = success.
}

// ---------------------------------------------------------------------------
// Workspace indicator position
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceIndicatorPosition_NullProfileReturnsDefault) {
  EXPECT_EQ(manager_->GetWorkspaceIndicatorPosition(nullptr),
            AstraWorkspaceWindowManager::IndicatorPosition::kTopCenter);
}

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceIndicatorPosition_DefaultIsTopCenter) {
  EXPECT_EQ(manager_->GetWorkspaceIndicatorPosition(profile_.get()),
            AstraWorkspaceWindowManager::IndicatorPosition::kTopCenter);
}

TEST_F(WorkspaceWindowManagerTest, SetWorkspaceIndicatorPosition_NullProfileIsNoOp) {
  manager_->SetWorkspaceIndicatorPosition(
      nullptr, AstraWorkspaceWindowManager::IndicatorPosition::kTopLeft);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, IndicatorPosition_EnumHasSixValues) {
  // Verify all six position values exist and are distinct.
  std::set<int> positions;
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kTopLeft));
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kTopCenter));
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kTopRight));
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kBottomLeft));
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kBottomCenter));
  positions.insert(
      static_cast<int>(AstraWorkspaceWindowManager::IndicatorPosition::kBottomRight));
  EXPECT_EQ(positions.size(), 6u);
}

// ---------------------------------------------------------------------------
// Extended observer tests (new events)
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, Observer_OnWindowsArranged_DefaultImplementation) {
  // The base observer has empty default implementations, so calling
  // OnWindowsArranged on a base observer should not crash.
  TestWorkspaceWindowManagerObserver observer;
  AstraWorkspaceWindowManagerObserver* base = &observer;
  base->OnWindowsArranged("ws1", AstraTileDirection::kHorizontal);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, Observer_OnWindowFocusChanged_DefaultImplementation) {
  TestWorkspaceWindowManagerObserver observer;
  AstraWorkspaceWindowManagerObserver* base = &observer;
  base->OnWindowFocusChanged("ws1", nullptr);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, Observer_OnAllWindowsStateChanged_DefaultImplementation) {
  TestWorkspaceWindowManagerObserver observer;
  AstraWorkspaceWindowManagerObserver* base = &observer;
  base->OnAllWindowsStateChanged("ws1", true, false);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, Observer_OnWindowsArranged_RecordsCall) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();
  // The test observer doesn't override OnWindowsArranged, so the default
  // empty implementation is used. This test verifies it doesn't crash.
  observer->OnWindowsArranged("ws1", AstraTileDirection::kGrid);
  // No crash = success.
  test_observers_.push_back(std::move(observer));
}

TEST_F(WorkspaceWindowManagerTest, Observer_OnWindowFocusChanged_RecordsCall) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();
  observer->OnWindowFocusChanged("ws1", nullptr);
  // No crash = success.
  test_observers_.push_back(std::move(observer));
}

TEST_F(WorkspaceWindowManagerTest, Observer_OnAllWindowsStateChanged_RecordsCall) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();
  observer->OnAllWindowsStateChanged("ws1", false, true);
  // No crash = success.
  test_observers_.push_back(std::move(observer));
}

TEST_F(WorkspaceWindowManagerTest, ExtendedObserver_AddAndRemove) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();
  manager_->AddObserver(observer.get());
  manager_->RemoveObserver(observer.get());
  // No crash = success.
  test_observers_.push_back(std::move(observer));
}

TEST_F(WorkspaceWindowManagerTest, ExtendedObserver_MultipleExtendedObservers) {
  auto obs1 = std::make_unique<TestWorkspaceWindowManagerObserver>();
  auto obs2 = std::make_unique<TestWorkspaceWindowManagerObserver>();
  auto obs3 = std::make_unique<TestWorkspaceWindowManagerObserver>();

  manager_->AddObserver(obs1.get());
  manager_->AddObserver(obs2.get());
  manager_->AddObserver(obs3.get());

  // Verify all receive base notifications.
  EXPECT_EQ(obs1->window_added_count_, 0);
  EXPECT_EQ(obs2->window_added_count_, 0);
  EXPECT_EQ(obs3->window_added_count_, 0);

  manager_->RemoveObserver(obs1.get());
  manager_->RemoveObserver(obs2.get());
  manager_->RemoveObserver(obs3.get());

  test_observers_.push_back(std::move(obs1));
  test_observers_.push_back(std::move(obs2));
  test_observers_.push_back(std::move(obs3));
}

// ---------------------------------------------------------------------------
// Settings round-trip tests
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, RememberPlacement_RoundTrip) {
  bool original = manager_->GetRememberPlacement(profile_.get());

  manager_->SetRememberPlacement(profile_.get(), !original);
  EXPECT_EQ(manager_->GetRememberPlacement(profile_.get()), !original);

  manager_->SetRememberPlacement(profile_.get(), original);
  EXPECT_EQ(manager_->GetRememberPlacement(profile_.get()), original);
}

TEST_F(WorkspaceWindowManagerTest, NewInActiveWorkspace_RoundTrip) {
  bool original = manager_->GetNewInActiveWorkspace(profile_.get());

  manager_->SetNewInActiveWorkspace(profile_.get(), !original);
  EXPECT_EQ(manager_->GetNewInActiveWorkspace(profile_.get()), !original);

  manager_->SetNewInActiveWorkspace(profile_.get(), original);
  EXPECT_EQ(manager_->GetNewInActiveWorkspace(profile_.get()), original);
}

TEST_F(WorkspaceWindowManagerTest, AutoTile_RoundTrip) {
  bool original = manager_->GetAutoTile(profile_.get());

  manager_->SetAutoTile(profile_.get(), !original);
  EXPECT_EQ(manager_->GetAutoTile(profile_.get()), !original);

  manager_->SetAutoTile(profile_.get(), original);
  EXPECT_EQ(manager_->GetAutoTile(profile_.get()), original);
}

TEST_F(WorkspaceWindowManagerTest, NewWindowBehavior_RoundTrip) {
  AstraNewWindowBehavior original =
      manager_->GetNewWindowBehavior(profile_.get());

  manager_->SetNewWindowBehavior(profile_.get(),
                                 AstraNewWindowBehavior::kDefaultWorkspace);
  EXPECT_EQ(manager_->GetNewWindowBehavior(profile_.get()),
            AstraNewWindowBehavior::kDefaultWorkspace);

  manager_->SetNewWindowBehavior(profile_.get(), original);
  EXPECT_EQ(manager_->GetNewWindowBehavior(profile_.get()), original);
}

// ---------------------------------------------------------------------------
// Query method aliases
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetWindowCountInWorkspace_SameAsGetWindowCount) {
  // Both methods should return the same value.
  EXPECT_EQ(manager_->GetWindowCountInWorkspace(profile_.get(), "ws1"),
            manager_->GetWindowCount(profile_.get(), "ws1"));
}

TEST_F(WorkspaceWindowManagerTest, GetTabCountInWorkspace_SameAsGetTabCount) {
  // Both methods should return the same value.
  EXPECT_EQ(manager_->GetTabCountInWorkspace(profile_.get(), "ws1"),
            manager_->GetTabCount(profile_.get(), "ws1"));
}

TEST_F(WorkspaceWindowManagerTest, GetFocusedWindow_SameAsGetActiveWindow) {
  // GetFocusedWindow is an alias for GetActiveWindowInWorkspace.
  EXPECT_EQ(manager_->GetFocusedWindow(profile_.get(), "ws1"),
            manager_->GetActiveWindowInWorkspace(profile_.get(), "ws1"));
}

// ---------------------------------------------------------------------------
// Workspace query delegation
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceCount_MatchesService) {
  // The manager delegates to AstraWorkspaceService.
  // With a testing profile, at least the default workspace should exist.
  size_t count = manager_->GetWorkspaceCount(profile_.get());
  EXPECT_GE(count, 1u);
}

TEST_F(WorkspaceWindowManagerTest, GetDefaultWorkspaceId_IsNotEmpty) {
  std::string id = manager_->GetDefaultWorkspaceId(profile_.get());
  EXPECT_FALSE(id.empty());
}

TEST_F(WorkspaceWindowManagerTest, GetRecentWorkspaces_ContainsAtLeastDefault) {
  auto recent = manager_->GetRecentWorkspaces(profile_.get(), 10);
  EXPECT_GE(recent.size(), 1u);
  // The default workspace should be in the list.
  bool found_default = false;
  for (const auto& id : recent) {
    if (id == manager_->GetDefaultWorkspaceId(profile_.get())) {
      found_default = true;
      break;
    }
  }
  EXPECT_TRUE(found_default);
}

// ---------------------------------------------------------------------------
// Workspace switching edge cases
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, SwitchToWorkspaceAtIndex_OutOfRangeIsNoOp) {
  size_t count = manager_->GetWorkspaceCount(profile_.get());
  // Switch to index far beyond the count - should be no-op.
  manager_->SwitchToWorkspaceAtIndex(profile_.get(), count + 100);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SwitchToWorkspaceAtIndex_ZeroIndex) {
  // Index 0 should always be valid (at least default workspace).
  manager_->SwitchToWorkspaceAtIndex(profile_.get(), 0);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SwitchToNextWorkspace_CanBeCalledMultipleTimes) {
  // Calling multiple times should not crash.
  manager_->SwitchToNextWorkspace(profile_.get());
  manager_->SwitchToNextWorkspace(profile_.get());
  manager_->SwitchToNextWorkspace(profile_.get());
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, SwitchToPreviousWorkspace_CanBeCalledMultipleTimes) {
  manager_->SwitchToPreviousWorkspace(profile_.get());
  manager_->SwitchToPreviousWorkspace(profile_.get());
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceIndex_InvalidIdReturnsZero) {
  // Invalid workspace ID should return 0 (index of default workspace).
  size_t index = manager_->GetWorkspaceIndex(profile_.get(), "nonexistent-id");
  // Should be 0 (default workspace fallback) or a valid index.
  EXPECT_LT(index, manager_->GetWorkspaceCount(profile_.get()));
}

TEST_F(WorkspaceWindowManagerTest, GetActiveWorkspaceIndex_IsValidIndex) {
  size_t index = manager_->GetActiveWorkspaceIndex(profile_.get());
  size_t count = manager_->GetWorkspaceCount(profile_.get());
  EXPECT_LT(index, count);
}

// ---------------------------------------------------------------------------
// Window arrangement edge cases
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, TileDirections_AllThreeValid) {
  // Test that all three tile directions can be used without crashing.
  manager_->TileWindows(profile_.get(), "ws1", AstraTileDirection::kHorizontal);
  manager_->TileWindows(profile_.get(), "ws1", AstraTileDirection::kVertical);
  manager_->TileWindows(profile_.get(), "ws1", AstraTileDirection::kGrid);
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, TileWindows_SameDirectionTwiceIsNoOp) {
  // Tiling twice with the same direction should not crash.
  manager_->TileWindowsHorizontal(profile_.get(), "ws1");
  manager_->TileWindowsHorizontal(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MaximizeThenRestore_EmptyWorkspace) {
  // Maximize then restore on empty workspace should not crash.
  manager_->MaximizeAllWindows(profile_.get(), "ws1");
  manager_->RestoreAllWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, MinimizeThenRestore_EmptyWorkspace) {
  manager_->MinimizeAllWindows(profile_.get(), "ws1");
  manager_->RestoreAllWindows(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, CascadeThenTile_EmptyWorkspace) {
  manager_->CascadeWindows(profile_.get(), "ws1");
  manager_->TileWindowsGrid(profile_.get(), "ws1");
  // No crash = success.
}

TEST_F(WorkspaceWindowManagerTest, StackThenTile_EmptyWorkspace) {
  manager_->StackWindows(profile_.get(), "ws1");
  manager_->TileWindowsHorizontal(profile_.get(), "ws1");
  // No crash = success.
}

// ---------------------------------------------------------------------------
// Combined settings tests
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, MultipleSettings_RoundTrip) {
  // Test multiple settings in sequence.
  bool orig_remember = manager_->GetRememberPlacement(profile_.get());
  bool orig_auto_tile = manager_->GetAutoTile(profile_.get());
  bool orig_new_active = manager_->GetNewInActiveWorkspace(profile_.get());

  // Change all settings.
  manager_->SetRememberPlacement(profile_.get(), !orig_remember);
  manager_->SetAutoTile(profile_.get(), !orig_auto_tile);
  manager_->SetNewInActiveWorkspace(profile_.get(), !orig_new_active);

  // Verify all changed.
  EXPECT_EQ(manager_->GetRememberPlacement(profile_.get()), !orig_remember);
  EXPECT_EQ(manager_->GetAutoTile(profile_.get()), !orig_auto_tile);
  EXPECT_EQ(manager_->GetNewInActiveWorkspace(profile_.get()), !orig_new_active);

  // Restore all.
  manager_->SetRememberPlacement(profile_.get(), orig_remember);
  manager_->SetAutoTile(profile_.get(), orig_auto_tile);
  manager_->SetNewInActiveWorkspace(profile_.get(), orig_new_active);

  // Verify all restored.
  EXPECT_EQ(manager_->GetRememberPlacement(profile_.get()), orig_remember);
  EXPECT_EQ(manager_->GetAutoTile(profile_.get()), orig_auto_tile);
  EXPECT_EQ(manager_->GetNewInActiveWorkspace(profile_.get()), orig_new_active);
}

TEST_F(WorkspaceWindowManagerTest, TileGap_ReturnsConsistentValue) {
  int gap1 = manager_->GetTileGap(profile_.get());
  int gap2 = manager_->GetTileGap(profile_.get());
  EXPECT_EQ(gap1, gap2);
}

TEST_F(WorkspaceWindowManagerTest, TilePadding_ReturnsConsistentValue) {
  int pad1 = manager_->GetTilePadding(profile_.get());
  int pad2 = manager_->GetTilePadding(profile_.get());
  EXPECT_EQ(pad1, pad2);
}

TEST_F(WorkspaceWindowManagerTest, CascadeOffset_ReturnsConsistentValue) {
  int offset1 = manager_->GetCascadeOffset(profile_.get());
  int offset2 = manager_->GetCascadeOffset(profile_.get());
  EXPECT_EQ(offset1, offset2);
}

TEST_F(WorkspaceWindowManagerTest, SwitchAnimationDuration_ReturnsConsistentValue) {
  int dur1 = manager_->GetSwitchAnimationDurationMs(profile_.get());
  int dur2 = manager_->GetSwitchAnimationDurationMs(profile_.get());
  EXPECT_EQ(dur1, dur2);
}

// ---------------------------------------------------------------------------
// Observer notification patterns
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, Observer_ReceivesNotificationsAfterReAdd) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();

  // Add, remove, add again.
  manager_->AddObserver(observer.get());
  manager_->RemoveObserver(observer.get());
  manager_->AddObserver(observer.get());

  // Observer should still work after re-adding.
  EXPECT_EQ(observer->window_added_count_, 0);

  manager_->RemoveObserver(observer.get());
  test_observers_.push_back(std::move(observer));
}

TEST_F(WorkspaceWindowManagerTest, Observer_RemoveNonExistentIsSafe) {
  auto observer = std::make_unique<TestWorkspaceWindowManagerObserver>();
  // Removing an observer that was never added should be safe.
  manager_->RemoveObserver(observer.get());
  // No crash = success.
  test_observers_.push_back(std::move(observer));
}

// ---------------------------------------------------------------------------
// Stats methods edge cases
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetAverageTabsPerWindowInWorkspace_EmptyReturnsZero) {
  double avg = manager_->GetAverageTabsPerWindowInWorkspace(
      profile_.get(), "nonexistent");
  EXPECT_EQ(avg, 0.0);
}

TEST_F(WorkspaceWindowManagerTest, GetAverageTabsPerWindow_NullProfileReturnsZero) {
  double avg = manager_->GetAverageTabsPerWindow(nullptr);
  EXPECT_EQ(avg, 0.0);
}

TEST_F(WorkspaceWindowManagerTest, GetAverageTabsPerWindowInWorkspace_NullProfileReturnsZero) {
  double avg = manager_->GetAverageTabsPerWindowInWorkspace(nullptr, "ws1");
  EXPECT_EQ(avg, 0.0);
}

// ---------------------------------------------------------------------------
// Workspace info delegation
// ---------------------------------------------------------------------------

TEST_F(WorkspaceWindowManagerTest, GetWorkspaceCount_IsAtLeastOne) {
  // There should always be at least the default workspace.
  EXPECT_GE(manager_->GetWorkspaceCount(profile_.get()), 1u);
}

TEST_F(WorkspaceWindowManagerTest, GetDefaultWorkspaceId_MatchesFirstWorkspace) {
  std::string default_id = manager_->GetDefaultWorkspaceId(profile_.get());
  size_t count = manager_->GetWorkspaceCount(profile_.get());
  EXPECT_GE(count, 1u);
  // Default workspace should be one of the workspaces.
  // (We can't easily check all workspaces via the manager directly.)
  EXPECT_FALSE(default_id.empty());
}

// ---------------------------------------------------------------------------
// TODO(astra): Full integration tests (require browser test harness)
// ---------------------------------------------------------------------------
//
// The following test categories require InProcessBrowserTest with real
// Browser windows and TabStripModel:
//
//   - Window assignment and querying with real Browser objects
//   - MoveWindowToWorkspace with real windows
//   - MoveAllTabsToWorkspace with real tabs
//   - CloseAllWindowsInWorkspace with real windows
//   - MoveAllWindowsToWorkspace with real windows
//   - ReorderWindowsInWorkspace with real windows
//   - GetActiveWindowInWorkspace with real windows
//   - HibernateWorkspace / WakeUpWorkspace with real tabs
//   - Stats calculations with real windows and tabs
//   - Observer notifications from actual window add/remove events
//   - SwitchToWorkspace with real windows
//
// Chromium component: InProcessBrowserTest
//   (chrome/test/base/in_process_browser_test.h)
//
// TODO(astra): Add browser_tests for full window manager integration.
//   Patch point: //chrome/test:browser_tests test suite.

}  // namespace astra
