#ifndef ASTRA_BROWSER_ASTRA_SESSION_RESTORE_HELPER_H_
#define ASTRA_BROWSER_ASTRA_SESSION_RESTORE_HELPER_H_

#include <cstddef>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/values.h"

#include "astra/browser/astra_session_metadata.h"

class Browser;
class PrefService;
class Profile;

namespace content {
class WebContents;
}

namespace astra {

// Observer interface for session restore events.
//
// UI layers and services can observe AstraSessionRestoreHelper to be
// notified when tabs or windows are restored with Astra metadata.
// This allows UI to update after restore completes without needing to
// poll or listen to lower-level Chromium events.
//
// Observers are registered statically since AstraSessionRestoreHelper
// is a static class.
//
// TODO(astra): Evaluate whether observers should be per-profile instead
// of global.  For now the observer list is global since session restore
// is a global event, but per-profile observers may be needed for multi-
// profile scenarios.
// Chromium component: Profile / ProfileManager (chrome/browser/profiles/).
class AstraSessionRestoreObserver : public base::CheckedObserver {
 public:
  // Called after a tab has been restored and its Astra metadata has been
  // applied.  At this point, AstraTabFeatures is fully populated with
  // the tab's restored metadata.
  //
  // The tab may not yet have finished loading (navigation is in progress),
  // but its Astra metadata (workspace, favorite, split view, etc.) is
  // available.
  virtual void OnTabRestored(content::WebContents* web_contents) {}

  // Called after a tab at a specific index has been restored.
  // Provides the tab index for progress tracking.
  virtual void OnTabRestored(int /* tab_index */) {}

  // Called after a window has been restored and its Astra metadata has been
  // applied.  At this point, AstraWindowFeatures is fully populated with
  // the window's restored metadata.
  //
  // The window's tabs may still be in the process of being restored, but
  // the window-level Astra state (workspace, saved bounds, etc.) is ready.
  virtual void OnWindowRestored(Browser* browser) {}

  // Called after all tabs and windows in a session restore batch have
  // been restored.  This is useful for UI that needs to do a single
  // refresh after all state is loaded.
  virtual void OnSessionRestoreComplete(Profile* profile) {}

  // Called when a session restore operation starts.
  virtual void OnSessionRestoreStarted() {}

  // Called when a session restore operation completes successfully.
  virtual void OnSessionRestoreCompleted() {}

  // Called when a session restore operation fails.
  // |error| contains a human-readable error description.
  virtual void OnSessionRestoreFailed(const std::string& /* error */) {}

  // Called when a session is saved (i.e., when Chromium's SessionService
  // persists a snapshot of the current session to disk).
  //
  // The stats parameter contains counts of tabs, windows, and workspaces
  // in the saved session.
  virtual void OnSessionSaved(Profile* /* profile */,
                              size_t /* tab_count */,
                              size_t /* window_count */) {}

  // Called when a named session is saved via SaveSession().
  virtual void OnSessionSaved(const std::string& /* session_name */) {}

  // Called when session metadata changes (e.g., after a merge operation
  // or when metadata is modified programmatically).
  virtual void OnSessionMetadataChanged() {}
};

// Mode controlling how many sessions are restored on startup.
//
// The actual restoration is performed by Chromium's SessionRestore;
// this enum represents Astra's configuration of which mode to use.
//
// TODO(astra): Map these modes to Chromium's session startup policies.
// Chromium owner: session_startup_pref (chrome/browser/prefs/)
enum class SessionRestoreMode {
  kRestoreAll,     // Restore all windows and tabs (default).
  kRestoreLast,    // Restore only the last active window.
  kRestoreNone,    // Don't restore anything — start with a blank new tab.
};

// Strategy controlling how tabs are loaded during session restore.
//
// These modes determine the aggressiveness of tab loading during restore.
// Chromium's actual tab loading is owned by SessionRestore; these modes
// control Astra's presentation and metadata participation.
enum class SessionRestoreLoadMode {
  kFull,     // Restore all tabs immediately.
  kLazy,     // Restore tabs lazily on activation.
  kSmart,    // Restore active workspace immediately, others lazily.
  kMinimal,  // Restore only active tab per window.
};

// Statistics about the most recent session restore operation.
//
// These stats are accumulated during a restore batch (between
// OnSessionRestoreComplete start and end) and reset at the beginning
// of each batch.
//
// Note: These are Astra's stats — they count only what the helper
// has observed through its patch points.  Chromium's SessionService
// maintains its own more complete statistics.
struct AstraSessionRestoreStats {
  // Number of tabs restored in the current/last batch.
  size_t tabs_restored = 0;

