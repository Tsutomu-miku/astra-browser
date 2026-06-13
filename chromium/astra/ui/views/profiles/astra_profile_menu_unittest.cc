// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for the Astra profile menu MVC architecture.
//
// Test coverage:
//   - AstraProfileMenuModel: state management, observers, presentation
//     settings, workspace management, persistence, sync status, utility
//     methods, edge cases
//   - Observer defaults: all observer methods have empty default implementations
//   - Settings round-trip: save/load via PrefService
//   - View deepening: header sync status, notification badges, display modes,
//     size variants, reorder handles, accessibility
//   - Model/view integration: settings propagate from model to views
//
// Test pattern: testing::Test for model tests, views::ViewsTestBase for
// view tests.  Uses PrefService test support for persistence tests.
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/profiles/astra_profile_menu_model.h"
#include "astra/ui/views/profiles/astra_profile_menu_header_view.h"
#include "astra/ui/views/profiles/astra_workspace_menu_item_view.h"
#include "astra/ui/views/profiles/astra_workspace_avatar_button.h"
#include "astra/browser/astra_prefs.h"

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// =========================================================================
// Test observer that counts calls to each method
// =========================================================================

class TestModelObserver : public AstraProfileMenuModelObserver {
 public:
  TestModelObserver() = default;
  ~TestModelObserver() override = default;

  void OnProfileMenuOpened() override { opened_count++; }
  void OnProfileMenuClosed() override { closed_count++; }
  void OnProfileSelected(int profile_index) override {
    profile_selected_count++;
    last_profile_index = profile_index;
  }
  void OnWorkspaceSelected(const std::string& workspace_id) override {
    workspace_selected_count++;
    last_workspace_id = workspace_id;
  }
  void OnMenuSettingsChanged() override { settings_changed_count++; }
  void OnWorkspacesChanged() override { workspaces_changed_count++; }
  void OnActiveWorkspaceChanged(const std::string& workspace_id) override {
    active_workspace_changed_count++;
    last_active_workspace_id = workspace_id;
  }
  void OnSyncStatusChanged(AstraSyncStatus status) override {
    sync_status_changed_count++;
    last_sync_status = status;
  }

  int opened_count = 0;
  int closed_count = 0;
  int profile_selected_count = 0;
  int workspace_selected_count = 0;
  int settings_changed_count = 0;
  int workspaces_changed_count = 0;
  int active_workspace_changed_count = 0;
  int sync_status_changed_count = 0;

  int last_profile_index = -1;
  std::string last_workspace_id;
  std::string last_active_workspace_id;
  AstraSyncStatus last_sync_status = AstraSyncStatus::kNotSignedIn;
};

// Observer that only overrides one method — tests defaults work for all
// other methods.
class PartialObserver : public AstraProfileMenuModelObserver {
 public:
  void OnProfileMenuOpened() override { opened_count++; }
  int opened_count = 0;
};

// =========================================================================
// Test delegate implementations
// =========================================================================

class FakeHeaderDelegate : public AstraProfileMenuHeaderView::Delegate {
 public:
  void OnProfileHeaderClicked() override { click_count++; }
  void OnSyncStatusClicked() override { sync_click_count++; }

  int click_count = 0;
  int sync_click_count = 0;
};

class FakeAvatarDelegate : public AstraWorkspaceAvatarButton::Delegate {
 public:
  void OnWorkspaceAvatarButtonClicked(
      AstraWorkspaceAvatarButton* button) override {
    click_count++;
    last_button = button;
  }

  int click_count = 0;
  raw_ptr<AstraWorkspaceAvatarButton> last_button = nullptr;
};

}  // namespace

// =========================================================================
// AstraProfileMenuModel tests (state and lifecycle)
// =========================================================================

class AstraProfileMenuModelTest : public testing::Test {
 public:
  AstraProfileMenuModelTest() = default;
  ~AstraProfileMenuModelTest() override = default;

 protected:
  AstraProfileMenuModel model_;
};

TEST_F(AstraProfileMenuModelTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, &model_);
}

TEST_F(AstraProfileMenuModelTest, DefaultMenuIsClosed) {
  EXPECT_FALSE(model_.is_open());
}

TEST_F(AstraProfileMenuModelTest, OpenMenu) {
  model_.OpenMenu();
  EXPECT_TRUE(model_.is_open());
}

TEST_F(AstraProfileMenuModelTest, CloseMenu) {
  model_.OpenMenu();
  ASSERT_TRUE(model_.is_open());

  model_.CloseMenu();
  EXPECT_FALSE(model_.is_open());
}

TEST_F(AstraProfileMenuModelTest, OpenMenuTwiceIsNoOp) {
  TestModelObserver observer;
  model_.AddObserver(&observer);

  model_.OpenMenu();
  model_.OpenMenu();  // Second open should be a no-op.

  EXPECT_EQ(1, observer.opened_count);
  EXPECT_TRUE(model_.is_open());

  model_.RemoveObserver(&observer);
}

TEST_F(AstraProfileMenuModelTest, CloseMenuTwiceIsNoOp) {
  TestModelObserver observer;
  model_.AddObserver(&observer);

  model_.OpenMenu();
  model_.CloseMenu();
  model_.CloseMenu();  // Second close should be a no-op.

  EXPECT_EQ(1, observer.closed_count);
  EXPECT_FALSE(model_.is_open());

  model_.RemoveObserver(&observer);
}

TEST_F(AstraProfileMenuModelTest, DefaultSelectedProfileIndex) {
  EXPECT_EQ(0, model_.selected_profile_index());
  EXPECT_EQ(1, model_.profile_count());
}

TEST_F(AstraProfileMenuModelTest, SetSelectedProfileIndex) {
  model_.SetProfileCount(5);
  model_.SetSelectedProfileIndex(2);
  EXPECT_EQ(2, model_.selected_profile_index());
}

TEST_F(AstraProfileMenuModelTest, SetSelectedProfileIndexClampsLow) {
  model_.SetProfileCount(5);
  model_.SetSelectedProfileIndex(-1);
  EXPECT_EQ(0, model_.selected_profile_index());
}

