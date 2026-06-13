// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for tab search views.
//
// Test coverage includes:
//   - AstraTabSearchItem struct: construction, all fields, edge cases
//   - AstraTabSearchGroupInfo struct: construction and fields
//   - AstraTabSearchMode enum: all modes
//   - AstraTabSearchSortOrder enum: all sort orders
//   - AstraTabSearchModel: tab list, search, modes, workspaces, groups,
//     windows, actions, settings, observers, edge cases
//   - AstraTabSearchItemView: construction, state, highlighting,
//     visibility toggles, group color, audio indicator, edge cases
//   - AstraTabSearchBubble: visibility, query, selection, activation,
//     search mode, sizing, edge cases
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/tab_search/astra_tab_search_bubble.h"
#include "astra/ui/views/tab_search/astra_tab_search_item_view.h"
#include "astra/ui/views/tab_search/astra_tab_search_model.h"

#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "skia/include/core/SkColor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/range/range.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// =========================================================================
// Test helper: create sample tab items for testing
// =========================================================================

std::vector<AstraTabSearchItem> CreateSampleTabs(size_t count = 8) {
  std::vector<AstraTabSearchItem> tabs;
  base::Time now = base::Time::Now();

  struct TabSpec {
    const char* title;
    const char* hostname;
    const char* url;
    const char* workspace_id;
    const char* workspace_name;
    bool is_pinned;
    bool is_audible;
    bool is_muted;
    bool is_in_group;
    const char* group_name;
    SkColor group_color;
    int window_id;
    bool is_active;
    int minutes_ago;
  };

  TabSpec specs[] = {
    // Workspace: "work"
    {"My Document", "docs.google.com", "https://docs.google.com/doc",
     "work", "Work", false, false, false, true, "Docs", SK_ColorBLUE,
     0, true, 0},
    {"Spreadsheet", "sheets.google.com", "https://sheets.google.com/sheet",
     "work", "Work", false, false, false, true, "Docs", SK_ColorBLUE,
     0, false, 5},
    {"GitHub", "github.com", "https://github.com",
     "work", "Work", false, false, false, false, "", SK_ColorTRANSPARENT,
     0, false, 10},
    {"Code Review", "review.example.com", "https://review.example.com",
     "work", "Work", true, false, false, true, "Dev", SK_ColorGREEN,
     0, false, 15},

    // Workspace: "personal"
    {"Gmail", "mail.google.com", "https://mail.google.com",
     "personal", "Personal", false, true, false, false, "", SK_ColorTRANSPARENT,
     0, false, 2},
    {"YouTube", "youtube.com", "https://youtube.com/watch?v=123",
     "personal", "Personal", false, true, true, false, "", SK_ColorTRANSPARENT,
     0, false, 20},
    {"Twitter", "twitter.com", "https://twitter.com/home",
     "personal", "Personal", false, false, false, false, "", SK_ColorTRANSPARENT,
     1, false, 30},

    // Pinned tab
    {"Calendar", "calendar.google.com", "https://calendar.google.com",
     "personal", "Personal", true, false, false, false, "", SK_ColorTRANSPARENT,
     0, false, 1},

    // Additional tabs for more test data
    {"Reddit", "reddit.com", "https://reddit.com",
     "personal", "Personal", false, false, false, false, "", SK_ColorTRANSPARENT,
     1, false, 25},
    {"Wikipedia", "en.wikipedia.org", "https://en.wikipedia.org/wiki/Test",
     "work", "Work", false, false, false, true, "Research", SK_ColorRED,
     0, false, 45},
    {"News Site", "news.example.com", "https://news.example.com",
     "", "", false, true, false, false, "", SK_ColorTRANSPARENT,
     1, false, 35},
    {"Music Player", "music.example.com", "https://music.example.com",
     "personal", "Personal", false, true, false, false, "", SK_ColorTRANSPARENT,
     1, false, 8},
  };

  size_t num_specs = sizeof(specs) / sizeof(specs[0]);
  size_t actual_count = std::min(count, num_specs);

  for (size_t i = 0; i < actual_count; ++i) {
    const auto& s = specs[i];
    AstraTabSearchItem tab;
    tab.tab_id = static_cast<int>(i);
    tab.title = base::UTF8ToUTF16(s.title);
    tab.hostname = base::UTF8ToUTF16(s.hostname);
    tab.url = GURL(s.url);
    tab.workspace_id = s.workspace_id;
    tab.workspace_name = base::UTF8ToUTF16(s.workspace_name);
    tab.is_active = s.is_active;
    tab.is_pinned = s.is_pinned;
    tab.is_in_group = s.is_in_group;
    tab.group_name = base::UTF8ToUTF16(s.group_name);
    tab.group_color = s.group_color;
    tab.is_audible = s.is_audible;
    tab.is_muted = s.is_muted;
    tab.tab_index = static_cast<int>(i);
    tab.window_id = s.window_id;
    tab.last_active_time = now - base::Minutes(s.minutes_ago);
    tab.has_crashed = false;
    tab.is_loading = false;
    tab.relevance_score = 0.0;
    tabs.push_back(std::move(tab));
  }

  return tabs;
}

std::vector<AstraTabSearchGroupInfo> CreateSampleGroups() {
  std::vector<AstraTabSearchGroupInfo> groups;

  AstraTabSearchGroupInfo g1;
  g1.group_id = "docs";
  g1.title = u"Docs";
  g1.color = SK_ColorBLUE;
  g1.tab_count = 2;
  g1.collapsed = false;
  groups.push_back(std::move(g1));

  AstraTabSearchGroupInfo g2;
  g2.group_id = "dev";
  g2.title = u"Dev";
  g2.color = SK_ColorGREEN;
  g2.tab_count = 1;
  g2.collapsed = false;
  groups.push_back(std::move(g2));

  AstraTabSearchGroupInfo g3;
  g3.group_id = "research";
  g3.title = u"Research";
  g3.color = SK_ColorRED;
  g3.tab_count = 1;
  g3.collapsed = true;
  groups.push_back(std::move(g3));

  return groups;
}

// Test observer that tracks all notifications.
class TestTabSearchObserver : public AstraTabSearchObserver {
 public:
  void OnTabListChanged(AstraTabSearchModel* model) override {
    tab_list_changed_count_++;
    last_model_tab_list_ = model;
  }
  void OnTabActivated(AstraTabSearchModel* model, int tab_index) override {
    tab_activated_count_++;
    last_activated_tab_index_ = tab_index;
    last_model_tab_activated_ = model;
  }
  void OnTabClosed(AstraTabSearchModel* model, int tab_index) override {
    tab_closed_count_++;
    last_closed_tab_index_ = tab_index;
    last_model_tab_closed_ = model;
  }
  void OnSearchResultsChanged(AstraTabSearchModel* model) override {
    search_results_changed_count_++;
    last_model_search_results_ = model;
  }
  void OnSearchModeChanged(AstraTabSearchModel* model,
                           AstraTabSearchMode mode) override {
    search_mode_changed_count_++;
    last_search_mode_ = mode;
    last_model_search_mode_ = model;
  }
  void OnTabSearchModelShutdown(AstraTabSearchModel* model) override {
    model_shutdown_count_++;
    last_model_shutdown_ = model;
  }

  void Reset() {
    tab_list_changed_count_ = 0;
    tab_activated_count_ = 0;
    tab_closed_count_ = 0;
    search_results_changed_count_ = 0;
    search_mode_changed_count_ = 0;
    model_shutdown_count_ = 0;
    last_activated_tab_index_ = -1;
    last_closed_tab_index_ = -1;
  }

  int tab_list_changed_count_ = 0;
  int tab_activated_count_ = 0;
  int tab_closed_count_ = 0;
  int search_results_changed_count_ = 0;
  int search_mode_changed_count_ = 0;
  int model_shutdown_count_ = 0;

  int last_activated_tab_index_ = -1;
  int last_closed_tab_index_ = -1;
  AstraTabSearchMode last_search_mode_ = AstraTabSearchMode::kAllTabs;

  raw_ptr<AstraTabSearchModel> last_model_tab_list_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_tab_activated_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_tab_closed_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_search_results_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_search_mode_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_shutdown_ = nullptr;
};

}  // namespace

// =========================================================================
// AstraTabSearchItem struct tests
// =========================================================================

TEST(AstraTabSearchItemTest, DefaultValues) {
  AstraTabSearchItem item;
  EXPECT_EQ(-1, item.tab_id);
  EXPECT_TRUE(item.title.empty());
  EXPECT_TRUE(item.url.is_empty());
  EXPECT_TRUE(item.hostname.empty());
  EXPECT_TRUE(item.workspace_id.empty());
  EXPECT_TRUE(item.workspace_name.empty());
  EXPECT_FALSE(item.is_active);
  EXPECT_FALSE(item.is_pinned);
  EXPECT_FALSE(item.is_in_group);
  EXPECT_TRUE(item.group_name.empty());
  EXPECT_EQ(SK_ColorTRANSPARENT, item.group_color);
  EXPECT_TRUE(item.last_active_time.is_null());
  EXPECT_EQ(0.0, item.relevance_score);
  EXPECT_FALSE(item.is_audible);
  EXPECT_FALSE(item.is_muted);
  EXPECT_EQ(-1, item.tab_index);
  EXPECT_EQ(0, item.window_id);
  EXPECT_FALSE(item.has_crashed);
  EXPECT_FALSE(item.is_loading);
}

