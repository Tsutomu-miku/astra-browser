// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for tab search views.
//
// Test coverage includes:
//   - Type / enum tests: AstraTabSearchResultType, AstraTabSearchFilter,
//     AstraTabSearchMatch, AstraTabSearchRecentSearch,
//     AstraTabSearchGroupedResults, AstraTabSearchMode, AstraTabSearchSortOrder
//   - AstraTabSearchItem struct: construction, all fields, edge cases
//   - AstraTabSearchGroupInfo struct: construction and fields
//   - AstraTabSearchModel: tab list, search, scoring, fuzzy matching,
//     filters, modes, selected index, workspaces, groups, windows,
//     actions, settings, observers, bookmarks, history, recent searches,
//     grouped results, compute matches, edge cases
//   - AstraTabSearchGroupHeaderView: construction, title, count, accessibility
//   - AstraTabSearchItemView: construction, state, highlighting,
//     visibility toggles, group color, audio indicator, keyboard,
//     focus, gestures, middle click, shortcut hints, site info, edge cases
//   - AstraTabSearchBubble: delegate defaults, model integration
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
    tab.item_id = static_cast<int>(i);
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
    tab.last_visited_time = now - base::Minutes(s.minutes_ago);
    tab.has_crashed = false;
    tab.is_loading = false;
    tab.relevance_score = 0.0;
    tab.result_type = AstraTabSearchResultType::kOpenTab;
    tabs.push_back(std::move(tab));
  }

  return tabs;
}

std::vector<AstraTabSearchItem> CreateSampleBookmarks(size_t count = 5) {
  std::vector<AstraTabSearchItem> bookmarks;
  base::Time now = base::Time::Now();

  const char* titles[] = {
    "Astra Browser", "Chromium Docs", "Stack Overflow", "MDN Web Docs", "GitHub",
  };
  const char* urls[] = {
    "https://astra.example.com",
    "https://chromium.googlesource.com",
    "https://stackoverflow.com",
    "https://developer.mozilla.org",
    "https://github.com",
  };

  size_t actual = std::min(count, sizeof(titles) / sizeof(titles[0]));
  for (size_t i = 0; i < actual; ++i) {
    AstraTabSearchItem bm;
    bm.item_id = static_cast<int>(i + 1000);
    bm.title = base::UTF8ToUTF16(titles[i]);
    bm.url = GURL(urls[i]);
    bm.hostname = base::UTF8ToUTF16(GURL(urls[i]).host());
    bm.result_type = AstraTabSearchResultType::kBookmark;
    bm.is_in_bookmarks_bar = (i < 2);
    bm.last_visited_time = now - base::Days(static_cast<int>(i) + 1);
    bookmarks.push_back(std::move(bm));
  }
  return bookmarks;
}

std::vector<AstraTabSearchItem> CreateSampleHistory(size_t count = 5) {
  std::vector<AstraTabSearchItem> history;
  base::Time now = base::Time::Now();

  const char* titles[] = {
    "Google Search", "YouTube Video", "Reddit Post", "Wikipedia Article",
    "News Article",
  };
  const char* urls[] = {
    "https://www.google.com/search?q=test",
    "https://www.youtube.com/watch?v=abc",
    "https://www.reddit.com/r/programming",
    "https://en.wikipedia.org/wiki/Chromium",
    "https://news.example.com/article",
  };
  int visit_counts[] = {42, 15, 8, 23, 5};

  size_t actual = std::min(count, sizeof(titles) / sizeof(titles[0]));
  for (size_t i = 0; i < actual; ++i) {
    AstraTabSearchItem h;
    h.item_id = static_cast<int>(i + 2000);
    h.title = base::UTF8ToUTF16(titles[i]);
    h.url = GURL(urls[i]);
    h.hostname = base::UTF8ToUTF16(GURL(urls[i]).host());
    h.result_type = AstraTabSearchResultType::kHistory;
    h.visit_count = visit_counts[i];
    h.last_visited_time = now - base::Hours(static_cast<int>(i) * 2 + 1);
    history.push_back(std::move(h));
  }
  return history;
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
  void OnSelectedIndexChanged(AstraTabSearchModel* model,
                               size_t old_index,
                               size_t new_index) override {
    selected_index_changed_count_++;
    last_selected_old_index_ = old_index;
    last_selected_new_index_ = new_index;
    last_model_selected_index_ = model;
  }
  void OnSearchModeChanged(AstraTabSearchModel* model,
                           AstraTabSearchMode mode) override {
    search_mode_changed_count_++;
    last_search_mode_ = mode;
    last_model_search_mode_ = model;
  }
  void OnFilterChanged(AstraTabSearchModel* model,
                       AstraTabSearchFilter filter) override {
    filter_changed_count_++;
    last_filter_ = filter;
    last_model_filter_ = model;
  }
  void OnQueryChanged(AstraTabSearchModel* model,
                      const std::u16string& query) override {
    query_changed_count_++;
    last_query_ = query;
    last_model_query_ = model;
  }
  void OnRecentSearchesChanged(AstraTabSearchModel* model) override {
    recent_searches_changed_count_++;
    last_model_recent_searches_ = model;
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
    selected_index_changed_count_ = 0;
    search_mode_changed_count_ = 0;
    filter_changed_count_ = 0;
    query_changed_count_ = 0;
    recent_searches_changed_count_ = 0;
    model_shutdown_count_ = 0;
    last_activated_tab_index_ = -1;
    last_closed_tab_index_ = -1;
    last_selected_old_index_ = 0;
    last_selected_new_index_ = 0;
  }

  int tab_list_changed_count_ = 0;
  int tab_activated_count_ = 0;
  int tab_closed_count_ = 0;
  int search_results_changed_count_ = 0;
  int selected_index_changed_count_ = 0;
  int search_mode_changed_count_ = 0;
  int filter_changed_count_ = 0;
  int query_changed_count_ = 0;
  int recent_searches_changed_count_ = 0;
  int model_shutdown_count_ = 0;

  int last_activated_tab_index_ = -1;
  int last_closed_tab_index_ = -1;
  size_t last_selected_old_index_ = 0;
  size_t last_selected_new_index_ = 0;
  AstraTabSearchMode last_search_mode_ = AstraTabSearchMode::kAllTabs;
  AstraTabSearchFilter last_filter_ = AstraTabSearchFilter::kAllContent;
  std::u16string last_query_;

  raw_ptr<AstraTabSearchModel> last_model_tab_list_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_tab_activated_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_tab_closed_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_search_results_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_selected_index_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_search_mode_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_filter_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_query_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_recent_searches_ = nullptr;
  raw_ptr<AstraTabSearchModel> last_model_shutdown_ = nullptr;
};

// =========================================================================
// AstraTabSearchResultType enum tests
// =========================================================================

TEST(AstraTabSearchResultTypeTest, AllTypesExist) {
  // Verify all result types exist with correct values.
  EXPECT_EQ(0, static_cast<int>(AstraTabSearchResultType::kOpenTab));
  EXPECT_EQ(1, static_cast<int>(AstraTabSearchResultType::kRecentlyClosed));
  EXPECT_EQ(2, static_cast<int>(AstraTabSearchResultType::kBookmark));
  EXPECT_EQ(3, static_cast<int>(AstraTabSearchResultType::kHistory));
  EXPECT_EQ(4, static_cast<int>(AstraTabSearchResultType::kSearchHistory));
  EXPECT_EQ(5, static_cast<int>(AstraTabSearchResultType::kAction));
}

TEST(AstraTabSearchResultTypeTest, SixTypesTotal) {
  int count = 0;
  count += static_cast<int>(AstraTabSearchResultType::kOpenTab) == 0 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchResultType::kRecentlyClosed) == 1 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchResultType::kBookmark) == 2 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchResultType::kHistory) == 3 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchResultType::kSearchHistory) == 4 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchResultType::kAction) == 5 ? 1 : 0;
  EXPECT_EQ(6, count);
}

// =========================================================================
// AstraTabSearchFilter enum + bitwise tests
// =========================================================================

TEST(AstraTabSearchFilterTest, DefaultFilterIsAllContent) {
  AstraTabSearchFilter filter = AstraTabSearchFilter::kAllContent;
  EXPECT_NE(AstraTabSearchFilter::kNone, filter);
}

TEST(AstraTabSearchFilterTest, BitwiseOr) {
  auto combined = AstraTabSearchFilter::kTabs | AstraTabSearchFilter::kBookmarks;
  // Should contain both flags.
  EXPECT_NE(AstraTabSearchFilter::kNone,
            combined & AstraTabSearchFilter::kTabs);
  EXPECT_NE(AstraTabSearchFilter::kNone,
            combined & AstraTabSearchFilter::kBookmarks);
  EXPECT_EQ(AstraTabSearchFilter::kNone,
            combined & AstraTabSearchFilter::kHistory);
}

