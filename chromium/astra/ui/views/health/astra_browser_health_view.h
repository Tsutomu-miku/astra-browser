// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_HEALTH_ASTRA_BROWSER_HEALTH_VIEW_H_
#define ASTRA_UI_VIEWS_HEALTH_ASTRA_BROWSER_HEALTH_VIEW_H_

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
class ProgressBar;
}  // namespace views

namespace astra {

// =========================================================================
// AstraHealthIssueRowView — single health issue / suggestion
// =========================================================================
//
// A row showing a health issue with severity indicator, description,
// and an action button.
//
// Layout:
//   +-------------------------------------------+
//   |  ⚠️  Heavy Tab              [Sleep it]   |
//   |     YouTube — 320 MB · 22% CPU            |
//   +-------------------------------------------+
// =========================================================================

class AstraHealthIssueRowView : public views::View {
 public:
  using ActionCallback = base::RepeatingCallback<void(const std::string& id)>;

  enum class Severity { kInfo, kWarning, kCritical };

  struct IssueInfo {
    std::string issue_id;
    std::u16string title;
    std::u16string description;
    Severity severity = Severity::kInfo;
    std::string action_label;
    std::string category;  // "memory", "tabs", "extensions", "storage", "privacy"
  };

  AstraHealthIssueRowView(const IssueInfo& info,
                          ActionCallback action_callback);
  ~AstraHealthIssueRowView() override;

  AstraHealthIssueRowView(const AstraHealthIssueRowView&) = delete;
  AstraHealthIssueRowView& operator=(const AstraHealthIssueRowView&) = delete;

  const std::string& issue_id() const { return issue_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnActionClicked();

  std::string issue_id_;
  std::u16string title_;
  std::u16string description_;
  Severity severity_;
  std::string action_label_;
  ActionCallback action_callback_;

  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> desc_label_ = nullptr;
  raw_ptr<views::MdTextButton> action_button_ = nullptr;
};

// =========================================================================
// AstraBrowserHealthView — browser health & cleanup panel
// =========================================================================
//
// A bubble / side panel showing browser health score, categorized
// optimization suggestions, and one-click cleanup actions.
//
// Layout:
//   +-------------------------------------------+
//   |  Browser Health               [Close]   |
//   +-------------------------------------------+
//   |  ╔══════════════════════════════╗        |
//   |  ║           72 / 100           ║        |
//   |  ║        Good overall          ║        |
//   |  ╚══════════════════════════════╝        |
//   +-------------------------------------------+
//   |  🧠 Memory         ████████░░  82%       |
//   |  📑 Tabs           █████░░░░░  55%       |
//   |  🧩 Extensions     ███░░░░░░░  30%       |
//   |  💾 Storage        ██████░░░░  60%       |
//   +-------------------------------------------+
//   |  Issues (5)                               |
//   |  ┌─────────────────────────────────────┐ |
//   |  │ ⚠️  YouTube is heavy    [Sleep]    │ |
//   |  │    320 MB · 22% CPU                 │ |
//   |  └─────────────────────────────────────┘ |
//   |  ┌─────────────────────────────────────┐ |
//   |  │ ℹ️  3 duplicate tabs    [Close]    │ |
//   |  │    example.com appears in 3 tabs    │ |
//   |  └─────────────────────────────────────┘ |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  [ Clean up all ]                         |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Health data comes from Astra's
// health service which aggregates from Chromium subsystems.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
//   - Custom Skia drawing for score dial and progress bars
//   - Process metrics (memory), TabStripModel (tabs),
//     ExtensionRegistry (extensions), StoragePartition (storage)
// =========================================================================

class AstraBrowserHealthView : public views::BubbleDialogDelegateView {
 public:
  using IssueActionCallback =
      base::RepeatingCallback<void(const std::string& issue_id)>;
  using CleanupAllCallback = base::RepeatingClosure;

  struct CategoryScore {
    std::string name;
    std::string emoji;
    int score;  // 0-100, higher is better
  };

  explicit AstraBrowserHealthView(views::View* anchor_view);
  ~AstraBrowserHealthView() override;

  AstraBrowserHealthView(const AstraBrowserHealthView&) = delete;
  AstraBrowserHealthView& operator=(
      const AstraBrowserHealthView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetOverallScore(int score);  // 0-100
  void SetCategoryScores(const std::vector<CategoryScore>& scores);

  void SetIssues(
      const std::vector<AstraHealthIssueRowView::IssueInfo>& issues);

  // -- Callbacks -----------------------------------------------------------

  void SetIssueActionCallback(IssueActionCallback callback);
  void SetCleanupAllCallback(CleanupAllCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  struct ScoreRow {
    std::string name;
    std::string emoji;
    int score = 0;
    raw_ptr<views::Label> label = nullptr;
    raw_ptr<views::View> bar = nullptr;
  };

  void BuildUI();
  void BuildScoreCard();
  void BuildCategoryScores();
  void BuildIssuesSection();
  void BuildCleanupButton();

  void RefreshScoreCard();
  void RefreshCategoryScores();
  void RefreshIssues();

  void OnIssueAction(const std::string& issue_id);
  void OnCleanupAllClicked();

  static std::u16string ScoreLabel(int score);
  static SkColor ScoreColor(int score);

  // Score data.
  int overall_score_ = 100;
  std::vector<CategoryScore> category_scores_;

  // Issues data.
  std::vector<AstraHealthIssueRowView::IssueInfo> issues_;

  // Callbacks.
  IssueActionCallback issue_action_callback_;
  CleanupAllCallback cleanup_all_callback_;

  // Child views.
  raw_ptr<views::Label> score_label_ = nullptr;
  raw_ptr<views::Label> score_desc_label_ = nullptr;
  raw_ptr<views::View> score_dial_ = nullptr;
  raw_ptr<views::View> categories_container_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> issues_list_ = nullptr;
  raw_ptr<views::Label> issues_count_label_ = nullptr;
  raw_ptr<views::MdTextButton> cleanup_button_ = nullptr;

  // Issue rows (owned by issues_list_).
  std::vector<raw_ptr<AstraHealthIssueRowView>> issue_rows_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_HEALTH_ASTRA_BROWSER_HEALTH_VIEW_H_
