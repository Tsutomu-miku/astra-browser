// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_FOCUS_SESSION_H_
#define ASTRA_BROWSER_ASTRA_FOCUS_SESSION_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace astra {

// Represents a single completed focus session stored in history.
struct AstraFocusSession {
  std::string id;
  base::Time start_time;
  std::optional<base::Time> end_time;  // Null for active sessions.
  base::TimeDelta duration;            // Total focused time, excluding pauses.
  int phase_work_count = 0;            // Number of work phases completed.
  int total_cycles = 0;                // Number of full pomodoro cycles.
  bool is_pomodoro = false;
  bool is_completed = false;           // False if ended early.
  int distraction_count = 0;           // Number of blocked distractions.
  bool whitelist_used = false;
  std::string note;                    // User-added note.
};

// Aggregate statistics computed from session history.
struct AstraFocusStats {
  int total_sessions = 0;
  base::TimeDelta total_focus_time;
  int sessions_this_week = 0;
  base::TimeDelta focus_time_this_week;
  base::TimeDelta longest_session;
  base::TimeDelta average_session;
  int current_streak_days = 0;
  int total_distractions_blocked = 0;
  int pomodoro_cycles_completed = 0;
};

// Computes statistics for the current calendar week (Monday–Sunday,
// local time) from the given session list.
//
// TODO(astra): Consider making week start day configurable.  Chromium
//   i18n / locale utilities may provide a better default weekday.
//   Chromium component: base::Time / i18n calendar helpers.
AstraFocusStats CalculateWeeklyStats(
    const std::vector<AstraFocusSession>& sessions);

// Computes all-time (total) statistics from the given session list.
AstraFocusStats CalculateTotalStats(
    const std::vector<AstraFocusSession>& sessions);

// Returns sessions whose start time falls within |start| (inclusive)
// and |end| (exclusive).
std::vector<AstraFocusSession> FilterSessionsByDateRange(
    const std::vector<AstraFocusSession>& sessions,
    base::Time start,
    base::Time end);

// Returns sessions that started on the same calendar day as |date|
// (local time).  Time-of-day components of |date| are ignored.
std::vector<AstraFocusSession> GetSessionsForDay(
    const std::vector<AstraFocusSession>& sessions,
    base::Time date);

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_FOCUS_SESSION_H_
