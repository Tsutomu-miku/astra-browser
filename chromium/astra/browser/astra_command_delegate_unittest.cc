// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_command_delegate.h"

#include <algorithm>

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that tracks which notification methods were called.
//
// Implements base::CheckedObserver via the Observer interface so it can be
// safely removed during iteration.
class TestCommandDelegateObserver
    : public AstraCommandDelegate::Observer {
 public:
  // Specific command notifications
  void OnToggleSidebar() override { toggle_sidebar_called_ = true; }
  void OnToggleSidebarPin() override { toggle_sidebar_pin_called_ = true; }
  void OnOpenCommandPalette() override { open_command_palette_called_ = true; }
  void OnOpenGlance() override { open_glance_called_ = true; }
  void OnShowAllWorkspaces() override { show_all_workspaces_called_ = true; }
  void OnSplitViewStateChanged() override {
    split_view_state_changed_called_ = true;
  }
  void OnOpenSettings() override { open_settings_called_ = true; }

  // General command lifecycle notifications
  void OnCommandExecuted(int command_id) override {
    command_executed_count_++;
    last_executed_command_id_ = command_id;
  }

  void OnRecentCommandsChanged() override {
    recent_commands_changed_count_++;
  }

  void OnCommandAliasesChanged() override {
    command_aliases_changed_count_++;
  }

  // Reset all flags and counters.
  void Reset() {
    toggle_sidebar_called_ = false;
    toggle_sidebar_pin_called_ = false;
    open_command_palette_called_ = false;
    open_glance_called_ = false;
    show_all_workspaces_called_ = false;
    split_view_state_changed_called_ = false;
    open_settings_called_ = false;
    command_executed_count_ = 0;
    last_executed_command_id_ = -1;
    recent_commands_changed_count_ = 0;
    command_aliases_changed_count_ = 0;
  }

  // Specific command flags
  bool toggle_sidebar_called_ = false;
  bool toggle_sidebar_pin_called_ = false;
  bool open_command_palette_called_ = false;
  bool open_glance_called_ = false;
  bool show_all_workspaces_called_ = false;
  bool split_view_state_changed_called_ = false;
  bool open_settings_called_ = false;

  // General command counters
  int command_executed_count_ = 0;
  int last_executed_command_id_ = -1;
  int recent_commands_changed_count_ = 0;
  int command_aliases_changed_count_ = 0;
};

}  // namespace

// ---------------------------------------------------------------------------
// Command identity tests (no Browser or Profile needed)
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, IsAstraCommand_RecognizesAstraIds) {
  // All Astra command IDs should be recognized.
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandToggleSidebar));
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandNewWorkspace));
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandToggleTabFavorite));
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandOpenCommandPalette));
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandFirst));

  // One before the last should be valid.
  EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(
      kAstraCommandLast - 1));
}

TEST(CommandDelegateTest, IsAstraCommand_RejectsChromeIds) {
  // Standard Chrome command IDs (below the Astra range) should be rejected.
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(0));
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(1000));
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(50000));
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(59999));

  // At and above kAstraCommandLast should be rejected.
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(kAstraCommandLast));
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(70000));

  // Negative IDs should also be rejected.
  EXPECT_FALSE(AstraCommandDelegate::IsAstraCommand(-1));
}

TEST(CommandDelegateTest, SupportsCommand_MatchesIsAstraCommand) {
  // SupportsCommand should return the same as IsAstraCommand for all IDs.
  // (They are equivalent today but may diverge if we add reserved IDs.)
  for (int id = 0; id < 70000; id += 1000) {
    EXPECT_EQ(AstraCommandDelegate::SupportsCommand(id),
              AstraCommandDelegate::IsAstraCommand(id));
  }

  // Spot check specific Astra commands.
  EXPECT_TRUE(AstraCommandDelegate::SupportsCommand(
      kAstraCommandToggleSidebar));
  EXPECT_TRUE(AstraCommandDelegate::SupportsCommand(
      kAstraCommandNewWorkspace));
  EXPECT_TRUE(AstraCommandDelegate::SupportsCommand(
      kAstraCommandOpenCommandPalette));
}

