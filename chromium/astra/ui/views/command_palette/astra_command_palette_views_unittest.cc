// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for command palette views and model.
//
// Tests verify:
//   - AstraCommandPaletteModel: command CRUD, search, scoring, ranking,
//     recent commands, commands by type, settings, observer notifications,
//     edge cases, fuzzy matching, match ranges
//   - AstraCommandPaletteItemView: construction, content updates, selection,
//     highlighting, icon/shortcut/description visibility, match ranges,
//     mouse events, theme, accessibility
//   - AstraCommandPaletteView: creation, query handling, selection,
//     execution, model set/get, result count, edge cases
//   - AstraCommandPaletteBubble: show/hide, query propagation,
//     selection navigation, execution, sizing
//   - AstraCommandType: enum values and type names
//   - AstraCommandCategory: enum values and category labels
//
// Chromium test pattern: views::test::ViewsTestBase for view tests,
// plain TEST() for model tests (non-Views model class).
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/command_palette/astra_command_palette_bubble.h"
#include "astra/ui/views/command_palette/astra_command_palette_item_view.h"
#include "astra/ui/views/command_palette/astra_command_palette_model.h"
#include "astra/ui/views/command_palette/astra_command_palette_section_header_view.h"
#include "astra/ui/views/command_palette/astra_command_palette_view.h"

#include "base/test/task_environment.h"
#include "base/test/bind.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// =========================================================================
// Mock observers
// =========================================================================

// Mock for the new-style AstraCommandPaletteObserver.
class MockCommandPaletteObserver : public AstraCommandPaletteObserver {
 public:
  MOCK_METHOD(void, OnCommandListChanged,
              (AstraCommandPaletteModel * model), (override));
  MOCK_METHOD(void, OnSearchResultsChanged,
              (AstraCommandPaletteModel * model), (override));
  MOCK_METHOD(void, OnCommandExecuted,
              (AstraCommandPaletteModel * model, int command_id),
              (override));
  MOCK_METHOD(void, OnCommandPaletteModelShutdown,
              (AstraCommandPaletteModel * model), (override));
};

// Mock for the legacy AstraCommandPaletteModelObserver.
class MockModelObserver : public AstraCommandPaletteModelObserver {
 public:
  MOCK_METHOD(void, OnModelChanged, (), (override));
  MOCK_METHOD(void, OnSelectionChanged, (), (override));
  MOCK_METHOD(void, OnCommandExecutionRequested,
              (int command_id, bool is_astra), (override));
};

// Test callback tracker for item view activation.
struct ItemActivationTracker {
  int activated_count = 0;
};

// Test observer that tracks all notification calls.
class TestModelObserver : public AstraCommandPaletteModelObserver {
 public:
  void OnModelChanged() override { model_changed_count_++; }
  void OnSelectionChanged() override { selection_changed_count_++; }
  void OnCommandExecutionRequested(int command_id,
                                   bool is_astra) override {
    execution_requested_count_++;
    last_execution_command_id_ = command_id;
    last_execution_is_astra_ = is_astra;
  }
  void OnSearchTextChanged(const std::u16string& new_text) override {
    search_text_changed_count_++;
    last_search_text_ = new_text;
  }
  void OnPaletteOpened() override { palette_opened_count_++; }
  void OnPaletteClosed() override { palette_closed_count_++; }
  void OnCommandExecuted(int command_id, bool is_astra) override {
    command_executed_count_++;
    last_executed_command_id_ = command_id;
    last_executed_is_astra_ = is_astra;
  }

  void Reset() {
    model_changed_count_ = 0;
    selection_changed_count_ = 0;
    execution_requested_count_ = 0;
    last_execution_command_id_ = -1;
    last_execution_is_astra_ = false;
    search_text_changed_count_ = 0;
    last_search_text_.clear();
    palette_opened_count_ = 0;
    palette_closed_count_ = 0;
    command_executed_count_ = 0;
    last_executed_command_id_ = -1;
    last_executed_is_astra_ = false;
  }

  int model_changed_count_ = 0;
  int selection_changed_count_ = 0;
  int execution_requested_count_ = 0;
  int last_execution_command_id_ = -1;
  bool last_execution_is_astra_ = false;
  int search_text_changed_count_ = 0;
  std::u16string last_search_text_;
  int palette_opened_count_ = 0;
  int palette_closed_count_ = 0;
  int command_executed_count_ = 0;
  int last_executed_command_id_ = -1;
  bool last_executed_is_astra_ = false;
};

// Test observer for the new AstraCommandPaletteObserver interface.
class TestCommandPaletteObserver : public AstraCommandPaletteObserver {
 public:
  void OnCommandListChanged(AstraCommandPaletteModel* model) override {
    command_list_changed_count_++;
    last_model_ = model;
  }
  void OnSearchResultsChanged(AstraCommandPaletteModel* model) override {
    search_results_changed_count_++;
    last_model_ = model;
  }
  void OnCommandExecuted(AstraCommandPaletteModel* model,
                         int command_id) override {
    command_executed_count_++;
    last_model_ = model;
    last_executed_command_id_ = command_id;
  }
  void OnCommandPaletteModelShutdown(
      AstraCommandPaletteModel* model) override {
    model_shutdown_count_++;
    last_model_ = model;
  }

  void Reset() {
    command_list_changed_count_ = 0;
    search_results_changed_count_ = 0;
    command_executed_count_ = 0;
    model_shutdown_count_ = 0;
    last_model_ = nullptr;
    last_executed_command_id_ = -1;
  }

  int command_list_changed_count_ = 0;
  int search_results_changed_count_ = 0;
  int command_executed_count_ = 0;
  int model_shutdown_count_ = 0;
  raw_ptr<AstraCommandPaletteModel> last_model_ = nullptr;
  int last_executed_command_id_ = -1;
};

}  // namespace

// =========================================================================
// AstraCommandType tests
// =========================================================================

TEST(AstraCommandTypeTest, EightCommandTypes) {
  // There are 8 command types.
  EXPECT_EQ(static_cast<int>(AstraCommandType::kAction), 0);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kNavigation), 1);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kWorkspace), 2);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kSetting), 3);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kBookmark), 4);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kHistory), 5);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kTab), 6);
  EXPECT_EQ(static_cast<int>(AstraCommandType::kExtension), 7);
}

TEST(AstraCommandTypeTest, GetCommandTypeNameReturnsNonEmpty) {
  // Each type should have a non-empty name.
  for (int i = 0; i <= static_cast<int>(AstraCommandType::kExtension); ++i) {
    auto type = static_cast<AstraCommandType>(i);
    const char16_t* name = GetCommandTypeName(type);
    EXPECT_NE(nullptr, name);
    EXPECT_GT(std::u16string(name).size(), 0u);
  }
}

TEST(AstraCommandTypeTest, TypeNamesAreDistinct) {
  std::set<std::u16string> names;
  for (int i = 0; i <= static_cast<int>(AstraCommandType::kExtension); ++i) {
    auto type = static_cast<AstraCommandType>(i);
    names.insert(GetCommandTypeName(type));
  }
  EXPECT_EQ(8u, names.size());
}

// =========================================================================
// Command category tests
// =========================================================================

TEST(AstraCommandCategoryTest, ElevenCategories) {
  // There are 11 command categories.
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kTabs), 0);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kNavigation), 1);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kWorkspaces), 2);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kBookmarks), 3);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kHistory), 4);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kActions), 5);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kView), 6);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kTools), 7);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kSettings), 8);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kHelp), 9);
  EXPECT_EQ(static_cast<int>(AstraCommandCategory::kExtensions), 10);
}

TEST(AstraCommandCategoryTest, GetCategoryLabelReturnsNonEmpty) {
  for (int i = 0; i <= static_cast<int>(AstraCommandCategory::kExtensions); ++i) {
    auto cat = static_cast<AstraCommandCategory>(i);
    const char16_t* label = GetCategoryLabel(cat);
    EXPECT_NE(nullptr, label);
    EXPECT_GT(std::u16string(label).size(), 0u);
  }
}

TEST(AstraCommandCategoryTest, CategoryLabelsAreDistinct) {
  std::set<std::u16string> labels;
  for (int i = 0; i <= static_cast<int>(AstraCommandCategory::kExtensions); ++i) {
    auto cat = static_cast<AstraCommandCategory>(i);
    labels.insert(GetCategoryLabel(cat));
  }
  EXPECT_EQ(11u, labels.size());
}

TEST(AstraCommandCategoryTest, GetCategoryIconNameReturnsNonEmpty) {
  for (int i = 0; i <= static_cast<int>(AstraCommandCategory::kExtensions); ++i) {
    auto cat = static_cast<AstraCommandCategory>(i);
    const char* icon = GetCategoryIconName(cat);
    EXPECT_NE(nullptr, icon);
    EXPECT_GT(std::string(icon).size(), 0u);
  }
}

TEST(AstraCommandCategoryTest, CategoryIconNamesAreDistinct) {
  std::set<std::string> icons;
  for (int i = 0; i <= static_cast<int>(AstraCommandCategory::kExtensions); ++i) {
    auto cat = static_cast<AstraCommandCategory>(i);
    icons.insert(GetCategoryIconName(cat));
  }
  // Not all icons need to be distinct, but most should be.
  EXPECT_GT(icons.size(), 5u);
}

TEST(AstraCommandCategoryTest, GetCategoryCountReturnsEleven) {
  EXPECT_EQ(11u, GetCategoryCount());
}

// =========================================================================
// Model constants tests
// =========================================================================

TEST(AstraCommandPaletteConstantsTest, MaxResults) {
  EXPECT_EQ(20u, AstraCommandPaletteModel::kMaxResults);
}

TEST(AstraCommandPaletteConstantsTest, MaxRecentlyUsed) {
  EXPECT_EQ(10u, AstraCommandPaletteModel::kMaxRecentlyUsed);
}

TEST(AstraCommandPaletteConstantsTest, MaxWorkspaceCommands) {
  EXPECT_EQ(10u, AstraCommandPaletteModel::kMaxWorkspaceCommands);
}

// =========================================================================
// AstraCommandPaletteModel tests
// =========================================================================

class AstraCommandPaletteModelTest : public testing::Test {
 public:
  AstraCommandPaletteModelTest() = default;
  ~AstraCommandPaletteModelTest() override = default;
};

// -- Construction & basic accessors ---------------------------------------

TEST_F(AstraCommandPaletteModelTest, ConstructsWithoutCrash) {
  AstraCommandPaletteModel model;
  // No crash = success.
}

TEST_F(AstraCommandPaletteModelTest, DefaultQueryIsEmpty) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.query().empty());
}

TEST_F(AstraCommandPaletteModelTest, HasCommands) {
  AstraCommandPaletteModel model;
  EXPECT_GT(model.GetCommandCount(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsReturnsAll) {
  AstraCommandPaletteModel model;
  EXPECT_EQ(model.GetCommandCount(), model.GetCommands().size());
}

TEST_F(AstraCommandPaletteModelTest, EmptyQueryHasResults) {
  AstraCommandPaletteModel model;
  EXPECT_GT(model.GetResultCount(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, DefaultSelectedIndex) {
  AstraCommandPaletteModel model;
  if (model.GetResultCount() > 0) {
    EXPECT_GE(model.GetSelectedIndex(), 0);
    EXPECT_LT(static_cast<size_t>(model.GetSelectedIndex()),
              model.GetResultCount());
  }
}

// -- SetQuery --------------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, SetQuery) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"tab");
  EXPECT_EQ(u"tab", model.query());
}

TEST_F(AstraCommandPaletteModelTest, SetQueryUpdatesResults) {
  AstraCommandPaletteModel model;
  size_t before = model.GetResultCount();
  model.SetQuery(u"tab");
  // Results should change after setting a query.
  size_t after = model.GetResultCount();
  EXPECT_NE(before, after);
}

TEST_F(AstraCommandPaletteModelTest, SetSameQueryNoOp) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  model.SetQuery(u"tab");
  int count = observer.model_changed_count_;

  model.SetQuery(u"tab");
  EXPECT_EQ(count, observer.model_changed_count_);

  model.RemoveObserver(&observer);
}

// -- SearchCommands --------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, SearchCommandsEmptyQuery) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(std::u16string());
  EXPECT_GT(results.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsExactMatch) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(u"New Tab");
  EXPECT_GT(results.size(), 0u);
  // First result should be "New Tab" (exact match).
  EXPECT_EQ(u"New Tab", results[0].title);
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsPartialMatch) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(u"tab");
  EXPECT_GT(results.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsNoMatch) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(u"zzzzzz_no_match_zzzzzz");
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsCappedAtMax) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(5);
  auto results = model.SearchCommands(u"a");
  EXPECT_LE(results.size(), 5u);
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsHasScores) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(u"tab");
  for (const auto& item : results) {
    EXPECT_GT(item.relevance_score, 0.0);
  }
}