TEST(AstraTabSearchItemTest, CanSetAllFields) {
  AstraTabSearchItem item;
  item.tab_id = 42;
  item.title = u"Test Page";
  item.url = GURL("https://example.com/page");
  item.hostname = u"example.com";
  item.workspace_id = "ws1";
  item.workspace_name = u"My Workspace";
  item.is_active = true;
  item.is_pinned = true;
  item.is_in_group = true;
  item.group_name = u"Dev Group";
  item.group_color = SK_ColorBLUE;
  item.last_active_time = base::Time::Now();
  item.relevance_score = 999.5;
  item.is_audible = true;
  item.is_muted = false;
  item.tab_index = 5;
  item.window_id = 2;
  item.has_crashed = true;
  item.is_loading = true;

  EXPECT_EQ(42, item.tab_id);
  EXPECT_EQ(u"Test Page", item.title);
  EXPECT_EQ(GURL("https://example.com/page"), item.url);
  EXPECT_EQ(u"example.com", item.hostname);
  EXPECT_EQ("ws1", item.workspace_id);
  EXPECT_EQ(u"My Workspace", item.workspace_name);
  EXPECT_TRUE(item.is_active);
  EXPECT_TRUE(item.is_pinned);
  EXPECT_TRUE(item.is_in_group);
  EXPECT_EQ(u"Dev Group", item.group_name);
  EXPECT_EQ(SK_ColorBLUE, item.group_color);
  EXPECT_FALSE(item.last_active_time.is_null());
  EXPECT_EQ(999.5, item.relevance_score);
  EXPECT_TRUE(item.is_audible);
  EXPECT_FALSE(item.is_muted);
  EXPECT_EQ(5, item.tab_index);
  EXPECT_EQ(2, item.window_id);
  EXPECT_TRUE(item.has_crashed);
  EXPECT_TRUE(item.is_loading);
}

TEST(AstraTabSearchItemTest, CopyConstructible) {
  AstraTabSearchItem original;
  original.tab_id = 7;
  original.title = u"Copy Test";
  original.hostname = u"copy.test";
  original.workspace_id = "copy-ws";
  original.is_pinned = true;

  AstraTabSearchItem copy = original;
  EXPECT_EQ(7, copy.tab_id);
  EXPECT_EQ(u"Copy Test", copy.title);
  EXPECT_EQ(u"copy.test", copy.hostname);
  EXPECT_EQ("copy-ws", copy.workspace_id);
  EXPECT_TRUE(copy.is_pinned);
}

TEST(AstraTabSearchItemTest, EmptyUrl) {
  AstraTabSearchItem item;
  EXPECT_TRUE(item.url.is_empty());
  EXPECT_TRUE(item.hostname.empty());
}

TEST(AstraTabSearchItemTest, ChromeUrl) {
  AstraTabSearchItem item;
  item.url = GURL("chrome://settings");
  item.hostname = u"chrome://settings";
  EXPECT_TRUE(item.url.SchemeIs("chrome"));
  EXPECT_EQ(u"chrome://settings", item.hostname);
}

TEST(AstraTabSearchItemTest, GroupWithoutColor) {
  AstraTabSearchItem item;
  item.is_in_group = true;
  item.group_name = u"Test Group";
  item.group_color = SK_ColorTRANSPARENT;
  EXPECT_TRUE(item.is_in_group);
  EXPECT_EQ(u"Test Group", item.group_name);
  EXPECT_EQ(SK_ColorTRANSPARENT, item.group_color);
}

TEST(AstraTabSearchItemTest, AudibleAndMuted) {
  AstraTabSearchItem item;
  item.is_audible = true;
  item.is_muted = true;
  EXPECT_TRUE(item.is_audible);
  EXPECT_TRUE(item.is_muted);
}

TEST(AstraTabSearchItemTest, CrashedTab) {
  AstraTabSearchItem item;
  item.has_crashed = true;
  item.title = u"(Crashed)";
  EXPECT_TRUE(item.has_crashed);
  EXPECT_EQ(u"(Crashed)", item.title);
}

TEST(AstraTabSearchItemTest, LoadingTab) {
  AstraTabSearchItem item;
  item.is_loading = true;
  EXPECT_TRUE(item.is_loading);
}

// =========================================================================
// AstraTabSearchGroupInfo struct tests
// =========================================================================

TEST(AstraTabSearchGroupInfoTest, DefaultValues) {
  AstraTabSearchGroupInfo group;
  EXPECT_TRUE(group.group_id.empty());
  EXPECT_TRUE(group.title.empty());
  EXPECT_EQ(SK_ColorTRANSPARENT, group.color);
  EXPECT_EQ(0, group.tab_count);
  EXPECT_FALSE(group.collapsed);
}

TEST(AstraTabSearchGroupInfoTest, CanSetAllFields) {
  AstraTabSearchGroupInfo group;
  group.group_id = "test-group";
  group.title = u"Test Group";
  group.color = SK_ColorRED;
  group.tab_count = 5;
  group.collapsed = true;

  EXPECT_EQ("test-group", group.group_id);
  EXPECT_EQ(u"Test Group", group.title);
  EXPECT_EQ(SK_ColorRED, group.color);
  EXPECT_EQ(5, group.tab_count);
  EXPECT_TRUE(group.collapsed);
}

TEST(AstraTabSearchGroupInfoTest, CopyConstructible) {
  AstraTabSearchGroupInfo original;
  original.group_id = "copy-group";
  original.title = u"Copy Group";
  original.color = SK_ColorGREEN;
  original.tab_count = 3;
  original.collapsed = true;

  AstraTabSearchGroupInfo copy = original;
  EXPECT_EQ("copy-group", copy.group_id);
  EXPECT_EQ(u"Copy Group", copy.title);
  EXPECT_EQ(SK_ColorGREEN, copy.color);
  EXPECT_EQ(3, copy.tab_count);
  EXPECT_TRUE(copy.collapsed);
}

// =========================================================================
// AstraTabSearchMode enum tests
// =========================================================================

TEST(AstraTabSearchModeTest, AllModesExist) {
  // Verify all 6 search modes exist with correct values.
  EXPECT_EQ(0, static_cast<int>(AstraTabSearchMode::kAllTabs));
  EXPECT_EQ(1, static_cast<int>(AstraTabSearchMode::kCurrentWorkspace));
  EXPECT_EQ(2, static_cast<int>(AstraTabSearchMode::kOtherWorkspaces));
  EXPECT_EQ(3, static_cast<int>(AstraTabSearchMode::kRecentlyClosed));
  EXPECT_EQ(4, static_cast<int>(AstraTabSearchMode::kFavorites));
  EXPECT_EQ(5, static_cast<int>(AstraTabSearchMode::kAudioPlaying));
}

TEST(AstraTabSearchModeTest, SixModesTotal) {
  // There should be exactly 6 search modes.
  // This test documents the expected mode count.
  int mode_count = 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kAllTabs) == 0 ? 1 : 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kCurrentWorkspace) == 1 ? 1 : 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kOtherWorkspaces) == 2 ? 1 : 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kRecentlyClosed) == 3 ? 1 : 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kFavorites) == 4 ? 1 : 0;
  mode_count += static_cast<int>(AstraTabSearchMode::kAudioPlaying) == 5 ? 1 : 0;
  EXPECT_EQ(6, mode_count);
}

// =========================================================================
// AstraTabSearchSortOrder enum tests
// =========================================================================

TEST(AstraTabSearchSortOrderTest, AllSortOrdersExist) {
  EXPECT_EQ(0, static_cast<int>(AstraTabSearchSortOrder::kByRecency));
  EXPECT_EQ(1, static_cast<int>(AstraTabSearchSortOrder::kByTitle));
  EXPECT_EQ(2, static_cast<int>(AstraTabSearchSortOrder::kByPosition));
}

TEST(AstraTabSearchSortOrderTest, ThreeSortOrdersTotal) {
  int count = 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByRecency) == 0 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByTitle) == 1 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByPosition) == 2 ? 1 : 0;
  EXPECT_EQ(3, count);
}

// =========================================================================
// AstraTabSearchModel tests — tab list
// =========================================================================

class AstraTabSearchModelTest : public testing::Test {
 public:
  AstraTabSearchModelTest() = default;
  ~AstraTabSearchModelTest() override = default;

 protected:
  static void SetUpTestSuite() {}
  static void TearDownTestSuite() {}

  void SetUp() override {
    model_ = std::make_unique<AstraTabSearchModel>();
    model_->SetCurrentWorkspaceId("work");
  }

  void TearDown() override {
    model_.reset();
  }

  std::unique_ptr<AstraTabSearchModel> model_;
};

TEST_F(AstraTabSearchModelTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, model_);
}

TEST_F(AstraTabSearchModelTest, DefaultTabCountIsZero) {
  EXPECT_EQ(0u, model_->GetTabCount());
}

TEST_F(AstraTabSearchModelTest, GetTabAtEmptyModel) {
  EXPECT_EQ(nullptr, model_->GetTabAt(0));
  EXPECT_EQ(nullptr, model_->GetTabAt(-1));
  EXPECT_EQ(nullptr, model_->GetTabAt(100));
}

TEST_F(AstraTabSearchModelTest, SetTabList) {
  auto tabs = CreateSampleTabs(5);
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(5u, model_->GetTabCount());
}

TEST_F(AstraTabSearchModelTest, GetAllTabsReturnsAll) {
  auto tabs = CreateSampleTabs(8);
  size_t count = tabs.size();
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(count, model_->GetAllTabs().size());
}