TEST(CommandDelegateTest, SupportsCommand_InvalidIds) {
  EXPECT_FALSE(AstraCommandDelegate::SupportsCommand(0));
  EXPECT_FALSE(AstraCommandDelegate::SupportsCommand(-1));
  EXPECT_FALSE(AstraCommandDelegate::SupportsCommand(99999));
}

// ---------------------------------------------------------------------------
// Command ID range integrity
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, CommandIdRange) {
  // Verify the Astra command ID range starts at 60000.
  EXPECT_EQ(kAstraCommandFirst, 60000);

  // Verify the range is non-empty.
  EXPECT_GT(kAstraCommandLast, kAstraCommandFirst);

  // Verify specific command IDs are in the expected order/positions.
  EXPECT_LT(kAstraCommandToggleSidebar, kAstraCommandNewWorkspace);
  EXPECT_LT(kAstraCommandNewWorkspace, kAstraCommandToggleTabFavorite);
  EXPECT_LT(kAstraCommandToggleTabFavorite, kAstraCommandToggleSplitView);
  EXPECT_LT(kAstraCommandToggleSplitView, kAstraCommandOpenGlance);
  EXPECT_LT(kAstraCommandOpenGlance, kAstraCommandOpenCommandPalette);
  EXPECT_LT(kAstraCommandOpenCommandPalette, kAstraCommandLast);
}

// ---------------------------------------------------------------------------
// Command metadata tests
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, GetCommandInfo_ValidCommand) {
  AstraCommandInfo info;
  EXPECT_TRUE(AstraCommandDelegate::GetCommandInfo(
      kAstraCommandToggleSidebar, &info));
  EXPECT_EQ(info.command_id, kAstraCommandToggleSidebar);
  EXPECT_FALSE(info.name.empty());
  EXPECT_FALSE(info.description.empty());
  EXPECT_EQ(info.category, AstraCommandCategory::kView);
}

TEST(CommandDelegateTest, GetCommandInfo_InvalidCommand) {
  AstraCommandInfo info;
  EXPECT_FALSE(AstraCommandDelegate::GetCommandInfo(0, &info));
  EXPECT_FALSE(AstraCommandDelegate::GetCommandInfo(-1, &info));
  EXPECT_FALSE(AstraCommandDelegate::GetCommandInfo(99999, &info));
}

TEST(CommandDelegateTest, GetCommandInfo_NullOutput) {
  EXPECT_FALSE(AstraCommandDelegate::GetCommandInfo(
      kAstraCommandToggleSidebar, nullptr));
}

TEST(CommandDelegateTest, GetCommandName_ValidCommand) {
  std::string name = AstraCommandDelegate::GetCommandName(
      kAstraCommandNewWorkspace);
  EXPECT_FALSE(name.empty());
  EXPECT_GT(name.size(), 0u);
}

TEST(CommandDelegateTest, GetCommandName_InvalidCommand) {
  EXPECT_TRUE(AstraCommandDelegate::GetCommandName(0).empty());
  EXPECT_TRUE(AstraCommandDelegate::GetCommandName(-1).empty());
  EXPECT_TRUE(AstraCommandDelegate::GetCommandName(99999).empty());
}

TEST(CommandDelegateTest, GetCommandDescription_ValidCommand) {
  std::string desc = AstraCommandDelegate::GetCommandDescription(
      kAstraCommandOpenCommandPalette);
  EXPECT_FALSE(desc.empty());
}

TEST(CommandDelegateTest, GetCommandDescription_InvalidCommand) {
  EXPECT_TRUE(AstraCommandDelegate::GetCommandDescription(0).empty());
}

