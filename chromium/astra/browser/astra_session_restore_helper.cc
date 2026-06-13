#include "astra/browser/astra_session_restore_helper.h"

#include <set>
#include <vector>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_session_metadata.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_window_features.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents.h"

namespace astra {

// ---------------------------------------------------------------------------
// Observer management
// ---------------------------------------------------------------------------

// static
base::ObserverList<AstraSessionRestoreObserver>&
AstraSessionRestoreHelper::GetObservers() {
  static base::NoDestructor<base::ObserverList<AstraSessionRestoreObserver>>
      observers;
  return *observers;
}

// static
void AstraSessionRestoreHelper::AddObserver(
    AstraSessionRestoreObserver* observer) {
  DCHECK(observer);
  GetObservers().AddObserver(observer);
}

// static
void AstraSessionRestoreHelper::RemoveObserver(
    AstraSessionRestoreObserver* observer) {
  DCHECK(observer);
  GetObservers().RemoveObserver(observer);
}

// ---------------------------------------------------------------------------
// Tab-level save / restore
// ---------------------------------------------------------------------------

// static
bool AstraSessionRestoreHelper::OnWillRestoreTab(
    Profile* /* profile */,
    content::WebContents* web_contents,
    const base::Value::Dict& extra_data) {
  if (!web_contents || extra_data.empty()) {
    return false;
  }

  // Validate metadata before applying to catch corrupt session data early.
  if (!ValidateAstraTabMetadata(extra_data)) {
    // TODO(astra): Log a warning about invalid session metadata.
    // Chromium component: logging via base/logging.h or UMA.
    return false;
  }

  if (!HasAstraMetadata(extra_data)) {
    return false;
  }

  // If restore is disabled for Astra metadata, skip application.
  // Note: We check via the web contents' profile.  For now this is a
  // no-op since we don't have easy profile access from WebContents here.
  // TODO(astra): Add profile-based enable check when the profile parameter
  // is fully wired.  Currently the profile parameter is accepted but
  // unused for workspace validation.
  // Chromium component: Profile (chrome/browser/profiles/profile.h).

  // TODO(astra): Validate workspace_id against the profile's workspace list.
  // If the workspace no longer exists, fall back to the default workspace.
  // This requires AstraWorkspaceService to be accessible here.
  // Chromium component: AstraWorkspaceService (ProfileKeyedService).

  ApplyAstraMetadataToWebContents(extra_data, web_contents);

  // Record restore stats.
  RecordTabRestoreStats(extra_data);

  return true;
}

// static
base::Value::Dict AstraSessionRestoreHelper::OnWillSaveTab(
    content::WebContents* web_contents) {
  base::Value::Dict metadata = ExtractAstraMetadataFromWebContents(web_contents);

  // Validate the metadata we're about to save to ensure we don't persist
  // corrupt data.  If validation fails, clear the dict so no Astra data
  // is written (safer than writing bad data).
  if (!metadata.empty() && !ValidateAstraTabMetadata(metadata)) {
    // TODO(astra): Log a warning about invalid metadata on save.
    // This would indicate a bug in AstraTabFeatures or the serialization.
    metadata.clear();
  }

  return metadata;
}

// static
void AstraSessionRestoreHelper::OnTabRestored(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  // Post-restore tab work:
  //   - Split view partner re-linking: when both tabs in a split pair have
  //     been restored, we may want to validate that the partner still exists
  //     and that the partner_id maps to a real tab.  For now, split view
  //     metadata is restored per-tab, and the split view controller resolves
  //     partners lazily when the UI is first shown.
  //   - Workspace validation: if the workspace no longer exists, the tab
  //     should be reassigned to the default workspace.
  //
  // TODO(astra): Evaluate whether split view partner resolution needs to
  // happen eagerly at restore time or can remain lazy.  Lazy resolution is
  // simpler and plays better with tab discarding and lazy session restore.

  // Notify all observers that a tab has been restored.
  for (auto& observer : GetObservers()) {
    observer.OnTabRestored(web_contents);
  }
}

// static
void AstraSessionRestoreHelper::OnTabRestoredAtIndex(int tab_index) {
  for (auto& observer : GetObservers()) {
    observer.OnTabRestored(tab_index);
  }
}

// ---------------------------------------------------------------------------
// Window-level save / restore
// ---------------------------------------------------------------------------

// static
base::Value::Dict AstraSessionRestoreHelper::OnWillSaveWindow(
    Profile* /* profile */,
    Browser* browser) {
  if (!browser) {
    return base::Value::Dict();
  }

  // Extract window-level Astra metadata from the Browser.
  // This includes workspace_id, saved bounds, minimized/maximized state,
  // and sidebar state.
  //
  // Chromium component: sessions::SessionWindow / BaseSessionService.
  // Patch point: chrome/browser/sessions/base_session_service.cc — where
  // window data is serialized for persistence.
  //
  // TODO(astra): Also include window-level sidebar state (visibility, width)
  // once per-window sidebar state is implemented in AstraWindowFeatures.
  // Currently sidebar state is profile-level (see astra_prefs.h).
  base::Value::Dict metadata = ExtractAstraWindowMetadataFromBrowser(browser);

  // Validate before saving.
  if (!metadata.empty() && !ValidateAstraWindowMetadata(metadata)) {
    // TODO(astra): Log a warning about invalid window metadata on save.
    metadata.clear();
  }

  return metadata;
}

// static
bool AstraSessionRestoreHelper::OnWillRestoreWindow(
    Profile* /* profile */,
    Browser* browser,
    const base::Value::Dict& extra_data) {
  if (!browser || extra_data.empty()) {
    return false;
  }

  // Validate metadata before applying.
  if (!ValidateAstraWindowMetadata(extra_data)) {
    return false;
  }

  if (!HasAstraWindowMetadata(extra_data)) {
    return false;
  }

  // Apply window-level Astra metadata from session restore.
  // This restores workspace_id, saved bounds, and window state.
  //
  // Chromium component: sessions::SessionRestore — where Browser windows are
  // created during session restore.
  // Patch point: chrome/browser/sessions/session_restore.cc — where a new
  // Browser is created from a SessionWindow.
  //
  // TODO(astra): Validate workspace_id against the profile's workspace list.
  // If the workspace no longer exists, fall back to the default workspace.
  // This requires AstraWorkspaceService to be accessible here.
  // Chromium component: AstraWorkspaceService (ProfileKeyedService).
  ApplyAstraWindowMetadataToBrowser(extra_data, browser);

  // Record restore stats.
  RecordWindowRestoreStats(extra_data);

  // TODO(astra): After restoring the window, we may want to ensure the
  // window's tabs are all in the same workspace (Arc-style model).
  // Currently tabs have their own workspace_id via AstraTabFeatures.
  // For the window-per-workspace model, we could normalize all tabs to
  // the window's workspace on restore.

  return true;
}

// static
void AstraSessionRestoreHelper::OnWindowRestored(Browser* browser) {
  if (!browser) {
    return;
  }

  // Post-restore window work:
  //   - Workspace assignment validation.
  //   - Sidebar state reconciliation with profile-level defaults.
  //   - Window bounds validation against the current display configuration.

  // Notify all observers that a window has been restored.
  for (auto& observer : GetObservers()) {
    observer.OnWindowRestored(browser);
  }
}

// ---------------------------------------------------------------------------
// Batch session restore notification
// ---------------------------------------------------------------------------

// static
void AstraSessionRestoreHelper::OnSessionRestoreComplete(Profile* profile) {
  if (!profile) {
    return;
  }

  // Mark restore as complete in stats.
  AstraSessionRestoreStats& stats = GetMutableRestoreStats();
  stats.restore_in_progress = false;
  stats.restore_end_time = base::Time::Now();

  // TODO(astra): After all windows/tabs are restored, do any batch
  // reconciliation:
  //   - Validate all workspace IDs against the workspace list.
  //   - Resolve all split view partner references.
  //   - Rebuild tab stack membership indices.
  //   - Trigger UI refresh for sidebar and workspace switcher.

  for (auto& observer : GetObservers()) {
    observer.OnSessionRestoreComplete(profile);
    observer.OnSessionRestoreCompleted();
  }
}

// ---------------------------------------------------------------------------
// Restore stats
// ---------------------------------------------------------------------------

// static
const AstraSessionRestoreStats& AstraSessionRestoreHelper::GetRestoreStats() {
  return GetMutableRestoreStats();
}

// static
void AstraSessionRestoreHelper::ResetRestoreStats() {
  GetMutableRestoreStats().Reset();
}

// static
AstraSessionRestoreStats& AstraSessionRestoreHelper::GetMutableRestoreStats() {
  static base::NoDestructor<AstraSessionRestoreStats> stats;
  return *stats;
}

// static
const AstraSessionRestoreStats& AstraSessionRestoreHelper::GetLastRestoreStats() {
  return GetRestoreStats();
}

// static
base::TimeDelta AstraSessionRestoreHelper::GetRestoreDuration() {
  return GetRestoreStats().GetRestoreDuration();
}

// static
double AstraSessionRestoreHelper::GetRestoreSuccessRate() {
  return GetRestoreStats().GetRestoreSuccessRate();
}

// static
bool AstraSessionRestoreHelper::IsRestoreInProgress() {
  return GetRestoreStats().IsRestoreInProgress();
}

// ---------------------------------------------------------------------------
// Session restore lifecycle
// ---------------------------------------------------------------------------

// static
void AstraSessionRestoreHelper::OnSessionRestoreStarted() {
  AstraSessionRestoreStats& stats = GetMutableRestoreStats();
  stats.Reset();
  stats.restore_in_progress = true;
  stats.restore_start_time = base::Time::Now();

  for (auto& observer : GetObservers()) {
    observer.OnSessionRestoreStarted();
  }
}

// static
void AstraSessionRestoreHelper::OnSessionRestoreFailed(const std::string& error) {
  AstraSessionRestoreStats& stats = GetMutableRestoreStats();
  stats.restore_in_progress = false;
  stats.restore_end_time = base::Time::Now();

  for (auto& observer : GetObservers()) {
    observer.OnSessionRestoreFailed(error);
  }
}

// ---------------------------------------------------------------------------
// Prefs helper
// ---------------------------------------------------------------------------

// static
PrefService* AstraSessionRestoreHelper::GetPrefs(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

// ---------------------------------------------------------------------------
// Session restore mode
// ---------------------------------------------------------------------------

// static
SessionRestoreMode AstraSessionRestoreHelper::GetRestoreMode(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return SessionRestoreMode::kRestoreAll;
  }

  std::string mode_str = prefs->GetString(prefs::kPrefSessionRestoreMode);
  return RestoreModeFromString(mode_str);
}

// static
void AstraSessionRestoreHelper::SetRestoreMode(Profile* profile,
                                                SessionRestoreMode mode) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  prefs->Set(prefs::kPrefSessionRestoreMode,
             base::Value(RestoreModeToString(mode)));
}

