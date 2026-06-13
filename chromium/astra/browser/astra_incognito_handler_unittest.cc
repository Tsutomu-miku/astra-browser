// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_incognito_handler.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestIncognitoObserver : public AstraIncognitoObserver {
 public:
  // Window events
  void OnIncognitoWindowCreated() override {
    window_created_count_++;
  }

  void OnIncognitoWindowClosed() override {
    window_closed_count_++;
  }

  void OnAllIncognitoWindowsClosed() override {
    all_windows_closed_count_++;
  }

  // Setting changes
  void OnShowSidebarBadgeChanged(bool enabled) override {
    show_sidebar_badge_changed_count_++;
    last_show_sidebar_badge_ = enabled;
  }

  void OnConfirmCloseAllChanged(bool enabled) override {
    confirm_close_all_changed_count_++;
    last_confirm_close_all_ = enabled;
  }

  void OnWarnOnExternalOpenChanged(bool enabled) override {
    warn_on_external_open_changed_count_++;
    last_warn_on_external_open_ = enabled;
  }

  void OnDefaultWorkspaceChanged(const std::string& workspace_id) override {
    default_workspace_changed_count_++;
    last_default_workspace_ = workspace_id;
  }

  // Catch-all
  void OnIncognitoSettingsChanged() override {
    settings_changed_count_++;
  }

  // Counters
  int window_created_count_ = 0;
  int window_closed_count_ = 0;
  int all_windows_closed_count_ = 0;
  int show_sidebar_badge_changed_count_ = 0;
  int confirm_close_all_changed_count_ = 0;
  int warn_on_external_open_changed_count_ = 0;
  int default_workspace_changed_count_ = 0;
  int settings_changed_count_ = 0;

  // Last recorded values
  bool last_show_sidebar_badge_ = false;
  bool last_confirm_close_all_ = false;
  bool last_warn_on_external_open_ = false;
  std::string last_default_workspace_;
};

}  // namespace

// Test fixture for AstraIncognitoHandler tests.
class IncognitoHandlerTest : public testing::Test {
 protected:
  IncognitoHandlerTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register prefs so the service can read them.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    service_ = std::make_unique<AstraIncognitoHandler>(profile_.get());
    DCHECK(service_);
  }

  ~IncognitoHandlerTest() override = default;

  void SetUp() override {
    // Service should start with default settings.
    ASSERT_TRUE(service_->ShouldShowSidebarBadge());
    ASSERT_FALSE(service_->ShouldConfirmCloseAll());
    ASSERT_FALSE(service_->ShouldWarnOnExternalOpen());
    ASSERT_EQ(service_->GetDefaultWorkspaceId(),
              prefs::kDefaultIncognitoDefaultWorkspace);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraIncognitoHandler> service_;
  std::vector<TestIncognitoObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, DefaultState_ShowSidebarBadgeEnabled) {
  EXPECT_TRUE(service_->ShouldShowSidebarBadge());
}

TEST_F(IncognitoHandlerTest, DefaultState_ConfirmCloseAllDisabled) {
  EXPECT_FALSE(service_->ShouldConfirmCloseAll());
}

TEST_F(IncognitoHandlerTest, DefaultState_WarnOnExternalOpenDisabled) {
  EXPECT_FALSE(service_->ShouldWarnOnExternalOpen());
}

TEST_F(IncognitoHandlerTest, DefaultState_DefaultWorkspaceIsDefault) {
  EXPECT_EQ(service_->GetDefaultWorkspaceId(), "default");
}

TEST_F(IncognitoHandlerTest, DefaultState_NoActiveSession) {
  EXPECT_FALSE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, DefaultState_ZeroWindowCount) {
  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
}

TEST_F(IncognitoHandlerTest, DefaultState_ZeroTabCount) {
  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
}

// ---------------------------------------------------------------------------
// Static utilities — IsIncognitoProfile
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, Static_IsIncognitoProfile_NullReturnsFalse) {
  EXPECT_FALSE(AstraIncognitoHandler::IsIncognitoProfile(nullptr));
}

