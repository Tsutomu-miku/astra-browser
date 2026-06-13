#ifndef ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

#include "astra/browser/astra_focus_session.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

// Focus / pomodoro phase type.
//
// Pomodoro cycle pattern: Work → Short Break → Work → Short Break →
// Work → Short Break → Work → Long Break → (repeat)
//
// Classic Pomodoro: 25 min work, 5 min short break, 15 min long break,
// long break every 4 work sessions.
enum class AstraFocusPhase {
  kWork,
  kShortBreak,
  kLongBreak,
};

// =========================================================================
// AstraFocusModeServiceObserver
// =========================================================================

class AstraFocusModeServiceObserver : public base::CheckedObserver {
 public:
  // Called when focus mode is entered.
  // |duration| is the total duration of the focus session.
  virtual void OnFocusModeEntered(base::TimeDelta duration) {}

  // Called when focus mode is exited.
  virtual void OnFocusModeExited() {}

  // Called periodically (every second) during an active focus session
  // so UI can update the remaining time display.
  virtual void OnFocusTimeUpdated(base::TimeDelta remaining) {}

  // Called when the distraction blocklist changes (site added/removed).
  virtual void OnDistractionBlocklistChanged() {}

  // Called when the focus / pomodoro phase changes.
  // |new_phase| is the phase that just started.
  // Only fired when pomodoro mode is active.
  virtual void OnFocusPhaseChanged(AstraFocusPhase new_phase) {}

  // Called when a full pomodoro cycle (N work sessions + long break)
  // is completed.  |cycle_count| is the number of completed cycles.
  virtual void OnPomodoroCycleCompleted(int cycle_count) {}

  // Called when the focus session is paused.
  virtual void OnFocusSessionPaused() {}

  // Called when the focus session is resumed from pause.
  virtual void OnFocusSessionResumed() {}

  // Called when the focus session ends (completes naturally or is ended
  // manually).  |total_duration| is the actual time spent in focus.
  virtual void OnFocusSessionCompleted(base::TimeDelta total_duration) {}

  // Called when the whitelist (always-allowed sites) changes.
  virtual void OnWhitelistChanged() {}

  // Called when a distraction warning is triggered (user navigated to a
  // blocked site during focus mode).  |url| is the blocked URL.
  virtual void OnDistractionWarning(const std::string& url) {}

  // Called when session stats are updated (total focus time, sessions
  // completed, etc.).
  virtual void OnStatsUpdated() {}

  // Called when session presets change (added, removed, modified).
  virtual void OnPresetsChanged() {}

  // Called when auto-start settings change.
  virtual void OnAutoStartSettingsChanged() {}

  // Called when a completed session is added to history.
  virtual void OnSessionAddedToHistory() {}

  // Called when session history is cleared.
  virtual void OnSessionHistoryCleared() {}

 protected:
  ~AstraFocusModeServiceObserver() override = default;
};