// static
bool AstraSessionRestoreHelper::IsRestoreEnabled(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefSessionRestoreEnabled);
}

// static
void AstraSessionRestoreHelper::SetRestoreEnabled(Profile* profile,
                                                   bool enabled) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSessionRestoreEnabled, enabled);
}

// static
bool AstraSessionRestoreHelper::IsLazyLoadingEnabled(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreLazyLoading;
  }
  return prefs->GetBoolean(prefs::kPrefSessionRestoreLazyLoading);
}

// static
bool AstraSessionRestoreHelper::ShouldShowRestorePrompt(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreShowPrompt;
  }
  return prefs->GetBoolean(prefs::kPrefSessionRestoreShowPrompt);
}

// ---------------------------------------------------------------------------
// Restore load mode
// ---------------------------------------------------------------------------

// static
SessionRestoreLoadMode AstraSessionRestoreHelper::GetRestoreLoadMode(
    Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return SessionRestoreLoadMode::kSmart;
  }

  std::string mode_str =
      prefs->GetString(prefs::kPrefSessionRestoreLoadMode);
  return RestoreLoadModeFromString(mode_str);
}

// static
void AstraSessionRestoreHelper::SetRestoreLoadMode(
    Profile* profile,
    SessionRestoreLoadMode mode) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  prefs->Set(prefs::kPrefSessionRestoreLoadMode,
             base::Value(RestoreLoadModeToString(mode)));
}