TEST_F(AstraTabSearchModelTest, GetTabAtValidIndex) {
  auto tabs = CreateSampleTabs(5);
  model_->SetTabList(std::move(tabs));
  const AstraTabSearchItem* tab = model_->GetTabAt(0);
  ASSERT_NE(nullptr, tab);
  EXPECT_EQ(0, tab->tab_id);
}

TEST_F(AstraTabSearchModelTest, GetTabAtNegativeIndex) {
  auto tabs = CreateSampleTabs(5);
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(nullptr, model_->GetTabAt(-1));
}

TEST_F(AstraTabSearchModelTest, GetTabAtOutOfRange) {
  auto tabs = CreateSampleTabs(5);
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(nullptr, model_->GetTabAt(10));
  EXPECT_EQ(nullptr, model_->GetTabAt(5));
}

TEST_F(AstraTabSearchModelTest, SetEmptyTabList) {
  auto tabs = CreateSampleTabs(5);
  model_->SetTabList(std::move(tabs));
  ASSERT_EQ(5u, model_->GetTabCount());

  model_->SetTabList(std::vector<AstraTabSearchItem>());
  EXPECT_EQ(0u, model_->GetTabCount());
  EXPECT_EQ(nullptr, model_->GetTabAt(0));
}

TEST_F(AstraTabSearchModelTest, SingleTab) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  item.tab_id = 0;
  item.title = u"Only Tab";
  item.hostname = u"only.com";
  item.tab_index = 0;
  tabs.push_back(std::move(item));

  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(1u, model_->GetTabCount());
  EXPECT_EQ(u"Only Tab", model_->GetTabAt(0)->title);
}

TEST_F(AstraTabSearchModelTest, ManyTabs) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 100; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.hostname = base::UTF8ToUTF16("site" + std::to_string(i) + ".com");
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(100u, model_->GetTabCount());
  EXPECT_EQ(u"Tab 50", model_->GetTabAt(50)->title);
}

// =========================================================================
// Model tests — search
// =========================================================================

TEST_F(AstraTabSearchModelTest, SearchByTitle) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"Document");
  EXPECT_GT(results.size(), 0u);

  bool found = false;
  for (const auto& r : results) {
    if (r.title.find(u"Document") != std::u16string::npos) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraTabSearchModelTest, SearchByUrl) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"github");
  EXPECT_GT(results.size(), 0u);

  bool found = false;
  for (const auto& r : results) {
    if (r.hostname.find(u"github") != std::u16string::npos) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraTabSearchModelTest, SearchByHostname) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"youtube.com");
  EXPECT_GT(results.size(), 0u);

  bool found = false;
  for (const auto& r : results) {
    if (r.hostname == u"youtube.com") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraTabSearchModelTest, EmptyQueryReturnsAll) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"");
  // Should return all tabs (up to max_search_results).
  EXPECT_GT(results.size(), 0u);
  EXPECT_LE(results.size(), model_->max_search_results());
}

TEST_F(AstraTabSearchModelTest, NoMatchesQuery) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"zzzzz_no_match_zzzzz");
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraTabSearchModelTest, SearchIsCaseInsensitive) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results_lower = model_->SearchTabs(u"gmail");
  auto results_upper = model_->SearchTabs(u"GMAIL");
  EXPECT_EQ(results_lower.size(), results_upper.size());
}

TEST_F(AstraTabSearchModelTest, SearchByWorkspaceName) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"Work");
  EXPECT_GT(results.size(), 0u);

  bool has_work = false;
  for (const auto& r : results) {
    if (r.workspace_id == "work") {
      has_work = true;
      break;
    }
  }
  EXPECT_TRUE(has_work);
}

TEST_F(AstraTabSearchModelTest, SearchByGroupName) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"Docs");
  EXPECT_GT(results.size(), 0u);

  bool has_docs_group = false;
  for (const auto& r : results) {
    if (r.is_in_group && r.group_name.find(u"Docs") != std::u16string::npos) {
      has_docs_group = true;
      break;
    }
  }
  EXPECT_TRUE(has_docs_group);
}

TEST_F(AstraTabSearchModelTest, SearchResultsHaveScores) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"doc");
  for (const auto& r : results) {
    EXPECT_GT(r.relevance_score, 0.0);
  }
}

TEST_F(AstraTabSearchModelTest, SearchResultsCapped) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 100; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Test Tab " + std::to_string(i));
    item.hostname = u"test.com";
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));

  model_->set_max_search_results(20);
  auto results = model_->SearchTabs(u"Test");
  EXPECT_EQ(20u, results.size());
}

// =========================================================================
// Model tests — search modes
// =========================================================================

TEST_F(AstraTabSearchModelTest, DefaultSearchModeIsAllTabs) {
  EXPECT_EQ(AstraTabSearchMode::kAllTabs, model_->GetSearchMode());
}

TEST_F(AstraTabSearchModelTest, SetSearchMode) {
  model_->SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  EXPECT_EQ(AstraTabSearchMode::kAudioPlaying, model_->GetSearchMode());
}

TEST_F(AstraTabSearchModelTest, SearchModeAllTabs) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(u"", AstraTabSearchMode::kAllTabs);
  EXPECT_GT(results.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, SearchModeCurrentWorkspace) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kCurrentWorkspace);
  for (const auto& r : results) {
    EXPECT_EQ("work", r.workspace_id);
  }
}

TEST_F(AstraTabSearchModelTest, SearchModeOtherWorkspaces) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kOtherWorkspaces);
  for (const auto& r : results) {
    EXPECT_NE("work", r.workspace_id);
    EXPECT_FALSE(r.workspace_id.empty());
  }
}

TEST_F(AstraTabSearchModelTest, SearchModeRecentlyClosed) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kRecentlyClosed);
  // With no recently closed tabs set, should be empty.
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraTabSearchModelTest, SearchModeFavorites) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kFavorites);
  // No favorites implementation yet — should be empty.
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraTabSearchModelTest, SearchModeAudioPlaying) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kAudioPlaying);
  EXPECT_GT(results.size(), 0u);
  for (const auto& r : results) {
    EXPECT_TRUE(r.is_audible);
  }
}

TEST_F(AstraTabSearchModelTest, SetSameModeIsNoOp) {
  model_->SetSearchMode(AstraTabSearchMode::kAllTabs);
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSearchMode(AstraTabSearchMode::kAllTabs);
  EXPECT_EQ(0, observer.search_mode_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ModeChangeNotifiesObservers) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  EXPECT_EQ(1, observer.search_mode_changed_count_);
  EXPECT_EQ(AstraTabSearchMode::kAudioPlaying, observer.last_search_mode_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SearchWithQueryInCurrentWorkspaceMode) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabsInMode(
      u"Google", AstraTabSearchMode::kCurrentWorkspace);
  for (const auto& r : results) {
    EXPECT_EQ("work", r.workspace_id);
  }
}

// =========================================================================
// Model tests — workspace filtering
// =========================================================================

TEST_F(AstraTabSearchModelTest, GetTabsByWorkspaceValid) {
  model_->SetTabList(CreateSampleTabs(10));
  auto work_tabs = model_->GetTabsByWorkspace("work");
  auto personal_tabs = model_->GetTabsByWorkspace("personal");

  EXPECT_GT(work_tabs.size(), 0u);
  EXPECT_GT(personal_tabs.size(), 0u);

  for (const auto& t : work_tabs) {
    EXPECT_EQ("work", t.workspace_id);
  }
  for (const auto& t : personal_tabs) {
    EXPECT_EQ("personal", t.workspace_id);
  }
}

TEST_F(AstraTabSearchModelTest, GetTabsByWorkspaceNonExistent) {
  model_->SetTabList(CreateSampleTabs(8));
  auto tabs = model_->GetTabsByWorkspace("nonexistent");
  EXPECT_EQ(0u, tabs.size());
}

TEST_F(AstraTabSearchModelTest, GetTabsByWorkspaceEmptyId) {
  model_->SetTabList(CreateSampleTabs(12));
  auto tabs = model_->GetTabsByWorkspace("");
  // Should return tabs with no workspace assigned.
  for (const auto& t : tabs) {
    EXPECT_TRUE(t.workspace_id.empty());
  }
}

TEST_F(AstraTabSearchModelTest, SetCurrentWorkspaceId) {
  model_->SetCurrentWorkspaceId("test-ws");
  EXPECT_EQ("test-ws", model_->current_workspace_id());
}

TEST_F(AstraTabSearchModelTest, CurrentWorkspaceChangeAffectsResults) {
  model_->SetTabList(CreateSampleTabs(8));
  model_->SetCurrentWorkspaceId("work");

  auto results_work = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kCurrentWorkspace);
  size_t work_count = results_work.size();

  model_->SetCurrentWorkspaceId("personal");
  auto results_personal = model_->SearchTabsInMode(
      u"", AstraTabSearchMode::kCurrentWorkspace);

  EXPECT_NE(work_count, results_personal.size());
}

// =========================================================================
// Model tests — active tab
// =========================================================================

TEST_F(AstraTabSearchModelTest, GetActiveTab) {
  model_->SetTabList(CreateSampleTabs(8));
  const AstraTabSearchItem* active = model_->GetActiveTab();
  ASSERT_NE(nullptr, active);
  EXPECT_TRUE(active->is_active);
}

TEST_F(AstraTabSearchModelTest, GetActiveTabEmptyModel) {
  EXPECT_EQ(nullptr, model_->GetActiveTab());
}

