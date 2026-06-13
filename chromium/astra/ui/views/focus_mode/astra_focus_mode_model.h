#ifndef ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MODEL_H_
#define ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

class PrefService;

namespace astra {

// =========================================================================
// Enums and structs
// =========================================================================

// Position of the focus mode indicator on the screen.
enum class AstraIndicatorPosition {
  kLeft,
  kRight,
  kHidden,
};

// Visual style of the focus mode indicator.
enum class AstraIndicatorStyle {
  kMinimal,   // Small dot/badge only.
  kFull,      // Full indicator with label and timer.
  kBadge,     // Compact badge with icon only.
};

// Levels of distraction blocking.
// Each level builds on the previous one (cumulative).
enum class AstraFocusBlockLevel {
  kNone,          // No blocking.
  kSocial,        // Block social media.
  kEntertainment, // Block entertainment + social.
  kNews,          // Block news + entertainment + social.
  kStrict,        // Block everything except work sites.
  kCustom,        // Custom blocklist.
};

// A completed or in-progress focus session record.
struct AstraFocusSession {
  std::string session_id;
  base::Time start_time;
  base::Time end_time;
  base::TimeDelta duration;
  bool is_completed = false;
  AstraFocusBlockLevel block_level = AstraFocusBlockLevel::kNone;
  int distraction_count = 0;
  std::string note;
};

// =========================================================================
// AstraFocusModeObserver — observer interface for focus mode state changes
// =========================================================================
//
// Observer interface for focus mode model state changes.
// All methods have empty default implementations so observers only need to
// override the events they care about.
//
// Chromium pattern: base::CheckedObserver + base::ObserverList.
class AstraFocusModeObserver : public base::CheckedObserver {
 public:
  // Called when focus mode is started.
  virtual void OnFocusModeStarted(AstraFocusModeModel* model) {}

  // Called when focus mode ends.
  virtual void OnFocusModeEnded(AstraFocusModeModel* model) {}

  // Called when the focus session is paused.
  virtual void OnFocusModePaused(AstraFocusModeModel* model) {}

  // Called when the focus session is resumed from pause.
  virtual void OnFocusModeResumed(AstraFocusModeModel* model) {}

  // Called periodically when the remaining time updates.
  virtual void OnFocusTimeUpdated(AstraFocusModeModel* model,
                                  base::TimeDelta remaining) {}

  // Called when the block level changes.
  virtual void OnBlockLevelChanged(AstraFocusModeModel* model,
                                   AstraFocusBlockLevel level) {}

  // Called when the blocked sites list changes.
  virtual void OnBlockedSitesChanged(AstraFocusModeModel* model) {}

  // Called when a focus session is completed (naturally or manually).
  virtual void OnSessionCompleted(AstraFocusModeModel* model,
                                  const AstraFocusSession& session) {}

  // Called when the model is about to be destroyed.
  virtual void OnFocusModeModelShutdown(AstraFocusModeModel* model) {}

 protected:
  ~AstraFocusModeObserver() override = default;
};

// =========================================================================
// AstraFocusModeModel — state and settings model for focus mode UI
// =========================================================================
//
// The model owns all focus mode UI state and presentation settings.
// It is the single source of truth for the focus mode views layer.
//
// Truth hierarchy:
//   - AstraFocusModeService (profile-scoped): owns session truth (timers,
//     blocklists, stats, persistence).
//   - AstraFocusModeModel (views-layer): owns UI presentation state and
//     settings (indicator position, style, display preferences).
//   - Views (indicator, menu bubble): pure presentation, no state.
//
// The model persists presentation settings via PrefService.  Runtime session
// state (is_active, is_paused, elapsed time) is projected from the service
// through the controller — the model mirrors it for UI convenience.
//
// Chromium subsystems reused:
//   - PrefService (persistence for presentation settings).
//   - Observer pattern (base::CheckedObserver / base::ObserverList).
//
// Chromium patch point: none — this is pure Astra presentation-layer code.
// =========================================================================

class AstraFocusModeModel {
 public:
  // Constructs a model backed by |pref_service| for settings persistence.
  // |pref_service| may be null for unit tests; in that case, settings use
  // in-memory defaults only.
  explicit AstraFocusModeModel(PrefService* pref_service = nullptr);
  ~AstraFocusModeModel();