// static
std::string AstraSessionRestoreHelper::RestoreLoadModeToString(
    SessionRestoreLoadMode mode) {
  switch (mode) {
    case SessionRestoreLoadMode::kFull:
      return "full";
    case SessionRestoreLoadMode::kLazy:
      return "lazy";
    case SessionRestoreLoadMode::kSmart:
      return "smart";
    case SessionRestoreLoadMode::kMinimal:
      return "minimal";
  }
  return "smart";
}

// static
SessionRestoreLoadMode AstraSessionRestoreHelper::RestoreLoadModeFromString(
    const std::string& mode_str) {
  if (mode_str == "full") {
    return SessionRestoreLoadMode::kFull;
  }
  if (mode_str == "lazy") {
    return SessionRestoreLoadMode::kLazy;
  }
  if (mode_str == "minimal") {
    return SessionRestoreLoadMode::kMinimal;
  }
  // Default to "smart" for unrecognized values.
  return SessionRestoreLoadMode::kSmart;
}

// ---------------------------------------------------------------------------
// Session mode string helpers
// ---------------------------------------------------------------------------

// static
std::string AstraSessionRestoreHelper::RestoreModeToString(
    SessionRestoreMode mode) {
  switch (mode) {
    case SessionRestoreMode::kRestoreAll:
      return "all";
    case SessionRestoreMode::kRestoreLast:
      return "last";
    case SessionRestoreMode::kRestoreNone:
      return "none";
  }
  // Fallback for future values.
  return "all";
}

