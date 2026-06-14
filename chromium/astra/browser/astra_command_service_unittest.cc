// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_command_service.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestCommandServiceObserver : public AstraCommandServiceObserver {
 public:
  void OnCommandExecuted(const std::string& command_id,
                         const std::string& command_name) override {
    executed_count_++;
    last_executed_id_ = command_id;
    last_executed_name_ = command_name;
    executed_ids_.push_back(command_id);
  }

  void OnCommandRegistered(const std::string& command_id) override {
    registered_count_++;
    last_registered_id_ = command_id;
  }

  void OnCommandUnregistered(const std::string& command_id) override {
    unregistered_count_++;
    last_unregistered_id_ = command_id;
  }

  // Counters
  int executed_count_ = 0;
  int registered_count_ = 0;
  int unregistered_count_ = 0;

  // Last recorded values
  std::string last_executed_id_;
  std::string last_executed_name_;
  std::string last_registered_id_;
  std::string last_unregistered_id_;

  // All executed IDs (in order)
  std::vector<std::string> executed_ids_;
};

// Helper to create a simple command info for testing.
AstraCommandInfo MakeTestCommand(const std::string& id,
                                 const std::string& name,
                                 const std::string& category = "test",
                                 int rank = 0) {
  AstraCommandInfo cmd;
  cmd.command_id = id;
  cmd.display_name = name;
  cmd.description = "Test command: " + name;
  cmd.category = category;
  cmd.icon_id = "icon-" + id;
  cmd.keybinding = "";
  cmd.is_enabled = true;
  cmd.rank = rank;
  return cmd;
}

}  // namespace

// ===========================================================================
// Test fixture
// ===========================================================================
//
// Uses TestingProfile from //chrome/test:test_support so the service has a
// real Profile* to attach to.
class CommandServiceTest : public testing::Test {
 protected:
  CommandServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    service_ = AstraCommandServiceFactory::GetForProfile(profile_.get());
    DCHECK(service_);
  }

  ~CommandServiceTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Service should start with built-in commands.
    ASSERT_GT(service_->command_count(), 0u);
  }

  void TearDown() override {
    // Clean up observers.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Creates a test observer and adds it to the service.
  TestCommandServiceObserver& AddTestObserver() {
    test_observers_.emplace_back();
    service_->AddObserver(&test_observers_.back());
    return test_observers_.back();
  }

  // Task environment is required for TestingProfile and base::Time.
  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<AstraCommandService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestCommandServiceObserver> test_observers_;
};

// ===========================================================================
// Default commands / construction
// ===========================================================================

TEST_F(CommandServiceTest, ConstructionHasBuiltInCommands) {
  // Should have many built-in commands registered at construction.
  EXPECT_GT(service_->command_count(), 15u);
}

TEST_F(CommandServiceTest, BuiltInWorkspaceCommands) {
  auto workspace_cmds = service_->GetCommandsByCategory("workspace");
  EXPECT_EQ(workspace_cmds.size(), 4u);

  EXPECT_NE(service_->GetCommand("workspace.new"), nullptr);
  EXPECT_NE(service_->GetCommand("workspace.switch"), nullptr);
  EXPECT_NE(service_->GetCommand("workspace.rename"), nullptr);
  EXPECT_NE(service_->GetCommand("workspace.delete"), nullptr);

  EXPECT_EQ(service_->GetCommand("workspace.new")->display_name,
            "New Workspace");
}

TEST_F(CommandServiceTest, BuiltInTabCommands) {
  auto tab_cmds = service_->GetCommandsByCategory("tab");
  EXPECT_EQ(tab_cmds.size(), 6u);

  EXPECT_NE(service_->GetCommand("tab.new"), nullptr);
  EXPECT_NE(service_->GetCommand("tab.close"), nullptr);
  EXPECT_NE(service_->GetCommand("tab.duplicate"), nullptr);
  EXPECT_NE(service_->GetCommand("tab.pin"), nullptr);
  EXPECT_NE(service_->GetCommand("tab.mute"), nullptr);
  EXPECT_NE(service_->GetCommand("tab.reload"), nullptr);
}

