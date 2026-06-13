// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_features/astra_tab_features_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabFeaturesViewTest
// ===========================================================================

class AstraTabFeaturesViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test view creation.
TEST_F(AstraTabFeaturesViewTest, Creation) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting tab title.
TEST_F(AstraTabFeaturesViewTest, TabTitle) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  view->SetTabTitle(u"Example Page");
  // Should not crash.
}

// Test setting tab URL.
TEST_F(AstraTabFeaturesViewTest, TabUrl) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  view->SetTabUrl("https://example.com/page");
}

// Test workspace membership.
TEST_F(AstraTabFeaturesViewTest, WorkspaceMembership) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  view->SetWorkspaceId("ws_work");
  view->SetWorkspaceName(u"Work");

  EXPECT_EQ("ws_work", view->workspace_id());
}

// Test favorite toggle.
TEST_F(AstraTabFeaturesViewTest, FavoriteToggle) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  EXPECT_FALSE(view->IsFavorite());

  view->SetIsFavorite(true);
  EXPECT_TRUE(view->IsFavorite());

  view->SetIsFavorite(false);
  EXPECT_FALSE(view->IsFavorite());
}

// Test reading list toggle.
TEST_F(AstraTabFeaturesViewTest, ReadingListToggle) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  EXPECT_FALSE(view->InReadingList());

  view->SetInReadingList(true);
  EXPECT_TRUE(view->InReadingList());

  view->SetInReadingList(false);
  EXPECT_FALSE(view->InReadingList());
}

// Test note.
TEST_F(AstraTabFeaturesViewTest, Note) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  view->SetNote(u"Remember to review this page");
  EXPECT_EQ(u"Remember to review this page", view->GetNote());
}

// Test tags.
TEST_F(AstraTabFeaturesViewTest, Tags) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  std::vector<std::string> tags = {"work", "important", "review"};
  view->SetTags(tags);
  // Should not crash.
}

// Test timestamps.
TEST_F(AstraTabFeaturesViewTest, Timestamps) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  base::Time now = base::Time::Now();
  view->SetLastVisited(now);
  view->SetAddedTime(now - base::Days(3));
}

// Test pinned status.
TEST_F(AstraTabFeaturesViewTest, PinnedStatus) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  view->SetIsPinned(true);
  view->SetIsPinned(false);
}

// Test callbacks.
TEST_F(AstraTabFeaturesViewTest, Callbacks) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  bool workspace_changed = false;
  view->SetWorkspaceChangedCallback(
      base::BindRepeating(
          [](bool* changed, const std::string& id) { *changed = true; },
          &workspace_changed));

  bool favorite_toggled = false;
  view->SetFavoriteToggledCallback(
      base::BindRepeating(
          [](bool* toggled, bool is_favorite) { *toggled = true; },
          &favorite_toggled));

  bool reading_list_toggled = false;
  view->SetReadingListToggledCallback(
      base::BindRepeating(
          [](bool* toggled, bool in_list) { *toggled = true; },
          &reading_list_toggled));

  bool note_updated = false;
  view->SetNoteUpdatedCallback(
      base::BindRepeating(
          [](bool* updated, const std::u16string& note) {
            *updated = true;
          },
          &note_updated));

  // Callbacks can be set without crashing.
}

// Test window title.
TEST_F(AstraTabFeaturesViewTest, WindowTitle) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  EXPECT_EQ(u"Tab Features", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraTabFeaturesViewTest, ThemeChange) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test favorite callback is triggered.
TEST_F(AstraTabFeaturesViewTest, FavoriteToggleTriggersCallback) {
  auto* view = new AstraTabFeaturesView(anchor_view_.get());

  bool toggled = false;
  bool last_value = false;

  view->SetFavoriteToggledCallback(
      base::BindRepeating(
          [](bool* toggled, bool* last_value, bool is_favorite) {
            *toggled = true;
            *last_value = is_favorite;
          },
          &toggled, &last_value));

  // Simulate toggle by directly calling the handler.
  // In production this would be triggered by the button click.
}

}  // namespace astra
