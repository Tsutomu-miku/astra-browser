#include "astra/browser/astra_focus_mode_service.h"

#include <algorithm>

#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Default tick interval for focus session updates (1 second).
constexpr base::TimeDelta kTickInterval = base::Seconds(1);

}  // namespace

// ---------------------------------------------------------------------------
// AstraFocusModeService
// ---------------------------------------------------------------------------

AstraFocusModeService::AstraFocusModeService(Profile* profile)
    : profile_(profile) {
  // Load persisted preferences from the profile's PrefService.
  LoadFromPrefs();
}

AstraFocusModeService::~AstraFocusModeService() = default;

void AstraFocusModeService::Shutdown() {
  // KeyedService shutdown: clear all observer references and stop the timer
  // before the profile goes away.
  tick_timer_.Stop();
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraFocusModeService::AddObserver(
    AstraFocusModeServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraFocusModeService::RemoveObserver(
    AstraFocusModeServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Focus mode control ------------------------------------------------------

void AstraFocusModeService::EnterFocusMode(base::TimeDelta duration) {
  if (duration <= base::TimeDelta()) {
    // Use default duration if none provided or if negative.
    duration = base::Minutes(default_focus_duration_minutes_);
  }

  // Entering simple focus mode stops pomodoro cycling.
  pomodoro_mode_active_ = false;
  completed_work_sessions_ = 0;
  completed_cycles_ = 0;

  if (is_focus_mode_active_) {
    // Extend the existing session.
    ExtendFocusSession(duration);
    return;
  }

  StartPhase(AstraFocusPhase::kWork, duration);
}

void AstraFocusModeService::ExitFocusMode() {
  if (!is_focus_mode_active_) {
    return;
  }

  // Calculate actual time spent in current phase.
  base::TimeDelta actual_duration;
  if (is_paused_) {
    actual_duration = phase_duration_ - paused_remaining_;
  } else {
    actual_duration = base::Time::Now() - phase_start_time_;
  }

  // Record the work session if we were in work phase.
  if (current_phase_ == AstraFocusPhase::kWork && actual_duration > base::TimeDelta()) {
    RecordCompletedWorkSession(actual_duration);
  }

  base::TimeDelta total_session_duration = actual_duration;

  is_focus_mode_active_ = false;
  is_paused_ = false;
  pomodoro_mode_active_ = false;
  tick_timer_.Stop();
  phase_start_time_ = base::Time();
  phase_duration_ = base::TimeDelta();
  paused_remaining_ = base::TimeDelta();
  completed_work_sessions_ = 0;
  completed_cycles_ = 0;

  // Clear persisted pause state.
  SavePauseStateToPrefs();

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnFocusModeExited();
    observer.OnFocusSessionCompleted(total_session_duration);
  }
}

void AstraFocusModeService::ToggleFocusMode() {
  if (is_focus_mode_active_) {
    ExitFocusMode();
  } else {
    EnterFocusMode(base::Minutes(default_focus_duration_minutes_));
  }
}

bool AstraFocusModeService::IsFocusModeActive() const {
  return is_focus_mode_active_;
}

base::TimeDelta AstraFocusModeService::GetRemainingTime() const {
  if (!is_focus_mode_active_) {
    return base::TimeDelta();
  }
  if (is_paused_) {
    return paused_remaining_;
  }
  base::TimeDelta elapsed = base::Time::Now() - phase_start_time_;
  base::TimeDelta remaining = phase_duration_ - elapsed;
  return remaining < base::TimeDelta() ? base::TimeDelta() : remaining;
}

base::TimeDelta AstraFocusModeService::GetTotalDuration() const {
  return phase_duration_;
}

void AstraFocusModeService::ExtendFocusSession(
    base::TimeDelta additional_duration) {
  if (!is_focus_mode_active_ || additional_duration <= base::TimeDelta()) {
    return;
  }

  phase_duration_ += additional_duration;

  // Notify observers of the updated time.
  for (auto& observer : observers_) {
    observer.OnFocusTimeUpdated(GetRemainingTime());
  }
}

// -- Pomodoro mode -----------------------------------------------------------

void AstraFocusModeService::StartPomodoro() {
  pomodoro_mode_active_ = true;
  completed_work_sessions_ = 0;
  completed_cycles_ = 0;

  // Start with a work session.
  StartPhase(AstraFocusPhase::kWork,
             base::Minutes(default_focus_duration_minutes_));
}

void AstraFocusModeService::StartBreak(bool is_long) {
  if (is_focus_mode_active_ && current_phase_ == AstraFocusPhase::kWork &&
      pomodoro_mode_active_) {
    // Switching from work to break mid-session: count the work session.
    completed_work_sessions_++;
  }

  AstraFocusPhase phase =
      is_long ? AstraFocusPhase::kLongBreak : AstraFocusPhase::kShortBreak;
  int minutes = is_long ? long_break_duration_minutes_
                        : short_break_duration_minutes_;
  StartPhase(phase, base::Minutes(minutes));
}

void AstraFocusModeService::SkipBreak() {
  if (!is_focus_mode_active_ || current_phase_ == AstraFocusPhase::kWork) {
    return;
  }

  // Skip the current break and go to the next work session.
  AdvanceToNextPhase();
}

AstraFocusPhase AstraFocusModeService::GetCurrentPhase() const {
  if (!is_focus_mode_active_) {
    // No active session — default to kWork as nominal phase.
    return AstraFocusPhase::kWork;
  }
  return current_phase_;
}

int AstraFocusModeService::GetCompletedWorkSessions() const {
  return completed_work_sessions_;
}

int AstraFocusModeService::GetCycleCount() const {
  return completed_cycles_;
}

// -- Distraction blocklist ---------------------------------------------------

void AstraFocusModeService::AddDistractionSite(const std::string& url_pattern) {
  if (url_pattern.empty()) {
    return;
  }

  // Check if already in the list.
  auto it = base::ranges::find(distraction_blocklist_, url_pattern);
  if (it != distraction_blocklist_.end()) {
    return;  // Already present.
  }

  distraction_blocklist_.push_back(url_pattern);
  SaveBlocklistToPrefs();

  for (auto& observer : observers_) {
    observer.OnDistractionBlocklistChanged();
  }
}

void AstraFocusModeService::RemoveDistractionSite(
    const std::string& url_pattern) {
  auto it = base::ranges::find(distraction_blocklist_, url_pattern);
  if (it == distraction_blocklist_.end()) {
    return;  // Not found.
  }

  distraction_blocklist_.erase(it);
  SaveBlocklistToPrefs();

  for (auto& observer : observers_) {
    observer.OnDistractionBlocklistChanged();
  }
}

bool AstraFocusModeService::IsSiteBlocked(const std::string& url) const {
  if (distraction_blocklist_.empty()) {
    return false;
  }

  // TODO(astra): Implement proper site blocking via Chromium's
  // HostContentSettingsMap or a NavigationThrottle. The current simple
  // pattern matching is a placeholder for the overlay skeleton.
  //
  // Chromium component: HostContentSettingsMap
  //   (components/content_settings/core/browser/host_content_settings_map.h)
  // or NavigationThrottle
  //   (content/public/browser/navigation_throttle.h)
  //
  // Patch point: NavigationThrottle added via a ThrottleProvider or
  // content settings rules checked at navigation time.
  //
  // Real implementation should use ContentSettingsPattern::Matches().

  for (const auto& pattern : distraction_blocklist_) {
    if (PatternMatchesUrl(pattern, url)) {
      return true;
    }
  }
  return false;
}

// -- Session pause / resume --------------------------------------------------

void AstraFocusModeService::PauseSession() {
  if (!is_focus_mode_active_ || is_paused_) {
    return;
  }

  is_paused_ = true;
  paused_remaining_ = GetRemainingTime();
  tick_timer_.Stop();
  SavePauseStateToPrefs();

  for (auto& observer : observers_) {
    observer.OnFocusSessionPaused();
  }
}

void AstraFocusModeService::ResumeSession() {
  if (!is_focus_mode_active_ || !is_paused_) {
    return;
  }

  is_paused_ = false;
  phase_start_time_ = base::Time::Now();
  phase_duration_ = paused_remaining_;
  paused_remaining_ = base::TimeDelta();
  tick_timer_.Start(FROM_HERE, kTickInterval, this,
                    &AstraFocusModeService::OnTick);
  SavePauseStateToPrefs();

  for (auto& observer : observers_) {
    observer.OnFocusSessionResumed();
    observer.OnFocusTimeUpdated(GetRemainingTime());
  }
}

bool AstraFocusModeService::IsSessionPaused() const {
  return is_focus_mode_active_ && is_paused_;
}

// -- Session stats -----------------------------------------------------------

base::TimeDelta AstraFocusModeService::GetTotalFocusTime() const {
  return total_focus_time_;
}

int AstraFocusModeService::GetSessionsCompleted() const {
  return total_sessions_completed_;
}

int AstraFocusModeService::GetTotalCyclesCompleted() const {
  return total_cycles_completed_;
}

void AstraFocusModeService::ResetStats() {
  total_focus_time_ = base::TimeDelta();
  total_sessions_completed_ = 0;
  total_cycles_completed_ = 0;
  SaveStatsToPrefs();

  for (auto& observer : observers_) {
    observer.OnStatsUpdated();
  }
}

// -- Whitelist (allowed sites) -----------------------------------------------

void AstraFocusModeService::AddWhitelistedSite(const std::string& url_pattern) {
  if (url_pattern.empty()) {
    return;
  }

  auto it = base::ranges::find(whitelist_, url_pattern);
  if (it != whitelist_.end()) {
    return;  // Already present.
  }

  whitelist_.push_back(url_pattern);
  SaveWhitelistToPrefs();

  for (auto& observer : observers_) {
    observer.OnWhitelistChanged();
  }
}

void AstraFocusModeService::RemoveWhitelistedSite(
    const std::string& url_pattern) {
  auto it = base::ranges::find(whitelist_, url_pattern);
  if (it == whitelist_.end()) {
    return;  // Not found.
  }

  whitelist_.erase(it);
  SaveWhitelistToPrefs();

  for (auto& observer : observers_) {
    observer.OnWhitelistChanged();
  }
}

bool AstraFocusModeService::IsSiteWhitelisted(const std::string& url) const {
  if (whitelist_.empty()) {
    return false;
  }

  for (const auto& pattern : whitelist_) {
    if (PatternMatchesUrl(pattern, url)) {
      return true;
    }
  }
  return false;
}

// -- Distraction warnings ----------------------------------------------------

bool AstraFocusModeService::TriggerDistractionWarning(const std::string& url) {
  if (!is_focus_mode_active_ || !warnings_enabled_) {
    return false;
  }

  // Check if site is actually blocked (not whitelisted and is in blocklist).
  if (IsSiteWhitelisted(url)) {
    return false;
  }
  if (!IsSiteBlocked(url)) {
    return false;
  }

  for (auto& observer : observers_) {
    observer.OnDistractionWarning(url);
  }

  return true;
}

void AstraFocusModeService::set_warnings_enabled(bool enabled) {
  if (warnings_enabled_ == enabled) {
    return;
  }
  warnings_enabled_ = enabled;
  SaveToPrefs();
}

// -- Session presets ---------------------------------------------------------

void AstraFocusModeService::SavePreset(const FocusPreset& preset) {
  if (preset.id.empty() || preset.duration_minutes <= 0) {
    return;
  }

  // Replace existing preset with same id, or append if new.
  auto it = std::find_if(presets_.begin(), presets_.end(),
                         [&](const FocusPreset& p) { return p.id == preset.id; });
  if (it != presets_.end()) {
    *it = preset;
  } else {
    presets_.push_back(preset);
  }

  SavePresetsToPrefs();

  for (auto& observer : observers_) {
    observer.OnPresetsChanged();
  }
}

void AstraFocusModeService::DeletePreset(const std::string& preset_id) {
  auto it = std::find_if(presets_.begin(), presets_.end(),
                         [&](const FocusPreset& p) { return p.id == preset_id; });
  if (it == presets_.end()) {
    return;
  }

  presets_.erase(it);
  SavePresetsToPrefs();

  for (auto& observer : observers_) {
    observer.OnPresetsChanged();
  }
}

bool AstraFocusModeService::StartSessionFromPreset(const std::string& preset_id) {
  auto it = std::find_if(presets_.begin(), presets_.end(),
                         [&](const FocusPreset& p) { return p.id == preset_id; });
  if (it == presets_.end()) {
    return false;
  }

  EnterFocusMode(base::Minutes(it->duration_minutes));
  return true;
}

// -- Auto-start settings -----------------------------------------------------

void AstraFocusModeService::set_auto_start_time(const std::string& time_hhmm) {
  // Basic validation: must be "HH:MM" format.
  if (time_hhmm.size() != 5 || time_hhmm[2] != ':') {
    return;
  }
  int hour = std::stoi(time_hhmm.substr(0, 2));
  int minute = std::stoi(time_hhmm.substr(3, 2));
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return;
  }

  auto_start_time_ = time_hhmm;
  SaveAutoStartSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnAutoStartSettingsChanged();
  }
}