  AstraFocusModeModel(const AstraFocusModeModel&) = delete;
  AstraFocusModeModel& operator=(const AstraFocusModeModel&) = delete;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraFocusModeObserver* observer);
  void RemoveObserver(AstraFocusModeObserver* observer);

  // -- Focus state ---------------------------------------------------------

  // Sets whether focus mode is active.
  void SetActive(bool active);

  // Returns true if focus mode is currently active.
  bool IsActive() const;

  // Sets the total duration of the current focus session.
  void SetDuration(base::TimeDelta duration);

  // Returns the total duration of the current focus session.
  base::TimeDelta GetDuration() const;

  // Returns the time remaining in the current session.
  // Returns zero if no session is active.
  base::TimeDelta GetTimeRemaining() const;

  // Returns the time elapsed in the current session.
  base::TimeDelta GetElapsedTime() const;

  // Returns the start time of the current session.
  // Returns a null Time if no session is active.
  base::Time GetStartTime() const;

  // Returns true if a session is active and has time remaining.
  bool IsSessionActive() const;

  // Returns true if the current session is paused.
  bool IsSessionPaused() const;

  // Pauses the current session. No-op if not active or already paused.
  void PauseSession();

  // Resumes a paused session. No-op if not active or not paused.
  void ResumeSession();

  // Resets the current session to initial state (duration set but not active).
  void ResetSession();

  // Extends the current session by |extension|.
  // No-op if no session is active.
  void ExtendSession(base::TimeDelta extension);

  // Ends the current session. Records session in history if completed.
  void EndSession();

  // -- Distraction blocking ------------------------------------------------

  // Sets the current block level.
  void SetBlockLevel(AstraFocusBlockLevel level);

  // Returns the current block level.
  AstraFocusBlockLevel GetBlockLevel() const;

  // Adds a custom blocked site. Sets block level to kCustom if not already.
  void AddBlockedSite(const GURL& url);

  // Removes a custom blocked site.
  void RemoveBlockedSite(const GURL& url);

  // Returns the list of custom blocked sites.
  const std::vector<GURL>& GetBlockedSites() const;

  // Returns true if |url| should be blocked based on current block level
  // and custom blocked/allowed sites.
  // TODO(astra): Wire to Chromium content settings for actual site blocking.
  //   Chromium owner: HostContentSettingsMap / content_settings.
  //   Patch point: Use content settings pattern matching for site blocking.
  bool IsSiteBlocked(const GURL& url) const;

  // Adds an allowed site (bypasses blocking in strict mode).
  void AddAllowedSite(const GURL& url);

  // Removes an allowed site.
  void RemoveAllowedSite(const GURL& url);

  // Returns the list of allowed sites.
  const std::vector<GURL>& GetAllowedSites() const;

  // Clears all custom blocked sites.
  void ClearBlockedSites();

  // Clears all allowed sites.
  void ClearAllowedSites();

  // -- Focus sessions / history --------------------------------------------

  // Returns the list of past focus sessions.
  const std::vector<AstraFocusSession>& GetSessionHistory() const;

  // Returns the total number of sessions in history.
  size_t GetSessionCount() const;

  // Returns the number of sessions completed today.
  size_t GetTodaySessionCount() const;

  // Returns the total focus time completed today.
  base::TimeDelta GetTodayTotalFocusTime() const;

  // Returns the total focus time for the current week (7 days).
  base::TimeDelta GetWeekTotalFocusTime() const;

  // Clears all session history.
  void ClearSessionHistory();

  // Returns the current in-progress session, or nullopt if none.
  absl::optional<AstraFocusSession> GetCurrentSession() const;

  // Sets a note on the current session.
  void SetSessionNote(const std::string& note);

  // Returns the note for the current session, or empty string if none.
  std::string GetSessionNote() const;

  // -- UI settings ---------------------------------------------------------

  // Sets whether the focus indicator is shown in the toolbar.
  void SetShowIndicator(bool show);