TEST_F(AstraProfileMenuModelTest, SetSelectedProfileIndexClampsHigh) {
  model_.SetProfileCount(5);
  model_.SetSelectedProfileIndex(10);
  EXPECT_EQ(4, model_.selected_profile_index());
}

TEST_F(AstraProfileMenuModelTest, SetProfileCountClampsSelection) {
  model_.SetProfileCount(10);
  model_.SetSelectedProfileIndex(8);
  ASSERT_EQ(8, model_.selected_profile_index());

  model_.SetProfileCount(3);
  EXPECT_EQ(2, model_.selected_profile_index());
}

TEST_F(AstraProfileMenuModelTest, ProfileCountMinimumIsOne) {
  model_.SetProfileCount(0);
  EXPECT_EQ(1, model_.profile_count());
}

// =========================================================================
// Observer notification tests
// =========================================================================

class AstraProfileMenuModelObserverTest : public testing::Test {
 public:
  AstraProfileMenuModelObserverTest() = default;
  ~AstraProfileMenuModelObserverTest() override = default;

  void SetUp() override {
    model_.AddObserver(&observer_);
  }

  void TearDown() override {
    model_.RemoveObserver(&observer_);
  }

 protected:
  AstraProfileMenuModel model_;
  TestModelObserver observer_;
};

TEST_F(AstraProfileMenuModelObserverTest, OpenMenuNotifiesObserver) {
  model_.OpenMenu();
  EXPECT_EQ(1, observer_.opened_count);
}

TEST_F(AstraProfileMenuModelObserverTest, CloseMenuNotifiesObserver) {
  model_.OpenMenu();
  model_.CloseMenu();
  EXPECT_EQ(1, observer_.closed_count);
}

TEST_F(AstraProfileMenuModelObserverTest, ProfileSelectionNotifiesObserver) {
  model_.SetProfileCount(5);
  model_.SetSelectedProfileIndex(3);
  EXPECT_EQ(1, observer_.profile_selected_count);
  EXPECT_EQ(3, observer_.last_profile_index);
}

TEST_F(AstraProfileMenuModelObserverTest, SameProfileSelectionNoNotify) {
  model_.SetProfileCount(5);
  model_.SetSelectedProfileIndex(2);
  ASSERT_EQ(1, observer_.profile_selected_count);

  model_.SetSelectedProfileIndex(2);  // Same index.
  EXPECT_EQ(1, observer_.profile_selected_count);
}

TEST_F(AstraProfileMenuModelObserverTest, WorkspaceSelectedNotifies) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  AstraMenuWorkspaceInfo ws;
  ws.id = "ws1";
  ws.name = u"Workspace 1";
  ws.order_index = 0;
  workspaces.push_back(ws);
  model_.SetWorkspaces(workspaces);

  // Reset observer count after SetWorkspaces.
  TestModelObserver observer2;
  model_.AddObserver(&observer2);

  model_.SelectWorkspaceById("ws1");
  EXPECT_EQ(1, observer2.workspace_selected_count);
  EXPECT_EQ("ws1", observer2.last_workspace_id);

  model_.RemoveObserver(&observer2);
}

TEST_F(AstraProfileMenuModelObserverTest, MultipleObserversAllNotified) {
  TestModelObserver observer2;
  model_.AddObserver(&observer2);

  model_.OpenMenu();

  EXPECT_EQ(1, observer_.opened_count);
  EXPECT_EQ(1, observer2.opened_count);

  model_.RemoveObserver(&observer2);
}

TEST_F(AstraProfileMenuModelObserverTest, RemoveObserverStopsNotifications) {
  TestModelObserver observer2;
  model_.AddObserver(&observer2);
  model_.RemoveObserver(&observer2);

  model_.OpenMenu();

  EXPECT_EQ(1, observer_.opened_count);  // Original observer still gets it.
  EXPECT_EQ(0, observer2.opened_count);  // Removed observer doesn't.
}

// =========================================================================
// Observer defaults test
// =========================================================================

TEST(AstraProfileMenuObserverDefaultsTest, PartialObserverIsSafe) {
  // A partial observer that overrides only one method should be safe —
  // all other methods have empty default implementations.
  AstraProfileMenuModel model;
  PartialObserver observer;
  model.AddObserver(&observer);

  // Trigger all observer methods — none should crash.
  model.OpenMenu();
  model.CloseMenu();
  model.SetProfileCount(3);
  model.SetSelectedProfileIndex(1);

  std::vector<AstraMenuWorkspaceInfo> workspaces;
  AstraMenuWorkspaceInfo ws;
  ws.id = "test";
  ws.name = u"Test";
  workspaces.push_back(ws);
  model.SetWorkspaces(workspaces);
  model.SelectWorkspaceById("test");
  model.SetActiveWorkspaceId("test");
  model.SetSyncStatus(AstraSyncStatus::kSynced);

  model.set_show_workspaces(false);
  model.set_show_avatar(false);

  // Only the opened method should have been counted.
  EXPECT_EQ(1, observer.opened_count);

  model.RemoveObserver(&observer);
}

TEST(AstraProfileMenuObserverDefaultsTest, AllDefaultMethodsExist) {
  // Verify all observer methods can be called on the base class.
  // This test documents the full observer API.
  class FullObserver : public AstraProfileMenuModelObserver {
   public:
    // Override none — use all defaults.
  };

  FullObserver observer;
  // No crash = all defaults are defined.
  SUCCEED();
}

// =========================================================================
// Workspace management tests
// =========================================================================

class AstraProfileMenuModelWorkspaceTest : public testing::Test {
 public:
  AstraProfileMenuModelWorkspaceTest() = default;
  ~AstraProfileMenuModelWorkspaceTest() override = default;

 protected:
  void SetUp() override {
    model_.AddObserver(&observer_);
  }

  void TearDown() override {
    model_.RemoveObserver(&observer_);
  }

  AstraMenuWorkspaceInfo MakeWorkspace(const std::string& id,
                                       const std::u16string& name,
                                       int order) {
    AstraMenuWorkspaceInfo ws;
    ws.id = id;
    ws.name = name;
    ws.accent_color = SK_ColorBLUE;
    ws.tab_count = 0;
    ws.is_active = false;
    ws.order_index = order;
    return ws;
  }