void AstraFocusModeService::set_auto_end_time(const std::string& time_hhmm) {
  if (time_hhmm.size() != 5 || time_hhmm[2] != ':') {
    return;
  }
  int hour = std::stoi(time_hhmm.substr(0, 2));
  int minute = std::stoi(time_hhmm.substr(3, 2));
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return;
  }

  auto_end_time_ = time_hhmm;
  SaveAutoStartSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnAutoStartSettingsChanged();
  }
}

void AstraFocusModeService::set_auto_start_days(const std::vector<int>& days) {
  // Validate: all values must be 0-6.
  for (int day : days) {
    if (day < 0 || day > 6) {
      return;
    }
  }

  auto_start_days_ = days;
  SaveAutoStartSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnAutoStartSettingsChanged();
  }
}

// -- Preferences -------------------------------------------------------------

void AstraFocusModeService::set_default_focus_duration_minutes(int minutes) {
  if (minutes <= 0) {
    return;
  }
  default_focus_duration_minutes_ = minutes;
  SaveToPrefs();
}

void AstraFocusModeService::set_auto_start_enabled(bool enabled) {
  auto_start_enabled_ = enabled;
  SaveToPrefs();
}

void AstraFocusModeService::set_short_break_duration_minutes(int minutes) {
  if (minutes <= 0) {
    return;
  }
  short_break_duration_minutes_ = minutes;
  SaveToPrefs();
}