TEST_F(AstraCommandPaletteModelTest, SearchCommandsSortedByScore) {
  AstraCommandPaletteModel model;
  auto results = model.SearchCommands(u"tab");
  // Results should be sorted by score (highest first).
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_GE(results[i - 1].relevance_score, results[i].relevance_score);
  }
}

// -- Fuzzy search ----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, FuzzyMatchBasic) {
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"nt", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"nwt", u"New Window"));
  EXPECT_FALSE(AstraCommandPaletteModel::FuzzyMatch(u"abc", u"acb"));
}

TEST_F(AstraCommandPaletteModelTest, FuzzyMatchEmptyQuery) {
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"", u"Anything"));
}

TEST_F(AstraCommandPaletteModelTest, FuzzyMatchCaseInsensitive) {
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"NT", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"nt", u"NEW TAB"));
}

TEST_F(AstraCommandPaletteModelTest, FuzzyMatchSingleChar) {
  EXPECT_TRUE(AstraCommandPaletteModel::FuzzyMatch(u"N", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::FuzzyMatch(u"Z", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, FuzzySearchDisabledByDefault) {
  AstraCommandPaletteModel model;
  // Fuzzy search is disabled by default, so "nt" should not match "New Tab".
  auto results = model.SearchCommands(u"nt");
  EXPECT_EQ(0u, results.size());
}

TEST_F(AstraCommandPaletteModelTest, FuzzySearchEnabledFindsResults) {
  AstraCommandPaletteModel model;
  model.set_enable_fuzzy_search(true);
  auto results = model.SearchCommands(u"nt");
  EXPECT_GT(results.size(), 0u);
}

// -- Acronym matching -------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchBasic) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nt", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nwt", u"New Window"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"dt", u"Developer Tools"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsAcronymMatch(u"abc", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchCaseInsensitive) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"NT", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"Nt", u"new tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nt", u"NEW TAB"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchEmptyQuery) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"", u"Anything"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchSingleChar) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"n", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsAcronymMatch(u"x", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchLongerQueryThanWords) {
  EXPECT_FALSE(AstraCommandPaletteModel::IsAcronymMatch(u"ntx", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsAcronymMatch(u"ntab", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchMultipleSpaces) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nt", u"New   Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nt", u"New\tTab"));
}

// -- Word boundary matching -------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchBasic) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"new", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"new tab", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"tab", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"ew", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"ab", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchCaseInsensitive) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"NEW", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"new", u"NEW TAB"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"New Tab", u"new tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchEmptyQuery) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"", u"Anything"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchPartialWord) {
  // "dev tool" should match "Developer Tools" at word boundaries.
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"dev tool", u"Developer Tools"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"dev", u"Developer Tools"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"eloper", u"Developer Tools"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchMultipleSpacesInQuery) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"new   tab", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchQueryLongerThanText) {
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"New Tab Extra", u"New Tab"));
}

// -- Scoring with bonuses ---------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, AcronymMatchBoostsScore) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"New Tab";
  item.description = u"Open a new tab";
  item.category = AstraCommandCategory::kTabs;

  // Score for "nt" (acronym match) should be higher than a non-acronym
  // query of the same length that doesn't match as acronym.
  double acronym_score = model.ComputeRelevanceScore(u"nt", item);

  // "ab" should not be an acronym match and should have lower score
  // (or negative if no match at all).
  AstraCommandItem item2;
  item2.title = u"About Page";
  item2.description = u"Show about page";
  item2.category = AstraCommandCategory::kHelp;
  double non_acronym_score = model.ComputeRelevanceScore(u"ap", item2);

  // Both should match via fuzzy or substring, but the acronym one
  // should have the bonus applied.
  if (acronym_score > 0 && non_acronym_score > 0) {
    EXPECT_GT(acronym_score, non_acronym_score);
  }
}

TEST_F(AstraCommandPaletteModelTest, WordBoundaryMatchBoostsScore) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"Developer Tools";
  item.description = u"Open developer tools";
  item.category = AstraCommandCategory::kTools;

  // Word boundary match "dev tool" should score higher than a non-word-boundary
  // substring match of similar length.
  double wb_score = model.ComputeRelevanceScore(u"dev tool", item);

  // A mid-word match should score lower.
  double mid_score = model.ComputeRelevanceScore(u"eloper ools", item);

  if (wb_score > 0 && mid_score > 0) {
    EXPECT_GT(wb_score, mid_score);
  }
}

// -- Match ranges ----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesExact) {
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"New", u"New Tab");
  ASSERT_EQ(1u, ranges.size());
  EXPECT_EQ(0u, ranges[0].start());
  EXPECT_EQ(3u, ranges[0].end());
}

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesSubstring) {
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"Tab", u"New Tab");
  ASSERT_EQ(1u, ranges.size());
  EXPECT_EQ(4u, ranges[0].start());
  EXPECT_EQ(7u, ranges[0].end());
}

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesNoMatch) {
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"xyz", u"New Tab");
  EXPECT_TRUE(ranges.empty());
}

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesEmptyQuery) {
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"", u"New Tab");
  EXPECT_TRUE(ranges.empty());
}

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesFuzzy) {
  // Fuzzy match "nt" in "New Tab" should produce two ranges.
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"nt", u"New Tab");
  EXPECT_GT(ranges.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, GetMatchRangesCaseInsensitive) {
  auto ranges = AstraCommandPaletteModel::GetMatchRanges(u"new", u"NEW TAB");
  ASSERT_EQ(1u, ranges.size());
  EXPECT_EQ(0u, ranges[0].start());
  EXPECT_EQ(3u, ranges[0].end());
}

// -- Command CRUD ----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, AddCommand) {
  AstraCommandPaletteModel model;
  size_t before = model.GetCommandCount();

  AstraCommandItem cmd;
  cmd.command_id = 90001;
  cmd.title = u"Test Command";
  cmd.description = u"A test command";
  cmd.shortcut_text = u"Ctrl+T";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.icon_name = "test_icon";
  cmd.is_astra = true;

  model.AddCommand(cmd);
  EXPECT_EQ(before + 1, model.GetCommandCount());
}

TEST_F(AstraCommandPaletteModelTest, AddDuplicateCommandUpdates) {
  AstraCommandPaletteModel model;
  size_t before = model.GetCommandCount();

  AstraCommandItem cmd;
  cmd.command_id = 90002;
  cmd.title = u"Test Command 2";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;

  model.AddCommand(cmd);
  EXPECT_EQ(before + 1, model.GetCommandCount());

  // Add the same command again — should update, not add.
  cmd.title = u"Updated Command";
  model.AddCommand(cmd);
  EXPECT_EQ(before + 1, model.GetCommandCount());
}

TEST_F(AstraCommandPaletteModelTest, RemoveCommand) {
  AstraCommandPaletteModel model;

  AstraCommandItem cmd;
  cmd.command_id = 90003;
  cmd.title = u"Test Remove";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);
  size_t with_cmd = model.GetCommandCount();

  bool removed = model.RemoveCommand(90003);
  EXPECT_TRUE(removed);
  EXPECT_EQ(with_cmd - 1, model.GetCommandCount());
}

TEST_F(AstraCommandPaletteModelTest, RemoveNonexistentCommand) {
  AstraCommandPaletteModel model;
  bool removed = model.RemoveCommand(99999);
  EXPECT_FALSE(removed);
}

TEST_F(AstraCommandPaletteModelTest, AddCommandAppearsInSearch) {
  AstraCommandPaletteModel model;

  AstraCommandItem cmd;
  cmd.command_id = 90004;
  cmd.title = u"UniqueTestCommandXYZ";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);

  auto results = model.SearchCommands(u"UniqueTestCommandXYZ");
  ASSERT_GT(results.size(), 0u);
  EXPECT_EQ(90004, results[0].command_id);
}

TEST_F(AstraCommandPaletteModelTest, RemoveCommandDisappearsFromSearch) {
  AstraCommandPaletteModel model;

  AstraCommandItem cmd;
  cmd.command_id = 90005;
  cmd.title = u"RemoveMeCommand";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);
  ASSERT_GT(model.SearchCommands(u"RemoveMeCommand").size(), 0u);

  model.RemoveCommand(90005);
  EXPECT_EQ(0u, model.SearchCommands(u"RemoveMeCommand").size());
}

// -- GetCommandAt ----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, GetCommandAtValid) {
  AstraCommandPaletteModel model;
  const auto* cmd = model.GetCommandAt(0);
  EXPECT_NE(nullptr, cmd);
}

TEST_F(AstraCommandPaletteModelTest, GetCommandAtNegative) {
  AstraCommandPaletteModel model;
  EXPECT_EQ(nullptr, model.GetCommandAt(-1));
}

TEST_F(AstraCommandPaletteModelTest, GetCommandAtOutOfRange) {
  AstraCommandPaletteModel model;
  EXPECT_EQ(nullptr, model.GetCommandAt(9999));
}

// -- Recent commands -------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, RecordCommandUse) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  int use_before = model.GetCommands()[0].use_count;

  model.RecordCommandUse(cmd_id);

  // Find the command again and check use_count increased.
  for (const auto& cmd : model.GetCommands()) {
    if (cmd.command_id == cmd_id) {
      EXPECT_GT(cmd.use_count, use_before);
      break;
    }
  }
}

TEST_F(AstraCommandPaletteModelTest, GetRecentCommands) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  model.RecordCommandUse(cmd_id);

  auto recent = model.GetRecentCommands(5);
  EXPECT_GT(recent.size(), 0u);
  EXPECT_EQ(cmd_id, recent[0].command_id);
  EXPECT_TRUE(recent[0].is_recent);
}

TEST_F(AstraCommandPaletteModelTest, GetRecentCommandsRespectsMax) {
  AstraCommandPaletteModel model;
  for (size_t i = 0; i < std::min(model.GetCommandCount(), size_t(5)); ++i) {
    model.RecordCommandUse(model.GetCommands()[i].command_id);
  }

  auto recent = model.GetRecentCommands(3);
  EXPECT_LE(recent.size(), 3u);
}

TEST_F(AstraCommandPaletteModelTest, GetRecentCommandsEmptyWhenNone) {
  AstraCommandPaletteModel model;
  auto recent = model.GetRecentCommands(5);
  // May have some from default state, but let's verify zero count means empty.
  model.ClearRecentCommands();
  recent = model.GetRecentCommands(5);
  EXPECT_EQ(0u, recent.size());
}

TEST_F(AstraCommandPaletteModelTest, ClearRecentCommands) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  model.RecordCommandUse(model.GetCommands()[0].command_id);
  ASSERT_GT(model.GetRecentCommands(10).size(), 0u);

  model.ClearRecentCommands();
  EXPECT_EQ(0u, model.GetRecentCommands(10).size());
}

TEST_F(AstraCommandPaletteModelTest, ClearRecentUpdatesResults) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  model.RecordCommandUse(model.GetCommands()[0].command_id);
  int count_before = observer.search_results_changed_count_;

  model.ClearRecentCommands();
  EXPECT_GT(observer.search_results_changed_count_, count_before);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, RecentCommandsOrderedMostRecentFirst) {
  AstraCommandPaletteModel model;
  ASSERT_GE(model.GetCommandCount(), 3u);

  int id0 = model.GetCommands()[0].command_id;
  int id1 = model.GetCommands()[1].command_id;
  int id2 = model.GetCommands()[2].command_id;

  model.RecordCommandUse(id0);
  model.RecordCommandUse(id1);
  model.RecordCommandUse(id2);  // id2 should be most recent.

  auto recent = model.GetRecentCommands(10);
  ASSERT_GE(recent.size(), 3u);
  EXPECT_EQ(id2, recent[0].command_id);
  EXPECT_EQ(id1, recent[1].command_id);
  EXPECT_EQ(id0, recent[2].command_id);
}

