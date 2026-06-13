// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_DUPLICATES_ASTRA_TAB_DUPLICATES_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_DUPLICATES_ASTRA_TAB_DUPLICATES_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Checkbox;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraDuplicateTabGroupView — one group of duplicate tabs
// =========================================================================
//
// A card showing a group of duplicate tabs (same URL/domain) with
// checkboxes to select which to close.
//
// Layout:
//   +-------------------------------------------+
//   |  📄 example.com/article (3 duplicates)    |
//   |  [x] Tab Title One          [Keep]        |
//   |  [x] Tab Title Two        [Close]      |
//   |  [x] Tab Title Three      [Close]      |
//   +-------------------------------------------+
// =========================================================================

class AstraDuplicateTabGroupView : public views::View {
 public:
  using CloseTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using KeepTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CloseGroupCallback =
      base::RepeatingCallback<void(const std::string& group_key)>;

  struct TabItem {
    std::string tab_id;
    std::u16string title;
    std::string url;
    std::string domain;
    bool is_active = false;
    bool is_pinned = false;
    base::Time last_accessed;
    int index_in_group = 0;
  };

  struct GroupInfo {
    std::string group_key;  // e.g. normalized URL or domain
    std::u16string label;   // display label
    std::vector<TabItem> tabs;
    int duplicate_count = 0;
  };

  AstraDuplicateTabGroupView(
      const GroupInfo& info,
      CloseTabCallback close_tab_callback,
      KeepTabCallback keep_tab_callback,
      CloseGroupCallback close_group_callback);
  ~AstraDuplicateTabGroupView() override;

  AstraDuplicateTabGroupView(const AstraDuplicateTabGroupView&) = delete;
  AstraDuplicateTabGroupView& operator=(
      const AstraDuplicateTabGroupView&) = delete;

  const std::string& group_key() const { return group_key_; }
  int duplicate_count() const { return duplicate_count_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void BuildTabRows();
  void OnCloseTab(const std::string& tab_id);
  void OnKeepTab(const std::string& tab_id);
  void OnCloseDuplicates();

  static std::u16 FormatLastAccessed(base::Time time);

  std::string group_key_;
  std::u16string label_;
  std::vector<TabItem> tabs_;
  int duplicate_count_ = 0;

  CloseTabCallback close_tab_callback_;
  KeepTabCallback keep_tab_callback_;
  CloseGroupCallback close_group_callback_;

  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::MdTextButton> close_all_button_ = nullptr;
  raw_ptr<views::View> tabs_list_ = nullptr;

  std::vector<raw_ptr<views::View>> tab_rows_;
};

// =========================================================================
// AstraTabDuplicatesView — duplicate tab finder panel
// =========================================================================
//
// A bubble showing duplicate tab groups: tabs with the same URL or domain,
// with options to close duplicates and keep one.
//
// Layout:
//   +-------------------------------------------+
//   |  Duplicate Tabs               [Close]    |
//   +-------------------------------------------+
//   |  Found 5 duplicate groups (12 tabs)        |
//   |  [ Close all duplicates ]                 |
//   +-------------------------------------------+
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📄 example.com/article (3 dupes)         │  |
//   |  │ [x] Tab Title 1    [Keep]        │  |
//   |  │ [x] Tab Title 2    [Close]       │  |
//   |  │ [x] Tab Title 3    [Close]       │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📄 github.com (2 dupes)               │  |
//   |  │ [x] Dashboard        [Keep]                │  |
//   |  │ [x] Pull Requests  [Close]         │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Duplicate detection is handled by
// Astra's tab analysis service.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - TabStripModel (source of tab data)
// =========================================================================

class AstraTabDuplicatesView : public views::BubbleDialogDelegateView {
 public:
  using CloseTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using KeepTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CloseAllDuplicatesCallback = base::RepeatingClosure;
  using RefreshCallback = base::RepeatingClosure;

  enum class MatchMode { kExactUrl, kSameDomain, kSameHost };

  explicit AstraTabDuplicatesView(views::View* anchor_view);
  ~AstraTabDuplicatesView() override;

  AstraTabDuplicatesView(const AstraTabDuplicatesView&) = delete;
  AstraTabDuplicatesView& operator=(const AstraTabDuplicatesView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetDuplicateGroups(
      const std::vector<AstraDuplicateTabGroupView::GroupInfo>& groups);
  void SetMatchMode(MatchMode mode);

  // -- Callbacks -----------------------------------------------------------

  void SetCloseTabCallback(CloseTabCallback callback);
  void SetKeepTabCallback(KeepTabCallback callback);
  void SetCloseAllDuplicatesCallback(CloseAllDuplicatesCallback callback);
  void SetRefreshCallback(RefreshCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSummarySection();
  void BuildGroupsList();

  void RefreshGroups();
  void RefreshSummary();

  void OnCloseTab(const std::string& tab_id);
  void OnKeepTab(const std::string& tab_id);
  void OnCloseAllDuplicates();
  void OnRefresh();

  static std::u16 MatchModeLabel(MatchMode mode);

  std::vector<AstraDuplicateTabGroupView::GroupInfo> groups_;
  MatchMode match_mode_ = MatchMode::kSameDomain;
  int total_duplicate_tabs_ = 0;

  CloseTabCallback close_tab_callback_;
  KeepTabCallback keep_tab_callback_;
  CloseAllDuplicatesCallback close_all_callback_;
  RefreshCallback refresh_callback_;

  raw_ptr<views::Label> summary_label_ = nullptr;
  raw_ptr<views::MdTextButton> close_all_button_ = nullptr;
  raw_ptr<views::MdTextButton> refresh_button_ = nullptr;
  raw_ptr<views::View> groups_list_ = nullptr;

  std::vector<raw_ptr<AstraDuplicateTabGroupView>> group_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_DUPLICATES_ASTRA_TAB_DUPLICATES_VIEW_H_