  // Number of windows restored in the current/last batch.
  size_t windows_restored = 0;

  // Number of tabs that failed to restore.
  size_t tabs_failed = 0;

  // Number of tabs that had Astra metadata applied.
  size_t tabs_with_metadata = 0;

  // Number of windows that had Astra metadata applied.
  size_t windows_with_metadata = 0;

  // Number of unique workspaces represented in restored tabs/windows.
  size_t workspaces_restored = 0;

  // Number of split view pairs restored.
  size_t split_view_pairs_restored = 0;

  // Number of favorite tabs restored.
  size_t favorite_tabs_restored = 0;

  // Number of tab stacks restored.
  size_t tab_stacks_restored = 0;

  // Number of PiP tabs restored.
  size_t pip_tabs_restored = 0;

  // Whether a restore batch is currently in progress.
  bool restore_in_progress = false;

  // When the restore started.
  base::Time restore_start_time;

  // When the restore completed.
  base::Time restore_end_time;

  // Resets all counts to zero and clears in_progress flag and timestamps.
  void Reset() {
    tabs_restored = 0;
    windows_restored = 0;
    tabs_failed = 0;
    tabs_with_metadata = 0;
    windows_with_metadata = 0;
    workspaces_restored = 0;
    split_view_pairs_restored = 0;
    favorite_tabs_restored = 0;
    tab_stacks_restored = 0;
    pip_tabs_restored = 0;
    restore_in_progress = false;
    restore_start_time = base::Time();
    restore_end_time = base::Time();
  }

  // Returns true if no restore has happened yet (all counts zero).
  bool IsEmpty() const {
    return tabs_restored == 0 && windows_restored == 0;
  }

  // Returns the duration of the last restore.
  // Returns zero duration if restore hasn't completed yet.
  base::TimeDelta GetRestoreDuration() const {
    if (restore_start_time.is_null() || restore_end_time.is_null()) {
      return base::TimeDelta();
    }
    return restore_end_time - restore_start_time;
  }

  // Returns the success rate as a percentage (0.0 - 100.0).
  // Returns 100.0 if no tabs were attempted (vacuous truth).
  double GetRestoreSuccessRate() const {
    size_t total = tabs_restored + tabs_failed;
    if (total == 0) {
      return 100.0;
    }
    return (static_cast<double>(tabs_restored) / total) * 100.0;
  }

  // Returns true if a restore is currently in progress.
  bool IsRestoreInProgress() const {
    return restore_in_progress;
  }
};

// Information about the last saved session.
//
// This is cached in PrefService so that startup UI can show session
// information without needing to load the full session from disk.
//
// Note: This is Astra's mirror of session info.  The canonical source
// is Chromium's SessionService.
struct AstraLastSessionInfo {
  // When the last session was saved.  Null time means no session saved.
  base::Time last_save_time;

  // Number of tabs in the last saved session.
  size_t tab_count = 0;

  // Number of windows in the last saved session.
  size_t window_count = 0;

  // Number of workspaces across all tabs/windows.
  size_t workspace_count = 0;

  // Returns true if there is a previously saved session.
  bool has_session() const { return !last_save_time.is_null() && tab_count > 0; }

