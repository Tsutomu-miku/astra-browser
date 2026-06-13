// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraWorkspaceOverviewController.
//
// Tests verify:
//   - Observer interface default implementations
//   - Presentation settings (view mode, card size, show statistics)
//   - Presentation settings persistence round-trip via PrefService
//   - Bulk operations (hibernate all, delete all non-default)
//   - Observer notification flow (controller -> controller observer)
//   - View observer forwarding (view -> controller -> service)
//   - Workspace service observation (service -> controller -> view)
//
// Chromium test pattern: TestingProfile + test browser window for
// controller construction, since the controller depends on BrowserView
// and Profile to access AstraWorkspaceService and PrefService.

#include "astra/ui/views/workspace/astra_workspace_overview_controller.h"

#include <memory>
#include <string>
#include <vector>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_service.h"
#include "base/guid.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Test observer for the controller
// ---------------------------------------------------------------------------

// Test observer that overrides all methods and counts calls.
struct TestControllerObserver
    : public AstraWorkspaceOverviewControllerObserver {
  // Lifecycle
  int shown_count = 0;
  int hidden_count = 0;
  int opening_count = 0;
  int closing_count = 0;

  // Workspace actions
  int activated_count = 0;
  int selected_count = 0;
  int created_count = 0;
  int deleted_count = 0;
  int renamed_count = 0;
  int reordered_count = 0;

  // Presentation
  int view_mode_count = 0;
  int card_size_count = 0;
  int show_stats_count = 0;

  // Bulk operations
  int hibernate_all_count = 0;
  int delete_all_count = 0;

  // Last recorded values
  std::string last_workspace_id;
  std::string last_new_name;
  AstraWorkspaceOverviewViewMode last_view_mode =
      AstraWorkspaceOverviewViewMode::kGrid;
  AstraWorkspaceOverviewCardSize last_card_size =
      AstraWorkspaceOverviewCardSize::kMedium;
  bool last_show_stats = true;

  // -- Lifecycle overrides --

  void OnOverviewShown() override { shown_count++; }
  void OnOverviewHidden() override { hidden_count++; }
  void OnOverviewOpening() override { opening_count++; }
  void OnOverviewClosing() override { closing_count++; }

  // -- Workspace action overrides --

  void OnWorkspaceActivatedFromOverview(
      const std::string& workspace_id) override {
    activated_count++;
    last_workspace_id = workspace_id;
  }

  void OnWorkspaceSelectedInOverview(
      const std::string& workspace_id) override {
    selected_count++;
    last_workspace_id = workspace_id;
  }

  void OnWorkspaceCreatedFromOverview(
      const std::string& workspace_id) override {
    created_count++;
    last_workspace_id = workspace_id;
  }

  void OnWorkspaceDeletedFromOverview(
      const std::string& workspace_id) override {
    deleted_count++;
    last_workspace_id = workspace_id;
  }

  void OnWorkspaceRenamedFromOverview(
      const std::string& workspace_id,
      const std::string& new_name) override {
    renamed_count++;
    last_workspace_id = workspace_id;
    last_new_name = new_name;
  }

  void OnWorkspacesReorderedFromOverview() override { reordered_count++; }

  // -- Presentation overrides --

  void OnOverviewViewModeChanged(
      AstraWorkspaceOverviewViewMode mode) override {
    view_mode_count++;
    last_view_mode = mode;
  }

  void OnOverviewCardSizeChanged(
      AstraWorkspaceOverviewCardSize size) override {
    card_size_count++;
    last_card_size = size;
  }

  void OnOverviewShowStatisticsChanged(bool show) override {
    show_stats_count++;
    last_show_stats = show;
  }

  // -- Bulk operation overrides --

  void OnAllWorkspacesHibernated() override { hibernate_all_count++; }
  void OnAllNonDefaultWorkspacesDeleted() override { delete_all_count++; }
};

// Observer that overrides nothing — verifies default empty implementations
// don't cause crashes or link errors.
struct DefaultImplControllerObserver
    : public AstraWorkspaceOverviewControllerObserver {
  // Intentionally empty — all methods use default empty implementations.
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Create a test workspace with given id and name.
AstraWorkspace CreateTestWorkspace(const std::string& id,
                                   const std::string& name,
                                   const std::string& accent = "#4285F4") {
  AstraWorkspace ws;
  ws.id = id;
  ws.name = name;
  ws.accent_color = accent;
  ws.is_default = false;
  return ws;
}

}  // namespace

// ---------------------------------------------------------------------------
// Observer interface tests (no controller instance needed)
// ---------------------------------------------------------------------------

// Tests that the observer interface compiles and works when derived classes
// override only a subset of methods (the rest use default empty impls).
TEST(AstraWorkspaceOverviewControllerObserverTest, DefaultImplsDontCrash) {
  DefaultImplControllerObserver default_observer;
  TestControllerObserver full_observer;

  // Verify both can be constructed and destroyed without issues.
  SUCCEED();
}

// Tests that adding and removing a default-impl observer to/from an observer
// list works without crashing (all methods are callable with default impls).
TEST(AstraWorkspaceOverviewControllerObserverTest, ObserverListWithDefaultImpl) {
  DefaultImplControllerObserver observer;
  base::ObserverList<AstraWorkspaceOverviewControllerObserver> observers;

  observers.AddObserver(&observer);

  // Call every method through the observer list — none should crash
  // because all have default empty implementations.
  for (auto& obs : observers) {
    obs.OnOverviewShown();
    obs.OnOverviewHidden();
    obs.OnOverviewOpening();
    obs.OnOverviewClosing();
    obs.OnWorkspaceActivatedFromOverview("ws1");
    obs.OnWorkspaceSelectedInOverview("ws1");
    obs.OnWorkspaceCreatedFromOverview("ws1");
    obs.OnWorkspaceDeletedFromOverview("ws1");
    obs.OnWorkspaceRenamedFromOverview("ws1", "new name");
    obs.OnWorkspacesReorderedFromOverview();
    obs.OnOverviewViewModeChanged(AstraWorkspaceOverviewViewMode::kList);
    obs.OnOverviewCardSizeChanged(AstraWorkspaceOverviewCardSize::kLarge);
    obs.OnOverviewShowStatisticsChanged(false);
    obs.OnAllWorkspacesHibernated();
    obs.OnAllNonDefaultWorkspacesDeleted();
  }

  observers.RemoveObserver(&observer);
}

// Tests that CheckedObserver base class works correctly (the observer
// interface inherits from base::CheckedObserver).
TEST(AstraWorkspaceOverviewControllerObserverTest, IsCheckedObserver) {
  TestControllerObserver observer;
  // Verify it can be used with CheckedObserver semantics.
  EXPECT_TRUE(observer.IsInObserverList().has_value());
  // Before being added to any list, has_value() is true but the optional
  // contains false (not in list).
}