// static
SessionRestoreMode AstraSessionRestoreHelper::RestoreModeFromString(
    const std::string& mode_str) {
  if (mode_str == "last") {
    return SessionRestoreMode::kRestoreLast;
  }
  if (mode_str == "none") {
    return SessionRestoreMode::kRestoreNone;
  }
  // Default to "all" for unrecognized values (including "all").
  return SessionRestoreMode::kRestoreAll;
}

// ---------------------------------------------------------------------------
// Last session info
// ---------------------------------------------------------------------------

// static
AstraLastSessionInfo AstraSessionRestoreHelper::GetLastSessionInfo(Profile* profile) {
  AstraLastSessionInfo info;

  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return info;
  }

  double save_time_us =
      prefs->GetDouble(prefs::kPrefSessionRestoreLastSaveTime);
  if (save_time_us > 0.0) {
    info.last_save_time = base::Time::FromDeltaSinceWindowsEpoch(
        base::Microseconds(static_cast<int64_t>(save_time_us)));
  }

  info.tab_count = static_cast<size_t>(
      prefs->GetInteger(prefs::kPrefSessionRestoreLastTabCount));
  info.window_count = static_cast<size_t>(
      prefs->GetInteger(prefs::kPrefSessionRestoreLastWindowCount));
  info.workspace_count = static_cast<size_t>(
      prefs->GetInteger(prefs::kPrefSessionRestoreLastWorkspaceCount));

  return info;
}

// static
void AstraSessionRestoreHelper::RecordSessionSaved(Profile* profile,
                                                    size_t tab_count,
                                                    size_t window_count,
                                                    size_t workspace_count) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  // Record save time (microseconds since Windows epoch, matching base::Time).
  double now_us = static_cast<double>(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  prefs->SetDouble(prefs::kPrefSessionRestoreLastSaveTime, now_us);

  // Record counts.
  prefs->SetInteger(prefs::kPrefSessionRestoreLastTabCount,
                    static_cast<int>(tab_count));
  prefs->SetInteger(prefs::kPrefSessionRestoreLastWindowCount,
                    static_cast<int>(window_count));
  prefs->SetInteger(prefs::kPrefSessionRestoreLastWorkspaceCount,
                    static_cast<int>(workspace_count));

  // Notify observers.
  for (auto& observer : GetObservers()) {
    observer.OnSessionSaved(profile, tab_count, window_count);
  }
}

// ---------------------------------------------------------------------------
// Presentation settings
// ---------------------------------------------------------------------------

// static
bool AstraSessionRestoreHelper::GetRestoreOnStartup(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreOnStartup;
  }
  return prefs->GetBoolean(prefs::kPrefSessionRestoreOnStartup);
}

// static
void AstraSessionRestoreHelper::SetRestoreOnStartup(Profile* profile,
                                                     bool enabled) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSessionRestoreOnStartup, enabled);
}