  // Resets all fields to default/empty values.
  void Reset() {
    last_save_time = base::Time();
    tab_count = 0;
    window_count = 0;
    workspace_count = 0;
  }
};

// Thin bridge between Chromium's session restore pipeline and Astra's
// per-tab metadata.  All methods are static — this is not an instantiable
// service, just a collection of hook functions that Chromium patch points
// call into.
//
// Chromium owns:
//   - SessionService: persistence of session data to disk.
//   - SessionRestore: orchestration of tab/window restoration on startup.
//   - TabRestoreService: "recently closed" tab restore (Ctrl+Shift+T).
//   - SerializedNavigationEntry / SessionTab: data carriers for tab state.
//
// Astra owns:
//   - Extra per-tab metadata that rides alongside Chromium's session data.
//   - The serialization format (base::Value::Dict with "astra.*" keys).
//   - Applying metadata to WebContents via AstraTabFeatures.
//
// This helper intentionally has minimal state — it tracks restore stats
// and reads/writes settings through PrefService, but all session logic
// is owned by Chromium.
//
// TODO(astra): Wire these helper methods into Chromium's session save/restore
// pipeline.  See chromium/astra/patches/0006-session-restore-metadata.md for
// exact patch points and function names.
// Chromium component: sessions / SessionService / SessionRestore /
// TabRestoreService.
class AstraSessionRestoreHelper {
 public:
  // -- Observer management ---------------------------------------------------

  static void AddObserver(AstraSessionRestoreObserver* observer);
  static void RemoveObserver(AstraSessionRestoreObserver* observer);

  // -- Tab-level save/restore ------------------------------------------------

  // Called by Chromium's session service when a tab is about to be restored
  // (either from a saved session on startup or via "reopen closed tab").
  //
  // |extra_data| is the Astra metadata dict that was attached to the tab's
  // session entry during save.  The helper applies it to |web_contents| via
  // AstraTabFeatures.
  //
  // |profile| is the profile associated with the tab; may be used in the
  // future for workspace validation (e.g., ensuring the workspace_id in the
  // metadata still exists in the profile's workspace list).
  //
  // Returns true if Astra metadata was found and successfully applied.
  // Returns false if the metadata was empty, invalid, or could not be
  // applied (e.g. null web_contents).
  //
  // Patch point: chrome/browser/sessions/session_restore.cc — where
  // WebContents are created during session restore.
  // Also: chrome/browser/sessions/tab_restore_service.cc — where closed
  // tabs are restored.
  static bool OnWillRestoreTab(Profile* profile,
                               content::WebContents* web_contents,
                               const base::Value::Dict& extra_data);

  // Called by Chromium's session service when saving a tab's session state.
  // Extracts Astra metadata from |web_contents| and returns it as a dict.
  // Returns an empty dict if the tab has no Astra metadata.
  //
  // Patch point: chrome/browser/sessions/base_session_service.cc or
  // sessions::SerializedNavigationEntry / sessions::SessionTab serialization.
  static base::Value::Dict OnWillSaveTab(content::WebContents* web_contents);

  // Called by Chromium after a tab has been fully restored (navigation
  // committed, WebContents fully initialized).  Astra can use this to trigger
  // post-restore work such as:
  //   - Re-linking split view pairs (since both partners need to exist).
  //   - Notifying UI observers about restored workspace membership.
  //
  // The default implementation is a no-op for logic; split view pairing and
  // other multi-tab coordination is done lazily when the UI first projects
  // tabs.  However, this method does notify all registered observers.
  //
  // Patch point: chrome/browser/sessions/session_restore.cc — after the
  // restore completes and TabStripModel has been populated.
  static void OnTabRestored(content::WebContents* web_contents);

  // Notifies observers that a tab at the given index has been restored.
  // Used for progress tracking during bulk restore operations.
  static void OnTabRestoredAtIndex(int tab_index);

  // -- Window-level metadata ------------------------------------------------

  // Called when saving window session data.  Returns Astra metadata that
  // should be attached to the window's session entry.
  //
  // Window-level Astra data includes:
  //   - Workspace ID (which workspace this window belongs to).
  //   - Saved window bounds (for multi-monitor restore).
  //   - Minimized/maximized state.
  //   - Sidebar state (visibility, width, pinned).
  //
  // Chromium component: sessions::SessionWindow / BaseSessionService.
  // Patch point: chrome/browser/sessions/base_session_service.cc — where
  // window data is serialized.
  static base::Value::Dict OnWillSaveWindow(Profile* profile,
                                             Browser* browser);

