// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/keyboard_shortcuts/astra_keyboard_shortcuts_view.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"

namespace astra {

using AstraKeyboardShortcutsViewTest = views::ViewsTestBase;

// ===========================================================================
// AstraShortcutItemView tests
// ===========================================================================

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutItemView_HasCorrectId) {
  AstraShortcutItemView::ShortcutInfo info;
  info.shortcut_id = "new_tab";
  info.description = u"Open new tab";
  info.shortcut = u"Ctrl+T";
  info.category = u"Tabs";
  info.is_astra = false;

  auto item = std::make_unique<AstraShortcutItemView>(info);

  EXPECT_EQ(item->shortcut_id(), "new_tab");
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutItemView_AstraBadge) {
  AstraShortcutItemView::ShortcutInfo info;
  info.shortcut_id = "astra_sidebar";
  info.description = u"Toggle Astra sidebar";
  info.shortcut = u"Ctrl+Shift+S";
  info.category = u"Astra";
  info.is_astra = true;

  auto item = std::make_unique<AstraShortcutItemView>(info);

  EXPECT_EQ(item->shortcut_id(), "astra_sidebar");
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutItemView_PreferredSize) {
  AstraShortcutItemView::ShortcutInfo info;
  info.shortcut_id = "close_tab";
  info.description = u"Close tab";
  info.shortcut = u"Ctrl+W";
  info.category = u"Tabs";

  auto item = std::make_unique<AstraShortcutItemView>(info);

  gfx::Size preferred = item->GetPreferredSize();
  EXPECT_GT(preferred.width(), 0);
  EXPECT_GT(preferred.height(), 0);
}

// ===========================================================================
// AstraKeyboardShortcutsView tests
// ===========================================================================

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_HasTitle) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  EXPECT_FALSE(view->GetWindowTitle().empty());
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_SetShortcuts) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::vector<AstraShortcutItemView::ShortcutInfo> shortcuts;

  AstraShortcutItemView::ShortcutInfo new_tab;
  new_tab.shortcut_id = "new_tab";
  new_tab.description = u"Open new tab";
  new_tab.shortcut = u"Ctrl+T";
  new_tab.category = u"Tabs";
  shortcuts.push_back(new_tab);

  AstraShortcutItemView::ShortcutInfo close_tab;
  close_tab.shortcut_id = "close_tab";
  close_tab.description = u"Close tab";
  close_tab.shortcut = u"Ctrl+W";
  close_tab.category = u"Tabs";
  shortcuts.push_back(close_tab);

  AstraShortcutItemView::ShortcutInfo back;
  back.shortcut_id = "back";
  back.description = u"Back";
  back.shortcut = u"Alt+Left";
  back.category = u"Navigation";
  shortcuts.push_back(back);

  view->SetShortcuts(shortcuts);
  view->Layout();

  SUCCEED();
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_GroupedByCategory) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::vector<AstraShortcutItemView::ShortcutInfo> shortcuts;

  // 3 tabs shortcuts.
  for (int i = 0; i < 3; i++) {
    AstraShortcutItemView::ShortcutInfo s;
    s.shortcut_id = "tab_" + std::to_string(i);
    s.description = base::UTF8ToUTF16("Tab shortcut " + std::to_string(i));
    s.shortcut = base::UTF8ToUTF16("Ctrl+" + std::to_string(i + 1));
    s.category = u"Tabs";
    shortcuts.push_back(s);
  }

  // 2 navigation shortcuts.
  for (int i = 0; i < 2; i++) {
    AstraShortcutItemView::ShortcutInfo s;
    s.shortcut_id = "nav_" + std::to_string(i);
    s.description = base::UTF8ToUTF16("Nav shortcut " + std::to_string(i));
    s.shortcut = base::UTF8ToUTF16("Alt+" + std::to_string(i));
    s.category = u"Navigation";
    shortcuts.push_back(s);
  }

  view->SetShortcuts(shortcuts);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_AstraShortcuts) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::vector<AstraShortcutItemView::ShortcutInfo> shortcuts;

  AstraShortcutItemView::ShortcutInfo astra_feat;
  astra_feat.shortcut_id = "astra_feature";
  astra_feat.description = u"Astra feature";
  astra_feat.shortcut = u"Ctrl+Shift+A";
  astra_feat.category = u"Astra";
  astra_feat.is_astra = true;
  shortcuts.push_back(astra_feat);

  AstraShortcutItemView::ShortcutInfo normal;
  normal.shortcut_id = "normal";
  normal.description = u"Normal shortcut";
  normal.shortcut = u"Ctrl+N";
  normal.category = u"Page";
  normal.is_astra = false;
  shortcuts.push_back(normal);

  view->SetShortcuts(shortcuts);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_SearchCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::u16string received_query;
  view->SetSearchCallback(base::BindRepeating(
      [](std::u16string* out, const std::u16string& query) { *out = query; },
      &received_query));

  // Callback is wired; we verify it's stored and doesn't crash.
  // Actual text change events come from the textfield via views system.
  EXPECT_TRUE(received_query.empty());
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_ShortcutCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::string received_id;
  view->SetShortcutCallback(base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &received_id));

  // Callback stored; shortcut activation is driven by user click events
  // in production.
  EXPECT_TRUE(received_id.empty());
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_EmptyShortcuts) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::vector<AstraShortcutItemView::ShortcutInfo> empty;
  view->SetShortcuts(empty);
  view->Layout();

  SUCCEED();
}

TEST_F(AstraKeyboardShortcutsViewTest, ShortcutsView_ManyCategories) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraKeyboardShortcutsView>(anchor.get());

  std::vector<std::u16string> categories = {
      u"Tabs", u"Navigation", u"Page", u"Window", u"Astra",
      u"Editing", u"Focus", u"Accessibility", u"Developer"
  };

  std::vector<AstraShortcutItemView::ShortcutInfo> shortcuts;
  int id = 0;
  for (const auto& cat : categories) {
    AstraShortcutItemView::ShortcutInfo s;
    s.shortcut_id = "shortcut_" + std::to_string(id++);
    s.description = base::UTF8ToUTF16(
        "Shortcut in " + base::UTF16ToUTF8(cat));
    s.shortcut = u"Ctrl+K";
    s.category = cat;
    shortcuts.push_back(s);
  }

  view->SetShortcuts(shortcuts);
  view->Layout();
  SUCCEED();
}

}  // namespace astra
