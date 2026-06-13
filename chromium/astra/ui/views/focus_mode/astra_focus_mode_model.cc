#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/guid.h"
#include "base/observer_list.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Default session preset durations (Pomodoro-style) as TimeDelta.
const base::TimeDelta kDefaultPresetDurations[] = {
    base::Minutes(25),   // Pomodoro standard.
    base::Minutes(45),   // Deep work session.
    base::Minutes(60),   // Hour-long session.
    base::Minutes(90),   // Ultradian rhythm.
};

// Default preset durations in minutes (legacy API).
constexpr int kDefaultPresetDurationsMinutes[] = {25, 45, 60, 90};

// Default blocklist categories (built-in sites for each level).
// These are example domain patterns — actual blocking is done by Chromium
// content settings. The model tracks which level is selected for UI display.
// TODO(astra): Wire built-in block categories to Chromium content settings.
//   Chromium owner: HostContentSettingsMap / content_settings patterns.

bool IsSocialMediaDomain(const GURL& url) {
  const std::string host = url.host();
  return host.find("facebook.com") != std::string::npos ||
         host.find("twitter.com") != std::string::npos ||
         host.find("x.com") != std::string::npos ||
         host.find("instagram.com") != std::string::npos ||
         host.find("tiktok.com") != std::string::npos ||
         host.find("linkedin.com") != std::string::npos ||
         host.find("reddit.com") != std::string::npos;
}

bool IsEntertainmentDomain(const GURL& url) {
  const std::string host = url.host();
  return host.find("youtube.com") != std::string::npos ||
         host.find("netflix.com") != std::string::npos ||
         host.find("hulu.com") != std::string::npos ||
         host.find("twitch.tv") != std::string::npos ||
         host.find("disneyplus.com") != std::string::npos;
}

bool IsNewsDomain(const GURL& url) {
  const std::string host = url.host();
  return host.find("cnn.com") != std::string::npos ||
         host.find("bbc.com") != std::string::npos ||
         host.find("nytimes.com") != std::string::npos ||
         host.find("washingtonpost.com") != std::string::npos ||
         host.find("foxnews.com") != std::string::npos;
}

bool IsWorkDomain(const GURL& url) {
  // Default "work" sites that are always allowed in strict mode.
  const std::string host = url.host();
  return host.find("google.com") != std::string::npos ||
         host.find("gmail.com") != std::string::npos ||
         host.find("docs.google.com") != std::string::npos ||
         host.find("drive.google.com") != std::string::npos ||
         host.find("github.com") != std::string::npos ||
         host.find("stackoverflow.com") != std::string::npos ||
         host.find("notion.so") != std::string::npos ||
         host.find("slack.com") != std::string::npos;
}

}  // namespace

// =========================================================================
// AstraFocusModeModel
// =========================================================================

AstraFocusModeModel::AstraFocusModeModel(PrefService* pref_service)
    : pref_service_(pref_service) {
  if (pref_service_) {
    LoadFromPrefs();
  }
}

AstraFocusModeModel::~AstraFocusModeModel() {
  NotifyModelShutdown();
}

// -- Observers -------------------------------------------------------------