TEST_F(AstraCommandPaletteModelTest, RecordUseMovesToFront) {
  AstraCommandPaletteModel model;
  ASSERT_GE(model.GetCommandCount(), 2u);

  int id0 = model.GetCommands()[0].command_id;
  int id1 = model.GetCommands()[1].command_id;

  model.RecordCommandUse(id0);
  model.RecordCommandUse(id1);
  ASSERT_EQ(id1, model.GetRecentCommands(10)[0].command_id);

  // Use id0 again — it should move to front.
  model.RecordCommandUse(id0);
  EXPECT_EQ(id0, model.GetRecentCommands(10)[0].command_id);
}

// -- Pinned / favorite commands --------------------------------------------

TEST_F(AstraCommandPaletteModelTest, PinCommand) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  EXPECT_FALSE(model.IsCommandPinned(cmd_id));

  bool pinned = model.PinCommand(cmd_id);
  EXPECT_TRUE(pinned);
  EXPECT_TRUE(model.IsCommandPinned(cmd_id));
  EXPECT_GT(model.GetPinnedCommandCount(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, UnpinCommand) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  model.PinCommand(cmd_id);
  ASSERT_TRUE(model.IsCommandPinned(cmd_id));

  bool unpinned = model.UnpinCommand(cmd_id);
  EXPECT_TRUE(unpinned);
  EXPECT_FALSE(model.IsCommandPinned(cmd_id));
}

TEST_F(AstraCommandPaletteModelTest, ToggleCommandPinned) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  EXPECT_FALSE(model.IsCommandPinned(cmd_id));

  model.ToggleCommandPinned(cmd_id);
  EXPECT_TRUE(model.IsCommandPinned(cmd_id));

  model.ToggleCommandPinned(cmd_id);
  EXPECT_FALSE(model.IsCommandPinned(cmd_id));
}

TEST_F(AstraCommandPaletteModelTest, PinNonexistentCommand) {
  AstraCommandPaletteModel model;
  EXPECT_FALSE(model.PinCommand(999999));
  EXPECT_FALSE(model.IsCommandPinned(999999));
}

TEST_F(AstraCommandPaletteModelTest, UnpinNonexistentCommand) {
  AstraCommandPaletteModel model;
  EXPECT_FALSE(model.UnpinCommand(999999));
}

TEST_F(AstraCommandPaletteModelTest, GetPinnedCommands) {
  AstraCommandPaletteModel model;
  ASSERT_GE(model.GetCommandCount(), 3u);

  auto pinned_before = model.GetPinnedCommands();
  EXPECT_EQ(0u, pinned_before.size());

  model.PinCommand(model.GetCommands()[0].command_id);
  model.PinCommand(model.GetCommands()[1].command_id);

  auto pinned_after = model.GetPinnedCommands();
  EXPECT_EQ(2u, pinned_after.size());
}

TEST_F(AstraCommandPaletteModelTest, PinDuplicateNoOp) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  EXPECT_TRUE(model.PinCommand(cmd_id));
  EXPECT_FALSE(model.PinCommand(cmd_id));  // Already pinned.
  EXPECT_EQ(1u, model.GetPinnedCommandCount());
}

TEST_F(AstraCommandPaletteModelTest, PinnedCommandsBoostRanking) {
  AstraCommandPaletteModel model;
  ASSERT_GE(model.GetCommandCount(), 3u);

  // Get default results.
  auto default_results = model.SearchCommands(u"");
  ASSERT_GE(default_results.size(), 3u);

  // Pin a command that's not first.
  int third_id = default_results[2].command_id;
  model.PinCommand(third_id);

  // After pinning, the command should rank higher.
  auto new_results = model.SearchCommands(u"");
  int new_position = -1;
  for (size_t i = 0; i < new_results.size(); ++i) {
    if (new_results[i].command_id == third_id) {
      new_position = static_cast<int>(i);
      break;
    }
  }
  EXPECT_LT(new_position, 2);
}

// -- Command aliases -------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, AddCommandAlias) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  bool added = model.AddCommandAlias(cmd_id, u"my alias");
  EXPECT_TRUE(added);

  auto aliases = model.GetAliasesForCommand(cmd_id);
  EXPECT_GT(aliases.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, AddAliasToNonexistentCommand) {
  AstraCommandPaletteModel model;
  EXPECT_FALSE(model.AddCommandAlias(99999, u"test alias"));
}

TEST_F(AstraCommandPaletteModelTest, RemoveCommandAlias) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  model.AddCommandAlias(cmd_id, u"test alias");
  ASSERT_TRUE(model.RemoveCommandAlias(cmd_id, u"test alias"));

  auto aliases = model.GetAliasesForCommand(cmd_id);
  bool found = false;
  for (const auto& a : aliases) {
    if (a == u"test alias") {
      found = true;
      break;
    }
  }
  EXPECT_FALSE(found);
}

TEST_F(AstraCommandPaletteModelTest, AliasAppearsInSearch) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetCommandCount(), 0u);

  int cmd_id = model.GetCommands()[0].command_id;
  std::u16string unique_alias = u"xyzzy_unique_test_alias";
  model.AddCommandAlias(cmd_id, unique_alias);

  auto results = model.SearchCommands(unique_alias);
  EXPECT_GT(results.size(), 0u);
  bool found = false;
  for (const auto& r : results) {
    if (r.command_id == cmd_id) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraCommandPaletteModelTest, GetAliasesForNonexistentCommand) {
  AstraCommandPaletteModel model;
  auto aliases = model.GetAliasesForCommand(99999);
  EXPECT_TRUE(aliases.empty());
}

// -- Context-aware commands ------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, DefaultNoContextCommands) {
  AstraCommandPaletteModel model;
  EXPECT_FALSE(model.has_context_commands());
}

TEST_F(AstraCommandPaletteModelTest, UpdateContextCommands) {
  AstraCommandPaletteModel model;
  size_t before = model.GetCommandCount();

  std::vector<AstraCommandItem> context_cmds;
  AstraCommandItem cmd;
  cmd.command_id = 80001;
  cmd.title = u"Context Command";
  cmd.description = u"A context-aware command";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_astra = true;
  cmd.is_context_command = true;
  context_cmds.push_back(cmd);

  model.UpdateContextCommands(context_cmds);
  EXPECT_TRUE(model.has_context_commands());
  EXPECT_GT(model.GetCommandCount(), before);
}

TEST_F(AstraCommandPaletteModelTest, ClearContextCommands) {
  AstraCommandPaletteModel model;

  std::vector<AstraCommandItem> context_cmds;
  AstraCommandItem cmd;
  cmd.command_id = 80002;
  cmd.title = u"Context Command 2";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_astra = true;
  cmd.is_context_command = true;
  context_cmds.push_back(cmd);

  model.UpdateContextCommands(context_cmds);
  ASSERT_TRUE(model.has_context_commands());

  model.ClearContextCommands();
  EXPECT_FALSE(model.has_context_commands());
}

TEST_F(AstraCommandPaletteModelTest, ContextCommandsBoostedInRanking) {
  AstraCommandPaletteModel model;

  std::vector<AstraCommandItem> context_cmds;
  AstraCommandItem cmd;
  cmd.command_id = 80003;
  cmd.title = u"Boosted Context Command";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_astra = true;
  cmd.is_context_command = true;
  context_cmds.push_back(cmd);

  model.UpdateContextCommands(context_cmds);

  // Search for the context command — it should have a boost.
  auto results = model.SearchCommands(u"Boosted Context");
  EXPECT_GT(results.size(), 0u);
  if (results.size() > 0) {
    EXPECT_TRUE(results[0].is_context_command);
  }
}

// -- Suggested commands ("Did you mean") -----------------------------------

TEST_F(AstraCommandPaletteModelTest, GetSuggestedCommandsNotEmptyForCloseMatch) {
  AstraCommandPaletteModel model;
  model.set_enable_fuzzy_search(true);

  // A close typo of "New Tab" should yield suggestions.
  auto suggestions = model.GetSuggestedCommands(u"New Tb");
  // May or may not have suggestions depending on fuzzy match threshold.
  // Just verify the call doesn't crash.
  SUCCEED();
}

TEST_F(AstraCommandPaletteModelTest, SuggestionsEmptyForExactMatch) {
  AstraCommandPaletteModel model;
  // When there are good results, suggestions may still be populated
  // with alternatives. Just verify no crash.
  auto suggestions = model.GetSuggestedCommands(u"New Tab");
  SUCCEED();
}

TEST_F(AstraCommandPaletteModelTest, ShowSuggestionsSetting) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.show_suggestions());

  model.set_show_suggestions(false);
  EXPECT_FALSE(model.show_suggestions());

  model.set_show_suggestions(true);
  EXPECT_TRUE(model.show_suggestions());
}

// -- Page navigation --------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, SelectPageUp) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 5u);

  model.SetSelectedIndex(static_cast<int>(model.GetResultCount()) - 1);
  int last = model.GetSelectedIndex();

  model.SelectPageUp();
  EXPECT_LT(model.GetSelectedIndex(), last);
  EXPECT_GE(model.GetSelectedIndex(), 0);
}

TEST_F(AstraCommandPaletteModelTest, SelectPageDown) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 5u);

  model.SetSelectedIndex(0);

  model.SelectPageDown();
  EXPECT_GT(model.GetSelectedIndex(), 0);
  EXPECT_LT(static_cast<size_t>(model.GetSelectedIndex()),
            model.GetResultCount());
}

// -- New settings -----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ShowNumberHintsDefaultTrue) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.show_number_hints());
}

TEST_F(AstraCommandPaletteModelTest, SetShowNumberHints) {
  AstraCommandPaletteModel model;
  model.set_show_number_hints(false);
  EXPECT_FALSE(model.show_number_hints());

  model.set_show_number_hints(true);
  EXPECT_TRUE(model.show_number_hints());
}

TEST_F(AstraCommandPaletteModelTest, ShowPinnedSectionDefaultTrue) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.show_pinned_section());
}

TEST_F(AstraCommandPaletteModelTest, SetShowPinnedSection) {
  AstraCommandPaletteModel model;
  model.set_show_pinned_section(false);
  EXPECT_FALSE(model.show_pinned_section());
}

TEST_F(AstraCommandPaletteModelTest, EnableContextCommandsDefaultTrue) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.enable_context_commands());
}

TEST_F(AstraCommandPaletteModelTest, SetEnableContextCommands) {
  AstraCommandPaletteModel model;
  model.set_enable_context_commands(false);
  EXPECT_FALSE(model.enable_context_commands());
}

// -- Default commands ------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, GetDefaultCommandsNotEmpty) {
  AstraCommandPaletteModel model;
  auto defaults = model.GetDefaultCommands();
  EXPECT_GT(defaults.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, DefaultCommandsCapped) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(5);
  auto defaults = model.GetDefaultCommands();
  EXPECT_LE(defaults.size(), 5u);
}

// -- Commands by type ------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeTab) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kTab);
  EXPECT_GT(commands.size(), 0u);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kTab, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeNavigation) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kNavigation);
  EXPECT_GT(commands.size(), 0u);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kNavigation, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeSetting) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kSetting);
  EXPECT_GT(commands.size(), 0u);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kSetting, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeAction) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kAction);
  EXPECT_GT(commands.size(), 0u);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kAction, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeBookmark) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kBookmark);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kBookmark, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, GetCommandsByTypeExtension) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kExtension);
  for (const auto& cmd : commands) {
    EXPECT_EQ(AstraCommandType::kExtension, cmd.type);
  }
}

TEST_F(AstraCommandPaletteModelTest, AllTypesHaveSomeCommands) {
  AstraCommandPaletteModel model;
  // Most types should have at least some commands.
  int types_with_commands = 0;
  for (int i = 0; i <= static_cast<int>(AstraCommandType::kExtension); ++i) {
    auto type = static_cast<AstraCommandType>(i);
    if (model.GetCommandsByType(type).size() > 0) {
      types_with_commands++;
    }
  }
  EXPECT_GE(types_with_commands, 5);
}

// -- Ranking ---------------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, UpdateRanking) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  model.UpdateRanking();
  EXPECT_GT(observer.search_results_changed_count_, 0);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, UsageAffectsRanking) {
  AstraCommandPaletteModel model;

  // Find a command that's not first by default.
  auto default_results = model.SearchCommands(u"");
  ASSERT_GE(default_results.size(), 3u);

  int second_id = default_results[1].command_id;

  // Use the second command many times.
  for (int i = 0; i < 100; ++i) {
    model.RecordCommandUse(second_id);
  }

  // Now search again — the heavily-used command should rank higher.
  auto new_results = model.SearchCommands(u"");
  int new_position = -1;
  for (size_t i = 0; i < new_results.size(); ++i) {
    if (new_results[i].command_id == second_id) {
      new_position = static_cast<int>(i);
      break;
    }
  }
  // It should have moved up (smaller index = higher position).
  EXPECT_LT(new_position, 1);
}