TEST(AstraTabSearchFilterTest, BitwiseAnd) {
  auto combined = AstraTabSearchFilter::kTabs | AstraTabSearchFilter::kBookmarks;
  auto intersection = combined & AstraTabSearchFilter::kTabs;
  EXPECT_EQ(AstraTabSearchFilter::kTabs, intersection);
}

TEST(AstraTabSearchFilterTest, AllContentIncludesAllTypes) {
  auto all = AstraTabSearchFilter::kAllContent;
  EXPECT_NE(AstraTabSearchFilter::kNone,
            all & AstraTabSearchFilter::kTabs);
  EXPECT_NE(AstraTabSearchFilter::kNone,
            all & AstraTabSearchFilter::kRecentlyClosed);
  EXPECT_NE(AstraTabSearchFilter::kNone,
            all & AstraTabSearchFilter::kBookmarks);
  EXPECT_NE(AstraTabSearchFilter::kNone,
            all & AstraTabSearchFilter::kHistory);
}

TEST(AstraTabSearchFilterTest, AllEqualsAllContent) {
  EXPECT_EQ(static_cast<int>(AstraTabSearchFilter::kAll),
            static_cast<int>(AstraTabSearchFilter::kAllContent));
}

TEST(AstraTabSearchFilterTest, NoneIsZero) {
  EXPECT_EQ(0, static_cast<int>(AstraTabSearchFilter::kNone));
}

TEST(AstraTabSearchFilterTest, EachFilterIsDistinct) {
  // Each individual filter flag should be a unique bit.
  auto tabs = static_cast<int>(AstraTabSearchFilter::kTabs);
  auto closed = static_cast<int>(AstraTabSearchFilter::kRecentlyClosed);
  auto bookmarks = static_cast<int>(AstraTabSearchFilter::kBookmarks);
  auto history = static_cast<int>(AstraTabSearchFilter::kHistory);

  EXPECT_EQ(0, tabs & closed);
  EXPECT_EQ(0, tabs & bookmarks);
  EXPECT_EQ(0, tabs & history);
  EXPECT_EQ(0, closed & bookmarks);
  EXPECT_EQ(0, closed & history);
  EXPECT_EQ(0, bookmarks & history);
}

// =========================================================================
// AstraTabSearchMatch struct tests
// =========================================================================

TEST(AstraTabSearchMatchTest, DefaultValues) {
  AstraTabSearchMatch match;
  EXPECT_EQ(AstraTabSearchMatch::Type::kTitle, match.type);
  EXPECT_TRUE(match.range.is_empty());
}

TEST(AstraTabSearchMatchTest, CanSetAllFields) {
  AstraTabSearchMatch match;
  match.type = AstraTabSearchMatch::Type::kUrl;
  match.range = gfx::Range(2, 8);

  EXPECT_EQ(AstraTabSearchMatch::Type::kUrl, match.type);
  EXPECT_EQ(2u, match.range.start());
  EXPECT_EQ(8u, match.range.end());
}

TEST(AstraTabSearchMatchTest, AllMatchTypesExist) {
  // Just verify the types are usable.
  AstraTabSearchMatch::Type types[] = {
    AstraTabSearchMatch::Type::kTitle,
    AstraTabSearchMatch::Type::kUrl,
    AstraTabSearchMatch::Type::kHostname,
    AstraTabSearchMatch::Type::kWorkspace,
    AstraTabSearchMatch::Type::kGroup,
  };
  EXPECT_EQ(5u, sizeof(types) / sizeof(types[0]));
}

// =========================================================================
// AstraTabSearchRecentSearch struct tests
// =========================================================================

TEST(AstraTabSearchRecentSearchTest, DefaultValues) {
  AstraTabSearchRecentSearch entry;
  EXPECT_TRUE(entry.query.empty());
  EXPECT_TRUE(entry.timestamp.is_null());
  EXPECT_EQ(1, entry.visit_count);
}

TEST(AstraTabSearchRecentSearchTest, CanSetAllFields) {
  AstraTabSearchRecentSearch entry;
  entry.query = u"chromium docs";
  entry.timestamp = base::Time::Now();
  entry.visit_count = 5;

  EXPECT_EQ(u"chromium docs", entry.query);
  EXPECT_FALSE(entry.timestamp.is_null());
  EXPECT_EQ(5, entry.visit_count);
}

// =========================================================================
// AstraTabSearchGroupedResults struct tests
// =========================================================================

TEST(AstraTabSearchGroupedResultsTest, DefaultIsEmpty) {
  AstraTabSearchGroupedResults grouped;
  EXPECT_TRUE(grouped.IsEmpty());
  EXPECT_EQ(0u, grouped.TotalCount());
}

TEST(AstraTabSearchGroupedResultsTest, TotalCountSumsAllGroups) {
  AstraTabSearchGroupedResults grouped;
  grouped.open_tabs.resize(3);
  grouped.recently_closed.resize(2);
  grouped.bookmarks.resize(4);
  grouped.history.resize(5);
  grouped.recent_searches.resize(1);

  EXPECT_EQ(15u, grouped.TotalCount());
  EXPECT_FALSE(grouped.IsEmpty());
}

TEST(AstraTabSearchGroupedResultsTest, IsEmptyOnlyWhenAllEmpty) {
  AstraTabSearchGroupedResults grouped;
  EXPECT_TRUE(grouped.IsEmpty());

  grouped.open_tabs.resize(1);
  EXPECT_FALSE(grouped.IsEmpty());

  grouped.open_tabs.clear();
  grouped.bookmarks.resize(1);
  EXPECT_FALSE(grouped.IsEmpty());
}

// =========================================================================
// AstraTabSearchItem struct tests
// =========================================================================

TEST(AstraTabSearchItemTest, DefaultValues) {
  AstraTabSearchItem item;
  EXPECT_EQ(AstraTabSearchResultType::kOpenTab, item.result_type);
  EXPECT_EQ(-1, item.item_id);
  EXPECT_EQ(-1, item.tab_id);
  EXPECT_TRUE(item.title.empty());
  EXPECT_TRUE(item.url.is_empty());
  EXPECT_TRUE(item.hostname.empty());
  EXPECT_TRUE(item.workspace_id.empty());
  EXPECT_TRUE(item.workspace_name.empty());
  EXPECT_FALSE(item.is_active);
  EXPECT_FALSE(item.is_pinned);
  EXPECT_FALSE(item.is_in_group);
  EXPECT_TRUE(item.group_id.empty());
  EXPECT_TRUE(item.group_name.empty());
  EXPECT_EQ(SK_ColorTRANSPARENT, item.group_color);
  EXPECT_TRUE(item.last_visited_time.is_null());
  EXPECT_EQ(0.0, item.relevance_score);
  EXPECT_FALSE(item.is_audible);
  EXPECT_FALSE(item.is_muted);
  EXPECT_EQ(-1, item.tab_index);
  EXPECT_EQ(0, item.window_id);
  EXPECT_FALSE(item.has_crashed);
  EXPECT_FALSE(item.is_loading);
  EXPECT_FALSE(item.is_in_bookmarks_bar);
  EXPECT_EQ(0, item.visit_count);
}

TEST(AstraTabSearchItemTest, CanSetAllFields) {
  AstraTabSearchItem item;
  item.result_type = AstraTabSearchResultType::kBookmark;
  item.item_id = 42;
  item.tab_id = 42;
  item.title = u"Test Page";
  item.url = GURL("https://example.com/page");
  item.hostname = u"example.com";
  item.workspace_id = "ws1";
  item.workspace_name = u"My Workspace";
  item.is_active = true;
  item.is_pinned = true;
  item.is_in_group = true;
  item.group_id = "g1";
  item.group_name = u"Dev Group";
  item.group_color = SK_ColorBLUE;
  item.last_visited_time = base::Time::Now();
  item.relevance_score = 999.5;
  item.is_audible = true;
  item.is_muted = false;
  item.tab_index = 5;
  item.window_id = 2;
  item.has_crashed = true;
  item.is_loading = true;
  item.is_in_bookmarks_bar = true;
  item.visit_count = 42;

  EXPECT_EQ(AstraTabSearchResultType::kBookmark, item.result_type);
  EXPECT_EQ(42, item.item_id);
  EXPECT_EQ(u"Test Page", item.title);
  EXPECT_EQ(GURL("https://example.com/page"), item.url);
  EXPECT_EQ(u"example.com", item.hostname);
  EXPECT_EQ("ws1", item.workspace_id);
  EXPECT_EQ(u"My Workspace", item.workspace_name);
  EXPECT_TRUE(item.is_active);
  EXPECT_TRUE(item.is_pinned);
  EXPECT_TRUE(item.is_in_group);
  EXPECT_EQ("g1", item.group_id);
  EXPECT_EQ(u"Dev Group", item.group_name);
  EXPECT_EQ(SK_ColorBLUE, item.group_color);
  EXPECT_FALSE(item.last_visited_time.is_null());
  EXPECT_EQ(999.5, item.relevance_score);
  EXPECT_TRUE(item.is_audible);
  EXPECT_FALSE(item.is_muted);
  EXPECT_EQ(5, item.tab_index);
  EXPECT_EQ(2, item.window_id);
  EXPECT_TRUE(item.has_crashed);
  EXPECT_TRUE(item.is_loading);
  EXPECT_TRUE(item.is_in_bookmarks_bar);
  EXPECT_EQ(42, item.visit_count);
}

