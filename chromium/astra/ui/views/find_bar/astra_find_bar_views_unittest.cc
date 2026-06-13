// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/find_bar/astra_find_bar_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraFindBarViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic find bar construction.
TEST_F(AstraFindBarViewTest, Create) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  EXPECT_TRUE(find_bar->GetFindText().empty());
  EXPECT_TRUE(find_bar->highlight_all());
  EXPECT_FALSE(find_bar->case_sensitive());
}

// Test find text.
TEST_F(AstraFindBarViewTest, FindText) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->SetFindText(u"hello");
  EXPECT_EQ(u"hello", find_bar->GetFindText());

  find_bar->SetFindText(u"");
  EXPECT_TRUE(find_bar->GetFindText().empty());

  find_bar->SetFindText(u"search query with spaces");
  EXPECT_EQ(u"search query with spaces", find_bar->GetFindText());
}

// Test match info.
TEST_F(AstraFindBarViewTest, MatchInfo) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->SetMatchInfo(1, 10);
  // Should not crash.
  SUCCEED();

  find_bar->SetMatchInfo(0, 0);
  SUCCEED();

  find_bar->SetMatchInfo(5, 5);
  SUCCEED();
}

// Test highlight all toggle.
TEST_F(AstraFindBarViewTest, HighlightAll) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  EXPECT_TRUE(find_bar->highlight_all());

  find_bar->SetHighlightAll(false);
  EXPECT_FALSE(find_bar->highlight_all());

  find_bar->SetHighlightAll(true);
  EXPECT_TRUE(find_bar->highlight_all());
}

// Test case sensitivity toggle.
TEST_F(AstraFindBarViewTest, CaseSensitive) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  EXPECT_FALSE(find_bar->case_sensitive());

  find_bar->SetCaseSensitive(true);
  EXPECT_TRUE(find_bar->case_sensitive());

  find_bar->SetCaseSensitive(false);
  EXPECT_FALSE(find_bar->case_sensitive());
}

// Test show/hide.
TEST_F(AstraFindBarViewTest, ShowHide) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->Hide();
  EXPECT_FALSE(find_bar->IsBarVisible());

  find_bar->Show();
  EXPECT_TRUE(find_bar->IsBarVisible());

  find_bar->Hide();
  EXPECT_FALSE(find_bar->IsBarVisible());
}

// Test preferred size.
TEST_F(AstraFindBarViewTest, PreferredSize) {
  auto find_bar = std::make_unique<AstraFindBarView>();
  gfx::Size size = find_bar->CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(36, size.height());
}

// Test null delegate doesn't crash.
TEST_F(AstraFindBarViewTest, NullDelegateNoCrash) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  // All operations with null delegate should not crash.
  find_bar->SetFindText(u"test");
  find_bar->SetMatchInfo(1, 5);
  find_bar->SetHighlightAll(false);
  find_bar->SetCaseSensitive(true);
  find_bar->Show();
  find_bar->Hide();
  SUCCEED();
}

// Test empty find text.
TEST_F(AstraFindBarViewTest, EmptyFindText) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->SetFindText(u"");
  find_bar->SetMatchInfo(0, 0);
  // Should not crash with empty text and zero matches.
  SUCCEED();
}

// Test find text with special characters.
TEST_F(AstraFindBarViewTest, SpecialCharacters) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->SetFindText(u"test@example.com");
  EXPECT_EQ(u"test@example.com", find_bar->GetFindText());

  find_bar->SetFindText(u"<script>alert(1)</script>");
  EXPECT_EQ(u"<script>alert(1)</script>", find_bar->GetFindText());

  find_bar->SetFindText(u"日本語テスト");
  EXPECT_EQ(u"日本語テスト", find_bar->GetFindText());
}

// Test many match count updates.
TEST_F(AstraFindBarViewTest, ManyMatchUpdates) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  find_bar->SetFindText(u"a");

  for (int i = 0; i < 100; ++i) {
    find_bar->SetMatchInfo(i, 100);
  }
  // Should not crash.
  SUCCEED();
}

// Test toggle combinations.
TEST_F(AstraFindBarViewTest, ToggleCombinations) {
  auto find_bar = std::make_unique<AstraFindBarView>();

  // Test all combinations of highlight and case sensitivity.
  find_bar->SetHighlightAll(false);
  find_bar->SetCaseSensitive(false);
  EXPECT_FALSE(find_bar->highlight_all());
  EXPECT_FALSE(find_bar->case_sensitive());

  find_bar->SetHighlightAll(true);
  find_bar->SetCaseSensitive(true);
  EXPECT_TRUE(find_bar->highlight_all());
  EXPECT_TRUE(find_bar->case_sensitive());

  find_bar->SetHighlightAll(false);
  find_bar->SetCaseSensitive(true);
  EXPECT_FALSE(find_bar->highlight_all());
  EXPECT_TRUE(find_bar->case_sensitive());
}

// TODO(astra): Add tests with ViewsTestBase for layout validation
// TODO(astra): Add tests for keyboard event handling
// TODO(astra): Add tests for delegate callbacks with mock delegate

}  // namespace astra