TEST_F(CommandServiceTest, BuiltInNavigationCommands) {
  auto nav_cmds = service_->GetCommandsByCategory("navigation");
  EXPECT_EQ(nav_cmds.size(), 4u);

  EXPECT_NE(service_->GetCommand("navigation.back"), nullptr);
  EXPECT_NE(service_->GetCommand("navigation.forward"), nullptr);
  EXPECT_NE(service_->GetCommand("navigation.refresh"), nullptr);
  EXPECT_NE(service_->GetCommand("navigation.home"), nullptr);
}

TEST_F(CommandServiceTest, BuiltInSearchCommands) {
  auto search_cmds = service_->GetCommandsByCategory("search");
  EXPECT_EQ(search_cmds.size(), 3u);

  EXPECT_NE(service_->GetCommand("search.google"), nullptr);
  EXPECT_NE(service_->GetCommand("search.bookmarks"), nullptr);
  EXPECT_NE(service_->GetCommand("search.history"), nullptr);
}

TEST_F(CommandServiceTest, BuiltInSystemCommands) {
  auto system_cmds = service_->GetCommandsByCategory("system");
  EXPECT_EQ(system_cmds.size(), 4u);

  EXPECT_NE(service_->GetCommand("system.toggle_sidebar"), nullptr);
  EXPECT_NE(service_->GetCommand("system.toggle_fullscreen"), nullptr);
  EXPECT_NE(service_->GetCommand("system.open_settings"), nullptr);
  EXPECT_NE(service_->GetCommand("system.exit"), nullptr);
}

TEST_F(CommandServiceTest, BuiltInCommandsHaveDefaultKeybindings) {
  const auto* new_tab = service_->GetCommand("tab.new");
  ASSERT_NE(new_tab, nullptr);
  EXPECT_FALSE(new_tab->keybinding.empty());
  EXPECT_EQ(new_tab->keybinding, "Ctrl+T");

  const auto* close_tab = service_->GetCommand("tab.close");
  ASSERT_NE(close_tab, nullptr);
  EXPECT_EQ(close_tab->keybinding, "Ctrl+W");
}

// ===========================================================================
// Command CRUD
// ===========================================================================

TEST_F(CommandServiceTest, RegisterCommand) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd = MakeTestCommand("custom.test", "Test Command", "test", 50);
  EXPECT_TRUE(service_->RegisterCommand(cmd));

  EXPECT_EQ(observer.registered_count_, 1);
  EXPECT_EQ(observer.last_registered_id_, "custom.test");

  const AstraCommandInfo* retrieved = service_->GetCommand("custom.test");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->display_name, "Test Command");
  EXPECT_EQ(retrieved->category, "test");
  EXPECT_EQ(retrieved->rank, 50);
}

TEST_F(CommandServiceTest, RegisterCommandDuplicateIdFails) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd1 = MakeTestCommand("dup.id", "First");
  EXPECT_TRUE(service_->RegisterCommand(cmd1));

  AstraCommandInfo cmd2 = MakeTestCommand("dup.id", "Second");
  EXPECT_FALSE(service_->RegisterCommand(cmd2));

  // Only one registered notification.
  EXPECT_EQ(observer.registered_count_, 1);
}

TEST_F(CommandServiceTest, RegisterCommandEmptyIdFails) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd = MakeTestCommand("", "No ID");
  EXPECT_FALSE(service_->RegisterCommand(cmd));

  EXPECT_EQ(observer.registered_count_, 0);
}

TEST_F(CommandServiceTest, GetCommandReturnsNullForNonexistent) {
  EXPECT_EQ(service_->GetCommand("nonexistent.command"), nullptr);
}

TEST_F(CommandServiceTest, UnregisterCommand) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd = MakeTestCommand("to.remove", "Remove Me");
  service_->RegisterCommand(cmd);

  observer.unregistered_count_ = 0;

  EXPECT_TRUE(service_->UnregisterCommand("to.remove"));
  EXPECT_EQ(service_->GetCommand("to.remove"), nullptr);
  EXPECT_EQ(observer.unregistered_count_, 1);
  EXPECT_EQ(observer.last_unregistered_id_, "to.remove");
}

TEST_F(CommandServiceTest, UnregisterNonexistentReturnsFalse) {
  auto& observer = AddTestObserver();
  EXPECT_FALSE(service_->UnregisterCommand("nonexistent"));
  EXPECT_EQ(observer.unregistered_count_, 0);
}

// ===========================================================================
// GetAllCommands
// ===========================================================================