  // Called when restoring window session data.  Applies Astra window
  // metadata from |extra_data| to |browser|.
  //
  // On restore, each Browser window is assigned to its workspace based on
  // the workspace_id in the session data.  Windows without a workspace_id
  // default to the "default" workspace.
  //
  // Returns true if Astra metadata was found and successfully applied.
  // Returns false if the metadata was empty, invalid, or could not be
  // applied.
  //
  // Chromium component: sessions::SessionRestore — where Browser windows are
  // created during session restore.
  // Patch point: chrome/browser/sessions/session_restore.cc — where a new
  // Browser is created from a SessionWindow.
  static bool OnWillRestoreWindow(Profile* profile,
                                  Browser* browser,
                                  const base::Value::Dict& extra_data);

  // Called by Chromium after a window has been fully restored (all its tabs
  // have been created).  Notifies observers that window restore is complete.
  //
  // Patch point: chrome/browser/sessions/session_restore.cc — after a
  // Browser window and all its tabs have been created.
  static void OnWindowRestored(Browser* browser);

  // -- Batch session restore notification ------------------------------------

  // Called when a full session restore batch is complete (all windows and
  // tabs have been restored).  Notifies observers.
  //
  // Patch point: chrome/browser/sessions/session_restore.cc — at the end
  // of SessionRestore::RestoreSession().
  static void OnSessionRestoreComplete(Profile* profile);

  // -- Restore stats ---------------------------------------------------------

  // Returns statistics about the most recent session restore.
  // Stats are accumulated during a restore batch and reset at the start
  // of each batch (when OnSessionRestoreComplete first fires, or via
  // ResetRestoreStats).
  //
  // Note: These are Astra's stats, counting only what the helper has
  // observed through its patch points.  For canonical stats, query
  // Chromium's SessionService directly.
  static const AstraSessionRestoreStats& GetRestoreStats();

  // Returns stats from the last completed restore.
  // Alias for GetRestoreStats() for API clarity.
  static const AstraSessionRestoreStats& GetLastRestoreStats();

  // Resets all restore statistics to zero.
  // Called automatically at the start of each restore batch.
  static void ResetRestoreStats();

  // Returns the duration of the last restore operation.
  // Returns zero duration if no restore has completed.
  static base::TimeDelta GetRestoreDuration();

  // Returns the success rate of the last restore (0.0 - 100.0).
  static double GetRestoreSuccessRate();

  // Returns whether a restore operation is currently in progress.
  static bool IsRestoreInProgress();

  // -- Restore load mode -----------------------------------------------------

  // Returns the current restore load mode.
  // Controls how aggressively tabs are loaded during restore.
  //
  // Default: kSmart.
  static SessionRestoreLoadMode GetRestoreLoadMode(Profile* profile);

  // Sets the restore load mode.
  static void SetRestoreLoadMode(Profile* profile,
                                 SessionRestoreLoadMode mode);

  // Converts a load mode enum to its string representation.
  static std::string RestoreLoadModeToString(SessionRestoreLoadMode mode);

  // Parses a string into a load mode enum.
  // Returns kSmart (the default) if the string is unrecognized.
  static SessionRestoreLoadMode RestoreLoadModeFromString(
      const std::string& mode_str);

  // -- Session restore mode --------------------------------------------------

  // Returns the current session restore mode for the given profile.
  // The mode controls how much of the previous session is restored on startup.
  //
  // Default: kRestoreAll.
  static SessionRestoreMode GetRestoreMode(Profile* profile);

  // Sets the session restore mode for the given profile.
  // Persists the setting via PrefService.
  static void SetRestoreMode(Profile* profile, SessionRestoreMode mode);

  // Returns true if session restore is enabled (Astra metadata participates).
  // When false, the helper returns empty metadata on save and does nothing
  // on restore — but Chromium's session restore still runs normally.
  static bool IsRestoreEnabled(Profile* profile);

  // Enables or disables Astra metadata participation in session restore.
  static void SetRestoreEnabled(Profile* profile, bool enabled);

  // Returns whether lazy loading is enabled for session restore.
  // When true (default), tabs are loaded on demand when first activated.
  static bool IsLazyLoadingEnabled(Profile* profile);