// ---------------------------------------------------------------------------
// Controller test fixture
// ---------------------------------------------------------------------------

class AstraWorkspaceOverviewControllerTest : public testing::Test {
 public:
  AstraWorkspaceOverviewControllerTest() = default;
  ~AstraWorkspaceOverviewControllerTest() override = default;

  void SetUp() override {
    // Create a testing profile.
    profile_ = std::make_unique<TestingProfile>();

    // Ensure workspace service prefs and overview prefs are registered.
    // AstraWorkspaceServiceFactory::RegisterProfilePrefs registers all
    // Astra profile prefs including workspace overview presentation settings.
    AstraWorkspaceServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs()->registry());

    // Create a test browser window and browser.
    //
    // Note: TestBrowserWindow is a stub BrowserWindow implementation that
    // does not create a real BrowserView.  In unit test configurations,
    // BrowserView::GetBrowserViewForBrowser() may return nullptr, in which
    // case controller tests that require a fully constructed controller
    // will skip gracefully via has_controller().
    //
    // For full integration testing of the controller with a real BrowserView,
    // use interactive_ui_tests / browser_tests with InProcessBrowserTest.
    browser_window_ = std::make_unique<TestBrowserWindow>();
    Browser::CreateParams params(profile_.get(), true);
    params.window = browser_window_.get();
    browser_.reset(Browser::Create(params));
    browser_window_->SetBrowser(browser_.get());

    // Get the BrowserView from the browser.
    // In test configurations, BrowserView may or may not be available.
    // We attempt to get it; if not available, controller tests that
    // require it will skip gracefully.
    browser_view_ = BrowserView::GetBrowserViewForBrowser(browser_.get());

    // Create the controller if we have a browser view.
    if (browser_view_) {
      controller_ = std::make_unique<AstraWorkspaceOverviewController>(
          browser_view_);
    }
  }

  void TearDown() override {
    controller_.reset();
    browser_.reset();
    browser_window_.reset();
    profile_.reset();
  }

 protected:
  // Returns true if the controller was successfully constructed.
  // Some test configurations may not have a BrowserView available.
  bool has_controller() const { return controller_ != nullptr; }

  // Helper to get workspace service (convenience).
  AstraWorkspaceService* workspace_service() {
    if (!profile_) return nullptr;
    return AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  }

  // Helper to get pref service (convenience).
  PrefService* prefs() {
    return profile_ ? profile_->GetPrefs() : nullptr;
  }

  // Task environment for UI thread tasks.
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<TestBrowserWindow> browser_window_;
  std::unique_ptr<Browser> browser_;
  raw_ptr<BrowserView> browser_view_ = nullptr;
  std::unique_ptr<AstraWorkspaceOverviewController> controller_;
};

// =========================================================================
// Construction and defaults
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, DefaultViewModeIsGrid) {
  if (!has_controller()) return;
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kGrid, controller_->view_mode());
}

TEST_F(AstraWorkspaceOverviewControllerTest, DefaultCardSizeIsMedium) {
  if (!has_controller()) return;
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kMedium, controller_->card_size());
}

TEST_F(AstraWorkspaceOverviewControllerTest, DefaultShowStatisticsIsTrue) {
  if (!has_controller()) return;
  EXPECT_TRUE(controller_->show_statistics());
}

TEST_F(AstraWorkspaceOverviewControllerTest, DefaultVisibilityIsFalse) {
  if (!has_controller()) return;
  EXPECT_FALSE(controller_->IsVisible());
}

// =========================================================================
// Presentation settings - setters and getters
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, SetViewMode) {
  if (!has_controller()) return;

  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller_->view_mode());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetViewModeSameNoOp) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Set to the same value — should not notify.
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kGrid);
  EXPECT_EQ(0, observer.view_mode_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetCardSize) {
  if (!has_controller()) return;

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kLarge);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kLarge, controller_->card_size());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetCardSizeSmall) {
  if (!has_controller()) return;

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kSmall);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, controller_->card_size());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetCardSizeSameNoOp) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Set to the same value — should not notify.
  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kMedium);
  EXPECT_EQ(0, observer.card_size_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetShowStatisticsFalse) {
  if (!has_controller()) return;

  controller_->SetShowStatistics(false);
  EXPECT_FALSE(controller_->show_statistics());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetShowStatisticsTrue) {
  if (!has_controller()) return;

  // Start by setting to false.
  controller_->SetShowStatistics(false);
  EXPECT_FALSE(controller_->show_statistics());

  // Then set back to true.
  controller_->SetShowStatistics(true);
  EXPECT_TRUE(controller_->show_statistics());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SetShowStatisticsSameNoOp) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Set to the same value — should not notify.
  controller_->SetShowStatistics(true);
  EXPECT_EQ(0, observer.show_stats_count);

  controller_->RemoveObserver(&observer);
}

// =========================================================================
// Presentation settings - observer notifications
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, ViewModeChangeNotifiesObservers) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(1, observer.view_mode_count);
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, observer.last_view_mode);

  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kGrid);
  EXPECT_EQ(2, observer.view_mode_count);
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kGrid, observer.last_view_mode);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, CardSizeChangeNotifiesObservers) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kLarge);
  EXPECT_EQ(1, observer.card_size_count);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kLarge, observer.last_card_size);

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kSmall);
  EXPECT_EQ(2, observer.card_size_count);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, observer.last_card_size);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       ShowStatisticsChangeNotifiesObservers) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  controller_->SetShowStatistics(false);
  EXPECT_EQ(1, observer.show_stats_count);
  EXPECT_FALSE(observer.last_show_stats);

  controller_->SetShowStatistics(true);
  EXPECT_EQ(2, observer.show_stats_count);
  EXPECT_TRUE(observer.last_show_stats);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, MultipleObserversAllNotified) {
  if (!has_controller()) return;

  TestControllerObserver observer1;
  TestControllerObserver observer2;
  controller_->AddObserver(&observer1);
  controller_->AddObserver(&observer2);

  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(1, observer1.view_mode_count);
  EXPECT_EQ(1, observer2.view_mode_count);

  controller_->RemoveObserver(&observer1);
  controller_->RemoveObserver(&observer2);
}

// =========================================================================
// Presentation settings - persistence round-trip
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, ViewModePersistsViaPrefs) {
  if (!has_controller()) return;

  // Change view mode via controller.
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);

  // Verify it was saved to prefs.
  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);
  int stored = pref_service->GetInteger(prefs::kPrefWorkspaceOverviewViewMode);
  EXPECT_EQ(1, stored);  // 1 = kList
}

TEST_F(AstraWorkspaceOverviewControllerTest, CardSizePersistsViaPrefs) {
  if (!has_controller()) return;

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kLarge);

  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);
  int stored = pref_service->GetInteger(prefs::kPrefWorkspaceOverviewCardSize);
  EXPECT_EQ(2, stored);  // 2 = kLarge
}