void AstraFocusModeModel::AddObserver(AstraFocusModeObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraFocusModeModel::RemoveObserver(AstraFocusModeObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Focus state -----------------------------------------------------------

void AstraFocusModeModel::SetActive(bool active) {
  if (is_active_ == active) {
    return;
  }
  is_active_ = active;

  if (active) {
    // Initialize session state when starting.
    if (session_start_time_.is_null()) {
      session_start_time_ = base::Time::Now();
    }
    if (total_duration_.is_zero() && !default_duration_.is_zero()) {
      total_duration_ = default_duration_;
      remaining_time_ = default_duration_;
    }
    NotifyFocusModeStarted();
  } else {
    // Record session if it had any duration.
    if (!total_duration_.is_zero() && !session_start_time_.is_null()) {
      RecordCompletedSession();
    }
    NotifyFocusModeEnded();
  }
}

bool AstraFocusModeModel::IsActive() const {
  return is_active_;
}

void AstraFocusModeModel::SetDuration(base::TimeDelta duration) {
  if (duration.is_negative()) {
    duration = base::TimeDelta();
  }
  total_duration_ = duration;
  if (is_active_ && remaining_time_ > duration) {
    remaining_time_ = duration;
  }
  // Also update legacy in-memory default if no session is active.
  if (!is_active_) {
    default_duration_ = duration;
  }
}

base::TimeDelta AstraFocusModeModel::GetDuration() const {
  return total_duration_;
}

base::TimeDelta AstraFocusModeModel::GetTimeRemaining() const {
  if (!is_active_ || is_paused_) {
    return remaining_time_;
  }
  // If active and not paused, compute from start + elapsed.
  // For the model's projected state, we just return remaining_time_ since
  // the controller updates it via SetRemainingTime.
  return remaining_time_;
}

base::TimeDelta AstraFocusModeModel::GetElapsedTime() const {
  return elapsed_time_;
}

base::Time AstraFocusModeModel::GetStartTime() const {
  return session_start_time_;
}

bool AstraFocusModeModel::IsSessionActive() const {
  return is_active_ && remaining_time_ > base::TimeDelta() && !is_paused_;
}

bool AstraFocusModeModel::IsSessionPaused() const {
  return is_active_ && is_paused_;
}

void AstraFocusModeModel::PauseSession() {
  if (!is_active_ || is_paused_) {
    return;
  }
  is_paused_ = true;
  NotifyFocusModePaused();
}

void AstraFocusModeModel::ResumeSession() {
  if (!is_active_ || !is_paused_) {
    return;
  }
  is_paused_ = false;
  NotifyFocusModeResumed();
}

void AstraFocusModeModel::ResetSession() {
  is_active_ = false;
  is_paused_ = false;
  session_start_time_ = base::Time();
  elapsed_time_ = base::TimeDelta();
  remaining_time_ = total_duration_;
  distractions_blocked_ = 0;
  session_note_.clear();
}

void AstraFocusModeModel::ExtendSession(base::TimeDelta extension) {
  if (!is_active_) {
    return;
  }
  if (extension.is_negative()) {
    // Negative extension shortens, but don't go below zero.
    if (remaining_time_ + extension < base::TimeDelta()) {
      extension = -remaining_time_;
    }
  }
  total_duration_ += extension;
  remaining_time_ += extension;
  NotifyFocusTimeUpdated();
}

void AstraFocusModeModel::EndSession() {
  if (!is_active_) {
    return;
  }
  // Record completed session.
  if (!total_duration_.is_zero() && !session_start_time_.is_null()) {
    RecordCompletedSession();
  }
  is_active_ = false;
  is_paused_ = false;
  NotifyFocusModeEnded();
}

// -- Distraction blocking --------------------------------------------------

void AstraFocusModeModel::SetBlockLevel(AstraFocusBlockLevel level) {
  if (block_level_ == level) {
    return;
  }
  block_level_ = level;
  NotifyBlockLevelChanged();
}

AstraFocusBlockLevel AstraFocusModeModel::GetBlockLevel() const {
  return block_level_;
}

void AstraFocusModeModel::AddBlockedSite(const GURL& url) {
  if (!url.is_valid()) {
    return;
  }
  // Check if already in the list.
  for (const auto& site : blocked_sites_) {
    if (site == url) {
      return;
    }
  }
  blocked_sites_.push_back(url);
  // Auto-set to custom level if adding sites to a non-custom level.
  if (block_level_ != AstraFocusBlockLevel::kCustom) {
    block_level_ = AstraFocusBlockLevel::kCustom;
    NotifyBlockLevelChanged();
  }
  NotifyBlockedSitesChanged();
}

void AstraFocusModeModel::RemoveBlockedSite(const GURL& url) {
  auto it = std::find(blocked_sites_.begin(), blocked_sites_.end(), url);
  if (it == blocked_sites_.end()) {
    return;
  }
  blocked_sites_.erase(it);
  NotifyBlockedSitesChanged();
}

const std::vector<GURL>& AstraFocusModeModel::GetBlockedSites() const {
  return blocked_sites_;
}

bool AstraFocusModeModel::IsSiteBlocked(const GURL& url) const {
  if (!url.is_valid()) {
    return false;
  }

  // Allowed sites always bypass blocking.
  for (const auto& allowed : allowed_sites_) {
    if (url == allowed || url.DomainIs(allowed.host())) {
      return false;
    }
  }

  switch (block_level_) {
    case AstraFocusBlockLevel::kNone:
      return false;

    case AstraFocusBlockLevel::kSocial:
      return IsSocialMediaDomain(url);

    case AstraFocusBlockLevel::kEntertainment:
      return IsSocialMediaDomain(url) || IsEntertainmentDomain(url);

    case AstraFocusBlockLevel::kNews:
      return IsSocialMediaDomain(url) || IsEntertainmentDomain(url) ||
             IsNewsDomain(url);

    case AstraFocusBlockLevel::kStrict:
      // Block everything except work sites and explicit allowlist.
      return !IsWorkDomain(url);

    case AstraFocusBlockLevel::kCustom:
      // Check custom blocklist.
      for (const auto& blocked : blocked_sites_) {
        if (url == blocked || url.DomainIs(blocked.host())) {
          return true;
        }
      }
      return false;
  }
  return false;
}

void AstraFocusModeModel::AddAllowedSite(const GURL& url) {
  if (!url.is_valid()) {
    return;
  }
  for (const auto& site : allowed_sites_) {
    if (site == url) {
      return;
    }
  }
  allowed_sites_.push_back(url);
  // Allowed sites affect block behavior, so notify.
  NotifyBlockedSitesChanged();
}

void AstraFocusModeModel::RemoveAllowedSite(const GURL& url) {
  auto it = std::find(allowed_sites_.begin(), allowed_sites_.end(), url);
  if (it == allowed_sites_.end()) {
    return;
  }
  allowed_sites_.erase(it);
  NotifyBlockedSitesChanged();
}

const std::vector<GURL>& AstraFocusModeModel::GetAllowedSites() const {
  return allowed_sites_;
}

void AstraFocusModeModel::ClearBlockedSites() {
  if (blocked_sites_.empty()) {
    return;
  }
  blocked_sites_.clear();
  NotifyBlockedSitesChanged();
}

void AstraFocusModeModel::ClearAllowedSites() {
  if (allowed_sites_.empty()) {
    return;
  }
  allowed_sites_.clear();
  NotifyBlockedSitesChanged();
}

// -- Focus sessions / history ----------------------------------------------

const std::vector<AstraFocusSession>& AstraFocusModeModel::GetSessionHistory()
    const {
  return session_history_;
}

size_t AstraFocusModeModel::GetSessionCount() const {
  return session_history_.size();
}

size_t AstraFocusModeModel::GetTodaySessionCount() const {
  base::Time today = base::Time::Now().LocalMidnight();
  size_t count = 0;
  for (const auto& session : session_history_) {
    if (session.start_time >= today && session.is_completed) {
      ++count;
    }
  }
  return count;
}

base::TimeDelta AstraFocusModeModel::GetTodayTotalFocusTime() const {
  base::Time today = base::Time::Now().LocalMidnight();
  base::TimeDelta total;
  for (const auto& session : session_history_) {
    if (session.start_time >= today && session.is_completed) {
      total += session.duration;
    }
  }
  return total;
}

base::TimeDelta AstraFocusModeModel::GetWeekTotalFocusTime() const {
  base::Time today = base::Time::Now();
  base::Time week_ago = today - base::Days(7);
  base::TimeDelta total;
  for (const auto& session : session_history_) {
    if (session.start_time >= week_ago && session.is_completed) {
      total += session.duration;
    }
  }
  return total;
}

void AstraFocusModeModel::ClearSessionHistory() {
  session_history_.clear();
}

absl::optional<AstraFocusSession> AstraFocusModeModel::GetCurrentSession()
    const {
  if (!is_active_) {
    return absl::nullopt;
  }
  AstraFocusSession session;
  session.session_id = current_preset_.empty() ? "current" : current_preset_;
  session.start_time = session_start_time_;
  session.end_time = base::Time();  // Not ended yet.
  session.duration = elapsed_time_;
  session.is_completed = false;
  session.block_level = block_level_;
  session.distraction_count = static_cast<int>(distractions_blocked_);
  session.note = session_note_;
  return session;
}

void AstraFocusModeModel::SetSessionNote(const std::string& note) {
  session_note_ = note;
}

std::string AstraFocusModeModel::GetSessionNote() const {
  return session_note_;
}

// -- UI settings -----------------------------------------------------------

void AstraFocusModeModel::SetShowIndicator(bool show) {
  if (show_indicator_ == show) {
    return;
  }
  show_indicator_ = show;
  // Also update pref-backed setting if available.
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefFocusModeShowIndicator, show);
  }
}