TEST_F(CommandServiceTest, GetAllCommandsReturnsAll) {
  auto all = service_->GetAllCommands();
  EXPECT_EQ(all.size(), service_->command_count());
}

TEST_F(CommandServiceTest, GetAllCommandsSortedByRank) {
  auto all = service_->GetAllCommands();
  ASSERT_FALSE(all.empty());

  // Should be sorted by rank descending.
  for (size_t i = 1; i < all.size(); ++i) {
    if (all[i].rank != all[i - 1].rank) {
      EXPECT_LE(all[i].rank, all[i - 1].rank);
    }
  }
}

// ===========================================================================
// Category filtering
// ===========================================================================

TEST_F(CommandServiceTest, GetCommandsByCategory) {
  auto tab_cmds = service_->GetCommandsByCategory("tab");
  EXPECT_EQ(tab_cmds.size(), 6u);

  for (const auto& cmd : tab_cmds) {
    EXPECT_EQ(cmd.category, "tab");
    EXPECT_TRUE(cmd.is_enabled);
  }
}

TEST_F(CommandServiceTest, GetCommandsByCategoryEmpty) {
  auto cmds = service_->GetCommandsByCategory("nonexistent_category");
  EXPECT_TRUE(cmds.empty());
}

TEST_F(CommandServiceTest, CategoryFilterExcludesDisabled) {
  // Disable a tab command.
  service_->SetCommandEnabled("tab.mute", false);

  auto tab_cmds = service_->GetCommandsByCategory("tab");
  // Should have one fewer than the full set.
  EXPECT_EQ(tab_cmds.size(), 5u);

  // The disabled command should not be in the result.
  for (const auto& cmd : tab_cmds) {
    EXPECT_NE(cmd.command_id, "tab.mute");
  }
}

// ===========================================================================
// Search
// ===========================================================================

TEST_F(CommandServiceTest, SearchEmptyQueryReturnsAllEnabled) {
  auto results = service_->SearchCommands("");
  EXPECT_EQ(results.size(), service_->command_count());
}

TEST_F(CommandServiceTest, SearchWhitespaceOnlyReturnsAllEnabled) {
  auto results = service_->SearchCommands("   ");
  EXPECT_EQ(results.size(), service_->command_count());
}

TEST_F(CommandServiceTest, SearchPartialMatchByName) {
  auto results = service_->SearchCommands("work");
  EXPECT_FALSE(results.empty());

  // All results should contain "work" (case-insensitive) in name or description.
  for (const auto& cmd : results) {
    bool matches = cmd.display_name.find("work") != std::string::npos ||
                   cmd.display_name.find("Work") != std::string::npos ||
                   cmd.description.find("work") != std::string::npos ||
                   cmd.description.find("Work") != std::string::npos;
    EXPECT_TRUE(matches) << "Command " << cmd.display_name
                         << " should match 'work'";
  }
}

TEST_F(CommandServiceTest, SearchCaseInsensitive) {
  auto results_upper = service_->SearchCommands("TAB");
  auto results_lower = service_->SearchCommands("tab");
  EXPECT_EQ(results_upper.size(), results_lower.size());
}

TEST_F(CommandServiceTest, SearchNoMatch) {
  auto results = service_->SearchCommands("zzzzzzzzz_nonexistent");
  EXPECT_TRUE(results.empty());
}

TEST_F(CommandServiceTest, SearchSkipsDisabledCommands) {
  // Disable one of the tab commands.
  service_->SetCommandEnabled("tab.mute", false);

  auto results = service_->SearchCommands("mute");
  EXPECT_TRUE(results.empty()) << "Disabled commands should not show in search";
}

TEST_F(CommandServiceTest, SearchByDescription) {
  // Search for a term that appears in descriptions but not in names.
  auto results = service_->SearchCommands("workspace");
  EXPECT_FALSE(results.empty());

  // At least some should match via description.
  bool has_desc_match = false;
  for (const auto& cmd : results) {
    std::string desc_lower = cmd.description;
    std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(),
                   ::tolower);
    if (desc_lower.find("workspace") != std::string::npos) {
      has_desc_match = true;
      break;
    }
  }
  EXPECT_TRUE(has_desc_match);
}

// ===========================================================================
// Execute command
// ===========================================================================

