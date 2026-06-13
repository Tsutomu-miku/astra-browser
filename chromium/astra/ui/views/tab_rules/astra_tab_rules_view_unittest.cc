// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_rules/astra_tab_rules_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabRulesViewTest
// ===========================================================================

class AstraTabRulesViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test rule item creation.
TEST_F(AstraTabRulesViewTest, RuleItemCreation) {
  AstraTabRuleItemView::RuleInfo info;
  info.rule_id = "rule_001";
  info.name = u"Auto-sleep work tabs";
  info.enabled = true;
  info.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  info.trigger_value = u"work.com";
  info.action_type = AstraTabRuleItemView::ActionType::kSleep;
  info.action_value = u"30 min idle";
  info.match_count = 5;

  auto item = std::make_unique<AstraTabRuleItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("rule_001", item->rule_id());
  EXPECT_TRUE(item->enabled());
}

// Test disabled rule item.
TEST_F(AstraTabRulesViewTest, RuleItemDisabled) {
  AstraTabRuleItemView::RuleInfo info;
  info.rule_id = "rule_002";
  info.name = u"Disabled rule";
  info.enabled = false;
  info.trigger_type = AstraTabRuleItemView::TriggerType::kUrlPattern;
  info.trigger_value = u"*.example.com";
  info.action_type = AstraTabRuleItemView::ActionType::kMoveToWorkspace;
  info.action_value = u"Reading";
  info.match_count = 0;

  auto item = std::make_unique<AstraTabRuleItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_FALSE(item->enabled());
}

// Test SetEnabled.
TEST_F(AstraTabRulesViewTest, RuleItemSetEnabled) {
  AstraTabRuleItemView::RuleInfo info;
  info.rule_id = "rule_003";
  info.name = u"Toggle test";
  info.enabled = true;
  info.trigger_type = AstraTabRuleItemView::TriggerType::kTitleContains;
  info.trigger_value = u"Dashboard";
  info.action_type = AstraTabRuleItemView::ActionType::kPin;
  info.match_count = 2;

  auto item = std::make_unique<AstraTabRuleItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(item->enabled());
  item->SetEnabled(false);
  EXPECT_FALSE(item->enabled());
  item->SetEnabled(true);
  EXPECT_TRUE(item->enabled());
}

// Test SetName.
TEST_F(AstraTabRulesViewTest, RuleItemSetName) {
  AstraTabRuleItemView::RuleInfo info;
  info.rule_id = "rule_004";
  info.name = u"Old Name";
  info.enabled = true;
  info.trigger_type = AstraTabRuleItemView::TriggerType::kIdleTime;
  info.trigger_value = u"60 min";
  info.action_type = AstraTabRuleItemView::ActionType::kCloseAfter;
  info.action_value = u"60";
  info.match_count = 10;

  auto item = std::make_unique<AstraTabRuleItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(u"Old Name", item->name());
  item->SetName(u"New Name");
  EXPECT_EQ(u"New Name", item->name());
}

// Test tab rules view creation.
TEST_F(AstraTabRulesViewTest, ViewCreation) {
  auto* view = new AstraTabRulesView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabRulesViewTest, WindowTitle) {
  auto* view = new AstraTabRulesView(anchor_view_.get());
  EXPECT_EQ(u"Tab Rules", view->GetWindowTitle());
}

// Test setting rules.
TEST_F(AstraTabRulesViewTest, SetRules) {
  auto* view = new AstraTabRulesView(anchor_view_.get());

  std::vector<AstraTabRuleItemView::RuleInfo> rules;

  AstraTabRuleItemView::RuleInfo rule1;
  rule1.rule_id = "rule1";
  rule1.name = u"Work tabs auto-sleep";
  rule1.enabled = true;
  rule1.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  rule1.trigger_value = u"mycompany.com";
  rule1.action_type = AstraTabRuleItemView::ActionType::kSleep;
  rule1.action_value = u"30 min idle";
  rule1.match_count = 4;
  rules.push_back(rule1);

  AstraTabRuleItemView::RuleInfo rule2;
  rule2.rule_id = "rule2";
  rule2.name = u"News to Reading";
  rule2.enabled = true;
  rule2.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  rule2.trigger_value = u"news.com";
  rule2.action_type = AstraTabRuleItemView::ActionType::kMoveToWorkspace;
  rule2.action_value = u"Reading";
  rule2.match_count = 3;
  rules.push_back(rule2);

  AstraTabRuleItemView::RuleInfo rule3;
  rule3.rule_id = "rule3";
  rule3.name = u"Social mute";
  rule3.enabled = false;
  rule3.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  rule3.trigger_value = u"socialmedia.com";
  rule3.action_type = AstraTabRuleItemView::ActionType::kMute;
  rule3.match_count = 2;
  rules.push_back(rule3);

  view->SetRules(rules);
  EXPECT_NE(nullptr, view);
}

// Test empty rules.
TEST_F(AstraTabRulesViewTest, EmptyRules) {
  auto* view = new AstraTabRulesView(anchor_view_.get());
  view->SetRules({});
  EXPECT_NE(nullptr, view);
}