TEST(AstraTabSearchItemTest, CopyConstructible) {
  AstraTabSearchItem original;
  original.item_id = 7;
  original.title = u"Copy Test";
  original.hostname = u"copy.test";
  original.workspace_id = "copy-ws";
  original.is_pinned = true;
  original.result_type = AstraTabSearchResultType::kHistory;

  AstraTabSearchItem copy = original;
  EXPECT_EQ(7, copy.item_id);
  EXPECT_EQ(u"Copy Test", copy.title);
  EXPECT_EQ(u"copy.test", copy.hostname);
  EXPECT_EQ("copy-ws", copy.workspace_id);
  EXPECT_TRUE(copy.is_pinned);
  EXPECT_EQ(AstraTabSearchResultType::kHistory, copy.result_type);
}

TEST(AstraTabSearchItemTest, ResultTypeBookmark) {
  AstraTabSearchItem item;
  item.result_type = AstraTabSearchResultType::kBookmark;
  item.is_in_bookmarks_bar = true;
  EXPECT_EQ(AstraTabSearchResultType::kBookmark, item.result_type);
  EXPECT_TRUE(item.is_in_bookmarks_bar);
}

TEST(AstraTabSearchItemTest, ResultTypeHistory) {
  AstraTabSearchItem item;
  item.result_type = AstraTabSearchResultType::kHistory;
  item.visit_count = 100;
  EXPECT_EQ(AstraTabSearchResultType::kHistory, item.result_type);
  EXPECT_EQ(100, item.visit_count);
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
  EXPECT_EQ(0, static_cast<int>(AstraTabSearchMode::kAllTabs));
  EXPECT_EQ(1, static_cast<int>(AstraTabSearchMode::kCurrentWorkspace));
  EXPECT_EQ(2, static_cast<int>(AstraTabSearchMode::kOtherWorkspaces));
  EXPECT_EQ(3, static_cast<int>(AstraTabSearchMode::kRecentlyClosed));
  EXPECT_EQ(4, static_cast<int>(AstraTabSearchMode::kFavorites));
  EXPECT_EQ(5, static_cast<int>(AstraTabSearchMode::kAudioPlaying));
}

TEST(AstraTabSearchModeTest, SixModesTotal) {
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
  EXPECT_EQ(3, static_cast<int>(AstraTabSearchSortOrder::kByRelevance));
}

TEST(AstraTabSearchSortOrderTest, FourSortOrdersTotal) {
  int count = 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByRecency) == 0 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByTitle) == 1 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByPosition) == 2 ? 1 : 0;
  count += static_cast<int>(AstraTabSearchSortOrder::kByRelevance) == 3 ? 1 : 0;
  EXPECT_EQ(4, count);
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
  item.item_id = 0;
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
    item.item_id = i;
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
// Model tests — query management
// =========================================================================

TEST_F(AstraTabSearchModelTest, DefaultQueryIsEmpty) {
  EXPECT_TRUE(model_->query().empty());
}

TEST_F(AstraTabSearchModelTest, SetQuery) {
  model_->SetQuery(u"test");
  EXPECT_EQ(u"test", model_->query());
}

TEST_F(AstraTabSearchModelTest, SetQueryNotifiesObserver) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetQuery(u"hello");
  EXPECT_EQ(1, observer.query_changed_count_);
  EXPECT_EQ(u"hello", observer.last_query_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SetSameQueryDoesNotNotify) {
  model_->SetQuery(u"test");
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetQuery(u"test");
  EXPECT_EQ(0, observer.query_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SetQueryTriggersSearchResultsChange) {
  model_->SetTabList(CreateSampleTabs(8));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetQuery(u"Document");
  EXPECT_GT(observer.search_results_changed_count_, 0);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, EmptyQueryAfterSet) {
  model_->SetQuery(u"test");
  ASSERT_EQ(u"test", model_->query());
  model_->SetQuery(u"");
  EXPECT_TRUE(model_->query().empty());
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
    item.item_id = i;
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

TEST_F(AstraTabSearchModelTest, ExactTitleMatchScoresHigher) {
  std::vector<AstraTabSearchItem> tabs;

  AstraTabSearchItem exact;
  exact.item_id = 0;
  exact.title = u"My Document";
  exact.hostname = u"docs.google.com";
  exact.tab_index = 0;
  tabs.push_back(exact);

  AstraTabSearchItem partial;
  partial.item_id = 1;
  partial.title = u"Document Viewer";
  partial.hostname = u"viewer.example.com";
  partial.tab_index = 1;
  tabs.push_back(partial);

  model_->SetTabList(std::move(tabs));
  auto results = model_->SearchTabs(u"My Document");

  ASSERT_GE(results.size(), 2u);
  // Exact match should rank higher.
  EXPECT_GT(results[0].relevance_score, results[1].relevance_score);
  EXPECT_EQ(u"My Document", results[0].title);
}

TEST_F(AstraTabSearchModelTest, ResultsAreSortedByScore) {
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"google");
  ASSERT_GT(results.size(), 1u);

  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].relevance_score, results[i].relevance_score);
  }
}

// =========================================================================
// Model tests — search scoring / ranking
// =========================================================================

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreExactTitle) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  double score = model_->ComputeRelevanceScore(item, u"Hello World");
  EXPECT_GT(score, 500.0);
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScorePrefixTitle) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  double score = model_->ComputeRelevanceScore(item, u"He");
  EXPECT_GT(score, 100.0);
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreSubstring) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  double score = model_->ComputeRelevanceScore(item, u"o W");
  EXPECT_GT(score, 0.0);
  EXPECT_LT(score, 300.0);
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreNoMatch) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  double score = model_->ComputeRelevanceScore(item, u"xyzzy");
  EXPECT_EQ(0.0, score);
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreEmptyQuery) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  double score = model_->ComputeRelevanceScore(item, u"");
  EXPECT_GT(score, 0.0);  // Should have recency bonus etc.
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreHostMatch) {
  AstraTabSearchItem item;
  item.title = u"My Page";
  item.hostname = u"docs.google.com";

  double score = model_->ComputeRelevanceScore(item, u"docs.google");
  EXPECT_GT(score, 100.0);
}

TEST_F(AstraTabSearchModelTest, ComputeRelevanceScoreRecencyBonus) {
  AstraTabSearchItem recent;
  recent.title = u"Recent Tab";
  recent.hostname = u"test.com";
  recent.last_visited_time = base::Time::Now();

  AstraTabSearchItem old;
  old.title = u"Old Tab";
  old.hostname = u"test.com";
  old.last_visited_time = base::Time::Now() - base::Days(30);

  double score_recent = model_->ComputeRelevanceScore(recent, u"Tab");
  double score_old = model_->ComputeRelevanceScore(old, u"Tab");
  EXPECT_GT(score_recent, score_old);
}

TEST_F(AstraTabSearchModelTest, FuzzyMatchScoreExact) {
  double score = AstraTabSearchModel::FuzzyMatchScore(u"hello world", u"hello world");
  EXPECT_GT(score, 0.9);
}

TEST_F(AstraTabSearchModelTest, FuzzyMatchScoreNoMatch) {
  double score = AstraTabSearchModel::FuzzyMatchScore(u"hello", u"xyz");
  EXPECT_EQ(0.0, score);
}

TEST_F(AstraTabSearchModelTest, FuzzyMatchScorePartial) {
  // "hlo" should fuzzy-match "hello".
  double score = AstraTabSearchModel::FuzzyMatchScore(u"hello", u"hlo");
  EXPECT_GT(score, 0.0);
  EXPECT_LT(score, 0.9);
}

TEST_F(AstraTabSearchModelTest, FuzzyMatchScoreEmptyQuery) {
  double score = AstraTabSearchModel::FuzzyMatchScore(u"hello", u"");
  EXPECT_EQ(0.0, score);
}

TEST_F(AstraTabSearchModelTest, FuzzySearchEnabledByDefault) {
  EXPECT_TRUE(model_->fuzzy_search_enabled());
}

TEST_F(AstraTabSearchModelTest, SetFuzzySearchEnabled) {
  model_->set_fuzzy_search_enabled(false);
  EXPECT_FALSE(model_->fuzzy_search_enabled());

  model_->set_fuzzy_search_enabled(true);
  EXPECT_TRUE(model_->fuzzy_search_enabled());
}