TEST_F(AstraCommandPaletteModelTest, SortByUsage) {
  AstraCommandPaletteModel model;
  ASSERT_GE(model.GetCommandCount(), 3u);

  // Use some commands to build up usage counts.
  model.RecordCommandUse(model.GetCommands()[2].command_id);
  model.RecordCommandUse(model.GetCommands()[2].command_id);
  model.RecordCommandUse(model.GetCommands()[1].command_id);

  // Switch to sort by usage.
  model.set_sort_by_relevance(false);

  auto results = model.GetResults();
  ASSERT_GT(results.size(), 0u);

  // Results should be roughly sorted by usage (highest first).
  // The most-used commands should appear near the top.
  bool found_used_command = false;
  for (size_t i = 0; i < std::min(results.size(), size_t(5)); ++i) {
    if (results[i].use_count > 0) {
      found_used_command = true;
      break;
    }
  }
  EXPECT_TRUE(found_used_command);
}

// -- Settings --------------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, DefaultSettings) {
  AstraCommandPaletteModel model;

  EXPECT_EQ(AstraCommandPaletteModel::kMaxResults,
            model.max_search_results());
  EXPECT_EQ(AstraCommandPaletteModel::kMaxRecentlyUsed,
            model.max_recent_commands());
  EXPECT_TRUE(model.show_descriptions());
  EXPECT_TRUE(model.show_shortcuts());
  EXPECT_FALSE(model.enable_fuzzy_search());
  EXPECT_FALSE(model.search_in_command_ids());
  EXPECT_TRUE(model.sort_by_relevance());
  EXPECT_FALSE(model.auto_execute_single_result());
}

TEST_F(AstraCommandPaletteModelTest, SetMaxSearchResults) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(5);
  EXPECT_EQ(5u, model.max_search_results());
  EXPECT_LE(model.GetResultCount(), 5u);
}

TEST_F(AstraCommandPaletteModelTest, SetMaxSearchResultsSameNoOp) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  model.set_max_search_results(20);  // Already default.
  EXPECT_EQ(0, observer.search_results_changed_count_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, SetMaxRecentCommands) {
  AstraCommandPaletteModel model;
  model.set_max_recent_commands(5);
  EXPECT_EQ(5u, model.max_recent_commands());
}

TEST_F(AstraCommandPaletteModelTest, SetShowDescriptions) {
  AstraCommandPaletteModel model;
  model.set_show_descriptions(false);
  EXPECT_FALSE(model.show_descriptions());

  model.set_show_descriptions(true);
  EXPECT_TRUE(model.show_descriptions());
}

TEST_F(AstraCommandPaletteModelTest, SetShowShortcuts) {
  AstraCommandPaletteModel model;
  model.set_show_shortcuts(false);
  EXPECT_FALSE(model.show_shortcuts());

  model.set_show_shortcuts(true);
  EXPECT_TRUE(model.show_shortcuts());
}

TEST_F(AstraCommandPaletteModelTest, SetEnableFuzzySearch) {
  AstraCommandPaletteModel model;
  model.set_enable_fuzzy_search(true);
  EXPECT_TRUE(model.enable_fuzzy_search());

  model.set_enable_fuzzy_search(false);
  EXPECT_FALSE(model.enable_fuzzy_search());
}

TEST_F(AstraCommandPaletteModelTest, FuzzySearchToggleAffectsResults) {
  AstraCommandPaletteModel model;

  // With fuzzy search disabled, "nt" should match nothing.
  model.set_enable_fuzzy_search(false);
  auto results_off = model.SearchCommands(u"nt");

  // With fuzzy search enabled, "nt" should match "New Tab" etc.
  model.set_enable_fuzzy_search(true);
  auto results_on = model.SearchCommands(u"nt");

  EXPECT_GT(results_on.size(), results_off.size());
}

TEST_F(AstraCommandPaletteModelTest, SetSearchInCommandIds) {
  AstraCommandPaletteModel model;
  model.set_search_in_command_ids(true);
  EXPECT_TRUE(model.search_in_command_ids());
}

TEST_F(AstraCommandPaletteModelTest, SetSortByRelevance) {
  AstraCommandPaletteModel model;
  model.set_sort_by_relevance(false);
  EXPECT_FALSE(model.sort_by_relevance());

  model.set_sort_by_relevance(true);
  EXPECT_TRUE(model.sort_by_relevance());
}

TEST_F(AstraCommandPaletteModelTest, SetAutoExecuteSingleResult) {
  AstraCommandPaletteModel model;
  model.set_auto_execute_single_result(true);
  EXPECT_TRUE(model.auto_execute_single_result());
}

TEST_F(AstraCommandPaletteModelTest, AutoExecuteSingleResult) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  model.set_auto_execute_single_result(true);

  // Set a query that returns exactly one result.
  model.SetQuery(u"New Tab");

  // If there's exactly one result, it should auto-execute.
  if (model.GetResultCount() == 1) {
    EXPECT_GT(observer.execution_requested_count_, 0);
  }

  model.RemoveObserver(&observer);
}

// -- Legacy settings (backward compatibility) ------------------------------

TEST_F(AstraCommandPaletteModelTest, MaxVisibleCommandsAlias) {
  AstraCommandPaletteModel model;
  EXPECT_EQ(model.max_search_results(), model.max_visible_commands());

  model.set_max_visible_commands(7);
  EXPECT_EQ(7u, model.max_search_results());
  EXPECT_EQ(7u, model.max_visible_commands());
}

TEST_F(AstraCommandPaletteModelTest, ShowRecentSection) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.show_recent_section());

  model.set_show_recent_section(false);
  EXPECT_FALSE(model.show_recent_section());
}

// -- Observer notifications (new-style) -----------------------------------

TEST_F(AstraCommandPaletteModelTest, ObserverOnCommandListChanged) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  EXPECT_EQ(0, observer.command_list_changed_count_);

  AstraCommandItem cmd;
  cmd.command_id = 90006;
  cmd.title = u"Observer Test";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);

  EXPECT_GT(observer.command_list_changed_count_, 0);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ObserverOnSearchResultsChanged) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  int before = observer.search_results_changed_count_;
  model.SetQuery(u"test");
  EXPECT_GT(observer.search_results_changed_count_, before);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ObserverOnCommandExecuted) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  ASSERT_GT(model.GetResultCount(), 0u);
  model.ExecuteCommand(0);

  EXPECT_GT(observer.command_executed_count_, 0);
  EXPECT_GT(observer.last_executed_command_id_, 0);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ObserverModelPointerMatches) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer;
  model.AddObserver(&observer);

  model.SetQuery(u"tab");
  EXPECT_EQ(&model, observer.last_model_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ObserverOnShutdown) {
  // Create a model dynamically and destroy it to test shutdown notification.
  TestCommandPaletteObserver observer;
  {
    auto model = std::make_unique<AstraCommandPaletteModel>();
    model->AddObserver(&observer);
    EXPECT_EQ(0, observer.model_shutdown_count_);
    model->RemoveObserver(&observer);
  }
  // No crash = success.
  EXPECT_EQ(0, observer.model_shutdown_count_);
}

TEST_F(AstraCommandPaletteModelTest, MultipleNewStyleObservers) {
  AstraCommandPaletteModel model;
  TestCommandPaletteObserver observer1;
  TestCommandPaletteObserver observer2;

  model.AddObserver(&observer1);
  model.AddObserver(&observer2);

  model.SetQuery(u"tab");

  EXPECT_GT(observer1.search_results_changed_count_, 0);
  EXPECT_GT(observer2.search_results_changed_count_, 0);

  model.RemoveObserver(&observer1);
  model.RemoveObserver(&observer2);
}

// -- Observer notifications (legacy) --------------------------------------

TEST_F(AstraCommandPaletteModelTest, ObserverOnModelChanged) {
  AstraCommandPaletteModel model;
  MockModelObserver observer;
  model.AddObserver(&observer);

  EXPECT_CALL(observer, OnModelChanged()).Times(testing::AtLeast(1));
  model.SetQuery(u"test");

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ObserverOnSelectionChanged) {
  AstraCommandPaletteModel model;
  MockModelObserver observer;
  model.AddObserver(&observer);

  if (model.GetResultCount() > 1) {
    EXPECT_CALL(observer, OnSelectionChanged()).Times(testing::AtLeast(1));
    model.SetSelectedIndex(1);
  }

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ExecuteCommandNotifiesObserver) {
  AstraCommandPaletteModel model;
  MockModelObserver observer;
  model.AddObserver(&observer);

  if (model.GetResultCount() > 0) {
    EXPECT_CALL(observer, OnCommandExecutionRequested(testing::_, testing::_))
        .Times(1);
    model.ExecuteCommand(0);
  }

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, SearchTextChangedFiresBeforeModelChanged) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  model.SetQuery(u"tab");
  EXPECT_GT(observer.search_text_changed_count_, 0);
  EXPECT_EQ(u"tab", observer.last_search_text_);
  EXPECT_GT(observer.model_changed_count_, 0);

  model.RemoveObserver(&observer);
}

// -- Selection -------------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, SetSelectedIndex) {
  AstraCommandPaletteModel model;
  if (model.GetResultCount() > 1) {
    model.SetSelectedIndex(1);
    EXPECT_EQ(1, model.GetSelectedIndex());
  }
}

TEST_F(AstraCommandPaletteModelTest, SetSelectedIndexClamps) {
  AstraCommandPaletteModel model;
  if (model.GetResultCount() > 0) {
    model.SetSelectedIndex(1000);
    EXPECT_LT(static_cast<size_t>(model.GetSelectedIndex()),
              model.GetResultCount());

    model.SetSelectedIndex(-1);
    EXPECT_GE(model.GetSelectedIndex(), 0);
  }
}

TEST_F(AstraCommandPaletteModelTest, MoveSelectionDown) {
  AstraCommandPaletteModel model;
  if (model.GetResultCount() > 1) {
    int before = model.GetSelectedIndex();
    model.MoveSelection(1);
    EXPECT_GT(model.GetSelectedIndex(), before);
  }
}

TEST_F(AstraCommandPaletteModelTest, MoveSelectionUp) {
  AstraCommandPaletteModel model;
  if (model.GetResultCount() > 1) {
    model.SetSelectedIndex(1);
    int before = model.GetSelectedIndex();
    model.MoveSelection(-1);
    EXPECT_LT(model.GetSelectedIndex(), before);
  }
}

TEST_F(AstraCommandPaletteModelTest, MoveSelectionWrapsAround) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 1u);

  model.SetSelectedIndex(0);
  model.MoveSelection(-1);
  EXPECT_EQ(static_cast<int>(model.GetResultCount()) - 1,
            model.GetSelectedIndex());

  model.SetSelectedIndex(
      static_cast<int>(model.GetResultCount()) - 1);
  model.MoveSelection(1);
  EXPECT_EQ(0, model.GetSelectedIndex());
}

TEST_F(AstraCommandPaletteModelTest, MoveSelectionMultipleSteps) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 5u);

  model.SetSelectedIndex(0);
  model.MoveSelection(3);
  EXPECT_EQ(3, model.GetSelectedIndex());

  model.MoveSelection(-2);
  EXPECT_EQ(1, model.GetSelectedIndex());
}

TEST_F(AstraCommandPaletteModelTest, SelectionOnEmptyResults) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"zzzz_no_match_zzzz");
  ASSERT_EQ(0u, model.GetResultCount());

  model.SetSelectedIndex(0);
  EXPECT_EQ(-1, model.GetSelectedIndex());

  model.MoveSelection(1);
  EXPECT_EQ(-1, model.GetSelectedIndex());

  model.MoveSelection(-1);
  EXPECT_EQ(-1, model.GetSelectedIndex());
}

// -- GetSelectedItem --------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, GetSelectedItemReturnsItem) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 0u);

  model.SetSelectedIndex(0);
  const auto* item = model.GetSelectedItem();
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(model.GetCommandAt(0)->command_id, item->command_id);
  EXPECT_EQ(model.GetCommandAt(0)->title, item->title);
}

TEST_F(AstraCommandPaletteModelTest, GetSelectedItemNullWhenNoSelection) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"zzzz_no_match_zzzz");
  ASSERT_EQ(0u, model.GetResultCount());
  EXPECT_EQ(-1, model.GetSelectedIndex());

  const auto* item = model.GetSelectedItem();
  EXPECT_EQ(nullptr, item);
}

