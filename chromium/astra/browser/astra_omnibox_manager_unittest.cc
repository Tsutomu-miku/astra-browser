// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_omnibox_manager.h"

#include <algorithm>

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_omnibox_action.h"
#include "astra/browser/astra_omnibox_provider.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that tracks which notification methods were called.
class TestOmniboxObserver : public AstraOmniboxManager::Observer {
 public:
  void OnOmniboxActionExecuted(AstraOmniboxActionType action_type,
                               const std::string& payload,
                               bool success) override {
    action_executed_count_++;
    last_action_type_ = action_type;
    last_payload_ = payload;
    last_success_ = success;
  }

  void OnSuggestionsChanged() override {
    suggestions_changed_count_++;
  }

  void OnProviderEnabledChanged(bool enabled) override {
    provider_enabled_changed_count_++;
    last_provider_enabled_ = enabled;
  }

  void OnOmniboxSettingsChanged() override {
    settings_changed_count_++;
  }

  void OnRecentActionsChanged() override {
    recent_actions_changed_count_++;
  }

  // Reset all flags and counters.
  void Reset() {
    action_executed_count_ = 0;
    last_action_type_ = AstraOmniboxActionType::kNone;
    last_payload_.clear();
    last_success_ = false;
    suggestions_changed_count_ = 0;
    provider_enabled_changed_count_ = 0;
    last_provider_enabled_ = false;
    settings_changed_count_ = 0;
    recent_actions_changed_count_ = 0;
  }

  // Counters and state.
  int action_executed_count_ = 0;
  AstraOmniboxActionType last_action_type_ = AstraOmniboxActionType::kNone;
  std::string last_payload_;
  bool last_success_ = false;

  int suggestions_changed_count_ = 0;
  int provider_enabled_changed_count_ = 0;
  bool last_provider_enabled_ = false;
  int settings_changed_count_ = 0;
  int recent_actions_changed_count_ = 0;
};

}  // namespace

// ---------------------------------------------------------------------------
// Action type and category tests (no Profile needed)
// ---------------------------------------------------------------------------

TEST(OmniboxManagerTest, ActionCategory_BasicMapping) {
  // Verify each action type maps to the expected category.
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kSwitchWorkspace),
            AstraOmniboxActionCategory::kWorkspace);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kSearchTabs),
            AstraOmniboxActionCategory::kTab);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kSearchFavorites),
            AstraOmniboxActionCategory::kTab);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kOpenCommandPalette),
            AstraOmniboxActionCategory::kNavigation);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kToggleSplitView),
            AstraOmniboxActionCategory::kTool);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kToggleFocusMode),
            AstraOmniboxActionCategory::kTool);
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kScreenshot),
            AstraOmniboxActionCategory::kTool);
  // kRunCommand defaults to Navigation category.
  EXPECT_EQ(GetActionCategory(AstraOmniboxActionType::kRunCommand),
            AstraOmniboxActionCategory::kNavigation);
}

TEST(OmniboxManagerTest, ActionCategoryLabel_NonEmpty) {
  EXPECT_NE(nullptr, GetActionCategoryLabel(
                         AstraOmniboxActionCategory::kWorkspace));
  EXPECT_NE(nullptr, GetActionCategoryLabel(AstraOmniboxActionCategory::kTab));
  EXPECT_NE(nullptr,
            GetActionCategoryLabel(AstraOmniboxActionCategory::kNavigation));
  EXPECT_NE(nullptr, GetActionCategoryLabel(AstraOmniboxActionCategory::kTool));

  // Labels should be non-empty strings.
  EXPECT_GT(strlen(GetActionCategoryLabel(AstraOmniboxActionCategory::kWorkspace)), 0u);
}

TEST(OmniboxManagerTest, ActionTypeLabel_NonEmpty) {
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kSwitchWorkspace).empty());
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kSearchTabs).empty());
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kRunCommand).empty());
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kToggleSplitView).empty());
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kToggleFocusMode).empty());
  EXPECT_FALSE(GetActionTypeLabel(AstraOmniboxActionType::kScreenshot).empty());

  // kNone should return empty string.
  EXPECT_TRUE(GetActionTypeLabel(AstraOmniboxActionType::kNone).empty());
}

// ---------------------------------------------------------------------------
// Action catalog tests
// ---------------------------------------------------------------------------

TEST(OmniboxManagerTest, ActionCatalog_NonEmpty) {
  auto actions = GetAllOmniboxActions();
  EXPECT_FALSE(actions.empty());
  EXPECT_GT(actions.size(), 5u);
}