bool AstraFocusModeModel::GetShowIndicator() const {
  return show_indicator_;
}

void AstraFocusModeModel::SetIndicatorPosition(AstraIndicatorPosition position) {
  if (indicator_position_ == position) {
    return;
  }
  indicator_position_ = position;
  // Also update pref-backed setting if available.
  if (pref_service_) {
    pref_service_->SetInteger(prefs::kPrefFocusModeIndicatorPosition,
                              static_cast<int>(position));
  }
}

AstraIndicatorPosition AstraFocusModeModel::GetIndicatorPosition() const {
  return indicator_position_;
}

void AstraFocusModeModel::SetShowTimer(bool show) {
  if (show_timer_ == show) {
    return;
  }
  show_timer_ = show;
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefFocusModeShowTimerInIndicator, show);
  }
}

bool AstraFocusModeModel::GetShowTimer() const {
  return show_timer_;
}

void AstraFocusModeModel::SetSoundEnabled(bool enabled) {
  if (sound_enabled_ == enabled) {
    return;
  }
  sound_enabled_ = enabled;
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefFocusModeNotificationSound, enabled);
  }
}

bool AstraFocusModeModel::GetSoundEnabled() const {
  return sound_enabled_;
}

void AstraFocusModeModel::SetNotificationEnabled(bool enabled) {
  if (notification_enabled_ == enabled) {
    return;
  }
  notification_enabled_ = enabled;
  // TODO(astra): Wire to Chromium notifications API for end-of-session alerts.
  //   Chromium owner: NotificationService / message_center.
  //   Patch point: Use NotificationUIManager for session end notifications.
}

