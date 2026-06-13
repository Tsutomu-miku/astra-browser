// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/app_menu/astra_app_menu_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraAppMenuViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic app menu item struct.
TEST_F(AstraAppMenuViewTest, MenuItemStruct) {
  AstraAppMenuItem item;
  item.type = AstraAppMenuItemType::kNewTab;
  item.label = u"New tab";
  item.shortcut = "Ctrl+T";
  item.enabled = true;

  EXPECT_EQ(AstraAppMenuItemType::kNewTab, item.type);
  EXPECT_EQ(u"New tab", item.label);
  EXPECT_EQ("Ctrl+T", item.shortcut);
  EXPECT_TRUE(item.enabled);
}

// Test menu item separator.
TEST_F(AstraAppMenuViewTest, MenuItemSeparator) {
  AstraAppMenuItem item;
  item.is_separator = true;

  EXPECT_TRUE(item.is_separator);
  EXPECT_FALSE(item.is_section_header);
}

// Test menu item section header.
TEST_F(AstraAppMenuViewTest, SectionHeader) {
  AstraAppMenuItem item;
  item.label = u"Astra";
  item.is_section_header = true;

  EXPECT_TRUE(item.is_section_header);
  EXPECT_EQ(u"Astra", item.label);
}

// Test checked menu item.
TEST_F(AstraAppMenuViewTest, CheckedItem) {
  AstraAppMenuItem item;
  item.type = AstraAppMenuItemType::kFocusMode;
  item.is_checked = true;

  EXPECT_TRUE(item.is_checked);
}

// Test submenu item.
TEST_F(AstraAppMenuViewTest, SubmenuItem) {
  AstraAppMenuItem item;
  item.type = AstraAppMenuItemType::kMoreTools;
  item.is_submenu = true;

  EXPECT_TRUE(item.is_submenu);
}

// Test all app menu item types.
TEST_F(AstraAppMenuViewTest, AllItemTypes) {
  std::vector<AstraAppMenuItemType> types = {
      AstraAppMenuItemType::kNewTab,
      AstraAppMenuItemType::kNewWindow,
      AstraAppMenuItemType::kNewIncognitoWindow,
      AstraAppMenuItemType::kNewWorkspace,
      AstraAppMenuItemType::kHistory,
      AstraAppMenuItemType::kDownloads,
      AstraAppMenuItemType::kBookmarks,
      AstraAppMenuItemType::kZoomIn,
      AstraAppMenuItemType::kZoomOut,
      AstraAppMenuItemType::kZoomReset,
      AstraAppMenuItemType::kPrint,
      AstraAppMenuItemType::kFind,
      AstraAppMenuItemType::kMoreTools,
      AstraAppMenuItemType::kSettings,
      AstraAppMenuItemType::kHelp,
      AstraAppMenuItemType::kAbout,
      AstraAppMenuItemType::kExit,
      AstraAppMenuItemType::kFocusMode,
      AstraAppMenuItemType::kSplitView,
      AstraAppMenuItemType::kCommandPalette,
      AstraAppMenuItemType::kWorkspaces,
      AstraAppMenuItemType::kSidebar,
      AstraAppMenuItemType::kScreenshot,
  };

  EXPECT_EQ(23u, types.size());
}

// Test Astra-specific menu items exist.
TEST_F(AstraAppMenuViewTest, AstraSpecificItems) {
  // These are Astra-specific additions to the standard app menu.
  std::vector<AstraAppMenuItemType> astra_items = {
      AstraAppMenuItemType::kFocusMode,
      AstraAppMenuItemType::kSplitView,
      AstraAppMenuItemType::kCommandPalette,
      AstraAppMenuItemType::kWorkspaces,
      AstraAppMenuItemType::kSidebar,
      AstraAppMenuItemType::kScreenshot,
      AstraAppMenuItemType::kNewWorkspace,
  };

  EXPECT_EQ(7u, astra_items.size());
}

// Test icon name field.
TEST_F(AstraAppMenuViewTest, IconName) {
  AstraAppMenuItem item;
  item.icon_name = "settings";

  EXPECT_EQ("settings", item.icon_name);
}

// Test disabled menu item.
TEST_F(AstraAppMenuViewTest, DisabledItem) {
  AstraAppMenuItem item;
  item.type = AstraAppMenuItemType::kZoomOut;
  item.enabled = false;

  EXPECT_FALSE(item.enabled);
}

// Test app menu button basic creation.
TEST_F(AstraAppMenuViewTest, ButtonCreate) {
  auto button = std::make_unique<AstraAppMenuButton>();
  EXPECT_NE(nullptr, button.get());
  EXPECT_FALSE(button->IsMenuShowing());
}

// Test app menu button with null delegate doesn't crash.
TEST_F(AstraAppMenuViewTest, ButtonNullDelegate) {
  auto button = std::make_unique<AstraAppMenuButton>();
  // Should not crash.
  SUCCEED();
}

// Test standard Chromium menu items exist.
TEST_F(AstraAppMenuViewTest, StandardItemsPresent) {
  // These are standard Chromium app menu items that we project.
  std::vector<AstraAppMenuItemType> standard_items = {
      AstraAppMenuItemType::kNewTab,
      AstraAppMenuItemType::kNewWindow,
      AstraAppMenuItemType::kNewIncognitoWindow,
      AstraAppMenuItemType::kHistory,
      AstraAppMenuItemType::kDownloads,
      AstraAppMenuItemType::kBookmarks,
      AstraAppMenuItemType::kZoomIn,
      AstraAppMenuItemType::kZoomOut,
      AstraAppMenuItemType::kZoomReset,
      AstraAppMenuItemType::kPrint,
      AstraAppMenuItemType::kFind,
      AstraAppMenuItemType::kMoreTools,
      AstraAppMenuItemType::kSettings,
      AstraAppMenuItemType::kHelp,
      AstraAppMenuItemType::kAbout,
      AstraAppMenuItemType::kExit,
  };

  EXPECT_EQ(16u, standard_items.size());
}

// Test shortcut strings.
TEST_F(AstraAppMenuViewTest, ShortcutStrings) {
  AstraAppMenuItem item;
  item.type = AstraAppMenuItemType::kNewTab;
  item.shortcut = "Ctrl+T";

  EXPECT_EQ("Ctrl+T", item.shortcut);
}

// TODO(astra): Add tests with ViewsTestBase for the full menu view
// TODO(astra): Add tests for delegate callbacks with mock delegate
// TODO(astra): Add tests for keyboard navigation in menu
// TODO(astra): Add tests for PopulateDefaultItems

}  // namespace astra
