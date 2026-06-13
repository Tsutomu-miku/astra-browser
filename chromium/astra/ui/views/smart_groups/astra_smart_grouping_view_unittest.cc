// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/smart_groups/astra_smart_grouping_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraSmartGroupingViewTest
// ===========================================================================

class AstraSmartGroupingViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test suggestion view creation.
TEST_F(AstraSmartGroupingViewTest, SuggestionCreation) {
  AstraSmartGroupSuggestionView::Suggestion s;
  s.suggestion_id = "group_work";
  s.name = u"Work";
  s.group_type = "domain";
  s.tab_count = 5;
  s.sample_domains = {"docs.google.com", "mail.google.com", "drive.google.com"};
  s.color = "#5B8FF9";

  auto view = std::make_unique<AstraSmartGroupSuggestionView>(
      s, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("group_work", view->suggestion_id());
  EXPECT_TRUE(view->selected());
}

// Test suggestion selection toggle.
TEST_F(AstraSmartGroupingViewTest, SuggestionSelectionToggle) {
  AstraSmartGroupSuggestionView::Suggestion s;
  s.suggestion_id = "test_group";
  s.name = u"Test";
  s.tab_count = 3;
  s.color = "#FF0000";

  auto view = std::make_unique<AstraSmartGroupSuggestionView>(
      s, base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(view->selected());
  view->SetSelected(false);
  EXPECT_FALSE(view->selected());
  view->SetSelected(true);
  EXPECT_TRUE(view->selected());
}

// Test various group types.
TEST_F(AstraSmartGroupingViewTest, VariousGroupTypes) {
  std::vector<std::string> types = {
      "domain", "time", "purpose", "workspace", "unknown"};

  for (const auto& type : types) {
    AstraSmartGroupSuggestionView::Suggestion s;
    s.suggestion_id = "group_" + type;
    s.name = base::UTF8ToUTF16(type);
    s.group_type = type;
    s.tab_count = 3;
    s.color = "#00FF00";

    auto view = std::make_unique<AstraSmartGroupSuggestionView>(
        s, base::DoNothing(), base::DoNothing());
    EXPECT_EQ("group_" + type, view->suggestion_id());
  }
}

// Test suggestion with many sample domains.
TEST_F(AstraSmartGroupingViewTest, ManySampleDomains) {
  AstraSmartGroupSuggestionView::Suggestion s;
  s.suggestion_id = "many_domains";
  s.name = u"Many Domains";
  s.group_type = "domain";
  s.tab_count = 10;
  s.sample_domains = {
      "a.com", "b.com", "c.com", "d.com", "e.com",
      "f.com", "g.com", "h.com", "i.com", "j.com",
  };
  s.color = "#123456";

  auto view = std::make_unique<AstraSmartGroupSuggestionView>(
      s, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("many_domains", view->suggestion_id());
}

// Test smart grouping view creation.
TEST_F(AstraSmartGroupingViewTest, ViewCreation) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting suggestions.
TEST_F(AstraSmartGroupingViewTest, SetSuggestions) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  std::vector<AstraSmartGroupSuggestionView::Suggestion> suggestions;

  AstraSmartGroupSuggestionView::Suggestion s1;
  s1.suggestion_id = "work";
  s1.name = u"Work";
  s1.group_type = "domain";
  s1.tab_count = 5;
  s1.sample_domains = {"docs.google.com", "mail.google.com"};
  s1.color = "#5B8FF9";
  suggestions.push_back(s1);

  AstraSmartGroupSuggestionView::Suggestion s2;
  s2.suggestion_id = "social";
  s2.name = u"Social";
  s2.group_type = "domain";
  s2.tab_count = 3;
  s2.sample_domains = {"twitter.com", "reddit.com"};
  s2.color = "#F97316";
  suggestions.push_back(s2);

  view->SetSuggestions(suggestions);
  // Should not crash.
}

// Test empty suggestions.
TEST_F(AstraSmartGroupingViewTest, EmptySuggestions) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());
  view->SetSuggestions({});
  // Should not crash.
}

// Test changing group by.
TEST_F(AstraSmartGroupingViewTest, ChangeGroupBy) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  view->SetGroupBy(AstraSmartGroupingView::GroupBy::kDomain);
  view->SetGroupBy(AstraSmartGroupingView::GroupBy::kTime);
  view->SetGroupBy(AstraSmartGroupingView::GroupBy::kPurpose);
  view->SetGroupBy(AstraSmartGroupingView::GroupBy::kWorkspace);
  // Should not crash.
}

// Test apply suggestions callback.
TEST_F(AstraSmartGroupingViewTest, ApplySuggestionsCallback) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  bool triggered = false;
  std::vector<std::string> applied_ids;

  view->SetApplySuggestionsCallback(
      base::BindRepeating(
          [](bool* t, std::vector<std::string>* ids,
             const std::vector<std::string>& applied) {
            *t = true;
            *ids = applied;
          },
          &triggered, &applied_ids));
}

// Test dismiss suggestion callback.
TEST_F(AstraSmartGroupingViewTest, DismissSuggestionCallback) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  bool triggered = false;
  std::string dismissed_id;

  view->SetDismissSuggestionCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& sid) {
            *t = true;
            *id = sid;
          },
          &triggered, &dismissed_id));
}

// Test group by changed callback.
TEST_F(AstraSmartGroupingViewTest, GroupByChangedCallback) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  bool triggered = false;
  std::string group_by;

  view->SetGroupByChangedCallback(
      base::BindRepeating(
          [](bool* t, std::string* gb, const std::string& type) {
            *t = true;
            *gb = type;
          },
          &triggered, &group_by));
}

// Test refresh callback.
TEST_F(AstraSmartGroupingViewTest, RefreshCallback) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  bool triggered = false;
  view->SetRefreshCallback(
      base::BindRepeating(
          [](bool* t) { *t = true; },
          &triggered));
}

// Test window title.
TEST_F(AstraSmartGroupingViewTest, WindowTitle) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());
  EXPECT_EQ(u"Smart Groups", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraSmartGroupingViewTest, ThemeChange) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test theme change on suggestion view.
TEST_F(AstraSmartGroupingViewTest, SuggestionThemeChange) {
  AstraSmartGroupSuggestionView::Suggestion s;
  s.suggestion_id = "theme_test";
  s.name = u"Theme Test";
  s.tab_count = 1;
  s.color = "#000000";

  auto view = std::make_unique<AstraSmartGroupSuggestionView>(
      s, base::DoNothing(), base::DoNothing());
  view->OnThemeChanged();
}

// Test many suggestions.
TEST_F(AstraSmartGroupingViewTest, ManySuggestions) {
  auto* view = new AstraSmartGroupingView(anchor_view_.get());

  std::vector<AstraSmartGroupSuggestionView::Suggestion> suggestions;
  for (int i = 0; i < 10; ++i) {
    AstraSmartGroupSuggestionView::Suggestion s;
    s.suggestion_id = "group_" + std::to_string(i);
    s.name = base::UTF8ToUTF16("Group " + std::to_string(i));
    s.group_type = "domain";
    s.tab_count = i + 1;
    s.color = "#" + std::to_string(100000 + i * 1000);
    suggestions.push_back(s);
  }

  view->SetSuggestions(suggestions);
  // Should handle many suggestions with scrolling.
}

}  // namespace astra
