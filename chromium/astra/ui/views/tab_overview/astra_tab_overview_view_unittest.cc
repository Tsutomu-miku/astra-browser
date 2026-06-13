// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_overview/astra_tab_overview_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace astra {

// ===========================================================================
// AstraTabOverviewViewTest
// ===========================================================================

class AstraTabOverviewViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test tab tile creation.
TEST_F(AstraTabOverviewViewTest, TileCreation) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "tab_001";
  info.title = u"Example Page";
  info.domain = "example.com";
  info.url = "https://example.com/page";
  info.favicon_color = SkColorSetRGB(0x5A, 0x9B, 0xE5);
  info.is_active = true;
  info.is_pinned = false;
  info.is_audible = false;
  info.tab_index = 0;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_001", tile->tab_id());
  EXPECT_TRUE(tile->is_active());
}

// Test inactive tile.
TEST_F(AstraTabOverviewViewTest, InactiveTile) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "tab_002";
  info.title = u"Inactive Tab";
  info.domain = "inactive.com";
  info.favicon_color = SK_ColorGREEN;
  info.is_active = false;
  info.tab_index = 1;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_FALSE(tile->is_active());
}

// Test pinned tab tile.
TEST_F(AstraTabOverviewViewTest, PinnedTabTile) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "pinned_1";
  info.title = u"Pinned Tab";
  info.domain = "pinned.com";
  info.favicon_color = SK_ColorRED;
  info.is_pinned = true;
  info.tab_index = 0;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("pinned_1", tile->tab_id());
}

// Test audible tab tile.
TEST_F(AstraTabOverviewViewTest, AudibleTabTile) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "audible_1";
  info.title = u"Music Player";
  info.domain = "music.com";
  info.favicon_color = SkColorSetRGB(0xEA, 0x43, 0x35);
  info.is_audible = true;
  info.tab_index = 2;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("audible_1", tile->tab_id());
}

// Test various favicon colors.
TEST_F(AstraTabOverviewViewTest, VariousFaviconColors) {
  std::vector<SkColor> colors = {
      SK_ColorRED,
      SK_ColorGREEN,
      SK_ColorBLUE,
      SK_ColorYELLOW,
      SkColorSetRGB(0x9C, 0x27, 0xB0),
      SkColorSetRGB(0xFF, 0x98, 0x00),
      SkColorSetRGB(0x00, 0x96, 0x88),
      SK_ColorGRAY,
  };

  for (size_t i = 0; i < colors.size(); i++) {
    AstraTabOverviewTileView::TabInfo info;
    info.tab_id = "color_" + std::to_string(i);
    info.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    info.domain = "site" + std::to_string(i) + ".com";
    info.favicon_color = colors[i];
    info.tab_index = static_cast<int>(i);

    auto tile = std::make_unique<AstraTabOverviewTileView>(
        info, base::DoNothing(), base::DoNothing());
    EXPECT_FALSE(tile->is_active());
  }
}

// Test tab overview view creation.
TEST_F(AstraTabOverviewViewTest, ViewCreation) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabOverviewViewTest, WindowTitle) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  EXPECT_EQ(u"Tab Overview", view->GetWindowTitle());
}

// Test setting tabs.
TEST_F(AstraTabOverviewViewTest, SetTabs) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());

  std::vector<AstraTabOverviewTileView::TabInfo> tabs;

  AstraTabOverviewTileView::TabInfo t1;
  t1.tab_id = "tab1";
  t1.title = u"Google";
  t1.domain = "google.com";
  t1.favicon_color = SkColorSetRGB(0xEA, 0x43, 0x35);
  t1.is_active = true;
  t1.tab_index = 0;
  tabs.push_back(t1);

  AstraTabOverviewTileView::TabInfo t2;
  t2.tab_id = "tab2";
  t2.title = u"YouTube";
  t2.domain = "youtube.com";
  t2.favicon_color = SK_ColorRED;
  t2.is_active = false;
  t2.tab_index = 1;
  tabs.push_back(t2);

  AstraTabOverviewTileView::TabInfo t3;
  t3.tab_id = "tab3";
  t3.title = u"GitHub";
  t3.domain = "github.com";
  t3.favicon_color = SK_ColorGRAY;
  t3.is_pinned = true;
  t3.tab_index = 2;
  tabs.push_back(t3);

  AstraTabOverviewTileView::TabInfo t4;
  t4.tab_id = "tab4";
  t4.title = u"Spotify";
  t4.domain = "spotify.com";
  t4.favicon_color = SkColorSetRGB(0x1D, 0xBF, 0x63);
  t4.is_audible = true;
  t4.tab_index = 3;
  tabs.push_back(t4);

  view->SetTabs(tabs);
  EXPECT_NE(nullptr, view);
}