bool AstraFocusModeModel::GetNotificationEnabled() const {
  return notification_enabled_;
}

void AstraFocusModeModel::SetShowBreakReminder(bool show) {
  if (show_break_reminder_ == show) {
    return;
  }
  show_break_reminder_ = show;
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefFocusModeBreakReminders, show);
  }
}

bool AstraFocusModeModel::GetShowBreakReminder() const {
  return show_break_reminder_;
}

void AstraFocusModeModel::SetBreakInterval(base::TimeDelta interval) {
  if (interval.is_negative()) {
    interval = base::TimeDelta();
  }
  break_interval_ = interval;
  if (pref_service_) {
    pref_service_->SetInteger(prefs::kPrefFocusModeBreakIntervalMinutes,
                              static_cast<int>(interval.InMinutes()));
  }
}

base::TimeDelta AstraFocusModeModel::GetBreakInterval() const {
  return break_interval_;
}

// -- Default durations -----------------------------------------------------

void AstraFocusModeModel::SetDefaultDuration(base::TimeDelta duration) {
  if (duration.is_negative()) {
    duration = base::TimeDelta();
  }
  default_duration_ = duration;
  if (pref_service_) {
    pref_service_->SetInteger(prefs::kPrefFocusModeDefaultDuration,
                              static_cast<int>(duration.InMinutes()));
  }
}

base::TimeDelta AstraFocusModeModel::GetDefaultDuration() const {
  return default_duration_;
}

// static
const std::vector<base::TimeDelta>& AstraFocusModeModel::GetPresetDurations() {
  static const std::vector<base::TimeDelta> presets(
      std::begin(kDefaultPresetDurations), std::end(kDefaultPresetDurations));
  return presets;
}

void AstraFocusModeModel::SetPreset(int index, base::TimeDelta duration) {
  // Presets are static defaults and not dynamically modifiable in this model.
  // This is a no-op for the base model.
  // TODO(astra): Support custom user presets persisted via PrefService.
}

// -- Presentation settings (legacy getters for compatibility) --------------

bool AstraFocusModeModel::auto_start() const {
  if (!pref_service_) {
    return false;
  }
  return pref_service_->GetBoolean(prefs::kPrefFocusModeAutoStart);
}

void AstraFocusModeModel::SetAutoStart(bool enabled) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefFocusModeAutoStart, enabled);
}

std::string AstraFocusModeModel::auto_start_time() const {
  if (!pref_service_) {
    return "09:00";
  }
  return pref_service_->GetString(prefs::kPrefFocusModeAutoStartTime);
}

void AstraFocusModeModel::SetAutoStartTime(const std::string& time_hhmm) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetString(prefs::kPrefFocusModeAutoStartTime, time_hhmm);
}

int AstraFocusModeModel::default_session_duration_minutes() const {
  if (!pref_service_) {
    return static_cast<int>(default_duration_.InMinutes());
  }
  return pref_service_->GetInteger(prefs::kPrefFocusModeDefaultDuration);
}