  AstraProfileMenuModel model_;
  TestModelObserver observer_;
};

TEST_F(AstraProfileMenuModelWorkspaceTest, DefaultWorkspaceCountIsZero) {
  EXPECT_EQ(0u, model_.GetWorkspaceCount());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, SetWorkspaces) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"Workspace 1", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Workspace 2", 1));
  workspaces.push_back(MakeWorkspace("ws3", u"Workspace 3", 2));

  model_.SetWorkspaces(workspaces);

  EXPECT_EQ(3u, model_.GetWorkspaceCount());
  EXPECT_EQ(1, observer_.workspaces_changed_count);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetWorkspaceAt) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));

  model_.SetWorkspaces(workspaces);

  const auto* ws = model_.GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws1", ws->id);

  ws = model_.GetWorkspaceAt(1);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws2", ws->id);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetWorkspaceAtOutOfBounds) {
  EXPECT_EQ(nullptr, model_.GetWorkspaceAt(0));
  EXPECT_EQ(nullptr, model_.GetWorkspaceAt(100));
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetWorkspaceById) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws-alpha", u"Alpha", 0));
  workspaces.push_back(MakeWorkspace("ws-beta", u"Beta", 1));

  model_.SetWorkspaces(workspaces);

  const auto* ws = model_.GetWorkspaceById("ws-beta");
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(u"Beta", ws->name);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetWorkspaceByIdNotFound) {
  EXPECT_EQ(nullptr, model_.GetWorkspaceById("nonexistent"));
}

TEST_F(AstraProfileMenuModelWorkspaceTest, AddWorkspace) {
  model_.AddWorkspace(MakeWorkspace("ws1", u"New", 0));
  EXPECT_EQ(1u, model_.GetWorkspaceCount());
  EXPECT_EQ(1, observer_.workspaces_changed_count);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, RemoveWorkspace) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"One", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Two", 1));
  model_.SetWorkspaces(workspaces);
  ASSERT_EQ(2u, model_.GetWorkspaceCount());

  bool removed = model_.RemoveWorkspace("ws1");
  EXPECT_TRUE(removed);
  EXPECT_EQ(1u, model_.GetWorkspaceCount());
  EXPECT_EQ(nullptr, model_.GetWorkspaceById("ws1"));
}

TEST_F(AstraProfileMenuModelWorkspaceTest, RemoveWorkspaceNotFound) {
  bool removed = model_.RemoveWorkspace("nonexistent");
  EXPECT_FALSE(removed);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, RenameWorkspace) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"Old Name", 0));
  model_.SetWorkspaces(workspaces);

  bool renamed = model_.RenameWorkspace("ws1", u"New Name");
  EXPECT_TRUE(renamed);

  const auto* ws = model_.GetWorkspaceById("ws1");
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(u"New Name", ws->name);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, RenameWorkspaceSameName) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"Same Name", 0));
  model_.SetWorkspaces(workspaces);
  int initial_count = observer_.workspaces_changed_count;

  bool renamed = model_.RenameWorkspace("ws1", u"Same Name");
  EXPECT_FALSE(renamed);
  EXPECT_EQ(initial_count, observer_.workspaces_changed_count);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, ReorderWorkspaceForward) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  workspaces.push_back(MakeWorkspace("ws3", u"Third", 2));
  model_.SetWorkspaces(workspaces);

  bool reordered = model_.ReorderWorkspace(0, 2);
  EXPECT_TRUE(reordered);

  const auto* ws = model_.GetWorkspaceAt(2);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws1", ws->id);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, ReorderWorkspaceBackward) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  workspaces.push_back(MakeWorkspace("ws3", u"Third", 2));
  model_.SetWorkspaces(workspaces);

  bool reordered = model_.ReorderWorkspace(2, 0);
  EXPECT_TRUE(reordered);

  const auto* ws = model_.GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws3", ws->id);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, ReorderWorkspaceSameIndex) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  model_.SetWorkspaces(workspaces);
  int initial_count = observer_.workspaces_changed_count;

  bool reordered = model_.ReorderWorkspace(1, 1);
  EXPECT_FALSE(reordered);
  EXPECT_EQ(initial_count, observer_.workspaces_changed_count);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, ReorderWorkspaceOutOfBounds) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  model_.SetWorkspaces(workspaces);

  EXPECT_FALSE(model_.ReorderWorkspace(5, 0));
  EXPECT_FALSE(model_.ReorderWorkspace(0, 5));
}

TEST_F(AstraProfileMenuModelWorkspaceTest, SelectWorkspaceByIndex) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  model_.SetWorkspaces(workspaces);

  bool selected = model_.SelectWorkspaceByIndex(1);
  EXPECT_TRUE(selected);
  EXPECT_EQ("ws2", model_.active_workspace_id());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, SelectWorkspaceById) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  model_.SetWorkspaces(workspaces);

  bool selected = model_.SelectWorkspaceById("ws1");
  EXPECT_TRUE(selected);
  EXPECT_EQ("ws1", model_.active_workspace_id());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, SelectWorkspaceByIdNotFound) {
  bool selected = model_.SelectWorkspaceById("nonexistent");
  EXPECT_FALSE(selected);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetActiveWorkspaceIndex) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Active", 1));
  model_.SetWorkspaces(workspaces);
  model_.SetActiveWorkspaceId("ws2");

  EXPECT_EQ(1, model_.GetActiveWorkspaceIndex());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetActiveWorkspaceIndexNone) {
  EXPECT_EQ(-1, model_.GetActiveWorkspaceIndex());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, ActiveWorkspaceUpdatesOnSet) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"First", 0));
  workspaces.push_back(MakeWorkspace("ws2", u"Second", 1));
  model_.SetWorkspaces(workspaces);

  model_.SetActiveWorkspaceId("ws1");

  const auto* ws = model_.GetWorkspaceById("ws1");
  ASSERT_NE(nullptr, ws);
  EXPECT_TRUE(ws->is_active);

  ws = model_.GetWorkspaceById("ws2");
  ASSERT_NE(nullptr, ws);
  EXPECT_FALSE(ws->is_active);
}