TEST(OmniboxManagerTest, ActionCatalog_AllHaveIds) {
  auto actions = GetAllOmniboxActions();
  for (const auto& action : actions) {
    EXPECT_FALSE(action.id.empty()) << "Action has empty id";
  }
}

TEST(OmniboxManagerTest, ActionCatalog_AllHaveTitles) {
  auto actions = GetAllOmniboxActions();
  for (const auto& action : actions) {
    EXPECT_FALSE(action.title.empty())
        << "Action '" << action.id << "' has empty title";
  }
}

TEST(OmniboxManagerTest, ActionCatalog_AllHaveValidTypes) {
  auto actions = GetAllOmniboxActions();
  for (const auto& action : actions) {
    EXPECT_NE(action.type, AstraOmniboxActionType::kNone)
        << "Action '" << action.id << "' has kNone type";
  }
}

TEST(OmniboxManagerTest, ActionCatalog_AllHaveCategories) {
  auto actions = GetAllOmniboxActions();
  for (const auto& action : actions) {
    // Category should match what GetActionCategory returns for the type.
    EXPECT_EQ(action.category, GetActionCategory(action.type))
        << "Action '" << action.id << "' category mismatch";
  }
}

TEST(OmniboxManagerTest, ActionCatalog_NoDuplicateIds) {
  auto actions = GetAllOmniboxActions();
  std::vector<std::string> ids;
  for (const auto& action : actions) {
    ids.push_back(action.id);
  }
  std::sort(ids.begin(), ids.end());
  for (size_t i = 1; i < ids.size(); ++i) {
    EXPECT_NE(ids[i], ids[i - 1]) << "Duplicate action id: " << ids[i];
  }
}

TEST(OmniboxManagerTest, GetActionsByCategory_Workspace) {
  auto actions = GetActionsByCategory(AstraOmniboxActionCategory::kWorkspace);
  EXPECT_FALSE(actions.empty());
  for (const auto& action : actions) {
    EXPECT_EQ(action.category, AstraOmniboxActionCategory::kWorkspace);
  }
}

TEST(OmniboxManagerTest, GetActionsByCategory_Tool) {
  auto actions = GetActionsByCategory(AstraOmniboxActionCategory::kTool);
  EXPECT_FALSE(actions.empty());
  for (const auto& action : actions) {
    EXPECT_EQ(action.category, AstraOmniboxActionCategory::kTool);
  }
}

TEST(OmniboxManagerTest, GetActionMetadata_ValidType) {
  AstraOmniboxAction action;
  EXPECT_TRUE(GetActionMetadata(AstraOmniboxActionType::kToggleSplitView,
                                &action));
  EXPECT_EQ(action.type, AstraOmniboxActionType::kToggleSplitView);
  EXPECT_FALSE(action.title.empty());
}

TEST(OmniboxManagerTest, GetActionMetadata_NoneType) {
  AstraOmniboxAction action;
  EXPECT_FALSE(GetActionMetadata(AstraOmniboxActionType::kNone, &action));
}

TEST(OmniboxManagerTest, GetActionMetadata_NullOutput) {
  EXPECT_FALSE(GetActionMetadata(AstraOmniboxActionType::kToggleSplitView,
                                 nullptr));
}

// ---------------------------------------------------------------------------
// Action search tests
// ---------------------------------------------------------------------------

TEST(OmniboxManagerTest, SearchActions_EmptyQuery_ReturnsAll) {
  auto results = SearchActions(u"");
  // Empty query should return all actions.
  EXPECT_EQ(results.size(), GetAllOmniboxActions().size());
}

TEST(OmniboxManagerTest, SearchActions_PrefixMatch) {
  // "Screenshot" should match multiple screenshot actions.
  auto results = SearchActions(u"screenshot");
  EXPECT_FALSE(results.empty());
  EXPECT_GT(results.size(), 1u);
}

TEST(OmniboxManagerTest, SearchActions_CaseInsensitive) {
  auto results1 = SearchActions(u"Toggle");
  auto results2 = SearchActions(u"toggle");
  EXPECT_EQ(results1.size(), results2.size());
}

TEST(OmniboxManagerTest, SearchActions_NoMatch) {
  auto results = SearchActions(u"xyz_nonexistent_action_123");
  EXPECT_TRUE(results.empty());
}