// Test empty tabs.
TEST_F(AstraTabOverviewViewTest, EmptyTabs) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  view->SetTabs({});
  EXPECT_NE(nullptr, view);
}

// Test group modes.
TEST_F(AstraTabOverviewViewTest, GroupModes) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());

  view->SetGroupMode(AstraTabOverviewView::GroupMode::kNone);
  SUCCEED();

  view->SetGroupMode(AstraTabOverviewView::GroupMode::kByDomain);
  SUCCEED();

  view->SetGroupMode(AstraTabOverviewView::GroupMode::kByWindow);
  SUCCEED();

  view->SetGroupMode(AstraTabOverviewView::GroupMode::kByRecent);
  SUCCEED();
}

// Test tab click callback.
TEST_F(AstraTabOverviewViewTest, TabClickCallback) {
  std::string clicked_tab;
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  view->SetTabClickCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &clicked_tab));

  std::vector<AstraTabOverviewTileView::TabInfo> tabs;
  AstraTabOverviewTileView::TabInfo t;
  t.tab_id = "click_me";
  t.title = u"Click Me";
  t.domain = "click.com";
  t.favicon_color = SK_ColorBLUE;
  t.tab_index = 0;
  tabs.push_back(t);
  view->SetTabs(tabs);

  EXPECT_TRUE(clicked_tab.empty());
}

// Test tab close callback.
TEST_F(AstraTabOverviewViewTest, TabCloseCallback) {
  std::string closed_tab;
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  view->SetTabCloseCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &closed_tab));

  std::vector<AstraTabOverviewTileView::TabInfo> tabs;
  AstraTabOverviewTileView::TabInfo t;
  t.tab_id = "close_me";
  t.title = u"Close Me";
  t.domain = "close.com";
  t.favicon_color = SK_ColorRED;
  t.tab_index = 0;
  tabs.push_back(t);
  view->SetTabs(tabs);

  EXPECT_TRUE(closed_tab.empty());
}

// Test search callback.
TEST_F(AstraTabOverviewViewTest, SearchCallback) {
  std::u16string search_query;
  auto* view = new AstraTabOverviewView(anchor_view_.get());
  view->SetSearchCallback(
      base::BindRepeating(
          [](std::u16string* out, const std::u16string& query) {
            *out = query;
          },
          &search_query));

  EXPECT_TRUE(search_query.empty());
}

// Test many tabs in grid.
TEST_F(AstraTabOverviewViewTest, ManyTabsGrid) {
  auto* view = new AstraTabOverviewView(anchor_view_.get());

  std::vector<AstraTabOverviewTileView::TabInfo> tabs;
  std::vector<SkColor> colors = {
      SK_ColorRED, SK_ColorGREEN, SK_ColorBLUE, SK_ColorYELLOW,
      SkColorSetRGB(0x9C, 0x27, 0xB0),
  };

  for (int i = 0; i < 20; i++) {
    AstraTabOverviewTileView::TabInfo t;
    t.tab_id = "tab_" + std::to_string(i);
    t.title = base::UTF8ToUTF16("Tab Title " + std::to_string(i));
    t.domain = "site" + std::to_string(i) + ".com";
    t.favicon_color = colors[i % colors.size()];
    t.is_active = (i == 5);
    t.is_pinned = (i < 3);
    t.is_audible = (i == 7 || i == 12);
    t.tab_index = i;
    tabs.push_back(t);
  }

  view->SetTabs(tabs);
  SUCCEED();
}

// Test long tab title.
TEST_F(AstraTabOverviewViewTest, LongTabTitle) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "long_title";
  info.title =
      u"This is a very long tab title that should definitely be "
      u"ellipsized within the small tile width";
  info.domain = "very-long-domain-name.example.com";
  info.favicon_color = SK_ColorMAGENTA;
  info.tab_index = 0;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("long_title", tile->tab_id());
}

// Test all combinations of flags.
TEST_F(AstraTabOverviewViewTest, CombinedFlags) {
  AstraTabOverviewTileView::TabInfo info;
  info.tab_id = "combo";
  info.title = u"Active + Audible Pinned";
  info.domain = "combo.com";
  info.favicon_color = SK_ColorCYAN;
  info.is_active = true;
  info.is_pinned = true;
  info.is_audible = true;
  info.tab_index = 0;

  auto tile = std::make_unique<AstraTabOverviewTileView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(tile->is_active());
  EXPECT_EQ("combo", tile->tab_id());
}

}  // namespace astra
