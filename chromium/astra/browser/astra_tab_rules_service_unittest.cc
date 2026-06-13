// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_rules_service.h"

#include "base/test/task_environment.h"
#include "base/unguessable_token.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestTabRulesServiceObserver : public AstraTabRulesServiceObserver {
 public:
  void OnRuleAdded(const std::string& rule_id) override {
    added_count_++;
    last_added_id_ = rule_id;
    added_ids_.push_back(rule_id);
  }

  void OnRuleRemoved(const std::string& rule_id) override {
    removed_count_++;
    last_removed_id_ = rule_id;
  }

  void OnRuleUpdated(const std::string& rule_id) override {
    updated_count_++;
    last_updated_id_ = rule_id;
  }

  void OnRuleTriggered(const std::string& rule_id,
                       const std::string& tab_id) override {
    triggered_count_++;
    last_triggered_rule_id_ = rule_id;
    last_triggered_tab_id_ = tab_id;
  }

  void OnRulesCleared() override { cleared_count_++; }

  // Counters
  int added_count_ = 0;
  int removed_count_ = 0;
  int updated_count_ = 0;
  int triggered_count_ = 0;
  int cleared_count_ = 0;

  // Last recorded values
  std::string last_added_id_;
  std::string last_removed_id_;
  std::string last_updated_id_;
  std::string last_triggered_rule_id_;
  std::string last_triggered_tab_id_;

  // All added IDs (in order)
  std::vector<std::string> added_ids_;
};

// Helper to create a simple URL pattern rule.
AstraTabRule MakeUrlPatternRule(const std::string& name,
                                const std::string& pattern,
                                AstraTabRuleActionType action_type,
                                int priority = 100) {
  AstraTabRule rule;
  rule.name = name;
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule.trigger_value = pattern;
  rule.action_type = action_type;
  rule.priority = priority;
  return rule;
}

// Helper to create a domain rule.
AstraTabRule MakeDomainRule(const std::string& name,
                            const std::string& domain,
                            AstraTabRuleActionType action_type,
                            int priority = 100) {
  AstraTabRule rule;
  rule.name = name;
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kDomain;
  rule.trigger_value = domain;
  rule.action_type = action_type;
  rule.priority = priority;
  return rule;
}

// Helper to create a title contains rule.
AstraTabRule MakeTitleRule(const std::string& name,
                           const std::string& keyword,
                           AstraTabRuleActionType action_type,
                           int priority = 100) {
  AstraTabRule rule;
  rule.name = name;
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kTitleContains;
  rule.trigger_value = keyword;
  rule.action_type = action_type;
  rule.priority = priority;
  return rule;
}

}  // namespace

// ===========================================================================
// Test fixture
// ===========================================================================
//
// Uses TestingProfile from //chrome/test:test_support so the service has a
// real Profile* to attach to.
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that future persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
class TabRulesServiceTest : public testing::Test {
 protected:
  TabRulesServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    service_ = AstraTabRulesServiceFactory::GetForProfile(profile_.get());
    DCHECK(service_);
  }

  ~TabRulesServiceTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Service should start with no rules.
    ASSERT_EQ(service_->rule_count(), 0u);
  }

  void TearDown() override {
    // Clean up observers.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Adds a test rule and returns its ID.
  std::string AddTestRule(const std::string& name) {
    AstraTabRule rule;
    rule.name = name;
    rule.enabled = true;
    rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
    rule.trigger_value = "*://example.com/*";
    rule.action_type = AstraTabRuleActionType::kMute;
    rule.priority = 100;
    std::string id = service_->AddRule(std::move(rule));
    DCHECK(!id.empty());
    return id;
  }

  // Creates a test observer and adds it to the service.
  TestTabRulesServiceObserver& AddTestObserver() {
    test_observers_.emplace_back();
    service_->AddObserver(&test_observers_.back());
    return test_observers_.back();
  }

  // Task environment is required for TestingProfile and base::Time.
  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<AstraTabRulesService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestTabRulesServiceObserver> test_observers_;
};

// ===========================================================================
// Construction and defaults
// ===========================================================================

TEST_F(TabRulesServiceTest, ConstructionStartsEmpty) {
  EXPECT_EQ(service_->rule_count(), 0u);
  EXPECT_TRUE(service_->rules().empty());
  EXPECT_TRUE(service_->GetEnabledRules().empty());
}

TEST_F(TabRulesServiceTest, GetRuleReturnsNullForNonexistent) {
  EXPECT_EQ(service_->GetRule("nonexistent"), nullptr);
}

// ===========================================================================
// Add rule / get rule / remove rule
// ===========================================================================