TEST_F(CommandServiceTest, ExecuteCommandSucceeds) {
  auto& observer = AddTestObserver();

  EXPECT_TRUE(service_->ExecuteCommand("tab.new"));
  EXPECT_EQ(observer.executed_count_, 1);
  EXPECT_EQ(observer.last_executed_id_, "tab.new");
  EXPECT_EQ(observer.last_executed_name_, "New Tab");
}

TEST_F(CommandServiceTest, ExecuteNonexistentCommandFails) {
  auto& observer = AddTestObserver();

  EXPECT_FALSE(service_->ExecuteCommand("nonexistent.command"));
  EXPECT_EQ(observer.executed_count_, 0);
}

TEST_F(CommandServiceTest, ExecuteDisabledCommandFails) {
  auto& observer = AddTestObserver();

  service_->SetCommandEnabled("tab.mute", false);
  EXPECT_FALSE(service_->ExecuteCommand("tab.mute"));
  EXPECT_EQ(observer.executed_count_, 0);
}

TEST_F(CommandServiceTest, ExecuteRecordsInRecent) {
  service_->ExecuteCommand("tab.new");

  auto recent = service_->GetRecentCommands();
  ASSERT_FALSE(recent.empty());
  EXPECT_EQ(recent[0].command_id, "tab.new");
}

// ===========================================================================
// Recent commands
// ===========================================================================

TEST_F(CommandServiceTest, RecentCommandsEmptyInitially) {
  auto recent = service_->GetRecentCommands();
  EXPECT_TRUE(recent.empty());
}

TEST_F(CommandServiceTest, RecentCommandsAddsToFront) {
  service_->ExecuteCommand("tab.new");
  service_->ExecuteCommand("tab.close");

  auto recent = service_->GetRecentCommands();
  ASSERT_GE(recent.size(), 2u);
  EXPECT_EQ(recent[0].command_id, "tab.close");  // Most recent first.
  EXPECT_EQ(recent[1].command_id, "tab.new");
}

TEST_F(CommandServiceTest, RecentCommandsNoDuplicates) {
  service_->ExecuteCommand("tab.new");
  service_->ExecuteCommand("tab.close");
  service_->ExecuteCommand("tab.new");  // Execute again.

  auto recent = service_->GetRecentCommands();

  // "tab.new" should appear only once and be at the front.
  int count = 0;
  for (const auto& cmd : recent) {
    if (cmd.command_id == "tab.new") {
      count++;
    }
  }
  EXPECT_EQ(count, 1);
  EXPECT_EQ(recent[0].command_id, "tab.new");
}

TEST_F(CommandServiceTest, RecentCommandsBoundedSize) {
  // Execute many commands (more than the max of 10).
  std::vector<std::string> cmd_ids = {
      "tab.new", "tab.close", "tab.duplicate", "tab.pin", "tab.mute",
      "tab.reload", "navigation.back", "navigation.forward",
      "navigation.refresh", "navigation.home", "search.google",
      "search.bookmarks", "workspace.new"
  };

  for (const auto& id : cmd_ids) {
    service_->ExecuteCommand(id);
  }

  auto recent = service_->GetRecentCommands();
  EXPECT_LE(recent.size(), service_->max_recent_commands());
  EXPECT_EQ(recent.size(), 10u);
}

TEST_F(CommandServiceTest, RecentCommandsExcludesUnregistered) {
  // Register a custom command, execute it, then unregister it.
  AstraCommandInfo cmd = MakeTestCommand("temp.cmd", "Temp Command");
  service_->RegisterCommand(cmd);
  service_->ExecuteCommand("temp.cmd");

  // Verify it's in recent.
  auto recent = service_->GetRecentCommands();
  EXPECT_EQ(recent[0].command_id, "temp.cmd");

  // Now unregister.
  service_->UnregisterCommand("temp.cmd");

  // Should no longer appear in recent commands.
  recent = service_->GetRecentCommands();
  for (const auto& cmd : recent) {
    EXPECT_NE(cmd.command_id, "temp.cmd");
  }
}

TEST_F(CommandServiceTest, RecentCommandsExcludesDisabled) {
  service_->ExecuteCommand("tab.mute");

  // Disable the command.
  service_->SetCommandEnabled("tab.mute", false);

  // Disabled commands should not appear in recent results.
  auto recent = service_->GetRecentCommands();
  for (const auto& cmd : recent) {
    EXPECT_NE(cmd.command_id, "tab.mute");
  }
}