  // Returns whether the focus indicator is shown.
  bool GetShowIndicator() const;

  // Sets the indicator position.
  void SetIndicatorPosition(AstraIndicatorPosition position);

  // Returns the indicator position.
  AstraIndicatorPosition GetIndicatorPosition() const;

  // Sets whether the countdown timer is shown.
  void SetShowTimer(bool show);

  // Returns whether the countdown timer is shown.
  bool GetShowTimer() const;

  // Sets whether sound is enabled at session end.
  void SetSoundEnabled(bool enabled);

  // Returns whether sound is enabled at session end.
  bool GetSoundEnabled() const;

  // Sets whether notifications are enabled at session end.
  void SetNotificationEnabled(bool enabled);

  // Returns whether notifications are enabled at session end.
  bool GetNotificationEnabled() const;

  // Sets whether break reminders are shown.
  void SetShowBreakReminder(bool show);

  // Returns whether break reminders are shown.
  bool GetShowBreakReminder() const;

  // Sets the break reminder interval.
  void SetBreakInterval(base::TimeDelta interval);

  // Returns the break reminder interval.
  base::TimeDelta GetBreakInterval() const;

  // -- Default durations ----------------------------------------------------

  // Sets the default session duration.
  void SetDefaultDuration(base::TimeDelta duration);

  // Returns the default session duration.
  base::TimeDelta GetDefaultDuration() const;

  // Returns the list of preset durations (25min, 45min, 60min, 90min).
  static const std::vector<base::TimeDelta>& GetPresetDurations();

  // Sets a preset at the given index.
  void SetPreset(int index, base::TimeDelta duration);

  // -- Presentation settings (legacy getters for compatibility) -------------
  // These are kept for backward compatibility with existing controller code.

  bool auto_start() const;
  void SetAutoStart(bool enabled);
  std::string auto_start_time() const;
  void SetAutoStartTime(const std::string& time_hhmm);
  int default_session_duration_minutes() const;
  void SetDefaultSessionDurationMinutes(int minutes);
  bool show_focus_indicator() const;
  void SetShowFocusIndicator(bool show);
  AstraIndicatorPosition indicator_position() const;
  void SetIndicatorPosition(AstraIndicatorPosition position);
  AstraIndicatorStyle indicator_style() const;
  void SetIndicatorStyle(AstraIndicatorStyle style);
  bool show_timer_in_indicator() const;
  void SetShowTimerInIndicator(bool show);
  bool show_session_stats() const;
  void SetShowSessionStats(bool show);
  bool block_distracting_sites() const;
  void SetBlockDistractingSites(bool block);
  bool show_distraction_warnings() const;
  void SetShowDistractionWarnings(bool show);
  bool notification_sound() const;
  void SetNotificationSound(bool enabled);
  bool break_reminders() const;
  void SetBreakReminders(bool enabled);
  int break_interval_minutes() const;
  void SetBreakIntervalMinutes(int minutes);
  int break_duration_minutes() const;
  void SetBreakDurationMinutes(int minutes);
  bool dim_non_focus_tabs() const;
  void SetDimNonFocusTabs(bool dim);
  bool hide_sidebar_in_focus_mode() const;
  void SetHideSidebarInFocusMode(bool hide);

  // -- Session state (legacy getters for compatibility) --------------------

  bool is_active() const { return is_active_; }
  bool is_paused() const { return is_paused_; }
  base::Time session_start_time() const { return session_start_time_; }
  base::TimeDelta elapsed_time() const { return elapsed_time_; }
  base::TimeDelta remaining_time() const { return remaining_time_; }
  base::TimeDelta total_duration() const { return total_duration_; }
  const std::string& current_preset() const { return current_preset_; }
  size_t distractions_blocked() const { return distractions_blocked_; }

  void SetPaused(bool paused);
  void SetSessionStartTime(base::Time start_time);
  void SetElapsedTime(base::TimeDelta elapsed);
  void SetRemainingTime(base::TimeDelta remaining);
  void SetTotalDuration(base::TimeDelta duration);
  void SetCurrentPreset(const std::string& preset_name);
  void SetDistractionsBlocked(size_t count);
  void IncrementDistractionsBlocked(const GURL& url);