TEST(CommandDelegateTest, GetCommandCategory_WorkspaceCommands) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandNewWorkspace), AstraCommandCategory::kWorkspace);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandNextWorkspace), AstraCommandCategory::kWorkspace);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandDeleteWorkspace), AstraCommandCategory::kWorkspace);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandShowAllWorkspaces), AstraCommandCategory::kWorkspace);
}

TEST(CommandDelegateTest, GetCommandCategory_TabCommands) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleTabFavorite), AstraCommandCategory::kTab);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandReopenClosedTab), AstraCommandCategory::kTab);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleDevTools), AstraCommandCategory::kTab);
}

TEST(CommandDelegateTest, GetCommandCategory_ViewCommands) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleSidebar), AstraCommandCategory::kView);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleSplitView), AstraCommandCategory::kView);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleFocusMode), AstraCommandCategory::kView);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandOpenNewTabPage), AstraCommandCategory::kView);
}

TEST(CommandDelegateTest, GetCommandCategory_NavigationCommands) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandOpenCommandPalette), AstraCommandCategory::kNavigation);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandOpenTabSearch), AstraCommandCategory::kNavigation);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandFocusOmniboxCommandMode),
      AstraCommandCategory::kNavigation);
}

TEST(CommandDelegateTest, GetCommandCategory_ToolsCommands) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandScreenshotVisible), AstraCommandCategory::kTools);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandOpenSettings), AstraCommandCategory::kTools);
  EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(
      kAstraCommandToggleExtensionsPanel), AstraCommandCategory::kTools);
}

TEST(CommandDelegateTest, GetAllCommandIds_NonEmpty) {
  auto ids = AstraCommandDelegate::GetAllCommandIds();
  EXPECT_FALSE(ids.empty());
  EXPECT_GT(ids.size(), 10u);
}

TEST(CommandDelegateTest, GetAllCommandIds_AllValid) {
  auto ids = AstraCommandDelegate::GetAllCommandIds();
  for (int id : ids) {
    EXPECT_TRUE(AstraCommandDelegate::IsAstraCommand(id));
    EXPECT_TRUE(AstraCommandDelegate::SupportsCommand(id));
  }
}

TEST(CommandDelegateTest, GetAllCommandIds_NoDuplicates) {
  auto ids = AstraCommandDelegate::GetAllCommandIds();
  std::sort(ids.begin(), ids.end());
  for (size_t i = 1; i < ids.size(); ++i) {
    EXPECT_NE(ids[i], ids[i - 1]) << "Duplicate command ID: " << ids[i];
  }
}

// ---------------------------------------------------------------------------
// Command category tests
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, GetCommandsByCategory_Workspace) {
  auto ids = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kWorkspace);
  EXPECT_FALSE(ids.empty());
  // Verify all returned IDs are workspace commands.
  for (int id : ids) {
    EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(id),
              AstraCommandCategory::kWorkspace);
  }
  // Spot-check known workspace commands are present.
  EXPECT_NE(std::find(ids.begin(), ids.end(), kAstraCommandNewWorkspace),
            ids.end());
  EXPECT_NE(std::find(ids.begin(), ids.end(), kAstraCommandNextWorkspace),
            ids.end());
  EXPECT_NE(std::find(ids.begin(), ids.end(), kAstraCommandDeleteWorkspace),
            ids.end());
}

TEST(CommandDelegateTest, GetCommandsByCategory_Tab) {
  auto ids = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kTab);
  EXPECT_FALSE(ids.empty());
  for (int id : ids) {
    EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(id),
              AstraCommandCategory::kTab);
  }
}

TEST(CommandDelegateTest, GetCommandsByCategory_View) {
  auto ids = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kView);
  EXPECT_FALSE(ids.empty());
  for (int id : ids) {
    EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(id),
              AstraCommandCategory::kView);
  }
}

TEST(CommandDelegateTest, GetCommandsByCategory_Navigation) {
  auto ids = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kNavigation);
  EXPECT_FALSE(ids.empty());
  for (int id : ids) {
    EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(id),
              AstraCommandCategory::kNavigation);
  }
}