TEST(OmniboxManagerTest, SearchActionsInCategory_Filters) {
  auto all = SearchActions(u"toggle");
  auto tool_only =
      SearchActionsInCategory(AstraOmniboxActionCategory::kTool, u"toggle");

  EXPECT_FALSE(all.empty());
  EXPECT_FALSE(tool_only.empty());
  EXPECT_LE(tool_only.size(), all.size());

  for (const auto& action : tool_only) {
    EXPECT_EQ(action.category, AstraOmniboxActionCategory::kTool);
  }
}

// ---------------------------------------------------------------------------
// AstraOmniboxSuggestion struct tests
// ---------------------------------------------------------------------------

TEST(OmniboxManagerTest, SuggestionStruct_DefaultValues) {
  AstraOmniboxSuggestion suggestion;
  EXPECT_TRUE(suggestion.display_text.empty());
  EXPECT_TRUE(suggestion.description.empty());
  EXPECT_EQ(suggestion.action_type, AstraOmniboxActionType::kNone);
  EXPECT_EQ(suggestion.category, AstraOmniboxActionCategory::kTool);
  EXPECT_TRUE(suggestion.payload.empty());
  EXPECT_EQ(suggestion.relevance, 700);
}

// ---------------------------------------------------------------------------
// Provider tests (no Profile needed for basic tests)
// ---------------------------------------------------------------------------

TEST(OmniboxManagerTest, Provider_DefaultEnabled) {
  AstraOmniboxProvider provider;
  EXPECT_TRUE(provider.enabled());
}

TEST(OmniboxManagerTest, Provider_SetEnabled) {
  AstraOmniboxProvider provider;
  provider.set_enabled(false);
  EXPECT_FALSE(provider.enabled());

  // Disabled provider should return empty suggestions.
  auto suggestions = provider.GetSuggestions(nullptr, u">sidebar");
  EXPECT_TRUE(suggestions.empty());

  provider.set_enabled(true);
  EXPECT_TRUE(provider.enabled());
}

TEST(OmniboxManagerTest, Provider_DefaultMaxSuggestions) {
  AstraOmniboxProvider provider;
  EXPECT_EQ(provider.max_suggestions(),
            AstraOmniboxProvider::kDefaultMaxSuggestions);
}

TEST(OmniboxManagerTest, Provider_SetMaxSuggestions) {
  AstraOmniboxProvider provider;
  provider.set_max_suggestions(3);
  EXPECT_EQ(provider.max_suggestions(), 3u);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_Workspace) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@workspace "),
            AstraOmniboxProvider::PrefixKind::kWorkspace);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@WORKSPACE test"),
            AstraOmniboxProvider::PrefixKind::kWorkspace);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_Command) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"> "),
            AstraOmniboxProvider::PrefixKind::kCommand);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u">sidebar"),
            AstraOmniboxProvider::PrefixKind::kCommand);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_Tab) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@tab "),
            AstraOmniboxProvider::PrefixKind::kTab);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@Tab test"),
            AstraOmniboxProvider::PrefixKind::kTab);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_Focus) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@focus"),
            AstraOmniboxProvider::PrefixKind::kFocus);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_Screenshot) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"@screenshot"),
            AstraOmniboxProvider::PrefixKind::kScreenshot);
}

TEST(OmniboxManagerTest, Provider_GetPrefixKind_None) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u"hello world"),
            AstraOmniboxProvider::PrefixKind::kNone);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixKind(u""),
            AstraOmniboxProvider::PrefixKind::kNone);
}

TEST(OmniboxManagerTest, Provider_MatchesQuery) {
  EXPECT_TRUE(AstraOmniboxProvider::MatchesQuery(u"@workspace "));
  EXPECT_TRUE(AstraOmniboxProvider::MatchesQuery(u">test"));
  EXPECT_TRUE(AstraOmniboxProvider::MatchesQuery(u"@tab foo"));
  EXPECT_FALSE(AstraOmniboxProvider::MatchesQuery(u"not a prefix"));
  EXPECT_FALSE(AstraOmniboxProvider::MatchesQuery(u""));
}

TEST(OmniboxManagerTest, Provider_ExtractQuery) {
  EXPECT_EQ(AstraOmniboxProvider::ExtractQuery(u"@workspace Test"), u"Test");
  EXPECT_EQ(AstraOmniboxProvider::ExtractQuery(u">sidebar"), u"sidebar");
  EXPECT_EQ(AstraOmniboxProvider::ExtractQuery(u"@tab  "), u"");
  // No prefix — returns entire string.
  EXPECT_EQ(AstraOmniboxProvider::ExtractQuery(u"hello"), u"hello");
}

