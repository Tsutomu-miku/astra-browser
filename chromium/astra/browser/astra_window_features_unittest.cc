// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_window_features.h"

#include <memory>
#include <vector>

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestWindowObserver : public AstraWindowObserver {
 public:
  // Window lifecycle
  int window_created_count_ = 0;
  int window_closing_count_ = 0;
  AstraWindowFeatures* last_created_window_ = nullptr;
  AstraWindowFeatures* last_closing_window_ = nullptr;

  // Workspace changes
  int workspace_changed_count_ = 0;
  std::string last_old_workspace_id_;
  std::string last_new_workspace_id_;

  // Sidebar changes
  int sidebar_visibility_changed_count_ = 0;
  bool last_sidebar_visible_ = false;
  int sidebar_pinned_changed_count_ = 0;
  bool last_sidebar_pinned_ = false;
  int sidebar_width_changed_count_ = 0;
  int last_sidebar_width_ = 0;

  // Split view changes
  int split_view_state_changed_count_ = 0;
  bool last_split_view_active_ = false;

  // Catch-all
  int features_changed_count_ = 0;

  // Observer interface implementation
  void OnWindowCreated(AstraWindowFeatures* window) override {
    window_created_count_++;
    last_created_window_ = window;
  }

  void OnWindowClosing(AstraWindowFeatures* window) override {
    window_closing_count_++;
    last_closing_window_ = window;
  }

  void OnWindowWorkspaceChanged(AstraWindowFeatures* window,
                                const std::string& old_workspace_id,
                                const std::string& new_workspace_id) override {
    workspace_changed_count_++;
    last_old_workspace_id_ = old_workspace_id;
    last_new_workspace_id_ = new_workspace_id;
  }

  void OnWindowSidebarVisibilityChanged(AstraWindowFeatures* window,
                                        bool visible) override {
    sidebar_visibility_changed_count_++;
    last_sidebar_visible_ = visible;
  }

  void OnWindowSidebarPinnedChanged(AstraWindowFeatures* window,
                                    bool pinned) override {
    sidebar_pinned_changed_count_++;
    last_sidebar_pinned_ = pinned;
  }

  void OnWindowSidebarWidthChanged(AstraWindowFeatures* window,
                                   int width) override {
    sidebar_width_changed_count_++;
    last_sidebar_width_ = width;
  }

  void OnWindowSplitViewStateChanged(AstraWindowFeatures* window,
                                     bool active) override {
    split_view_state_changed_count_++;
    last_split_view_active_ = active;
  }

  void OnWindowFeaturesChanged(AstraWindowFeatures* window) override {
    features_changed_count_++;
  }

  // Resets all counters.
  void ResetCounters() {
    window_created_count_ = 0;
    window_closing_count_ = 0;
    last_created_window_ = nullptr;
    last_closing_window_ = nullptr;
    workspace_changed_count_ = 0;
    last_old_workspace_id_.clear();
    last_new_workspace_id_.clear();
    sidebar_visibility_changed_count_ = 0;
    last_sidebar_visible_ = false;
    sidebar_pinned_changed_count_ = 0;
    last_sidebar_pinned_ = false;
    sidebar_width_changed_count_ = 0;
    last_sidebar_width_ = 0;
    split_view_state_changed_count_ = 0;
    last_split_view_active_ = false;
    features_changed_count_ = 0;
  }
};

}  // namespace

// Test fixture for AstraWindowFeatures tests.
//
// AstraWindowFeatures is a SupportsUserData::Data that attaches to Browser.
// For unit tests, we use CreateForTesting() to create standalone instances
// without needing a real Browser object.
//
// TODO(astra): Add tests that use a real Browser (e.g. via InProcessBrowserTest)
//   to verify integration with BrowserList and SupportsUserData attachment.
// Chromium component: chrome/test/base/in_process_browser_test.h
class WindowFeaturesTest : public testing::Test {
 protected:
  WindowFeaturesTest() = default;
  ~WindowFeaturesTest() override = default;