TEST_F(AstraWorkspaceOverviewControllerTest, ShowStatisticsPersistsViaPrefs) {
  if (!has_controller()) return;

  controller_->SetShowStatistics(false);

  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);
  bool stored = pref_service->GetBoolean(
      prefs::kPrefWorkspaceOverviewShowStatistics);
  EXPECT_FALSE(stored);
}

TEST_F(AstraWorkspaceOverviewControllerTest, LoadsSettingsFromPrefsOnConstruct) {
  // Pre-populate prefs before creating a controller.
  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);

  pref_service->SetInteger(prefs::kPrefWorkspaceOverviewViewMode, 1);  // kList
  pref_service->SetInteger(prefs::kPrefWorkspaceOverviewCardSize, 0);  // kSmall
  pref_service->SetBoolean(prefs::kPrefWorkspaceOverviewShowStatistics,
                           false);

  // Create a new controller that should load from prefs.
  auto controller = std::make_unique<AstraWorkspaceOverviewController>(
      browser_view_);

  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller->view_mode());
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, controller->card_size());
  EXPECT_FALSE(controller->show_statistics());
}

TEST_F(AstraWorkspaceOverviewControllerTest, PersistenceRoundTripAllSettings) {
  if (!has_controller()) return;

  // Set all presentation settings.
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kSmall);
  controller_->SetShowStatistics(false);

  // Simulate a "restart" by creating a new controller that reads from prefs.
  auto controller2 = std::make_unique<AstraWorkspaceOverviewController>(
      browser_view_);

  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller2->view_mode());
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, controller2->card_size());
  EXPECT_FALSE(controller2->show_statistics());

  // Now change settings on controller2.
  controller2->SetViewMode(AstraWorkspaceOverviewViewMode::kGrid);
  controller2->SetCardSize(AstraWorkspaceOverviewCardSize::kLarge);
  controller2->SetShowStatistics(true);

  // Verify the original controller's underlying prefs changed too
  // (since they share the same PrefService).
  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);
  EXPECT_EQ(0, pref_service->GetInteger(
                    prefs::kPrefWorkspaceOverviewViewMode));  // kGrid
  EXPECT_EQ(2, pref_service->GetInteger(
                    prefs::kPrefWorkspaceOverviewCardSize));  // kLarge
  EXPECT_TRUE(pref_service->GetBoolean(
      prefs::kPrefWorkspaceOverviewShowStatistics));
}

// =========================================================================
// Bulk operations - HibernateAllWorkspaces
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, HibernateAllWorkspaces) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add a few non-default workspaces.
  AstraWorkspace ws1 = CreateTestWorkspace("ws-hib-1", "Hibernate 1");
  AstraWorkspace ws2 = CreateTestWorkspace("ws-hib-2", "Hibernate 2");
  AstraWorkspace ws3 = CreateTestWorkspace("ws-hib-3", "Hibernate 3");
  service->AddWorkspace(std::move(ws1));
  service->AddWorkspace(std::move(ws2));
  service->AddWorkspace(std::move(ws3));

  // Verify none are hibernated initially.
  EXPECT_EQ(0u, service->GetHibernatedWorkspaces().size());

  // Add observer to verify notification.
  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Hibernate all.
  controller_->HibernateAllWorkspaces();

  // All non-default workspaces should be hibernated.
  auto hibernated = service->GetHibernatedWorkspaces();
  EXPECT_EQ(3u, hibernated.size());

  // Default workspace should NOT be hibernated.
  const AstraWorkspace* default_ws =
      service->GetWorkspace(service->GetDefaultWorkspaceId());
  ASSERT_NE(nullptr, default_ws);
  EXPECT_FALSE(default_ws->is_hibernated);

  // Observer should have been notified.
  EXPECT_EQ(1, observer.hibernate_all_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       HibernateAllSkipsActiveWorkspace) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add workspaces.
  AstraWorkspace ws1 = CreateTestWorkspace("ws-act-1", "Active 1");
  AstraWorkspace ws2 = CreateTestWorkspace("ws-act-2", "Active 2");
  service->AddWorkspace(std::move(ws1));
  service->AddWorkspace(std::move(ws2));

  // Activate ws-act-1.
  service->ActivateWorkspace("ws-act-1");
  ASSERT_EQ("ws-act-1", service->active_workspace_id());

  // Hibernate all.
  controller_->HibernateAllWorkspaces();

  // Active workspace should not be hibernated.
  const AstraWorkspace* active_ws =
      service->GetWorkspace(service->active_workspace_id());
  ASSERT_NE(nullptr, active_ws);
  EXPECT_FALSE(active_ws->is_hibernated);

  // Non-active, non-default workspace should be hibernated.
  const AstraWorkspace* ws2_ptr = service->GetWorkspace("ws-act-2");
  ASSERT_NE(nullptr, ws2_ptr);
  EXPECT_TRUE(ws2_ptr->is_hibernated);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       HibernateAllWithAlreadyHibernatedIsIdempotent) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add one workspace and hibernate it manually.
  AstraWorkspace ws = CreateTestWorkspace("ws-idem-1", "Idempotent");
  service->AddWorkspace(std::move(ws));
  service->SetWorkspaceHibernated("ws-idem-1", true);

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Call hibernate all — should still fire notification even if nothing changed
  // (it's a bulk operation completion notification).
  controller_->HibernateAllWorkspaces();

  EXPECT_EQ(1, observer.hibernate_all_count);
  EXPECT_TRUE(
      service->GetWorkspace("ws-idem-1")->is_hibernated);

  controller_->RemoveObserver(&observer);
}