void AstraFocusModeService::set_long_break_duration_minutes(int minutes) {
  if (minutes <= 0) {
    return;
  }
  long_break_duration_minutes_ = minutes;
  SaveToPrefs();
}

void AstraFocusModeService::set_long_break_interval(int sessions) {
  if (sessions <= 0) {
    return;
  }
  long_break_interval_ = sessions;
  SaveToPrefs();
}

void AstraFocusModeService::set_auto_start_next_phase(bool enabled) {
  auto_start_next_phase_ = enabled;
  SaveToPrefs();
}

// -- Private helpers ---------------------------------------------------------

void AstraFocusModeService::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  default_focus_duration_minutes_ =
      prefs->GetInteger(prefs::kPrefFocusModeDefaultDuration);

  auto_start_enabled_ = prefs->GetBoolean(prefs::kPrefFocusModeAutoStart);

  // Load blocklist.
  {
    const base::Value::List& blocklist =
        prefs->GetList(prefs::kPrefFocusModeBlocklist);
    distraction_blocklist_.clear();
    for (const auto& item : blocklist) {
      if (item.is_string()) {
        distraction_blocklist_.push_back(item.GetString());
      }
    }
  }

  // Load whitelist.
  {
    const base::Value::List& whitelist =
        prefs->GetList(prefs::kPrefFocusModeWhitelist);
    whitelist_.clear();
    for (const auto& item : whitelist) {
      if (item.is_string()) {
        whitelist_.push_back(item.GetString());
      }
    }
  }

  // Load pomodoro prefs.
  short_break_duration_minutes_ =
      prefs->GetInteger(prefs::kPrefFocusModeShortBreakDuration);
  long_break_duration_minutes_ =
      prefs->GetInteger(prefs::kPrefFocusModeLongBreakDuration);
  long_break_interval_ =
      prefs->GetInteger(prefs::kPrefFocusModeLongBreakInterval);
  auto_start_next_phase_ =
      prefs->GetBoolean(prefs::kPrefFocusModeAutoStartNextPhase);

  // Load session stats.
  total_focus_time_ = base::Seconds(
      prefs->GetInteger(prefs::kPrefFocusModeTotalFocusSeconds));
  total_sessions_completed_ =
      prefs->GetInteger(prefs::kPrefFocusModeSessionsCompleted);
  total_cycles_completed_ =
      prefs->GetInteger(prefs::kPrefFocusModeCyclesCompleted);

  // Load distraction warning prefs.
  warnings_enabled_ =
      prefs->GetBoolean(prefs::kPrefFocusModeWarningsEnabled);

  // Load auto-start time settings.
  auto_start_time_ = prefs->GetString(prefs::kPrefFocusModeAutoStartTime);
  auto_end_time_ = prefs->GetString(prefs::kPrefFocusModeAutoEndTime);

  // Load auto-start days.
  {
    const base::Value::List& days =
        prefs->GetList(prefs::kPrefFocusModeAutoStartDays);
    auto_start_days_.clear();
    for (const auto& item : days) {
      if (item.is_int()) {
        auto_start_days_.push_back(item.GetInt());
      }
    }
  }

  // Load session presets.
  {
    const base::Value::List& presets =
        prefs->GetList(prefs::kPrefFocusModePresets);
    presets_.clear();
    for (const auto& item : presets) {
      if (!item.is_dict()) {
        continue;
      }
      const auto* dict = item.GetIfDict();
      if (!dict) {
        continue;
      }
      FocusPreset preset;
      const std::string* id = dict->FindString("id");
      const std::string* name = dict->FindString("name");
      std::optional<int> duration = dict->FindInt("duration");
      std::optional<int> break_duration = dict->FindInt("break_duration");
      if (id && name && duration.has_value()) {
        preset.id = *id;
        preset.name = *name;
        preset.duration_minutes = duration.value();
        preset.break_duration_minutes = break_duration.value_or(5);
        presets_.push_back(preset);
      }
    }
  }

  // Load pause state (if a session was paused at shutdown).
  bool paused = prefs->GetBoolean(prefs::kPrefFocusModePaused);
  if (paused) {
    int remaining_seconds =
        prefs->GetInteger(prefs::kPrefFocusModePausedRemainingSeconds);
    if (remaining_seconds > 0) {
      // Restore paused state.  The session is not "active" in terms of the
      // timer, but the paused state is preserved for user awareness.
      is_paused_ = true;
      paused_remaining_ = base::Seconds(remaining_seconds);
      // Note: is_focus_mode_active_ remains false at startup — the user must
      // explicitly resume.  This prevents unexpected focus mode activation
      // after browser restart.
    }
  }
}