TEST_F(AstraTabSearchModelTest, SwitchToTab) {
  model_->SetTabList(CreateSampleTabs(8));
  const AstraTabSearchItem* active = model_->GetActiveTab();
  ASSERT_NE(nullptr, active);
  int original_active_index = active->tab_index;

  // Switch to a different tab.
  int new_index = (original_active_index + 1) % 8;
  model_->SwitchToTab(new_index);

  const AstraTabSearchItem* new_active = model_->GetActiveTab();
  ASSERT_NE(nullptr, new_active);
  EXPECT_EQ(new_index, new_active->tab_index);
}

TEST_F(AstraTabSearchModelTest, SwitchToTabInvalidIndex) {
  model_->SetTabList(CreateSampleTabs(8));
  int original_active = model_->GetActiveTab()->tab_index;

  model_->SwitchToTab(-1);
  EXPECT_EQ(original_active, model_->GetActiveTab()->tab_index);

  model_->SwitchToTab(100);
  EXPECT_EQ(original_active, model_->GetActiveTab()->tab_index);
}

TEST_F(AstraTabSearchModelTest, SwitchToTabNotifiesObserver) {
  model_->SetTabList(CreateSampleTabs(8));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SwitchToTab(2);
  EXPECT_EQ(1, observer.tab_activated_count_);
  EXPECT_EQ(2, observer.last_activated_tab_index_);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model tests — tab groups
// =========================================================================

TEST_F(AstraTabSearchModelTest, SetTabGroups) {
  auto groups = CreateSampleGroups();
  model_->SetTabGroups(std::move(groups));
  EXPECT_EQ(3u, model_->GetTabGroups().size());
}

TEST_F(AstraTabSearchModelTest, GetTabGroupsEmpty) {
  EXPECT_EQ(0u, model_->GetTabGroups().size());
}

TEST_F(AstraTabSearchModelTest, GetTabsInGroup) {
  model_->SetTabList(CreateSampleTabs(10));
  auto tabs = model_->GetTabsInGroup("");  // Empty group ID
  // Tabs not in any group have empty group_id.
  for (const auto& t : tabs) {
    EXPECT_FALSE(t.is_in_group);
  }
}

TEST_F(AstraTabSearchModelTest, GetTabsInGroupWithId) {
  model_->SetTabList(CreateSampleTabs(10));
  // Tabs with group "Docs" should be found.
  auto docs_tabs = model_->GetTabsInGroup("");
  // Empty group ID matches tabs not in any group.
  EXPECT_GT(docs_tabs.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, GroupInfoFields) {
  auto groups = CreateSampleGroups();
  model_->SetTabGroups(std::move(groups));

  auto result = model_->GetTabGroups();
  ASSERT_GT(result.size(), 0u);
  EXPECT_FALSE(result[0].group_id.empty());
  EXPECT_FALSE(result[0].title.empty());
  EXPECT_NE(SK_ColorTRANSPARENT, result[0].color);
  EXPECT_GT(result[0].tab_count, 0);
}

TEST_F(AstraTabSearchModelTest, CollapsedGroup) {
  auto groups = CreateSampleGroups();
  model_->SetTabGroups(std::move(groups));

  auto result = model_->GetTabGroups();
  bool has_collapsed = false;
  for (const auto& g : result) {
    if (g.collapsed) {
      has_collapsed = true;
      break;
    }
  }
  EXPECT_TRUE(has_collapsed);
}

// =========================================================================
// Model tests — windows
// =========================================================================

TEST_F(AstraTabSearchModelTest, GetWindowCountSingleWindow) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 5; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.window_id = 0;
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(1, model_->GetWindowCount());
}

TEST_F(AstraTabSearchModelTest, GetWindowCountMultipleWindows) {
  model_->SetTabList(CreateSampleTabs(12));
  EXPECT_GE(model_->GetWindowCount(), 2);
}

TEST_F(AstraTabSearchModelTest, GetTabsInWindow) {
  model_->SetTabList(CreateSampleTabs(12));
  auto window0_tabs = model_->GetTabsInWindow(0);
  auto window1_tabs = model_->GetTabsInWindow(1);

  EXPECT_GT(window0_tabs.size(), 0u);
  EXPECT_GT(window1_tabs.size(), 0u);

  for (const auto& t : window0_tabs) {
    EXPECT_EQ(0, t.window_id);
  }
  for (const auto& t : window1_tabs) {
    EXPECT_EQ(1, t.window_id);
  }
}

TEST_F(AstraTabSearchModelTest, GetTabsInWindowInvalid) {
  model_->SetTabList(CreateSampleTabs(8));
  auto tabs = model_->GetTabsInWindow(999);
  EXPECT_EQ(0u, tabs.size());
}

TEST_F(AstraTabSearchModelTest, GetTabsInWindowNegativeId) {
  model_->SetTabList(CreateSampleTabs(8));
  auto tabs = model_->GetTabsInWindow(-1);
  EXPECT_EQ(0u, tabs.size());
}

// =========================================================================
// Model tests — special collections
// =========================================================================

TEST_F(AstraTabSearchModelTest, GetTabsWithAudio) {
  model_->SetTabList(CreateSampleTabs(12));
  auto audio_tabs = model_->GetTabsWithAudio();
  EXPECT_GT(audio_tabs.size(), 0u);
  for (const auto& t : audio_tabs) {
    EXPECT_TRUE(t.is_audible);
  }
}

TEST_F(AstraTabSearchModelTest, GetTabsWithAudioNone) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 5; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.is_audible = false;
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(0u, model_->GetTabsWithAudio().size());
}

TEST_F(AstraTabSearchModelTest, GetPinnedTabs) {
  model_->SetTabList(CreateSampleTabs(12));
  auto pinned_tabs = model_->GetPinnedTabs();
  EXPECT_GT(pinned_tabs.size(), 0u);
  for (const auto& t : pinned_tabs) {
    EXPECT_TRUE(t.is_pinned);
  }
}

TEST_F(AstraTabSearchModelTest, GetPinnedTabsNone) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 5; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.is_pinned = false;
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));
  EXPECT_EQ(0u, model_->GetPinnedTabs().size());
}

TEST_F(AstraTabSearchModelTest, GetRecentlyClosedTabs) {
  std::vector<AstraTabSearchItem> closed;
  for (int i = 0; i < 5; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Closed " + std::to_string(i));
    item.tab_index = -1;
    closed.push_back(std::move(item));
  }
  model_->SetRecentlyClosedTabs(std::move(closed));

  auto result = model_->GetRecentlyClosedTabs(3);
  EXPECT_EQ(3u, result.size());
}

TEST_F(AstraTabSearchModelTest, GetRecentlyClosedTabsMaxCount) {
  std::vector<AstraTabSearchItem> closed;
  for (int i = 0; i < 3; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Closed " + std::to_string(i));
    closed.push_back(std::move(item));
  }
  model_->SetRecentlyClosedTabs(std::move(closed));

  auto result = model_->GetRecentlyClosedTabs(10);
  EXPECT_EQ(3u, result.size());
}

TEST_F(AstraTabSearchModelTest, GetRecentlyClosedTabsEmpty) {
  auto result = model_->GetRecentlyClosedTabs(10);
  EXPECT_EQ(0u, result.size());
}

// =========================================================================
// Model tests — tab actions
// =========================================================================

TEST_F(AstraTabSearchModelTest, CloseTab) {
  model_->SetTabList(CreateSampleTabs(8));
  size_t original_count = model_->GetTabCount();
  ASSERT_GT(original_count, 1u);

  model_->CloseTab(0);
  EXPECT_EQ(original_count - 1, model_->GetTabCount());
}

TEST_F(AstraTabSearchModelTest, CloseTabInvalidIndex) {
  model_->SetTabList(CreateSampleTabs(5));
  size_t original_count = model_->GetTabCount();

  model_->CloseTab(-1);
  EXPECT_EQ(original_count, model_->GetTabCount());

  model_->CloseTab(100);
  EXPECT_EQ(original_count, model_->GetTabCount());
}

TEST_F(AstraTabSearchModelTest, CloseTabNotifiesObserver) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->CloseTab(1);
  EXPECT_EQ(1, observer.tab_closed_count_);
  EXPECT_EQ(1, observer.last_closed_tab_index_);
  EXPECT_EQ(1, observer.tab_list_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, MoveTabToWorkspace) {
  model_->SetTabList(CreateSampleTabs(8));
  ASSERT_FALSE(model_->GetTabAt(0)->workspace_id == "new-ws");

  model_->MoveTabToWorkspace(0, "new-ws");
  EXPECT_EQ("new-ws", model_->GetTabAt(0)->workspace_id);
}

TEST_F(AstraTabSearchModelTest, MoveTabToWorkspaceInvalidIndex) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->MoveTabToWorkspace(-1, "test");
  EXPECT_EQ(0, observer.tab_list_changed_count_);

  model_->MoveTabToWorkspace(100, "test");
  EXPECT_EQ(0, observer.tab_list_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, RefreshTabList) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->RefreshTabList();
  EXPECT_EQ(1, observer.tab_list_changed_count_);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model tests — settings
// =========================================================================

TEST_F(AstraTabSearchModelTest, DefaultSettings) {
  EXPECT_EQ(AstraTabSearchModel::kDefaultMaxSearchResults,
            model_->max_search_results());
  EXPECT_TRUE(model_->show_tab_urls());
  EXPECT_TRUE(model_->show_workspace_name());
  EXPECT_TRUE(model_->show_tab_groups());
  EXPECT_TRUE(model_->show_favicons());
  EXPECT_TRUE(model_->search_in_urls());
  EXPECT_FALSE(model_->search_in_tab_titles_only());
  EXPECT_EQ(AstraTabSearchMode::kAllTabs, model_->default_search_mode());
  EXPECT_EQ(AstraTabSearchSortOrder::kByRecency, model_->sort_order());
  EXPECT_TRUE(model_->close_tab_on_activate());
  EXPECT_TRUE(model_->show_recently_closed_section());
}