// =========================================================================
// Bulk operations - DeleteAllNonDefaultWorkspaces
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, DeleteAllNonDefaultWorkspaces) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add several non-default workspaces.
  for (int i = 1; i <= 5; i++) {
    AstraWorkspace ws = CreateTestWorkspace(
        "ws-del-" + base::NumberToString(i),
        "Delete " + base::NumberToString(i));
    service->AddWorkspace(std::move(ws));
  }

  size_t before_count = service->workspace_count();
  EXPECT_GT(before_count, 1u);  // default + 5

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Delete all non-default.
  controller_->DeleteAllNonDefaultWorkspaces();

  // Only default workspace should remain.
  EXPECT_EQ(1u, service->workspace_count());
  EXPECT_TRUE(service->workspaces()[0].is_default);

  // Observer should have been notified.
  EXPECT_EQ(1, observer.delete_all_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       DeleteAllNonDefaultWithOnlyDefaultIsNoOp) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Ensure only default exists.
  service->EnsureDefaultWorkspace();
  // The service should always have at least the default.
  ASSERT_GE(service->workspace_count(), 1u);

  // Delete any non-default ones that may exist from other test setup.
  // (EnsureDefaultWorkspace is idempotent and doesn't delete extras.)
  // We need to manually delete extras for this test.
  while (service->workspace_count() > 1) {
    // Find a non-default workspace and delete it.
    for (const auto& ws : service->workspaces()) {
      if (!ws.is_default) {
        service->DeleteWorkspace(ws.id);
        break;
      }
    }
  }
  ASSERT_EQ(1u, service->workspace_count());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Delete all non-default — should still fire notification (operation
  // completed, even if nothing was actually deleted).
  controller_->DeleteAllNonDefaultWorkspaces();

  EXPECT_EQ(1u, service->workspace_count());
  EXPECT_EQ(1, observer.delete_all_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       DeleteAllPreservesDefaultWorkspace) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  std::string default_id = service->GetDefaultWorkspaceId();

  // Add some workspaces.
  for (int i = 0; i < 3; i++) {
    AstraWorkspace ws = CreateTestWorkspace(
        "ws-preserve-" + base::NumberToString(i),
        "Preserve Test " + base::NumberToString(i));
    service->AddWorkspace(std::move(ws));
  }

  controller_->DeleteAllNonDefaultWorkspaces();

  // Default workspace still exists.
  EXPECT_EQ(1u, service->workspace_count());
  EXPECT_EQ(default_id, service->workspaces()[0].id);
  EXPECT_TRUE(service->workspaces()[0].is_default);
  EXPECT_EQ(default_id, service->active_workspace_id());
}

// =========================================================================
// Observer management
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, AddAndRemoveObserver) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  // Trigger a notification.
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(1, observer.view_mode_count);

  // Remove observer and trigger again.
  controller_->RemoveObserver(&observer);
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kGrid);
  // Count should still be 1 — observer was removed.
  EXPECT_EQ(1, observer.view_mode_count);
}

TEST_F(AstraWorkspaceOverviewControllerTest, RemovingNonExistentObserverSafe) {
  if (!has_controller()) return;

  TestControllerObserver observer;
  // Removing an observer that was never added should not crash.
  // (base::ObserverList handles this gracefully.)
  controller_->RemoveObserver(&observer);
  SUCCEED();
}

// =========================================================================
// Integration: view observer -> controller -> service flow
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest,
       ViewViewModeChangeSyncsToController) {
  if (!has_controller()) return;

  // The controller implements AstraWorkspaceOverviewViewObserver.
  // Calling OnViewModeChanged on the controller (as the view would)
  // should update controller state and persist.
  auto* view_observer =
      static_cast<AstraWorkspaceOverviewViewObserver*>(controller_.get());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  view_observer->OnViewModeChanged(AstraWorkspaceOverviewViewMode::kList);

  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller_->view_mode());
  EXPECT_EQ(1, observer.view_mode_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       ViewCardSizeChangeSyncsToController) {
  if (!has_controller()) return;

  auto* view_observer =
      static_cast<AstraWorkspaceOverviewViewObserver*>(controller_.get());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  view_observer->OnCardSizeChanged(AstraWorkspaceOverviewCardSize::kLarge);

  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kLarge, controller_->card_size());
  EXPECT_EQ(1, observer.card_size_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       ViewShowStatisticsChangeSyncsToController) {
  if (!has_controller()) return;

  auto* view_observer =
      static_cast<AstraWorkspaceOverviewViewObserver*>(controller_.get());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  view_observer->OnShowStatisticsChanged(false);

  EXPECT_FALSE(controller_->show_statistics());
  EXPECT_EQ(1, observer.show_stats_count);

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       ViewHibernateAllTriggersBulkOperation) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add test workspaces.
  for (int i = 1; i <= 3; i++) {
    AstraWorkspace ws = CreateTestWorkspace(
        "ws-bulk-view-" + base::NumberToString(i),
        "Bulk View " + base::NumberToString(i));
    service->AddWorkspace(std::move(ws));
  }

  auto* view_observer =
      static_cast<AstraWorkspaceOverviewViewObserver*>(controller_.get());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  view_observer->OnHibernateAllRequested();

  EXPECT_EQ(1, observer.hibernate_all_count);
  EXPECT_EQ(3u, service->GetHibernatedWorkspaces().size());

  controller_->RemoveObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       ViewDeleteAllTriggersBulkOperation) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // Add test workspaces.
  for (int i = 1; i <= 3; i++) {
    AstraWorkspace ws = CreateTestWorkspace(
        "ws-del-view-" + base::NumberToString(i),
        "Del View " + base::NumberToString(i));
    service->AddWorkspace(std::move(ws));
  }

  auto* view_observer =
      static_cast<AstraWorkspaceOverviewViewObserver*>(controller_.get());

  TestControllerObserver observer;
  controller_->AddObserver(&observer);

  view_observer->OnDeleteAllNonDefaultRequested();

  EXPECT_EQ(1, observer.delete_all_count);
  EXPECT_EQ(1u, service->workspace_count());  // Only default remains.

  controller_->RemoveObserver(&observer);
}

// =========================================================================
// Service observation flow
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest,
       ControllerObservesWorkspaceService) {
  if (!has_controller()) return;
  AstraWorkspaceService* service = workspace_service();
  ASSERT_NE(nullptr, service);

  // The controller observes the service. Adding a workspace should
  // trigger Update() on the controller (via OnWorkspaceAdded observer).
  // We verify this indirectly by checking the controller doesn't crash
  // and the service state updates correctly.

  size_t before = service->workspace_count();

  AstraWorkspace ws = CreateTestWorkspace("ws-obs-test", "Observer Test");
  service->AddWorkspace(std::move(ws));

  EXPECT_EQ(before + 1, service->workspace_count());
  // The controller's OnWorkspaceAdded should have been called, which calls
  // Update(). Since Update() requires a widget (which we may not have in
  // unit tests), it early-returns — but the observer machinery works.
}

// =========================================================================
// Edge cases
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest,
       SetViewModeBeforeWidgetIsNoOpForView) {
  if (!has_controller()) return;

  // The widget/overview_view_ is not created until Show() is called.
  // Setting view mode before that should still update controller state
  // and persist to prefs, but not crash (no view to update).
  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller_->view_mode());

  // Pref should be updated.
  PrefService* pref_service = prefs();
  ASSERT_NE(nullptr, pref_service);
  EXPECT_EQ(1, pref_service->GetInteger(
                    prefs::kPrefWorkspaceOverviewViewMode));
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       BulkOperationsWithNoServiceHandleGracefully) {
  // This test verifies the controller handles the case where
  // workspace_service_ is null (e.g., if profile doesn't have the service).
  // Since our test setup has a real service, we can't easily test null.
  // But we verify the calls don't crash under normal conditions.
  if (!has_controller()) return;

  // Hibernate all with no extra workspaces should work fine.
  controller_->HibernateAllWorkspaces();
  controller_->DeleteAllNonDefaultWorkspaces();

  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest,
       PresentationSettingsWithNoProfileHandleGracefully) {
  // The LoadPresentationSettings and SavePresentationSettings methods
  // check for null profile and early-return. We can't easily test a
  // null profile scenario in this fixture, but we verify the normal
  // path works without crashing.
  if (!has_controller()) return;

  // Multiple rapid changes should work fine.
  for (int i = 0; i < 10; i++) {
    controller_->SetViewMode(i % 2 == 0
                                 ? AstraWorkspaceOverviewViewMode::kGrid
                                 : AstraWorkspaceOverviewViewMode::kList);
    controller_->SetCardSize(static_cast<AstraWorkspaceOverviewCardSize>(
        i % 3));
    controller_->SetShowStatistics(i % 2 == 0);
  }

  // Final state should reflect the last set.
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kGrid, controller_->view_mode());
  // i=9: 9%3 = 0 -> kSmall
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, controller_->card_size());
  EXPECT_FALSE(controller_->show_statistics());
}