void AstraFocusModeService::SaveToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetInteger(prefs::kPrefFocusModeDefaultDuration,
                    default_focus_duration_minutes_);
  prefs->SetBoolean(prefs::kPrefFocusModeAutoStart, auto_start_enabled_);
  prefs->SetInteger(prefs::kPrefFocusModeShortBreakDuration,
                    short_break_duration_minutes_);
  prefs->SetInteger(prefs::kPrefFocusModeLongBreakDuration,
                    long_break_duration_minutes_);
  prefs->SetInteger(prefs::kPrefFocusModeLongBreakInterval,
                    long_break_interval_);
  prefs->SetBoolean(prefs::kPrefFocusModeAutoStartNextPhase,
                    auto_start_next_phase_);
  prefs->SetBoolean(prefs::kPrefFocusModeWarningsEnabled, warnings_enabled_);
}

void AstraFocusModeService::SaveBlocklistToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  base::Value::List list;
  for (const auto& pattern : distraction_blocklist_) {
    list.Append(pattern);
  }
  prefs->SetList(prefs::kPrefFocusModeBlocklist, std::move(list));
}

void AstraFocusModeService::SaveWhitelistToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  base::Value::List list;
  for (const auto& pattern : whitelist_) {
    list.Append(pattern);
  }
  prefs->SetList(prefs::kPrefFocusModeWhitelist, std::move(list));
}