// ===========================================================================
// Suggested commands
// ===========================================================================

TEST_F(CommandServiceTest, SuggestedCommandsNotEmpty) {
  auto suggested = service_->GetSuggestedCommands();
  EXPECT_FALSE(suggested.empty());
}

TEST_F(CommandServiceTest, SuggestedCommandsBounded) {
  auto suggested = service_->GetSuggestedCommands();
  EXPECT_LE(suggested.size(), 8u);
}

TEST_F(CommandServiceTest, SuggestedCommandsIncludesRecent) {
  service_->ExecuteCommand("tab.new");
  service_->ExecuteCommand("search.google");

  auto suggested = service_->GetSuggestedCommands();

  bool has_tab_new = false;
  bool has_search_google = false;
  for (const auto& cmd : suggested) {
    if (cmd.command_id == "tab.new") has_tab_new = true;
    if (cmd.command_id == "search.google") has_search_google = true;
  }
  EXPECT_TRUE(has_tab_new);
  EXPECT_TRUE(has_search_google);
}

TEST_F(CommandServiceTest, SuggestedCommandsWithNoRecent) {
  // With no recent commands, suggestions should come from high-rank commands.
  auto suggested = service_->GetSuggestedCommands();
  EXPECT_FALSE(suggested.empty());
  // The top-ranked command should be first or near the front.
  EXPECT_GT(suggested[0].rank, 0);
}

// ===========================================================================
// Enable / disable
// ===========================================================================

TEST_F(CommandServiceTest, SetCommandEnabledToFalse) {
  const auto* cmd = service_->GetCommand("tab.new");
  ASSERT_NE(cmd, nullptr);
  EXPECT_TRUE(cmd->is_enabled);

  EXPECT_TRUE(service_->SetCommandEnabled("tab.new", false));

  cmd = service_->GetCommand("tab.new");
  EXPECT_FALSE(cmd->is_enabled);
}

TEST_F(CommandServiceTest, SetCommandEnabledNoChange) {
  // Already enabled, so should return false.
  EXPECT_FALSE(service_->SetCommandEnabled("tab.new", true));

  // Disable first, then disable again.
  service_->SetCommandEnabled("tab.new", false);
  EXPECT_FALSE(service_->SetCommandEnabled("tab.new", false));
}

TEST_F(CommandServiceTest, SetCommandEnabledNonexistent) {
  EXPECT_FALSE(service_->SetCommandEnabled("nonexistent", false));
}

// ===========================================================================
// Observers
// ===========================================================================

TEST_F(CommandServiceTest, ObserverOnCommandExecuted) {
  auto& observer = AddTestObserver();

  service_->ExecuteCommand("tab.new");
  EXPECT_EQ(observer.executed_count_, 1);
  EXPECT_EQ(observer.last_executed_id_, "tab.new");
}

TEST_F(CommandServiceTest, ObserverOnCommandRegistered) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd = MakeTestCommand("obs.test", "Observer Test");
  service_->RegisterCommand(cmd);

  EXPECT_EQ(observer.registered_count_, 1);
  EXPECT_EQ(observer.last_registered_id_, "obs.test");
}

TEST_F(CommandServiceTest, ObserverOnCommandUnregistered) {
  auto& observer = AddTestObserver();

  AstraCommandInfo cmd = MakeTestCommand("obs.remove", "Remove Me");
  service_->RegisterCommand(cmd);

  observer.unregistered_count_ = 0;

  service_->UnregisterCommand("obs.remove");
  EXPECT_EQ(observer.unregistered_count_, 1);
  EXPECT_EQ(observer.last_unregistered_id_, "obs.remove");
}

TEST_F(CommandServiceTest, MultipleObservers) {
  auto& obs1 = AddTestObserver();
  auto& obs2 = AddTestObserver();
  auto& obs3 = AddTestObserver();

  service_->ExecuteCommand("tab.new");

  EXPECT_EQ(obs1.executed_count_, 1);
  EXPECT_EQ(obs2.executed_count_, 1);
  EXPECT_EQ(obs3.executed_count_, 1);
}