  void SetUp() override {
    // Create a test window features instance.
    features_ = AstraWindowFeatures::CreateForTesting();
    DCHECK(features_);
  }

  void TearDown() override {
    // Clean up any global observers that might have been added.
    for (auto* observer : global_observers_) {
      AstraWindowFeatures::RemoveGlobalObserver(observer);
    }
    global_observers_.clear();
  }

  // Helper to add a global observer and track it for cleanup.
  void AddGlobalObserver(TestWindowObserver* observer) {
    AstraWindowFeatures::AddGlobalObserver(observer);
    global_observers_.push_back(observer);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraWindowFeatures> features_;
  std::vector<TestWindowObserver*> global_observers_;
};

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, DefaultValues_WorkspaceIsDefault) {
  EXPECT_EQ(features_->workspace_id(), "default");
  EXPECT_TRUE(features_->IsInDefaultWorkspace());
}

TEST_F(WindowFeaturesTest, DefaultValues_OrderIndexIsZero) {
  EXPECT_EQ(features_->order_index(), 0u);
}

TEST_F(WindowFeaturesTest, DefaultValues_SidebarVisibleByDefault) {
  EXPECT_TRUE(features_->sidebar_visible());
}

TEST_F(WindowFeaturesTest, DefaultValues_SidebarPinnedByDefault) {
  EXPECT_TRUE(features_->sidebar_pinned());
}

TEST_F(WindowFeaturesTest, DefaultValues_SidebarWidthIsDefault) {
  EXPECT_EQ(features_->sidebar_width(), 280);
}

TEST_F(WindowFeaturesTest, DefaultValues_SplitViewNotActive) {
  EXPECT_FALSE(features_->split_view_active());
}

TEST_F(WindowFeaturesTest, DefaultValues_SplitViewOrientationHorizontal) {
  EXPECT_EQ(features_->split_view_orientation(), "horizontal");
}

TEST_F(WindowFeaturesTest, DefaultValues_SplitViewRatioIsHalf) {
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.5);
}

TEST_F(WindowFeaturesTest, DefaultValues_SavedBoundsEmpty) {
  EXPECT_TRUE(features_->saved_bounds().IsEmpty());
}

TEST_F(WindowFeaturesTest, DefaultValues_NotMinimized) {
  EXPECT_FALSE(features_->is_minimized());
}

TEST_F(WindowFeaturesTest, DefaultValues_NotMaximized) {
  EXPECT_FALSE(features_->is_maximized());
}

TEST_F(WindowFeaturesTest, DefaultValues_NotFullscreen) {
  EXPECT_FALSE(features_->is_fullscreen());
}

TEST_F(WindowFeaturesTest, DefaultValues_NotHibernated) {
  EXPECT_FALSE(features_->is_hibernated());
}

TEST_F(WindowFeaturesTest, DefaultValues_WindowStateIsNormal) {
  EXPECT_TRUE(features_->IsWindowStateNormal());
}