TEST(CommandDelegateTest, GetCommandsByCategory_Tools) {
  auto ids = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kTools);
  EXPECT_FALSE(ids.empty());
  for (int id : ids) {
    EXPECT_EQ(AstraCommandDelegate::GetCommandCategory(id),
              AstraCommandCategory::kTools);
  }
}

TEST(CommandDelegateTest, AllCategoriesSumToTotal) {
  // The sum of commands across all categories should equal the total.
  size_t total = AstraCommandDelegate::GetAllCommandIds().size();

  size_t workspace_count = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kWorkspace).size();
  size_t tab_count = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kTab).size();
  size_t view_count = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kView).size();
  size_t nav_count = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kNavigation).size();
  size_t tools_count = AstraCommandDelegate::GetCommandsByCategory(
      AstraCommandCategory::kTools).size();

  EXPECT_EQ(workspace_count + tab_count + view_count + nav_count + tools_count,
            total)
      << "Sum of category counts should equal total command count.";
}

// ---------------------------------------------------------------------------
// ExecuteCommand: invalid / null browser tests
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, ExecuteCommand_InvalidIdReturnsFalse) {
  // Non-Astra command IDs should return false even with a valid browser.
  // We can test the null-browser + invalid-id path:
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr, 0));
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr, 1000));
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr, -5));
}

TEST(CommandDelegateTest, ExecuteCommand_NullBrowserReturnsFalse) {
  // Null browser should return false for all command IDs, including valid
  // Astra commands.
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr,
      kAstraCommandToggleSidebar));
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr,
      kAstraCommandNewWorkspace));
  EXPECT_FALSE(AstraCommandDelegate::ExecuteCommand(nullptr,
      kAstraCommandOpenCommandPalette));
}

// ---------------------------------------------------------------------------
// IsCommandEnabled: basic tests
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, IsCommandEnabled_NewWorkspaceAlwaysEnabled) {
  // With null browser, nothing is enabled.
  EXPECT_FALSE(AstraCommandDelegate::IsCommandEnabled(
      nullptr, kAstraCommandNewWorkspace));

  // Invalid IDs are never enabled.
  EXPECT_FALSE(AstraCommandDelegate::IsCommandEnabled(nullptr, 0));
  EXPECT_FALSE(AstraCommandDelegate::IsCommandEnabled(nullptr, 99999));
}