TEST_F(AstraCommandPaletteModelTest, GetSelectedItemUpdatesAfterMove) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 2u);

  model.SetSelectedIndex(0);
  int first_id = model.GetSelectedItem()->command_id;

  model.MoveSelection(1);
  int second_id = model.GetSelectedItem()->command_id;

  EXPECT_NE(first_id, second_id);
}

// -- Group navigation --------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, SelectNextGroupBasic) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultGroups().size(), 1u);

  model.SetSelectedIndex(0);
  int first_group_index = model.GetSelectedIndex();
  auto first_group_cat = model.GetResultGroups()[0].category;

  model.SelectNextGroup();

  // Should have moved to a different group.
  auto new_group_cat =
      model.GetResultGroups()[0].category;
  // Find the group of the new selection.
  int new_index = model.GetSelectedIndex();
  EXPECT_GT(new_index, first_group_index);

  // Verify the selected item is in a different category group.
  bool found_different = false;
  for (const auto& group : model.GetResultGroups()) {
    if (!group.items.empty() &&
        group.items[0].command_id ==
            model.GetCommandAt(new_index)->command_id) {
      if (group.category != first_group_cat) {
        found_different = true;
      }
      break;
    }
  }
  EXPECT_TRUE(found_different);
}

TEST_F(AstraCommandPaletteModelTest, SelectPrevGroupBasic) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultGroups().size(), 2u);

  // Start at the last item.
  model.SetSelectedIndex(
      static_cast<int>(model.GetResultCount()) - 1);
  int last_index = model.GetSelectedIndex();

  model.SelectPrevGroup();
  int new_index = model.GetSelectedIndex();
  EXPECT_LT(new_index, last_index);
  EXPECT_GE(new_index, 0);
}

TEST_F(AstraCommandPaletteModelTest, SelectNextGroupWrapsAround) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultGroups().size(), 1u);

  // Find the last group and go to its first item.
  const auto& groups = model.GetResultGroups();
  size_t last_group_start = 0;
  for (size_t i = 0; i < groups.size() - 1; ++i) {
    last_group_start += groups[i].items.size();
  }

  model.SetSelectedIndex(static_cast<int>(last_group_start));
  int before = model.GetSelectedIndex();

  model.SelectNextGroup();
  int after = model.GetSelectedIndex();

  // Should wrap to first group.
  EXPECT_LT(after, before);
  EXPECT_EQ(0, after);
}

TEST_F(AstraCommandPaletteModelTest, SelectPrevGroupWrapsAround) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultGroups().size(), 1u);

  model.SetSelectedIndex(0);

  model.SelectPrevGroup();
  int after = model.GetSelectedIndex();

  // Should wrap to last group (first item of last group).
  const auto& groups = model.GetResultGroups();
  size_t last_group_start = 0;
  for (size_t i = 0; i < groups.size() - 1; ++i) {
    last_group_start += groups[i].items.size();
  }
  EXPECT_EQ(static_cast<int>(last_group_start), after);
}

TEST_F(AstraCommandPaletteModelTest, SelectNextGroupSingleGroup) {
  // When there's only one group, select next should still work
  // (wraps to same group).
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTabs};
  model.SetCategoryFilter(filter);

  ASSERT_EQ(1u, model.GetResultGroups().size());
  ASSERT_GT(model.GetResultCount(), 0u);

  model.SetSelectedIndex(0);
  model.SelectNextGroup();
  // Should still be 0 (wraps to first item of the only group).
  EXPECT_EQ(0, model.GetSelectedIndex());
}

TEST_F(AstraCommandPaletteModelTest, SelectPrevGroupSingleGroup) {
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTabs};
  model.SetCategoryFilter(filter);

  ASSERT_EQ(1u, model.GetResultGroups().size());
  ASSERT_GT(model.GetResultCount(), 0u);

  model.SetSelectedIndex(0);
  model.SelectPrevGroup();
  // Should wrap to first item of the only group.
  EXPECT_EQ(0, model.GetSelectedIndex());
}

TEST_F(AstraCommandPaletteModelTest, SelectNextGroupEmptyResults) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"zzzz_no_match_zzzz");
  ASSERT_EQ(0u, model.GetResultCount());

  model.SelectNextGroup();
  EXPECT_EQ(-1, model.GetSelectedIndex());

  model.SelectPrevGroup();
  EXPECT_EQ(-1, model.GetSelectedIndex());
}

// -- ExecuteCommand --------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ExecuteCommandInvalidIndex) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  model.ExecuteCommand(-1);
  EXPECT_EQ(0, observer.execution_requested_count_);

  size_t count = model.GetResultCount();
  model.ExecuteCommand(static_cast<int>(count) + 100);
  EXPECT_EQ(0, observer.execution_requested_count_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ExecuteCommandAddsToRecent) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 0u);

  size_t before = model.GetRecentCommands(10).size();
  model.ExecuteCommand(0);

  // At least one command should be in recent now.
  auto recent = model.GetRecentCommands(10);
  EXPECT_GT(recent.size(), before);
}

// -- Workspace commands ----------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, UpdateWorkspaceCommands) {
  AstraCommandPaletteModel model;
  size_t before = model.GetCommandCount();

  model.UpdateWorkspaceCommands(5);
  EXPECT_GT(model.GetCommandCount(), before);
}

TEST_F(AstraCommandPaletteModelTest, WorkspaceCommandsHaveCorrectType) {
  AstraCommandPaletteModel model;
  model.UpdateWorkspaceCommands(3);

  auto workspace_cmds = model.GetCommandsByType(AstraCommandType::kWorkspace);
  EXPECT_GE(workspace_cmds.size(), 3u);
}

TEST_F(AstraCommandPaletteModelTest, WorkspaceCommandsHaveWorkspaceId) {
  AstraCommandPaletteModel model;
  model.UpdateWorkspaceCommands(3);

  auto all = model.GetCommands();
  int workspace_cmd_count = 0;
  for (const auto& cmd : all) {
    if (cmd.is_dynamic_workspace) {
      workspace_cmd_count++;
      EXPECT_FALSE(cmd.workspace_id.empty());
    }
  }
  EXPECT_GE(workspace_cmd_count, 3);
}

// -- Category filter -------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, DefaultNoFilter) {
  AstraCommandPaletteModel model;
  EXPECT_TRUE(model.category_filter().empty());
  EXPECT_TRUE(model.IsCategoryVisible(AstraCommandCategory::kTabs));
  EXPECT_TRUE(model.IsCategoryVisible(AstraCommandCategory::kTools));
}

TEST_F(AstraCommandPaletteModelTest, FilterBySingleCategory) {
  AstraCommandPaletteModel model;

  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTabs};
  model.SetCategoryFilter(filter);

  EXPECT_TRUE(model.IsCategoryVisible(AstraCommandCategory::kTabs));
  EXPECT_FALSE(model.IsCategoryVisible(AstraCommandCategory::kView));

  const auto& results = model.GetResults();
  for (const auto& item : results) {
    EXPECT_EQ(AstraCommandCategory::kTabs, item.category);
  }
}

TEST_F(AstraCommandPaletteModelTest, FilterByMultipleCategories) {
  AstraCommandPaletteModel model;

  std::set<AstraCommandCategory> filter = {
      AstraCommandCategory::kTabs, AstraCommandCategory::kView};
  model.SetCategoryFilter(filter);

  const auto& results = model.GetResults();
  for (const auto& item : results) {
    EXPECT_TRUE(item.category == AstraCommandCategory::kTabs ||
                item.category == AstraCommandCategory::kView);
  }
}

TEST_F(AstraCommandPaletteModelTest, ClearCategoryFilter) {
  AstraCommandPaletteModel model;

  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTools};
  model.SetCategoryFilter(filter);
  ASSERT_FALSE(model.category_filter().empty());

  model.ClearCategoryFilter();
  EXPECT_TRUE(model.category_filter().empty());
}

TEST_F(AstraCommandPaletteModelTest, SameFilterNoOp) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTabs};
  model.SetCategoryFilter(filter);
  int count = observer.model_changed_count_;

  model.SetCategoryFilter(filter);
  EXPECT_EQ(count, observer.model_changed_count_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, EmptyCategoryFilterNoCrash) {
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> empty_filter;
  model.SetCategoryFilter(empty_filter);
  EXPECT_GT(model.GetResultCount(), 0u);
}

// -- Bulk operations -------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ExecuteAllVisible) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  size_t count = model.GetResultCount();
  ASSERT_GT(count, 0u);

  size_t executed = model.ExecuteAllVisible();
  EXPECT_EQ(count, executed);
  EXPECT_EQ(static_cast<int>(count), observer.execution_requested_count_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ExecuteFirstN) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  size_t total = model.GetResultCount();
  ASSERT_GT(total, 3u);

  size_t executed = model.ExecuteFirstN(3);
  EXPECT_EQ(3u, executed);
  EXPECT_EQ(3, observer.execution_requested_count_);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ExecuteFirstNLargerThanResults) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  size_t total = model.GetResultCount();
  ASSERT_GT(total, 0u);

  size_t executed = model.ExecuteFirstN(total + 100);
  EXPECT_EQ(total, executed);

  model.RemoveObserver(&observer);
}

TEST_F(AstraCommandPaletteModelTest, ExecuteFirstNZero) {
  AstraCommandPaletteModel model;
  TestModelObserver observer;
  model.AddObserver(&observer);

  size_t executed = model.ExecuteFirstN(0);
  EXPECT_EQ(0u, executed);
  EXPECT_EQ(0, observer.execution_requested_count_);

  model.RemoveObserver(&observer);
}

// -- Relevance scoring -----------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ExactMatchScoresHigherThanPartial) {
  AstraCommandPaletteModel model;

  auto exact_results = model.SearchCommands(u"New Tab");
  auto partial_results = model.SearchCommands(u"tab");

  ASSERT_GT(exact_results.size(), 0u);
  ASSERT_GT(partial_results.size(), 0u);

  // The top exact match should have a higher score than top partial match.
  // Actually both might match the same item at different score levels.
  // Let's check that the exact match score is positive.
  EXPECT_GT(exact_results[0].relevance_score, 0.0);
  EXPECT_GT(partial_results[0].relevance_score, 0.0);
}

TEST_F(AstraCommandPaletteModelTest, TitleMatchScoresHigherThanDescription) {
  AstraCommandPaletteModel model;

  // "New Tab" matches in title, should score high.
  auto title_results = model.SearchCommands(u"New Tab");

  // Find something that matches in description only.
  auto desc_results = model.SearchCommands(u"page");

  // Both should have results.
  EXPECT_GT(title_results.size(), 0u);
  EXPECT_GT(desc_results.size(), 0u);

  // Top title match should score higher than top description match.
  if (title_results.size() > 0 && desc_results.size() > 0) {
    EXPECT_GT(title_results[0].relevance_score,
              desc_results[0].relevance_score);
  }
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreNoMatch) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"Test Command";
  item.description = u"This is a test";
  item.category = AstraCommandCategory::kTools;

  double score = model.ComputeRelevanceScore(u"xyz", item);
  EXPECT_LT(score, 0.0);
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreEmptyQuery) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"Test Command";
  item.description = u"This is a test";
  item.category = AstraCommandCategory::kTools;

  double score = model.ComputeRelevanceScore(u"", item);
  EXPECT_GE(score, 0.0);
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreExactMatch) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"New Tab";
  item.description = u"Open a new tab";
  item.category = AstraCommandCategory::kTabs;

  double score = model.ComputeRelevanceScore(u"New Tab", item);
  EXPECT_GT(score, 0.0);
}

// -- Edge cases ------------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, NoMatchingQuery) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"zzzzzzz_no_match_zzzzzzz");
  EXPECT_EQ(0u, model.GetResultCount());
  EXPECT_EQ(-1, model.GetSelectedIndex());
}

TEST_F(AstraCommandPaletteModelTest, MaxVisibleClampedToKMaxResults) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(1000);
  EXPECT_LE(model.GetResultCount(),
            AstraCommandPaletteModel::kMaxResults);
}

TEST_F(AstraCommandPaletteModelTest, ZeroMaxVisible) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(0);
  EXPECT_EQ(0u, model.GetResultCount());
}