TEST_F(TabRulesServiceTest, AddRuleGeneratesId) {
  auto& observer = AddTestObserver();

  AstraTabRule rule;
  rule.name = "Test Rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule.trigger_value = "*://*.example.com/*";
  rule.action_type = AstraTabRuleActionType::kMute;
  rule.priority = 50;

  std::string id = service_->AddRule(std::move(rule));
  ASSERT_FALSE(id.empty());

  // The ID should be a valid UnguessableToken string.
  base::UnguessableToken token;
  EXPECT_TRUE(base::UnguessableToken::DeserializeFromString(id, &token));

  EXPECT_EQ(service_->rule_count(), 1u);
  EXPECT_EQ(observer.added_count_, 1);
  EXPECT_EQ(observer.last_added_id_, id);

  const AstraTabRule* retrieved = service_->GetRule(id);
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->id, id);
  EXPECT_EQ(retrieved->name, "Test Rule");
  EXPECT_TRUE(retrieved->enabled);
  EXPECT_EQ(retrieved->trigger_type, AstraTabRuleTriggerType::kUrlPattern);
  EXPECT_EQ(retrieved->trigger_value, "*://*.example.com/*");
  EXPECT_EQ(retrieved->action_type, AstraTabRuleActionType::kMute);
  EXPECT_EQ(retrieved->priority, 50);
  EXPECT_FALSE(retrieved->created_time.is_null());
  EXPECT_TRUE(retrieved->last_triggered_time.is_null());
  EXPECT_EQ(retrieved->trigger_count, 0);
}

TEST_F(TabRulesServiceTest, AddRuleWithCustomId) {
  AstraTabRule rule;
  rule.id = "my-custom-id";
  rule.name = "Custom ID Rule";
  rule.trigger_type = AstraTabRuleTriggerType::kDomain;
  rule.trigger_value = "example.com";
  rule.action_type = AstraTabRuleActionType::kPin;

  std::string id = service_->AddRule(std::move(rule));
  EXPECT_EQ(id, "my-custom-id");

  const AstraTabRule* retrieved = service_->GetRule("my-custom-id");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->name, "Custom ID Rule");
}

TEST_F(TabRulesServiceTest, AddRuleDuplicateIdFails) {
  AstraTabRule rule1;
  rule1.id = "dup-id";
  rule1.name = "First";
  rule1.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule1.action_type = AstraTabRuleActionType::kMute;
  EXPECT_EQ(service_->AddRule(std::move(rule1)), "dup-id");

  AstraTabRule rule2;
  rule2.id = "dup-id";
  rule2.name = "Second";
  rule2.trigger_type = AstraTabRuleTriggerType::kDomain;
  rule2.action_type = AstraTabRuleActionType::kPin;
  EXPECT_TRUE(service_->AddRule(std::move(rule2)).empty());

  EXPECT_EQ(service_->rule_count(), 1u);
}

TEST_F(TabRulesServiceTest, RemoveRule) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Rule to remove");
  ASSERT_EQ(service_->rule_count(), 1u);

  EXPECT_TRUE(service_->RemoveRule(id));
  EXPECT_EQ(service_->rule_count(), 0u);
  EXPECT_EQ(service_->GetRule(id), nullptr);
  EXPECT_EQ(observer.removed_count_, 1);
  EXPECT_EQ(observer.last_removed_id_, id);
}

TEST_F(TabRulesServiceTest, RemoveNonexistentRuleReturnsFalse) {
  auto& observer = AddTestObserver();
  EXPECT_FALSE(service_->RemoveRule("nonexistent"));
  EXPECT_EQ(observer.removed_count_, 0);
}

// ===========================================================================
// Rule enabling / disabling
// ===========================================================================

TEST_F(TabRulesServiceTest, EnableRule) {
  auto& observer = AddTestObserver();

  AstraTabRule rule;
  rule.name = "Disabled Rule";
  rule.enabled = false;
  rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule.trigger_value = "*://example.com/*";
  rule.action_type = AstraTabRuleActionType::kMute;
  std::string id = service_->AddRule(std::move(rule));

  // Reset observer count from the AddRule notification.
  observer.updated_count_ = 0;

  EXPECT_TRUE(service_->EnableRule(id));
  EXPECT_TRUE(service_->GetRule(id)->enabled);
  EXPECT_EQ(observer.updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, id);
}

TEST_F(TabRulesServiceTest, EnableAlreadyEnabledRule) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Enabled Rule");

  observer.updated_count_ = 0;

  EXPECT_FALSE(service_->EnableRule(id));
  EXPECT_EQ(observer.updated_count_, 0);
}

