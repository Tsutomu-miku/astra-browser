// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_SUGGESTIONS_ASTRA_TAB_SUGGESTIONS_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_SUGGESTIONS_ASTRA_TAB_SUGGESTIONS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabSuggestionItemView — single tab suggestion card
// =========================================================================
//
// A card showing one tab suggestion with reason, relevance score,
// and open button.
//
// Layout:
//   +-------------------------------------------+
//   |  📌 Continue: Project Dashboard             |
//   |     work.com · last visited 2h ago        |
//   |     Because you visit this daily           |
//   |                             [ Open ]       |
//   +-------------------------------------------+
// =========================================================================

class AstraTabSuggestionItemView : public views::View {
 public:
  using OpenCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;
  using DismissCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;

  enum class SuggestionType {
    kContinue,       // Continue where you left off
    kReopen,         // Recently closed
    kRelated,        // Related to current tab
    kDaily,          // Daily visit pattern
    kMorningRoutine, // Morning routine
    kEveningWindDown // Evening wind-down
  };

  struct SuggestionInfo {
    std::string suggestion_id;
    std::u16string title;
    std::string url;
    std::string domain;
    std::u16string reason;  // Why this is suggested
    SuggestionType type = SuggestionType::kContinue;
    int relevance_score = 0;  // 0-100
    base::Time last_visited;
    int visit_count = 0;
    bool is_openable = true;
  };

  AstraTabSuggestionItemView(const SuggestionInfo& info,
                             OpenCallback open_callback,
                             DismissCallback dismiss_callback);
  ~AstraTabSuggestionItemView() override;

  AstraTabSuggestionItemView(const AstraTabSuggestionItemView&) = delete;
  AstraTabSuggestionItemView& operator=(
      const AstraTabSuggestionItemView&) = delete;

  const std::string& suggestion_id() const { return suggestion_id_; }
  int relevance_score() const { return relevance_score_; }

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnOpenClicked();
  void OnDismissClicked();

  void PaintRelevanceBar(gfx::Canvas* canvas);

  static std::u16 TypeIcon(SuggestionType type);
  static std::u16 TypeLabel(SuggestionType type);

  std::string suggestion_id_;
  std::u16string title_;
  std::string url_;
  std::string domain_;
  std::u16string reason_;
  SuggestionType type_ = SuggestionType::kContinue;
  int relevance_score_ = 0;
  base::Time last_visited_;
  int visit_count_ = 0;
  bool is_openable_ = true;

  OpenCallback open_callback_;
  DismissCallback dismiss_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> subtitle_label_ = nullptr;
  raw_ptr<views::Label> reason_label_ = nullptr;
  raw_ptr<views::MdTextButton> open_button_ = nullptr;
  raw_ptr<views::MdTextButton> dismiss_button_ = nullptr;
};

// =========================================================================
// AstraTabSuggestionsView — smart tab suggestions panel
// =========================================================================
//
// A bubble showing smart tab suggestions: tabs you might want to open,
// based on browsing patterns, time of day, and current context.
//
// Layout:
//   +-------------------------------------------+
//   |  Suggestions for You         [Close]    |
//   +-------------------------------------------+
//   |  Based on your browsing habits            |
//   |  [Refresh]                                |
//   +-------------------------------------------+
//   |  📌 Continue...                            |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ Project Dashboard                     │  |
//   │  │ work.com · 2h ago                    │  |
//   │  │ Because you visit this daily         │  |
//   │  │ ▓▓▓▓▓▓▓░░░ [Open] [×]               │  |
//   |  └─────────────────────────────────────┘  |
//   |  🕐 Recently closed...                     |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ News Article                          │  |
//   │  │ news.com · 30m ago                   │  |
//   │  │ You closed this earlier today        │  |
//   │  │ ▓▓▓▓▓░░░░░░ [Open] [×]              │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Suggestions are generated by Astra's
// tab suggestion engine based on browsing history and patterns.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - History service (source of visit data)
//   - TabStripModel (for context)
// =========================================================================

class AstraTabSuggestionsView : public views::BubbleDialogDelegateView {
 public:
  using OpenSuggestionCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;
  using DismissSuggestionCallback =
      base::RepeatingCallback<void(const std::string& suggestion_id)>;
  using RefreshCallback = base::RepeatingClosure;

  explicit AstraTabSuggestionsView(views::View* anchor_view);
  ~AstraTabSuggestionsView() override;

  AstraTabSuggestionsView(const AstraTabSuggestionsView&) = delete;
  AstraTabSuggestionsView& operator=(
      const AstraTabSuggestionsView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetSuggestions(
      const std::vector<AstraTabSuggestionItemView::SuggestionInfo>& suggestions);

  // -- Callbacks -----------------------------------------------------------

  void SetOpenSuggestionCallback(OpenSuggestionCallback callback);
  void SetDismissSuggestionCallback(DismissSuggestionCallback callback);
  void SetRefreshCallback(RefreshCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildHeaderSection();
  void BuildSuggestionsList();

  void RefreshSuggestions();

  void OnOpenSuggestion(const std::string& id);
  void OnDismissSuggestion(const std::string& id);
  void OnRefresh();

  std::vector<AstraTabSuggestionItemView::SuggestionInfo> suggestions_;

  OpenSuggestionCallback open_callback_;
  DismissSuggestionCallback dismiss_callback_;
  RefreshCallback refresh_callback_;

  raw_ptr<views::Label> subtitle_label_ = nullptr;
  raw_ptr<views::MdTextButton> refresh_button_ = nullptr;
  raw_ptr<views::View> suggestions_list_ = nullptr;

  std::vector<raw_ptr<AstraTabSuggestionItemView>> suggestion_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SUGGESTIONS_ASTRA_TAB_SUGGESTIONS_VIEW_H_