TEST_F(IncognitoHandlerTest, Static_IsIncognitoProfile_RegularProfile) {
  // TestingProfile is a regular (non-OTR) profile.
  EXPECT_FALSE(AstraIncognitoHandler::IsIncognitoProfile(profile_.get()));
}

TEST_F(IncognitoHandlerTest, Static_IsIncognitoWebContents_NullReturnsFalse) {
  EXPECT_FALSE(AstraIncognitoHandler::IsIncognitoWebContents(nullptr));
}

// ---------------------------------------------------------------------------
// Projected Chromium state
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, ShouldAutoDeleteHistoryOnExit_FalseForRegular) {
  // Regular profile — history is not auto-deleted by default.
  EXPECT_FALSE(service_->ShouldAutoDeleteHistoryOnExit());
}

TEST_F(IncognitoHandlerTest, ShouldBlockThirdPartyCookies_FalseForRegular) {
  // Regular profile — third-party cookies not blocked by default.
  EXPECT_FALSE(service_->ShouldBlockThirdPartyCookies());
}

TEST_F(IncognitoHandlerTest, IsIncognitoModeAvailable_TrueByDefault) {
  EXPECT_TRUE(service_->IsIncognitoModeAvailable());
}

// ---------------------------------------------------------------------------
// Static utilities — Split view / Glance
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, Static_IsSplitViewAllowed_AlwaysTrue) {
  EXPECT_TRUE(AstraIncognitoHandler::IsSplitViewAllowed(profile_.get()));
  EXPECT_TRUE(AstraIncognitoHandler::IsSplitViewAllowed(nullptr));
}

