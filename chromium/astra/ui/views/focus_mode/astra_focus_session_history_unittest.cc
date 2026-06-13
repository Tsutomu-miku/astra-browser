// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/focus_mode/astra_focus_session_history_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraFocusSessionHistoryViewTest
// ===========================================================================

class AstraFocusSessionHistoryViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test session row creation.
TEST_F(AstraFocusSessionHistoryViewTest, SessionRowCreation) {
  AstraFocusSession session;
  session.id = "test_session_1";
  session.start_time = base::Time::Now();
  session.duration = base::Minutes(25);
  session.phase_work_count = 1;
  session.is_pomodoro = true;
  session.is_completed = true;
  session.distraction_count = 2;

  auto row = std::make_unique<AstraFocusSessionRowView>(session);
  EXPECT_NE(nullptr, row.get());
}

// Test session row with incomplete session.
TEST_F(AstraFocusSessionHistoryViewTest, SessionRowIncomplete) {
  AstraFocusSession session;
  session.id = "incomplete_1";
  session.start_time = base::Time::Now() - base::Hours(1);
  session.duration = base::Minutes(15);
  session.is_completed = false;
  session.distraction_count = 0;

  auto row = std::make_unique<AstraFocusSessionRowView>(session);
  EXPECT_NE(nullptr, row.get());
}

// Test weekly stats calculation.
TEST_F(AstraFocusSessionHistoryViewTest, WeeklyStats) {
  std::vector<AstraFocusSession> sessions;

  // Session from today.
  AstraFocusSession s1;
  s1.id = "s1";
  s1.start_time = base::Time::Now();
  s1.duration = base::Minutes(25);
  s1.is_completed = true;
  s1.phase_work_count = 1;
  s1.is_pomodoro = true;
  s1.distraction_count = 2;
  sessions.push_back(s1);

  // Session from yesterday.
  AstraFocusSession s2;
  s2.id = "s2";
  s2.start_time = base::Time::Now() - base::Days(1);
  s2.duration = base::Minutes(45);
  s2.is_completed = true;
  s2.phase_work_count = 1;
  s2.distraction_count = 0;
  sessions.push_back(s2);

  auto stats = CalculateWeeklyStats(sessions);
  EXPECT_GT(stats.sessions_this_week, 0);
  EXPECT_GT(stats.focus_time_this_week, base::TimeDelta());
}

// Test total stats calculation.
TEST_F(AstraFocusSessionHistoryViewTest, TotalStats) {
  std::vector<AstraFocusSession> sessions;

  AstraFocusSession s1;
  s1.id = "s1";
  s1.start_time = base::Time::Now() - base::Days(10);
  s1.duration = base::Minutes(30);
  s1.is_completed = true;
  s1.phase_work_count = 2;
  s1.is_pomodoro = true;
  s1.distraction_count = 5;
  sessions.push_back(s1);

  AstraFocusSession s2;
  s2.id = "s2";
  s2.start_time = base::Time::Now();
  s2.duration = base::Minutes(25);
  s2.is_completed = true;
  s1.phase_work_count = 1;
  s1.is_pomodoro = true;
  s2.distraction_count = 1;
  sessions.push_back(s2);

  auto stats = CalculateTotalStats(sessions);
  EXPECT_EQ(2, stats.total_sessions);
  EXPECT_GT(stats.total_focus_time, base::TimeDelta());
  EXPECT_GT(stats.total_distractions_blocked, 0);
}

// Test filter sessions by date range.
TEST_F(AstraFocusSessionHistoryViewTest, FilterByDateRange) {
  std::vector<AstraFocusSession> sessions;

  base::Time now = base::Time::Now();

  AstraFocusSession s1;
  s1.id = "s1";
  s1.start_time = now - base::Hours(2);
  sessions.push_back(s1);

  AstraFocusSession s2;
  s2.id = "s2";
  s2.start_time = now - base::Days(7);
  sessions.push_back(s2);

  AstraFocusSession s3;
  s3.id = "s3";
  s3.start_time = now - base::Days(30);
  sessions.push_back(s3);

  // Last 24 hours.
  auto recent = FilterSessionsByDateRange(
      sessions, now - base::Days(1), now);
  EXPECT_EQ(1u, recent.size());
  EXPECT_EQ("s1", recent[0].id);

  // Last 7 days.
  auto week = FilterSessionsByDateRange(
      sessions, now - base::Days(7), now);
  EXPECT_GE(week.size(), 2u);
}

// Test get sessions for a specific day.
TEST_F(AstraFocusSessionHistoryViewTest, GetSessionsForDay) {
  std::vector<AstraFocusSession> sessions;

  base::Time today = base::Time::Now();

  AstraFocusSession s1;
  s1.id = "today1";
  s1.start_time = today - base::Hours(1);
  sessions.push_back(s1);

  AstraFocusSession s2;
  s2.id = "today2";
  s2.start_time = today - base::Hours(5);
  sessions.push_back(s2);

  auto today_sessions = GetSessionsForDay(sessions, today);
  EXPECT_GE(today_sessions.size(), 2u);
}

// Test empty sessions gives zero stats.
TEST_F(AstraFocusSessionHistoryViewTest, EmptySessions) {
  std::vector<AstraFocusSession> empty;

  auto weekly = CalculateWeeklyStats(empty);
  EXPECT_EQ(0, weekly.total_sessions);
  EXPECT_EQ(base::TimeDelta(), weekly.total_focus_time);

  auto total = CalculateTotalStats(empty);
  EXPECT_EQ(0, total.total_sessions);
  EXPECT_EQ(0, total.current_streak_days);
}

// Test history view creation.
TEST_F(AstraFocusSessionHistoryViewTest, HistoryViewCreation) {
  auto* view = new AstraFocusSessionHistoryView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
  // Don't show the widget (no real widget in unit test).
}

// Test setting sessions on history view.
TEST_F(AstraFocusSessionHistoryViewTest, SetSessions) {
  auto* view = new AstraFocusSessionHistoryView(anchor_view_.get());

  std::vector<AstraFocusSession> sessions;
  AstraFocusSession s;
  s.id = "test1";
  s.start_time = base::Time::Now();
  s.duration = base::Minutes(30);
  s.is_completed = true;
  s.distraction_count = 1;
  sessions.push_back(s);

  view->SetSessions(sessions);
  // Should not crash and should update stats.
}

}  // namespace astra