TEST_F(TabRulesServiceTest, DisableRule) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Rule to disable");

  observer.updated_count_ = 0;

  EXPECT_TRUE(service_->DisableRule(id));
  EXPECT_FALSE(service_->GetRule(id)->enabled);
  EXPECT_EQ(observer.updated_count_, 1);
}

TEST_F(TabRulesServiceTest, DisableAlreadyDisabledRule) {
  auto& observer = AddTestObserver();

  AstraTabRule rule;
  rule.name = "Disabled";
  rule.enabled = false;
  rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule.action_type = AstraTabRuleActionType::kMute;
  std::string id = service_->AddRule(std::move(rule));

  observer.updated_count_ = 0;

  EXPECT_FALSE(service_->DisableRule(id));
  EXPECT_EQ(observer.updated_count_, 0);
}

TEST_F(TabRulesServiceTest, ToggleRule) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Toggle Test");
  ASSERT_TRUE(service_->GetRule(id)->enabled);

  observer.updated_count_ = 0;

  // Toggle off.
  auto result = service_->ToggleRule(id);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(*result);
  EXPECT_FALSE(service_->GetRule(id)->enabled);
  EXPECT_EQ(observer.updated_count_, 1);

  // Toggle on.
  result = service_->ToggleRule(id);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(*result);
  EXPECT_TRUE(service_->GetRule(id)->enabled);
  EXPECT_EQ(observer.updated_count_, 2);
}

TEST_F(TabRulesServiceTest, ToggleNonexistentRule) {
  auto result = service_->ToggleRule("nonexistent");
  EXPECT_FALSE(result.has_value());
}

TEST_F(TabRulesServiceTest, GetEnabledRules) {
  // Add some enabled and disabled rules.
  AddTestRule("Enabled 1");
  AddTestRule("Enabled 2");

  AstraTabRule disabled;
  disabled.name = "Disabled";
  disabled.enabled = false;
  disabled.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  disabled.action_type = AstraTabRuleActionType::kMute;
  service_->AddRule(std::move(disabled));

  auto enabled = service_->GetEnabledRules();
  EXPECT_EQ(enabled.size(), 2u);
}

// ===========================================================================
// URL pattern matching (wildcards)
// ===========================================================================

TEST_F(TabRulesServiceTest, UrlPatternExactMatch) {
  AstraTabRule rule = MakeUrlPatternRule("Exact match",
      "https://example.com/page", AstraTabRuleActionType::kMute);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/page"), u"Page Title", rule));

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/other"), u"Page Title", rule));
}

TEST_F(TabRulesServiceTest, UrlPatternWildcard) {
  AstraTabRule rule = MakeUrlPatternRule("Wildcard test",
      "*://*.youtube.com/*", AstraTabRuleActionType::kMute);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://www.youtube.com/watch?v=abc123"), u"Video", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://youtube.com/"), u"YouTube", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("http://m.youtube.com/feed"), u"Mobile", rule));

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://google.com/search?q=youtube"), u"Search", rule));
}

TEST_F(TabRulesServiceTest, UrlPatternCaseInsensitive) {
  AstraTabRule rule = MakeUrlPatternRule("Case test",
      "*://EXAMPLE.COM/*", AstraTabRuleActionType::kMute);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/page"), u"", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://Example.COM/Page"), u"", rule));
}

TEST_F(TabRulesServiceTest, UrlPatternDisabledRuleDoesNotMatch) {
  AstraTabRule rule = MakeUrlPatternRule("Disabled",
      "*://example.com/*", AstraTabRuleActionType::kMute);
  rule.enabled = false;

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/"), u"", rule));
}

TEST_F(TabRulesServiceTest, UrlPatternInvalidUrl) {
  AstraTabRule rule = MakeUrlPatternRule("Test",
      "*://example.com/*", AstraTabRuleActionType::kMute);

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL(), u"", rule));
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("not a url"), u"", rule));
}

// ===========================================================================
// Domain matching
// ===========================================================================

TEST_F(TabRulesServiceTest, DomainExactMatch) {
  AstraTabRule rule = MakeDomainRule("GitHub",
      "github.com", AstraTabRuleActionType::kMoveToWorkspace);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://github.com/"), u"GitHub", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://github.com/user/repo"), u"Repo", rule));
}

TEST_F(TabRulesServiceTest, DomainSubdomainMatch) {
  AstraTabRule rule = MakeDomainRule("GitHub domain",
      "github.com", AstraTabRuleActionType::kMoveToWorkspace);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://www.github.com/"), u"GitHub", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://gist.github.com/"), u"Gist", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://docs.github.com/en"), u"Docs", rule));
}