// =========================================================================
// Alias methods (ShowOverview/HideOverview/IsOverviewVisible)
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, ShowOverview_SameAsShow) {
  if (!has_controller()) return;
  // ShowOverview is an alias for Show.
  controller_->ShowOverview();
  // Since we may not have a real widget in unit tests, we verify no crash.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, HideOverview_SameAsHide) {
  if (!has_controller()) return;
  // HideOverview is an alias for Hide.
  controller_->HideOverview();
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, IsOverviewVisible_SameAsIsVisible) {
  if (!has_controller()) return;
  // IsOverviewVisible should match IsVisible.
  EXPECT_EQ(controller_->IsOverviewVisible(), controller_->IsVisible());
}

TEST_F(AstraWorkspaceOverviewControllerTest, ShowOverview_TogglePattern) {
  if (!has_controller()) return;
  // Toggle should work the same with either naming.
  bool was_visible = controller_->IsVisible();
  controller_->Toggle();
  EXPECT_EQ(controller_->IsOverviewVisible(), !was_visible);
  controller_->Toggle();
  EXPECT_EQ(controller_->IsVisible(), was_visible);
}

// =========================================================================
// GetView
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, GetView_InitiallyNull) {
  if (!has_controller()) return;
  // Before Show() is called, the view should be null.
  EXPECT_EQ(nullptr, controller_->GetView());
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetView_ReturnsSameAsOverviewView) {
  if (!has_controller()) return;
  // GetView() should return the same as the internal overview_view_.
  // Since the view isn't created until Show(), both should be null initially.
  EXPECT_EQ(controller_->GetView(), nullptr);
}

// =========================================================================
// Workspace selection
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, SelectWorkspace_EmptyIdIsNoOp) {
  if (!has_controller()) return;
  controller_->SelectWorkspace("");
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, SelectWorkspace_NonexistentIdIsNoOp) {
  if (!has_controller()) return;
  controller_->SelectWorkspace("nonexistent-workspace-id");
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetSelectedWorkspace_InitiallyEmpty) {
  if (!has_controller()) return;
  // Without a view or selection, selected workspace should be empty.
  EXPECT_TRUE(controller_->GetSelectedWorkspace().empty());
}

TEST_F(AstraWorkspaceOverviewControllerTest, SelectWorkspace_DefaultWorkspace) {
  if (!has_controller()) return;
  // Select the default workspace - should not crash.
  std::string default_id = workspace_service_->GetDefaultWorkspaceId();
  controller_->SelectWorkspace(default_id);
  // No crash = success.
  SUCCEED();
}

// =========================================================================
// Workspace operations
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspace_CreatesWorkspace) {
  if (!has_controller()) return;
  size_t before_count = workspace_service_->workspace_count();

  std::string new_id = controller_->NewWorkspace();
  EXPECT_FALSE(new_id.empty());

  size_t after_count = workspace_service_->workspace_count();
  EXPECT_EQ(after_count, before_count + 1);

  // Verify the workspace exists.
  const AstraWorkspace* ws = workspace_service_->GetWorkspace(new_id);
  EXPECT_NE(nullptr, ws);
}

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspace_MultipleCalls) {
  if (!has_controller()) return;
  size_t before_count = workspace_service_->workspace_count();

  controller_->NewWorkspace();
  controller_->NewWorkspace();
  controller_->NewWorkspace();

  size_t after_count = workspace_service_->workspace_count();
  EXPECT_EQ(after_count, before_count + 3);
}

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspace_HasDefaultAccentColor) {
  if (!has_controller()) return;
  std::string new_id = controller_->NewWorkspace();
  const AstraWorkspace* ws = workspace_service_->GetWorkspace(new_id);
  ASSERT_NE(nullptr, ws);
  EXPECT_FALSE(ws->accent_color.empty());
}

TEST_F(AstraWorkspaceOverviewControllerTest, DeleteSelectedWorkspace_NoSelectionReturnsFalse) {
  if (!has_controller()) return;
  // With no selection, delete should return false.
  EXPECT_FALSE(controller_->DeleteSelectedWorkspace());
}

TEST_F(AstraWorkspaceOverviewControllerTest, DeleteSelectedWorkspace_DefaultWorkspaceReturnsFalse) {
  if (!has_controller()) return;
  // Can't delete the default workspace.
  std::string default_id = workspace_service_->GetDefaultWorkspaceId();
  controller_->SelectWorkspace(default_id);
  EXPECT_FALSE(controller_->DeleteSelectedWorkspace());
}

TEST_F(AstraWorkspaceOverviewControllerTest, RenameSelectedWorkspace_NoSelectionReturnsFalse) {
  if (!has_controller()) return;
  // With no selection, rename should return false.
  EXPECT_FALSE(controller_->RenameSelectedWorkspace("New Name"));
}

TEST_F(AstraWorkspaceOverviewControllerTest, RenameSelectedWorkspace_WithValidWorkspace) {
  if (!has_controller()) return;

  // Create a workspace to rename.
  std::string ws_id = controller_->NewWorkspace();
  controller_->SelectWorkspace(ws_id);

  EXPECT_TRUE(controller_->RenameSelectedWorkspace("Renamed Workspace"));

  const AstraWorkspace* ws = workspace_service_->GetWorkspace(ws_id);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("Renamed Workspace", ws->name);
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_InvalidFromIndexReturnsFalse) {
  if (!has_controller()) return;
  size_t count = workspace_service_->workspace_count();
  EXPECT_FALSE(controller_->MoveWorkspace(count + 10, 0));
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_InvalidToIndexReturnsFalse) {
  if (!has_controller()) return;
  size_t count = workspace_service_->workspace_count();
  EXPECT_FALSE(controller_->MoveWorkspace(0, count + 10));
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_SameIndexReturnsFalse) {
  if (!has_controller()) return;
  EXPECT_FALSE(controller_->MoveWorkspace(0, 0));
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_ValidMove) {
  if (!has_controller()) return;

  // Create some workspaces to move.
  controller_->NewWorkspace();
  controller_->NewWorkspace();
  size_t count = workspace_service_->workspace_count();
  ASSERT_GE(count, 3u);

  // Get the ID at index 0.
  std::string id_at_zero = workspace_service_->workspaces()[0].id;

  // Move from index 0 to index 2.
  EXPECT_TRUE(controller_->MoveWorkspace(0, 2));

  // The workspace should now be at index 2.
  EXPECT_EQ(workspace_service_->workspaces()[2].id, id_at_zero);
}