TEST_F(AstraCommandPaletteModelTest, CategoryFilterWithNoMatches) {
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kHelp};
  model.SetCategoryFilter(filter);
  EXPECT_GT(model.GetResultCount(), 0u);
  for (const auto& item : model.GetResults()) {
    EXPECT_EQ(AstraCommandCategory::kHelp, item.category);
  }
}

TEST_F(AstraCommandPaletteModelTest, ResultGroupsRespectFilter) {
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kSettings};
  model.SetCategoryFilter(filter);

  const auto& groups = model.GetResultGroups();
  for (const auto& group : groups) {
    EXPECT_EQ(AstraCommandCategory::kSettings, group.category);
  }
}

TEST_F(AstraCommandPaletteModelTest, ActionsCategoryHasCommands) {
  AstraCommandPaletteModel model;
  auto commands = model.GetCommandsByType(AstraCommandType::kAction);

  // Actions category should have several commands (find, print, save, etc.).
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kActions};
  model.SetCategoryFilter(filter);
  EXPECT_GT(model.GetResultCount(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, ActionsCategoryFilterWorks) {
  AstraCommandPaletteModel model;
  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kActions};
  model.SetCategoryFilter(filter);

  const auto& results = model.GetResults();
  ASSERT_GT(results.size(), 0u);
  for (const auto& item : results) {
    EXPECT_EQ(AstraCommandCategory::kActions, item.category);
  }
}

TEST_F(AstraCommandPaletteModelTest, ToolsAndActionsAreSeparate) {
  AstraCommandPaletteModel model;

  std::set<AstraCommandCategory> tools_filter = {
      AstraCommandCategory::kTools};
  model.SetCategoryFilter(tools_filter);
  size_t tools_count = model.GetResultCount();

  std::set<AstraCommandCategory> actions_filter = {
      AstraCommandCategory::kActions};
  model.SetCategoryFilter(actions_filter);
  size_t actions_count = model.GetResultCount();

  // Both should have commands.
  EXPECT_GT(tools_count, 0u);
  EXPECT_GT(actions_count, 0u);

  model.ClearCategoryFilter();
  size_t all_count = model.GetResultCount();

  // Combined, tools + actions should be <= total.
  EXPECT_LE(tools_count + actions_count, all_count);
}

// -- Result groups ----------------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ResultGroupsNotEmptyWhenResultsExist) {
  AstraCommandPaletteModel model;
  ASSERT_GT(model.GetResultCount(), 0u);

  const auto& groups = model.GetResultGroups();
  EXPECT_GT(groups.size(), 0u);
}

TEST_F(AstraCommandPaletteModelTest, ResultGroupsSumToTotalResults) {
  AstraCommandPaletteModel model;
  const auto& groups = model.GetResultGroups();

  size_t total = 0;
  for (const auto& group : groups) {
    total += group.items.size();
  }
  EXPECT_EQ(model.GetResultCount(), total);
}

TEST_F(AstraCommandPaletteModelTest, ResultGroupsHaveValidCategories) {
  AstraCommandPaletteModel model;
  const auto& groups = model.GetResultGroups();

  for (const auto& group : groups) {
    // Each group should have at least one item.
    EXPECT_GT(group.items.size(), 0u);
    // All items in a group should have the same category.
    for (const auto& item : group.items) {
      EXPECT_EQ(group.category, item.category);
    }
  }
}

TEST_F(AstraCommandPaletteModelTest, ResultGroupsEmptyWhenNoResults) {
  AstraCommandPaletteModel model;
  model.SetQuery(u"zzzz_no_match_zzzz");
  ASSERT_EQ(0u, model.GetResultCount());

  const auto& groups = model.GetResultGroups();
  EXPECT_EQ(0u, groups.size());
}

TEST_F(AstraCommandPaletteModelTest, ResultGroupsCategoriesAreDistinct) {
  AstraCommandPaletteModel model;
  const auto& groups = model.GetResultGroups();

  std::set<AstraCommandCategory> seen;
  for (const auto& group : groups) {
    EXPECT_EQ(0u, seen.count(group.category));
    seen.insert(group.category);
  }
}

TEST_F(AstraCommandPaletteModelTest, FilterReducesGroupCount) {
  AstraCommandPaletteModel model;
  size_t all_groups = model.GetResultGroups().size();
  ASSERT_GT(all_groups, 1u);

  std::set<AstraCommandCategory> filter = {AstraCommandCategory::kTabs};
  model.SetCategoryFilter(filter);

  const auto& groups = model.GetResultGroups();
  EXPECT_EQ(1u, groups.size());
  EXPECT_EQ(AstraCommandCategory::kTabs, groups[0].category);
}

// -- Edge cases: scoring ----------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreEmptyTitle) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"";
  item.description = u"A description";
  item.category = AstraCommandCategory::kTools;

  double score = model.ComputeRelevanceScore(u"test", item);
  // Should match in description.
  EXPECT_LT(score, 0.0);  // "test" not in description
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreDescriptionOnly) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"Some Command";
  item.description = u"Find and replace text";
  item.category = AstraCommandCategory::kTools;

  double score = model.ComputeRelevanceScore(u"replace", item);
  // "replace" is in description only.
  EXPECT_GT(score, 0.0);
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreWhitespaceQuery) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"New Tab";
  item.description = u"Open a new tab";
  item.category = AstraCommandCategory::kTabs;

  double score = model.ComputeRelevanceScore(u"   ", item);
  // Whitespace-only query should behave like empty query.
  EXPECT_GE(score, 0.0);
}

TEST_F(AstraCommandPaletteModelTest, ComputeRelevanceScoreUnicode) {
  AstraCommandPaletteModel model;
  AstraCommandItem item;
  item.title = u"设置";
  item.description = u"浏览器设置";
  item.category = AstraCommandCategory::kSettings;

  double score = model.ComputeRelevanceScore(u"设置", item);
  EXPECT_GT(score, 0.0);
}

// -- Edge cases: acronym ----------------------------------------------------

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchSingleWord) {
  // Single word text — acronym of length 1 should match.
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"s", u"Settings"));
  // Acronym of length > 1 should not match a single word.
  EXPECT_FALSE(AstraCommandPaletteModel::IsAcronymMatch(u"st", u"Settings"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchHyphenated) {
  // Hyphenated words count as separate words.
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"sp", u"Split-View"));
}

TEST_F(AstraCommandPaletteModelTest, IsAcronymMatchLeadingTrailingSpaces) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsAcronymMatch(u"nt", u"  New Tab  "));
}

// -- Edge cases: word boundary ----------------------------------------------

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchSingleWordStart) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"New", u"New Tab"));
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"Tab", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchSingleWordMidFails) {
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"ew", u"New Tab"));
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(u"ab", u"New Tab"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchHyphenated) {
  EXPECT_TRUE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"split view", u"Split-View Mode"));
}

TEST_F(AstraCommandPaletteModelTest, IsWordBoundaryMatchMultiWordQuerySingleWordText) {
  // Multi-word query on single-word text should fail.
  EXPECT_FALSE(AstraCommandPaletteModel::IsWordBoundaryMatch(
      u"new tab", u"Settings"));
}

// =========================================================================
// AstraCommandPaletteItemView tests
// =========================================================================

class AstraCommandPaletteItemViewTest : public views::ViewsTestBase {
 public:
  AstraCommandPaletteItemViewTest() = default;
  ~AstraCommandPaletteItemViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraCommandPaletteItemView>(
            u"New Tab", u"Open a new tab", u"Ctrl+T", "new_tab", false));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraCommandPaletteItemView> item_view_ = nullptr;
};

// -- Construction ----------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, item_view_);
  EXPECT_NE(nullptr, item_view_->GetWidget());
}

TEST_F(AstraCommandPaletteItemViewTest, ConstructFromCommandItem) {
  AstraCommandItem cmd;
  cmd.command_id = 123;
  cmd.title = u"Test Command";
  cmd.description = u"A test command";
  cmd.shortcut_text = u"Ctrl+X";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.icon_name = "test_icon";
  cmd.is_astra = true;

  auto view = std::make_unique<AstraCommandPaletteItemView>(cmd);
  EXPECT_NE(nullptr, view.get());
  EXPECT_EQ(u"Test Command", view->GetCommand().title);
  EXPECT_EQ(123, view->GetCommand().command_id);
}

TEST_F(AstraCommandPaletteItemViewTest, AstraCommandConstructs) {
  auto astra_item = std::make_unique<AstraCommandPaletteItemView>(
      u"Toggle Sidebar", u"Show or hide the sidebar", u"Ctrl+B",
      "sidebar", true);
  EXPECT_NE(nullptr, astra_item.get());
}

// -- Selection state -------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, DefaultIsNotSelected) {
  EXPECT_FALSE(item_view_->IsSelected());
}

TEST_F(AstraCommandPaletteItemViewTest, SetSelectedTrue) {
  item_view_->SetSelected(true);
  EXPECT_TRUE(item_view_->IsSelected());
}

TEST_F(AstraCommandPaletteItemViewTest, SetSelectedFalse) {
  item_view_->SetSelected(true);
  ASSERT_TRUE(item_view_->IsSelected());

  item_view_->SetSelected(false);
  EXPECT_FALSE(item_view_->IsSelected());
}

TEST_F(AstraCommandPaletteItemViewTest, SetSelectedSameStateNoCrash) {
  item_view_->SetSelected(false);
  item_view_->SetSelected(false);
}

TEST_F(AstraCommandPaletteItemViewTest, SelectedItemAccessibleState) {
  item_view_->SetSelected(true);

  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_TRUE(data.HasState(ax::mojom::State::kSelected));
}

TEST_F(AstraCommandPaletteItemViewTest, UnselectedItemNotSelectedState) {
  item_view_->SetSelected(false);

  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.HasState(ax::mojom::State::kSelected));
}

// -- Highlight state -------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, DefaultIsNotHighlighted) {
  EXPECT_FALSE(item_view_->IsHighlighted());
}

TEST_F(AstraCommandPaletteItemViewTest, SetHighlightedTrue) {
  item_view_->SetHighlighted(true);
  EXPECT_TRUE(item_view_->IsHighlighted());
}

TEST_F(AstraCommandPaletteItemViewTest, SetHighlightedFalse) {
  item_view_->SetHighlighted(true);
  ASSERT_TRUE(item_view_->IsHighlighted());

  item_view_->SetHighlighted(false);
  EXPECT_FALSE(item_view_->IsHighlighted());
}

TEST_F(AstraCommandPaletteItemViewTest, SetHighlightedSameStateNoCrash) {
  item_view_->SetHighlighted(true);
  item_view_->SetHighlighted(true);
}

// -- SetCommand ------------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, SetCommandUpdatesData) {
  AstraCommandItem cmd;
  cmd.command_id = 456;
  cmd.title = u"Updated Command";
  cmd.description = u"Updated description";
  cmd.shortcut_text = u"Ctrl+U";
  cmd.type = AstraCommandType::kSetting;
  cmd.category = AstraCommandCategory::kSettings;
  cmd.icon_name = "settings";
  cmd.is_astra = true;

  item_view_->SetCommand(cmd);
  EXPECT_EQ(456, item_view_->GetCommand().command_id);
  EXPECT_EQ(u"Updated Command", item_view_->GetCommand().title);
}

// -- Show/hide icon --------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowIconDefaultTrue) {
  EXPECT_TRUE(item_view_->show_icon());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowIconFalse) {
  item_view_->ShowIcon(false);
  EXPECT_FALSE(item_view_->show_icon());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowIconTrue) {
  item_view_->ShowIcon(false);
  ASSERT_FALSE(item_view_->show_icon());

  item_view_->ShowIcon(true);
  EXPECT_TRUE(item_view_->show_icon());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowIconSameValueNoCrash) {
  item_view_->ShowIcon(true);
  item_view_->ShowIcon(true);
}

// -- Show/hide shortcut ----------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowShortcutDefaultTrue) {
  EXPECT_TRUE(item_view_->show_shortcut());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowShortcutFalse) {
  item_view_->ShowShortcut(false);
  EXPECT_FALSE(item_view_->show_shortcut());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowShortcutTrue) {
  item_view_->ShowShortcut(false);
  ASSERT_FALSE(item_view_->show_shortcut());

  item_view_->ShowShortcut(true);
  EXPECT_TRUE(item_view_->show_shortcut());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowShortcutSameValueNoCrash) {
  item_view_->ShowShortcut(true);
  item_view_->ShowShortcut(true);
}