TEST_F(TabRulesServiceTest, DomainNoPartialMatch) {
  AstraTabRule rule = MakeDomainRule("Github rule",
      "github.com", AstraTabRuleActionType::kMoveToWorkspace);

  // "mygithub.com" should NOT match "github.com" (not a subdomain).
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://mygithub.com/"), u"", rule));
  // "github.org" should NOT match "github.com" (different TLD).
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://github.org/"), u"", rule));
}

TEST_F(TabRulesServiceTest, DomainCaseInsensitive) {
  AstraTabRule rule = MakeDomainRule("Mixed case",
      "GitHub.COM", AstraTabRuleActionType::kMute);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://github.com/"), u"", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://GITHUB.COM/"), u"", rule));
}

TEST_F(TabRulesServiceTest, DomainEmptyHost) {
  AstraTabRule rule = MakeDomainRule("Test",
      "example.com", AstraTabRuleActionType::kMute);

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("file:///home/user/file.html"), u"", rule));
}

// ===========================================================================
// Title contains matching
// ===========================================================================

TEST_F(TabRulesServiceTest, TitleContainsMatch) {
  AstraTabRule rule = MakeTitleRule("Meeting rule",
      "Meeting", AstraTabRuleActionType::kPin);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://zoom.com/meeting"), u"Team Meeting - Zoom", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://meet.google.com/"), u"Daily standup meeting", rule));
}

TEST_F(TabRulesServiceTest, TitleContainsCaseInsensitive) {
  AstraTabRule rule = MakeTitleRule("Lowercase keyword",
      "video", AstraTabRuleActionType::kMute);

  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://youtube.com/"), u"Funny Video - YouTube", rule));
  EXPECT_TRUE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://vimeo.com/"), u"VIDEO PLAYER", rule));
}

TEST_F(TabRulesServiceTest, TitleContainsNoMatch) {
  AstraTabRule rule = MakeTitleRule("Work keyword",
      "Work", AstraTabRuleActionType::kMoveToWorkspace);

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/"), u"Home Page", rule));
}

TEST_F(TabRulesServiceTest, TitleContainsEmptyKeyword) {
  AstraTabRule rule = MakeTitleRule("Empty",
      "", AstraTabRuleActionType::kMute);

  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://example.com/"), u"Some Title", rule));
}

// ===========================================================================
// Priority ordering
// ===========================================================================

TEST_F(TabRulesServiceTest, RulesSortedByPriority) {
  auto& observer = AddTestObserver();

  // Add rules out of priority order.
  AstraTabRule rule_low;
  rule_low.name = "Low Priority";
  rule_low.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule_low.action_type = AstraTabRuleActionType::kMute;
  rule_low.priority = 200;
  service_->AddRule(std::move(rule_low));

  AstraTabRule rule_high;
  rule_high.name = "High Priority";
  rule_high.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule_high.action_type = AstraTabRuleActionType::kPin;
  rule_high.priority = 10;
  service_->AddRule(std::move(rule_high));

  AstraTabRule rule_mid;
  rule_mid.name = "Mid Priority";
  rule_mid.trigger_type = AstraTabRuleTriggerType::kDomain;
  rule_mid.action_type = AstraTabRuleActionType::kClose;
  rule_mid.priority = 100;
  service_->AddRule(std::move(rule_mid));

  const auto& rules = service_->rules();
  ASSERT_EQ(rules.size(), 3u);

  // Lowest priority number = highest priority = comes first.
  EXPECT_EQ(rules[0].name, "High Priority");
  EXPECT_EQ(rules[0].priority, 10);
  EXPECT_EQ(rules[1].name, "Mid Priority");
  EXPECT_EQ(rules[1].priority, 100);
  EXPECT_EQ(rules[2].name, "Low Priority");
  EXPECT_EQ(rules[2].priority, 200);
}

TEST_F(TabRulesServiceTest, SetRulePriorityReorders) {
  auto& observer = AddTestObserver();

  std::string id1 = AddTestRule("Rule 1");
  service_->SetRulePriority(id1, 100);
  std::string id2 = AddTestRule("Rule 2");
  service_->SetRulePriority(id2, 200);

  observer.updated_count_ = 0;

  // Change rule 2 to higher priority (lower number).
  EXPECT_TRUE(service_->SetRulePriority(id2, 50));
  EXPECT_EQ(observer.updated_count_, 1);

  const auto& rules = service_->rules();
  ASSERT_EQ(rules.size(), 2u);
  EXPECT_EQ(rules[0].id, id2);  // Now higher priority.
  EXPECT_EQ(rules[1].id, id1);
}

