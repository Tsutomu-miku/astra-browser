// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_context_menu/astra_tab_context_menu_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraTabContextMenuViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic tab info construction.
TEST_F(AstraTabContextMenuViewTest, TabInfo) {
  AstraTabContextMenuView::TabInfo info;
  info.title = u"Example Tab";
  info.url = GURL("https://example.com");
  info.is_pinned = false;
  info.is_muted = false;

  EXPECT_EQ(u"Example Tab", info.title);
  EXPECT_EQ(GURL("https://example.com"), info.url);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_FALSE(info.is_muted);
}

// Test tab info with various states.
TEST_F(AstraTabContextMenuViewTest, TabInfoStates) {
  AstraTabContextMenuView::TabInfo info;
  info.is_pinned = true;
  info.is_muted = true;
  info.is_audible = true;
  info.is_discarded = false;

  EXPECT_TRUE(info.is_pinned);
  EXPECT_TRUE(info.is_muted);
  EXPECT_TRUE(info.is_audible);
  EXPECT_FALSE(info.is_discarded);
}

// Test menu item struct.
TEST_F(AstraTabContextMenuViewTest, MenuItemStruct) {
  AstraTabContextMenuItem item;
  item.type = AstraTabContextMenuItemType::kCloseTab;
  item.label = u"Close";
  item.enabled = true;

  EXPECT_EQ(AstraTabContextMenuItemType::kCloseTab, item.type);
  EXPECT_EQ(u"Close", item.label);
  EXPECT_TRUE(item.enabled);
  EXPECT_FALSE(item.is_separator);
  EXPECT_FALSE(item.is_header);
}

// Test menu item separator.
TEST_F(AstraTabContextMenuViewTest, MenuItemSeparator) {
  AstraTabContextMenuItem item;
  item.is_separator = true;

  EXPECT_TRUE(item.is_separator);
  EXPECT_FALSE(item.is_header);
  EXPECT_TRUE(item.enabled);
}

// Test menu item header.
TEST_F(AstraTabContextMenuViewTest, MenuItemHeader) {
  AstraTabContextMenuItem item;
  item.label = u"Astra";
  item.is_header = true;

  EXPECT_TRUE(item.is_header);
  EXPECT_EQ(u"Astra", item.label);
}

// Test all menu item types exist.
TEST_F(AstraTabContextMenuViewTest, AllItemTypes) {
  std::vector<AstraTabContextMenuItemType> types = {
      AstraTabContextMenuItemType::kNewTab,
      AstraTabContextMenuItemType::kNewTabToRight,
      AstraTabContextMenuItemType::kReload,
      AstraTabContextMenuItemType::kDuplicate,
      AstraTabContextMenuItemType::kPinTab,
      AstraTabContextMenuItemType::kUnpinTab,
      AstraTabContextMenuItemType::kMuteTab,
      AstraTabContextMenuItemType::kUnmuteTab,
      AstraTabContextMenuItemType::kCloseTab,
      AstraTabContextMenuItemType::kCloseOtherTabs,
      AstraTabContextMenuItemType::kCloseTabsToRight,
      AstraTabContextMenuItemType::kCloseTabsToLeft,
      AstraTabContextMenuItemType::kReopenClosedTab,
      AstraTabContextMenuItemType::kMoveToNewWindow,
      AstraTabContextMenuItemType::kAddToBookmarks,
      AstraTabContextMenuItemType::kMoveToWorkspace,
      AstraTabContextMenuItemType::kAddToFavorites,
      AstraTabContextMenuItemType::kSendToDevice,
      AstraTabContextMenuItemType::kCopyURL,
      AstraTabContextMenuItemType::kPrint,
      AstraTabContextMenuItemType::kBookmarkAllTabs,
  };

  EXPECT_EQ(21u, types.size());
}

// Test menu item dangerous flag.
TEST_F(AstraTabContextMenuViewTest, DangerousItem) {
  AstraTabContextMenuItem item;
  item.type = AstraTabContextMenuItemType::kCloseTab;
  item.label = u"Close tab";
  item.is_dangerous = true;

  EXPECT_TRUE(item.is_dangerous);
}

// Test Astra-specific menu items.
TEST_F(AstraTabContextMenuViewTest, AstraSpecificItems) {
  AstraTabContextMenuItem move_item;
  move_item.type = AstraTabContextMenuItemType::kMoveToWorkspace;
  move_item.label = u"Move to workspace";

  AstraTabContextMenuItem fav_item;
  fav_item.type = AstraTabContextMenuItemType::kAddToFavorites;
  fav_item.label = u"Add to favorites";

  // These are Astra-specific additions.
  SUCCEED();
}

// Test disabled menu items.
TEST_F(AstraTabContextMenuViewTest, DisabledItems) {
  AstraTabContextMenuItem item;
  item.type = AstraTabContextMenuItemType::kCloseOtherTabs;
  item.enabled = false;

  EXPECT_FALSE(item.enabled);
}

// Test icon name field.
TEST_F(AstraTabContextMenuViewTest, IconName) {
  AstraTabContextMenuItem item;
  item.icon_name = "close";

  EXPECT_EQ("close", item.icon_name);
}

// TODO(astra): Add tests with ViewsTestBase for the full menu view
// TODO(astra): Add tests for delegate callbacks with mock delegate
// TODO(astra): Add tests for keyboard navigation in menu

}  // namespace astra