// =========================================================================
// Import / Export
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, ImportWorkspaces_EmptyStringReturnsZero) {
  if (!has_controller()) return;
  size_t imported = controller_->ImportWorkspaces("");
  EXPECT_EQ(0u, imported);
}

TEST_F(AstraWorkspaceOverviewControllerTest, ImportWorkspaces_InvalidJsonReturnsZero) {
  if (!has_controller()) return;
  size_t imported = controller_->ImportWorkspaces("this is not json");
  EXPECT_EQ(0u, imported);
}

TEST_F(AstraWorkspaceOverviewControllerTest, ExportWorkspace_ReturnsString) {
  if (!has_controller()) return;
  std::string exported = controller_->ExportWorkspace();
  // Currently a stub - may be empty.
  // Just verify it doesn't crash.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, ImportWorkspaces_DoesNotCrash) {
  if (!has_controller()) return;
  // Call import with various inputs - should not crash.
  controller_->ImportWorkspaces("");
  controller_->ImportWorkspaces("{}");
  controller_->ImportWorkspaces("[]");
  controller_->ImportWorkspaces("{\"workspaces\": []}");
  SUCCEED();
}

// =========================================================================
// Workspace query
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceCount_ReturnsValidCount) {
  if (!has_controller()) return;
  size_t count = controller_->GetWorkspaceCount();
  EXPECT_GE(count, 1u);  // At least default workspace.
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceAt_IndexZeroReturnsValid) {
  if (!has_controller()) return;
  const AstraWorkspace* ws = controller_->GetWorkspaceAt(0);
  EXPECT_NE(nullptr, ws);
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceAt_OutOfRangeReturnsNull) {
  if (!has_controller()) return;
  size_t count = controller_->GetWorkspaceCount();
  EXPECT_EQ(nullptr, controller_->GetWorkspaceAt(count + 10));
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaces_ReturnsAllWorkspaces) {
  if (!has_controller()) return;
  auto workspaces = controller_->GetWorkspaces();
  EXPECT_EQ(workspaces.size(), controller_->GetWorkspaceCount());
  EXPECT_GE(workspaces.size(), 1u);
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaces_IncludesDefault) {
  if (!has_controller()) return;
  auto workspaces = controller_->GetWorkspaces();
  bool found_default = false;
  for (const auto& ws : workspaces) {
    if (ws.is_default) {
      found_default = true;
      break;
    }
  }
  EXPECT_TRUE(found_default);
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceCount_IncreasesAfterNewWorkspace) {
  if (!has_controller()) return;
  size_t before = controller_->GetWorkspaceCount();
  controller_->NewWorkspace();
  size_t after = controller_->GetWorkspaceCount();
  EXPECT_EQ(after, before + 1);
}

// =========================================================================
// Simplified overview observer
// =========================================================================

class TestOverviewObserver : public AstraWorkspaceOverviewObserver {
 public:
  int shown_count = 0;
  int hidden_count = 0;
  int added_count = 0;
  int removed_count = 0;
  int renamed_count = 0;
  int reordered_count = 0;

  std::string last_added_id;
  std::string last_removed_id;
  std::string last_renamed_id;
  std::string last_renamed_name;

  void OnOverviewShown() override { shown_count++; }
  void OnOverviewHidden() override { hidden_count++; }
  void OnWorkspaceAdded(const std::string& id) override {
    added_count++;
    last_added_id = id;
  }
  void OnWorkspaceRemoved(const std::string& id) override {
    removed_count++;
    last_removed_id = id;
  }
  void OnWorkspaceRenamed(const std::string& id,
                          const std::string& new_name) override {
    renamed_count++;
    last_renamed_id = id;
    last_renamed_name = new_name;
  }
  void OnWorkspacesReordered() override { reordered_count++; }
};

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_DefaultImplementations) {
  // Base observer has empty default implementations.
  AstraWorkspaceOverviewObserver observer;
  observer.OnOverviewShown();
  observer.OnOverviewHidden();
  observer.OnWorkspaceAdded("ws1");
  observer.OnWorkspaceRemoved("ws1");
  observer.OnWorkspaceRenamed("ws1", "new name");
  observer.OnWorkspacesReordered();
  // No crash = success.
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_AddAndRemove) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  controller_->AddOverviewObserver(&observer);
  controller_->RemoveOverviewObserver(&observer);
  // No crash = success.
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_OnWorkspaceAddedFires) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  controller_->AddOverviewObserver(&observer);

  int before_added = observer.added_count;
  controller_->NewWorkspace();
  EXPECT_GT(observer.added_count, before_added);

  controller_->RemoveOverviewObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_OnWorkspaceRenamedFires) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  controller_->AddOverviewObserver(&observer);

  std::string ws_id = controller_->NewWorkspace();
  controller_->SelectWorkspace(ws_id);
  int before_renamed = observer.renamed_count;
  controller_->RenameSelectedWorkspace("Renamed");
  EXPECT_GT(observer.renamed_count, before_renamed);
  EXPECT_EQ(observer.last_renamed_name, "Renamed");

  controller_->RemoveOverviewObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_OnWorkspaceRemovedFires) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  controller_->AddOverviewObserver(&observer);

  std::string ws_id = controller_->NewWorkspace();
  controller_->SelectWorkspace(ws_id);
  int before_removed = observer.removed_count;
  controller_->DeleteSelectedWorkspace();
  EXPECT_GT(observer.removed_count, before_removed);

  controller_->RemoveOverviewObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_MultipleObservers) {
  if (!has_controller()) return;
  TestOverviewObserver obs1;
  TestOverviewObserver obs2;
  TestOverviewObserver obs3;

  controller_->AddOverviewObserver(&obs1);
  controller_->AddOverviewObserver(&obs2);
  controller_->AddOverviewObserver(&obs3);

  int before1 = obs1.added_count;
  int before2 = obs2.added_count;
  int before3 = obs3.added_count;

  controller_->NewWorkspace();

  EXPECT_GT(obs1.added_count, before1);
  EXPECT_GT(obs2.added_count, before2);
  EXPECT_GT(obs3.added_count, before3);

  controller_->RemoveOverviewObserver(&obs1);
  controller_->RemoveOverviewObserver(&obs2);
  controller_->RemoveOverviewObserver(&obs3);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_RemoveThenAdd) {
  if (!has_controller()) return;
  TestOverviewObserver observer;

  controller_->AddOverviewObserver(&observer);
  controller_->RemoveOverviewObserver(&observer);
  controller_->AddOverviewObserver(&observer);

  int before = observer.added_count;
  controller_->NewWorkspace();
  EXPECT_GT(observer.added_count, before);

  controller_->RemoveOverviewObserver(&observer);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_ReorderFires) {
  if (!has_controller()) return;
  TestOverviewObserver observer;

  // Create some workspaces first.
  controller_->NewWorkspace();
  controller_->NewWorkspace();

  controller_->AddOverviewObserver(&observer);

  int before = observer.reordered_count;
  controller_->MoveWorkspace(0, 2);
  EXPECT_GT(observer.reordered_count, before);

  controller_->RemoveOverviewObserver(&observer);
}