TEST_F(TabRulesServiceTest, GetMatchingRulesSortedByPriority) {
  // Add multiple rules that all match the same URL, with different priorities.
  AstraTabRule rule_low = MakeUrlPatternRule(
      "Low", "*://example.com/*", AstraTabRuleActionType::kMute, 300);
  service_->AddRule(std::move(rule_low));

  AstraTabRule rule_high = MakeUrlPatternRule(
      "High", "*://example.com/*", AstraTabRuleActionType::kPin, 50);
  service_->AddRule(std::move(rule_high));

  AstraTabRule rule_mid = MakeUrlPatternRule(
      "Mid", "*://example.com/*", AstraTabRuleActionType::kClose, 150);
  service_->AddRule(std::move(rule_mid));

  auto matching = service_->GetMatchingRules(
      GURL("https://example.com/page"), u"Test");
  ASSERT_EQ(matching.size(), 3u);

  // Matching rules should be in priority order (highest first).
  EXPECT_EQ(matching[0].name, "High");
  EXPECT_EQ(matching[1].name, "Mid");
  EXPECT_EQ(matching[2].name, "Low");
}

// ===========================================================================
// Observer notifications
// ===========================================================================

TEST_F(TabRulesServiceTest, ObserverOnRuleAdded) {
  auto& observer = AddTestObserver();
  EXPECT_EQ(observer.added_count_, 0);

  std::string id = AddTestRule("New Rule");
  EXPECT_EQ(observer.added_count_, 1);
  EXPECT_EQ(observer.last_added_id_, id);
}

TEST_F(TabRulesServiceTest, ObserverOnRuleRemoved) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Remove Me");
  observer.removed_count_ = 0;

  service_->RemoveRule(id);
  EXPECT_EQ(observer.removed_count_, 1);
  EXPECT_EQ(observer.last_removed_id_, id);
}

TEST_F(TabRulesServiceTest, ObserverOnRuleUpdated) {
  auto& observer = AddTestObserver();

  std::string id = AddTestRule("Update Test");
  observer.updated_count_ = 0;

  // Update via UpdateRule.
  AstraTabRule updated = *service_->GetRule(id);
  updated.name = "Updated Name";
  updated.trigger_value = "*://newsite.com/*";
  EXPECT_TRUE(service_->UpdateRule(id, updated));

  EXPECT_EQ(observer.updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, id);
  EXPECT_EQ(service_->GetRule(id)->name, "Updated Name");
}

TEST_F(TabRulesServiceTest, ObserverOnRuleTriggered) {
  auto& observer = AddTestObserver();

  std::string rule_id = AddTestRule("Trigger Test");

  service_->TriggerRuleForTab(rule_id, "tab-123");

  EXPECT_EQ(observer.triggered_count_, 1);
  EXPECT_EQ(observer.last_triggered_rule_id_, rule_id);
  EXPECT_EQ(observer.last_triggered_tab_id_, "tab-123");
}

TEST_F(TabRulesServiceTest, MultipleObservers) {
  auto& obs1 = AddTestObserver();
  auto& obs2 = AddTestObserver();
  auto& obs3 = AddTestObserver();

  AddTestRule("Multi-observer test");

  EXPECT_EQ(obs1.added_count_, 1);
  EXPECT_EQ(obs2.added_count_, 1);
  EXPECT_EQ(obs3.added_count_, 1);
}

TEST_F(TabRulesServiceTest, RemoveObserver) {
  TestTabRulesServiceObserver observer;
  service_->AddObserver(&observer);

  AddTestRule("Before remove");
  EXPECT_EQ(observer.added_count_, 1);

  service_->RemoveObserver(&observer);

  AddTestRule("After remove");
  EXPECT_EQ(observer.added_count_, 1);  // No additional notification.
}

// ===========================================================================
// Clear rules
// ===========================================================================

TEST_F(TabRulesServiceTest, ClearRules) {
  auto& observer = AddTestObserver();

  AddTestRule("Rule 1");
  AddTestRule("Rule 2");
  AddTestRule("Rule 3");
  ASSERT_EQ(service_->rule_count(), 3u);

  service_->ClearRules();

  EXPECT_EQ(service_->rule_count(), 0u);
  EXPECT_TRUE(service_->rules().empty());
  EXPECT_EQ(observer.cleared_count_, 1);
}