TEST_F(IncognitoHandlerTest, Static_IsGlanceAllowed_AlwaysTrue) {
  EXPECT_TRUE(AstraIncognitoHandler::IsGlanceAllowed(profile_.get()));
  EXPECT_TRUE(AstraIncognitoHandler::IsGlanceAllowed(nullptr));
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraIncognitoObserver {};

  DefaultObserver observer;
  service_->AddObserver(&observer);

  // Trigger all observer paths.
  service_->SetShowSidebarBadge(false);
  service_->SetConfirmCloseAll(true);
  service_->SetWarnOnExternalOpen(true);
  service_->SetDefaultWorkspaceId("work");
  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowClosed();
  service_->CloseAllIncognitoWindows();

  service_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, AddRemoveObserver_NoCrash) {
  TestIncognitoObserver observer;

  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(IncognitoHandlerTest, RemoveNonexistentObserver_NoCrash) {
  TestIncognitoObserver observer;

  service_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// Show sidebar badge setting
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, SetShowSidebarBadge_ChangesValue) {
  ASSERT_TRUE(service_->ShouldShowSidebarBadge());

  service_->SetShowSidebarBadge(false);
  EXPECT_FALSE(service_->ShouldShowSidebarBadge());

  service_->SetShowSidebarBadge(true);
  EXPECT_TRUE(service_->ShouldShowSidebarBadge());
}

TEST_F(IncognitoHandlerTest, SetShowSidebarBadge_SameValueNoOp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  ASSERT_TRUE(service_->ShouldShowSidebarBadge());
  service_->SetShowSidebarBadge(true);

  EXPECT_EQ(observer.show_sidebar_badge_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, SetShowSidebarBadge_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetShowSidebarBadge(false);

  EXPECT_EQ(observer.show_sidebar_badge_changed_count_, 1);
  EXPECT_FALSE(observer.last_show_sidebar_badge_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Confirm close all setting
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, SetConfirmCloseAll_ChangesValue) {
  ASSERT_FALSE(service_->ShouldConfirmCloseAll());

  service_->SetConfirmCloseAll(true);
  EXPECT_TRUE(service_->ShouldConfirmCloseAll());

  service_->SetConfirmCloseAll(false);
  EXPECT_FALSE(service_->ShouldConfirmCloseAll());
}

TEST_F(IncognitoHandlerTest, SetConfirmCloseAll_SameValueNoOp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->ShouldConfirmCloseAll());
  service_->SetConfirmCloseAll(false);

  EXPECT_EQ(observer.confirm_close_all_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, SetConfirmCloseAll_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetConfirmCloseAll(true);

  EXPECT_EQ(observer.confirm_close_all_changed_count_, 1);
  EXPECT_TRUE(observer.last_confirm_close_all_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Warn on external open setting
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, SetWarnOnExternalOpen_ChangesValue) {
  ASSERT_FALSE(service_->ShouldWarnOnExternalOpen());

  service_->SetWarnOnExternalOpen(true);
  EXPECT_TRUE(service_->ShouldWarnOnExternalOpen());

  service_->SetWarnOnExternalOpen(false);
  EXPECT_FALSE(service_->ShouldWarnOnExternalOpen());
}

TEST_F(IncognitoHandlerTest, SetWarnOnExternalOpen_SameValueNoOp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->ShouldWarnOnExternalOpen());
  service_->SetWarnOnExternalOpen(false);

  EXPECT_EQ(observer.warn_on_external_open_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, SetWarnOnExternalOpen_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetWarnOnExternalOpen(true);

  EXPECT_EQ(observer.warn_on_external_open_changed_count_, 1);
  EXPECT_TRUE(observer.last_warn_on_external_open_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Default workspace setting
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, SetDefaultWorkspaceId_ChangesValue) {
  ASSERT_EQ(service_->GetDefaultWorkspaceId(), "default");

  service_->SetDefaultWorkspaceId("work");
  EXPECT_EQ(service_->GetDefaultWorkspaceId(), "work");

  service_->SetDefaultWorkspaceId("personal");
  EXPECT_EQ(service_->GetDefaultWorkspaceId(), "personal");
}

TEST_F(IncognitoHandlerTest, SetDefaultWorkspaceId_SameValueNoOp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  ASSERT_EQ(service_->GetDefaultWorkspaceId(), "default");
  service_->SetDefaultWorkspaceId("default");

  EXPECT_EQ(observer.default_workspace_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, SetDefaultWorkspaceId_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetDefaultWorkspaceId("work");

  EXPECT_EQ(observer.default_workspace_changed_count_, 1);
  EXPECT_EQ(observer.last_default_workspace_, "work");
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, SetDefaultWorkspaceId_EmptyString) {
  service_->SetDefaultWorkspaceId("");
  EXPECT_EQ(service_->GetDefaultWorkspaceId(), "");
}

// ---------------------------------------------------------------------------
// Session tracking — window creation
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_IncrementsCount) {
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 0);
  ASSERT_FALSE(service_->HasActiveIncognitoSession());

  service_->NotifyIncognitoWindowCreated();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 1);
  EXPECT_TRUE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_MultipleWindows) {
  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 3);
  EXPECT_TRUE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_DefaultTabCount) {
  service_->NotifyIncognitoWindowCreated();
  // Default tab count is 1 per window.
  EXPECT_EQ(service_->GetIncognitoTabCount(), 1);
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_CustomTabCount) {
  service_->NotifyIncognitoWindowCreated(5);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 5);
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_MultipleWindowsWithTabs) {
  service_->NotifyIncognitoWindowCreated(3);
  service_->NotifyIncognitoWindowCreated(5);
  service_->NotifyIncognitoWindowCreated(2);

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 3);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 10);  // 3 + 5 + 2
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_ZeroTabCountClampsToOne) {
  service_->NotifyIncognitoWindowCreated(0);
  // Minimum tab count is 1 per window.
  EXPECT_EQ(service_->GetIncognitoTabCount(), 1);
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_NegativeTabCountClampsToOne) {
  service_->NotifyIncognitoWindowCreated(-5);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 1);
}

