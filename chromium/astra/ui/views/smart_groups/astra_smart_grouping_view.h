// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SMART_GROUPS_ASTRA_SMART_GROUPING_VIEW_H_
#define ASTRA_UI_VIEWS_SMART_GROUPS_ASTRA_SMART_GROUPING_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
class Checkbox;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSmartGroupSuggestionView — single grouping suggestion card
// =========================================================================
//
// A card showing a suggested tab group: group name, tab count, preview
// of tabs in the group, and a button to apply the suggestion.
//
// Layout:
//   +-------------------------------------------+
//   |  [x]  Work-related (5 tabs)     [Group]   |
//   |  🔗 docs.google.com, mail.google.com...   |
//   +-------------------------------------------+
// =========================================================================

class AstraSmartGroupSuggestionView : public views::View {
 public:
  using ApplyCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;
  using DismissCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;

  struct Suggestion {
    std::string suggestion_id;
    std::u16string name;
    std::string group_type;  // "domain", "time", "purpose", "workspace"
    int tab_count = 0;
    std::vector<std::string> sample_domains;
    std::vector<std::string> tab_ids;
    std::string color;  // hex color like "#5B8FF9"
  };

  AstraSmartGroupSuggestionView(const Suggestion& suggestion,
                                ApplyCallback apply_callback,
                                DismissCallback dismiss_callback);
  ~AstraSmartGroupSuggestionView() override;

  AstraSmartGroupSuggestionView(const AstraSmartGroupSuggestionView&) = delete;
  AstraSmartGroupSuggestionView& operator=(
      const AstraSmartGroupSuggestionView&) = delete;

  const std::string& suggestion_id() const { return suggestion_id_; }

  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnApplyClicked();
  void OnDismissClicked();
  void OnToggled();

  std::string suggestion_id_;
  std::u16string name_;
  std::string group_type_;
  int tab_count_ = 0;
  std::vector<std::string> sample_domains_;
  std::string color_;
  bool selected_ = true;

  ApplyCallback apply_callback_;
  DismissCallback dismiss_callback_;

  raw_ptr<views::Checkbox> checkbox_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> sample_label_ = nullptr;
  raw_ptr<views::MdTextButton> apply_button_ = nullptr;
  raw_ptr<views::MdTextButton> dismiss_button_ = nullptr;
  raw_ptr<views::View> color_dot_ = nullptr;
};

// =========================================================================
// AstraSmartGroupingView — smart tab grouping suggestions panel
// =========================================================================
//
// A bubble showing AI/rule-based suggestions for organizing tabs into
// groups. Supports grouping by domain, time of day, purpose, or
// suggested workspace assignment.
//
// Layout:
//   +-------------------------------------------+
//   |  Smart Groups                 [Close]    |
//   +-------------------------------------------+
//   |  Group by: [Domain] [Time] [Purpose]      |
//   +-------------------------------------------+
//   |  Suggestions (3)                           |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ [x]  Work (5 tabs)         [Group]  │  |
//   |  │  docs.google.com, mail.google.com…  │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ [x]  Social (3 tabs)       [Group]  │  |
//   |  │  twitter.com, reddit.com…           │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ [x]  Shopping (2 tabs)     [Group]  │  |
//   |  │  amazon.com, ebay.com…              │  |
//   |  └─────────────────────────────────────┘  |
//   +-------------------------------------------+
//   |  [ Apply selected ] [ Refresh suggestions ] |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Suggestions come from Astra's
// smart grouping service which analyzes tab metadata.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::Checkbox
//   - TabStripModel (source of tab data)
// =========================================================================

class AstraSmartGroupingView : public views::BubbleDialogDelegateView {
 public:
  using ApplySuggestionsCallback =
      base::RepeatingCallback<void(const std::vector<std::string>& ids)>;
  using DismissSuggestionCallback =
      base::RepeatingCallback<void(const std::string& id)>;
  using GroupByChangedCallback =
      base::RepeatingCallback<void(const std::string& group_by)>;
  using RefreshCallback = base::RepeatingClosure;

  enum class GroupBy { kDomain, kTime, kPurpose, kWorkspace };

  explicit AstraSmartGroupingView(views::View* anchor_view);
  ~AstraSmartGroupingView() override;

  AstraSmartGroupingView(const AstraSmartGroupingView&) = delete;
  AstraSmartGroupingView& operator=(
      const AstraSmartGroupingView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetSuggestions(
      const std::vector<AstraSmartGroupSuggestionView::Suggestion>& suggestions);

  void SetGroupBy(GroupBy group_by);

  // -- Callbacks -----------------------------------------------------------

  void SetApplySuggestionsCallback(ApplySuggestionsCallback callback);
  void SetDismissSuggestionCallback(DismissSuggestionCallback callback);
  void SetGroupByChangedCallback(GroupByChangedCallback callback);
  void SetRefreshCallback(RefreshCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildGroupByRow();
  void BuildSuggestionsList();
  void BuildActionButtons();

  void RefreshSuggestionsList();

  void OnGroupByDomain();
  void OnGroupByTime();
  void OnGroupByPurpose();
  void OnGroupByWorkspace();
  void OnApplySelected();
  void OnRefreshClicked();
  void OnApplySuggestion(const std::string& id);
  void OnDismissSuggestion(const std::string& id);

  std::vector<AstraSmartGroupSuggestionView::Suggestion> suggestions_;
  GroupBy group_by_ = GroupBy::kDomain;

  ApplySuggestionsCallback apply_callback_;
  DismissSuggestionCallback dismiss_callback_;
  GroupByChangedCallback group_by_changed_callback_;
  RefreshCallback refresh_callback_;

  raw_ptr<views::MdTextButton> domain_button_ = nullptr;
  raw_ptr<views::MdTextButton> time_button_ = nullptr;
  raw_ptr<views::MdTextButton> purpose_button_ = nullptr;
  raw_ptr<views::MdTextButton> workspace_button_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> suggestions_list_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::MdTextButton> apply_button_ = nullptr;
  raw_ptr<views::MdTextButton> refresh_button_ = nullptr;

  std::vector<raw_ptr<AstraSmartGroupSuggestionView>> suggestion_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SMART_GROUPS_ASTRA_SMART_GROUPING_VIEW_H_
