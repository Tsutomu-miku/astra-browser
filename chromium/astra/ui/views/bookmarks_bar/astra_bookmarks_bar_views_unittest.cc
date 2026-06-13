// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_model.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraBookmarksBarModelTest : public testing::Test {
 protected:
  void SetUp() override {
    // Test with null profile (basic functionality only).
    // Full tests require a real BookmarkModel.
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(AstraBookmarksBarModelTest, CreateWithNullProfile) {
  // Model should handle null profile gracefully.
  AstraBookmarksBarModel model(nullptr);

  EXPECT_TRUE(model.IsVisible());
  EXPECT_EQ(0u, model.GetItemCount());
  EXPECT_TRUE(model.GetItems().empty());
  EXPECT_EQ(nullptr, model.GetItem(-1));
  EXPECT_EQ(nullptr, model.GetItem(0));
  EXPECT_EQ(nullptr, model.GetItemAtIndex(0));
}

TEST_F(AstraBookmarksBarModelTest, VisibilityToggle) {
  AstraBookmarksBarModel model(nullptr);

  EXPECT_TRUE(model.IsVisible());

  model.SetVisible(false);
  EXPECT_FALSE(model.IsVisible());

  model.ToggleVisible();
  EXPECT_TRUE(model.IsVisible());

  model.ToggleVisible();
  EXPECT_FALSE(model.IsVisible());
}

TEST_F(AstraBookmarksBarModelTest, ShowOnNtpOnly) {
  AstraBookmarksBarModel model(nullptr);

  EXPECT_FALSE(model.show_on_ntp_only());

  model.set_show_on_ntp_only(true);
  EXPECT_TRUE(model.show_on_ntp_only());
}

TEST_F(AstraBookmarksBarModelTest, DisplaySettings) {
  AstraBookmarksBarModel model(nullptr);

  // Defaults.
  EXPECT_TRUE(model.show_favicons());
  EXPECT_TRUE(model.show_text());
  EXPECT_EQ(AstraBookmarksBarModel::kDefaultMaxItemWidth,
            model.max_item_width());
  EXPECT_TRUE(model.show_other_bookmarks());

  // Toggle favicons.
  model.set_show_favicons(false);
  EXPECT_FALSE(model.show_favicons());

  // Toggle text.
  model.set_show_text(false);
  EXPECT_FALSE(model.show_text());

  // Change max item width.
  model.set_max_item_width(100);
  EXPECT_EQ(100, model.max_item_width());

  // Clamp min.
  model.set_max_item_width(0);
  EXPECT_EQ(AstraBookmarksBarModel::kMinItemWidth,
            model.max_item_width());

  // Clamp max.
  model.set_max_item_width(1000);
  EXPECT_EQ(AstraBookmarksBarModel::kMaxItemWidthLimit,
            model.max_item_width());

  // Toggle other bookmarks.
  model.set_show_other_bookmarks(false);
  EXPECT_FALSE(model.show_other_bookmarks());
}

TEST_F(AstraBookmarksBarModelTest, FolderChildrenEmpty) {
  AstraBookmarksBarModel model(nullptr);

  // With no model data, folder children should be empty.
  auto children = model.GetFolderChildren(0);
  EXPECT_TRUE(children.empty());
}

TEST_F(AstraBookmarksBarModelTest, FormatTitle) {
  // Test with no max width.
  std::u16string title = u"Test Bookmark Title";
  gfx::FontList font_list;

  // No max width: returns title as-is.
  auto result = AstraBookmarksBarModel::FormatTitle(
      title, 0, font_list);
  EXPECT_EQ(title, result);

  // Empty title.
  result = AstraBookmarksBarModel::FormatTitle(u"", 100, font_list);
  EXPECT_EQ(u"", result);
}

TEST_F(AstraBookmarksBarModelTest, BookmarkOperationsNoCrash) {
  // Test that operations don't crash with null model.
  AstraBookmarksBarModel model(nullptr);

  model.AddBookmark(u"Test", GURL("https://example.com"));
  model.AddFolder(u"Folder");
  model.RemoveBookmark(1);
  model.RenameBookmark(1, u"New Name");
  model.ChangeBookmarkURL(1, GURL("https://new.com"));
  model.MoveBookmark(1, 2);
  model.OpenBookmark(1);
  model.OpenBookmarkInNewTab(1);
  model.OpenFolderInNewTabs(1);
  model.Refresh();

  // Should not crash.
  SUCCEED();
}

// TODO(astra): Add tests for AstraBookmarksBarItemView
// TODO(astra): Add tests for AstraBookmarksBarView
// These require views test support (ViewsTestBase).

}  // namespace astra