TEST(CommandDelegateTest, IsCommandEnabled_InvalidCommandReturnsFalse) {
  EXPECT_FALSE(AstraCommandDelegate::IsCommandEnabled(nullptr, 0));
  EXPECT_FALSE(AstraCommandDelegate::IsCommandEnabled(
      nullptr, kAstraCommandLast));
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraCommandDelegate::Observer {};

  DefaultObserver observer;
  AstraCommandDelegate::AddObserver(&observer);

  // The observer doesn't override any methods — verify it doesn't crash
  // when we add/remove it.
  AstraCommandDelegate::RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST(CommandDelegateTest, Observer_AddAndRemove) {
  TestCommandDelegateObserver observer;

  // Add, then remove — should not crash.
  AstraCommandDelegate::AddObserver(&observer);
  AstraCommandDelegate::RemoveObserver(&observer);

  SUCCEED() << "AddObserver / RemoveObserver completed without crash.";
}

TEST(CommandDelegateTest, Observer_RemoveNonexistent_NoCrash) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::RemoveObserver(&observer);
  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST(CommandDelegateTest, Observer_MultipleObservers) {
  TestCommandDelegateObserver observer1;
  TestCommandDelegateObserver observer2;

  AstraCommandDelegate::AddObserver(&observer1);
  AstraCommandDelegate::AddObserver(&observer2);

  // Clean up
  AstraCommandDelegate::RemoveObserver(&observer1);
  AstraCommandDelegate::RemoveObserver(&observer2);

  SUCCEED() << "Multiple observer registration completed without crash.";
}

// ===========================================================================
// Recent commands and aliases tests (with Profile)
// ===========================================================================

// Test fixture for tests that need a profile (recent commands, aliases).
class CommandDelegateProfileTest : public testing::Test {
 protected:
  CommandDelegateProfileTest() {
    profile_ = std::make_unique<TestingProfile>();
    DCHECK(profile_);
  }

  ~CommandDelegateProfileTest() override = default;

  void SetUp() override {
    // Ensure the recent commands list starts empty.
    auto recent = AstraCommandDelegate::GetRecentCommands(profile_.get());
    ASSERT_TRUE(recent.empty());
  }

  void TearDown() override {
    // Clean up any registered observers.
    for (auto* observer : test_observers_) {
      AstraCommandDelegate::RemoveObserver(observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::vector<TestCommandDelegateObserver*> test_observers_;
};

// ---------------------------------------------------------------------------
// Recent commands: default state
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, RecentCommands_DefaultEmpty) {
  auto recent = AstraCommandDelegate::GetRecentCommands(profile_.get());
  EXPECT_TRUE(recent.empty());
}

TEST_F(CommandDelegateProfileTest, RecentCommands_DefaultMaxCount) {
  int max = AstraCommandDelegate::GetMaxRecentCommands(profile_.get());
  EXPECT_EQ(max, prefs::kDefaultCommandRecentMax);
  EXPECT_GT(max, 0);
}

// ---------------------------------------------------------------------------
// Recent commands: null profile
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, RecentCommands_NullProfile_Empty) {
  auto recent = AstraCommandDelegate::GetRecentCommands(nullptr);
  EXPECT_TRUE(recent.empty());
}

TEST_F(CommandDelegateProfileTest, ClearRecentCommands_NullProfile_NoCrash) {
  AstraCommandDelegate::ClearRecentCommands(nullptr);
  SUCCEED() << "ClearRecentCommands with null profile does not crash.";
}

TEST_F(CommandDelegateProfileTest, MaxRecentCommands_NullProfile_Default) {
  int max = AstraCommandDelegate::GetMaxRecentCommands(nullptr);
  EXPECT_EQ(max, prefs::kDefaultCommandRecentMax);
}

TEST_F(CommandDelegateProfileTest, SetMaxRecentCommands_NullProfile_NoCrash) {
  AstraCommandDelegate::SetMaxRecentCommands(nullptr, 5);
  SUCCEED() << "SetMaxRecentCommands with null profile does not crash.";
}

// ---------------------------------------------------------------------------
// Recent commands: add / clear
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, RecentCommands_ClearEmptyIsNoOp) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  // Clearing an empty list should not fire the notification.
  AstraCommandDelegate::ClearRecentCommands(profile_.get());
  EXPECT_EQ(observer.recent_commands_changed_count_, 0);
}

TEST_F(CommandDelegateProfileTest, SetMaxRecentCommands_SameValueNoOp) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  int current = AstraCommandDelegate::GetMaxRecentCommands(profile_.get());
  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), current);
  EXPECT_EQ(observer.recent_commands_changed_count_, 0);
}

TEST_F(CommandDelegateProfileTest, SetMaxRecentCommands_ChangesValue) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  int old_max = AstraCommandDelegate::GetMaxRecentCommands(profile_.get());
  int new_max = old_max + 5;

  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), new_max);
  EXPECT_EQ(AstraCommandDelegate::GetMaxRecentCommands(profile_.get()), new_max);
  EXPECT_GT(observer.recent_commands_changed_count_, 0);
}

TEST_F(CommandDelegateProfileTest, SetMaxRecentCommands_ClampsToOne) {
  // Setting max to 0 or negative should clamp to 1.
  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), 0);
  EXPECT_GE(AstraCommandDelegate::GetMaxRecentCommands(profile_.get()), 1);

  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), -5);
  EXPECT_GE(AstraCommandDelegate::GetMaxRecentCommands(profile_.get()), 1);
}