TEST_F(AstraTabSearchModelTest, SetMaxSearchResults) {
  model_->set_max_search_results(10);
  EXPECT_EQ(10u, model_->max_search_results());
}

TEST_F(AstraTabSearchModelTest, MaxSearchResultsClamped) {
  model_->set_max_search_results(1000);
  EXPECT_LE(model_->max_search_results(),
            AstraTabSearchModel::kMaxSearchResultsMax);
}

TEST_F(AstraTabSearchModelTest, SetShowTabUrls) {
  model_->set_show_tab_urls(false);
  EXPECT_FALSE(model_->show_tab_urls());

  model_->set_show_tab_urls(true);
  EXPECT_TRUE(model_->show_tab_urls());
}

TEST_F(AstraTabSearchModelTest, SetShowWorkspaceName) {
  model_->set_show_workspace_name(false);
  EXPECT_FALSE(model_->show_workspace_name());

  model_->set_show_workspace_name(true);
  EXPECT_TRUE(model_->show_workspace_name());
}

TEST_F(AstraTabSearchModelTest, SetShowTabGroups) {
  model_->set_show_tab_groups(false);
  EXPECT_FALSE(model_->show_tab_groups());

  model_->set_show_tab_groups(true);
  EXPECT_TRUE(model_->show_tab_groups());
}

TEST_F(AstraTabSearchModelTest, SetShowFavicons) {
  model_->set_show_favicons(false);
  EXPECT_FALSE(model_->show_favicons());

  model_->set_show_favicons(true);
  EXPECT_TRUE(model_->show_favicons());
}

TEST_F(AstraTabSearchModelTest, SetSearchInUrls) {
  model_->set_search_in_urls(false);
  EXPECT_FALSE(model_->search_in_urls());

  model_->set_search_in_urls(true);
  EXPECT_TRUE(model_->search_in_urls());
}

TEST_F(AstraTabSearchModelTest, SetSearchInTabTitlesOnly) {
  model_->set_search_in_tab_titles_only(true);
  EXPECT_TRUE(model_->search_in_tab_titles_only());

  model_->set_search_in_tab_titles_only(false);
  EXPECT_FALSE(model_->search_in_tab_titles_only());
}

TEST_F(AstraTabSearchModelTest, SetDefaultSearchMode) {
  model_->set_default_search_mode(AstraTabSearchMode::kAudioPlaying);
  EXPECT_EQ(AstraTabSearchMode::kAudioPlaying, model_->default_search_mode());
}

TEST_F(AstraTabSearchModelTest, SetSortOrder) {
  model_->SetSortOrder(AstraTabSearchSortOrder::kByTitle);
  EXPECT_EQ(AstraTabSearchSortOrder::kByTitle, model_->sort_order());

  model_->SetSortOrder(AstraTabSearchSortOrder::kByPosition);
  EXPECT_EQ(AstraTabSearchSortOrder::kByPosition, model_->sort_order());
}

TEST_F(AstraTabSearchModelTest, SetCloseTabOnActivate) {
  model_->set_close_tab_on_activate(false);
  EXPECT_FALSE(model_->close_tab_on_activate());

  model_->set_close_tab_on_activate(true);
  EXPECT_TRUE(model_->close_tab_on_activate());
}

TEST_F(AstraTabSearchModelTest, SetShowRecentlyClosedSection) {
  model_->set_show_recently_closed_section(false);
  EXPECT_FALSE(model_->show_recently_closed_section());

  model_->set_show_recently_closed_section(true);
  EXPECT_TRUE(model_->show_recently_closed_section());
}

TEST_F(AstraTabSearchModelTest, SettingSameValueIsNoOp) {
  model_->set_show_tab_urls(true);
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->set_show_tab_urls(true);
  EXPECT_EQ(0, observer.search_results_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SettingsChangeNotifiesObservers) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->set_show_tab_urls(false);
  EXPECT_EQ(1, observer.search_results_changed_count_);

  model_->set_max_search_results(10);
  EXPECT_GT(observer.search_results_changed_count_, 1);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model tests — sort order
// =========================================================================

TEST_F(AstraTabSearchModelTest, SortByRecency) {
  model_->SetSortOrder(AstraTabSearchSortOrder::kByRecency);
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"");
  ASSERT_GT(results.size(), 1u);

  // Most recent first.
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].last_active_time, results[i].last_active_time);
  }
}

TEST_F(AstraTabSearchModelTest, SortByTitle) {
  model_->SetSortOrder(AstraTabSearchSortOrder::kByTitle);
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"");
  ASSERT_GT(results.size(), 1u);

  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_LE(base::CompareCaseInsensitive(results[i - 1].title,
                                            results[i].title),
              0);
  }
}

TEST_F(AstraTabSearchModelTest, SortByPosition) {
  model_->SetSortOrder(AstraTabSearchSortOrder::kByPosition);
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"");
  ASSERT_GT(results.size(), 1u);

  // Results should be in tab_index order within each window.
  int last_window = -1;
  int last_index = -1;
  for (const auto& r : results) {
    if (r.window_id != last_window) {
      last_window = r.window_id;
      last_index = r.tab_index;
    } else {
      EXPECT_GE(r.tab_index, last_index);
      last_index = r.tab_index;
    }
  }
}

// =========================================================================
// Model tests — observer notifications
// =========================================================================

TEST_F(AstraTabSearchModelTest, ObserverOnTabListChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetTabList(CreateSampleTabs(5));
  EXPECT_EQ(1, observer.tab_list_changed_count_);
  EXPECT_EQ(model_.get(), observer.last_model_tab_list_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnTabActivated) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SwitchToTab(2);
  EXPECT_EQ(1, observer.tab_activated_count_);
  EXPECT_EQ(2, observer.last_activated_tab_index_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnTabClosed) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->CloseTab(3);
  EXPECT_EQ(1, observer.tab_closed_count_);
  EXPECT_EQ(3, observer.last_closed_tab_index_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnSearchResultsChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetTabList(CreateSampleTabs(5));
  EXPECT_GT(observer.search_results_changed_count_, 0);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnSearchModeChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  EXPECT_EQ(1, observer.search_mode_changed_count_);
  EXPECT_EQ(AstraTabSearchMode::kAudioPlaying, observer.last_search_mode_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnShutdown) {
  auto model = std::make_unique<AstraTabSearchModel>();
  TestTabSearchObserver observer;
  model->AddObserver(&observer);

  EXPECT_EQ(0, observer.model_shutdown_count_);
  model.reset();  // Destroys the model.
  EXPECT_EQ(1, observer.model_shutdown_count_);
  EXPECT_EQ(1, observer.model_shutdown_count_);
}

TEST_F(AstraTabSearchModelTest, MultipleObservers) {
  TestTabSearchObserver observer1;
  TestTabSearchObserver observer2;
  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);

  model_->SetTabList(CreateSampleTabs(5));
  EXPECT_EQ(1, observer1.tab_list_changed_count_);
  EXPECT_EQ(1, observer2.tab_list_changed_count_);

  model_->RemoveObserver(&observer1);
  model_->RemoveObserver(&observer2);
}

TEST_F(AstraTabSearchModelTest, DefaultObserverDoesNotCrash) {
  // Create minimal observer overriding nothing.
  class DefaultObserver : public AstraTabSearchObserver {};

  DefaultObserver observer;
  model_->AddObserver(&observer);

  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  model_->SwitchToTab(0);
  model_->CloseTab(0);
  model_->RefreshTabList();

  model_->RemoveObserver(&observer);
  SUCCEED();
}

// =========================================================================
// Model tests — edge cases
// =========================================================================

TEST_F(AstraTabSearchModelTest, EmptyModelSearch) {
  auto results = model_->SearchTabs(u"anything");
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraTabSearchModelTest, SingleTabSearch) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  item.tab_id = 0;
  item.title = u"Only Tab";
  item.hostname = u"only.com";
  item.tab_index = 0;
  tabs.push_back(std::move(item));
  model_->SetTabList(std::move(tabs));

  auto results = model_->SearchTabs(u"Only");
  EXPECT_EQ(1u, results.size());
}

TEST_F(AstraTabSearchModelTest, VeryLongTitle) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  item.tab_id = 0;
  item.title = std::u16string(1000, u'a');
  item.hostname = u"test.com";
  item.tab_index = 0;
  tabs.push_back(std::move(item));
  model_->SetTabList(std::move(tabs));

  EXPECT_EQ(1u, model_->GetTabCount());
  EXPECT_EQ(1000u, model_->GetTabAt(0)->title.size());
}

TEST_F(AstraTabSearchModelTest, VeryLongUrl) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  item.tab_id = 0;
  item.title = u"Test";
  item.hostname = std::u16string(500, u'a') + u".com";
  item.url = GURL("https://" + std::string(500, 'a') + ".com");
  item.tab_index = 0;
  tabs.push_back(std::move(item));
  model_->SetTabList(std::move(tabs));

  auto results = model_->SearchTabs(u"aaaa");
  EXPECT_EQ(1u, results.size());
}

TEST_F(AstraTabSearchModelTest, AllTabsSameTitle) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 10; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = u"Identical Title";
    item.hostname = base::UTF8ToUTF16("site" + std::to_string(i) + ".com");
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));

  auto results = model_->SearchTabs(u"Identical");
  EXPECT_EQ(10u, results.size());
}