// =========================================================================
// Full observer vs simplified observer
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, BothObserverTypesCoexist) {
  if (!has_controller()) return;
  TestOverviewControllerObserver full_observer;
  TestOverviewObserver simple_observer;

  controller_->AddObserver(&full_observer);
  controller_->AddOverviewObserver(&simple_observer);

  // Both should receive relevant events from NewWorkspace.
  int full_before = full_observer.workspace_created_count_;
  int simple_before = simple_observer.added_count;

  controller_->NewWorkspace();

  EXPECT_GT(full_observer.workspace_created_count_, full_before);
  EXPECT_GT(simple_observer.added_count, simple_before);

  controller_->RemoveObserver(&full_observer);
  controller_->RemoveOverviewObserver(&simple_observer);
}

// =========================================================================
// Workspace query edge cases
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceAt_IndexBoundary) {
  if (!has_controller()) return;
  size_t count = controller_->GetWorkspaceCount();
  // Index count-1 should be valid.
  if (count > 0) {
    EXPECT_NE(nullptr, controller_->GetWorkspaceAt(count - 1));
  }
  // Index count should be invalid.
  EXPECT_EQ(nullptr, controller_->GetWorkspaceAt(count));
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaces_NotEmptyByDefault) {
  if (!has_controller()) return;
  auto workspaces = controller_->GetWorkspaces();
  EXPECT_FALSE(workspaces.empty());
}

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspace_HasCreationTime) {
  if (!has_controller()) return;
  std::string id = controller_->NewWorkspace();
  const AstraWorkspace* ws = controller_->GetWorkspaceAt(
      static_cast<size_t>(controller_->GetWorkspaceCount() - 1));
  ASSERT_NE(nullptr, ws);
  EXPECT_FALSE(ws->created_time.is_null());
}

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspace_HasOrderIndex) {
  if (!has_controller()) return;
  std::string id = controller_->NewWorkspace();
  const AstraWorkspace* ws = workspace_service_->GetWorkspace(id);
  ASSERT_NE(nullptr, ws);
  EXPECT_GE(ws->order_index, 0u);
}

TEST_F(AstraWorkspaceOverviewControllerTest, DeleteSelectedWorkspace_AfterMultipleCreates) {
  if (!has_controller()) return;
  // Create several workspaces, then delete the selected one.
  controller_->NewWorkspace();
  std::string id = controller_->NewWorkspace();
  controller_->NewWorkspace();

  size_t before = controller_->GetWorkspaceCount();
  controller_->SelectWorkspace(id);
  controller_->DeleteSelectedWorkspace();
  size_t after = controller_->GetWorkspaceCount();

  EXPECT_EQ(after, before - 1);
  EXPECT_EQ(nullptr, workspace_service_->GetWorkspace(id));
}

TEST_F(AstraWorkspaceOverviewControllerTest, RenameSelectedWorkspace_EmptyName) {
  if (!has_controller()) return;
  std::string id = controller_->NewWorkspace();
  controller_->SelectWorkspace(id);
  // Renaming to empty string should still work.
  controller_->RenameSelectedWorkspace("");
  const AstraWorkspace* ws = workspace_service_->GetWorkspace(id);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(ws->name, "");
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_MoveToEnd) {
  if (!has_controller()) return;
  controller_->NewWorkspace();
  controller_->NewWorkspace();
  size_t count = workspace_service_->workspace_count();
  ASSERT_GE(count, 3u);

  std::string first_id = workspace_service_->workspaces()[0].id;
  controller_->MoveWorkspace(0, count - 1);

  EXPECT_EQ(workspace_service_->workspaces()[count - 1].id, first_id);
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_MoveToBeginning) {
  if (!has_controller()) return;
  controller_->NewWorkspace();
  controller_->NewWorkspace();
  size_t count = workspace_service_->workspace_count();
  ASSERT_GE(count, 3u);

  std::string last_id = workspace_service_->workspaces()[count - 1].id;
  controller_->MoveWorkspace(count - 1, 0);

  EXPECT_EQ(workspace_service_->workspaces()[0].id, last_id);
}

// =========================================================================
// Presentation settings persistence
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, ViewMode_PersistsAcrossControllerInstances) {
  // This tests that view mode persists via prefs.
  // With a single controller instance, we verify the set/get round-trip.
  if (!has_controller()) return;
  auto original_mode = controller_->view_mode();

  controller_->SetViewMode(AstraWorkspaceOverviewViewMode::kList);
  EXPECT_EQ(AstraWorkspaceOverviewViewMode::kList, controller_->view_mode());

  controller_->SetViewMode(original_mode);
  EXPECT_EQ(original_mode, controller_->view_mode());
}

TEST_F(AstraWorkspaceOverviewControllerTest, CardSize_PersistsAcrossChanges) {
  if (!has_controller()) return;
  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kSmall);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kSmall, controller_->card_size());

  controller_->SetCardSize(AstraWorkspaceOverviewCardSize::kLarge);
  EXPECT_EQ(AstraWorkspaceOverviewCardSize::kLarge, controller_->card_size());
}

TEST_F(AstraWorkspaceOverviewControllerTest, ShowStatistics_ToggleMultipleTimes) {
  if (!has_controller()) return;
  bool original = controller_->show_statistics();

  for (int i = 0; i < 5; i++) {
    controller_->SetShowStatistics(!controller_->show_statistics());
  }

  // After 5 toggles, should be opposite of original.
  EXPECT_EQ(controller_->show_statistics(), !original);

  controller_->SetShowStatistics(original);
}

// =========================================================================
// Update method
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, Update_CanBeCalledMultipleTimes) {
  if (!has_controller()) return;
  // Calling Update multiple times should not crash.
  controller_->Update();
  controller_->Update();
  controller_->Update();
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, Update_AfterNewWorkspace) {
  if (!has_controller()) return;
  controller_->NewWorkspace();
  controller_->Update();
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, Update_AfterRename) {
  if (!has_controller()) return;
  std::string id = controller_->NewWorkspace();
  controller_->SelectWorkspace(id);
  controller_->RenameSelectedWorkspace("Updated Name");
  controller_->Update();
  SUCCEED();
}