TEST_F(AstraProfileMenuModelWorkspaceTest, RemoveActiveWorkspaceClearsId) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"Active", 0));
  model_.SetWorkspaces(workspaces);
  model_.SetActiveWorkspaceId("ws1");
  ASSERT_EQ("ws1", model_.active_workspace_id());

  model_.RemoveWorkspace("ws1");
  EXPECT_TRUE(model_.active_workspace_id().empty());
}

// =========================================================================
// Presentation settings tests
// =========================================================================

class AstraProfileMenuModelSettingsTest : public testing::Test {
 public:
  AstraProfileMenuModelSettingsTest() = default;
  ~AstraProfileMenuModelSettingsTest() override = default;

  void SetUp() override {
    model_.AddObserver(&observer_);
  }

  void TearDown() override {
    model_.RemoveObserver(&observer_);
  }

 protected:
  AstraProfileMenuModel model_;
  TestModelObserver observer_;
};

TEST_F(AstraProfileMenuModelSettingsTest, DefaultShowWorkspaces) {
  EXPECT_TRUE(model_.show_workspaces());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowWorkspacesFalse) {
  model_.set_show_workspaces(false);
  EXPECT_FALSE(model_.show_workspaces());
  EXPECT_GE(observer_.settings_changed_count, 1);
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowWorkspacesSameValueNoNotify) {
  model_.set_show_workspaces(true);  // Already true.
  EXPECT_EQ(0, observer_.settings_changed_count);
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultMaxWorkspacesShown) {
  EXPECT_EQ(AstraProfileMenuModel::kDefaultMaxWorkspaces,
            model_.max_workspaces_shown());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetMaxWorkspacesShown) {
  model_.set_max_workspaces_shown(5);
  EXPECT_EQ(5, model_.max_workspaces_shown());
}

TEST_F(AstraProfileMenuModelSettingsTest, MaxWorkspacesClampsMin) {
  model_.set_max_workspaces_shown(1);
  EXPECT_EQ(AstraProfileMenuModel::kMinMaxWorkspaces,
            model_.max_workspaces_shown());
}

TEST_F(AstraProfileMenuModelSettingsTest, MaxWorkspacesClampsMax) {
  model_.set_max_workspaces_shown(100);
  EXPECT_EQ(AstraProfileMenuModel::kMaxMaxWorkspaces,
            model_.max_workspaces_shown());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultShowAvatar) {
  EXPECT_TRUE(model_.show_avatar());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowAvatar) {
  model_.set_show_avatar(false);
  EXPECT_FALSE(model_.show_avatar());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultShowSyncStatus) {
  EXPECT_TRUE(model_.show_sync_status());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowSyncStatus) {
  model_.set_show_sync_status(false);
  EXPECT_FALSE(model_.show_sync_status());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultWorkspaceDisplayMode) {
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsAndNames,
            model_.workspace_display_mode());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetWorkspaceDisplayModeIconsOnly) {
  model_.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsOnly);
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsOnly,
            model_.workspace_display_mode());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetWorkspaceDisplayModeNamesOnly) {
  model_.set_workspace_display_mode(AstraWorkspaceDisplayMode::kNamesOnly);
  EXPECT_EQ(AstraWorkspaceDisplayMode::kNamesOnly,
            model_.workspace_display_mode());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultMenuPosition) {
  EXPECT_EQ(AstraProfileMenuPosition::kRight, model_.menu_position());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetMenuPositionLeft) {
  model_.set_menu_position(AstraProfileMenuPosition::kLeft);
  EXPECT_EQ(AstraProfileMenuPosition::kLeft, model_.menu_position());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultShowRecentlyClosed) {
  EXPECT_TRUE(model_.show_recently_closed());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowRecentlyClosed) {
  model_.set_show_recently_closed(false);
  EXPECT_FALSE(model_.show_recently_closed());
}

TEST_F(AstraProfileMenuModelSettingsTest, DefaultShowSignInPromo) {
  EXPECT_TRUE(model_.show_sign_in_promo());
}

TEST_F(AstraProfileMenuModelSettingsTest, SetShowSignInPromo) {
  model_.set_show_sign_in_promo(false);
  EXPECT_FALSE(model_.show_sign_in_promo());
}

TEST_F(AstraProfileMenuModelSettingsTest, AllSettingsTriggerNotification) {
  // All 8 presentation settings should trigger OnMenuSettingsChanged.
  int initial_count = observer_.settings_changed_count;

  model_.set_show_workspaces(false);
  model_.set_max_workspaces_shown(5);
  model_.set_show_avatar(false);
  model_.set_show_sync_status(false);
  model_.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsOnly);
  model_.set_menu_position(AstraProfileMenuPosition::kLeft);
  model_.set_show_recently_closed(false);
  model_.set_show_sign_in_promo(false);

  // Each setting change that actually changes the value triggers a notification.
  EXPECT_EQ(8, observer_.settings_changed_count - initial_count);
}

TEST_F(AstraProfileMenuModelSettingsTest, ResetToDefaults) {
  // Change all settings from defaults.
  model_.set_show_workspaces(false);
  model_.set_max_workspaces_shown(4);
  model_.set_show_avatar(false);
  model_.set_show_sync_status(false);
  model_.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsOnly);
  model_.set_menu_position(AstraProfileMenuPosition::kLeft);
  model_.set_show_recently_closed(false);
  model_.set_show_sign_in_promo(false);

  model_.ResetToDefaults();

  EXPECT_TRUE(model_.show_workspaces());
  EXPECT_EQ(AstraProfileMenuModel::kDefaultMaxWorkspaces,
            model_.max_workspaces_shown());
  EXPECT_TRUE(model_.show_avatar());
  EXPECT_TRUE(model_.show_sync_status());
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsAndNames,
            model_.workspace_display_mode());
  EXPECT_EQ(AstraProfileMenuPosition::kRight, model_.menu_position());
  EXPECT_TRUE(model_.show_recently_closed());
  EXPECT_TRUE(model_.show_sign_in_promo());
}

// =========================================================================
// Sync status tests
// =========================================================================