void AstraFocusModeService::SaveStatsToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetInteger(prefs::kPrefFocusModeTotalFocusSeconds,
                    static_cast<int>(total_focus_time_.InSeconds()));
  prefs->SetInteger(prefs::kPrefFocusModeSessionsCompleted,
                    total_sessions_completed_);
  prefs->SetInteger(prefs::kPrefFocusModeCyclesCompleted,
                    total_cycles_completed_);
}

void AstraFocusModeService::SavePresetsToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  base::Value::List list;
  for (const auto& preset : presets_) {
    base::Value::Dict dict;
    dict.Set("id", preset.id);
    dict.Set("name", preset.name);
    dict.Set("duration", preset.duration_minutes);
    dict.Set("break_duration", preset.break_duration_minutes);
    list.Append(std::move(dict));
  }
  prefs->SetList(prefs::kPrefFocusModePresets, std::move(list));
}

void AstraFocusModeService::SaveAutoStartSettingsToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetString(prefs::kPrefFocusModeAutoStartTime, auto_start_time_);
  prefs->SetString(prefs::kPrefFocusModeAutoEndTime, auto_end_time_);

  base::Value::List days;
  for (int day : auto_start_days_) {
    days.Append(day);
  }
  prefs->SetList(prefs::kPrefFocusModeAutoStartDays, std::move(days));
}