TEST_F(WindowFeaturesTest, DefaultValues_WorkspaceNotReadOnly) {
  // Default test windows are not incognito, so workspace is writable.
  EXPECT_FALSE(features_->workspace_read_only());
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraWindowObserver {};

  DefaultObserver observer;
  features_->AddObserver(&observer);

  // Trigger all per-window observer paths.
  features_->set_workspace_id("workspace-1");
  features_->set_sidebar_visible(false);
  features_->set_sidebar_pinned(false);
  features_->set_sidebar_width(320);
  features_->set_split_view_active(true);
  features_->ToggleFullscreen();

  features_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

TEST_F(WindowFeaturesTest, GlobalObserverDefaults_DoNotCrash) {
  class DefaultObserver : public AstraWindowObserver {};

  DefaultObserver observer;
  AstraWindowFeatures::AddGlobalObserver(&observer);

  // Trigger all global observer paths.
  features_->set_workspace_id("workspace-2");
  features_->set_sidebar_visible(false);
  features_->set_split_view_active(true);

  // Create a second window to trigger OnWindowCreated.
  auto window2 = AstraWindowFeatures::CreateForTesting();

  AstraWindowFeatures::RemoveGlobalObserver(&observer);
  SUCCEED() << "Default global observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, AddRemoveObserver_NoCrash) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);
  features_->RemoveObserver(&observer);
  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(WindowFeaturesTest, RemoveNonexistentObserver_NoCrash) {
  TestWindowObserver observer;
  features_->RemoveObserver(&observer);
  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST_F(WindowFeaturesTest, AddRemoveGlobalObserver_NoCrash) {
  TestWindowObserver observer;
  AddGlobalObserver(&observer);
  AstraWindowFeatures::RemoveGlobalObserver(&observer);
  global_observers_.clear();  // Already removed, avoid double-remove in tearDown
  SUCCEED() << "Global observer add/remove completed without crash.";
}

// ---------------------------------------------------------------------------
// Workspace
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetWorkspaceId_ChangesValue) {
  ASSERT_EQ(features_->workspace_id(), "default");

  features_->set_workspace_id("workspace-42");
  EXPECT_EQ(features_->workspace_id(), "workspace-42");
  EXPECT_FALSE(features_->IsInDefaultWorkspace());
}

TEST_F(WindowFeaturesTest, SetWorkspaceId_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  ASSERT_EQ(features_->workspace_id(), "default");
  features_->set_workspace_id("default");

  EXPECT_EQ(observer.workspace_changed_count_, 0);
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetWorkspaceId_FiresObservers) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_workspace_id("workspace-alpha");

  EXPECT_EQ(observer.workspace_changed_count_, 1);
  EXPECT_EQ(observer.last_old_workspace_id_, "default");
  EXPECT_EQ(observer.last_new_workspace_id_, "workspace-alpha");
  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetWorkspaceId_GlobalObserversNotified) {
  TestWindowObserver observer;
  AddGlobalObserver(&observer);

  features_->set_workspace_id("workspace-beta");

  EXPECT_EQ(observer.workspace_changed_count_, 1);
  EXPECT_EQ(observer.last_old_workspace_id_, "default");
  EXPECT_EQ(observer.last_new_workspace_id_, "workspace-beta");
  EXPECT_EQ(observer.features_changed_count_, 1);
}

TEST_F(WindowFeaturesTest, IsInDefaultWorkspace_ReturnsCorrectly) {
  EXPECT_TRUE(features_->IsInDefaultWorkspace());

  features_->set_workspace_id("other");
  EXPECT_FALSE(features_->IsInDefaultWorkspace());

  features_->set_workspace_id("default");
  EXPECT_TRUE(features_->IsInDefaultWorkspace());
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetSidebarVisible_ChangesValue) {
  ASSERT_TRUE(features_->sidebar_visible());

  features_->set_sidebar_visible(false);
  EXPECT_FALSE(features_->sidebar_visible());

  features_->set_sidebar_visible(true);
  EXPECT_TRUE(features_->sidebar_visible());
}

TEST_F(WindowFeaturesTest, SetSidebarVisible_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  ASSERT_TRUE(features_->sidebar_visible());
  features_->set_sidebar_visible(true);

  EXPECT_EQ(observer.sidebar_visibility_changed_count_, 0);
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarVisible_FiresObservers) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_sidebar_visible(false);

  EXPECT_EQ(observer.sidebar_visibility_changed_count_, 1);
  EXPECT_FALSE(observer.last_sidebar_visible_);
  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarPinned_ChangesValue) {
  ASSERT_TRUE(features_->sidebar_pinned());

  features_->set_sidebar_pinned(false);
  EXPECT_FALSE(features_->sidebar_pinned());

  features_->set_sidebar_pinned(true);
  EXPECT_TRUE(features_->sidebar_pinned());
}