TEST_F(IncognitoHandlerTest, NotifyWindowCreated_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();

  EXPECT_EQ(observer.window_created_count_, 1);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Session tracking — window closure
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_DecrementsCount) {
  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 1);

  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
  EXPECT_FALSE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_Multiple) {
  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 3);

  service_->NotifyIncognitoWindowClosed();
  EXPECT_EQ(service_->GetIncognitoWindowCount(), 2);

  service_->NotifyIncognitoWindowClosed();
  EXPECT_EQ(service_->GetIncognitoWindowCount(), 1);
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_ZeroCountNoOp) {
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 0);

  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
  EXPECT_FALSE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_ReducesTabCount) {
  service_->NotifyIncognitoWindowCreated(5);
  ASSERT_EQ(service_->GetIncognitoTabCount(), 5);

  service_->NotifyIncognitoWindowClosed(3);

  EXPECT_EQ(service_->GetIncognitoTabCount(), 2);
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_TabCountDoesNotGoNegative) {
  service_->NotifyIncognitoWindowCreated(2);
  ASSERT_EQ(service_->GetIncognitoTabCount(), 2);

  // Close with more tabs than exist.
  service_->NotifyIncognitoWindowClosed(10);

  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_AllClosedResetsTabCount) {
  service_->NotifyIncognitoWindowCreated(3);
  service_->NotifyIncognitoWindowCreated(5);
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 2);
  ASSERT_EQ(service_->GetIncognitoTabCount(), 8);

  // Close both windows.
  service_->NotifyIncognitoWindowClosed(3);
  service_->NotifyIncognitoWindowClosed(5);

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
  EXPECT_FALSE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, NotifyWindowClosed_FiresObservers) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(observer.window_closed_count_, 0);

  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(observer.window_closed_count_, 1);
  EXPECT_GE(observer.settings_changed_count_, 2);  // create + close

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// All windows closed event
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, AllWindowsClosed_FiresOnLastWindowClose) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(observer.all_windows_closed_count_, 0);

  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(observer.all_windows_closed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, AllWindowsClosed_NotFiredWhenWindowsRemain) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(observer.all_windows_closed_count_, 0);

  service_->NotifyIncognitoWindowClosed();

  // Still one window left — should not fire all-closed.
  EXPECT_EQ(observer.all_windows_closed_count_, 0);
  EXPECT_TRUE(service_->HasActiveIncognitoSession());

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, AllWindowsClosed_FiresOnSecondClose) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  service_->NotifyIncognitoWindowCreated();

  service_->NotifyIncognitoWindowClosed();
  ASSERT_EQ(observer.all_windows_closed_count_, 0);

  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(observer.all_windows_closed_count_, 1);
  EXPECT_FALSE(service_->HasActiveIncognitoSession());

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, AllWindowsClosed_NotFiredFromZero) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  // Close from zero — should not fire all-closed.
  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(observer.all_windows_closed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// CloseAllIncognitoWindows
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, CloseAllIncognitoWindows_NoWindowsNoOp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->CloseAllIncognitoWindows();

  EXPECT_EQ(observer.all_windows_closed_count_, 0);
  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, CloseAllIncognitoWindows_ClosesAllWindows) {
  service_->NotifyIncognitoWindowCreated(5);
  service_->NotifyIncognitoWindowCreated(3);
  service_->NotifyIncognitoWindowCreated(2);
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 3);
  ASSERT_EQ(service_->GetIncognitoTabCount(), 10);

  service_->CloseAllIncognitoWindows();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
  EXPECT_FALSE(service_->HasActiveIncognitoSession());
}

TEST_F(IncognitoHandlerTest, CloseAllIncognitoWindows_FiresAllClosedEvent) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  ASSERT_EQ(observer.all_windows_closed_count_, 0);

  service_->CloseAllIncognitoWindows();

  EXPECT_EQ(observer.all_windows_closed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(IncognitoHandlerTest, CloseAllIncognitoWindows_FiresSettingsChanged) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->NotifyIncognitoWindowCreated();
  int settings_count_before = observer.settings_changed_count_;

  service_->CloseAllIncognitoWindows();

  EXPECT_GT(observer.settings_changed_count_, settings_count_before);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Workspace behavior (regular profile)
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, WorkspaceMutations_AllowedForRegularProfile) {
  // TestingProfile is a regular profile, not incognito.
  EXPECT_TRUE(service_->AreWorkspaceMutationsAllowed());
}