void AstraFocusModeService::SavePauseStateToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetBoolean(prefs::kPrefFocusModePaused, is_paused_);
  prefs->SetInteger(prefs::kPrefFocusModePausedRemainingSeconds,
                    static_cast<int>(paused_remaining_.InSeconds()));
}

void AstraFocusModeService::RecordCompletedWorkSession(
    base::TimeDelta duration) {
  total_focus_time_ += duration;
  total_sessions_completed_++;
  SaveStatsToPrefs();

  for (auto& observer : observers_) {
    observer.OnStatsUpdated();
  }
}

void AstraFocusModeService::RecordCompletedCycle() {
  total_cycles_completed_++;
  SaveStatsToPrefs();

  for (auto& observer : observers_) {
    observer.OnStatsUpdated();
  }
}

bool AstraFocusModeService::PatternMatchesUrl(const std::string& pattern,
                                              const std::string& url) const {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    // Fall back to simple substring matching if not a valid URL.
    return url.find(pattern) != std::string::npos;
  }

  std::string host = gurl.host();
  if (pattern.starts_with("*.")) {
    // Wildcard pattern: *.example.com
    std::string suffix = pattern.substr(2);
    return host == suffix || host.ends_with("." + suffix);
  } else {
    // Exact or suffix match.
    return host == pattern || host.ends_with("." + pattern);
  }
}

void AstraFocusModeService::StartPhase(AstraFocusPhase phase,
                                       base::TimeDelta duration) {
  DCHECK(duration > base::TimeDelta());

  bool was_active = is_focus_mode_active_;
  is_focus_mode_active_ = true;
  is_paused_ = false;
  paused_remaining_ = base::TimeDelta();
  current_phase_ = phase;
  phase_start_time_ = base::Time::Now();
  phase_duration_ = duration;

  // Start the tick timer if not already running.
  if (!tick_timer_.IsRunning()) {
    tick_timer_.Start(FROM_HERE, kTickInterval, this,
                      &AstraFocusModeService::OnTick);
  }

  // Notify observers.
  if (!was_active) {
    for (auto& observer : observers_) {
      observer.OnFocusModeEntered(phase_duration_);
    }
  }

  // Always fire phase change notification when phase changes.
  for (auto& observer : observers_) {
    observer.OnFocusPhaseChanged(current_phase_);
    observer.OnFocusTimeUpdated(GetRemainingTime());
  }
}

void AstraFocusModeService::AdvanceToNextPhase() {
  if (!pomodoro_mode_active_) {
    // Not in pomodoro mode — just exit focus mode when phase ends.
    ExitFocusMode();
    return;
  }

  AstraFocusPhase next_phase;
  base::TimeDelta next_duration;

  if (current_phase_ == AstraFocusPhase::kWork) {
    // Work phase ended — count it and decide break type.
    completed_work_sessions_++;

    // Record the completed work session to cumulative stats.
    RecordCompletedWorkSession(phase_duration_);

    // Check if we just finished a full cycle (long break interval work sessions).
    bool is_cycle_complete =
        (completed_work_sessions_ % long_break_interval_ == 0);

    if (is_cycle_complete) {
      next_phase = AstraFocusPhase::kLongBreak;
      next_duration = base::Minutes(long_break_duration_minutes_);
      completed_cycles_++;

      // Record the completed cycle to cumulative stats.
      RecordCompletedCycle();

      // Notify cycle completion.
      for (auto& observer : observers_) {
        observer.OnPomodoroCycleCompleted(completed_cycles_);
      }
    } else {
      next_phase = AstraFocusPhase::kShortBreak;
      next_duration = base::Minutes(short_break_duration_minutes_);
    }
  } else {
    // Break phase ended — go back to work.
    next_phase = AstraFocusPhase::kWork;
    next_duration = base::Minutes(default_focus_duration_minutes_);
  }

  if (auto_start_next_phase_) {
    StartPhase(next_phase, next_duration);
  } else {
    // Manual mode: stop at the end of the phase.
    // UI will show "Start next phase" button.
    // We still transition the phase state so UI knows what comes next,
    // but we stop the timer and mark focus mode as inactive.
    is_focus_mode_active_ = false;
    tick_timer_.Stop();
    phase_start_time_ = base::Time();
    phase_duration_ = base::TimeDelta();
    // Keep pomodoro_mode_active_ true so the UI knows we're in a cycle.
    // Keep completed_work_sessions_ and completed_cycles_ for progress display.

    for (auto& observer : observers_) {
      observer.OnFocusModeExited();
    }
  }
}