  // -- Session stats (legacy getters for compatibility) --------------------

  int total_focus_minutes_today() const { return total_focus_minutes_today_; }
  void SetTotalFocusMinutesToday(int minutes);
  void AddFocusMinutesToday(int minutes);
  int sessions_today() const { return sessions_today_; }
  void SetSessionsToday(int count);
  void IncrementSessionsToday();
  int current_streak_days() const { return current_streak_days_; }
  void SetCurrentStreakDays(int days);

  // -- Session presets (legacy) --------------------------------------------

  // Returns the list of default session preset durations (in minutes).
  static const std::vector<int>& GetDefaultPresetDurations();

  // -- Utility methods ------------------------------------------------------

  // Formats a TimeDelta as a compact duration string.
  static std::u16string FormatDuration(base::TimeDelta duration);

  // Formats time remaining in a session.
  static std::u16string FormatTimeRemaining(base::TimeDelta remaining);

  // Calculates progress percentage (0.0 to 1.0).
  static double CalculateProgressPercentage(base::TimeDelta elapsed,
                                           base::TimeDelta total);

  // Calculates progress percentage from current session state.
  double SessionProgress() const;

  // Resets all runtime session state to defaults (does not change settings).
  void ResetSessionState();

  // Resets all stats to zero.
  void ResetStats();

 private:
  // Generates a unique session ID.
  static std::string GenerateSessionId();

  // Records the current session as completed and adds it to history.
  void RecordCompletedSession();

  // Loads presentation settings from PrefService.
  void LoadFromPrefs();

  // Notifies observers that focus mode started.
  void NotifyFocusModeStarted();

  // Notifies observers that focus mode ended.
  void NotifyFocusModeEnded();

  // Notifies observers that focus mode was paused.
  void NotifyFocusModePaused();

  // Notifies observers that focus mode was resumed.
  void NotifyFocusModeResumed();

  // Notifies observers that time remaining was updated.
  void NotifyFocusTimeUpdated();

  // Notifies observers that block level changed.
  void NotifyBlockLevelChanged();

  // Notifies observers that blocked sites changed.
  void NotifyBlockedSitesChanged();

  // Notifies observers that a session was completed.
  void NotifySessionCompleted(const AstraFocusSession& session);

  // Notifies observers that the model is shutting down.
  void NotifyModelShutdown();

  raw_ptr<PrefService> pref_service_ = nullptr;
  base::ObserverList<AstraFocusModeObserver> observers_;

  // -- Runtime session state -----------------------------------------------

  bool is_active_ = false;
  bool is_paused_ = false;
  base::Time session_start_time_;
  base::TimeDelta elapsed_time_;
  base::TimeDelta remaining_time_;
  base::TimeDelta total_duration_;
  std::string current_preset_;
  size_t distractions_blocked_ = 0;
  std::string session_note_;

  // -- Distraction blocking ------------------------------------------------

  AstraFocusBlockLevel block_level_ = AstraFocusBlockLevel::kNone;
  std::vector<GURL> blocked_sites_;
  std::vector<GURL> allowed_sites_;

  // -- Session history -----------------------------------------------------

  std::vector<AstraFocusSession> session_history_;

  // -- Session stats (today) -----------------------------------------------

  int total_focus_minutes_today_ = 0;
  int sessions_today_ = 0;
  int current_streak_days_ = 0;

  // -- UI settings (in-memory defaults) ------------------------------------
  // These mirror the pref-backed settings but are kept in memory for fast
  // access and to support null pref_service (tests).

  bool show_indicator_ = true;
  AstraIndicatorPosition indicator_position_ = AstraIndicatorPosition::kRight;
  bool show_timer_ = true;
  bool sound_enabled_ = true;
  bool notification_enabled_ = true;
  bool show_break_reminder_ = false;
  base::TimeDelta break_interval_ = base::Minutes(25);
  base::TimeDelta default_duration_ = base::Minutes(25);
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MODEL_H_