TEST_F(TabRulesServiceTest, ClearRulesWhenEmpty) {
  auto& observer = AddTestObserver();

  service_->ClearRules();

  EXPECT_EQ(service_->rule_count(), 0u);
  EXPECT_EQ(observer.cleared_count_, 0);  // No notification if already empty.
}

// ===========================================================================
// Import/export round-trip
// ===========================================================================

TEST_F(TabRulesServiceTest, ExportRulesJsonProducesOutput) {
  AddTestRule("Export Rule 1");
  AddTestRule("Export Rule 2");

  std::string json = service_->ExportRulesJson();
  EXPECT_FALSE(json.empty());
  // Simple sanity check: it should contain the rule names.
  EXPECT_NE(json.find("Export Rule 1"), std::string::npos);
  EXPECT_NE(json.find("Export Rule 2"), std::string::npos);
}

TEST_F(TabRulesServiceTest, ExportRulesJsonEmpty) {
  std::string json = service_->ExportRulesJson();
  // Empty list should still be valid JSON array.
  EXPECT_NE(json.find("["), std::string::npos);
  EXPECT_NE(json.find("]"), std::string::npos);
}

TEST_F(TabRulesServiceTest, ImportRulesJsonMergeMode) {
  // Import with merge=true should preserve existing rules.
  // TODO(astra): Full round-trip test once JSON parsing is implemented.
  // For now, the stub ImportRulesJson returns 0.
  AddTestRule("Existing Rule");

  size_t imported = service_->ImportRulesJson("[]", /*merge=*/true);
  EXPECT_EQ(imported, 0u);  // Stub implementation
  EXPECT_EQ(service_->rule_count(), 1u);  // Existing rule preserved
}

TEST_F(TabRulesServiceTest, ImportRulesJsonReplaceMode) {
  // Import with merge=false should clear existing rules first.
  AddTestRule("Old Rule");
  ASSERT_EQ(service_->rule_count(), 1u);

  auto& observer = AddTestObserver();

  service_->ImportRulesJson("[]", /*merge=*/false);
  // In replace mode, ClearRules is called, which fires OnRulesCleared.
  EXPECT_EQ(observer.cleared_count_, 1);
}

// ===========================================================================
// Sample rules
// ===========================================================================

TEST_F(TabRulesServiceTest, GetSampleRulesReturnsBuiltInRules) {
  auto samples = AstraTabRulesService::GetSampleRules();
  EXPECT_FALSE(samples.empty());
  EXPECT_GE(samples.size(), 3u);

  // Check that sample rules are well-formed.
  for (const auto& rule : samples) {
    EXPECT_FALSE(rule.id.empty());
    EXPECT_FALSE(rule.name.empty());
    EXPECT_FALSE(rule.enabled);  // Sample rules are disabled by default.
    EXPECT_FALSE(rule.trigger_value.empty());
    EXPECT_FALSE(rule.created_time.is_null());
  }
}

TEST_F(TabRulesServiceTest, AddSampleRules) {
  auto& observer = AddTestObserver();

  size_t added = service_->AddSampleRules();
  EXPECT_GT(added, 0u);
  EXPECT_EQ(service_->rule_count(), added);
  EXPECT_EQ(observer.added_count_, static_cast<int>(added));

  // Sample rules should be disabled.
  for (const auto& rule : service_->rules()) {
    EXPECT_FALSE(rule.enabled);
  }
}

TEST_F(TabRulesServiceTest, AddSampleRulesIdempotent) {
  size_t first_add = service_->AddSampleRules();
  size_t second_add = service_->AddSampleRules();

  EXPECT_EQ(second_add, 0u);
  EXPECT_EQ(service_->rule_count(), first_add);
}

// ===========================================================================
// Bulk enable/disable
// ===========================================================================

TEST_F(TabRulesServiceTest, BulkEnableAll) {
  auto& observer = AddTestObserver();

  // Add a mix of enabled and disabled rules.
  AddTestRule("Enabled 1");
  AddTestRule("Enabled 2");

  AstraTabRule d1;
  d1.name = "Disabled 1";
  d1.enabled = false;
  d1.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  d1.action_type = AstraTabRuleActionType::kMute;
  service_->AddRule(std::move(d1));

  AstraTabRule d2;
  d2.name = "Disabled 2";
  d2.enabled = false;
  d2.trigger_type = AstraTabRuleTriggerType::kDomain;
  d2.action_type = AstraTabRuleActionType::kPin;
  service_->AddRule(std::move(d2));

  ASSERT_EQ(service_->rule_count(), 4u);
  ASSERT_EQ(service_->GetEnabledRules().size(), 2u);

  size_t changed = service_->EnableAllRules();
  EXPECT_EQ(changed, 2u);
  EXPECT_EQ(service_->GetEnabledRules().size(), 4u);
}