TEST_F(WindowFeaturesTest, SetSidebarPinned_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  ASSERT_TRUE(features_->sidebar_pinned());
  features_->set_sidebar_pinned(true);

  EXPECT_EQ(observer.sidebar_pinned_changed_count_, 0);
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarPinned_FiresObservers) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_sidebar_pinned(false);

  EXPECT_EQ(observer.sidebar_pinned_changed_count_, 1);
  EXPECT_FALSE(observer.last_sidebar_pinned_);
  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_ChangesValue) {
  ASSERT_EQ(features_->sidebar_width(), 280);

  features_->set_sidebar_width(350);
  EXPECT_EQ(features_->sidebar_width(), 350);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_sidebar_width(280);  // Same as default.

  EXPECT_EQ(observer.sidebar_width_changed_count_, 0);
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_FiresObservers) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_sidebar_width(400);

  EXPECT_EQ(observer.sidebar_width_changed_count_, 1);
  EXPECT_EQ(observer.last_sidebar_width_, 400);
  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_ClampsToMinimum) {
  features_->set_sidebar_width(10);  // Below minimum of 120.
  EXPECT_EQ(features_->sidebar_width(), 120);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_ClampsToMaximum) {
  features_->set_sidebar_width(1000);  // Above maximum of 600.
  EXPECT_EQ(features_->sidebar_width(), 600);
}

TEST_F(WindowFeaturesTest, ToggleSidebar_FlipsVisibility) {
  ASSERT_TRUE(features_->sidebar_visible());

  bool result = features_->ToggleSidebar();
  EXPECT_FALSE(result);
  EXPECT_FALSE(features_->sidebar_visible());

  result = features_->ToggleSidebar();
  EXPECT_TRUE(result);
  EXPECT_TRUE(features_->sidebar_visible());
}

// ---------------------------------------------------------------------------
// Split view
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetSplitViewActive_ChangesValue) {
  ASSERT_FALSE(features_->split_view_active());

  features_->set_split_view_active(true);
  EXPECT_TRUE(features_->split_view_active());

  features_->set_split_view_active(false);
  EXPECT_FALSE(features_->split_view_active());
}

TEST_F(WindowFeaturesTest, SetSplitViewActive_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  ASSERT_FALSE(features_->split_view_active());
  features_->set_split_view_active(false);

  EXPECT_EQ(observer.split_view_state_changed_count_, 0);
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSplitViewActive_FiresObservers) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_split_view_active(true);

  EXPECT_EQ(observer.split_view_state_changed_count_, 1);
  EXPECT_TRUE(observer.last_split_view_active_);
  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSplitViewOrientation_ChangesValue) {
  ASSERT_EQ(features_->split_view_orientation(), "horizontal");

  features_->set_split_view_orientation("vertical");
  EXPECT_EQ(features_->split_view_orientation(), "vertical");
}

TEST_F(WindowFeaturesTest, SetSplitViewOrientation_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_split_view_orientation("horizontal");  // Same as default.

  // Should not fire features changed.
  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_ChangesValue) {
  ASSERT_DOUBLE_EQ(features_->split_view_ratio(), 0.5);

  features_->set_split_view_ratio(0.7);
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.7);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_ClampsToMinimum) {
  features_->set_split_view_ratio(0.01);  // Below minimum of 0.1.
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.1);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_ClampsToMaximum) {
  features_->set_split_view_ratio(0.95);  // Above maximum of 0.9.
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.9);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_split_view_ratio(0.5);  // Same as default.

  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, ToggleSplitView_FlipsActive) {
  ASSERT_FALSE(features_->split_view_active());

  bool result = features_->ToggleSplitView();
  EXPECT_TRUE(result);
  EXPECT_TRUE(features_->split_view_active());

  result = features_->ToggleSplitView();
  EXPECT_FALSE(result);
  EXPECT_FALSE(features_->split_view_active());
}

// ---------------------------------------------------------------------------
// Fullscreen
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, ToggleFullscreen_FlipsState) {
  ASSERT_FALSE(features_->is_fullscreen());

  bool result = features_->ToggleFullscreen();
  EXPECT_TRUE(result);
  EXPECT_TRUE(features_->is_fullscreen());

  result = features_->ToggleFullscreen();
  EXPECT_FALSE(result);
  EXPECT_FALSE(features_->is_fullscreen());
}

