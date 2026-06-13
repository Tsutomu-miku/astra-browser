// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/health/astra_browser_health_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraBrowserHealthViewTest
// ===========================================================================

class AstraBrowserHealthViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test health issue row creation.
TEST_F(AstraBrowserHealthViewTest, IssueRowCreation) {
  AstraHealthIssueRowView::IssueInfo info;
  info.issue_id = "heavy_tab_1";
  info.title = u"YouTube is heavy";
  info.description = u"320 MB · 22% CPU";
  info.severity = AstraHealthIssueRowView::Severity::kWarning;
  info.action_label = "Sleep";

  auto row = std::make_unique<AstraHealthIssueRowView>(
      info, base::DoNothing());

  EXPECT_EQ("heavy_tab_1", row->issue_id());
}

// Test issue row with info severity.
TEST_F(AstraBrowserHealthViewTest, InfoSeverity) {
  AstraHealthIssueRowView::IssueInfo info;
  info.issue_id = "info_1";
  info.title = u"Info issue";
  info.description = u"Just an FYI";
  info.severity = AstraHealthIssueRowView::Severity::kInfo;
  info.action_label = "Fix";

  auto row = std::make_unique<AstraHealthIssueRowView>(
      info, base::DoNothing());

  EXPECT_EQ("info_1", row->issue_id());
}

// Test issue row with critical severity.
TEST_F(AstraBrowserHealthViewTest, CriticalSeverity) {
  AstraHealthIssueRowView::IssueInfo info;
  info.issue_id = "critical_1";
  info.title = u"Critical issue";
  info.description = u"Needs attention now";
  info.severity = AstraHealthIssueRowView::Severity::kCritical;
  info.action_label = "Fix now";

  auto row = std::make_unique<AstraHealthIssueRowView>(
      info, base::DoNothing());

  EXPECT_EQ("critical_1", row->issue_id());
}

// Test issue row with no action.
TEST_F(AstraBrowserHealthViewTest, IssueWithNoAction) {
  AstraHealthIssueRowView::IssueInfo info;
  info.issue_id = "no_action";
  info.title = u"Info only";
  info.description = u"No action available";
  info.severity = AstraHealthIssueRowView::Severity::kInfo;
  info.action_label = "";

  auto row = std::make_unique<AstraHealthIssueRowView>(
      info, base::DoNothing());

  EXPECT_EQ("no_action", row->issue_id());
}

// Test health view creation.
TEST_F(AstraBrowserHealthViewTest, ViewCreation) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting overall score.
TEST_F(AstraBrowserHealthViewTest, OverallScore) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  view->SetOverallScore(72);
  // Should not crash.
}

// Test score boundaries.
TEST_F(AstraBrowserHealthViewTest, ScoreBoundaries) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  // Min score.
  view->SetOverallScore(0);

  // Max score.
  view->SetOverallScore(100);

  // Over max should be clamped.
  view->SetOverallScore(150);

  // Under min should be clamped.
  view->SetOverallScore(-10);
}

// Test category scores.
TEST_F(AstraBrowserHealthViewTest, CategoryScores) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  std::vector<AstraBrowserHealthView::CategoryScore> scores = {
      {"Memory", "🧠", 82},
      {"Tabs", "📑", 55},
      {"Extensions", "🧩", 30},
      {"Storage", "💾", 60},
  };

  view->SetCategoryScores(scores);
  // Should not crash.
}

// Test empty category scores.
TEST_F(AstraBrowserHealthViewTest, EmptyCategoryScores) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());
  view->SetCategoryScores({});
  // Should not crash.
}

// Test setting issues.
TEST_F(AstraBrowserHealthViewTest, SetIssues) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  std::vector<AstraHealthIssueRowView::IssueInfo> issues;

  AstraHealthIssueRowView::IssueInfo issue1;
  issue1.issue_id = "heavy_youtube";
  issue1.title = u"YouTube is heavy";
  issue1.description = u"320 MB · 22% CPU";
  issue1.severity = AstraHealthIssueRowView::Severity::kWarning;
  issue1.action_label = "Sleep";
  issues.push_back(issue1);

  AstraHealthIssueRowView::IssueInfo issue2;
  issue2.issue_id = "duplicate_tabs";
  issue2.title = u"3 duplicate tabs";
  issue2.description = u"example.com appears in 3 tabs";
  issue2.severity = AstraHealthIssueRowView::Severity::kInfo;
  issue2.action_label = "Close dups";
  issues.push_back(issue2);

  AstraHealthIssueRowView::IssueInfo issue3;
  issue3.issue_id = "unused_extensions";
  issue3.title = u"5 unused extensions";
  issue3.description = u"Not used in 30 days";
  issue3.severity = AstraHealthIssueRowView::Severity::kCritical;
  issue3.action_label = "Review";
  issues.push_back(issue3);

  view->SetIssues(issues);
  // Should not crash.
}