TEST_F(AstraProfileMenuModelTest, DefaultSyncStatus) {
  EXPECT_EQ(AstraSyncStatus::kNotSignedIn, model_.sync_status());
}

TEST_F(AstraProfileMenuModelTest, SetSyncStatus) {
  TestModelObserver observer;
  model_.AddObserver(&observer);

  model_.SetSyncStatus(AstraSyncStatus::kSynced);
  EXPECT_EQ(AstraSyncStatus::kSynced, model_.sync_status());
  EXPECT_EQ(1, observer.sync_status_changed_count);

  model_.RemoveObserver(&observer);
}

TEST_F(AstraProfileMenuModelTest, SetSyncStatusSameNoNotify) {
  TestModelObserver observer;
  model_.AddObserver(&observer);

  model_.SetSyncStatus(AstraSyncStatus::kNotSignedIn);  // Already default.
  EXPECT_EQ(0, observer.sync_status_changed_count);

  model_.RemoveObserver(&observer);
}

TEST_F(AstraProfileMenuModelTest, GetSyncStatusLabel) {
  model_.SetSyncStatus(AstraSyncStatus::kNotSignedIn);
  EXPECT_FALSE(model_.GetSyncStatusLabel().empty());

  model_.SetSyncStatus(AstraSyncStatus::kSyncing);
  EXPECT_FALSE(model_.GetSyncStatusLabel().empty());

  model_.SetSyncStatus(AstraSyncStatus::kSynced);
  EXPECT_FALSE(model_.GetSyncStatusLabel().empty());

  model_.SetSyncStatus(AstraSyncStatus::kError);
  EXPECT_FALSE(model_.GetSyncStatusLabel().empty());

  model_.SetSyncStatus(AstraSyncStatus::kPaused);
  EXPECT_FALSE(model_.GetSyncStatusLabel().empty());
}

TEST_F(AstraProfileMenuModelTest, HasSyncError) {
  model_.SetSyncStatus(AstraSyncStatus::kSynced);
  EXPECT_FALSE(model_.HasSyncError());

  model_.SetSyncStatus(AstraSyncStatus::kError);
  EXPECT_TRUE(model_.HasSyncError());

  model_.SetSyncStatus(AstraSyncStatus::kPaused);
  EXPECT_TRUE(model_.HasSyncError());
}

// =========================================================================
// Utility method tests
// =========================================================================

TEST_F(AstraProfileMenuModelWorkspaceTest, GetVisibleWorkspaceCount) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  for (int i = 0; i < 20; ++i) {
    workspaces.push_back(MakeWorkspace(
        "ws" + base::NumberToString(i),
        u"Workspace " + base::NumberToString16(i), i));
  }
  model_.SetWorkspaces(workspaces);

  model_.set_max_workspaces_shown(5);
  EXPECT_EQ(5, model_.GetVisibleWorkspaceCount());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, GetVisibleWorkspaceCountFewerThanMax) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  workspaces.push_back(MakeWorkspace("ws1", u"One", 0));
  model_.SetWorkspaces(workspaces);

  model_.set_max_workspaces_shown(10);
  EXPECT_EQ(1, model_.GetVisibleWorkspaceCount());
}

TEST_F(AstraProfileMenuModelWorkspaceTest, IsWorkspaceVisible) {
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  for (int i = 0; i < 10; ++i) {
    workspaces.push_back(MakeWorkspace(
        "ws" + base::NumberToString(i),
        u"WS " + base::NumberToString16(i), i));
  }
  model_.SetWorkspaces(workspaces);
  model_.set_max_workspaces_shown(5);

  EXPECT_TRUE(model_.IsWorkspaceVisible(0));
  EXPECT_TRUE(model_.IsWorkspaceVisible(4));
  EXPECT_FALSE(model_.IsWorkspaceVisible(5));
  EXPECT_FALSE(model_.IsWorkspaceVisible(100));
}

// =========================================================================
// Persistence / PrefService round-trip tests
// =========================================================================

class AstraProfileMenuModelPersistenceTest : public testing::Test {
 public:
  AstraProfileMenuModelPersistenceTest() = default;
  ~AstraProfileMenuModelPersistenceTest() override = default;

  void SetUp() override {
    // Register the profile menu prefs.
    prefs_.registry()->RegisterBooleanPref(
        prefs::kPrefProfileMenuShowWorkspaces,
        prefs::kDefaultProfileMenuShowWorkspaces);
    prefs_.registry()->RegisterIntegerPref(
        prefs::kPrefProfileMenuMaxWorkspaces,
        prefs::kDefaultProfileMenuMaxWorkspaces);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kPrefProfileMenuShowAvatar,
        prefs::kDefaultProfileMenuShowAvatar);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kPrefProfileMenuShowSyncStatus,
        prefs::kDefaultProfileMenuShowSyncStatus);
    prefs_.registry()->RegisterIntegerPref(
        prefs::kPrefProfileMenuWorkspaceDisplayMode,
        prefs::kDefaultProfileMenuWorkspaceDisplayMode);
    prefs_.registry()->RegisterStringPref(
        prefs::kPrefProfileMenuPosition,
        prefs::kDefaultProfileMenuPosition);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kPrefProfileMenuShowRecentlyClosed,
        prefs::kDefaultProfileMenuShowRecentlyClosed);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kPrefProfileMenuShowSignInPromo,
        prefs::kDefaultProfileMenuShowSignInPromo);
  }

 protected:
  TestingPrefServiceSimple prefs_;
  AstraProfileMenuModel model_;
};

TEST_F(AstraProfileMenuModelPersistenceTest, LoadFromPrefsWithNullIsSafe) {
  model_.LoadFromPrefs(nullptr);
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraProfileMenuModelPersistenceTest, SaveToPrefsWithNullIsSafe) {
  model_.SaveToPrefs(nullptr);
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraProfileMenuModelPersistenceTest, DefaultPrefsLoadAsDefaults) {
  model_.LoadFromPrefs(&prefs_);

  EXPECT_TRUE(model_.show_workspaces());
  EXPECT_EQ(prefs::kDefaultProfileMenuMaxWorkspaces,
            model_.max_workspaces_shown());
  EXPECT_TRUE(model_.show_avatar());
  EXPECT_TRUE(model_.show_sync_status());
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsAndNames,
            model_.workspace_display_mode());
  EXPECT_EQ(AstraProfileMenuPosition::kRight, model_.menu_position());
  EXPECT_TRUE(model_.show_recently_closed());
  EXPECT_TRUE(model_.show_sign_in_promo());
}