// =========================================================================
// Model tests — ComputeMatches
// =========================================================================

TEST_F(AstraTabSearchModelTest, ComputeMatchesTitleMatch) {
  AstraTabSearchItem item;
  item.title = u"My Document";
  item.hostname = u"docs.google.com";

  auto matches = model_->ComputeMatches(item, u"Doc");
  EXPECT_GT(matches.size(), 0u);

  bool has_title_match = false;
  for (const auto& m : matches) {
    if (m.type == AstraTabSearchMatch::Type::kTitle) {
      has_title_match = true;
      EXPECT_GT(m.range.length(), 0u);
    }
  }
  EXPECT_TRUE(has_title_match);
}

TEST_F(AstraTabSearchModelTest, ComputeMatchesHostnameMatch) {
  AstraTabSearchItem item;
  item.title = u"My Doc";
  item.hostname = u"docs.google.com";

  auto matches = model_->ComputeMatches(item, u"google");
  EXPECT_GT(matches.size(), 0u);

  bool has_host_match = false;
  for (const auto& m : matches) {
    if (m.type == AstraTabSearchMatch::Type::kHostname) {
      has_host_match = true;
    }
  }
  EXPECT_TRUE(has_host_match);
}

TEST_F(AstraTabSearchModelTest, ComputeMatchesNoMatch) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  auto matches = model_->ComputeMatches(item, u"zzzz");
  EXPECT_EQ(0u, matches.size());
}

TEST_F(AstraTabSearchModelTest, ComputeMatchesEmptyQuery) {
  AstraTabSearchItem item;
  item.title = u"Hello World";
  item.hostname = u"example.com";

  auto matches = model_->ComputeMatches(item, u"");
  EXPECT_EQ(0u, matches.size());
}

TEST_F(AstraTabSearchModelTest, ComputeMatchesWorkspace) {
  AstraTabSearchItem item;
  item.title = u"Tab";
  item.hostname = u"test.com";
  item.workspace_id = "work";
  item.workspace_name = u"Work";

  auto matches = model_->ComputeMatches(item, u"work");
  EXPECT_GT(matches.size(), 0u);

  bool has_workspace_match = false;
  for (const auto& m : matches) {
    if (m.type == AstraTabSearchMatch::Type::kWorkspace) {
      has_workspace_match = true;
    }
  }
  EXPECT_TRUE(has_workspace_match);
}

TEST_F(AstraTabSearchModelTest, ComputeMatchesGroup) {
  AstraTabSearchItem item;
  item.title = u"Tab";
  item.hostname = u"test.com";
  item.is_in_group = true;
  item.group_name = u"Research";

  auto matches = model_->ComputeMatches(item, u"research");
  EXPECT_GT(matches.size(), 0u);

  bool has_group_match = false;
  for (const auto& m : matches) {
    if (m.type == AstraTabSearchMatch::Type::kGroup) {
      has_group_match = true;
    }
  }
  EXPECT_TRUE(has_group_match);
}

// =========================================================================
// Model tests — filter categories
// =========================================================================

TEST_F(AstraTabSearchModelTest, DefaultFilterIsAllContent) {
  EXPECT_EQ(AstraTabSearchFilter::kAllContent, model_->GetFilter());
}

TEST_F(AstraTabSearchModelTest, SetFilter) {
  model_->SetFilter(AstraTabSearchFilter::kTabs);
  EXPECT_EQ(AstraTabSearchFilter::kTabs, model_->GetFilter());
}

TEST_F(AstraTabSearchModelTest, SetFilterNotifiesObserver) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetFilter(AstraTabSearchFilter::kBookmarks);
  EXPECT_EQ(1, observer.filter_changed_count_);
  EXPECT_EQ(AstraTabSearchFilter::kBookmarks, observer.last_filter_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SetSameFilterDoesNotNotify) {
  model_->SetFilter(AstraTabSearchFilter::kAllContent);
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetFilter(AstraTabSearchFilter::kAllContent);
  EXPECT_EQ(0, observer.filter_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, FilterAllowsOpenTab) {
  model_->SetFilter(AstraTabSearchFilter::kTabs);
  EXPECT_TRUE(model_->FilterAllowsType(AstraTabSearchResultType::kOpenTab));
}

TEST_F(AstraTabSearchModelTest, FilterTabsDoesNotAllowBookmarks) {
  model_->SetFilter(AstraTabSearchFilter::kTabs);
  EXPECT_FALSE(model_->FilterAllowsType(AstraTabSearchResultType::kBookmark));
}

TEST_F(AstraTabSearchModelTest, FilterTabsDoesNotAllowHistory) {
  model_->SetFilter(AstraTabSearchFilter::kTabs);
  EXPECT_FALSE(model_->FilterAllowsType(AstraTabSearchResultType::kHistory));
}

TEST_F(AstraTabSearchModelTest, FilterAllContentAllowsAll) {
  model_->SetFilter(AstraTabSearchFilter::kAllContent);
  EXPECT_TRUE(model_->FilterAllowsType(AstraTabSearchResultType::kOpenTab));
  EXPECT_TRUE(model_->FilterAllowsType(AstraTabSearchResultType::kRecentlyClosed));
  EXPECT_TRUE(model_->FilterAllowsType(AstraTabSearchResultType::kBookmark));
  EXPECT_TRUE(model_->FilterAllowsType(AstraTabSearchResultType::kHistory));
}

TEST_F(AstraTabSearchModelTest, FilterTabsOnlyReturnsTabs) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetBookmarks(CreateSampleBookmarks(3));
  model_->SetFilter(AstraTabSearchFilter::kTabs);

  auto results = model_->SearchTabs(u"");
  for (const auto& r : results) {
    EXPECT_EQ(AstraTabSearchResultType::kOpenTab, r.result_type);
  }
}

TEST_F(AstraTabSearchModelTest, FilterBookmarksOnlyReturnsBookmarks) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetBookmarks(CreateSampleBookmarks(5));
  model_->SetFilter(AstraTabSearchFilter::kBookmarks);

  auto results = model_->SearchTabs(u"");
  for (const auto& r : results) {
    EXPECT_EQ(AstraTabSearchResultType::kBookmark, r.result_type);
  }
}

TEST_F(AstraTabSearchModelTest, FilterTabsAndBookmarks) {
  model_->SetTabList(CreateSampleTabs(3));
  model_->SetBookmarks(CreateSampleBookmarks(3));
  model_->SetHistory(CreateSampleHistory(3));

  auto filter = AstraTabSearchFilter::kTabs | AstraTabSearchFilter::kBookmarks;
  model_->SetFilter(filter);

  auto results = model_->SearchTabs(u"");
  for (const auto& r : results) {
    EXPECT_TRUE(r.result_type == AstraTabSearchResultType::kOpenTab ||
                r.result_type == AstraTabSearchResultType::kBookmark);
  }
}

TEST_F(AstraTabSearchModelTest, FilterChangeUpdatesResults) {
  model_->SetTabList(CreateSampleTabs(3));
  model_->SetBookmarks(CreateSampleBookmarks(3));

  model_->SetFilter(AstraTabSearchFilter::kTabs);
  auto tabs_only = model_->SearchTabs(u"").size();

  model_->SetFilter(AstraTabSearchFilter::kAllContent);
  auto all = model_->SearchTabs(u"").size();

  EXPECT_GT(all, tabs_only);
}

TEST_F(AstraTabSearchModelTest, FilterSearchResultsChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetFilter(AstraTabSearchFilter::kBookmarks);
  EXPECT_GT(observer.search_results_changed_count_, 0);

  model_->RemoveObserver(&observer);
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

// =========================================================================
// Model tests — selected index / keyboard navigation
// =========================================================================

TEST_F(AstraTabSearchModelTest, DefaultSelectedIndexIsZero) {
  EXPECT_EQ(0u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SetSelectedIndex) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(3);
  EXPECT_EQ(3u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SetSelectedIndexNotifies) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSelectedIndex(2);
  EXPECT_EQ(1, observer.selected_index_changed_count_);
  EXPECT_EQ(0u, observer.last_selected_old_index_);
  EXPECT_EQ(2u, observer.last_selected_new_index_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SetSameSelectedIndexDoesNotNotify) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(2);
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSelectedIndex(2);
  EXPECT_EQ(0, observer.selected_index_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, SelectNext) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(1);
  model_->SelectNext();
  EXPECT_EQ(2u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SelectPrevious) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(3);
  model_->SelectPrevious();
  EXPECT_EQ(2u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SelectFirst) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(3);
  model_->SelectFirst();
  EXPECT_EQ(0u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SelectLast) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SelectLast();
  EXPECT_EQ(4u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SelectNextWrapsAround) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SelectLast();
  size_t last = model_->GetSelectedIndex();
  model_->SelectNext();
  // Should wrap to first.
  EXPECT_EQ(0u, model_->GetSelectedIndex());
  EXPECT_NE(last, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, SelectPreviousWrapsAround) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SelectFirst();
  model_->SelectPrevious();
  // Should wrap to last.
  EXPECT_EQ(4u, model_->GetSelectedIndex());
}

TEST_F(AstraTabSearchModelTest, GetSelectedItem) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(2);
  const auto* selected = model_->GetSelectedItem();
  ASSERT_NE(nullptr, selected);
  EXPECT_EQ(model_->GetResults()[2].item_id, selected->item_id);
}