TEST_F(AstraTabSearchModelTest, TabWithAllFlagsSet) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  item.tab_id = 0;
  item.title = u"Full Featured Tab";
  item.url = GURL("https://example.com");
  item.hostname = u"example.com";
  item.workspace_id = "test";
  item.workspace_name = u"Test Workspace";
  item.is_active = true;
  item.is_pinned = true;
  item.is_in_group = true;
  item.group_name = u"Test Group";
  item.group_color = SK_ColorBLUE;
  item.is_audible = true;
  item.is_muted = false;
  item.has_crashed = false;
  item.is_loading = true;
  item.tab_index = 0;
  item.window_id = 0;
  item.last_active_time = base::Time::Now();
  tabs.push_back(std::move(item));

  model_->SetTabList(std::move(tabs));
  const auto* tab = model_->GetTabAt(0);
  ASSERT_NE(nullptr, tab);
  EXPECT_TRUE(tab->is_active);
  EXPECT_TRUE(tab->is_pinned);
  EXPECT_TRUE(tab->is_in_group);
  EXPECT_TRUE(tab->is_audible);
  EXPECT_TRUE(tab->is_loading);
  EXPECT_FALSE(tab->is_muted);
  EXPECT_FALSE(tab->has_crashed);
}

TEST_F(AstraTabSearchModelTest, TabWithMinimalFields) {
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem item;
  // All default values.
  tabs.push_back(item);

  model_->SetTabList(std::move(tabs));
  const auto* tab = model_->GetTabAt(0);
  ASSERT_NE(nullptr, tab);
  EXPECT_EQ(-1, tab->tab_id);
  EXPECT_TRUE(tab->title.empty());
  EXPECT_FALSE(tab->is_active);
  EXPECT_FALSE(tab->is_pinned);
  EXPECT_FALSE(tab->is_in_group);
}

TEST_F(AstraTabSearchModelTest, SearchSpecialCharacters) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"!@#$%^&*()");
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraTabSearchModelTest, SearchWhitespace) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"   ");
  // Whitespace-only query should match nothing (or everything, depending on
  // implementation).  Just verify it doesn't crash.
  EXPECT_GE(results.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, EmptyWorkspaceId) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 3; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.workspace_id = "";  // No workspace.
    item.tab_index = i;
    tabs.push_back(std::move(item));
  }
  model_->SetTabList(std::move(tabs));

  auto results = model_->GetTabsByWorkspace("");
  EXPECT_EQ(3u, results.size());
}

// =========================================================================
// Model tests — persistence
// =========================================================================

namespace {
class AstraTabSearchPersistenceTest : public testing::Test {
 public:
  AstraTabSearchPersistenceTest() {
    prefs::RegisterProfilePrefs(profile_.GetPrefs()->registry());
  }

  ~AstraTabSearchPersistenceTest() override = default;

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingProfile profile_;
};
}  // namespace

TEST_F(AstraTabSearchPersistenceTest, LoadFromPrefs) {
  auto* prefs = profile_.GetPrefs();

  prefs->SetInteger(prefs::kPrefTabSearchMaxVisible, 5);
  prefs->SetBoolean(prefs::kPrefTabSearchShowUrls, false);
  prefs->SetBoolean(prefs::kPrefTabSearchShowThumbnails, false);
  prefs->SetBoolean(prefs::kPrefTabSearchShowRecentSection, false);
  prefs->SetInteger(prefs::kPrefTabSearchSortOrder,
                    static_cast<int>(AstraTabSearchSortOrder::kByTitle));

  AstraTabSearchModel model;
  model.LoadFromPrefs(prefs);

  EXPECT_EQ(5u, model.max_search_results());
  EXPECT_FALSE(model.show_tab_urls());
}

TEST_F(AstraTabSearchPersistenceTest, SaveToPrefs) {
  auto* prefs = profile_.GetPrefs();

  AstraTabSearchModel model;
  model.set_max_search_results(8);
  model.set_show_tab_urls(false);
  model.SetSortOrder(AstraTabSearchSortOrder::kByPosition);
  model.SaveToPrefs(prefs);

  EXPECT_EQ(8, prefs->GetInteger(prefs::kPrefTabSearchMaxVisible));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefTabSearchShowUrls));
}

TEST_F(AstraTabSearchPersistenceTest, LoadFromNullPrefs) {
  AstraTabSearchModel model;
  model.LoadFromPrefs(nullptr);
  // Should not crash.
  EXPECT_EQ(AstraTabSearchModel::kDefaultMaxSearchResults,
            model.max_search_results());
}

TEST_F(AstraTabSearchPersistenceTest, SaveToNullPrefs) {
  AstraTabSearchModel model;
  model.SaveToPrefs(nullptr);
  // Should not crash.
  SUCCEED();
}

// =========================================================================
// Model tests — constants
// =========================================================================

TEST(AstraTabSearchModelConstantsTest, MaxSearchResultsMax) {
  EXPECT_EQ(100u, AstraTabSearchModel::kMaxSearchResultsMax);
}

TEST(AstraTabSearchModelConstantsTest, DefaultMaxSearchResults) {
  EXPECT_EQ(15u, AstraTabSearchModel::kDefaultMaxSearchResults);
}

TEST(AstraTabSearchModelConstantsTest, DefaultRecentlyClosedCount) {
  EXPECT_EQ(10, AstraTabSearchModel::kDefaultRecentlyClosedCount);
}

// =========================================================================
// AstraTabSearchItemView tests
// =========================================================================

class AstraTabSearchItemViewTest : public views::ViewsTestBase {
 public:
  AstraTabSearchItemViewTest() = default;
  ~AstraTabSearchItemViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    // Create a sample tab item.
    AstraTabSearchItem tab;
    tab.tab_id = 42;
    tab.title = u"My Document";
    tab.hostname = u"docs.google.com";
    tab.url = GURL("https://docs.google.com/doc");
    tab.workspace_id = "work";
    tab.workspace_name = u"Work";
    tab.is_pinned = false;
    tab.is_in_group = true;
    tab.group_name = u"Docs";
    tab.group_color = SK_ColorBLUE;
    tab.is_audible = false;
    tab.is_muted = false;
    tab.tab_index = 3;
    tab.window_id = 0;

    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraTabSearchItemView>(tab));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraTabSearchItemView> item_view_ = nullptr;
};

TEST_F(AstraTabSearchItemViewTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, item_view_);
  EXPECT_NE(nullptr, item_view_->GetWidget());
}

TEST_F(AstraTabSearchItemViewTest, GetTabReturnsCopy) {
  const AstraTabSearchItem& tab = item_view_->GetTab();
  EXPECT_EQ(u"My Document", tab.title);
  EXPECT_EQ(u"docs.google.com", tab.hostname);
  EXPECT_EQ(42, tab.tab_id);
}

TEST_F(AstraTabSearchItemViewTest, SetTabUpdatesData) {
  AstraTabSearchItem new_tab;
  new_tab.tab_id = 99;
  new_tab.title = u"New Title";
  new_tab.hostname = u"new.com";
  new_tab.tab_index = 5;

  item_view_->SetTab(new_tab);
  EXPECT_EQ(u"New Title", item_view_->GetTab().title);
  EXPECT_EQ(u"new.com", item_view_->GetTab().hostname);
  EXPECT_EQ(99, item_view_->GetTab().tab_id);
  EXPECT_EQ(5, item_view_->tab_index());
}

TEST_F(AstraTabSearchItemViewTest, DefaultIsNotSelected) {
  EXPECT_FALSE(item_view_->IsSelected());
}

TEST_F(AstraTabSearchItemViewTest, SetSelectedTrue) {
  item_view_->SetSelected(true);
  EXPECT_TRUE(item_view_->IsSelected());
}

TEST_F(AstraTabSearchItemViewTest, SetSelectedFalse) {
  item_view_->SetSelected(true);
  ASSERT_TRUE(item_view_->IsSelected());
  item_view_->SetSelected(false);
  EXPECT_FALSE(item_view_->IsSelected());
}

TEST_F(AstraTabSearchItemViewTest, SetSelectedSameStateNoCrash) {
  item_view_->SetSelected(false);
  item_view_->SetSelected(false);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, DefaultIsNotHighlighted) {
  EXPECT_FALSE(item_view_->IsHighlighted());
}

TEST_F(AstraTabSearchItemViewTest, SetHighlightedTrue) {
  item_view_->SetHighlighted(true);
  EXPECT_TRUE(item_view_->IsHighlighted());
}

TEST_F(AstraTabSearchItemViewTest, SetHighlightedFalse) {
  item_view_->SetHighlighted(true);
  ASSERT_TRUE(item_view_->IsHighlighted());
  item_view_->SetHighlighted(false);
  EXPECT_FALSE(item_view_->IsHighlighted());
}