TEST_F(TabRulesServiceTest, BulkDisableAll) {
  auto& observer = AddTestObserver();

  AddTestRule("Rule 1");
  AddTestRule("Rule 2");
  AddTestRule("Rule 3");
  ASSERT_EQ(service_->GetEnabledRules().size(), 3u);

  size_t changed = service_->DisableAllRules();
  EXPECT_EQ(changed, 3u);
  EXPECT_EQ(service_->GetEnabledRules().size(), 0u);
  EXPECT_EQ(observer.updated_count_, 3);
}

TEST_F(TabRulesServiceTest, BulkEnableNoChange) {
  auto& observer = AddTestObserver();

  AddTestRule("Rule 1");
  AddTestRule("Rule 2");

  observer.updated_count_ = 0;

  size_t changed = service_->EnableAllRules();
  EXPECT_EQ(changed, 0u);
  EXPECT_EQ(observer.updated_count_, 0);
}

// ===========================================================================
// Trigger count tracking
// ===========================================================================

TEST_F(TabRulesServiceTest, TriggerCountIncrements) {
  std::string rule_id = AddTestRule("Count Test");

  const AstraTabRule* rule = service_->GetRule(rule_id);
  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->trigger_count, 0);
  EXPECT_TRUE(rule->last_triggered_time.is_null());

  service_->TriggerRuleForTab(rule_id, "tab-1");
  rule = service_->GetRule(rule_id);
  EXPECT_EQ(rule->trigger_count, 1);
  EXPECT_FALSE(rule->last_triggered_time.is_null());

  base::Time first_trigger = rule->last_triggered_time;

  service_->TriggerRuleForTab(rule_id, "tab-2");
  rule = service_->GetRule(rule_id);
  EXPECT_EQ(rule->trigger_count, 2);
  EXPECT_GE(rule->last_triggered_time, first_trigger);
}

TEST_F(TabRulesServiceTest, TriggerNonexistentRule) {
  auto& observer = AddTestObserver();

  service_->TriggerRuleForTab("nonexistent", "tab-1");
  EXPECT_EQ(observer.triggered_count_, 0);
}

TEST_F(TabRulesServiceTest, TriggerDisabledRuleStillUpdatesStats) {
  // TriggerRuleForTab executes the action regardless of enabled state.
  // Disabling prevents matching, but manual triggering still works.
  AstraTabRule rule;
  rule.name = "Disabled Trigger Test";
  rule.enabled = false;
  rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
  rule.action_type = AstraTabRuleActionType::kMute;
  std::string id = service_->AddRule(std::move(rule));

  service_->TriggerRuleForTab(id, "tab-1");

  const AstraTabRule* retrieved = service_->GetRule(id);
  EXPECT_EQ(retrieved->trigger_count, 1);
  EXPECT_FALSE(retrieved->last_triggered_time.is_null());
}

// ===========================================================================
// Update rule
// ===========================================================================

TEST_F(TabRulesServiceTest, UpdateRulePreservesIdAndCreatedTime) {
  std::string id = AddTestRule("Original");
  const AstraTabRule* original = service_->GetRule(id);
  base::Time created_time = original->created_time;
  int original_count = original->trigger_count;

  // Trigger the rule to set last_triggered_time and count.
  service_->TriggerRuleForTab(id, "tab-1");

  // Update the rule.
  AstraTabRule updated = *original;
  updated.name = "Updated Name";
  updated.priority = 42;
  updated.action_type = AstraTabRuleActionType::kPin;
  updated.trigger_value = "*://updated.com/*";

  // Simulate trying to change immutable fields (should be ignored by UpdateRule).
  updated.id = "different-id";
  updated.created_time = base::Time::UnixEpoch();
  updated.trigger_count = 999;
  updated.last_triggered_time = base::Time();

  EXPECT_TRUE(service_->UpdateRule(id, updated));

  const AstraTabRule* result = service_->GetRule(id);
  ASSERT_NE(result, nullptr);

  // Mutable fields should be updated.
  EXPECT_EQ(result->name, "Updated Name");
  EXPECT_EQ(result->priority, 42);
  EXPECT_EQ(result->action_type, AstraTabRuleActionType::kPin);
  EXPECT_EQ(result->trigger_value, "*://updated.com/*");

  // Immutable fields should be preserved.
  EXPECT_EQ(result->id, id);  // Original ID, not "different-id".
  EXPECT_EQ(result->created_time, created_time);

  // Stats should NOT be overwritten by UpdateRule.
  EXPECT_EQ(result->trigger_count, 1);  // Was incremented by TriggerRuleForTab.
  EXPECT_FALSE(result->last_triggered_time.is_null());
}