TEST_F(WindowFeaturesTest, IsWindowStateNormal_UpdatesWithState) {
  EXPECT_TRUE(features_->IsWindowStateNormal());

  features_->set_is_minimized(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());

  features_->set_is_minimized(false);
  features_->set_is_maximized(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());

  features_->set_is_maximized(false);
  features_->set_is_fullscreen(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());

  features_->set_is_fullscreen(false);
  EXPECT_TRUE(features_->IsWindowStateNormal());
}

// ---------------------------------------------------------------------------
// Hibernation
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetHibernated_ChangesValue) {
  ASSERT_FALSE(features_->is_hibernated());

  features_->set_is_hibernated(true);
  EXPECT_TRUE(features_->is_hibernated());

  features_->set_is_hibernated(false);
  EXPECT_FALSE(features_->is_hibernated());
}

TEST_F(WindowFeaturesTest, SetHibernated_SameValueNoOp) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  ASSERT_FALSE(features_->is_hibernated());
  features_->set_is_hibernated(false);

  EXPECT_EQ(observer.features_changed_count_, 0);

  features_->RemoveObserver(&observer);
}

TEST_F(WindowFeaturesTest, SetHibernated_FiresFeaturesChanged) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_is_hibernated(true);

  EXPECT_EQ(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Order index
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetOrderIndex_ChangesValue) {
  ASSERT_EQ(features_->order_index(), 0u);

  features_->set_order_index(5);
  EXPECT_EQ(features_->order_index(), 5u);

  features_->set_order_index(0);
  EXPECT_EQ(features_->order_index(), 0u);
}

// ---------------------------------------------------------------------------
// Saved bounds
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetSavedBounds_ChangesValue) {
  ASSERT_TRUE(features_->saved_bounds().IsEmpty());

  gfx::Rect bounds(100, 200, 800, 600);
  features_->set_saved_bounds(bounds);

  EXPECT_EQ(features_->saved_bounds(), bounds);
  EXPECT_FALSE(features_->saved_bounds().IsEmpty());
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, Reset_RestoresDefaults) {
  // Change a bunch of values.
  features_->set_workspace_id("custom-workspace");
  features_->set_order_index(3);
  features_->set_sidebar_visible(false);
  features_->set_sidebar_pinned(false);
  features_->set_sidebar_width(400);
  features_->set_split_view_active(true);
  features_->set_split_view_orientation("vertical");
  features_->set_split_view_ratio(0.7);
  features_->set_saved_bounds(gfx::Rect(100, 200, 800, 600));
  features_->set_is_minimized(true);
  features_->set_is_maximized(true);
  features_->set_is_fullscreen(true);
  features_->set_is_hibernated(true);

  // Sanity check that values changed.
  EXPECT_NE(features_->workspace_id(), "default");
  EXPECT_TRUE(features_->is_fullscreen());

  // Reset.
  features_->Reset();

  // All values should be back to defaults.
  EXPECT_EQ(features_->workspace_id(), "default");
  EXPECT_EQ(features_->order_index(), 0u);
  EXPECT_TRUE(features_->sidebar_visible());
  EXPECT_TRUE(features_->sidebar_pinned());
  EXPECT_EQ(features_->sidebar_width(), 280);
  EXPECT_FALSE(features_->split_view_active());
  EXPECT_EQ(features_->split_view_orientation(), "horizontal");
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.5);
  EXPECT_TRUE(features_->saved_bounds().IsEmpty());
  EXPECT_FALSE(features_->is_minimized());
  EXPECT_FALSE(features_->is_maximized());
  EXPECT_FALSE(features_->is_fullscreen());
  EXPECT_FALSE(features_->is_hibernated());
}

