// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_SESSION_HISTORY_VIEW_H_
#define ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_SESSION_HISTORY_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

#include "astra/browser/astra_focus_session.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraFocusSessionRowView — single session history row
// =========================================================================
//
// A row in the session history list.  Shows date, duration, completion
// status, and distraction count for a single focus session.
//
// Layout:
//   +---------------------------------------------+
//   |  📅 Jun 12       45m 00s      ✅ Completed  |
//   |  Work phase: 2  ·  Pomodoro  ·  3 blocked   |
//   +---------------------------------------------+
// =========================================================================

class AstraFocusSessionRowView : public views::View {
 public:
  explicit AstraFocusSessionRowView(const AstraFocusSession& session);
  ~AstraFocusSessionRowView() override;

  AstraFocusSessionRowView(const AstraFocusSessionRowView&) = delete;
  AstraFocusSessionRowView& operator=(const AstraFocusSessionRowView&) = delete;

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void UpdateFromSession();

  AstraFocusSession session_;

  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::Label> duration_label_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::Label> details_label_ = nullptr;
};

// =========================================================================
// AstraFocusSessionHistoryView — focus session history panel
// =========================================================================
//
// A bubble / side panel showing focus session statistics and history.
//
// Layout:
//   +-------------------------------------------+
//   |  Focus History                    [Close]|
//   +-------------------------------------------+
//   |  This Week                                |
//   |  ┌─────────┬─────────┬─────────┐         |
//   |  │  3h 15m │  4 sess │  5 day  │         |
//   |  │  focus  │ completed │ streak │        |
//   |  └─────────┴─────────┴─────────┘         |
//   +-------------------------------------------+
//   |  All Time                                 |
//   |  Total focus: 12h 30m                     |
//   |  Sessions: 18                             |
//   |  Distractions blocked: 47                 |
//   |  Pomodoro cycles: 36                      |
//   +-------------------------------------------+
//   |  Recent Sessions                          |
//   |  ┌─────────────────────────────────┐     |
//   |  │ 📅 Today   45m  ✅  2 phases   │     |
//   |  │ 📅 Yesterday  1h 30m  ✅  3 ph │     |
//   |  │ 📅 Jun 10  25m  ⏹  1 phase    │     |
//   |  └─────────────────────────────────┘     |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Session data is pushed in from
// the controller / service layer.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
// =========================================================================

class AstraFocusSessionHistoryView
    : public views::BubbleDialogDelegateView {
 public:
  explicit AstraFocusSessionHistoryView(views::View* anchor_view);
  ~AstraFocusSessionHistoryView() override;

  AstraFocusSessionHistoryView(const AstraFocusSessionHistoryView&) = delete;
  AstraFocusSessionHistoryView& operator=(
      const AstraFocusSessionHistoryView&) = delete;

  // -- Data setters --------------------------------------------------------

  void SetSessions(const std::vector<AstraFocusSession>& sessions);

  // -- Statistics ----------------------------------------------------------

  void SetWeeklyStats(const AstraFocusStats& stats);
  void SetTotalStats(const AstraFocusStats& stats);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildWeeklyStats();
  void BuildTotalStats();
  void BuildSessionList();

  void RefreshStats();
  void RefreshSessionList();

  std::u16string FormatDuration(base::TimeDelta duration) const;

  // Stats.
  AstraFocusStats weekly_stats_;
  AstraFocusStats total_stats_;

  // Sessions (for list display).
  std::vector<AstraFocusSession> sessions_;

  // Weekly stat cards.
  raw_ptr<views::View> weekly_focus_card_ = nullptr;
  raw_ptr<views::View> weekly_sessions_card_ = nullptr;
  raw_ptr<views::View> weekly_streak_card_ = nullptr;

  raw_ptr<views::Label> weekly_focus_value_ = nullptr;
  raw_ptr<views::Label> weekly_sessions_value_ = nullptr;
  raw_ptr<views::Label> weekly_streak_value_ = nullptr;

  // Total stats.
  raw_ptr<views::Label> total_focus_label_ = nullptr;
  raw_ptr<views::Label> total_sessions_label_ = nullptr;
  raw_ptr<views::Label> total_distractions_label_ = nullptr;
  raw_ptr<views::Label> total_pomodoro_label_ = nullptr;

  // Session list.
  raw_ptr<views::ScrollView> session_scroll_ = nullptr;
  raw_ptr<views::View> session_list_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_SESSION_HISTORY_VIEW_H_