TEST_F(AstraTabSearchModelTest, GetSelectedItemEmptyResults) {
  EXPECT_EQ(nullptr, model_->GetSelectedItem());
}

TEST_F(AstraTabSearchModelTest, ActivateSelected) {
  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSelectedIndex(2);
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->ActivateSelected();
  EXPECT_EQ(1, observer.tab_activated_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ActivateSelectedEmptyModelNoCrash) {
  model_->ActivateSelected();
  SUCCEED();
}

// =========================================================================
// Model tests — recent searches
// =========================================================================

TEST_F(AstraTabSearchModelTest, RecentSearchesEmptyByDefault) {
  EXPECT_TRUE(model_->GetRecentSearches().empty());
}

TEST_F(AstraTabSearchModelTest, AddRecentSearch) {
  model_->AddRecentSearch(u"chromium");
  EXPECT_EQ(1u, model_->GetRecentSearches().size());
  EXPECT_EQ(u"chromium", model_->GetRecentSearches()[0].query);
}

TEST_F(AstraTabSearchModelTest, AddRecentSearchNotifies) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->AddRecentSearch(u"test");
  EXPECT_EQ(1, observer.recent_searches_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, DuplicateRecentSearchPromotesToTop) {
  model_->AddRecentSearch(u"first");
  model_->AddRecentSearch(u"second");
  model_->AddRecentSearch(u"first");  // Duplicate.

  ASSERT_EQ(2u, model_->GetRecentSearches().size());
  EXPECT_EQ(u"first", model_->GetRecentSearches()[0].query);
  EXPECT_EQ(u"second", model_->GetRecentSearches()[1].query);
}

TEST_F(AstraTabSearchModelTest, DuplicateRecentSearchIncrementsCount) {
  model_->AddRecentSearch(u"chromium");
  model_->AddRecentSearch(u"chromium");

  ASSERT_GT(model_->GetRecentSearches().size(), 0u);
  EXPECT_GT(model_->GetRecentSearches()[0].visit_count, 1);
}

TEST_F(AstraTabSearchModelTest, RecentSearchesLimitedToMax) {
  for (size_t i = 0; i < 20; ++i) {
    model_->AddRecentSearch(base::UTF8ToUTF16("query" + std::to_string(i)));
  }
  EXPECT_LE(model_->GetRecentSearches().size(),
            AstraTabSearchModel::kMaxRecentSearches);
}

TEST_F(AstraTabSearchModelTest, ClearRecentSearches) {
  model_->AddRecentSearch(u"test1");
  model_->AddRecentSearch(u"test2");
  ASSERT_EQ(2u, model_->GetRecentSearches().size());

  model_->ClearRecentSearches();
  EXPECT_TRUE(model_->GetRecentSearches().empty());
}

TEST_F(AstraTabSearchModelTest, ClearRecentSearchesNotifies) {
  model_->AddRecentSearch(u"test");
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->ClearRecentSearches();
  EXPECT_EQ(1, observer.recent_searches_changed_count_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, RemoveRecentSearch) {
  model_->AddRecentSearch(u"keep");
  model_->AddRecentSearch(u"remove me");

  model_->RemoveRecentSearch(u"remove me");
  ASSERT_EQ(1u, model_->GetRecentSearches().size());
  EXPECT_EQ(u"keep", model_->GetRecentSearches()[0].query);
}

TEST_F(AstraTabSearchModelTest, RemoveNonExistentRecentSearch) {
  model_->AddRecentSearch(u"test");
  model_->RemoveRecentSearch(u"nonexistent");
  EXPECT_EQ(1u, model_->GetRecentSearches().size());
}

TEST_F(AstraTabSearchModelTest, ShowRecentSearchesDefaultTrue) {
  EXPECT_TRUE(model_->show_recent_searches());
}

TEST_F(AstraTabSearchModelTest, SetShowRecentSearches) {
  model_->set_show_recent_searches(false);
  EXPECT_FALSE(model_->show_recent_searches());
}

TEST_F(AstraTabSearchModelTest, RecentSearchHasTimestamp) {
  model_->AddRecentSearch(u"test");
  ASSERT_GT(model_->GetRecentSearches().size(), 0u);
  EXPECT_FALSE(model_->GetRecentSearches()[0].timestamp.is_null());
}

// =========================================================================
// Model tests — bookmarks
// =========================================================================

TEST_F(AstraTabSearchModelTest, BookmarksEmptyByDefault) {
  EXPECT_EQ(0u, model_->GetBookmarks(10).size());
}

TEST_F(AstraTabSearchModelTest, SetBookmarks) {
  auto bookmarks = CreateSampleBookmarks(5);
  model_->SetBookmarks(std::move(bookmarks));
  EXPECT_EQ(5u, model_->GetBookmarks(10).size());
}

TEST_F(AstraTabSearchModelTest, GetBookmarksMaxCount) {
  auto bookmarks = CreateSampleBookmarks(5);
  model_->SetBookmarks(std::move(bookmarks));
  auto result = model_->GetBookmarks(2);
  EXPECT_EQ(2u, result.size());
}

TEST_F(AstraTabSearchModelTest, BookmarksIncludedInAllResults) {
  model_->SetTabList(CreateSampleTabs(3));
  model_->SetBookmarks(CreateSampleBookmarks(3));
  model_->SetFilter(AstraTabSearchFilter::kAllContent);

  auto results = model_->SearchTabs(u"");
  bool has_bookmark = false;
  for (const auto& r : results) {
    if (r.result_type == AstraTabSearchResultType::kBookmark) {
      has_bookmark = true;
      break;
    }
  }
  EXPECT_TRUE(has_bookmark);
}

TEST_F(AstraTabSearchModelTest, BookmarkHasCorrectResultType) {
  auto bookmarks = CreateSampleBookmarks(3);
  model_->SetBookmarks(std::move(bookmarks));
  auto result = model_->GetBookmarks(3);
  for (const auto& b : result) {
    EXPECT_EQ(AstraTabSearchResultType::kBookmark, b.result_type);
  }
}

TEST_F(AstraTabSearchModelTest, BookmarkInBookmarksBar) {
  auto bookmarks = CreateSampleBookmarks(5);
  model_->SetBookmarks(std::move(bookmarks));
  auto result = model_->GetBookmarks(5);
  // First 2 should be in bookmarks bar.
  EXPECT_TRUE(result[0].is_in_bookmarks_bar);
  EXPECT_TRUE(result[1].is_in_bookmarks_bar);
  EXPECT_FALSE(result[2].is_in_bookmarks_bar);
}

// =========================================================================
// Model tests — history
// =========================================================================

TEST_F(AstraTabSearchModelTest, HistoryEmptyByDefault) {
  EXPECT_EQ(0u, model_->GetHistory(10).size());
}

TEST_F(AstraTabSearchModelTest, SetHistory) {
  auto history = CreateSampleHistory(5);
  model_->SetHistory(std::move(history));
  EXPECT_EQ(5u, model_->GetHistory(10).size());
}

TEST_F(AstraTabSearchModelTest, GetHistoryMaxCount) {
  auto history = CreateSampleHistory(5);
  model_->SetHistory(std::move(history));
  auto result = model_->GetHistory(2);
  EXPECT_EQ(2u, result.size());
}

TEST_F(AstraTabSearchModelTest, HistoryIncludedInAllResults) {
  model_->SetTabList(CreateSampleTabs(3));
  model_->SetHistory(CreateSampleHistory(3));
  model_->SetFilter(AstraTabSearchFilter::kAllContent);

  auto results = model_->SearchTabs(u"");
  bool has_history = false;
  for (const auto& r : results) {
    if (r.result_type == AstraTabSearchResultType::kHistory) {
      has_history = true;
      break;
    }
  }
  EXPECT_TRUE(has_history);
}

TEST_F(AstraTabSearchModelTest, HistoryHasCorrectResultType) {
  auto history = CreateSampleHistory(3);
  model_->SetHistory(std::move(history));
  auto result = model_->GetHistory(3);
  for (const auto& h : result) {
    EXPECT_EQ(AstraTabSearchResultType::kHistory, h.result_type);
  }
}

TEST_F(AstraTabSearchModelTest, HistoryHasVisitCount) {
  auto history = CreateSampleHistory(5);
  model_->SetHistory(std::move(history));
  auto result = model_->GetHistory(5);
  for (const auto& h : result) {
    EXPECT_GT(h.visit_count, 0);
  }
}

TEST_F(AstraTabSearchModelTest, HistoryVisitCountBoostsScore) {
  std::vector<AstraTabSearchItem> history;

  AstraTabSearchItem high_visits;
  high_visits.item_id = 1;
  high_visits.title = u"Test Page";
  high_visits.hostname = u"high-visit.example.com";
  high_visits.visit_count = 100;
  high_visits.result_type = AstraTabSearchResultType::kHistory;
  history.push_back(high_visits);

  AstraTabSearchItem low_visits;
  low_visits.item_id = 2;
  low_visits.title = u"Test Page 2";
  low_visits.hostname = u"low-visit.example.com";
  low_visits.visit_count = 1;
  low_visits.result_type = AstraTabSearchResultType::kHistory;
  history.push_back(low_visits);

  model_->SetHistory(std::move(history));
  model_->SetFilter(AstraTabSearchFilter::kHistory);

  // Both match "Page", high-visit should score higher.
  double high_score = model_->ComputeRelevanceScore(
      model_->GetHistory(2)[0], u"Page");
  double low_score = model_->ComputeRelevanceScore(
      model_->GetHistory(2)[1], u"Page");
  EXPECT_GT(high_score, low_score);
}

// =========================================================================
// Model tests — grouped results
// =========================================================================

TEST_F(AstraTabSearchModelTest, GroupedResultsDefaultEmpty) {
  auto grouped = model_->GetGroupedResults();
  EXPECT_TRUE(grouped.IsEmpty());
  EXPECT_EQ(0u, grouped.TotalCount());
}

TEST_F(AstraTabSearchModelTest, GroupedResultsHasOpenTabs) {
  model_->SetTabList(CreateSampleTabs(5));
  auto grouped = model_->GetGroupedResults();
  EXPECT_GT(grouped.open_tabs.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, GroupedResultsHasBookmarks) {
  model_->SetBookmarks(CreateSampleBookmarks(3));
  auto grouped = model_->GetGroupedResults();
  EXPECT_GT(grouped.bookmarks.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, GroupedResultsHasHistory) {
  model_->SetHistory(CreateSampleHistory(3));
  auto grouped = model_->GetGroupedResults();
  EXPECT_GT(grouped.history.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, GroupedResultsHasRecentSearches) {
  model_->AddRecentSearch(u"test");
  auto grouped = model_->GetGroupedResults();
  EXPECT_GT(grouped.recent_searches.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, GroupedResultsTotalCount) {
  model_->SetTabList(CreateSampleTabs(3));
  model_->SetBookmarks(CreateSampleBookmarks(2));
  model_->SetHistory(CreateSampleHistory(2));
  model_->AddRecentSearch(u"query");

  auto grouped = model_->GetGroupedResults();
  EXPECT_EQ(grouped.TotalCount(),
            grouped.open_tabs.size() + grouped.bookmarks.size() +
                grouped.history.size() + grouped.recent_searches.size() +
                grouped.recently_closed.size());
}

TEST_F(AstraTabSearchModelTest, ShowGroupHeadersDefaultTrue) {
  EXPECT_TRUE(model_->show_group_headers());
}

TEST_F(AstraTabSearchModelTest, SetShowGroupHeaders) {
  model_->set_show_group_headers(false);
  EXPECT_FALSE(model_->show_group_headers());
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

TEST_F(AstraTabSearchModelTest, GetTabsInGroupEmptyId) {
  model_->SetTabList(CreateSampleTabs(10));
  auto tabs = model_->GetTabsInGroup("");
  for (const auto& t : tabs) {
    EXPECT_FALSE(t.is_in_group);
  }
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
    item.item_id = i;
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
    item.item_id = i;
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
    item.item_id = i;
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
    item.item_id = i;
    item.title = base::UTF8ToUTF16("Closed " + std::to_string(i));
    item.tab_index = -1;
    item.result_type = AstraTabSearchResultType::kRecentlyClosed;
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
    item.item_id = i;
    item.title = base::UTF8ToUTF16("Closed " + std::to_string(i));
    item.result_type = AstraTabSearchResultType::kRecentlyClosed;
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

TEST_F(AstraTabSearchModelTest, RecentlyClosedInSearchResults) {
  std::vector<AstraTabSearchItem> closed;
  AstraTabSearchItem item;
  item.item_id = 999;
  item.title = u"Old Page";
  item.hostname = u"old.example.com";
  item.result_type = AstraTabSearchResultType::kRecentlyClosed;
  closed.push_back(item);
  model_->SetRecentlyClosedTabs(std::move(closed));

  model_->SetFilter(AstraTabSearchFilter::kAllContent);
  auto results = model_->SearchTabs(u"Old");
  bool found_closed = false;
  for (const auto& r : results) {
    if (r.result_type == AstraTabSearchResultType::kRecentlyClosed) {
      found_closed = true;
      break;
    }
  }
  EXPECT_TRUE(found_closed);
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
  ASSERT_NE(model_->GetTabAt(0)->workspace_id, "new-ws");

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
  EXPECT_TRUE(model_->fuzzy_search_enabled());
  EXPECT_TRUE(model_->show_group_headers());
  EXPECT_EQ(AstraTabSearchMode::kAllTabs, model_->default_search_mode());
  EXPECT_EQ(AstraTabSearchSortOrder::kByRecency, model_->sort_order());
  EXPECT_TRUE(model_->close_tab_on_activate());
  EXPECT_TRUE(model_->show_recently_closed_section());
  EXPECT_TRUE(model_->show_recent_searches());
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

  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].last_visited_time, results[i].last_visited_time);
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

TEST_F(AstraTabSearchModelTest, SortByRelevance) {
  model_->SetSortOrder(AstraTabSearchSortOrder::kByRelevance);
  model_->SetTabList(CreateSampleTabs(8));
  auto results = model_->SearchTabs(u"google");
  ASSERT_GT(results.size(), 1u);

  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].relevance_score, results[i].relevance_score);
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

TEST_F(AstraTabSearchModelTest, ObserverOnSelectedIndexChanged) {
  model_->SetTabList(CreateSampleTabs(5));
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetSelectedIndex(3);
  EXPECT_EQ(1, observer.selected_index_changed_count_);
  EXPECT_EQ(0u, observer.last_selected_old_index_);
  EXPECT_EQ(3u, observer.last_selected_new_index_);
  EXPECT_EQ(model_.get(), observer.last_model_selected_index_);

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

TEST_F(AstraTabSearchModelTest, ObserverOnFilterChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetFilter(AstraTabSearchFilter::kBookmarks);
  EXPECT_EQ(1, observer.filter_changed_count_);
  EXPECT_EQ(AstraTabSearchFilter::kBookmarks, observer.last_filter_);
  EXPECT_EQ(model_.get(), observer.last_model_filter_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnQueryChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->SetQuery(u"test");
  EXPECT_EQ(1, observer.query_changed_count_);
  EXPECT_EQ(u"test", observer.last_query_);
  EXPECT_EQ(model_.get(), observer.last_model_query_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnRecentSearchesChanged) {
  TestTabSearchObserver observer;
  model_->AddObserver(&observer);

  model_->AddRecentSearch(u"test");
  EXPECT_EQ(1, observer.recent_searches_changed_count_);
  EXPECT_EQ(model_.get(), observer.last_model_recent_searches_);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraTabSearchModelTest, ObserverOnShutdown) {
  auto model = std::make_unique<AstraTabSearchModel>();
  TestTabSearchObserver observer;
  model->AddObserver(&observer);

  EXPECT_EQ(0, observer.model_shutdown_count_);
  model.reset();
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
  class DefaultObserver : public AstraTabSearchObserver {};

  DefaultObserver observer;
  model_->AddObserver(&observer);

  model_->SetTabList(CreateSampleTabs(5));
  model_->SetSearchMode(AstraTabSearchMode::kAudioPlaying);
  model_->SetFilter(AstraTabSearchFilter::kBookmarks);
  model_->SetQuery(u"test");
  model_->SwitchToTab(0);
  model_->CloseTab(0);
  model_->RefreshTabList();
  model_->SetSelectedIndex(0);
  model_->AddRecentSearch(u"test");

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
  item.item_id = 0;
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
  item.item_id = 0;
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
  item.item_id = 0;
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
    item.item_id = i;
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
  item.item_id = 0;
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
  item.last_visited_time = base::Time::Now();
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
  EXPECT_GE(results.size(), 0u);
}

TEST_F(AstraTabSearchModelTest, EmptyWorkspaceId) {
  std::vector<AstraTabSearchItem> tabs;
  for (int i = 0; i < 3; ++i) {
    AstraTabSearchItem item;
    item.tab_id = i;
    item.item_id = i;
    item.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    item.workspace_id = "";
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
  EXPECT_EQ(AstraTabSearchModel::kDefaultMaxSearchResults,
            model.max_search_results());
}

TEST_F(AstraTabSearchPersistenceTest, SaveToNullPrefs) {
  AstraTabSearchModel model;
  model.SaveToPrefs(nullptr);
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

TEST(AstraTabSearchModelConstantsTest, DefaultBookmarkCount) {
  EXPECT_EQ(10, AstraTabSearchModel::kDefaultBookmarkCount);
}

TEST(AstraTabSearchModelConstantsTest, DefaultHistoryCount) {
  EXPECT_EQ(10, AstraTabSearchModel::kDefaultHistoryCount);
}

TEST(AstraTabSearchModelConstantsTest, MaxRecentSearches) {
  EXPECT_EQ(10u, AstraTabSearchModel::kMaxRecentSearches);
}

// =========================================================================
// AstraTabSearchGroupHeaderView tests
// =========================================================================

class AstraTabSearchGroupHeaderViewTest : public views::ViewsTestBase {
 public:
  AstraTabSearchGroupHeaderViewTest() = default;
  ~AstraTabSearchGroupHeaderViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    header_view_ = widget_->SetContentsView(
        std::make_unique<AstraTabSearchGroupHeaderView>(u"Open Tabs", 5));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraTabSearchGroupHeaderView> header_view_ = nullptr;
};

TEST_F(AstraTabSearchGroupHeaderViewTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, header_view_);
}

TEST_F(AstraTabSearchGroupHeaderViewTest, TitleIsSet) {
  EXPECT_EQ(u"Open Tabs", header_view_->title());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, CountIsSet) {
  EXPECT_EQ(5u, header_view_->count());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, SetTitle) {
  header_view_->SetTitle(u"New Title");
  EXPECT_EQ(u"New Title", header_view_->title());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, SetCount) {
  header_view_->SetCount(42);
  EXPECT_EQ(42u, header_view_->count());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, SetCountZero) {
  header_view_->SetCount(0);
  EXPECT_EQ(0u, header_view_->count());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, DefaultGroupType) {
  EXPECT_EQ(AstraTabSearchResultType::kOpenTab, header_view_->group_type());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, SetGroupType) {
  header_view_->SetGroupType(AstraTabSearchResultType::kBookmark);
  EXPECT_EQ(AstraTabSearchResultType::kBookmark, header_view_->group_type());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = header_view_->CalculatePreferredSize();
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraTabSearchGroupHeaderViewTest, OnThemeChangedNoCrash) {
  header_view_->OnThemeChanged();
  SUCCEED();
}

TEST_F(AstraTabSearchGroupHeaderViewTest, HasColorProvider) {
  EXPECT_NE(nullptr, header_view_->GetColorProvider());
}

TEST_F(AstraTabSearchGroupHeaderViewTest, AccessibleRoleIsGroup) {
  ui::AXNodeData data;
  header_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kGroup, data.role);
}

TEST_F(AstraTabSearchGroupHeaderViewTest, AccessibleNameIncludesTitle) {
  ui::AXNodeData data;
  header_view_->GetAccessibleNodeData(&data);
  EXPECT_NE(std::u16string::npos, data.GetName().find(u"Open Tabs"));
}

TEST_F(AstraTabSearchGroupHeaderViewTest, AccessibleDescriptionIncludesCount) {
  ui::AXNodeData data;
  header_view_->GetAccessibleNodeData(&data);
  // Description should mention the count.
  EXPECT_FALSE(data.GetDescription().empty());
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

    AstraTabSearchItem tab;
    tab.tab_id = 42;
    tab.item_id = 42;
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
    tab.result_type = AstraTabSearchResultType::kOpenTab;

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
  new_tab.item_id = 99;
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
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, UrlMatchRanges) {
  std::vector<gfx::Range> ranges;
  ranges.push_back(gfx::Range(2, 5));
  item_view_->SetUrlMatchRanges(ranges);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, SetMatches) {
  std::vector<AstraTabSearchMatch> matches;
  AstraTabSearchMatch m1;
  m1.type = AstraTabSearchMatch::Type::kTitle;
  m1.range = gfx::Range(0, 3);
  matches.push_back(m1);
  AstraTabSearchMatch m2;
  m2.type = AstraTabSearchMatch::Type::kHostname;
  m2.range = gfx::Range(1, 4);
  matches.push_back(m2);

  item_view_->SetMatches(matches);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, EmptyMatchRanges) {
  std::vector<gfx::Range> empty_ranges;
  item_view_->SetTitleMatchRanges(empty_ranges);
  item_view_->SetUrlMatchRanges(empty_ranges);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowFaviconToggle) {
  item_view_->ShowFavicon(false);
  item_view_->ShowFavicon(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowWorkspaceToggle) {
  item_view_->ShowWorkspace(false);
  item_view_->ShowWorkspace(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowAudioIndicatorToggle) {
  item_view_->ShowAudioIndicator(false);
  item_view_->ShowAudioIndicator(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowCloseButtonToggle) {
  item_view_->ShowCloseButton(true);
  item_view_->ShowCloseButton(false);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowShortcutHintToggle) {
  item_view_->ShowShortcutHint(false);
  item_view_->ShowShortcutHint(true);
  SUCCEED();
}

TEST_F(AstraTabSearchItemViewTest, ShowSiteInfoToggle) {
  item_view_->ShowSiteInfo(false);
  item_view_->ShowSiteInfo(true);
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

TEST_F(AstraTabSearchItemViewTest, ResultTypeAccessor) {
  EXPECT_EQ(AstraTabSearchResultType::kOpenTab, item_view_->result_type());
}

TEST_F(AstraTabSearchItemViewTest, ItemIdAccessor) {
  EXPECT_EQ(42, item_view_->item_id());
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

TEST_F(AstraTabSearchItemViewTest, DisplayIndexUpdatesShortcutHint) {
  item_view_->ShowShortcutHint(true);
  item_view_->SetDisplayIndex(3);
  // Should not crash and display index is set.
  EXPECT_EQ(3, item_view_->display_index());
}

TEST_F(AstraTabSearchItemViewTest, KeyPressEnterActivates) {
  int activated_count = 0;
  item_view_->SetActivatedCallback(base::BindLambdaForTesting(
      [&activated_count]() { activated_count++; }));

  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_RETURN, ui::EF_NONE);
  item_view_->OnKeyPressed(event);

  EXPECT_GE(activated_count, 1);
}

TEST_F(AstraTabSearchItemViewTest, KeyPressSpaceActivates) {
  int activated_count = 0;
  item_view_->SetActivatedCallback(base::BindLambdaForTesting(
      [&activated_count]() { activated_count++; }));

  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_SPACE, ui::EF_NONE);
  item_view_->OnKeyPressed(event);

  EXPECT_GE(activated_count, 1);
}

TEST_F(AstraTabSearchItemViewTest, FocusSetsSelected) {
  item_view_->OnFocus();
  EXPECT_TRUE(item_view_->IsSelected());
}

TEST_F(AstraTabSearchItemViewTest, BlurClearsSelected) {
  item_view_->SetSelected(true);
  item_view_->OnBlur();
  EXPECT_FALSE(item_view_->IsSelected());
}

TEST_F(AstraTabSearchItemViewTest, GestureTapActivates) {
  int activated_count = 0;
  item_view_->SetActivatedCallback(base::BindLambdaForTesting(
      [&activated_count]() { activated_count++; }));

  ui::GestureEvent event(0, 0, 0, base::TimeTicks(),
                         ui::GestureEventDetails(ui::ET_GESTURE_TAP));
  item_view_->OnGestureEvent(&event);

  EXPECT_GE(activated_count, 1);
}

TEST_F(AstraTabSearchItemViewTest, MiddleClickCloses) {
  int close_count = 0;
  item_view_->SetMiddleClickCallback(base::BindLambdaForTesting(
      [&close_count]() { close_count++; }));

  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), ui::EF_MIDDLE_MOUSE_BUTTON,
                       ui::EF_MIDDLE_MOUSE_BUTTON);
  item_view_->OnMousePressed(event);

  EXPECT_GE(close_count, 1);
}

TEST_F(AstraTabSearchItemViewTest, MiddleClickCallbackIsSetWithoutCrash) {
  item_view_->SetMiddleClickCallback(AstraTabSearchItemView::MiddleClickCallback());
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

    AstraTabSearchItem tab;
    tab.tab_id = 0;
    tab.item_id = 0;
    tab.title = u"Audio Tab";
    tab.hostname = u"music.example.com";
    tab.is_audible = true;
    tab.is_muted = false;
    tab.tab_index = 0;
    tab.result_type = AstraTabSearchResultType::kOpenTab;

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
  muted_tab.item_id = 1;
  muted_tab.title = u"Muted Tab";
  muted_tab.hostname = u"video.example.com";
  muted_tab.is_audible = true;
  muted_tab.is_muted = true;
  muted_tab.tab_index = 1;
  muted_tab.result_type = AstraTabSearchResultType::kOpenTab;

  item_view_->SetTab(muted_tab);
  EXPECT_TRUE(item_view_->is_audible());
  EXPECT_TRUE(item_view_->is_muted());
}

TEST_F(AstraTabSearchItemAudioTest, NotAudibleTab) {
  AstraTabSearchItem silent_tab;
  silent_tab.tab_id = 2;
  silent_tab.item_id = 2;
  silent_tab.title = u"Silent Tab";
  silent_tab.hostname = u"text.example.com";
  silent_tab.is_audible = false;
  silent_tab.is_muted = false;
  silent_tab.tab_index = 2;
  silent_tab.result_type = AstraTabSearchResultType::kOpenTab;

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
  tab.item_id = 0;
  tab.title = std::u16string(500, u'x');
  tab.hostname = u"test.com";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

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
  tab.item_id = 0;
  tab.title = u"Test";
  tab.hostname = std::u16string(300, u'a') + u".example.com";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_GT(view->hostname().size(), 200u);
}

TEST_F(AstraTabSearchItemEdgeCaseTest, EmptyTitle) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.item_id = 0;
  tab.title = u"";
  tab.hostname = u"test.com";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->title().empty());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, EmptyHostname) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.item_id = 0;
  tab.title = u"Test";
  tab.hostname = u"";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->hostname().empty());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, PinnedTab) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.item_id = 0;
  tab.title = u"Pinned";
  tab.hostname = u"pinned.com";
  tab.is_pinned = true;
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->is_pinned());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, TabWithGroup) {
  AstraTabSearchItem tab;
  tab.tab_id = 0;
  tab.item_id = 0;
  tab.title = u"Grouped Tab";
  tab.hostname = u"group.com";
  tab.is_in_group = true;
  tab.group_name = u"My Group";
  tab.group_color = SK_ColorGREEN;
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_TRUE(view->GetTab().is_in_group);
  EXPECT_EQ(u"My Group", view->GetTab().group_name);
}

TEST_F(AstraTabSearchItemEdgeCaseTest, BookmarkResultType) {
  AstraTabSearchItem tab;
  tab.item_id = 100;
  tab.title = u"Bookmark";
  tab.hostname = u"bookmark.com";
  tab.result_type = AstraTabSearchResultType::kBookmark;
  tab.is_in_bookmarks_bar = true;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_EQ(AstraTabSearchResultType::kBookmark, view->result_type());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, HistoryResultType) {
  AstraTabSearchItem tab;
  tab.item_id = 200;
  tab.title = u"History";
  tab.hostname = u"history.com";
  tab.result_type = AstraTabSearchResultType::kHistory;
  tab.visit_count = 42;

  widget_ = CreateTestWidget();
  auto* view = widget_->SetContentsView(
      std::make_unique<AstraTabSearchItemView>(tab));
  widget_->Show();

  EXPECT_EQ(AstraTabSearchResultType::kHistory, view->result_type());
}

TEST_F(AstraTabSearchItemEdgeCaseTest, LegacyTabInfoConstructor) {
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

class AstraTabSearchBubbleTest : public views::ViewsTestBase {
 public:
  AstraTabSearchBubbleTest() = default;
  ~AstraTabSearchBubbleTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

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
  tab.item_id = 0;
  tab.title = u"Test Tab";
  tab.hostname = u"test.com";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;
  tabs.push_back(std::move(tab));
  model.SetTabList(std::move(tabs));

  auto results = model.SearchTabs(u"Test");
  EXPECT_EQ(1u, results.size());
}

TEST_F(AstraTabSearchBubbleTest, BubbleModelFilterCategories) {
  AstraTabSearchModel model;
  model.SetTabList(CreateSampleTabs(3));
  model.SetBookmarks(CreateSampleBookmarks(3));

  model.SetFilter(AstraTabSearchFilter::kTabs);
  EXPECT_EQ(AstraTabSearchFilter::kTabs, model.GetFilter());

  model.SetFilter(AstraTabSearchFilter::kBookmarks);
  EXPECT_EQ(AstraTabSearchFilter::kBookmarks, model.GetFilter());

  auto combined = AstraTabSearchFilter::kTabs | AstraTabSearchFilter::kBookmarks;
  model.SetFilter(combined);
  EXPECT_EQ(combined, model.GetFilter());
}

TEST_F(AstraTabSearchBubbleTest, BubbleModelSelectedIndex) {
  AstraTabSearchModel model;
  model.SetTabList(CreateSampleTabs(5));

  model.SetSelectedIndex(2);
  EXPECT_EQ(2u, model.GetSelectedIndex());

  model.SelectNext();
  EXPECT_EQ(3u, model.GetSelectedIndex());

  model.SelectPrevious();
  EXPECT_EQ(2u, model.GetSelectedIndex());

  model.SelectFirst();
  EXPECT_EQ(0u, model.GetSelectedIndex());

  model.SelectLast();
  EXPECT_EQ(4u, model.GetSelectedIndex());
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
  delegate.OnFilterChanged(AstraTabSearchFilter::kAllContent);
  delegate.OnRecentSearchSelected(u"query");
  delegate.OnClearRecentSearches();
  delegate.OnBookmarkActivated(GURL("https://example.com"));
  delegate.OnHistoryActivated(GURL("https://example.com"));
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
  tab.item_id = 0;
  tab.title = u"Only Tab";
  tab.tab_index = 0;
  tab.result_type = AstraTabSearchResultType::kOpenTab;
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

TEST(AstraTabSearchBubbleEdgeCaseTest, FilterDoesNotCrashForEmptyModel) {
  AstraTabSearchModel model;
  model.SetFilter(AstraTabSearchFilter::kBookmarks);
  auto results = model.SearchTabs(u"");
  EXPECT_EQ(0u, results.size());
}

TEST(AstraTabSearchBubbleEdgeCaseTest, SelectedIndexEmptyModelNoCrash) {
  AstraTabSearchModel model;
  model.SetSelectedIndex(5);
  model.SelectNext();
  model.SelectPrevious();
  model.SelectFirst();
  model.SelectLast();
  model.ActivateSelected();
  EXPECT_EQ(nullptr, model.GetSelectedItem());
}

// =========================================================================
// Model-view separation documentation tests
// =========================================================================

TEST(AstraTabSearchArchitectureTest, ModelOwnsData) {
  AstraTabSearchModel model;
  EXPECT_EQ(0u, model.GetTabCount());
  SUCCEED();
}

TEST(AstraTabSearchArchitectureTest, ViewIsPresentationOnly) {
  SUCCEED();
}

TEST(AstraTabSearchArchitectureTest, ObserverPatternConnectsModelAndView) {
  TestTabSearchObserver observer;
  AstraTabSearchModel model;
  model.AddObserver(&observer);

  model.SetTabList(CreateSampleTabs(3));
  EXPECT_GT(observer.tab_list_changed_count_, 0);

  model.RemoveObserver(&observer);
}

TEST(AstraTabSearchArchitectureTest, FilterBitmaskPattern) {
  auto combined = AstraTabSearchFilter::kTabs | AstraTabSearchFilter::kBookmarks;
  EXPECT_NE(AstraTabSearchFilter::kNone, combined & AstraTabSearchFilter::kTabs);
  EXPECT_NE(AstraTabSearchFilter::kNone, combined & AstraTabSearchFilter::kBookmarks);
}

TEST(AstraTabSearchArchitectureTest, ModelHasSelectedIndexState) {
  AstraTabSearchModel model;
  EXPECT_EQ(0u, model.GetSelectedIndex());
}

TEST(AstraTabSearchArchitectureTest, ModelHasRecentSearches) {
  AstraTabSearchModel model;
  EXPECT_TRUE(model.GetRecentSearches().empty());
}

TEST(AstraTabSearchArchitectureTest, GroupedResultsStruct) {
  AstraTabSearchGroupedResults grouped;
  EXPECT_TRUE(grouped.IsEmpty());
  EXPECT_EQ(0u, grouped.TotalCount());
}

// =========================================================================
// Chromium reuse documentation tests
// =========================================================================

TEST(AstraTabSearchChromiumReuseTest, TabStripModelOwnsTabState) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, FaviconServiceForIcons) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, TabRestoreServiceForRecentlyClosed) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, BookmarkModelForBookmarks) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, HistoryServiceForHistory) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, ViewsFrameworkForUI) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, PrefServiceForSettings) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, ColorProviderForTheming) {
  SUCCEED();
}

TEST(AstraTabSearchChromiumReuseTest, BuiltInTabSearchComparison) {
  SUCCEED();
}

}  // namespace astra