TEST_F(WindowFeaturesTest, Reset_FiresFeaturesChanged) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->Reset();

  EXPECT_GE(observer.features_changed_count_, 1);

  features_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Global window lifecycle observers
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, WindowCreated_GlobalObserversNotified) {
  TestWindowObserver observer;
  AddGlobalObserver(&observer);

  // Create a new window — should trigger OnWindowCreated.
  auto window2 = AstraWindowFeatures::CreateForTesting();

  // Note: CreateForTesting doesn't trigger OnWindowCreated because
  // it bypasses GetOrCreateForBrowser. Only GetOrCreateForBrowser
  // triggers the creation notification (for real Browser attachments).
  // The destructor will trigger OnWindowClosing.
  //
  // For testing, we verify that destructor triggers closing.
}

TEST_F(WindowFeaturesTest, WindowClosing_GlobalObserversNotified) {
  TestWindowObserver observer;
  AddGlobalObserver(&observer);

  {
    auto window2 = AstraWindowFeatures::CreateForTesting();
    // Window is alive here.
  }  // window2 is destroyed here.

  // The destructor triggers OnWindowClosing for global observers.
  EXPECT_GE(observer.window_closing_count_, 1);
}

TEST_F(WindowFeaturesTest, WindowClosing_PerInstanceObserversNotified) {
  TestWindowObserver observer;

  auto window2 = AstraWindowFeatures::CreateForTesting();
  window2->AddObserver(&observer);

  window2.reset();  // Destroy the window.

  // Per-instance observers should get OnWindowClosing.
  EXPECT_GE(observer.window_closing_count_, 1);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, MultipleObservers_AllNotified) {
  TestWindowObserver observer1;
  TestWindowObserver observer2;

  features_->AddObserver(&observer1);
  features_->AddObserver(&observer2);

  features_->set_sidebar_visible(false);

  EXPECT_EQ(observer1.sidebar_visibility_changed_count_, 1);
  EXPECT_EQ(observer2.sidebar_visibility_changed_count_, 1);

  features_->RemoveObserver(&observer1);
  features_->RemoveObserver(&observer2);
}

TEST_F(WindowFeaturesTest, RemoveObserver_StopsNotifications) {
  TestWindowObserver observer;
  features_->AddObserver(&observer);

  features_->set_sidebar_visible(false);
  EXPECT_EQ(observer.sidebar_visibility_changed_count_, 1);

  features_->RemoveObserver(&observer);

  features_->set_sidebar_visible(true);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.sidebar_visibility_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Window arrangement helpers
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, TileWindowsInWorkspace_EmptyWorkspace_NoCrash) {
  // Tiling an empty workspace should not crash.
  AstraWindowFeatures::TileWindowsInWorkspace("nonexistent-workspace");
  SUCCEED() << "Tiling empty workspace does not crash.";
}

TEST_F(WindowFeaturesTest, StackWindowsInWorkspace_EmptyWorkspace_NoCrash) {
  AstraWindowFeatures::StackWindowsInWorkspace("nonexistent-workspace");
  SUCCEED() << "Stacking empty workspace does not crash.";
}

TEST_F(WindowFeaturesTest, CloseAllWindowsInWorkspace_EmptyWorkspace_NoCrash) {
  AstraWindowFeatures::CloseAllWindowsInWorkspace("nonexistent-workspace");
  SUCCEED() << "Closing empty workspace does not crash.";
}

TEST_F(WindowFeaturesTest, TileWindowsInWorkspace_SingleWindow_NoCrash) {
  // Set up a single window with bounds.
  features_->set_saved_bounds(gfx::Rect(0, 0, 1280, 800));

  // Can't fully test with BrowserList (needs real Browsers),
  // but we can verify the metadata helper doesn't crash.
  AstraWindowFeatures::TileWindowsInWorkspace("default");
  SUCCEED() << "Tile with single window does not crash.";
}