// static
int AstraSessionRestoreHelper::GetMaxTabsPerWorkspaceRestore(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreMaxTabsPerWorkspace;
  }
  int value =
      prefs->GetInteger(prefs::kPrefSessionRestoreMaxTabsPerWorkspace);
  return std::max(0, value);
}

// static
void AstraSessionRestoreHelper::SetMaxTabsPerWorkspaceRestore(Profile* profile,
                                                              int max_tabs) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  int clamped = std::max(0, max_tabs);
  prefs->SetInteger(prefs::kPrefSessionRestoreMaxTabsPerWorkspace, clamped);
}

// static
bool AstraSessionRestoreHelper::GetShowRestorePrompt(Profile* profile) {
  return ShouldShowRestorePrompt(profile);
}

// static
void AstraSessionRestoreHelper::SetShowRestorePrompt(Profile* profile,
                                                      bool show) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSessionRestoreShowPrompt, show);
}

// static
bool AstraSessionRestoreHelper::GetRestoreWorkspacesIndividually(
    Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreWorkspacesIndividually;
  }
  return prefs->GetBoolean(prefs::kPrefSessionRestoreWorkspacesIndividually);
}

// static
void AstraSessionRestoreHelper::SetRestoreWorkspacesIndividually(
    Profile* profile,
    bool enabled) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSessionRestoreWorkspacesIndividually, enabled);
}

// static
bool AstraSessionRestoreHelper::GetBackgroundTabLoading(Profile* profile) {
  return IsLazyLoadingEnabled(profile);
}

// static
void AstraSessionRestoreHelper::SetBackgroundTabLoading(Profile* profile,
                                                        bool enabled) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSessionRestoreLazyLoading, enabled);
}

// static
int AstraSessionRestoreHelper::GetAutoSaveSessionInterval(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreAutoSaveInterval;
  }
  int value =
      prefs->GetInteger(prefs::kPrefSessionRestoreAutoSaveInterval);
  return std::max(0, value);
}

// static
void AstraSessionRestoreHelper::SetAutoSaveSessionInterval(Profile* profile,
                                                           int minutes) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  int clamped = std::max(0, minutes);
  prefs->SetInteger(prefs::kPrefSessionRestoreAutoSaveInterval, clamped);
}

// static
int AstraSessionRestoreHelper::GetMaxSavedSessions(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSessionRestoreMaxSavedSessions;
  }
  int value = prefs->GetInteger(prefs::kPrefSessionRestoreMaxSavedSessions);
  return std::max(1, value);
}

// static
void AstraSessionRestoreHelper::SetMaxSavedSessions(Profile* profile,
                                                    int max_sessions) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }
  int clamped = std::max(1, max_sessions);
  prefs->SetInteger(prefs::kPrefSessionRestoreMaxSavedSessions, clamped);
}

// ---------------------------------------------------------------------------
// Named session management
// ---------------------------------------------------------------------------

// static
void AstraSessionRestoreHelper::SaveSession(Profile* profile,
                                            const std::string& session_name) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs || session_name.empty()) {
    return;
  }

  // Build session metadata from current state.
  // TODO(astra): Populate with actual session data from Chromium's
  // SessionService.  For now, we store a minimal placeholder.
  // Chromium component: SessionService (chrome/browser/sessions/).
  // Patch point: SessionService::GetLastSession.
  AstraSessionMetadata session;
  session.session_name = session_name;
  session.save_time = base::Time::Now();

  // Store in the saved sessions dict pref.
  const base::Value::Dict& saved_sessions =
      prefs->GetDict(prefs::kPrefSessionRestoreSavedSessions);
  base::Value::Dict updated = saved_sessions.Clone();
  updated.Set(session_name, session.ToDict());

  // Enforce max saved sessions limit.
  int max_sessions = GetMaxSavedSessions(profile);
  if (static_cast<int>(updated.size()) > max_sessions) {
    // Remove oldest entries — for simplicity, just keep the newest N by
    // save_time.  Since we don't have timestamps easily accessible in the
    // dict keys, we iterate and find the oldest.
    // TODO(astra): Implement proper LRU eviction for saved sessions.
    // For now, we just let it grow beyond the limit on add, and trim
    // on next load.
  }

  prefs->Set(prefs::kPrefSessionRestoreSavedSessions,
             base::Value(std::move(updated)));

  // Notify observers.
  for (auto& observer : GetObservers()) {
    observer.OnSessionSaved(session_name);
    observer.OnSessionMetadataChanged();
  }
}