void AstraFocusModeModel::SetDefaultSessionDurationMinutes(int minutes) {
  if (!pref_service_) {
    default_duration_ = base::Minutes(minutes);
    return;
  }
  pref_service_->SetInteger(prefs::kPrefFocusModeDefaultDuration, minutes);
  default_duration_ = base::Minutes(minutes);
}

bool AstraFocusModeModel::show_focus_indicator() const {
  return show_indicator_;
}

void AstraFocusModeModel::SetShowFocusIndicator(bool show) {
  SetShowIndicator(show);
}

AstraIndicatorPosition AstraFocusModeModel::indicator_position() const {
  return indicator_position_;
}

void AstraFocusModeModel::SetIndicatorPosition(AstraIndicatorPosition position) {
  SetIndicatorPosition(position);
}

AstraIndicatorStyle AstraFocusModeModel::indicator_style() const {
  if (!pref_service_) {
    return AstraIndicatorStyle::kFull;
  }
  return static_cast<AstraIndicatorStyle>(
      pref_service_->GetInteger(prefs::kPrefFocusModeIndicatorStyle));
}

void AstraFocusModeModel::SetIndicatorStyle(AstraIndicatorStyle style) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefFocusModeIndicatorStyle,
                            static_cast<int>(style));
}

bool AstraFocusModeModel::show_timer_in_indicator() const {
  return show_timer_;
}

void AstraFocusModeModel::SetShowTimerInIndicator(bool show) {
  SetShowTimer(show);
}

bool AstraFocusModeModel::show_session_stats() const {
  if (!pref_service_) {
    return true;
  }
  return pref_service_->GetBoolean(prefs::kPrefFocusModeShowSessionStats);
}

void AstraFocusModeModel::SetShowSessionStats(bool show) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefFocusModeShowSessionStats, show);
}

bool AstraFocusModeModel::block_distracting_sites() const {
  return block_level_ != AstraFocusBlockLevel::kNone;
}

void AstraFocusModeModel::SetBlockDistractingSites(bool block) {
  if (block && block_level_ == AstraFocusBlockLevel::kNone) {
    SetBlockLevel(AstraFocusBlockLevel::kSocial);
  } else if (!block && block_level_ != AstraFocusBlockLevel::kNone) {
    SetBlockLevel(AstraFocusBlockLevel::kNone);
  }
}

bool AstraFocusModeModel::show_distraction_warnings() const {
  if (!pref_service_) {
    return true;
  }
  return pref_service_->GetBoolean(prefs::kPrefFocusModeWarningsEnabled);
}

void AstraFocusModeModel::SetShowDistractionWarnings(bool show) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefFocusModeWarningsEnabled, show);
}

bool AstraFocusModeModel::notification_sound() const {
  return sound_enabled_;
}

void AstraFocusModeModel::SetNotificationSound(bool enabled) {
  SetSoundEnabled(enabled);
}

bool AstraFocusModeModel::break_reminders() const {
  return show_break_reminder_;
}

void AstraFocusModeModel::SetBreakReminders(bool enabled) {
  SetShowBreakReminder(enabled);
}

int AstraFocusModeModel::break_interval_minutes() const {
  return static_cast<int>(break_interval_.InMinutes());
}

void AstraFocusModeModel::SetBreakIntervalMinutes(int minutes) {
  SetBreakInterval(base::Minutes(minutes));
}

int AstraFocusModeModel::break_duration_minutes() const {
  if (!pref_service_) {
    return 5;
  }
  return pref_service_->GetInteger(prefs::kPrefFocusModeBreakDurationMinutes);
}

void AstraFocusModeModel::SetBreakDurationMinutes(int minutes) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefFocusModeBreakDurationMinutes, minutes);
}

bool AstraFocusModeModel::dim_non_focus_tabs() const {
  if (!pref_service_) {
    return false;
  }
  return pref_service_->GetBoolean(prefs::kPrefFocusModeDimNonFocusTabs);
}

void AstraFocusModeModel::SetDimNonFocusTabs(bool dim) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefFocusModeDimNonFocusTabs, dim);
}

bool AstraFocusModeModel::hide_sidebar_in_focus_mode() const {
  if (!pref_service_) {
    return true;
  }
  return pref_service_->GetBoolean(prefs::kPrefFocusModeHideSidebar);
}