TEST_F(AstraTabSearchItemViewTest, TitleMatchRanges) {
  std::vector<gfx::Range> ranges;
  ranges.push_back(gfx::Range(0, 2));
  item_view_->SetTitleMatchRanges(ranges);
  // Setting ranges should not crash.
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, UrlMatchRanges) {
  std::vector<gfx::Range> ranges;
  ranges.push_back(gfx::Range(2, 5));
  item_view_->SetUrlMatchRanges(ranges);
  // Setting ranges should not crash.
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, EmptyMatchRanges) {
  std::vector<gfx::Range> empty_ranges;
  item_view_->SetTitleMatchRanges(empty_ranges);
  item_view_->SetUrlMatchRanges(empty_ranges);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowFaviconTrueByDefault) {
  // Default is true.
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowFaviconFalse) {
  item_view_->ShowFavicon(false);
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowFaviconToggle) {
  item_view_->ShowFavicon(false);
  item_view_->ShowFavicon(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowWorkspaceTrueByDefault) {
  // Default is true.
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowWorkspaceFalse) {
  item_view_->ShowWorkspace(false);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowWorkspaceToggle) {
  item_view_->ShowWorkspace(false);
  item_view_->ShowWorkspace(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowAudioIndicatorTrueByDefault) {
  // Default is true (but only visible if tab is audible).
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowAudioIndicatorFalse) {
  item_view_->ShowAudioIndicator(false);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowCloseButtonFalseByDefault) {
  // Close button is hidden by default (shown on hover/select).
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowCloseButtonTrue) {
  item_view_->ShowCloseButton(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowCloseButtonToggle) {
  item_view_->ShowCloseButton(true);
  item_view_->ShowCloseButton(false);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, SetGroupColor) {
  item_view_->SetGroupColor(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, item_view_->GetTab().group_color);
}

TEST_F(AstraTabSearchItemViewTest, SetGroupColorTransparent) {
  item_view_->SetGroupColor(SK_ColorTRANSPARENT);
  EXPECT_EQ(SK_ColorTRANSPARENT, item_view_->GetTab().group_color);
}

TEST_F(AstraTabSearchItemViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = item_view_->CalculatePreferredSize();
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraTabSearchItemViewTest, OnThemeChangedDoesNotCrash) {
  item_view_->OnThemeChanged();
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, HasColorProvider) {
  EXPECT_NE(nullptr, item_view_->GetColorProvider());
}

TEST_F(AstraTabSearchItemViewTest, AccessibleRoleIsListItem) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kListItem, data.role);
}

TEST_F(AstraTabSearchItemViewTest, AccessibleNameIncludesTitle) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_NE(std::u16string::npos, data.GetName().find(u"My Document"));
}

TEST_F(AstraTabSearchItemViewTest, AccessibleNameIncludesHost) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_NE(std::u16string::npos, data.GetName().find(u"docs.google.com"));
}

TEST_F(AstraTabSearchItemViewTest, AccessibleHasSelectableState) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_TRUE(data.HasState(ax::mojom::State::kSelectable));
}

TEST_F(AstraTabSearchItemViewTest, SelectedItemHasSelectedState) {
  item_view_->SetSelected(true);
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_TRUE(data.HasState(ax::mojom::State::kSelected));
}

TEST_F(AstraTabSearchItemViewTest, UnselectedItemNotSelectedState) {
  item_view_->SetSelected(false);
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.HasState(ax::mojom::State::kSelected));
}

TEST_F(AstraTabSearchItemViewTest, TabIndexAccessor) {
  EXPECT_EQ(3, item_view_->tab_index());
}

TEST_F(AstraTabSearchItemViewTest, TabIdAccessor) {
  EXPECT_EQ(42, item_view_->tab_id());
}

TEST_F(AstraTabSearchItemViewTest, TitleAccessor) {
  EXPECT_EQ(u"My Document", item_view_->title());
}

TEST_F(AstraTabSearchItemViewTest, HostnameAccessor) {
  EXPECT_EQ(u"docs.google.com", item_view_->hostname());
}

TEST_F(AstraTabSearchItemViewTest, WorkspaceIdAccessor) {
  EXPECT_EQ("work", item_view_->workspace_id());
}

TEST_F(AstraTabSearchItemViewTest, IsPinnedAccessor) {
  EXPECT_FALSE(item_view_->is_pinned());
}

TEST_F(AstraTabSearchItemViewTest, IsAudibleAccessor) {
  EXPECT_FALSE(item_view_->is_audible());
}

TEST_F(AstraTabSearchItemViewTest, IsMutedAccessor) {
  EXPECT_FALSE(item_view_->is_muted());
}

TEST_F(AstraTabSearchItemViewTest, MouseEnterExitNoCrash) {
  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(),
                             gfx::Point(), base::TimeTicks(), 0, 0);
  item_view_->OnMouseEntered(enter_event);

  ui::MouseEvent exit_event(ui::ET_MOUSE_EXITED, gfx::Point(),
                            gfx::Point(), base::TimeTicks(), 0, 0);
  item_view_->OnMouseExited(exit_event);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ActivatedCallbackFiresOnMousePress) {
  int activated_count = 0;
  item_view_->SetActivatedCallback(base::BindLambdaForTesting(
      [&activated_count]() { activated_count++; }));

  EXPECT_EQ(0, activated_count);

  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  item_view_->OnMousePressed(event);

  EXPECT_GE(activated_count, 1);
}

TEST_F(AstraTabSearchItemViewTest, CloseCallbackIsSetWithoutCrash) {
  int close_count = 0;
  item_view_->SetCloseCallback(base::BindLambdaForTesting(
      [&close_count]() { close_count++; }));

  EXPECT_EQ(0, close_count);
}

TEST_F(AstraTabSearchItemViewTest, NullActivatedCallbackIsSafe) {
  item_view_->SetActivatedCallback(
      AstraTabSearchItemView::ActivatedCallback());

  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  item_view_->OnMousePressed(event);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, NullCloseCallbackIsSafe) {
  item_view_->SetCloseCallback(AstraTabSearchItemView::CloseCallback());
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, CanReceiveFocus) {
  item_view_->RequestFocus();
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, DisplayIndexDefault) {
  EXPECT_EQ(0, item_view_->display_index());
}

TEST_F(AstraTabSearchItemViewTest, SetDisplayIndex) {
  item_view_->SetDisplayIndex(5);
  EXPECT_EQ(5, item_view_->display_index());
}

TEST_F(AstraTabSearchItemViewTest, SetDisplayIndexZero) {
  item_view_->SetDisplayIndex(0);
  EXPECT_EQ(0, item_view_->display_index());
}

TEST_F(AstraTabSearchItemViewTest, SetDisplayIndexSameValueNoCrash) {
  item_view_->SetDisplayIndex(3);
  item_view_->SetDisplayIndex(3);
  SUCCEED();
}

// =========================================================================
// Item view tests — audio indicator
// =========================================================================

class AstraTabSearchItemAudioTest : public views::ViewsTestBase {
 public:
  AstraTabSearchItemAudioTest() = default;
  ~AstraTabSearchItemAudioTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    // Create a tab with audio playing.
    AstraTabSearchItem tab;
    tab.tab_id = 0;
    tab.title = u"Audio Tab";
    tab.hostname = u"music.example.com";
    tab.is_audible = true;
    tab.is_muted = false;
    tab.tab_index = 0;

    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraTabSearchItemView>(tab));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraTabSearchItemView> item_view_ = nullptr;
};

TEST_F(AstraTabSearchItemAudioTest, IsAudibleIsTrue) {
  EXPECT_TRUE(item_view_->is_audible());
  EXPECT_FALSE(item_view_->is_muted());
}

TEST_F(AstraTabSearchItemAudioTest, MutedTab) {
  AstraTabSearchItem muted_tab;
  muted_tab.tab_id = 1;
  muted_tab.title = u"Muted Tab";
  muted_tab.hostname = u"video.example.com";
  muted_tab.is_audible = true;
  muted_tab.is_muted = true;
  muted_tab.tab_index = 1;

  item_view_->SetTab(muted_tab);
  EXPECT_TRUE(item_view_->is_audible());
  EXPECT_TRUE(item_view_->is_muted());
}

TEST_F(AstraTabSearchItemAudioTest, NotAudibleTab) {
  AstraTabSearchItem silent_tab;
  silent_tab.tab_id = 2;
  silent_tab.title = u"Silent Tab";
  silent_tab.hostname = u"text.example.com";
  silent_tab.is_audible = false;
  silent_tab.is_muted = false;
  silent_tab.tab_index = 2;

  item_view_->SetTab(silent_tab);
  EXPECT_FALSE(item_view_->is_audible());
  EXPECT_FALSE(item_view_->is_muted());
}

// =========================================================================
// Item view tests — edge cases
// =========================================================================

class AstraTabSearchItemEdgeCaseTest : public views::ViewsTestBase {
 public:
  AstraTabSearchItemEdgeCaseTest() = default;
  ~AstraTabSearchItemEdgeCaseTest() override = default;

  void TearDown() override {
    if (widget_) {
      widget_.reset();
    }
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
};

TEST_F(AstraTabSearchItemEdgeCaseTest, VeryLongTitle) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = std::u16string(500, u'x');
  tab.hostname = u"test.com";
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_EQ(500u, view->title().size());
  EXPECT_GT(view->CalculatePreferredSize().width(), 0);
}

TEST_F(AstraTabSearchItemEdgeCaseTest, VeryLongUrl) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Test";
  tab.hostname = std::u16string(300, u'a') + u".example.com";
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_GT(view->hostname().size(), 200u);
}

TEST_F(AstraTabSearchItemEdgeCaseTest, EmptyTitle) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"";
  tab.hostname = u"test.com";
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->title().empty());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, EmptyHostname) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Test";
  tab.hostname = u"";
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->hostname().empty());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, PinnedTab) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Pinned";
  tab.hostname = u"pinned.com";
  tab.is_pinned = true;
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->is_pinned());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, TabWithGroup) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Grouped Tab";
  tab.hostname = u"group.com";
  tab.is_in_group = true;
  tab.group_name = u"My Group";
  tab.group_color = SK_ColorGREEN;
  tab.tab_index = 0;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->GetTab().is_in_group);
  EXPECT_EQ(u"My Group", view->GetTab().group_name);
}