TEST_F(IncognitoHandlerTest, WorkspaceActivation_AffectsServiceForRegularProfile) {
  EXPECT_TRUE(service_->DoesWorkspaceActivationAffectService());
}

// ---------------------------------------------------------------------------
// Favorite behavior (regular profile)
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, FavoritesMutable_ForRegularProfile) {
  EXPECT_TRUE(service_->AreFavoritesMutable());
}

TEST_F(IncognitoHandlerTest, FavoriteFoldersMutable_ForRegularProfile) {
  EXPECT_TRUE(service_->AreFavoriteFoldersMutable());
}

TEST_F(IncognitoHandlerTest, DefaultFavoriteState_FalseForRegularProfile) {
  EXPECT_FALSE(service_->DefaultFavoriteStateForProfile());
}

// ---------------------------------------------------------------------------
// Sidebar indicator (regular profile)
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, ShouldShowIncognitoIndicator_FalseForRegularProfile) {
  // Regular profile with badge enabled — still no indicator since not incognito.
  ASSERT_TRUE(service_->ShouldShowSidebarBadge());
  EXPECT_FALSE(service_->ShouldShowIncognitoIndicator());
}

TEST_F(IncognitoHandlerTest, ShouldShowIncognitoIndicator_BadgeDisabledRegular) {
  service_->SetShowSidebarBadge(false);
  EXPECT_FALSE(service_->ShouldShowIncognitoIndicator());
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, MultipleObservers_AllNotified) {
  TestIncognitoObserver observer1;
  TestIncognitoObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->SetShowSidebarBadge(false);

  EXPECT_EQ(observer1.show_sidebar_badge_changed_count_, 1);
  EXPECT_EQ(observer2.show_sidebar_badge_changed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(IncognitoHandlerTest, RemoveObserver_StopsNotifications) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetConfirmCloseAll(true);
  EXPECT_EQ(observer.confirm_close_all_changed_count_, 1);

  service_->RemoveObserver(&observer);

  service_->SetConfirmCloseAll(false);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.confirm_close_all_changed_count_, 1);
}

TEST_F(IncognitoHandlerTest, MultipleObservers_WindowEvents) {
  TestIncognitoObserver observer1;
  TestIncognitoObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->NotifyIncognitoWindowCreated();

  EXPECT_EQ(observer1.window_created_count_, 1);
  EXPECT_EQ(observer2.window_created_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, Shutdown_CleansUp) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, pref-based operations should not notify observers.
  service_->SetShowSidebarBadge(false);
  // Note: session tracking may still work since it doesn't depend on prefs.
  // The important thing is no crash.
  SUCCEED() << "Shutdown completed without crash.";
}

TEST_F(IncognitoHandlerTest, Shutdown_MultipleCallsSafe) {
  service_->Shutdown();
  service_->Shutdown();
  SUCCEED() << "Multiple Shutdown() calls do not crash.";
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, PrefsPersist_ShowSidebarBadge) {
  service_->SetShowSidebarBadge(false);

  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());
  EXPECT_FALSE(service2->ShouldShowSidebarBadge());
}

TEST_F(IncognitoHandlerTest, PrefsPersist_ConfirmCloseAll) {
  service_->SetConfirmCloseAll(true);

  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());
  EXPECT_TRUE(service2->ShouldConfirmCloseAll());
}

TEST_F(IncognitoHandlerTest, PrefsPersist_WarnOnExternalOpen) {
  service_->SetWarnOnExternalOpen(true);

  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());
  EXPECT_TRUE(service2->ShouldWarnOnExternalOpen());
}