TEST(OmniboxManagerTest, Provider_GetPrefixLabel) {
  EXPECT_FALSE(AstraOmniboxProvider::GetPrefixLabel(
      AstraOmniboxProvider::PrefixKind::kWorkspace).empty());
  EXPECT_FALSE(AstraOmniboxProvider::GetPrefixLabel(
      AstraOmniboxProvider::PrefixKind::kCommand).empty());
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixLabel(
                AstraOmniboxProvider::PrefixKind::kNone),
            u"");
}

TEST(OmniboxManagerTest, Provider_GetPrefixCategory) {
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixCategory(
                AstraOmniboxProvider::PrefixKind::kWorkspace),
            AstraOmniboxActionCategory::kWorkspace);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixCategory(
                AstraOmniboxProvider::PrefixKind::kTab),
            AstraOmniboxActionCategory::kTab);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixCategory(
                AstraOmniboxProvider::PrefixKind::kCommand),
            AstraOmniboxActionCategory::kNavigation);
  EXPECT_EQ(AstraOmniboxProvider::GetPrefixCategory(
                AstraOmniboxProvider::PrefixKind::kSplit),
            AstraOmniboxActionCategory::kTool);
}

// ===========================================================================
// Manager tests with Profile (settings, persistence, recent actions)
// ===========================================================================

class OmniboxManagerProfileTest : public testing::Test {
 protected:
  OmniboxManagerProfileTest() {
    profile_ = std::make_unique<TestingProfile>();
    DCHECK(profile_);
    // The profile registers all Astra prefs via AstraWorkspaceServiceFactory
    // or the testing profile's pref registration.
    // For the overlay skeleton, we explicitly register them.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());

    manager_ = std::make_unique<AstraOmniboxManager>(profile_.get());
  }

  ~OmniboxManagerProfileTest() override = default;

  void SetUp() override {
    // Ensure manager starts with default state.
    ASSERT_TRUE(manager_->provider_enabled());
    ASSERT_TRUE(manager_->show_astra_suggestions());
  }

  void TearDown() override {
    // Clean up any registered observers.
    for (auto* observer : test_observers_) {
      manager_->RemoveObserver(observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraOmniboxManager> manager_;
  std::vector<TestOmniboxObserver*> test_observers_;
};

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, Default_ProviderEnabled) {
  EXPECT_TRUE(manager_->provider_enabled());
}

TEST_F(OmniboxManagerProfileTest, Default_ShowAstraSuggestions) {
  EXPECT_TRUE(manager_->show_astra_suggestions());
}

TEST_F(OmniboxManagerProfileTest, Default_MaxAstraSuggestions) {
  EXPECT_EQ(manager_->max_astra_suggestions(),
            prefs::kDefaultOmniboxMaxAstraSuggestions);
  EXPECT_GT(manager_->max_astra_suggestions(), 0);
}

TEST_F(OmniboxManagerProfileTest, Default_SuggestionPosition) {
  EXPECT_EQ(manager_->suggestion_position(),
            prefs::kDefaultOmniboxSuggestionPosition);
  EXPECT_FALSE(manager_->suggestion_position().empty());
}

TEST_F(OmniboxManagerProfileTest, Default_AllCategoriesEnabled) {
  EXPECT_TRUE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));
  EXPECT_TRUE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTab));
  EXPECT_TRUE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kNavigation));
  EXPECT_TRUE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTool));
}

TEST_F(OmniboxManagerProfileTest, Default_RecentActionsEmpty) {
  auto recent = manager_->GetRecentActions();
  EXPECT_TRUE(recent.empty());
}

TEST_F(OmniboxManagerProfileTest, Default_MaxRecentActions) {
  EXPECT_EQ(manager_->max_recent_actions(),
            prefs::kDefaultOmniboxMaxRecentActions);
  EXPECT_GT(manager_->max_recent_actions(), 0);
}