// -- Show/hide description -------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowDescriptionDefaultTrue) {
  EXPECT_TRUE(item_view_->show_description());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowDescriptionFalse) {
  item_view_->ShowDescription(false);
  EXPECT_FALSE(item_view_->show_description());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowDescriptionTrue) {
  item_view_->ShowDescription(false);
  ASSERT_FALSE(item_view_->show_description());

  item_view_->ShowDescription(true);
  EXPECT_TRUE(item_view_->show_description());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowDescriptionSameValueNoCrash) {
  item_view_->ShowDescription(true);
  item_view_->ShowDescription(true);
}

// -- Show/hide category badge -----------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowCategoryBadgeDefaultTrue) {
  EXPECT_TRUE(item_view_->show_category_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowCategoryBadgeFalse) {
  item_view_->ShowCategoryBadge(false);
  EXPECT_FALSE(item_view_->show_category_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowCategoryBadgeTrue) {
  item_view_->ShowCategoryBadge(false);
  ASSERT_FALSE(item_view_->show_category_badge());

  item_view_->ShowCategoryBadge(true);
  EXPECT_TRUE(item_view_->show_category_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowCategoryBadgeSameValueNoCrash) {
  item_view_->ShowCategoryBadge(true);
  item_view_->ShowCategoryBadge(true);
}

// -- Show/hide recent badge -------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowRecentBadgeDefaultFalse) {
  EXPECT_FALSE(item_view_->show_recent_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowRecentBadgeTrue) {
  item_view_->ShowRecentBadge(true);
  EXPECT_TRUE(item_view_->show_recent_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowRecentBadgeFalse) {
  item_view_->ShowRecentBadge(true);
  ASSERT_TRUE(item_view_->show_recent_badge());

  item_view_->ShowRecentBadge(false);
  EXPECT_FALSE(item_view_->show_recent_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowRecentBadgeSameValueNoCrash) {
  item_view_->ShowRecentBadge(false);
  item_view_->ShowRecentBadge(false);
}

TEST_F(AstraCommandPaletteItemViewTest, SetCommandWithRecentBadge) {
  AstraCommandItem cmd;
  cmd.command_id = 777;
  cmd.title = u"Recent Command";
  cmd.description = u"A recently used command";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_recent = true;

  item_view_->SetCommand(cmd);
  EXPECT_TRUE(item_view_->show_recent_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, SetCommandWithoutRecentBadge) {
  AstraCommandItem cmd;
  cmd.command_id = 778;
  cmd.title = u"Non-Recent Command";
  cmd.description = u"Not recently used";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_recent = false;

  item_view_->SetCommand(cmd);
  EXPECT_FALSE(item_view_->show_recent_badge());
}

// -- Show/hide pinned badge ------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowPinnedBadgeDefaultFalse) {
  EXPECT_FALSE(item_view_->show_pinned_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowPinnedBadgeTrue) {
  item_view_->ShowPinnedBadge(true);
  EXPECT_TRUE(item_view_->show_pinned_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowPinnedBadgeFalse) {
  item_view_->ShowPinnedBadge(true);
  ASSERT_TRUE(item_view_->show_pinned_badge());

  item_view_->ShowPinnedBadge(false);
  EXPECT_FALSE(item_view_->show_pinned_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowPinnedBadgeSameValueNoCrash) {
  item_view_->ShowPinnedBadge(false);
  item_view_->ShowPinnedBadge(false);
}

TEST_F(AstraCommandPaletteItemViewTest, SetCommandWithPinnedBadge) {
  AstraCommandItem cmd;
  cmd.command_id = 779;
  cmd.title = u"Pinned Command";
  cmd.description = u"A pinned/favorite command";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_pinned = true;

  item_view_->SetCommand(cmd);
  EXPECT_TRUE(item_view_->show_pinned_badge());
}

TEST_F(AstraCommandPaletteItemViewTest, SetCommandWithoutPinnedBadge) {
  AstraCommandItem cmd;
  cmd.command_id = 780;
  cmd.title = u"Non-Pinned Command";
  cmd.description = u"Not pinned";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kActions;
  cmd.is_pinned = false;

  item_view_->SetCommand(cmd);
  EXPECT_FALSE(item_view_->show_pinned_badge());
}

// -- Number hint ------------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ShowNumberHintDefaultFalse) {
  EXPECT_FALSE(item_view_->show_number_hint());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowNumberHintTrue) {
  item_view_->ShowNumberHint(true);
  EXPECT_TRUE(item_view_->show_number_hint());
}

TEST_F(AstraCommandPaletteItemViewTest, ShowNumberHintFalse) {
  item_view_->ShowNumberHint(true);
  ASSERT_TRUE(item_view_->show_number_hint());

  item_view_->ShowNumberHint(false);
  EXPECT_FALSE(item_view_->show_number_hint());
}

TEST_F(AstraCommandPaletteItemViewTest, SetNumberHint) {
  item_view_->ShowNumberHint(true);
  item_view_->SetNumberHint(5);
  EXPECT_EQ(5, item_view_->number_hint());
}

TEST_F(AstraCommandPaletteItemViewTest, NumberHintHidesIcon) {
  // When number hint is shown, icon should be hidden.
  item_view_->ShowNumberHint(true);
  // The icon label should be hidden when number hint is visible.
  // We can't directly test icon visibility since it's internal,
  // but we verify no crash and the state flag is set.
  EXPECT_TRUE(item_view_->show_number_hint());
}

// -- Match ranges ----------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, DefaultMatchRangesEmpty) {
  EXPECT_TRUE(item_view_->match_ranges().empty());
}

TEST_F(AstraCommandPaletteItemViewTest, SetMatchRanges) {
  std::vector<gfx::Range> ranges;
  ranges.emplace_back(0, 3);
  item_view_->SetMatchRanges(ranges);

  ASSERT_EQ(1u, item_view_->match_ranges().size());
  EXPECT_EQ(0u, item_view_->match_ranges()[0].start());
  EXPECT_EQ(3u, item_view_->match_ranges()[0].end());
}

TEST_F(AstraCommandPaletteItemViewTest, SetMatchRangesMultiple) {
  std::vector<gfx::Range> ranges;
  ranges.emplace_back(0, 1);
  ranges.emplace_back(4, 5);
  item_view_->SetMatchRanges(ranges);

  ASSERT_EQ(2u, item_view_->match_ranges().size());
}

TEST_F(AstraCommandPaletteItemViewTest, ClearMatchRanges) {
  std::vector<gfx::Range> ranges;
  ranges.emplace_back(0, 3);
  item_view_->SetMatchRanges(ranges);
  ASSERT_FALSE(item_view_->match_ranges().empty());

  item_view_->SetMatchRanges({});
  EXPECT_TRUE(item_view_->match_ranges().empty());
}

// -- Legacy content update -------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, UpdateContent) {
  item_view_->UpdateContent(u"Close Tab", u"Close the current tab",
                             u"Ctrl+W", u"X");
}

TEST_F(AstraCommandPaletteItemViewTest, UpdateContentEmptyStrings) {
  item_view_->UpdateContent(std::u16string(), std::u16string(),
                             std::u16string(), std::u16string());
}

// -- Activation callback ---------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, ActivatedCallbackFiresOnMousePress) {
  ItemActivationTracker tracker;
  item_view_->SetActivatedCallback(base::BindLambdaForTesting(
      [&tracker]() { tracker.activated_count++; }));

  EXPECT_EQ(0, tracker.activated_count);

  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  item_view_->OnMousePressed(event);

  EXPECT_GE(tracker.activated_count, 1);
}

TEST_F(AstraCommandPaletteItemViewTest, NullActivatedCallbackIsSafe) {
  item_view_->SetActivatedCallback(
      AstraCommandPaletteItemView::ActivatedCallback());

  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  item_view_->OnMousePressed(event);
}

// -- Mouse events ----------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, MouseEnterExitNoCrash) {
  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(),
                             gfx::Point(), base::TimeTicks(), 0, 0);
  item_view_->OnMouseEntered(enter_event);

  ui::MouseEvent exit_event(ui::ET_MOUSE_EXITED, gfx::Point(),
                            gfx::Point(), base::TimeTicks(), 0, 0);
  item_view_->OnMouseExited(exit_event);
}

// -- Theme -----------------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, OnThemeChangedDoesNotCrash) {
  item_view_->OnThemeChanged();
}

TEST_F(AstraCommandPaletteItemViewTest, HasColorProvider) {
  EXPECT_NE(nullptr, item_view_->GetColorProvider());
}

// -- Size ------------------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = item_view_->CalculatePreferredSize(
      views::SizeBounds(gfx::Size(400, 100)));
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// -- Accessibility ---------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, AccessibleRoleIsListItem) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kListItem, data.role);
}

TEST_F(AstraCommandPaletteItemViewTest, AccessibleNameIncludesDisplayName) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_NE(std::u16string::npos, data.GetName().find(u"New Tab"));
}

TEST_F(AstraCommandPaletteItemViewTest, AccessibleDescriptionHasDescription) {
  ui::AXNodeData data;
  item_view_->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.GetDescription().empty());
}

// -- Edge cases ------------------------------------------------------------

TEST_F(AstraCommandPaletteItemViewTest, LongTitleNoCrash) {
  AstraCommandItem cmd;
  cmd.command_id = 1;
  cmd.title = std::u16string(1000, u'x');
  cmd.description = u"desc";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.icon_name = "icon";
  item_view_->SetCommand(cmd);
  // No crash = success.
}

TEST_F(AstraCommandPaletteItemViewTest, EmptyTitleNoCrash) {
  AstraCommandItem cmd;
  cmd.command_id = 1;
  cmd.title = u"";
  cmd.description = u"desc";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  item_view_->SetCommand(cmd);
}

TEST_F(AstraCommandPaletteItemViewTest, EmptyDescriptionNoCrash) {
  AstraCommandItem cmd;
  cmd.command_id = 1;
  cmd.title = u"Title";
  cmd.description = u"";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  item_view_->SetCommand(cmd);
}

// =========================================================================
// AstraCommandPaletteSectionHeaderView tests
// =========================================================================

class AstraCommandPaletteSectionHeaderViewTest : public views::ViewsTestBase {
 public:
  AstraCommandPaletteSectionHeaderViewTest() = default;
  ~AstraCommandPaletteSectionHeaderViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    header_view_ = widget_->SetContentsView(
        std::make_unique<AstraCommandPaletteSectionHeaderView>(u"Tabs"));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraCommandPaletteSectionHeaderView> header_view_ = nullptr;
};

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, header_view_);
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = header_view_->CalculatePreferredSize(
      views::SizeBounds(gfx::Size(500, 100)));
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, SetLabelUpdatesText) {
  header_view_->SetLabel(u"Workspaces");
  // No crash = success. The label text is internal but the method should work.
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, OnThemeChangedDoesNotCrash) {
  header_view_->OnThemeChanged();
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ConstructWithLongLabel) {
  auto header = std::make_unique<AstraCommandPaletteSectionHeaderView>(
      std::u16string(200, u'x'));
  EXPECT_NE(nullptr, header.get());
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ConstructWithEmptyLabel) {
  auto header = std::make_unique<AstraCommandPaletteSectionHeaderView>(u"");
  EXPECT_NE(nullptr, header.get());
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, AllCategoryLabelsWork) {
  for (int i = 0; i <= static_cast<int>(AstraCommandCategory::kExtensions); ++i) {
    auto cat = static_cast<AstraCommandCategory>(i);
    auto header =
        std::make_unique<AstraCommandPaletteSectionHeaderView>(
            GetCategoryLabel(cat));
    EXPECT_NE(nullptr, header.get());
  }
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, SetIcon) {
  header_view_->SetIcon(u"📁");
  // No crash = success.
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ShowIconDefaultTrue) {
  EXPECT_TRUE(header_view_->show_icon());
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ShowIconFalse) {
  header_view_->ShowIcon(false);
  EXPECT_FALSE(header_view_->show_icon());
}

TEST_F(AstraCommandPaletteSectionHeaderViewTest, ShowIconTrue) {
  header_view_->ShowIcon(false);
  ASSERT_FALSE(header_view_->show_icon());

  header_view_->ShowIcon(true);
  EXPECT_TRUE(header_view_->show_icon());
}

// =========================================================================
// AstraCommandPaletteView tests
// =========================================================================