  // Returns whether the restore session prompt is shown on startup.
  static bool ShouldShowRestorePrompt(Profile* profile);

  // -- Last session info -----------------------------------------------------

  // Returns information about the last saved session.
  //
  // The data is read from PrefService, where it was recorded by
  // RecordSessionSaved().  This is a cached copy — for canonical info,
  // query Chromium's SessionService directly.
  //
  // Returns a struct with last_save_time, tab_count, window_count,
  // and workspace_count.
  static AstraLastSessionInfo GetLastSessionInfo(Profile* profile);

  // Records that a session was saved with the given counts.
  // Called by Chromium's SessionService patch point after persisting
  // a session snapshot to disk.
  //
  // Updates PrefService with the save timestamp and counts.
  // Notifies observers via OnSessionSaved().
  //
  // Patch point: chrome/browser/sessions/base_session_service.cc —
  // after SaveSession() or ScheduleCommand() persists the session.
  static void RecordSessionSaved(Profile* profile,
                                 size_t tab_count,
                                 size_t window_count,
                                 size_t workspace_count = 0);

  // -- Presentation settings --------------------------------------------------
  //
  // These settings control how session restore is presented and configured.
  // All settings persist through PrefService.

  // Whether to restore the session on startup.
  // Default: true.
  static bool GetRestoreOnStartup(Profile* profile);
  static void SetRestoreOnStartup(Profile* profile, bool enabled);

  // Maximum number of tabs to restore per workspace.
  // 0 means no limit.
  // Default: 0 (no limit).
  static int GetMaxTabsPerWorkspaceRestore(Profile* profile);
  static void SetMaxTabsPerWorkspaceRestore(Profile* profile, int max_tabs);

  // Whether to show a confirmation prompt before restoring.
  // Default: false.
  static bool GetShowRestorePrompt(Profile* profile);
  static void SetShowRestorePrompt(Profile* profile, bool show);

  // Whether to restore workspaces one at a time (instead of all at once).
  // Default: false.
  static bool GetRestoreWorkspacesIndividually(Profile* profile);
  static void SetRestoreWorkspacesIndividually(Profile* profile, bool enabled);

  // Whether to load background tabs lazily.
  // Default: true.
  static bool GetBackgroundTabLoading(Profile* profile);
  static void SetBackgroundTabLoading(Profile* profile, bool enabled);

  // Auto-save interval in minutes.
  // 0 means auto-save is disabled.
  // Default: 5 minutes.
  static int GetAutoSaveSessionInterval(Profile* profile);
  static void SetAutoSaveSessionInterval(Profile* profile, int minutes);

  // Maximum number of saved sessions to keep.
  // Default: 10.
  static int GetMaxSavedSessions(Profile* profile);
  static void SetMaxSavedSessions(Profile* profile, int max_sessions);

  // -- Named session management -----------------------------------------------
  //
  // Named sessions are saved sessions stored in PrefService.
  // Each session has a name and contains Astra-level metadata.
  // Note: Actual tab data is owned by Chromium's SessionService;
  // these are Astra's metadata snapshots.

  // Saves the current session with a given name.
  // Stores session metadata in PrefService.
  // Notifies observers via OnSessionSaved(name).
  static void SaveSession(Profile* profile, const std::string& session_name);

  // Loads a named session's metadata.
  // Returns the session metadata, or an empty struct if not found.
  static AstraSessionMetadata LoadSession(Profile* profile,
                                          const std::string& session_name);

  // Deletes a named session.
  // Returns true if the session existed and was deleted.
  static bool DeleteSavedSession(Profile* profile,
                                  const std::string& session_name);

  // Returns a list of all saved session names.
  static std::vector<std::string> ListSavedSessions(Profile* profile);

  // Returns whether there is a last (most recent) session available.
  static bool HasLastSession(Profile* profile);

  // Gets the most recently saved session metadata.
  // Returns empty metadata if no session has been saved.
  static AstraSessionMetadata GetLastSession(Profile* profile);

  // Clears the last session data.
  static void ClearLastSession(Profile* profile);

  // -- Bulk operations -------------------------------------------------------