// ---------------------------------------------------------------------------
// Settings get/set
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, SetProviderEnabled_ChangesValue) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->SetProviderEnabled(false);
  EXPECT_FALSE(manager_->provider_enabled());
  EXPECT_GT(observer.provider_enabled_changed_count_, 0);
  EXPECT_FALSE(observer.last_provider_enabled_);
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetProviderEnabled_SameValue_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  // Already enabled — setting to true should be no-op.
  manager_->SetProviderEnabled(true);
  EXPECT_EQ(observer.provider_enabled_changed_count_, 0);
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetShowAstraSuggestions_ChangesValue) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->SetShowAstraSuggestions(false);
  EXPECT_FALSE(manager_->show_astra_suggestions());
  EXPECT_GT(observer.settings_changed_count_, 0);
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetShowAstraSuggestions_SameValue_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->SetShowAstraSuggestions(true);
  EXPECT_EQ(observer.settings_changed_count_, 0);
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetMaxAstraSuggestions_ChangesValue) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  int new_max = manager_->max_astra_suggestions() + 3;
  manager_->SetMaxAstraSuggestions(new_max);
  EXPECT_EQ(manager_->max_astra_suggestions(), new_max);
  EXPECT_GT(observer.settings_changed_count_, 0);
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetMaxAstraSuggestions_SameValue_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  int current = manager_->max_astra_suggestions();
  manager_->SetMaxAstraSuggestions(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetMaxAstraSuggestions_ClampsLow) {
  manager_->SetMaxAstraSuggestions(0);
  EXPECT_GE(manager_->max_astra_suggestions(), 1);
}

TEST_F(OmniboxManagerProfileTest, SetMaxAstraSuggestions_ClampsHigh) {
  manager_->SetMaxAstraSuggestions(100);
  EXPECT_LE(manager_->max_astra_suggestions(), 20);
}

TEST_F(OmniboxManagerProfileTest, SetSuggestionPosition_ValidTop) {
  manager_->SetSuggestionPosition("top");
  EXPECT_EQ(manager_->suggestion_position(), "top");
}

TEST_F(OmniboxManagerProfileTest, SetSuggestionPosition_ValidBottom) {
  manager_->SetSuggestionPosition("bottom");
  EXPECT_EQ(manager_->suggestion_position(), "bottom");
}

TEST_F(OmniboxManagerProfileTest, SetSuggestionPosition_Invalid_NoOp) {
  std::string original = manager_->suggestion_position();
  manager_->SetSuggestionPosition("invalid_value");
  EXPECT_EQ(manager_->suggestion_position(), original);
}

TEST_F(OmniboxManagerProfileTest, SetSuggestionPosition_SameValue_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  std::string current = manager_->suggestion_position();
  manager_->SetSuggestionPosition(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);
}

// ---------------------------------------------------------------------------
// Category filtering
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, SetCategoryEnabled_DisablesWorkspace) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->SetCategoryEnabled(AstraOmniboxActionCategory::kWorkspace, false);
  EXPECT_FALSE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));
  // Other categories should still be enabled.
  EXPECT_TRUE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTab));
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetCategoryEnabled_SameValue_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->SetCategoryEnabled(AstraOmniboxActionCategory::kWorkspace, true);
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, DisableAllCategories) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->DisableAllCategories();
  EXPECT_FALSE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));
  EXPECT_FALSE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTab));
  EXPECT_FALSE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kNavigation));
  EXPECT_FALSE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTool));
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, EnableAllCategories) {
  // First disable all.
  manager_->DisableAllCategories();
  ASSERT_FALSE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));

  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->EnableAllCategories();
  EXPECT_TRUE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));
  EXPECT_TRUE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTab));
  EXPECT_TRUE(manager_->IsCategoryEnabled(
      AstraOmniboxActionCategory::kNavigation));
  EXPECT_TRUE(
      manager_->IsCategoryEnabled(AstraOmniboxActionCategory::kTool));
  EXPECT_GT(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, EnableAll_WhenAllAlreadyEnabled_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->EnableAllCategories();
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, DisableAll_WhenAllAlreadyDisabled_NoOp) {
  manager_->DisableAllCategories();

  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->DisableAllCategories();
  EXPECT_EQ(observer.suggestions_changed_count_, 0);
}