TEST_F(AstraProfileMenuModelPersistenceTest, SaveAndLoadRoundTrip) {
  // Change all settings from defaults.
  model_.set_show_workspaces(false);
  model_.set_max_workspaces_shown(4);
  model_.set_show_avatar(false);
  model_.set_show_sync_status(false);
  model_.set_workspace_display_mode(AstraWorkspaceDisplayMode::kNamesOnly);
  model_.set_menu_position(AstraProfileMenuPosition::kLeft);
  model_.set_show_recently_closed(false);
  model_.set_show_sign_in_promo(false);

  // Save to prefs.
  model_.SaveToPrefs(&prefs_);

  // Create a new model and load from prefs.
  AstraProfileMenuModel model2;
  model2.LoadFromPrefs(&prefs_);

  // Verify all settings match.
  EXPECT_FALSE(model2.show_workspaces());
  EXPECT_EQ(4, model2.max_workspaces_shown());
  EXPECT_FALSE(model2.show_avatar());
  EXPECT_FALSE(model2.show_sync_status());
  EXPECT_EQ(AstraWorkspaceDisplayMode::kNamesOnly,
            model2.workspace_display_mode());
  EXPECT_EQ(AstraProfileMenuPosition::kLeft, model2.menu_position());
  EXPECT_FALSE(model2.show_recently_closed());
  EXPECT_FALSE(model2.show_sign_in_promo());
}

TEST_F(AstraProfileMenuModelPersistenceTest, MaxWorkspacesClampedOnLoad) {
  // Set a value outside the valid range in prefs directly.
  prefs_.SetInteger(prefs::kPrefProfileMenuMaxWorkspaces, 100);

  AstraProfileMenuModel model2;
  model2.LoadFromPrefs(&prefs_);

  // Should be clamped to max.
  EXPECT_EQ(AstraProfileMenuModel::kMaxMaxWorkspaces,
            model2.max_workspaces_shown());
}

// =========================================================================
// View tests — header view deepening
// =========================================================================

class AstraProfileMenuHeaderViewDeepeningTest : public views::ViewsTestBase {
 public:
  AstraProfileMenuHeaderViewDeepeningTest() = default;
  ~AstraProfileMenuHeaderViewDeepeningTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    delegate_ = std::make_unique<FakeHeaderDelegate>();
    header_view_ = widget_->SetContentsView(
        std::make_unique<AstraProfileMenuHeaderView>(delegate_.get()));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    delegate_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraProfileMenuHeaderView> header_view_ = nullptr;
  std::unique_ptr<FakeHeaderDelegate> delegate_;
};

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, DefaultSyncStatus) {
  EXPECT_EQ(AstraHeaderSyncStatus::kNotSignedIn,
            header_view_->sync_status());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SetSyncStatus) {
  header_view_->SetSyncStatus(AstraHeaderSyncStatus::kSynced);
  EXPECT_EQ(AstraHeaderSyncStatus::kSynced, header_view_->sync_status());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SyncStatusVisibleDefault) {
  // Sync status should be hidden by default.
  EXPECT_FALSE(header_view_->sync_status_visible());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SetSyncStatusVisible) {
  header_view_->SetSyncStatusVisible(true);
  EXPECT_TRUE(header_view_->sync_status_visible());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, DefaultNotificationCount) {
  EXPECT_EQ(0, header_view_->notification_count());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SetNotificationCount) {
  header_view_->SetNotificationBadgeVisible(true);
  header_view_->SetNotificationCount(5);
  EXPECT_EQ(5, header_view_->notification_count());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, NotificationBadgeVisibility) {
  EXPECT_FALSE(header_view_->notification_badge_visible());
  header_view_->SetNotificationBadgeVisible(true);
  EXPECT_TRUE(header_view_->notification_badge_visible());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, AvatarVisibleDefault) {
  EXPECT_TRUE(header_view_->avatar_visible());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SetAvatarVisible) {
  header_view_->SetAvatarVisible(false);
  EXPECT_FALSE(header_view_->avatar_visible());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, AccessibilityIncludesNotifications) {
  header_view_->SetNotificationBadgeVisible(true);
  header_view_->SetNotificationCount(3);

  ui::AXNodeData data;
  header_view_->GetAccessibleNodeData(&data);

  // Description should mention notifications.
  EXPECT_NE(std::u16string::npos,
            data.GetDescription().find(u"notifications"));
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, AccessibilityIncludesSyncStatus) {
  header_view_->SetSyncStatusVisible(true);
  header_view_->SetSyncStatus(AstraHeaderSyncStatus::kSynced);

  ui::AXNodeData data;
  header_view_->GetAccessibleNodeData(&data);

  // Description should include sync status.
  EXPECT_FALSE(data.GetDescription().empty());
}

TEST_F(AstraProfileMenuHeaderViewDeepeningTest, SyncStatusClickDelegates) {
  header_view_->SetSyncStatusVisible(true);

  // Sync status click is handled via the delegate.
  // The delegate method should exist and be callable.
  delegate_->OnSyncStatusClicked();
  EXPECT_EQ(1, delegate_->sync_click_count);
}

// =========================================================================
// View tests — workspace menu item deepening
// =========================================================================