  // Extracts Astra metadata from a list of WebContents and returns a
  // parallel list of metadata dicts.
  //
  // The returned vector has the same size as |web_contents_list|.
  // Entries for null WebContents or tabs with no Astra metadata are
  // empty dicts.
  //
  // This is a convenience for batch operations (e.g., saving all tabs
  // in a window at once).
  static std::vector<base::Value::Dict> BulkExtractMetadataFromTabs(
      const std::vector<content::WebContents*>& web_contents_list);

  // Applies a list of metadata dicts to a parallel list of WebContents.
  //
  // Both vectors must have the same size.  Each metadata dict is applied
  // to the corresponding WebContents.
  //
  // Returns the number of tabs that successfully had metadata applied
  // (i.e., metadata was non-empty and valid).
  //
  // Skips null WebContents and empty/invalid metadata entries.
  static size_t BulkApplyMetadataToTabs(
      const std::vector<base::Value::Dict>& metadata_list,
      const std::vector<content::WebContents*>& web_contents_list,
      Profile* profile);

  // Restores all workspaces from the last saved session.
  // Notifies observers as workspaces are restored.
  //
  // Note: Actual tab/window creation is handled by Chromium's SessionRestore;
  // this method handles Astra metadata and state management.
  static void RestoreAllWorkspaces(Profile* profile);

  // Restores a single workspace by ID.
  // Returns true if the workspace was found and restore initiated.
  static bool RestoreWorkspace(Profile* profile,
                                const std::string& workspace_id);

  // Closes all tabs in a given workspace.
  // Returns the number of tabs closed.
  //
  // Note: Actual tab closing is handled by Chromium's TabStripModel;
  // this method handles Astra metadata cleanup and notifications.
  static size_t CloseAllTabsInWorkspace(Profile* profile,
                                         const std::string& workspace_id);

  // -- Session restore lifecycle ----------------------------------------------

  // Marks the start of a session restore operation.
  // Initializes stats and notifies observers.
  static void OnSessionRestoreStarted();

  // Marks a session restore operation as failed.
  // Records error and notifies observers.
  static void OnSessionRestoreFailed(const std::string& error);

  // -- Session mode helpers --------------------------------------------------

  // Converts a SessionRestoreMode enum value to its string representation.
  static std::string RestoreModeToString(SessionRestoreMode mode);

  // Parses a string into a SessionRestoreMode enum value.
  // Returns kRestoreAll (the default) if the string is unrecognized.
  static SessionRestoreMode RestoreModeFromString(const std::string& mode_str);

  AstraSessionRestoreHelper() = delete;
  ~AstraSessionRestoreHelper() = delete;
  AstraSessionRestoreHelper(const AstraSessionRestoreHelper&) = delete;
  AstraSessionRestoreHelper& operator=(const AstraSessionRestoreHelper&) =
      delete;

 private:
  // Static observer list for session restore events.
  //
  // Uses base::ObserverList with CheckedObserver to ensure observers are
  // properly removed before destruction.
  //
  // TODO(astra): Consider per-profile observer lists for multi-profile
  // scenarios.  For now, a global list is sufficient.
  static base::ObserverList<AstraSessionRestoreObserver>& GetObservers();

  // Static restore stats storage.
  //
  // Accumulates counters during a restore batch.  Reset at the start of
  // each batch and accessible via GetRestoreStats().
  //
  // TODO(astra): Consider per-profile stats when adding multi-profile
  // observer support.  For now, a global stats struct is sufficient.
  static AstraSessionRestoreStats& GetMutableRestoreStats();

  // Helper: gets the PrefService from a Profile, or returns nullptr.
  static PrefService* GetPrefs(Profile* profile);

  // Helper: increments tab-related restore stats from metadata.
  // Called after successfully restoring a tab with Astra metadata.
  static void RecordTabRestoreStats(const base::Value::Dict& metadata);

  // Helper: increments window-related restore stats from metadata.
  // Called after successfully restoring a window with Astra metadata.
  static void RecordWindowRestoreStats(const base::Value::Dict& metadata);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SESSION_RESTORE_HELPER_H_