// ---------------------------------------------------------------------------
// Recent actions
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, AddRecentAction_AddsToList) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  auto recent = manager_->GetRecentActions();
  ASSERT_EQ(recent.size(), 1u);
  EXPECT_EQ(recent[0], AstraOmniboxActionType::kToggleSplitView);
  EXPECT_GT(observer.recent_actions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, AddRecentAction_MovesToFront) {
  // Add two actions.
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  manager_->AddRecentAction(AstraOmniboxActionType::kSwitchWorkspace);

  auto recent1 = manager_->GetRecentActions();
  ASSERT_EQ(recent1.size(), 2u);
  EXPECT_EQ(recent1[0], AstraOmniboxActionType::kSwitchWorkspace);
  EXPECT_EQ(recent1[1], AstraOmniboxActionType::kToggleSplitView);

  // Now add the first one again — it should move to front.
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);

  auto recent2 = manager_->GetRecentActions();
  ASSERT_EQ(recent2.size(), 2u);
  EXPECT_EQ(recent2[0], AstraOmniboxActionType::kToggleSplitView);
  EXPECT_EQ(recent2[1], AstraOmniboxActionType::kSwitchWorkspace);
}

TEST_F(OmniboxManagerProfileTest, AddRecentAction_NoneType_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->AddRecentAction(AstraOmniboxActionType::kNone);
  auto recent = manager_->GetRecentActions();
  EXPECT_TRUE(recent.empty());
  EXPECT_EQ(observer.recent_actions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, AddRecentAction_WithPayload) {
  manager_->AddRecentAction(AstraOmniboxActionType::kRunCommand, "60000");
  auto details = manager_->GetRecentActionDetails();
  ASSERT_FALSE(details.empty());
  // The detail should have the payload from the recent action.
  EXPECT_EQ(details[0].default_payload, "60000");
}

TEST_F(OmniboxManagerProfileTest, ClearRecentActions_EmptiesList) {
  // Add some actions first.
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  manager_->AddRecentAction(AstraOmniboxActionType::kSwitchWorkspace);
  ASSERT_FALSE(manager_->GetRecentActions().empty());

  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->ClearRecentActions();
  EXPECT_TRUE(manager_->GetRecentActions().empty());
  EXPECT_GT(observer.recent_actions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, ClearRecentActions_EmptyList_NoOp) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->ClearRecentActions();
  EXPECT_EQ(observer.recent_actions_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, SetMaxRecentActions_TruncatesList) {
  // Add more actions than the new max.
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  manager_->AddRecentAction(AstraOmniboxActionType::kSwitchWorkspace);
  manager_->AddRecentAction(AstraOmniboxActionType::kSearchTabs);
  manager_->AddRecentAction(AstraOmniboxActionType::kScreenshot);
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleFocusMode);
  ASSERT_GE(manager_->GetRecentActions().size(), 5u);

  manager_->SetMaxRecentActions(2);
  auto recent = manager_->GetRecentActions();
  EXPECT_EQ(recent.size(), 2u);
  // First two (most recent) should be preserved.
  EXPECT_EQ(recent[0], AstraOmniboxActionType::kToggleFocusMode);
  EXPECT_EQ(recent[1], AstraOmniboxActionType::kScreenshot);
}

TEST_F(OmniboxManagerProfileTest, SetMaxRecentActions_Clamps) {
  manager_->SetMaxRecentActions(0);
  EXPECT_GE(manager_->max_recent_actions(), 1);

  manager_->SetMaxRecentActions(1000);
  EXPECT_LE(manager_->max_recent_actions(), 50);
}

TEST_F(OmniboxManagerProfileTest, GetRecentActionDetails_HasMetadata) {
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);

  auto details = manager_->GetRecentActionDetails();
  ASSERT_FALSE(details.empty());
  EXPECT_FALSE(details[0].title.empty());
  EXPECT_FALSE(details[0].description.empty());
  EXPECT_EQ(details[0].type, AstraOmniboxActionType::kToggleSplitView);
}

// ---------------------------------------------------------------------------
// Observer tests
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraOmniboxManager::Observer {};

  DefaultObserver observer;
  manager_->AddObserver(&observer);

  // Trigger notifications — the default observer should not crash.
  manager_->SetProviderEnabled(false);
  manager_->SetShowAstraSuggestions(false);
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);

  manager_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

TEST_F(OmniboxManagerProfileTest, Observer_AddAndRemove) {
  TestOmniboxObserver observer;

  manager_->AddObserver(&observer);
  manager_->RemoveObserver(&observer);

  SUCCEED() << "AddObserver / RemoveObserver completed without crash.";
}