void AstraFocusModeModel::SetHideSidebarInFocusMode(bool hide) {
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefFocusModeHideSidebar, hide);
}

// -- Session state (legacy setters) ----------------------------------------

void AstraFocusModeModel::SetPaused(bool paused) {
  if (is_paused_ == paused) {
    return;
  }
  is_paused_ = paused;
  if (paused) {
    NotifyFocusModePaused();
  } else {
    NotifyFocusModeResumed();
  }
}

void AstraFocusModeModel::SetSessionStartTime(base::Time start_time) {
  session_start_time_ = start_time;
}

void AstraFocusModeModel::SetElapsedTime(base::TimeDelta elapsed) {
  if (elapsed_time_ == elapsed) {
    return;
  }
  elapsed_time_ = elapsed;
}

void AstraFocusModeModel::SetRemainingTime(base::TimeDelta remaining) {
  if (remaining_time_ == remaining) {
    return;
  }
  remaining_time_ = remaining;
  NotifyFocusTimeUpdated();
}

void AstraFocusModeModel::SetTotalDuration(base::TimeDelta duration) {
  total_duration_ = duration;
}

void AstraFocusModeModel::SetCurrentPreset(const std::string& preset_name) {
  current_preset_ = preset_name;
}

void AstraFocusModeModel::SetDistractionsBlocked(size_t count) {
  distractions_blocked_ = count;
}

void AstraFocusModeModel::IncrementDistractionsBlocked(const GURL& url) {
  ++distractions_blocked_;
}

// -- Session stats (legacy) ------------------------------------------------

void AstraFocusModeModel::SetTotalFocusMinutesToday(int minutes) {
  total_focus_minutes_today_ = minutes;
}

void AstraFocusModeModel::AddFocusMinutesToday(int minutes) {
  total_focus_minutes_today_ += minutes;
}

void AstraFocusModeModel::SetSessionsToday(int count) {
  sessions_today_ = count;
}

void AstraFocusModeModel::IncrementSessionsToday() {
  ++sessions_today_;
}

void AstraFocusModeModel::SetCurrentStreakDays(int days) {
  current_streak_days_ = days;
}

// -- Session presets (legacy) ----------------------------------------------

// static
const std::vector<int>& AstraFocusModeModel::GetDefaultPresetDurations() {
  static const std::vector<int> presets(
      std::begin(kDefaultPresetDurationsMinutes),
      std::end(kDefaultPresetDurationsMinutes));
  return presets;
}

// -- Utility methods -------------------------------------------------------

// static
std::u16string AstraFocusModeModel::FormatDuration(base::TimeDelta duration) {
  if (duration < base::Minutes(1)) {
    int seconds = static_cast<int>(duration.InSeconds());
    return base::ASCIIToUTF16(base::StringPrintf("%ds", seconds));
  }
  if (duration < base::Hours(1)) {
    int total_seconds = static_cast<int>(duration.InSeconds());
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    return base::ASCIIToUTF16(
        base::StringPrintf("%02d:%02d", minutes, seconds));
  }
  int total_minutes = static_cast<int>(duration.InMinutes());
  int hours = total_minutes / 60;
  int minutes = total_minutes % 60;
  return base::ASCIIToUTF16(
      base::StringPrintf("%dh %02dm", hours, minutes));
}

// static
std::u16string AstraFocusModeModel::FormatTimeRemaining(
    base::TimeDelta remaining) {
  if (remaining.is_negative()) {
    remaining = base::TimeDelta();
  }
  if (remaining < base::Hours(1)) {
    int total_seconds = static_cast<int>(remaining.InSeconds());
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    return base::ASCIIToUTF16(
        base::StringPrintf("%02d:%02d", minutes, seconds));
  }
  int total_seconds = static_cast<int>(remaining.InSeconds());
  int hours = total_seconds / 3600;
  int minutes = (total_seconds % 3600) / 60;
  int seconds = total_seconds % 60;
  return base::ASCIIToUTF16(
      base::StringPrintf("%d:%02d:%02d", hours, minutes, seconds));
}

// static
double AstraFocusModeModel::CalculateProgressPercentage(
    base::TimeDelta elapsed,
    base::TimeDelta total) {
  if (total.is_zero() || total.is_negative()) {
    return 0.0;
  }
  if (elapsed <= base::TimeDelta()) {
    return 0.0;
  }
  if (elapsed >= total) {
    return 1.0;
  }
  return elapsed.InSecondsF() / total.InSecondsF();
}

