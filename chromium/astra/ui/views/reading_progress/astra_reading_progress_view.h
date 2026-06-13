// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_READING_PROGRESS_ASTRA_READING_PROGRESS_VIEW_H_
#define ASTRA_UI_VIEWS_READING_PROGRESS_ASTRA_READING_PROGRESS_VIEW_H_

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
class Slider;
}  // namespace views

namespace astra {

// =========================================================================
// AstraReadingProgressItemView — single reading progress item
// =========================================================================
//
// A card showing one article's reading progress: title, domain, progress
// bar, estimated time remaining.
//
// Layout:
//   +-------------------------------------------+
//   |  The Future of AI                          |
//   |  techreview.com                            |
//   |  [██████████░░░░] 62% · 4 min left        |
//   +-------------------------------------------+
// =========================================================================

class AstraReadingProgressItemView : public views::View {
 public:
  using OpenCallback =
      base::RepeatingCallback<void(const std::string& article_id)>;
  using RemoveCallback =
      base::RepeatingCallback<void(const std::string& article_id)>;

  struct ArticleInfo {
    std::string article_id;
    std::u16string title;
    std::u16string domain;
    int progress_percent = 0;  // 0-100
    int total_words = 0;
    int read_words = 0;
    base::TimeDelta time_remaining;
    base::Time last_read;
  };

  AstraReadingProgressItemView(const ArticleInfo& info,
                               OpenCallback open_callback,
                               RemoveCallback remove_callback);
  ~AstraReadingProgressItemView() override;

  AstraReadingProgressItemView(const AstraReadingProgressItemView&) = delete;
  AstraReadingProgressItemView& operator=(
      const AstraReadingProgressItemView&) = delete;

  const std::string& article_id() const { return article_id_; }
  int progress_percent() const { return progress_percent_; }

  void SetProgress(int percent);

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnOpenClicked();
  void OnRemoveClicked();
  void PaintProgressBar(gfx::Canvas* canvas);

  static std::u16 FormatTimeRemaining(base::TimeDelta delta);

  std::string article_id_;
  std::u16string title_;
  std::u16string domain_;
  int progress_percent_ = 0;
  int total_words_ = 0;
  int read_words_ = 0;
  base::TimeDelta time_remaining_;
  base::Time last_read_;

  OpenCallback open_callback_;
  RemoveCallback remove_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::MdTextButton> open_button_ = nullptr;
  raw_ptr<views::MdTextButton> remove_button_ = nullptr;
};

// =========================================================================
// AstraReadingProgressView — reading progress panel
// =========================================================================
//
// A bubble showing articles currently being read with progress tracking,
// reading time estimates, and reading stats.
//
// Layout:
//   +-------------------------------------------+
//   |  Reading Progress             [Close]    |
//   +-------------------------------------------+
//   |  This week: 12 articles · 3h 24m read     |
//   |  Streak: 7 days                            |
//   +-------------------------------------------+
//   |  Currently reading (3)                     |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ The Future of AI                      │  |
//   │  │ techreview.com                       │  |
//   │  │ [██████████░░░░] 62% · 4 min left   │  |
//   │  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   │  │ Deep Learning Fundamentals           │  |
//   │  │ arxiv.org                            │  |
//   │  │ [█████░░░░░░░░░░░] 28% · 12 min left│  |
//   │  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Reading progress data comes from Astra's
// reading progress service which tracks scroll position and reading time.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - content::WebContents (for scroll observation)
// =========================================================================

class AstraReadingProgressView : public views::BubbleDialogDelegateView {
 public:
  using OpenArticleCallback =
      base::RepeatingCallback<void(const std::string& article_id)>;
  using RemoveArticleCallback =
      base::RepeatingCallback<void(const std::string& article_id)>;
  using ViewAllCallback = base::RepeatingClosure;

  struct WeeklyStats {
    int articles_read = 0;
    base::TimeDelta total_read_time;
    int current_streak_days = 0;
    int longest_streak_days = 0;
  };

  explicit AstraReadingProgressView(views::View* anchor_view);
  ~AstraReadingProgressView() override;

  AstraReadingProgressView(const AstraReadingProgressView&) = delete;
  AstraReadingProgressView& operator=(const AstraReadingProgressView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetArticles(
      const std::vector<AstraReadingProgressItemView::ArticleInfo>& articles);
  void SetWeeklyStats(const WeeklyStats& stats);

  // -- Callbacks -----------------------------------------------------------

  void SetOpenArticleCallback(OpenArticleCallback callback);
  void SetRemoveArticleCallback(RemoveArticleCallback callback);
  void SetViewAllCallback(ViewAllCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildStatsSection();
  void BuildArticlesList();

  void RefreshArticles();
  void RefreshStats();

  static std::u16 FormatReadTime(base::TimeDelta delta);

  std::vector<AstraReadingProgressItemView::ArticleInfo> articles_;
  WeeklyStats stats_;

  OpenArticleCallback open_callback_;
  RemoveArticleCallback remove_callback_;
  ViewAllCallback view_all_callback_;

  raw_ptr<views::Label> articles_read_label_ = nullptr;
  raw_ptr<views::Label> streak_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::View> articles_list_ = nullptr;
  raw_ptr<views::MdTextButton> view_all_button_ = nullptr;

  std::vector<raw_ptr<AstraReadingProgressItemView>> article_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_READING_PROGRESS_ASTRA_READING_PROGRESS_VIEW_H_