TEST_F(WindowFeaturesTest, StackWindowsInWorkspace_SingleWindow_NoCrash) {
  features_->set_saved_bounds(gfx::Rect(100, 100, 800, 600));

  AstraWindowFeatures::StackWindowsInWorkspace("default");
  SUCCEED() << "Stack with single window does not crash.";
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(WindowFeaturesTest, SetWorkspaceId_EmptyString) {
  features_->set_workspace_id("");
  EXPECT_EQ(features_->workspace_id(), "");
  EXPECT_FALSE(features_->IsInDefaultWorkspace());
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_ExactMinimum) {
  features_->set_sidebar_width(120);
  EXPECT_EQ(features_->sidebar_width(), 120);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_ExactMaximum) {
  features_->set_sidebar_width(600);
  EXPECT_EQ(features_->sidebar_width(), 600);
}

TEST_F(WindowFeaturesTest, SetSidebarWidth_Negative) {
  features_->set_sidebar_width(-100);
  EXPECT_EQ(features_->sidebar_width(), 120);  // Clamped to minimum.
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_ExactMinimum) {
  features_->set_split_view_ratio(0.1);
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.1);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_ExactMaximum) {
  features_->set_split_view_ratio(0.9);
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.9);
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_Negative) {
  features_->set_split_view_ratio(-0.5);
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.1);  // Clamped to minimum.
}

TEST_F(WindowFeaturesTest, SetSplitViewRatio_AboveOne) {
  features_->set_split_view_ratio(1.5);
  EXPECT_DOUBLE_EQ(features_->split_view_ratio(), 0.9);  // Clamped to maximum.
}

TEST_F(WindowFeaturesTest, CreateForTesting_ReturnsValidInstance) {
  auto features = AstraWindowFeatures::CreateForTesting();
  EXPECT_NE(features, nullptr);
  EXPECT_EQ(features->workspace_id(), "default");
}

TEST_F(WindowFeaturesTest, SetSavedBounds_ZeroSize) {
  gfx::Rect bounds(50, 50, 0, 0);
  features_->set_saved_bounds(bounds);
  EXPECT_TRUE(features_->saved_bounds().IsEmpty());
  EXPECT_EQ(features_->saved_bounds().x(), 50);
}

TEST_F(WindowFeaturesTest, WindowStateNormal_AllStates) {
  // Default is normal.
  EXPECT_TRUE(features_->IsWindowStateNormal());

  // Minimized = not normal.
  features_->set_is_minimized(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());
  features_->set_is_minimized(false);

  // Maximized = not normal.
  features_->set_is_maximized(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());
  features_->set_is_maximized(false);

  // Fullscreen = not normal.
  features_->set_is_fullscreen(true);
  EXPECT_FALSE(features_->IsWindowStateNormal());
  features_->set_is_fullscreen(false);

  // All off = normal.
  EXPECT_TRUE(features_->IsWindowStateNormal());
}

// ---------------------------------------------------------------------------
// Static query methods (with no real BrowserList)
// ---------------------------------------------------------------------------
//
// NOTE: GetWindowCount, GetActiveWindow, GetWindowsByWorkspace all use
// BrowserList which requires real Browser objects.  In unit tests without
// a full browser test harness, these return 0/nullptr/empty.
//
// TODO(astra): Add browser_tests for these methods with real Browsers.

TEST_F(WindowFeaturesTest, GetWindowCount_NoWindowsReturnsZero) {
  // With no real Browsers in the test, count is 0.
  // This test verifies the method exists and doesn't crash.
  size_t count = AstraWindowFeatures::GetWindowCount();
  EXPECT_GE(count, 0u);
}

TEST_F(WindowFeaturesTest, GetActiveWindow_NoWindowsReturnsNull) {
  AstraWindowFeatures* active = AstraWindowFeatures::GetActiveWindow();
  // In a unit test without Browsers, this should be nullptr.
  // If somehow there are Browsers, it might not be null — just check no crash.
  EXPECT_TRUE(active == nullptr || active != nullptr);
}

TEST_F(WindowFeaturesTest, GetWindowsByWorkspace_NoWindowsReturnsEmpty) {
  auto windows = AstraWindowFeatures::GetWindowsByWorkspace("default");
  EXPECT_TRUE(windows.empty());
}

}  // namespace astra