class AstraFocusModeService final : public KeyedService {
 public:
  explicit AstraFocusModeService(Profile* profile);
  AstraFocusModeService(const AstraFocusModeService&) = delete;
  AstraFocusModeService& operator=(const AstraFocusModeService&) = delete;
  ~AstraFocusModeService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraFocusModeServiceObserver* observer);
  void RemoveObserver(AstraFocusModeServiceObserver* observer);

  // -- Focus mode control ------------------------------------------------

  // Enters focus mode for |duration|. If focus mode is already active,
  // extends the session by |duration|.
  // Note: this starts a simple (non-pomodoro) focus session.
  void EnterFocusMode(base::TimeDelta duration);

  // Exits focus mode immediately.  Also stops pomodoro mode if active.
  void ExitFocusMode();

  // Toggles focus mode on/off. When turning on, uses the default duration.
  void ToggleFocusMode();

  // Returns true if focus mode is currently active (any phase).
  bool IsFocusModeActive() const;

  // Returns the remaining time in the current phase.
  // Returns base::TimeDelta() if focus mode is not active.
  base::TimeDelta GetRemainingTime() const;

  // Returns the total duration of the current phase.
  // Returns base::TimeDelta() if focus mode is not active.
  base::TimeDelta GetTotalDuration() const;

  // Extends the current phase by |additional_duration|.
  // No-op if focus mode is not active.
  void ExtendFocusSession(base::TimeDelta additional_duration);

  // -- Pomodoro mode -----------------------------------------------------

  // Starts a pomodoro cycle: work → short break → work → ... → long break.
  // Begins with a work session using the default focus duration.
  // If pomodoro is already active, resets the cycle counter.
  void StartPomodoro();

  // Starts a break phase (short or long).
  // If focus mode is not active, starts a break-only session.
  // If a work session is active, switches to break phase.
  // |is_long| if true starts a long break, otherwise short break.
  void StartBreak(bool is_long = false);

  // Skips the current break phase and starts the next work session.
  // No-op if not currently in a break phase.
  void SkipBreak();

  // Returns the current focus / pomodoro phase.
  AstraFocusPhase GetCurrentPhase() const;

  // Returns the number of completed work sessions in the current
  // pomodoro run.  Resets when pomodoro mode is stopped or restarted.
  int GetCompletedWorkSessions() const;

  // Returns the number of full pomodoro cycles completed.
  // A full cycle = N work sessions + a long break (where N = long break interval).
  int GetCycleCount() const;

  // Whether pomodoro cycle mode is active.
  // Pomodoro mode is active when StartPomodoro() has been called and
  // focus mode is running in a work-break cycle pattern.
  bool pomodoro_mode_active() const { return pomodoro_mode_active_; }

  // -- Session pause / resume ------------------------------------------------

  // Pauses the current focus session.  The timer stops and remaining time
  // is preserved.  No-op if not active or already paused.
  void PauseSession();

  // Resumes a paused focus session.  The timer restarts from where it
  // left off.  No-op if not active or not paused.
  void ResumeSession();

  // Returns true if the current focus session is paused.
  bool IsSessionPaused() const;

  // -- Session stats --------------------------------------------------------

  // Returns the total accumulated focus time across all completed sessions.
  // Persisted across browser restarts via PrefService.
  base::TimeDelta GetTotalFocusTime() const;

  // Returns the total number of completed focus sessions.
  int GetSessionsCompleted() const;

  // Returns the total number of completed pomodoro cycles.
  int GetTotalCyclesCompleted() const;

  // Resets all cumulative session statistics to zero.
  void ResetStats();

  // -- Session history ------------------------------------------------------

  // Returns the full session history (most recent sessions first).
  // TODO(astra): Wire session history to PrefService for persistence across
  //   browser restarts.  Chromium component: PrefService / ListPref.
  //   Chromium owner: PrefService (components/prefs/pref_service.h)
  const std::vector<AstraFocusSession>& GetSessionHistory() const;

  // Computes and returns aggregate focus stats from session history.
  // Includes both all-time and weekly stats.
  AstraFocusStats GetFocusStats() const;

  // Adds or updates a user note on a session.  Returns true if the session
  // was found and the note was set.
  bool AddSessionNote(const std::string& session_id, const std::string& note);

  // Clears all session history.  Does not affect cumulative stats.
  void ClearSessionHistory();

  // Maximum number of sessions kept in history.  When the limit is reached,
  // oldest sessions are dropped.  Default: 100.
  size_t max_history_entries() const { return max_history_entries_; }
  void set_max_history_entries(size_t max);

  // -- Distraction blocklist ---------------------------------------------

  // Adds a URL pattern to the distraction blocklist.
  // Patterns follow Chromium content settings pattern format
  // (e.g., "youtube.com", "*.reddit.com").
  // TODO(astra): Use Chromium's content settings pattern matching.
  // Chromium component: ContentSettingsPattern
  //   (components/content_settings/core/common/content_settings_pattern.h)
  void AddDistractionSite(const std::string& url_pattern);

  // Removes a URL pattern from the distraction blocklist.
  void RemoveDistractionSite(const std::string& url_pattern);

  // Returns the full list of distraction site patterns.
  const std::vector<std::string>& distraction_blocklist() const {
    return distraction_blocklist_;
  }

  // Returns true if |url| matches any pattern in the blocklist.
  // TODO(astra): Implement proper pattern matching using
  // ContentSettingsPattern or URL matcher utilities.
  // Chromium component: url::Origin or GURL matching utilities.
  bool IsSiteBlocked(const std::string& url) const;

  // -- Whitelist (allowed sites) -------------------------------------------

  // Adds a URL pattern to the whitelist (always-allowed sites).
  // Whitelisted sites bypass the distraction blocklist during focus mode.
  // Patterns use the same format as the blocklist.
  void AddWhitelistedSite(const std::string& url_pattern);

  // Removes a URL pattern from the whitelist.
  void RemoveWhitelistedSite(const std::string& url_pattern);

  // Returns the full list of whitelisted site patterns.
  const std::vector<std::string>& whitelist() const { return whitelist_; }

  // Returns true if |url| matches any pattern in the whitelist.
  bool IsSiteWhitelisted(const std::string& url) const;

  // -- Distraction warnings ------------------------------------------------

  // Triggers a distraction warning for the given URL.
  // Called when the user navigates to a blocked site during focus mode.
  // Returns true if a warning was actually triggered (warnings enabled and
  // site is blocked).
  //
  // Note: actual navigation blocking is performed by Chromium's NavigationThrottle
  // or content settings.  This method only records and projects the warning state.
  // Chromium component: NavigationThrottle / HostContentSettingsMap.
  bool TriggerDistractionWarning(const std::string& url);

  // Whether distraction warnings are enabled.
  bool warnings_enabled() const { return warnings_enabled_; }
  void set_warnings_enabled(bool enabled);

  // -- Session presets ----------------------------------------------------

  // A session preset is a named focus configuration (duration + optional
  // break duration).  Users can save commonly-used focus configurations and
  // quickly start a session from a preset.
  struct FocusPreset {
    std::string id;       // Unique identifier.
    std::string name;     // User-visible name, e.g. "Deep work".
    int duration_minutes = 25;   // Focus duration in minutes.
    int break_duration_minutes = 5;  // Break duration in minutes (for pomodoro).
  };

  // Adds or updates a preset.  If a preset with the same id exists, it is
  // replaced.  Notifies OnPresetsChanged.
  void SavePreset(const FocusPreset& preset);

  // Removes a preset by id.  No-op if not found.
  void DeletePreset(const std::string& preset_id);

  // Returns all saved presets.
  const std::vector<FocusPreset>& presets() const { return presets_; }

  // Starts a focus session using a preset's duration.
  // Returns true if the session started (preset found and focus mode activated).
  bool StartSessionFromPreset(const std::string& preset_id);

  // -- Auto-start settings ------------------------------------------------

  // Auto-start time (HH:MM 24-hour format).
  const std::string& auto_start_time() const { return auto_start_time_; }
  void set_auto_start_time(const std::string& time_hhmm);

  // Auto-end time (HH:MM 24-hour format).
  const std::string& auto_end_time() const { return auto_end_time_; }
  void set_auto_end_time(const std::string& time_hhmm);

  // Days of week for auto-start (0=Sun ... 6=Sat).
  const std::vector<int>& auto_start_days() const { return auto_start_days_; }
  void set_auto_start_days(const std::vector<int>& days);

  // -- Preferences -------------------------------------------------------

  // Default focus duration in minutes. Read from / written to prefs.
  int default_focus_duration_minutes() const {
    return default_focus_duration_minutes_;
  }
  void set_default_focus_duration_minutes(int minutes);

  // Whether focus mode auto-starts on certain conditions (e.g., work hours).
  // TODO(astra): Implement auto-start logic. For now this is just a pref.
  bool auto_start_enabled() const { return auto_start_enabled_; }
  void set_auto_start_enabled(bool enabled);

  // Short break duration in minutes (pomodoro mode).
  int short_break_duration_minutes() const {
    return short_break_duration_minutes_;
  }
  void set_short_break_duration_minutes(int minutes);

  // Long break duration in minutes (pomodoro mode).
  int long_break_duration_minutes() const {
    return long_break_duration_minutes_;
  }
  void set_long_break_duration_minutes(int minutes);

  // Number of work sessions before a long break (pomodoro mode).
  int long_break_interval() const { return long_break_interval_; }
  void set_long_break_interval(int sessions);

  // Whether to auto-start the next phase in pomodoro mode.
  bool auto_start_next_phase() const { return auto_start_next_phase_; }
  void set_auto_start_next_phase(bool enabled);

 private:
  // Loads persisted preferences from the profile's PrefService.
  void LoadFromPrefs();

  // Saves current preferences to the profile's PrefService.
  void SaveToPrefs();

  // Saves the blocklist to prefs.
  void SaveBlocklistToPrefs();

  // Saves the whitelist to prefs.
  void SaveWhitelistToPrefs();

  // Saves session stats to prefs.
  void SaveStatsToPrefs();

  // Saves presets to prefs.
  void SavePresetsToPrefs();

  // Saves auto-start settings to prefs.
  void SaveAutoStartSettingsToPrefs();

  // Saves pause state to prefs.
  void SavePauseStateToPrefs();

  // Records a completed work session to stats.
  // |duration| is the actual time spent in the work phase.
  void RecordCompletedWorkSession(base::TimeDelta duration);

  // Records a completed full pomodoro cycle to stats.
  void RecordCompletedCycle();

  // Creates a session record from the current session state and adds it
  // to session history.  Called when a focus session ends.
  void AddCompletedSessionToHistory(bool is_completed);

  // Helper: checks if a URL matches a pattern using the same logic as
  // IsSiteBlocked and IsSiteWhitelisted.
  bool PatternMatchesUrl(const std::string& pattern,
                         const std::string& url) const;

  // Timer callback — fired every second during an active focus session.
  void OnTick();

  // Checks if the current phase has expired and transitions if so.
  void CheckExpired();

  // Transitions to the next pomodoro phase.
  // Called when the current phase expires (and auto-start is enabled)
  // or when the user manually advances (SkipBreak, etc.).
  void AdvanceToNextPhase();

  // Starts a specific phase (work / short break / long break).
  // Internal helper used by EnterFocusMode, StartBreak, etc.
  void StartPhase(AstraFocusPhase phase, base::TimeDelta duration);

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraFocusModeServiceObserver> observers_;

  // -- Runtime focus session state ---------------------------------------

  bool is_focus_mode_active_ = false;
  AstraFocusPhase current_phase_ = AstraFocusPhase::kWork;
  base::Time phase_start_time_;
  base::TimeDelta phase_duration_;
  bool is_paused_ = false;
  base::TimeDelta paused_remaining_;

  // Per-session tracking — accumulated across all phases of the current
  // focus session.  Reset when a new session begins.
  std::string current_session_id_;
  base::Time current_session_start_time_;
  base::TimeDelta current_session_focus_time_;
  int current_session_work_count_ = 0;
  int current_session_cycles_ = 0;
  int current_session_distraction_count_ = 0;
  bool current_session_whitelist_used_ = false;
  bool session_ended_naturally_ = false;

  // -- Pomodoro state ----------------------------------------------------

  bool pomodoro_mode_active_ = false;
  int completed_work_sessions_ = 0;
  int completed_cycles_ = 0;

  // Tick timer that fires every second to update remaining time and
  // check for phase expiration.
  base::RepeatingTimer tick_timer_;

  // -- Persisted preferences ---------------------------------------------

  int default_focus_duration_minutes_ = 25;  // Pomodoro default: 25 min.
  bool auto_start_enabled_ = false;
  std::vector<std::string> distraction_blocklist_;
  std::vector<std::string> whitelist_;

  int short_break_duration_minutes_ = 5;    // Pomodoro default: 5 min.
  int long_break_duration_minutes_ = 15;    // Pomodoro default: 15 min.
  int long_break_interval_ = 4;             // Pomodoro default: every 4 work sessions.
  bool auto_start_next_phase_ = false;      // Manual by default.

  // -- Persisted stats ---------------------------------------------------

  base::TimeDelta total_focus_time_;
  int total_sessions_completed_ = 0;
  int total_cycles_completed_ = 0;

  // -- Distraction warnings ----------------------------------------------

  bool warnings_enabled_ = true;

  // -- Session presets ---------------------------------------------------

  std::vector<FocusPreset> presets_;

  // -- Auto-start settings -----------------------------------------------

  std::string auto_start_time_ = "09:00";
  std::string auto_end_time_ = "17:00";
  std::vector<int> auto_start_days_;

  // -- Session history ----------------------------------------------------

  std::vector<AstraFocusSession> session_history_;
  size_t max_history_entries_ = 100;
};

// =========================================================================
// AstraFocusModeServiceFactory
// =========================================================================
//
// Factory for AstraFocusModeService.
//
// Incognito behavior: the factory uses kOwnInstance for incognito profiles
// because focus mode is a per-browsing-context feature. An incognito window
// should have its own focus session that does not affect the main profile's
// active session. The default duration and blocklist prefs are still shared
// with the original profile (via pref forwarding), but the active session
// state is per-instance.
//
// Guest sessions also get their own instance (kOwnInstance).
// System profile gets no instance (kNone).
// =========================================================================

class AstraFocusModeServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraFocusModeService* GetForProfile(Profile* profile);
  static AstraFocusModeServiceFactory* GetInstance();

  // Registers focus-mode-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation. Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraFocusModeServiceFactory();
  ~AstraFocusModeServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_FOCUS_MODE_SERVICE_H_