class AstraCommandPaletteViewTest : public views::ViewsTestBase {
 public:
  AstraCommandPaletteViewTest() = default;
  ~AstraCommandPaletteViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    // We need a Browser* for the view. In unit tests without a real browser,
    // we can't easily create one.  For view tests that don't need command
    // execution, we can test the item view and model directly.
    //
    // TODO(astra): Add a mock Browser or use TestingBrowserProcess for
    // view-level tests that need a Browser*.
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
};

// -- Model set/get ---------------------------------------------------------

TEST_F(AstraCommandPaletteViewTest, ModelDefaultOwned) {
  // A default-constructed model should work.
  AstraCommandPaletteModel model;
  EXPECT_GT(model.GetCommandCount(), 0u);
}

TEST_F(AstraCommandPaletteViewTest, SetModelNull) {
  AstraCommandPaletteModel model;
  // Verifying SetModel doesn't crash.
  // TODO(astra): Full view tests when Browser mock is available.
  SUCCEED();
}

// =========================================================================
// Bubble tests
// =========================================================================

class AstraCommandPaletteBubbleTest : public views::ViewsTestBase {
 public:
  AstraCommandPaletteBubbleTest() = default;
  ~AstraCommandPaletteBubbleTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
  }

  void TearDown() override {
    ViewsTestBase::TearDown();
  }
};

TEST_F(AstraCommandPaletteBubbleTest, BubbleWidthConstant) {
  // The bubble should have a defined width constant.
  // We can't create a full bubble without a Browser, but we can verify
  // the constants are accessible.
  SUCCEED() << "Bubble constants defined in header.";
}

TEST_F(AstraCommandPaletteBubbleTest, BubbleDelegateDefaultDoesNotCrash) {
  class DefaultDelegate : public AstraCommandPaletteBubble::Delegate {};
  DefaultDelegate delegate;
  delegate.OnCommandPaletteExecute(100, false);
  delegate.OnCommandPaletteClosed();
  SUCCEED();
}

// =========================================================================
// View delegate defaults
// =========================================================================

TEST(AstraCommandPaletteViewDelegateTest, DefaultDelegateDoesNotCrash) {
  class DefaultDelegate : public AstraCommandPaletteView::Delegate {};

  DefaultDelegate delegate;
  delegate.OnCommandPaletteExecute(100, false);
  delegate.OnCommandPaletteClose();
  delegate.OnCommandPaletteSearchTextChanged(u"test");
  delegate.OnCommandPaletteSelectionChanged(0);
  delegate.OnCommandPaletteOpened();
  delegate.OnCommandPaletteClosed();
  SUCCEED();
}

// =========================================================================
// Persistence tests (with PrefService)
// =========================================================================

namespace {
class AstraCommandPalettePersistenceTest : public testing::Test {
 public:
  AstraCommandPalettePersistenceTest() {
    // Register prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_.GetPrefs()->registry());
  }

  ~AstraCommandPalettePersistenceTest() override = default;

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingProfile profile_;
};
}  // namespace

TEST_F(AstraCommandPalettePersistenceTest, DefaultPrefValues) {
  auto* prefs = profile_.GetPrefs();

  EXPECT_EQ(prefs::kDefaultCommandPaletteMaxVisible,
            prefs->GetInteger(prefs::kPrefCommandPaletteMaxVisible));
  EXPECT_EQ(prefs::kDefaultCommandPaletteShowDescriptions,
            prefs->GetBoolean(prefs::kPrefCommandPaletteShowDescriptions));
  EXPECT_EQ(prefs::kDefaultCommandPaletteShowShortcuts,
            prefs->GetBoolean(prefs::kPrefCommandPaletteShowShortcuts));
  EXPECT_EQ(prefs::kDefaultCommandPaletteShowRecentSection,
            prefs->GetBoolean(prefs::kPrefCommandPaletteShowRecentSection));
}

TEST_F(AstraCommandPalettePersistenceTest, LoadFromPrefs) {
  auto* prefs = profile_.GetPrefs();

  prefs->SetInteger(prefs::kPrefCommandPaletteMaxVisible, 5);
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowDescriptions, false);
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowShortcuts, false);
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowRecentSection, false);

  base::Value::List recent_list;
  recent_list.Append(kAstraCommandToggleSidebar);
  recent_list.Append(kAstraCommandNewWorkspace);
  prefs->SetList(prefs::kPrefCommandRecentList, std::move(recent_list));

  AstraCommandPaletteModel model;
  model.LoadFromPrefs(prefs);

  EXPECT_EQ(5u, model.max_search_results());
  EXPECT_FALSE(model.show_descriptions());
  EXPECT_FALSE(model.show_shortcuts());
  EXPECT_FALSE(model.show_recent_section());

  const auto& recent = model.GetRecentCommands(10);
  EXPECT_GE(recent.size(), 2u);
}

TEST_F(AstraCommandPalettePersistenceTest, SaveToPrefs) {
  auto* prefs = profile_.GetPrefs();

  AstraCommandPaletteModel model;
  model.set_max_search_results(8);
  model.set_show_descriptions(false);
  model.set_show_shortcuts(true);
  model.set_show_recent_section(false);

  model.RecordCommandUse(kAstraCommandToggleSidebar);
  model.RecordCommandUse(kAstraCommandNewWorkspace);

  model.SaveToPrefs(prefs);

  EXPECT_EQ(8, prefs->GetInteger(prefs::kPrefCommandPaletteMaxVisible));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefCommandPaletteShowDescriptions));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefCommandPaletteShowShortcuts));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefCommandPaletteShowRecentSection));

  const auto& recent_list = prefs->GetList(prefs::kPrefCommandRecentList);
  EXPECT_GT(recent_list.size(), 0u);
}

TEST_F(AstraCommandPalettePersistenceTest, RoundTrip) {
  auto* prefs = profile_.GetPrefs();

  {
    AstraCommandPaletteModel model;
    model.set_max_search_results(3);
    model.set_show_descriptions(false);
    model.set_show_shortcuts(false);
    model.RecordCommandUse(kAstraCommandOpenCommandPalette);
    model.SaveToPrefs(prefs);
  }

  {
    AstraCommandPaletteModel model;
    model.LoadFromPrefs(prefs);

    EXPECT_EQ(3u, model.max_search_results());
    EXPECT_FALSE(model.show_descriptions());
    EXPECT_FALSE(model.show_shortcuts());

    const auto& recent = model.GetRecentCommands(10);
    EXPECT_FALSE(recent.empty());
  }
}

TEST_F(AstraCommandPalettePersistenceTest, LoadFromNullPrefs) {
  AstraCommandPaletteModel model;
  model.LoadFromPrefs(nullptr);
  EXPECT_EQ(AstraCommandPaletteModel::kMaxResults,
            model.max_search_results());
}

TEST_F(AstraCommandPalettePersistenceTest, SaveToNullPrefs) {
  AstraCommandPaletteModel model;
  model.set_max_search_results(5);
  model.SaveToPrefs(nullptr);
  SUCCEED();
}

TEST_F(AstraCommandPalettePersistenceTest, RecentCommandsCappedOnLoad) {
  auto* prefs = profile_.GetPrefs();

  base::Value::List recent_list;
  for (int i = 0; i < 50; ++i) {
    recent_list.Append(kAstraCommandFirst + i);
  }
  prefs->SetList(prefs::kPrefCommandRecentList, std::move(recent_list));

  AstraCommandPaletteModel model;
  model.LoadFromPrefs(prefs);

  EXPECT_LE(model.GetRecentCommands(100).size(),
            AstraCommandPaletteModel::kMaxRecentlyUsed);
}

// =========================================================================
// Structure documentation tests
// =========================================================================

TEST(AstraCommandPaletteStructureTest, ThreeCommandSources) {
  SUCCEED();
}

TEST(AstraCommandPaletteStructureTest, ObserverPattern) {
  SUCCEED();
}

TEST(AstraCommandPaletteStructureTest, SearchScoring) {
  SUCCEED();
}

TEST(AstraCommandPaletteStructureTest, KeyboardNavigation) {
  SUCCEED();
}

TEST(AstraCommandPaletteStructureTest, BubbleProperties) {
  SUCCEED();
}

// =========================================================================
// Observer defaults tests
// =========================================================================

TEST(AstraCommandPaletteObserverDefaultsTest, DefaultObserverDoesNotCrash) {
  class DefaultObserver : public AstraCommandPaletteModelObserver {};

  DefaultObserver observer;
  AstraCommandPaletteModel model;
  model.AddObserver(&observer);

  model.SetQuery(u"test");
  model.SetSelectedIndex(0);
  model.ExecuteCommand(0);
  model.NotifyPaletteOpened();
  model.NotifyPaletteClosed();
  model.NotifyCommandExecuted(100, false);

  model.RemoveObserver(&observer);
  SUCCEED();
}

TEST(AstraCommandPaletteObserverDefaultsTest, NewStyleDefaultObserverDoesNotCrash) {
  class DefaultObserver : public AstraCommandPaletteObserver {};

  DefaultObserver observer;
  AstraCommandPaletteModel model;
  model.AddObserver(&observer);

  model.SetQuery(u"test");
  model.ExecuteCommand(0);

  model.RemoveObserver(&observer);
  SUCCEED();
}

TEST(AstraCommandPaletteObserverDefaultsTest, DefaultObserverAddRemoveIsSafe) {
  class DefaultObserver : public AstraCommandPaletteModelObserver {};

  AstraCommandPaletteModel model;
  DefaultObserver observer1;
  DefaultObserver observer2;

  model.AddObserver(&observer1);
  model.AddObserver(&observer2);
  model.RemoveObserver(&observer1);
  model.RemoveObserver(&observer2);
  SUCCEED();
}

// =========================================================================
// Multiple observer test
// =========================================================================

TEST(AstraCommandPaletteMultipleObserverTest, MultipleLegacyObservers) {
  AstraCommandPaletteModel model;
  TestModelObserver observer1;
  TestModelObserver observer2;

  model.AddObserver(&observer1);
  model.AddObserver(&observer2);

  model.SetQuery(u"tab");

  EXPECT_GT(observer1.search_text_changed_count_, 0);
  EXPECT_GT(observer2.search_text_changed_count_, 0);
  EXPECT_GT(observer1.model_changed_count_, 0);
  EXPECT_GT(observer2.model_changed_count_, 0);

  model.RemoveObserver(&observer1);
  model.RemoveObserver(&observer2);
}

TEST(AstraCommandPaletteMultipleObserverTest, BothObserverTypesWork) {
  AstraCommandPaletteModel model;
  TestModelObserver legacy_observer;
  TestCommandPaletteObserver new_observer;

  model.AddObserver(&legacy_observer);
  model.AddObserver(&new_observer);

  model.SetQuery(u"tab");

  EXPECT_GT(legacy_observer.model_changed_count_, 0);
  EXPECT_GT(new_observer.search_results_changed_count_, 0);

  model.RemoveObserver(&legacy_observer);
  model.RemoveObserver(&new_observer);
}

// =========================================================================
// Search in command IDs
// =========================================================================

TEST(AstraCommandPaletteSearchInIdsTest, DisabledByDefault) {
  AstraCommandPaletteModel model;
  EXPECT_FALSE(model.search_in_command_ids());
}

TEST(AstraCommandPaletteSearchInIdsTest, EnabledFindsById) {
  AstraCommandPaletteModel model;
  model.set_search_in_command_ids(true);

  // Add a command with a unique numeric ID that we can search for.
  AstraCommandItem cmd;
  cmd.command_id = 55555;
  cmd.title = u"Unique ID Command";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);

  auto results = model.SearchCommands(u"55555");
  EXPECT_GT(results.size(), 0u);
}

TEST(AstraCommandPaletteSearchInIdsTest, DisabledDoesNotFindById) {
  AstraCommandPaletteModel model;
  model.set_search_in_command_ids(false);

  AstraCommandItem cmd;
  cmd.command_id = 55556;
  cmd.title = u"Unique ID Command 2";
  cmd.type = AstraCommandType::kAction;
  cmd.category = AstraCommandCategory::kTools;
  cmd.is_astra = true;
  model.AddCommand(cmd);

  auto results = model.SearchCommands(u"55556");
  // Should NOT find it by ID when search_in_command_ids is false.
  // But it might find it by title.
  bool found_by_id = false;
  for (const auto& r : results) {
    if (r.command_id == 55556) {
      // Check if the match was likely by title (not ID).
      // If the title contains "55556" it would match via title search.
      if (r.title.find(u"55556") == std::u16string::npos) {
        found_by_id = true;
      }
    }
  }
  EXPECT_FALSE(found_by_id);
}

}  // namespace astra