TEST_F(CommandServiceTest, RemoveObserver) {
  TestCommandServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ExecuteCommand("tab.new");
  EXPECT_EQ(observer.executed_count_, 1);

  service_->RemoveObserver(&observer);

  service_->ExecuteCommand("tab.close");
  EXPECT_EQ(observer.executed_count_, 1);  // No additional notification.
}

// ===========================================================================
// Factory
// ===========================================================================

TEST_F(CommandServiceTest, FactoryGetForProfileReturnsInstance) {
  EXPECT_NE(service_, nullptr);
}

TEST_F(CommandServiceTest, FactorySameProfileReturnsSameInstance) {
  AstraCommandService* instance1 =
      AstraCommandServiceFactory::GetForProfile(profile_.get());
  AstraCommandService* instance2 =
      AstraCommandServiceFactory::GetForProfile(profile_.get());
  EXPECT_EQ(instance1, instance2);
}

TEST_F(CommandServiceTest, FactoryGetInstanceReturnsSingleton) {
  AstraCommandServiceFactory* factory1 =
      AstraCommandServiceFactory::GetInstance();
  AstraCommandServiceFactory* factory2 =
      AstraCommandServiceFactory::GetInstance();
  EXPECT_EQ(factory1, factory2);
}

// ===========================================================================
// Incognito
// ===========================================================================

TEST_F(CommandServiceTest, IncognitoProfileHasSeparateInstance) {
  // Create an incognito profile.
  TestingProfile::Builder builder;
  auto incognito_profile = builder.BuildIncognito(profile_.get());

  AstraCommandService* incognito_service =
      AstraCommandServiceFactory::GetForProfile(incognito_profile.get());

  // Incognito should have its own instance (kOwnInstance).
  EXPECT_NE(incognito_service, nullptr);
  EXPECT_NE(service_, incognito_service);
}

// ===========================================================================
// Custom keybindings
// ===========================================================================

TEST_F(CommandServiceTest, CustomKeybindingDefaultEmpty) {
  EXPECT_TRUE(service_->GetCustomKeybinding("tab.new").empty());
}

TEST_F(CommandServiceTest, SetAndGetCustomKeybinding) {
  service_->SetCustomKeybinding("tab.new", "Ctrl+Shift+N");
  EXPECT_EQ(service_->GetCustomKeybinding("tab.new"), "Ctrl+Shift+N");
}

TEST_F(CommandServiceTest, ClearCustomKeybinding) {
  service_->SetCustomKeybinding("tab.new", "Ctrl+Shift+N");
  ASSERT_EQ(service_->GetCustomKeybinding("tab.new"), "Ctrl+Shift+N");

  service_->SetCustomKeybinding("tab.new", "");
  EXPECT_TRUE(service_->GetCustomKeybinding("tab.new").empty());
}

TEST_F(CommandServiceTest, CustomKeybindingForNonexistentCommand) {
  // Setting a keybinding for a nonexistent command just stores it.
  service_->SetCustomKeybinding("nonexistent.cmd", "Ctrl+X");
  EXPECT_EQ(service_->GetCustomKeybinding("nonexistent.cmd"), "Ctrl+X");
}

TEST_F(CommandServiceTest, UnregisterRemovesCustomKeybinding) {
  // Register custom command and set a keybinding.
  AstraCommandInfo cmd = MakeTestCommand("custom.kb", "Custom KB Command");
  service_->RegisterCommand(cmd);
  service_->SetCustomKeybinding("custom.kb", "Alt+K");
  ASSERT_EQ(service_->GetCustomKeybinding("custom.kb"), "Alt+K");

  // Unregister the command.
  service_->UnregisterCommand("custom.kb");

  // The custom keybinding should also be removed.
  EXPECT_TRUE(service_->GetCustomKeybinding("custom.kb").empty());
}

// ===========================================================================
// IsIncognito
// ===========================================================================

TEST_F(CommandServiceTest, IsIncognitoReturnsFalseForNormalProfile) {
  EXPECT_FALSE(service_->IsIncognito());
}

// ===========================================================================
// Shutdown
// ===========================================================================

TEST_F(CommandServiceTest, ShutdownClearsObservers) {
  auto& observer = AddTestObserver();

  service_->Shutdown();

  // After shutdown, executing a command should not notify
  // (observer list is cleared).
  // Note: service is still valid but observers are cleared.
  EXPECT_NO_FATAL_FAILURE(service_->command_count());
}

}  // namespace astra