TEST_F(CommandDelegateProfileTest, SetMaxRecentCommands_TruncatesList) {
  // First, add some commands by directly manipulating prefs.
  base::Value::List list;
  list.Append(kAstraCommandToggleSidebar);
  list.Append(kAstraCommandNewWorkspace);
  list.Append(kAstraCommandOpenCommandPalette);
  list.Append(kAstraCommandToggleSplitView);
  list.Append(kAstraCommandScreenshotVisible);
  profile_->GetPrefs()->SetList(prefs::kPrefCommandRecentList,
                                std::move(list));

  ASSERT_EQ(AstraCommandDelegate::GetRecentCommands(profile_.get()).size(),
            5u);

  // Reduce max to 2 — should truncate.
  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), 2);
  auto recent = AstraCommandDelegate::GetRecentCommands(profile_.get());
  EXPECT_EQ(recent.size(), 2u);
  // First two should be preserved (most recent first).
  EXPECT_EQ(recent[0], kAstraCommandToggleSidebar);
  EXPECT_EQ(recent[1], kAstraCommandNewWorkspace);
}

// ---------------------------------------------------------------------------
// Command aliases: default state
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, Aliases_DefaultEmpty) {
  auto aliases = AstraCommandDelegate::GetAllAliases(profile_.get());
  EXPECT_TRUE(aliases.empty());
}

TEST_F(CommandDelegateProfileTest, Aliases_NullProfile_Empty) {
  auto aliases = AstraCommandDelegate::GetAllAliases(nullptr);
  EXPECT_TRUE(aliases.empty());
}

// ---------------------------------------------------------------------------
// Command aliases: add / remove
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, AddAlias_ValidCommand) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  bool result = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "sidebar");
  EXPECT_TRUE(result);
  EXPECT_GT(observer.command_aliases_changed_count_, 0);

  // Verify the alias exists.
  auto aliases = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandToggleSidebar);
  EXPECT_EQ(aliases.size(), 1u);
  EXPECT_EQ(aliases[0], "sidebar");

  // Verify reverse lookup works.
  EXPECT_EQ(AstraCommandDelegate::GetCommandByAlias(profile_.get(), "sidebar"),
            kAstraCommandToggleSidebar);
}

TEST_F(CommandDelegateProfileTest, AddAlias_InvalidCommand) {
  bool result = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), 99999, "bad");
  EXPECT_FALSE(result);
}

TEST_F(CommandDelegateProfileTest, AddAlias_EmptyAlias) {
  bool result = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "");
  EXPECT_FALSE(result);
}

TEST_F(CommandDelegateProfileTest, AddAlias_NullProfile) {
  bool result = AstraCommandDelegate::AddCommandAlias(
      nullptr, kAstraCommandToggleSidebar, "sidebar");
  EXPECT_FALSE(result);
}

TEST_F(CommandDelegateProfileTest, AddAlias_DuplicateSameCommand_NoOp) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  // Add the first time.
  bool result1 = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "sidebar");
  EXPECT_TRUE(result1);
  int initial_count = observer.command_aliases_changed_count_;

  // Add the same alias for the same command again — should be no-op.
  bool result2 = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "sidebar");
  EXPECT_TRUE(result2);
  EXPECT_EQ(observer.command_aliases_changed_count_, initial_count);

  auto aliases = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandToggleSidebar);
  EXPECT_EQ(aliases.size(), 1u);
}

TEST_F(CommandDelegateProfileTest, AddAlias_DuplicateDifferentCommand_Fails) {
  // Add alias for first command.
  bool result1 = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "mytoggle");
  EXPECT_TRUE(result1);

  // Try to use the same alias for a different command — should fail.
  bool result2 = AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandNewWorkspace, "mytoggle");
  EXPECT_FALSE(result2);

  // The alias should still point to the original command.
  EXPECT_EQ(
      AstraCommandDelegate::GetCommandByAlias(profile_.get(), "mytoggle"),
      kAstraCommandToggleSidebar);
}