void AstraFocusModeService::OnTick() {
  CheckExpired();

  if (is_focus_mode_active_) {
    for (auto& observer : observers_) {
      observer.OnFocusTimeUpdated(GetRemainingTime());
    }
  }
}

void AstraFocusModeService::CheckExpired() {
  if (!is_focus_mode_active_) {
    return;
  }

  base::TimeDelta elapsed = base::Time::Now() - phase_start_time_;
  if (elapsed >= phase_duration_) {
    AdvanceToNextPhase();
  }
}

// ---------------------------------------------------------------------------
// AstraFocusModeServiceFactory
// ---------------------------------------------------------------------------

// static
AstraFocusModeService* AstraFocusModeServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraFocusModeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraFocusModeServiceFactory* AstraFocusModeServiceFactory::GetInstance() {
  static base::NoDestructor<AstraFocusModeServiceFactory> instance;
  return instance.get();
}

// static
void AstraFocusModeServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Default focus duration: 25 minutes (Pomodoro-style default).
  registry->RegisterIntegerPref(prefs::kPrefFocusModeDefaultDuration, 25);

  // Distraction site blocklist: list of URL pattern strings.
  // Default: empty list — user populates their own distractions.
  registry->RegisterListPref(prefs::kPrefFocusModeBlocklist);

  // Whether focus mode auto-starts (e.g., during work hours).
  // Default: false — user opts in.
  registry->RegisterBooleanPref(prefs::kPrefFocusModeAutoStart, false);

  // Short break duration for pomodoro mode (minutes).
  // Default: 5 minutes — standard Pomodoro short break.
  registry->RegisterIntegerPref(prefs::kPrefFocusModeShortBreakDuration,
                                prefs::kDefaultFocusShortBreakMinutes);

  // Long break duration for pomodoro mode (minutes).
  // Default: 15 minutes — standard Pomodoro long break.
  registry->RegisterIntegerPref(prefs::kPrefFocusModeLongBreakDuration,
                                prefs::kDefaultFocusLongBreakMinutes);

  // Number of work sessions before a long break.
  // Default: 4 — classic Pomodoro cycle.
  registry->RegisterIntegerPref(prefs::kPrefFocusModeLongBreakInterval,
                                prefs::kDefaultFocusLongBreakInterval);

  // Whether to auto-start the next phase in pomodoro mode.
  // Default: false — user manually starts each phase.
  registry->RegisterBooleanPref(prefs::kPrefFocusModeAutoStartNextPhase,
                                prefs::kDefaultFocusAutoStartNextPhase);

  // TODO(astra): Add additional focus mode prefs as needed:
  //   - Auto-start time range (start_time, end_time).
  //   - Auto-start days of week.
  //   - Whether to show the focus indicator.
  //   - Whether to auto-hide sidebar in focus mode.
  //   - Whether to auto-minimize toolbar in focus mode.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration.
}

AstraFocusModeServiceFactory::AstraFocusModeServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraFocusModeService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito uses kOwnInstance because focus mode is a
              // per-browsing-context feature. An incognito window has its
              // own focus session that doesn't affect the main profile's
              // active session. Prefs are still shared (forwarded) — only
              // the runtime session state is per-instance.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions get their own ephemeral focus mode instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user-facing focus mode.
              .Build()) {}

AstraFocusModeServiceFactory::~AstraFocusModeServiceFactory() = default;

KeyedService*
AstraFocusModeServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraFocusModeService(Profile::FromBrowserContext(context));
}

}  // namespace astra