TEST_F(IncognitoHandlerTest, PrefsPersist_DefaultWorkspace) {
  service_->SetDefaultWorkspaceId("work");

  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());
  EXPECT_EQ(service2->GetDefaultWorkspaceId(), "work");
}

TEST_F(IncognitoHandlerTest, PrefsPersist_DefaultValues) {
  // Create a fresh service — should have default values.
  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());

  EXPECT_TRUE(service2->ShouldShowSidebarBadge());
  EXPECT_FALSE(service2->ShouldConfirmCloseAll());
  EXPECT_FALSE(service2->ShouldWarnOnExternalOpen());
  EXPECT_EQ(service2->GetDefaultWorkspaceId(), "default");
}

TEST_F(IncognitoHandlerTest, PrefsPersist_SessionStateNotPersisted) {
  // Session tracking (window count, tab count) is runtime state, not
  // persisted to prefs.  A new service should start at zero.
  service_->NotifyIncognitoWindowCreated(5);
  ASSERT_EQ(service_->GetIncognitoWindowCount(), 1);
  ASSERT_EQ(service_->GetIncognitoTabCount(), 5);

  auto service2 = std::make_unique<AstraIncognitoHandler>(profile_.get());
  EXPECT_EQ(service2->GetIncognitoWindowCount(), 0);
  EXPECT_EQ(service2->GetIncognitoTabCount(), 0);
  EXPECT_FALSE(service2->HasActiveIncognitoSession());
}

// ---------------------------------------------------------------------------
// Combined settings — multiple pref changes
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, MultiplePrefChanges_EachFiresSpecificObserver) {
  TestIncognitoObserver observer;
  service_->AddObserver(&observer);

  service_->SetShowSidebarBadge(false);
  EXPECT_EQ(observer.show_sidebar_badge_changed_count_, 1);
  EXPECT_EQ(observer.confirm_close_all_changed_count_, 0);

  service_->SetConfirmCloseAll(true);
  EXPECT_EQ(observer.confirm_close_all_changed_count_, 1);
  EXPECT_EQ(observer.warn_on_external_open_changed_count_, 0);

  service_->SetWarnOnExternalOpen(true);
  EXPECT_EQ(observer.warn_on_external_open_changed_count_, 1);
  EXPECT_EQ(observer.default_workspace_changed_count_, 0);

  service_->SetDefaultWorkspaceId("work");
  EXPECT_EQ(observer.default_workspace_changed_count_, 1);

  // Each change should fire the catch-all too.
  EXPECT_EQ(observer.settings_changed_count_, 4);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(IncognitoHandlerTest, EdgeCase_SetShowSidebarBadgeAfterShutdown) {
  service_->Shutdown();
  // Should not crash — just returns early.
  service_->SetShowSidebarBadge(false);
  SUCCEED() << "Setting change after shutdown does not crash.";
}

TEST_F(IncognitoHandlerTest, EdgeCase_SetDefaultWorkspaceAfterShutdown) {
  service_->Shutdown();
  service_->SetDefaultWorkspaceId("test");
  SUCCEED() << "Workspace ID change after shutdown does not crash.";
}

TEST_F(IncognitoHandlerTest, EdgeCase_NotifyCloseFromZeroMultipleTimes) {
  // Closing from zero multiple times should not crash or go negative.
  service_->NotifyIncognitoWindowClosed();
  service_->NotifyIncognitoWindowClosed();
  service_->NotifyIncognitoWindowClosed();

  EXPECT_EQ(service_->GetIncognitoWindowCount(), 0);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
}

TEST_F(IncognitoHandlerTest, EdgeCase_LargeTabCount) {
  service_->NotifyIncognitoWindowCreated(10000);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 10000);

  service_->NotifyIncognitoWindowCreated(10000);
  EXPECT_EQ(service_->GetIncognitoTabCount(), 20000);

  service_->CloseAllIncognitoWindows();
  EXPECT_EQ(service_->GetIncognitoTabCount(), 0);
}

}  // namespace astra