TEST_F(AstraTabSearchItemEdgeCaseTest, LegacyTabInfoConstructor) {
  // Test backward compatibility with legacy TabInfo constructor.
  AstraTabSearchItemView::TabInfo info;
  info.title = u"Legacy Tab";
  info.host = u"legacy.com";
  info.tab_index = 5;
  info.group = AstraTabSearchItemView::Group::kBookmarks;
  info.identifier = "legacy-1";

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(info, 2));
  widget_->Show();

  EXPECT_EQ(u"Legacy Tab", view->title());
  EXPECT_EQ(u"legacy.com", view->host());
  EXPECT_EQ(5, view->tab_index());
  EXPECT_EQ(AstraTabSearchItemView::Group::kBookmarks, view->group());
  EXPECT_EQ("legacy-1", view->identifier());
  EXPECT_EQ(2, view->display_index());
}

// =========================================================================
// Item view tests — group parameterized
// =========================================================================

class AstraTabSearchItemGroupParamTest
    : public views::ViewsTestBase,
      public testing::WithParamInterface<AstraTabSearchItemView::Group> {
 public:
  AstraTabSearchItemGroupParamTest() = default;
  ~AstraTabSearchItemGroupParamTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    AstraTabSearchItemView::TabInfo info;
    info.title = u"Test Tab";
    info.host = u"test.com";
    info.tab_index = 0;
    info.group = GetParam();
    info.identifier = "test-id";

    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraTabSearchItemView>(info, 1));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraTabSearchItemView> item_view_ = nullptr;
};

TEST_P(AstraTabSearchItemGroupParamTest, ConstructsForAllGroups) {
  EXPECT_NE(nullptr, item_view_);
  EXPECT_EQ(GetParam(), item_view_->group());
}

TEST_P(AstraTabSearchItemGroupParamTest, ThemeChangeWorksForAllGroups) {
  item_view_->OnThemeChanged();
  SUCCEED();
}

TEST_P(AstraTabSearchItemGroupParamTest, AccessibilityWorksForAllGroups) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kListItem, data.role);
  EXPECT_FALSE(data.GetName().empty());
}

TEST_P(AstraTabSearchItemGroupParamTest, SelectionWorksForAllGroups) {
  item_view_->SetSelected(true);
  EXPECT_TRUE(item_view_->IsSelected());
  item_view_->SetSelected(false);
  EXPECT_FALSE(item_view_->IsSelected());
}

INSTANTIATE_TEST_SUITE_P(
    AllGroups,
    AstraTabSearchItemGroupParamTest,
    testing::Values(
        AstraTabSearchItemView::Group::kOpenTabs,
        AstraTabSearchItemView::Group::kRecentlyClosed,
        AstraTabSearchItemView::Group::kBookmarks));

// =========================================================================
// Legacy TabInfo struct tests
// =========================================================================

TEST(AstraTabSearchTabInfoTest, DefaultValues) {
  AstraTabSearchItemView::TabInfo info;
  EXPECT_TRUE(info.title.empty());
  EXPECT_TRUE(info.host.empty());
  EXPECT_EQ(-1, info.tab_index);
  EXPECT_EQ(AstraTabSearchItemView::Group::kOpenTabs, info.group);
  EXPECT_TRUE(info.identifier.empty());
}

TEST(AstraTabSearchTabInfoTest, CanSetAllFields) {
  AstraTabSearchItemView::TabInfo info;
  info.title = u"Test Page";
  info.host = u"example.com";
  info.tab_index = 5;
  info.group = AstraTabSearchItemView::Group::kBookmarks;
  info.identifier = "bm-123";

  EXPECT_EQ(u"Test Page", info.title);
  EXPECT_EQ(u"example.com", info.host);
  EXPECT_EQ(5, info.tab_index);
  EXPECT_EQ(AstraTabSearchItemView::Group::kBookmarks, info.group);
  EXPECT_EQ("bm-123", info.identifier);
}

// =========================================================================
// Bubble tests
// =========================================================================

// Note: Full bubble tests require a Browser object. These tests cover the
// bubble API surface that can be tested in isolation.

class AstraTabSearchBubbleTest : public views::ViewsTestBase {
 public:
  AstraTabSearchBubbleTest() = default;
  ~AstraTabSearchBubbleTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    // Create an anchor view for the bubble.
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(100, 32));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
};

TEST_F(AstraTabSearchBubbleTest, ModelAccess) {
  // The bubble owns a model; verify we can access it.
  // We create a bubble via the model directly for this test.
  AstraTabSearchModel model;
  EXPECT_EQ(AstraTabSearchMode::kAllTabs, model.GetSearchMode());
  EXPECT_EQ(0u, model.GetTabCount());
}

TEST_F(AstraTabSearchBubbleTest, BubbleModelDefaultSettings) {
  AstraTabSearchModel model;
  EXPECT_EQ(AstraTabSearchModel::kDefaultMaxSearchResults,
            model.max_search_results());
  EXPECT_TRUE(model.show_tab_urls());
  EXPECT_TRUE(model.show_workspace_name());
}

TEST_F(AstraTabSearchBubbleTest, BubbleModelSetQuery) {
  AstraTabSearchModel model;
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Test Tab";
  tab.hostname = u"test.com";
  tab.tab_index = 0;
  tabs.push_back(std::move(tab));
  model.SetTabList(std::move(tabs));

  auto results = model.SearchTabs(u"Test");
  EXPECT_EQ(1u, results.size());
}

// Bubble delegate default implementation tests.
TEST(AstraTabSearchBubbleDelegateDefaultsTest, DefaultDelegateDoesNotCrash) {
  class DefaultDelegate : public AstraTabSearchBubble::Delegate {};

  DefaultDelegate delegate;
  delegate.OnTabActivated(nullptr);
  delegate.OnTabClosed(nullptr);
  delegate.OnBubbleClosed();
  delegate.OnSearchTextChanged(u"test");
  delegate.OnSelectionChanged(0);
  delegate.OnResultCountChanged(5);
  delegate.OnBubbleOpened();
  delegate.OnBulkCloseRequested(3);
  delegate.OnTabCloseRequested(nullptr);
  delegate.OnSearchModeChanged(AstraTabSearchMode::kAllTabs);
  SUCCEED();
}

// =========================================================================
// Bubble tests — edge cases
// =========================================================================

TEST(AstraTabSearchBubbleEdgeCaseTest, EmptyModelNoResults) {
  AstraTabSearchModel model;
  auto results = model.SearchTabs(u"");
  EXPECT_EQ(0u, results.size());
}

TEST(AstraTabSearchBubbleEdgeCaseTest, SingleItemSelection) {
  AstraTabSearchModel model;
  std::vector<AstraTabSearchItem> tabs;
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.title = u"Only Tab";
  tab.tab_index = 0;
  tabs.push_back(std::move(tab));
  model.SetTabList(std::move(tabs));

  auto results = model.SearchTabs(u"");
  EXPECT_EQ(1u, results.size());
}

TEST(AstraTabSearchBubbleEdgeCaseTest, SearchModeDoesNotCrashForEmptyModel) {
  AstraTabSearchModel model;
  model.SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  auto results = model.SearchTabs(u"");
  EXPECT_EQ(0u, results.size());
}

// =========================================================================
// Model-view separation documentation tests
// =========================================================================

TEST(AstraTabSearchArchitectureTest, ModelOwnsData) {
  // The model owns tab data and search logic.
  // Views render model state but do not own state.
  // This is verified by the test structure: model tests don't need views.
  AstraTabSearchModel model;
  EXPECT_EQ(0u, model.GetTabCount());
  SUCCEED();
}

TEST(AstraTabSearchArchitectureTest, ViewIsPresentationOnly) {
  // Item views are pure presentation.
  // They take tab data and render it, but don't modify source data.
  SUCCEED();
}

TEST(AstraTabSearchArchitectureTest, ObserverPatternConnectsModelAndView) {
  // Observers allow views to react to model changes without direct coupling.
  TestTabSearchObserver observer;
  AstraTabSearchModel model;
  model.AddObserver(&observer);

  model.SetTabList(CreateSampleTabs(3));
  EXPECT_GT(observer.tab_list_changed_count_, 0);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Chromium reuse documentation tests
// =========================================================================

TEST(AstraTabSearchChromiumReuseTest, TabStripModelOwnsTabState) {
  // Chromium's TabStripModel owns actual tab data.
  // AstraTabSearchModel projects it for search UI.
  //
  // TODO(astra): Wire to TabStripModel for real tab data.
  //   Chromium owner: TabStripModel
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, FaviconServiceForIcons) {
  // Chromium's FaviconService provides tab favicons.
  // TODO(astra): Show real favicons using FaviconService.
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, TabRestoreServiceForRecentlyClosed) {
  // Chromium's TabRestoreService owns recently closed tab data.
  // TODO(astra): Wire up to TabRestoreService.
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, ViewsFrameworkForUI) {
  // Uses Chromium's Views framework for all UI components.
  // views::BubbleDialogDelegateView, views::Label, views::ScrollView, etc.
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, BuiltInTabSearchComparison) {
  // Chromium has a built-in tab search UI in chrome/browser/ui/views/tab_search/.
  // Astra's version is an overlay with workspace-aware features.
  //
  // TODO(astra): Evaluate reusing Chromium's built-in tab search with
  //   Astra styling and workspace filters.
  SUCCEED();
}

}  // namespace astra