TEST_F(OmniboxManagerProfileTest, Observer_RemoveNonexistent_NoCrash) {
  TestOmniboxObserver observer;
  manager_->RemoveObserver(&observer);
  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST_F(OmniboxManagerProfileTest, Observer_MultipleObservers) {
  TestOmniboxObserver observer1;
  TestOmniboxObserver observer2;

  manager_->AddObserver(&observer1);
  manager_->AddObserver(&observer2);
  test_observers_.push_back(&observer1);
  test_observers_.push_back(&observer2);

  manager_->SetProviderEnabled(false);

  // Both observers should have received the notification.
  EXPECT_GT(observer1.provider_enabled_changed_count_, 0);
  EXPECT_GT(observer2.provider_enabled_changed_count_, 0);
}

TEST_F(OmniboxManagerProfileTest, Observer_NullAdd_NoCrash) {
  manager_->AddObserver(nullptr);
  manager_->RemoveObserver(nullptr);
  SUCCEED() << "Adding/removing null observer does not crash.";
}

// ---------------------------------------------------------------------------
// Action execution (with null browser)
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, ExecuteAction_NullBrowser_ReturnsFalse) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  bool result =
      manager_->ExecuteAction(nullptr, AstraOmniboxActionType::kToggleSplitView,
                              "");
  EXPECT_FALSE(result);

  // Observer should still be notified.
  EXPECT_GT(observer.action_executed_count_, 0);
  EXPECT_EQ(observer.last_action_type_,
            AstraOmniboxActionType::kToggleSplitView);
  EXPECT_FALSE(observer.last_success_);
}

TEST_F(OmniboxManagerProfileTest, ExecuteAction_NoneType_NoRecent) {
  TestOmniboxObserver observer;
  manager_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  manager_->ExecuteAction(nullptr, AstraOmniboxActionType::kNone, "");
  auto recent = manager_->GetRecentActions();
  EXPECT_TRUE(recent.empty());
}

TEST_F(OmniboxManagerProfileTest, ExecuteSuggestion_NullBrowser) {
  AstraOmniboxSuggestion suggestion;
  suggestion.action_type = AstraOmniboxActionType::kToggleSplitView;
  suggestion.payload = "";

  bool result = manager_->ExecuteSuggestion(nullptr, suggestion);
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Action catalog access through manager
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, GetAllActions_NonEmpty) {
  auto actions = manager_->GetAllActions();
  EXPECT_FALSE(actions.empty());
}

TEST_F(OmniboxManagerProfileTest, GetActionsByCategory_ThroughManager) {
  auto actions =
      manager_->GetActionsByCategory(AstraOmniboxActionCategory::kTool);
  EXPECT_FALSE(actions.empty());
  for (const auto& action : actions) {
    EXPECT_EQ(action.category, AstraOmniboxActionCategory::kTool);
  }
}

TEST_F(OmniboxManagerProfileTest, SearchActions_ThroughManager) {
  auto results = manager_->SearchActions(u"screenshot");
  EXPECT_FALSE(results.empty());
}

// ---------------------------------------------------------------------------
// Persistence round-trip tests
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, Persistence_ProviderEnabled) {
  // Change value.
  manager_->SetProviderEnabled(false);
  EXPECT_FALSE(manager_->provider_enabled());

  // Verify it's in prefs.
  EXPECT_FALSE(profile_->GetPrefs()->GetBoolean(
      prefs::kPrefOmniboxProviderEnabled));

  // Create a new manager reading from the same profile — should pick up
  // the persisted value.
  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_FALSE(manager2->provider_enabled());
}

TEST_F(OmniboxManagerProfileTest, Persistence_ShowAstraSuggestions) {
  manager_->SetShowAstraSuggestions(false);

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_FALSE(manager2->show_astra_suggestions());
}

TEST_F(OmniboxManagerProfileTest, Persistence_MaxAstraSuggestions) {
  manager_->SetMaxAstraSuggestions(8);

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_EQ(manager2->max_astra_suggestions(), 8);
}

TEST_F(OmniboxManagerProfileTest, Persistence_SuggestionPosition) {
  manager_->SetSuggestionPosition("top");

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_EQ(manager2->suggestion_position(), "top");
}

TEST_F(OmniboxManagerProfileTest, Persistence_CategoryEnabled) {
  manager_->SetCategoryEnabled(AstraOmniboxActionCategory::kWorkspace, false);
  manager_->SetCategoryEnabled(AstraOmniboxActionCategory::kTool, false);

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_FALSE(manager2->IsCategoryEnabled(
      AstraOmniboxActionCategory::kWorkspace));
  EXPECT_TRUE(
      manager2->IsCategoryEnabled(AstraOmniboxActionCategory::kTab));
  EXPECT_TRUE(manager2->IsCategoryEnabled(
      AstraOmniboxActionCategory::kNavigation));
  EXPECT_FALSE(
      manager2->IsCategoryEnabled(AstraOmniboxActionCategory::kTool));
}