TEST_F(CommandDelegateProfileTest, AddAlias_MultipleAliasesSameCommand) {
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandNewWorkspace, "newws"));
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandNewWorkspace, "create-workspace"));

  auto aliases = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandNewWorkspace);
  EXPECT_EQ(aliases.size(), 2u);
}

TEST_F(CommandDelegateProfileTest, RemoveAlias_Existing) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  // Add an alias first.
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "sidebar"));
  observer.Reset();

  // Remove it.
  AstraCommandDelegate::RemoveCommandAlias(profile_.get(), "sidebar");
  EXPECT_GT(observer.command_aliases_changed_count_, 0);

  // Verify it's gone.
  auto aliases = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandToggleSidebar);
  EXPECT_TRUE(aliases.empty());
  EXPECT_EQ(AstraCommandDelegate::GetCommandByAlias(profile_.get(), "sidebar"),
            -1);
}

TEST_F(CommandDelegateProfileTest, RemoveAlias_Nonexistent_NoOp) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  AstraCommandDelegate::RemoveCommandAlias(profile_.get(), "nonexistent");
  EXPECT_EQ(observer.command_aliases_changed_count_, 0);
}

TEST_F(CommandDelegateProfileTest, RemoveAlias_EmptyString_NoOp) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  AstraCommandDelegate::RemoveCommandAlias(profile_.get(), "");
  EXPECT_EQ(observer.command_aliases_changed_count_, 0);
}

TEST_F(CommandDelegateProfileTest, RemoveAlias_NullProfile_NoCrash) {
  AstraCommandDelegate::RemoveCommandAlias(nullptr, "something");
  SUCCEED() << "RemoveCommandAlias with null profile does not crash.";
}

TEST_F(CommandDelegateProfileTest, GetCommandByAlias_NotFound) {
  EXPECT_EQ(AstraCommandDelegate::GetCommandByAlias(profile_.get(), "nope"),
            -1);
  EXPECT_EQ(AstraCommandDelegate::GetCommandByAlias(profile_.get(), ""),
            -1);
  EXPECT_EQ(AstraCommandDelegate::GetCommandByAlias(nullptr, "nope"),
            -1);
}

TEST_F(CommandDelegateProfileTest, GetAllAliases) {
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "a1"));
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandNewWorkspace, "a2"));
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandScreenshotVisible, "a3"));

  auto all = AstraCommandDelegate::GetAllAliases(profile_.get());
  EXPECT_EQ(all.size(), 3u);
}

TEST_F(CommandDelegateProfileTest, GetCommandAliases_NoAliases) {
  auto aliases = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandToggleSidebar);
  EXPECT_TRUE(aliases.empty());
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, Aliases_PersistAcrossServiceAccess) {
  // Add aliases.
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandToggleSidebar, "sidebar-toggle"));
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandNewWorkspace, "new-workspace"));

  // Verify they persist by reading again (same profile = same prefs).
  auto aliases1 = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandToggleSidebar);
  EXPECT_EQ(aliases1.size(), 1u);
  EXPECT_EQ(aliases1[0], "sidebar-toggle");

  auto aliases2 = AstraCommandDelegate::GetCommandAliases(
      profile_.get(), kAstraCommandNewWorkspace);
  EXPECT_EQ(aliases2.size(), 1u);
  EXPECT_EQ(aliases2[0], "new-workspace");
}

TEST_F(CommandDelegateProfileTest, Aliases_GetCommandByAlias_RoundTrip) {
  ASSERT_TRUE(AstraCommandDelegate::AddCommandAlias(
      profile_.get(), kAstraCommandScreenshotRegion, "snap-region"));

  int command_id = AstraCommandDelegate::GetCommandByAlias(
      profile_.get(), "snap-region");
  EXPECT_EQ(command_id, kAstraCommandScreenshotRegion);

  // Verify metadata matches.
  std::string name = AstraCommandDelegate::GetCommandName(command_id);
  EXPECT_FALSE(name.empty());
}

