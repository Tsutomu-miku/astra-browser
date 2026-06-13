// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_focus_session.h"

#include <algorithm>

#include "base/check.h"

namespace astra {

namespace {

// Returns the start of the day (midnight, local time) for |time|.
base::Time StartOfDay(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  base::Time result;
  bool ok = base::Time::FromLocalExploded(exploded, &result);
  DCHECK(ok);
  return result;
}

// Returns the start of the current week (Monday midnight, local time).
// Week is defined as Monday–Sunday.
base::Time StartOfWeek(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  // day_of_week: 0 = Sunday, 1 = Monday, ..., 6 = Saturday.
  // Convert to Monday-based: Monday = 0, Tuesday = 1, ..., Sunday = 6.
  int monday_based = (exploded.day_of_week + 6) % 7;

  base::Time start_of_today = StartOfDay(time);
  return start_of_today - base::Days(monday_based);
}

// Returns true if |session| started on the same calendar day as |date|
// (local time).
bool IsSameDay(const AstraFocusSession& session, base::Time date) {
  base::Time day_start = StartOfDay(date);
  base::Time day_end = day_start + base::Days(1);
  return session.start_time >= day_start && session.start_time < day_end;
}

}  // namespace

AstraFocusStats CalculateWeeklyStats(
    const std::vector<AstraFocusSession>& sessions) {
  base::Time now = base::Time::Now();
  base::Time week_start = StartOfWeek(now);
  base::Time week_end = week_start + base::Days(7);

  std::vector<AstraFocusSession> weekly =
      FilterSessionsByDateRange(sessions, week_start, week_end);

  AstraFocusStats stats = CalculateTotalStats(weekly);

  // The all-total fields in |stats| are actually the weekly totals since
  // we filtered to this week.  Reassign to the weekly-specific fields
  // and compute the rest from the full list.
  AstraFocusStats full_stats = CalculateTotalStats(sessions);

  // Populate weekly fields from the filtered set.
  full_stats.sessions_this_week = stats.total_sessions;
  full_stats.focus_time_this_week = stats.total_focus_time;

  // Note: longest_session, average_session, current_streak_days, etc.
  // are all-time values already computed by CalculateTotalStats.
  return full_stats;
}

AstraFocusStats CalculateTotalStats(
    const std::vector<AstraFocusSession>& sessions) {
  AstraFocusStats stats;

  if (sessions.empty()) {
    return stats;
  }

  base::TimeDelta longest;
  base::TimeDelta total_focus;
  int total_distractions = 0;
  int total_cycles = 0;

  for (const auto& session : sessions) {
    stats.total_sessions++;
    total_focus += session.duration;
    total_distractions += session.distraction_count;
    total_cycles += session.total_cycles;

    if (session.duration > longest) {
      longest = session.duration;
    }
  }

  stats.total_focus_time = total_focus;
  stats.longest_session = longest;
  stats.total_distractions_blocked = total_distractions;
  stats.pomodoro_cycles_completed = total_cycles;

  if (stats.total_sessions > 0) {
    stats.average_session = total_focus / stats.total_sessions;
  }

  // Compute current streak (consecutive days with at least one session,
  // going backward from today).
  base::Time today = StartOfDay(base::Time::Now());
  int streak = 0;
  base::Time current_day = today;

  // Check today first, then go backward.
  while (true) {
    bool has_session = false;
    base::Time day_end = current_day + base::Days(1);
    for (const auto& session : sessions) {
      if (session.start_time >= current_day && session.start_time < day_end) {
        has_session = true;
        break;
      }
    }
    if (has_session) {
      streak++;
      current_day -= base::Days(1);
    } else {
      break;
    }
    // Safety cap to avoid infinite loop in edge cases.
    if (streak > 3650) {
      break;
    }
  }

  stats.current_streak_days = streak;

  return stats;
}

std::vector<AstraFocusSession> FilterSessionsByDateRange(
    const std::vector<AstraFocusSession>& sessions,
    base::Time start,
    base::Time end) {
  std::vector<AstraFocusSession> result;
  for (const auto& session : sessions) {
    if (session.start_time >= start && session.start_time < end) {
      result.push_back(session);
    }
  }
  return result;
}

std::vector<AstraFocusSession> GetSessionsForDay(
    const std::vector<AstraFocusSession>& sessions,
    base::Time date) {
  std::vector<AstraFocusSession> result;
  for (const auto& session : sessions) {
    if (IsSameDay(session, date)) {
      result.push_back(session);
    }
  }
  return result;
}

}  // namespace astra