// Test add rule callback.
TEST_F(AstraTabRulesViewTest, AddRuleCallback) {
  bool callback_called = false;
  auto* view = new AstraTabRulesView(anchor_view_.get());
  view->SetAddRuleCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test toggle rule callback.
TEST_F(AstraTabRulesViewTest, ToggleRuleCallback) {
  std::string toggled_id;
  bool toggled_value = false;
  auto* view = new AstraTabRulesView(anchor_view_.get());
  view->SetToggleRuleCallback(
      base::BindRepeating(
          [](std::string* out_id, bool* out_val,
             const std::string& id, bool val) {
            *out_id = id;
            *out_val = val;
          },
          &toggled_id, &toggled_value));

  std::vector<AstraTabRuleItemView::RuleInfo> rules;
  AstraTabRuleItemView::RuleInfo rule;
  rule.rule_id = "toggle_me";
  rule.name = u"Test Rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  rule.trigger_value = u"test.com";
  rule.action_type = AstraTabRuleItemView::ActionType::kSleep;
  rule.match_count = 1;
  rules.push_back(rule);
  view->SetRules(rules);

  EXPECT_TRUE(toggled_id.empty());
}

// Test delete rule callback.
TEST_F(AstraTabRulesViewTest, DeleteRuleCallback) {
  std::string deleted_id;
  auto* view = new AstraTabRulesView(anchor_view_.get());
  view->SetDeleteRuleCallback(
      base::BindRepeating(
          [](std::string* out_id, const std::string& id) {
            *out_id = id;
          },
          &deleted_id));

  std::vector<AstraTabRuleItemView::RuleInfo> rules;
  AstraTabRuleItemView::RuleInfo rule;
  rule.rule_id = "delete_me";
  rule.name = u"Temp Rule";
  rule.enabled = true;
  rule.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  rule.trigger_value = u"temp.com";
  rule.action_type = AstraTabRuleItemView::ActionType::kGroup;
  rule.match_count = 1;
  rules.push_back(rule);
  view->SetRules(rules);

  EXPECT_TRUE(deleted_id.empty());
}

// Test all trigger types.
TEST_F(AstraTabRulesViewTest, AllTriggerTypes) {
  auto* view = new AstraTabRulesView(anchor_view_.get());

  std::vector<AstraTabRuleItemView::RuleInfo> rules;

  AstraTabRuleItemView::RuleInfo r1;
  r1.rule_id = "t1";
  r1.name = u"URL Pattern";
  r1.trigger_type = AstraTabRuleItemView::TriggerType::kUrlPattern;
  r1.trigger_value = u"*://*.example.com/*";
  r1.action_type = AstraTabRuleItemView::ActionType::kSleep;
  rules.push_back(r1);

  AstraTabRuleItemView::RuleInfo r2;
  r2.rule_id = "t2";
  r2.name = u"Domain";
  r2.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
  r2.trigger_value = u"example.com";
  r2.action_type = AstraTabRuleItemView::ActionType::kMoveToWorkspace;
  r2.action_value = u"Work";
  rules.push_back(r2);

  AstraTabRuleItemView::RuleInfo r3;
  r3.rule_id = "t3";
  r3.name = u"Title Contains";
  r3.trigger_type = AstraTabRuleItemView::TriggerType::kTitleContains;
  r3.trigger_value = u"Dashboard";
  r3.action_type = AstraTabRuleItemView::ActionType::kPin;
  rules.push_back(r3);

  AstraTabRuleItemView::RuleInfo r4;
  r4.rule_id = "t4";
  r4.name = u"Idle Time";
  r4.trigger_type = AstraTabRuleItemView::TriggerType::kIdleTime;
  r4.trigger_value = u"60 min";
  r4.action_type = AstraTabRuleItemView::ActionType::kCloseAfter;
  r4.action_value = u"60";
  rules.push_back(r4);

  view->SetRules(rules);
  SUCCEED();
}

// Test all action types.
TEST_F(AstraTabRulesViewTest, AllActionTypes) {
  auto* view = new AstraTabRulesView(anchor_view_.get());

  std::vector<AstraTabRuleItemView::RuleInfo> rules;

  auto make_rule = [](const std::string& id, const std::u16string& name,
                       AstraTabRuleItemView::ActionType action,
                       const std::u16string& action_val) {
    AstraTabRuleItemView::RuleInfo r;
    r.rule_id = id;
    r.name = name;
    r.trigger_type = AstraTabRuleItemView::TriggerType::kDomain;
    r.trigger_value = u"test.com";
    r.action_type = action;
    r.action_value = action_val;
    return r;
  };

  rules.push_back(make_rule("a1", u"Move to workspace",
      AstraTabRuleItemView::ActionType::kMoveToWorkspace, u"Work"));
  rules.push_back(make_rule("a2", u"Sleep tab",
      AstraTabRuleItemView::ActionType::kSleep, std::u16string()));
  rules.push_back(make_rule("a3", u"Pin tab",
      AstraTabRuleItemView::ActionType::kPin, std::u16string()));
  rules.push_back(make_rule("a4", u"Mute tab",
      AstraTabRuleItemView::ActionType::kMute, std::u16string()));
  rules.push_back(make_rule("a5", u"Group tab",
      AstraTabRuleItemView::ActionType::kGroup, u"Work Group"));
  rules.push_back(make_rule("a6", u"Close after",
      AstraTabRuleItemView::ActionType::kCloseAfter, u"60 min"));

  view->SetRules(rules);
  SUCCEED();
}

}  // namespace astra