// static
AstraSessionMetadata AstraSessionRestoreHelper::LoadSession(
    Profile* profile,
    const std::string& session_name) {
  AstraSessionMetadata session;

  PrefService* prefs = GetPrefs(profile);
  if (!prefs || session_name.empty()) {
    return session;
  }

  const base::Value::Dict& saved_sessions =
      prefs->GetDict(prefs::kPrefSessionRestoreSavedSessions);
  const base::Value::Dict* session_dict =
      saved_sessions.FindDict(session_name);
  if (session_dict) {
    session.FromDict(*session_dict);
  }

  return session;
}

// static
bool AstraSessionRestoreHelper::DeleteSavedSession(
    Profile* profile,
    const std::string& session_name) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs || session_name.empty()) {
    return false;
  }

  const base::Value::Dict& saved_sessions =
      prefs->GetDict(prefs::kPrefSessionRestoreSavedSessions);
  if (!saved_sessions.contains(session_name)) {
    return false;
  }

  base::Value::Dict updated = saved_sessions.Clone();
  updated.Remove(session_name);
  prefs->Set(prefs::kPrefSessionRestoreSavedSessions,
             base::Value(std::move(updated)));

  // Notify observers.
  for (auto& observer : GetObservers()) {
    observer.OnSessionMetadataChanged();
  }

  return true;
}

// static
std::vector<std::string> AstraSessionRestoreHelper::ListSavedSessions(
    Profile* profile) {
  std::vector<std::string> names;

  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return names;
  }

  const base::Value::Dict& saved_sessions =
      prefs->GetDict(prefs::kPrefSessionRestoreSavedSessions);
  for (const auto [name, _] : saved_sessions) {
    names.push_back(name);
  }

  return names;
}

// static
bool AstraSessionRestoreHelper::HasLastSession(Profile* profile) {
  return GetLastSessionInfo(profile).has_session();
}

// static
AstraSessionMetadata AstraSessionRestoreHelper::GetLastSession(
    Profile* profile) {
  AstraSessionMetadata session;

  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return session;
  }

  AstraLastSessionInfo info = GetLastSessionInfo(profile);
  if (!info.has_session()) {
    return session;
  }

  session.session_name = "last";
  session.save_time = info.last_save_time;

  // TODO(astra): Load actual session data from Chromium's SessionService.
  // For now, we just populate the metadata with counts from the last
  // session info.
  // Chromium component: SessionService (chrome/browser/sessions/).

  return session;
}

// static
void AstraSessionRestoreHelper::ClearLastSession(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  prefs->SetDouble(prefs::kPrefSessionRestoreLastSaveTime, 0.0);
  prefs->SetInteger(prefs::kPrefSessionRestoreLastTabCount, 0);
  prefs->SetInteger(prefs::kPrefSessionRestoreLastWindowCount, 0);
  prefs->SetInteger(prefs::kPrefSessionRestoreLastWorkspaceCount, 0);

  // Notify observers.
  for (auto& observer : GetObservers()) {
    observer.OnSessionMetadataChanged();
  }
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

// static
std::vector<base::Value::Dict>
AstraSessionRestoreHelper::BulkExtractMetadataFromTabs(
    const std::vector<content::WebContents*>& web_contents_list) {
  std::vector<base::Value::Dict> results;
  results.reserve(web_contents_list.size());

  for (content::WebContents* web_contents : web_contents_list) {
    results.push_back(OnWillSaveTab(web_contents));
  }

  return results;
}

// static
size_t AstraSessionRestoreHelper::BulkApplyMetadataToTabs(
    const std::vector<base::Value::Dict>& metadata_list,
    const std::vector<content::WebContents*>& web_contents_list,
    Profile* profile) {
  if (metadata_list.size() != web_contents_list.size()) {
    // Mismatched sizes — can't apply bulk.
    // TODO(astra): Log a warning about mismatched sizes.
    return 0;
  }

  size_t applied_count = 0;

  for (size_t i = 0; i < metadata_list.size(); ++i) {
    if (OnWillRestoreTab(profile, web_contents_list[i], metadata_list[i])) {
      applied_count++;
    }
  }

  return applied_count;
}

// static
void AstraSessionRestoreHelper::RestoreAllWorkspaces(Profile* profile) {
  if (!profile) {
    return;
  }

  // TODO(astra): Restore all workspaces from saved session data.
  // This requires integration with Chromium's SessionRestore to create
  // windows and tabs for each workspace.
  // Chromium component: SessionRestore (chrome/browser/sessions/).
  // Patch point: SessionRestore::RestoreSession.

  // Mark restore as started.
  OnSessionRestoreStarted();

  // Simulate restoring workspaces — in production this would iterate
  // over workspace data and create windows/tabs.
  // For now, we just mark it complete after notifying start.
  // TODO(astra): Implement actual workspace restore using SessionRestore.

  // Mark as complete.
  OnSessionRestoreComplete(profile);
}

// static
bool AstraSessionRestoreHelper::RestoreWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile || workspace_id.empty()) {
    return false;
  }

  // TODO(astra): Restore a single workspace from saved session data.
  // Chromium component: SessionRestore / BrowserList.

  OnSessionRestoreStarted();

  // In production, create the window(s) and tabs for this workspace.
  // For now, we just simulate completion.

  OnSessionRestoreComplete(profile);
  return true;
}