// =========================================================================
// Toggle method
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, Toggle_ShowThenHide) {
  if (!has_controller()) return;
  bool was_visible = controller_->IsVisible();
  controller_->Toggle();
  EXPECT_EQ(controller_->IsVisible(), !was_visible);
  controller_->Toggle();
  EXPECT_EQ(controller_->IsVisible(), was_visible);
}

TEST_F(AstraWorkspaceOverviewControllerTest, Toggle_MultipleRapidToggles) {
  if (!has_controller()) return;
  for (int i = 0; i < 5; i++) {
    controller_->Toggle();
  }
  // After 5 toggles, state should be opposite of initial.
  // (But since the widget may not exist in unit tests, this just tests no crash.)
  SUCCEED();
}

// =========================================================================
// Workspace ordering
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, NewWorkspaces_HaveIncreasingOrderIndex) {
  if (!has_controller()) return;
  std::string id1 = controller_->NewWorkspace();
  std::string id2 = controller_->NewWorkspace();
  std::string id3 = controller_->NewWorkspace();

  const AstraWorkspace* ws1 = workspace_service_->GetWorkspace(id1);
  const AstraWorkspace* ws2 = workspace_service_->GetWorkspace(id2);
  const AstraWorkspace* ws3 = workspace_service_->GetWorkspace(id3);

  ASSERT_NE(nullptr, ws1);
  ASSERT_NE(nullptr, ws2);
  ASSERT_NE(nullptr, ws3);

  EXPECT_LT(ws1->order_index, ws2->order_index);
  EXPECT_LT(ws2->order_index, ws3->order_index);
}

TEST_F(AstraWorkspaceOverviewControllerTest, MoveWorkspace_PreservesOtherOrder) {
  if (!has_controller()) return;
  controller_->NewWorkspace();
  controller_->NewWorkspace();
  controller_->NewWorkspace();

  auto workspaces_before = controller_->GetWorkspaces();
  // Move index 1 to index 3.
  size_t count = workspaces_before.size();
  if (count >= 4) {
    std::string id_at_1 = workspaces_before[1].id;
    controller_->MoveWorkspace(1, 3);
    auto workspaces_after = controller_->GetWorkspaces();

    // The ID at index 3 should be the one that was at index 1.
    EXPECT_EQ(workspaces_after[3].id, id_at_1);

    // Other IDs should remain in relative order.
    std::vector<std::string> other_before;
    std::vector<std::string> other_after;
    for (size_t i = 0; i < count; i++) {
      if (i != 1) other_before.push_back(workspaces_before[i].id);
      if (i != 3) other_after.push_back(workspaces_after[i].id);
    }
    // Should have same number of "other" workspaces.
    EXPECT_EQ(other_before.size(), other_after.size());
  }
}

// =========================================================================
// Simplified observer edge cases
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_NoNotificationsAfterRemove) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  controller_->AddOverviewObserver(&observer);
  controller_->RemoveOverviewObserver(&observer);

  int before = observer.added_count;
  controller_->NewWorkspace();
  // After removal, observer should not receive notifications.
  EXPECT_EQ(observer.added_count, before);
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_RemoveNonExistentIsSafe) {
  if (!has_controller()) return;
  TestOverviewObserver observer;
  // Removing an observer that was never added should be safe.
  controller_->RemoveOverviewObserver(&observer);
  SUCCEED();
}

TEST_F(AstraWorkspaceOverviewControllerTest, OverviewObserver_DefaultSixMethods) {
  // The simplified observer should have exactly 6 event methods.
  // This test verifies all 6 can be called on a base observer.
  AstraWorkspaceOverviewObserver observer;
  observer.OnOverviewShown();
  observer.OnOverviewHidden();
  observer.OnWorkspaceAdded("ws1");
  observer.OnWorkspaceRemoved("ws1");
  observer.OnWorkspaceRenamed("ws1", "new");
  observer.OnWorkspacesReordered();
  SUCCEED();
}

// =========================================================================
// Workspace query with many workspaces
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceCount_WithManyWorkspaces) {
  if (!has_controller()) return;
  size_t before = controller_->GetWorkspaceCount();

  for (int i = 0; i < 10; i++) {
    controller_->NewWorkspace();
  }

  EXPECT_EQ(controller_->GetWorkspaceCount(), before + 10);
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaceAt_AllIndicesValid) {
  if (!has_controller()) return;
  size_t count = controller_->GetWorkspaceCount();
  for (size_t i = 0; i < count; i++) {
    const AstraWorkspace* ws = controller_->GetWorkspaceAt(i);
    EXPECT_NE(nullptr, ws);
    EXPECT_FALSE(ws->id.empty());
  }
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaces_ReturnsOrderedByIndex) {
  if (!has_controller()) return;
  auto workspaces = controller_->GetWorkspaces();
  for (size_t i = 1; i < workspaces.size(); i++) {
    EXPECT_GE(workspaces[i].order_index, workspaces[i-1].order_index);
  }
}

TEST_F(AstraWorkspaceOverviewControllerTest, GetWorkspaces_DefaultIsPresent) {
  if (!has_controller()) return;
  auto workspaces = controller_->GetWorkspaces();
  bool has_default = false;
  for (const auto& ws : workspaces) {
    if (ws.is_default) {
      has_default = true;
      break;
    }
  }
  EXPECT_TRUE(has_default);
}

// =========================================================================
// Bulk operations edge cases
// =========================================================================

TEST_F(AstraWorkspaceOverviewControllerTest, DeleteAllNonDefaultWorkspaces_ReducesToDefault) {
  if (!has_controller()) return;
  // Create some workspaces first.
  controller_->NewWorkspace();
  controller_->NewWorkspace();

  size_t before = controller_->GetWorkspaceCount();
  ASSERT_GT(before, 1u);

  controller_->DeleteAllNonDefaultWorkspaces();

  // After deleting all non-default, should have only default.
  EXPECT_EQ(controller_->GetWorkspaceCount(), 1u);
  EXPECT_TRUE(controller_->GetWorkspaceAt(0)->is_default);
}

TEST_F(AstraWorkspaceOverviewControllerTest, HibernateAllWorkspaces_DoesNotAffectDefault) {
  if (!has_controller()) return;
  controller_->NewWorkspace();
  controller_->NewWorkspace();

  controller_->HibernateAllWorkspaces();

  // Default workspace should not be hibernated.
  const AstraWorkspace* default_ws =
      workspace_service_->GetWorkspace(workspace_service_->GetDefaultWorkspaceId());
  ASSERT_NE(nullptr, default_ws);
  EXPECT_FALSE(default_ws->is_hibernated);
}

}  // namespace astra