// Test empty issues list.
TEST_F(AstraBrowserHealthViewTest, EmptyIssues) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());
  view->SetIssues({});
  // Should not crash.
}

// Test issue action callback.
TEST_F(AstraBrowserHealthViewTest, IssueActionCallback) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  bool triggered = false;
  std::string issue_id;

  view->SetIssueActionCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& iid) {
            *t = true;
            *id = iid;
          },
          &triggered, &issue_id));
}

// Test cleanup all callback.
TEST_F(AstraBrowserHealthViewTest, CleanupAllCallback) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  bool triggered = false;
  view->SetCleanupAllCallback(
      base::BindRepeating(
          [](bool* t) { *t = true; },
          &triggered));
}

// Test window title.
TEST_F(AstraBrowserHealthViewTest, WindowTitle) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());
  EXPECT_EQ(u"Browser Health", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraBrowserHealthViewTest, ThemeChange) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test theme change on issue row.
TEST_F(AstraBrowserHealthViewTest, IssueRowThemeChange) {
  AstraHealthIssueRowView::IssueInfo info;
  info.issue_id = "theme_test";
  info.title = u"Theme Test";
  info.description = u"Testing theme change";
  info.severity = AstraHealthIssueRowView::Severity::kInfo;
  info.action_label = "Fix";

  auto row = std::make_unique<AstraHealthIssueRowView>(
      info, base::DoNothing());
  row->OnThemeChanged();
}

// Test score label helper.
TEST_F(AstraBrowserHealthViewTest, ScoreLabel) {
  EXPECT_EQ(u"Excellent", AstraBrowserHealthView::ScoreLabel(95));
  EXPECT_EQ(u"Excellent", AstraBrowserHealthView::ScoreLabel(90));
  EXPECT_EQ(u"Good", AstraBrowserHealthView::ScoreLabel(80));
  EXPECT_EQ(u"Good", AstraBrowserHealthView::ScoreLabel(70));
  EXPECT_EQ(u"Fair", AstraBrowserHealthView::ScoreLabel(60));
  EXPECT_EQ(u"Fair", AstraBrowserHealthView::ScoreLabel(50));
  EXPECT_EQ(u"Needs attention", AstraBrowserHealthView::ScoreLabel(40));
  EXPECT_EQ(u"Needs attention", AstraBrowserHealthView::ScoreLabel(30));
  EXPECT_EQ(u"Poor", AstraBrowserHealthView::ScoreLabel(20));
  EXPECT_EQ(u"Poor", AstraBrowserHealthView::ScoreLabel(0));
}

// Test score color helper.
TEST_F(AstraBrowserHealthViewTest, ScoreColor) {
  // Just check it doesn't crash for various scores.
  SkColor color1 = AstraBrowserHealthView::ScoreColor(95);
  SkColor color2 = AstraBrowserHealthView::ScoreColor(70);
  SkColor color3 = AstraBrowserHealthView::ScoreColor(40);
  SkColor color4 = AstraBrowserHealthView::ScoreColor(10);

  // All should be valid colors.
  EXPECT_NE(0u, color1);
  EXPECT_NE(0u, color2);
  EXPECT_NE(0u, color3);
  EXPECT_NE(0u, color4);
}

// Test many issues (scrollable).
TEST_F(AstraBrowserHealthViewTest, ManyIssues) {
  auto* view = new AstraBrowserHealthView(anchor_view_.get());

  std::vector<AstraHealthIssueRowView::IssueInfo> issues;
  for (int i = 0; i < 12; ++i) {
    AstraHealthIssueRowView::IssueInfo issue;
    issue.issue_id = "issue_" + std::to_string(i);
    issue.title = base::UTF8ToUTF16("Issue " + std::to_string(i));
    issue.description = u"Description of the issue";
    issue.severity = (i % 3 == 0)
        ? AstraHealthIssueRowView::Severity::kCritical
        : (i % 3 == 1)
            ? AstraHealthIssueRowView::Severity::kWarning
            : AstraHealthIssueRowView::Severity::kInfo;
    issue.action_label = "Fix";
    issues.push_back(issue);
  }

  view->SetIssues(issues);
  // Should handle many issues with scrolling.
}

}  // namespace astra