// static
size_t AstraSessionRestoreHelper::CloseAllTabsInWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile || workspace_id.empty()) {
    return 0;
  }

  // TODO(astra): Close all tabs in a workspace using TabStripModel.
  // Chromium component: TabStripModel (chrome/browser/ui/tabs/).
  // For now, return 0 as a placeholder.

  // Notify observers of metadata change (tab count changed).
  for (auto& observer : GetObservers()) {
    observer.OnSessionMetadataChanged();
  }

  return 0;
}

// ---------------------------------------------------------------------------
// Stats recording helpers
// ---------------------------------------------------------------------------

// static
void AstraSessionRestoreHelper::RecordTabRestoreStats(
    const base::Value::Dict& metadata) {
  AstraSessionRestoreStats& stats = GetMutableRestoreStats();
  stats.tabs_restored++;

  if (HasAstraMetadata(metadata)) {
    stats.tabs_with_metadata++;
  }

  // Track favorite tabs.
  if (std::optional<bool> is_fav = metadata.FindBool(kMetaKeyIsFavorite)) {
    if (*is_fav) {
      stats.favorite_tabs_restored++;
    }
  }

  // Track split view pairs.
  // Count each tab with a partner; actual pair count would be divided by 2,
  // but we count per-tab for simplicity.
  if (std::optional<bool> in_split = metadata.FindBool(kMetaKeyIsInSplitView)) {
    if (*in_split) {
      // Each split view tab contributes half a pair.
      // We'll count pairs at the end by dividing by 2 approximately.
      stats.split_view_pairs_restored++;
    }
  }

  // Track tab stacks.
  // We track unique stack IDs in the workspace counter for now.
  if (metadata.contains(kMetaKeyTabStackId) ||
      metadata.contains(kMetaKeyStackParentId)) {
    stats.tab_stacks_restored++;
  }

  // Track PiP tabs.
  if (std::optional<bool> is_pip = metadata.FindBool(kMetaKeyIsPipTab)) {
    if (*is_pip) {
      stats.pip_tabs_restored++;
    }
  }

  // Track workspaces.
  if (const std::string* ws_id = metadata.FindString(kMetaKeyWorkspaceId)) {
    // We can't easily count unique workspaces in a static counter without
    // extra state.  The workspaces_restored counter is incremented in
    // OnWillRestoreWindow for window-level workspace tracking.
    // TODO(astra): Use a set to track unique workspace IDs during restore.
  }
}

// static
void AstraSessionRestoreHelper::RecordWindowRestoreStats(
    const base::Value::Dict& metadata) {
  AstraSessionRestoreStats& stats = GetMutableRestoreStats();
  stats.windows_restored++;

  if (HasAstraWindowMetadata(metadata)) {
    stats.windows_with_metadata++;
  }
}

}  // namespace astra