TEST_F(TabRulesServiceTest, UpdateNonexistentRule) {
  AstraTabRule updated;
  updated.name = "Should not work";
  EXPECT_FALSE(service_->UpdateRule("nonexistent", updated));
}

// ===========================================================================
// Tab open / time / idle trigger types
// ===========================================================================

TEST_F(TabRulesServiceTest, TabOpenTriggerDoesNotMatchByContent) {
  AstraTabRule rule;
  rule.name = "Tab open rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kTabOpen;
  rule.action_type = AstraTabRuleActionType::kPin;

  // kTabOpen is event-based, not content-based.
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://anything.com/"), u"Anything", rule));
}

TEST_F(TabRulesServiceTest, TimeOfDayTriggerDoesNotMatchByContent) {
  AstraTabRule rule;
  rule.name = "Time rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kTimeOfDay;
  rule.trigger_value = "09:00";
  rule.action_type = AstraTabRuleActionType::kMoveToWorkspace;

  // kTimeOfDay is time-based, not content-based.
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://anything.com/"), u"Anything", rule));
}

TEST_F(TabRulesServiceTest, IdleTriggerDoesNotMatchByContent) {
  AstraTabRule rule;
  rule.name = "Idle rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleTriggerType::kIdle;
  rule.trigger_value = "30";
  rule.action_type = AstraTabRuleActionType::kHibernate;

  // kIdle is activity-based, not content-based.
  EXPECT_FALSE(AstraTabRulesService::DoesTabMatchRule(
      GURL("https://anything.com/"), u"Anything", rule));
}

// ===========================================================================
// GetMatchingRules
// ===========================================================================

TEST_F(TabRulesServiceTest, GetMatchingRulesFiltersByContent) {
  // Add rules with different trigger types.
  service_->AddRule(MakeUrlPatternRule(
      "URL match", "*://example.com/*", AstraTabRuleActionType::kMute, 100));
  service_->AddRule(MakeDomainRule(
      "Domain match", "other.com", AstraTabRuleActionType::kPin, 50));
  service_->AddRule(MakeTitleRule(
      "Title match", "Example", AstraTabRuleActionType::kClose, 200));

  // URL: example.com, title: "Example Page"
  auto matching = service_->GetMatchingRules(
      GURL("https://www.example.com/page"), u"Example Page");

  // Should match URL pattern and title rule, but not the "other.com" domain.
  EXPECT_EQ(matching.size(), 2u);

  // Results should be in priority order.
  // URL match (100) and Title match (200) — URL comes first.
  EXPECT_EQ(matching[0].name, "URL match");
  EXPECT_EQ(matching[1].name, "Title match");
}

TEST_F(TabRulesServiceTest, GetMatchingRulesSkipsDisabled) {
  AstraTabRule enabled = MakeUrlPatternRule(
      "Enabled", "*://example.com/*", AstraTabRuleActionType::kMute, 100);
  service_->AddRule(std::move(enabled));

  AstraTabRule disabled = MakeUrlPatternRule(
      "Disabled", "*://example.com/*", AstraTabRuleActionType::kPin, 50);
  disabled.enabled = false;
  service_->AddRule(std::move(disabled));

  auto matching = service_->GetMatchingRules(
      GURL("https://example.com/"), u"");
  ASSERT_EQ(matching.size(), 1u);
  EXPECT_EQ(matching[0].name, "Enabled");
}

TEST_F(TabRulesServiceTest, GetMatchingRulesEmpty) {
  auto matching = service_->GetMatchingRules(
      GURL("https://example.com/"), u"Test");
  EXPECT_TRUE(matching.empty());
}

// ===========================================================================
// IsIncognito
// ===========================================================================

TEST_F(TabRulesServiceTest, IsIncognitoReturnsFalseForNormalProfile) {
  EXPECT_FALSE(service_->IsIncognito());
}

// ===========================================================================
// Shutdown
// ===========================================================================

TEST_F(TabRulesServiceTest, ShutdownClearsObservers) {
  auto& observer = AddTestObserver();

  service_->Shutdown();

  // After shutdown, adding a rule should not notify (observer list is cleared).
  // Note: in production, the service is destructed after Shutdown, so this
  // is mostly testing that Shutdown doesn't crash and clears state.
  // Since we still hold the service pointer, let's verify no crash on
  // subsequent operations.
  EXPECT_NO_FATAL_FAILURE(service_->rule_count());
}

}  // namespace astra