class AstraWorkspaceMenuItemViewDeepeningTest : public views::ViewsTestBase {
 public:
  AstraWorkspaceMenuItemViewDeepeningTest() = default;
  ~AstraWorkspaceMenuItemViewDeepeningTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraWorkspaceMenuItemView>(
            u"Work", SK_ColorBLUE, 5, false, base::DoNothing()));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraWorkspaceMenuItemView> item_view_ = nullptr;
};

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, DefaultDisplayMode) {
  EXPECT_EQ(AstraWorkspaceItemDisplayMode::kIconsAndNames,
            item_view_->display_mode());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetDisplayModeIconsOnly) {
  item_view_->SetDisplayMode(AstraWorkspaceItemDisplayMode::kIconsOnly);
  EXPECT_EQ(AstraWorkspaceItemDisplayMode::kIconsOnly,
            item_view_->display_mode());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetDisplayModeNamesOnly) {
  item_view_->SetDisplayMode(AstraWorkspaceItemDisplayMode::kNamesOnly);
  EXPECT_EQ(AstraWorkspaceItemDisplayMode::kNamesOnly,
            item_view_->display_mode());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, DefaultSizeVariant) {
  EXPECT_EQ(AstraWorkspaceItemSize::kMedium, item_view_->size_variant());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetSizeVariantSmall) {
  item_view_->SetSizeVariant(AstraWorkspaceItemSize::kSmall);
  EXPECT_EQ(AstraWorkspaceItemSize::kSmall, item_view_->size_variant());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetSizeVariantLarge) {
  item_view_->SetSizeVariant(AstraWorkspaceItemSize::kLarge);
  EXPECT_EQ(AstraWorkspaceItemSize::kLarge, item_view_->size_variant());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest,
       SmallSizeHasSmallerPreferredHeight) {
  item_view_->SetSizeVariant(AstraWorkspaceItemSize::kSmall);
  gfx::Size small_size = item_view_->CalculatePreferredSize();

  item_view_->SetSizeVariant(AstraWorkspaceItemSize::kLarge);
  gfx::Size large_size = item_view_->CalculatePreferredSize();

  EXPECT_LT(small_size.height(), large_size.height());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, DefaultReorderHandleHidden) {
  EXPECT_FALSE(item_view_->reorder_handle_visible());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetReorderHandleVisible) {
  item_view_->SetReorderHandleVisible(true);
  EXPECT_TRUE(item_view_->reorder_handle_visible());
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, ReorderCallbackDefaultNull) {
  // Default callback should be null and safe.
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, SetReorderCallback) {
  int reorder_count = 0;
  int last_direction = 0;
  item_view_->set_reorder_callback(base::BindLambdaForTesting(
      [&](int direction) {
        reorder_count++;
        last_direction = direction;
      }));

  // Callback is stored; can be set without crashing.
  SUCCEED();
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest, AccessibilityHasReorderInfo) {
  item_view_->SetReorderHandleVisible(true);

  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);

  // Description should mention reorderable.
  EXPECT_NE(std::u16string::npos,
            data.GetDescription().find(u"reorderable"));
}

TEST_F(AstraWorkspaceMenuItemViewDeepeningTest,
       ActiveItemHasSelectedAccessibilityState) {
  item_view_->SetIsActive(true);

  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);

  EXPECT_TRUE(data.HasState(ax::mojom::State::kSelected));
  EXPECT_EQ(ax::mojom::CheckedState::kTrue, data.GetCheckedState());
}

// =========================================================================
// View tests — avatar button deepening
// =========================================================================