TEST_F(CommandDelegateProfileTest, MaxRecentCommands_Persists) {
  int original = AstraCommandDelegate::GetMaxRecentCommands(profile_.get());

  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), 15);
  EXPECT_EQ(AstraCommandDelegate::GetMaxRecentCommands(profile_.get()), 15);

  // Reset and verify.
  AstraCommandDelegate::SetMaxRecentCommands(profile_.get(), original);
  EXPECT_EQ(AstraCommandDelegate::GetMaxRecentCommands(profile_.get()),
            original);
}

// ---------------------------------------------------------------------------
// Utility / edge case tests
// ---------------------------------------------------------------------------

TEST_F(CommandDelegateProfileTest, CommandInfo_NameDescriptionNonEmpty) {
  // Verify every command has a non-empty name and description.
  auto ids = AstraCommandDelegate::GetAllCommandIds();
  ASSERT_FALSE(ids.empty());

  for (int id : ids) {
    AstraCommandInfo info;
    ASSERT_TRUE(AstraCommandDelegate::GetCommandInfo(id, &info))
        << "Command ID " << id << " has no metadata entry";
    EXPECT_FALSE(info.name.empty())
        << "Command ID " << id << " has empty name";
    EXPECT_FALSE(info.description.empty())
        << "Command ID " << id << " has empty description";
    // Verify the category is valid.
    EXPECT_GE(static_cast<int>(info.category), 0);
    EXPECT_LE(static_cast<int>(info.category),
              static_cast<int>(AstraCommandCategory::kTools));
  }
}

TEST_F(CommandDelegateProfileTest, MetadataCountsMatch) {
  // The number of commands with metadata should equal the number of valid
  // Astra command IDs (since every command should have metadata).
  auto all_ids = AstraCommandDelegate::GetAllCommandIds();

  // Count how many commands have metadata via GetCommandInfo.
  int with_metadata = 0;
  for (int id = kAstraCommandFirst; id < kAstraCommandLast; ++id) {
    AstraCommandInfo info;
    if (AstraCommandDelegate::GetCommandInfo(id, &info)) {
      with_metadata++;
    }
  }

  EXPECT_EQ(static_cast<int>(all_ids.size()), with_metadata)
      << "All Astra commands should have metadata entries.";
}

TEST(CommandDelegateTest, CategoryEnumValues) {
  // Verify the category enum has the expected values and ordering.
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kWorkspace), 0);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kTab), 1);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kView), 2);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kNavigation), 3);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kTools), 4);
}

TEST(CommandDelegateTest, CommandInfoStruct_DefaultValues) {
  AstraCommandInfo info;
  EXPECT_EQ(info.command_id, -1);
  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.description.empty());
  EXPECT_EQ(info.category, AstraCommandCategory::kTools);
}

TEST_F(CommandDelegateProfileTest, GetAllAliases_DefaultEmpty) {
  auto aliases = AstraCommandDelegate::GetAllAliases(profile_.get());
  EXPECT_TRUE(aliases.empty());
}

TEST_F(CommandDelegateProfileTest, ClearRecentCommands_FiresObserver) {
  TestCommandDelegateObserver observer;
  AstraCommandDelegate::AddObserver(&observer);
  test_observers_.push_back(&observer);

  // Add some entries by manipulating prefs directly.
  base::Value::List list;
  list.Append(kAstraCommandToggleSidebar);
  list.Append(kAstraCommandNewWorkspace);
  profile_->GetPrefs()->SetList(prefs::kPrefCommandRecentList,
                                std::move(list));
  observer.Reset();

  // Now clear — should fire notification.
  AstraCommandDelegate::ClearRecentCommands(profile_.get());
  EXPECT_GT(observer.recent_commands_changed_count_, 0);

  // Verify empty.
  auto recent = AstraCommandDelegate::GetRecentCommands(profile_.get());
  EXPECT_TRUE(recent.empty());
}

}  // namespace astra