TEST_F(OmniboxManagerProfileTest, Persistence_RecentActions) {
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  manager_->AddRecentAction(AstraOmniboxActionType::kSwitchWorkspace);
  manager_->AddRecentAction(AstraOmniboxActionType::kScreenshot);

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  auto recent = manager2->GetRecentActions();
  EXPECT_EQ(recent.size(), 3u);
  // Most recent first.
  EXPECT_EQ(recent[0], AstraOmniboxActionType::kScreenshot);
  EXPECT_EQ(recent[1], AstraOmniboxActionType::kSwitchWorkspace);
  EXPECT_EQ(recent[2], AstraOmniboxActionType::kToggleSplitView);
}

TEST_F(OmniboxManagerProfileTest, Persistence_MaxRecentActions) {
  manager_->SetMaxRecentActions(3);

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  EXPECT_EQ(manager2->max_recent_actions(), 3);
}

TEST_F(OmniboxManagerProfileTest, Persistence_RecentActionsWithPayload) {
  manager_->AddRecentAction(AstraOmniboxActionType::kRunCommand, "60001");

  auto manager2 = std::make_unique<AstraOmniboxManager>(profile_.get());
  auto details = manager2->GetRecentActionDetails();
  ASSERT_FALSE(details.empty());
  EXPECT_EQ(details[0].default_payload, "60001");
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(OmniboxManagerProfileTest, MatchesAstraPrefix_EmptyString) {
  EXPECT_FALSE(manager_->MatchesAstraPrefix(u""));
}

TEST_F(OmniboxManagerProfileTest, MatchesAstraPrefix_PlainText) {
  EXPECT_FALSE(manager_->MatchesAstraPrefix(u"hello world"));
}

TEST_F(OmniboxManagerProfileTest, MatchesAstraPrefix_Workspace) {
  EXPECT_TRUE(manager_->MatchesAstraPrefix(u"@workspace test"));
}

TEST_F(OmniboxManagerProfileTest, GetSuggestions_DisabledProvider_Empty) {
  manager_->SetProviderEnabled(false);
  auto suggestions = manager_->GetSuggestions(nullptr, u">sidebar");
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(OmniboxManagerProfileTest, GetSuggestions_DisabledShowSuggestions_Empty) {
  manager_->SetShowAstraSuggestions(false);
  auto suggestions = manager_->GetSuggestions(nullptr, u">sidebar");
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(OmniboxManagerProfileTest, GetSuggestions_NoPrefix_Empty) {
  auto suggestions = manager_->GetSuggestions(nullptr, u"no prefix here");
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(OmniboxManagerProfileTest, ProviderAccess_NonNull) {
  EXPECT_NE(manager_->provider(), nullptr);
  EXPECT_NE(
      const_cast<const AstraOmniboxManager*>(manager_.get())->provider(),
      nullptr);
}

TEST_F(OmniboxManagerProfileTest, ActionCategoryEnum_Values) {
  // Verify enum has expected values and ordering.
  EXPECT_EQ(static_cast<int>(AstraOmniboxActionCategory::kWorkspace), 0);
  EXPECT_EQ(static_cast<int>(AstraOmniboxActionCategory::kTab), 1);
  EXPECT_EQ(static_cast<int>(AstraOmniboxActionCategory::kNavigation), 2);
  EXPECT_EQ(static_cast<int>(AstraOmniboxActionCategory::kTool), 3);
}

TEST_F(OmniboxManagerProfileTest, GetRecentActionDetails_EmptyWhenNoActions) {
  auto details = manager_->GetRecentActionDetails();
  EXPECT_TRUE(details.empty());
}

TEST_F(OmniboxManagerProfileTest, ClearRecentActions_ThenAdd) {
  manager_->AddRecentAction(AstraOmniboxActionType::kToggleSplitView);
  manager_->ClearRecentActions();
  ASSERT_TRUE(manager_->GetRecentActions().empty());

  // Adding after clear should work normally.
  manager_->AddRecentAction(AstraOmniboxActionType::kSwitchWorkspace);
  auto recent = manager_->GetRecentActions();
  ASSERT_EQ(recent.size(), 1u);
  EXPECT_EQ(recent[0], AstraOmniboxActionType::kSwitchWorkspace);
}

}  // namespace astra