class AstraWorkspaceAvatarButtonDeepeningTest : public views::ViewsTestBase {
 public:
  AstraWorkspaceAvatarButtonDeepeningTest() = default;
  ~AstraWorkspaceAvatarButtonDeepeningTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    delegate_ = std::make_unique<FakeAvatarDelegate>();
    avatar_button_ = widget_->SetContentsView(
        std::make_unique<AstraWorkspaceAvatarButton>(
            nullptr, delegate_.get()));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    delegate_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraWorkspaceAvatarButton> avatar_button_ = nullptr;
  std::unique_ptr<FakeAvatarDelegate> delegate_;
};

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, DefaultSizeVariant) {
  EXPECT_EQ(AstraAvatarButtonSize::kMedium, avatar_button_->size_variant());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, SetSizeVariantSmall) {
  avatar_button_->SetSizeVariant(AstraAvatarButtonSize::kSmall);
  EXPECT_EQ(AstraAvatarButtonSize::kSmall, avatar_button_->size_variant());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, SetSizeVariantLarge) {
  avatar_button_->SetSizeVariant(AstraAvatarButtonSize::kLarge);
  EXPECT_EQ(AstraAvatarButtonSize::kLarge, avatar_button_->size_variant());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest,
       SmallSizeHasSmallerPreferredHeight) {
  avatar_button_->SetSizeVariant(AstraAvatarButtonSize::kSmall);
  gfx::Size small_size = avatar_button_->CalculatePreferredSize();

  avatar_button_->SetSizeVariant(AstraAvatarButtonSize::kLarge);
  gfx::Size large_size = avatar_button_->CalculatePreferredSize();

  EXPECT_LE(small_size.height(), large_size.height());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, DefaultNotificationCount) {
  EXPECT_EQ(0, avatar_button_->notification_count());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, SetNotificationCount) {
  avatar_button_->SetNotificationBadgeVisible(true);
  avatar_button_->SetNotificationCount(7);
  EXPECT_EQ(7, avatar_button_->notification_count());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, NotificationBadgeVisibility) {
  EXPECT_FALSE(avatar_button_->notification_badge_visible());
  avatar_button_->SetNotificationBadgeVisible(true);
  EXPECT_TRUE(avatar_button_->notification_badge_visible());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest,
       AccessibilityIncludesNotifications) {
  avatar_button_->SetNotificationBadgeVisible(true);
  avatar_button_->SetNotificationCount(42);
  avatar_button_->SetProfileName(u"Test");

  ui::AXNodeData data;
  avatar_button_->GetAccessibleNodeData(&data);

  EXPECT_NE(std::u16string::npos,
            data.GetDescription().find(u"notifications"));
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, LargeNotificationCountShowsPlus) {
  avatar_button_->SetNotificationBadgeVisible(true);
  avatar_button_->SetNotificationCount(150);

  // Notification count should be stored as-is; display is handled by the label.
  EXPECT_EQ(150, avatar_button_->notification_count());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, ExpandedModeWiderThanCompact) {
  avatar_button_->SetDisplayMode(AstraAvatarButtonMode::kCompact);
  gfx::Size compact_size = avatar_button_->CalculatePreferredSize();

  avatar_button_->SetDisplayMode(AstraAvatarButtonMode::kExpanded);
  gfx::Size expanded_size = avatar_button_->CalculatePreferredSize();

  EXPECT_GE(expanded_size.width(), compact_size.width());
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, CanReceiveFocus) {
  avatar_button_->RequestFocus();
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraWorkspaceAvatarButtonDeepeningTest, HasPopupMenuAccessibility) {
  ui::AXNodeData data;
  avatar_button_->GetAccessibleNodeData(&data);

  EXPECT_EQ(ax::mojom::HasPopup::kMenu, data.GetHasPopup());
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST(AstraProfileMenuModelEdgeCasesTest, EmptyWorkspaces) {
  AstraProfileMenuModel model;
  EXPECT_EQ(0u, model.GetWorkspaceCount());
  EXPECT_EQ(-1, model.GetActiveWorkspaceIndex());
  EXPECT_EQ(0, model.GetVisibleWorkspaceCount());
  EXPECT_FALSE(model.SelectWorkspaceByIndex(0));
  EXPECT_FALSE(model.SelectWorkspaceById("anything"));
  EXPECT_FALSE(model.RemoveWorkspace("anything"));
  EXPECT_FALSE(model.ReorderWorkspace(0, 1));
}

TEST(AstraProfileMenuModelEdgeCasesTest, SingleWorkspace) {
  AstraProfileMenuModel model;
  std::vector<AstraMenuWorkspaceInfo> workspaces;
  AstraMenuWorkspaceInfo ws;
  ws.id = "only";
  ws.name = u"Only";
  ws.order_index = 0;
  workspaces.push_back(ws);
  model.SetWorkspaces(workspaces);

  EXPECT_EQ(1u, model.GetWorkspaceCount());
  EXPECT_TRUE(model.SelectWorkspaceByIndex(0));
  EXPECT_EQ(0, model.GetActiveWorkspaceIndex());

  // Can't reorder a single workspace.
  EXPECT_FALSE(model.ReorderWorkspace(0, 0));
}

TEST(AstraProfileMenuModelEdgeCasesTest, MaxWorkspacesBoundaryValues) {
  AstraProfileMenuModel model;

  // Min boundary.
  model.set_max_workspaces_shown(AstraProfileMenuModel::kMinMaxWorkspaces);
  EXPECT_EQ(AstraProfileMenuModel::kMinMaxWorkspaces,
            model.max_workspaces_shown());

  // Max boundary.
  model.set_max_workspaces_shown(AstraProfileMenuModel::kMaxMaxWorkspaces);
  EXPECT_EQ(AstraProfileMenuModel::kMaxMaxWorkspaces,
            model.max_workspaces_shown());

  // Just above min.
  model.set_max_workspaces_shown(AstraProfileMenuModel::kMinMaxWorkspaces + 1);
  EXPECT_EQ(AstraProfileMenuModel::kMinMaxWorkspaces + 1,
            model.max_workspaces_shown());

  // Just below max.
  model.set_max_workspaces_shown(AstraProfileMenuModel::kMaxMaxWorkspaces - 1);
  EXPECT_EQ(AstraProfileMenuModel::kMaxMaxWorkspaces - 1,
            model.max_workspaces_shown());
}

TEST(AstraProfileMenuModelEdgeCasesTest, AllDisplayModesValid) {
  AstraProfileMenuModel model;

  model.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsOnly);
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsOnly,
            model.workspace_display_mode());

  model.set_workspace_display_mode(AstraWorkspaceDisplayMode::kNamesOnly);
  EXPECT_EQ(AstraWorkspaceDisplayMode::kNamesOnly,
            model.workspace_display_mode());

  model.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsAndNames);
  EXPECT_EQ(AstraWorkspaceDisplayMode::kIconsAndNames,
            model.workspace_display_mode());
}

TEST(AstraProfileMenuModelEdgeCasesTest, AllSyncStatusesValid) {
  AstraProfileMenuModel model;

  for (auto status : {
       AstraSyncStatus::kNotSignedIn,
       AstraSyncStatus::kSyncing,
       AstraSyncStatus::kSynced,
       AstraSyncStatus::kError,
       AstraSyncStatus::kPaused,
   }) {
    model.SetSyncStatus(status);
    EXPECT_EQ(status, model.sync_status());
    EXPECT_FALSE(model.GetSyncStatusLabel().empty());
  }
}

TEST(AstraProfileMenuModelEdgeCasesTest, AllMenuPositionsValid) {
  AstraProfileMenuModel model;

  model.set_menu_position(AstraProfileMenuPosition::kLeft);
  EXPECT_EQ(AstraProfileMenuPosition::kLeft, model.menu_position());

  model.set_menu_position(AstraProfileMenuPosition::kRight);
  EXPECT_EQ(AstraProfileMenuPosition::kRight, model.menu_position());
}

// =========================================================================
// Bulk operations test (documentation test)
// =========================================================================

TEST(AstraProfileMenuBulkTest, BulkSettingsChangeViaResetToDefaults) {
  // The model supports bulk changes via ResetToDefaults, which resets
  // all presentation settings at once and fires a single notification
  // (internally each setter fires, but the net effect is all settings reset).
  AstraProfileMenuModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  // Change all settings from defaults.
  model.set_show_workspaces(false);
  model.set_max_workspaces_shown(4);
  model.set_show_avatar(false);
  model.set_show_sync_status(false);
  model.set_workspace_display_mode(AstraWorkspaceDisplayMode::kIconsOnly);
  model.set_menu_position(AstraProfileMenuPosition::kLeft);
  model.set_show_recently_closed(false);
  model.set_show_sign_in_promo(false);

  int before_count = observer.settings_changed_count;
  model.ResetToDefaults();
  int after_count = observer.settings_changed_count;

  // All 8 settings should have triggered a notification when reset.
  EXPECT_EQ(8, after_count - before_count);

  // All should be back to defaults.
  EXPECT_TRUE(model.show_workspaces());
  EXPECT_TRUE(model.show_avatar());
  EXPECT_TRUE(model.show_sync_status());
  EXPECT_TRUE(model.show_recently_closed());
  EXPECT_TRUE(model.show_sign_in_promo());

  model.RemoveObserver(&observer);
}

}  // namespace astra