double AstraFocusModeModel::SessionProgress() const {
  return CalculateProgressPercentage(elapsed_time_, total_duration_);
}

void AstraFocusModeModel::ResetSessionState() {
  is_active_ = false;
  is_paused_ = false;
  session_start_time_ = base::Time();
  elapsed_time_ = base::TimeDelta();
  remaining_time_ = base::TimeDelta();
  total_duration_ = base::TimeDelta();
  current_preset_.clear();
  distractions_blocked_ = 0;
  session_note_.clear();
}

void AstraFocusModeModel::ResetStats() {
  total_focus_minutes_today_ = 0;
  sessions_today_ = 0;
  current_streak_days_ = 0;
}

// -- Private helpers -------------------------------------------------------

// static
std::string AstraFocusModeModel::GenerateSessionId() {
  return base::GenerateGUID();
}

void AstraFocusModeModel::RecordCompletedSession() {
  AstraFocusSession session;
  session.session_id = GenerateSessionId();
  session.start_time = session_start_time_;
  session.end_time = base::Time::Now();
  session.duration = elapsed_time_;
  session.is_completed = true;
  session.block_level = block_level_;
  session.distraction_count = static_cast<int>(distractions_blocked_);
  session.note = session_note_;

  session_history_.push_back(session);

  // Update today's stats.
  base::Time today = base::Time::Now().LocalMidnight();
  if (session.start_time >= today) {
    total_focus_minutes_today_ +=
        static_cast<int>(session.duration.InMinutes());
    ++sessions_today_;
  }

  NotifySessionCompleted(session);
}

void AstraFocusModeModel::LoadFromPrefs() {
  DCHECK(pref_service_);
  // Settings are read on-demand from pref_service_ in the getter methods.
  // This method syncs in-memory defaults from prefs for settings that have
  // dual representations.

  show_indicator_ =
      pref_service_->GetBoolean(prefs::kPrefFocusModeShowIndicator);
  indicator_position_ = static_cast<AstraIndicatorPosition>(
      pref_service_->GetInteger(prefs::kPrefFocusModeIndicatorPosition));
  show_timer_ =
      pref_service_->GetBoolean(prefs::kPrefFocusModeShowTimerInIndicator);
  sound_enabled_ =
      pref_service_->GetBoolean(prefs::kPrefFocusModeNotificationSound);
  show_break_reminder_ =
      pref_service_->GetBoolean(prefs::kPrefFocusModeBreakReminders);
  break_interval_ = base::Minutes(
      pref_service_->GetInteger(prefs::kPrefFocusModeBreakIntervalMinutes));
  default_duration_ = base::Minutes(
      pref_service_->GetInteger(prefs::kPrefFocusModeDefaultDuration));
}

void AstraFocusModeModel::NotifyFocusModeStarted() {
  for (auto& observer : observers_) {
    observer.OnFocusModeStarted(this);
  }
}

void AstraFocusModeModel::NotifyFocusModeEnded() {
  for (auto& observer : observers_) {
    observer.OnFocusModeEnded(this);
  }
}

void AstraFocusModeModel::NotifyFocusModePaused() {
  for (auto& observer : observers_) {
    observer.OnFocusModePaused(this);
  }
}

void AstraFocusModeModel::NotifyFocusModeResumed() {
  for (auto& observer : observers_) {
    observer.OnFocusModeResumed(this);
  }
}

void AstraFocusModeModel::NotifyFocusTimeUpdated() {
  for (auto& observer : observers_) {
    observer.OnFocusTimeUpdated(this, remaining_time_);
  }
}

void AstraFocusModeModel::NotifyBlockLevelChanged() {
  for (auto& observer : observers_) {
    observer.OnBlockLevelChanged(this, block_level_);
  }
}

void AstraFocusModeModel::NotifyBlockedSitesChanged() {
  for (auto& observer : observers_) {
    observer.OnBlockedSitesChanged(this);
  }
}

void AstraFocusModeModel::NotifySessionCompleted(
    const AstraFocusSession& session) {
  for (auto& observer : observers_) {
    observer.OnSessionCompleted(this, session);
  }
}

void AstraFocusModeModel::NotifyModelShutdown() {
  for (auto& observer : observers_) {
    observer.OnFocusModeModelShutdown(this);
  }
}

}  // namespace astra
