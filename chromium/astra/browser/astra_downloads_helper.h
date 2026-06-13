#ifndef ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_H_
#define ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace download {
class DownloadItem;
class DownloadManager;
}  // namespace download

namespace astra {

// =========================================================================
// AstraDownloadState — download state enumeration
// =========================================================================
//
// Projected download state values, mirroring the states of Chromium's
// download::DownloadItem.  These are used by Astra UI surfaces to display
// download status without depending on the full download subsystem.
//
// Chromium owner: download::DownloadItem::DownloadState
//   (components/download/public/common/download_item.h)
enum class AstraDownloadState {
  kInProgress,   // Download is actively downloading
  kCompleted,    // Download finished successfully
  kCancelled,    // Download was cancelled by the user
  kFailed,       // Download failed due to an error
  kInterrupted,  // Download was interrupted (network issue, etc.)
};

// =========================================================================
// AstraDownloadItem — projected download item data
// =========================================================================
//
// A lightweight struct representing a single download entry, projected from
// Chromium's download::DownloadItem.  This struct is a pure projection —
// it never mutates download state.
//
// Chromium owner: download::DownloadItem
//   (components/download/public/common/download_item.h)
//
// TODO(astra): Use the full download::DownloadItem type from Chromium
// instead of this projection struct, once the overlay is building against
// a full Chromium checkout.  This struct mirrors the fields we need for
// sidebar presentation.
struct AstraDownloadItem {
  // Stable identifier for the download item.
  // Corresponds to DownloadItem::GetId().
  int id = 0;

  // Source URL of the download.
  GURL url;

  // Destination file name (base name, not full path).
  std::string file_name;

  // Full file path of the downloaded file on disk.
  base::FilePath file_path;

  // Total size of the download in bytes.
  // -1 means the total size is unknown.
  int64_t total_bytes = -1;

  // Number of bytes received so far.
  int64_t received_bytes = 0;

  // Current state of the download.
  AstraDownloadState state = AstraDownloadState::kInProgress;

  // When the download started.
  base::Time start_time;

  // When the download ended (completed, failed, or cancelled).
  // Null if still in progress.
  base::Time end_time;

  // Whether the download has been flagged as dangerous by safe browsing.
  bool is_dangerous = false;

  // MIME type of the downloaded file, if known.
  std::string mime_type;

  // Whether the download is currently paused.
  bool is_paused = false;
};

// =========================================================================
// AstraDownloadsObserver — observer interface
// =========================================================================
//
// Observer interface for AstraDownloadsHelper.  Notifies when download
// state changes (start, progress, complete, fail, remove) or when
// download presentation settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh the downloads sidebar
// section when download state changes.  The browser layer never depends
// on Views code.
class AstraDownloadsObserver : public base::CheckedObserver {
 public:
  // Called when a new download starts.
  // |download_id| is the ID of the newly started download.
  virtual void OnDownloadStarted(int download_id) {}

  // Called when a download's progress or state is updated.
  // |download_id| is the ID of the updated download.
  virtual void OnDownloadUpdated(int download_id) {}

  // Called when a download completes successfully.
  // |download_id| is the ID of the completed download.
  virtual void OnDownloadCompleted(int download_id) {}

  // Called when a download fails.
  // |download_id| is the ID of the failed download.
  // |error| is a human-readable error description.
  virtual void OnDownloadFailed(int download_id, const std::string& error) {}

  // Called when a download is removed from the history.
  // |download_id| is the ID of the removed download.
  virtual void OnDownloadRemoved(int download_id) {}

  // Called when all downloads are cleared.
  virtual void OnAllDownloadsCleared() {}

  // Called when downloads presentation settings change.
  // The UI should refresh its downloads section presentation.
  virtual void OnDownloadsSettingsChanged() {}

 protected:
  ~AstraDownloadsObserver() override = default;
};

// =========================================================================
// AstraDownloadsHelper — downloads projection helper
// =========================================================================
//
// Helper class for download-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// DownloadManager API.  It provides a clean interface for the Astra UI
// layer to query download state and perform download operations without
// directly depending on the full download subsystem.
//
// Truth source: Chromium's DownloadManager
//   (content/public/browser/download_manager.h)
// and download::DownloadItem
//   (components/download/public/common/download_item.h)
//
// Projection principles:
//   - Downloads are never stored in Astra-owned memory long-term.
//   - All download access goes through Chromium's DownloadManager.
//   - Astra only projects metadata (count, state, progress, speed).
//
// Astra-specific presentation preferences (sidebar visibility, sort
// order, display mode, etc.) are persisted via the profile's PrefService.
// These are purely presentation concerns and never affect the underlying
// download data managed by Chromium.
//
// TODO(astra): Proper DownloadManager::Observer integration.
//   Currently this helper does not observe the download manager for live
//   updates.  To get reactive updates, we need to implement
//   download::DownloadManager::Observer or use the AllDownloadItemNotifier.
// Chromium owner: DownloadManager
//   (content/public/browser/download_manager.h)
// Chromium observer: DownloadManager::Observer
//   (content/public/browser/download_manager.h)
// Chromium patch point: None needed — DownloadManager::Observer is a public
//   interface that any KeyedService can use.
class AstraDownloadsHelper : public KeyedService {
 public:
  explicit AstraDownloadsHelper(Profile* profile);
  AstraDownloadsHelper(const AstraDownloadsHelper&) = delete;
  AstraDownloadsHelper& operator=(const AstraDownloadsHelper&) = delete;
  ~AstraDownloadsHelper() override;

  // -- Download list queries ----------------------------------------------

  // Returns the total number of downloads (all states).
  //
  // TODO(astra): Query DownloadManager for the actual count.
  // Chromium method: DownloadManager::InProgressCount() + GetAllDownloads().size()
  size_t GetDownloadCount() const;

  // Returns the number of active (in-progress) downloads.
  size_t GetActiveDownloadCount() const;

  // Returns the number of completed downloads.
  size_t GetCompletedDownloadCount() const;

  // Returns a specific download by its ID.
  // Returns an empty (zero-initialized) AstraDownloadItem if not found.
  //
  // TODO(astra): Use DownloadManager::GetDownload() to retrieve the item.
  AstraDownloadItem GetDownload(int id) const;

  // Returns all downloads as a list of projected items.
  // Downloads are returned in most-recent-first order.
  //
  // TODO(astra): Query DownloadManager::GetAllDownloads() and project each item.
  std::vector<AstraDownloadItem> GetAllDownloads() const;

  // Returns only active (in-progress) downloads.
  std::vector<AstraDownloadItem> GetActiveDownloads() const;

  // Returns recently completed downloads, up to |max_count|.
  // If |max_count| is 0, uses the configured max_recent_downloads pref.
  std::vector<AstraDownloadItem> GetRecentDownloads(int max_count = 0) const;

  // -- Progress and speed -------------------------------------------------

  // Returns the download progress as a value from 0.0 to 1.0.
  // Returns 0.0 if the download is not found or total size is unknown.
  double GetDownloadProgress(int id) const;

  // Returns the current download speed in bytes per second.
  // Returns 0 if the download is not found or is not active.
  //
  // TODO(astra): Use DownloadItem::CurrentSpeed() for real speed.
  int64_t GetDownloadSpeed(int id) const;

  // Returns true if there are any active downloads.
  bool IsDownloading() const;

  // -- Download operations ------------------------------------------------

  // Pauses a download.
  // No-op if the download is not found or is already paused.
  //
  // TODO(astra): Call DownloadItem::Pause().
  // Chromium method: download::DownloadItem::Pause()
  void PauseDownload(int id);

  // Resumes a paused download.
  // No-op if the download is not found or is not paused.
  //
  // TODO(astra): Call DownloadItem::Resume().
  // Chromium method: download::DownloadItem::Resume()
  void ResumeDownload(int id);

  // Cancels a download.
  // No-op if the download is not found or is already complete/cancelled.
  //
  // TODO(astra): Call DownloadItem::Cancel().
  // Chromium method: download::DownloadItem::Cancel()
  void CancelDownload(int id);

  // Removes a download from the history.
  // No-op if the download is not found.
  //
  // TODO(astra): Call DownloadItem::Remove().
  // Chromium method: download::DownloadItem::Remove()
  void RemoveDownload(int id);

  // Removes all completed downloads from the history.
  // Active downloads are not affected.
  //
  // TODO(astra): Iterate completed downloads and call Remove() on each.
  void ClearCompletedDownloads();

  // Opens the downloaded file.
  // No-op if the download is not found or is not complete.
  //
  // TODO(astra): Call DownloadItem::OpenDownload().
  // Chromium method: download::DownloadItem::OpenDownload()
  void OpenDownload(int id);

  // Shows the downloaded file in the file manager / Finder.
  // No-op if the download is not found or the file doesn't exist.
  //
  // TODO(astra): Call DownloadItem::ShowDownloadInShell().
  // Chromium method: download::DownloadItem::ShowDownloadInShell()
  void ShowDownloadInFolder(int id);

  // -- Bulk operations ----------------------------------------------------

  // Pauses all active downloads.
  void PauseAllDownloads();

  // Resumes all paused downloads.
  void ResumeAllDownloads();

  // Cancels all active downloads.
  void CancelAllDownloads();

  // Clears all download history (all states).
  void ClearAllDownloads();

  // -- Presentation settings ----------------------------------------------

  // Returns whether downloads are shown in the sidebar.
  //
  // Persisted via PrefService.  Default: kDefaultShowDownloadsInSidebar.
  bool GetShowDownloadsInSidebar() const;

  // Sets whether downloads are shown in the sidebar.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetShowDownloadsInSidebar(bool show);

  // Toggles whether downloads are shown in the sidebar.
  // Returns the new state.
  bool ToggleShowDownloadsInSidebar();

  // Returns whether download notifications are shown.
  //
  // Persisted via PrefService.  Default: kDefaultShowDownloadNotifications.
  bool GetShowDownloadNotifications() const;

  // Sets whether download notifications are shown.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetShowDownloadNotifications(bool show);

  // Toggles download notifications.
  // Returns the new state.
  bool ToggleShowDownloadNotifications();

  // Returns whether downloads are auto-opened when complete.
  //
  // Persisted via PrefService.  Default: kDefaultAutoOpenDownloads.
  bool GetAutoOpenDownloads() const;

  // Sets whether downloads auto-open on completion.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetAutoOpenDownloads(bool auto_open);

  // Toggles auto-open downloads.
  // Returns the new state.
  bool ToggleAutoOpenDownloads();

  // Returns the downloads sort order.
  // Values: "newest_first" or "oldest_first".
  //
  // Persisted via PrefService.  Default: kDefaultDownloadsSortOrder.
  std::string GetDownloadsSortOrder() const;

  // Sets the downloads sort order.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetDownloadsSortOrder(const std::string& sort_order);

  // Returns the maximum number of recent downloads to show.
  //
  // Persisted via PrefService.  Default: kDefaultMaxRecentDownloads.
  // The value is clamped to [kMinRecentDownloads, kMaxRecentDownloadsLimit].
  int GetMaxRecentDownloads() const;

  // Sets the maximum number of recent downloads.
  // Values are clamped to the valid range.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetMaxRecentDownloads(int max_count);

  // Returns whether download speed is shown in the UI.
  //
  // Persisted via PrefService.  Default: kDefaultShowDownloadSpeed.
  bool GetShowDownloadSpeed() const;

  // Sets whether download speed is shown.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetShowDownloadSpeed(bool show);

  // Toggles show download speed.
  // Returns the new state.
  bool ToggleShowDownloadSpeed();

  // Returns whether file size is shown in the UI.
  //
  // Persisted via PrefService.  Default: kDefaultShowFileSize.
  bool GetShowFileSize() const;

  // Sets whether file size is shown.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetShowFileSize(bool show);

  // Toggles show file size.
  // Returns the new state.
  bool ToggleShowFileSize();

  // Returns whether download progress bar is shown.
  //
  // Persisted via PrefService.  Default: kDefaultShowDownloadProgress.
  bool GetShowDownloadProgress() const;

  // Sets whether download progress bar is shown.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetShowDownloadProgress(bool show);

  // Toggles show download progress.
  // Returns the new state.
  bool ToggleShowDownloadProgress();

  // Returns the downloads display mode.
  // Values: "list" or "compact".
  //
  // Persisted via PrefService.  Default: kDefaultDownloadsDisplayMode.
  std::string GetDownloadsDisplayMode() const;

  // Sets the downloads display mode.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetDownloadsDisplayMode(const std::string& mode);

  // Returns whether the user is prompted for download location.
  //
  // Persisted via PrefService.  Default: kDefaultPromptForDownloadLocation.
  bool GetPromptForDownloadLocation() const;

  // Sets whether to prompt for download location.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetPromptForDownloadLocation(bool prompt);

  // Toggles prompt for download location.
  // Returns the new state.
  bool TogglePromptForDownloadLocation();

  // Returns whether safe browsing warnings are shown for dangerous downloads.
  //
  // Persisted via PrefService.  Default: kDefaultSafeBrowsingWarnings.
  bool GetSafeBrowsingWarnings() const;

  // Sets whether safe browsing warnings are shown.
  // Fires OnDownloadsSettingsChanged observer notification.
  void SetSafeBrowsingWarnings(bool show);

  // Toggles safe browsing warnings.
  // Returns the new state.
  bool ToggleSafeBrowsingWarnings();

  // -- Utility methods ----------------------------------------------------

  // Formats a byte count into a human-readable string (e.g. "1.5 MB").
  static std::string FormatFileSize(int64_t bytes);

  // Formats a speed value (bytes per second) into a human-readable string
  // (e.g. "2.3 MB/s").
  static std::string FormatDownloadSpeed(int64_t bytes_per_second);

  // Formats a time duration into a human-readable ETA string
  // (e.g. "2m 30s", "1h 5m", or "Calculating..." for zero duration).
  static std::string FormatTimeRemaining(base::TimeDelta remaining);

  // Calculates progress as a value from 0.0 to 1.0.
  // Returns 0.0 if total is <= 0 or total < received.
  static double CalculateProgress(int64_t received, int64_t total);

  // -- Constants ----------------------------------------------------------

  // Default: show downloads in sidebar.
  static constexpr bool kDefaultShowDownloadsInSidebar = true;

  // Default: show download notifications.
  static constexpr bool kDefaultShowDownloadNotifications = true;

  // Default: do not auto-open downloads.
  static constexpr bool kDefaultAutoOpenDownloads = false;

  // Default sort order: newest first.
  static constexpr char kDefaultDownloadsSortOrder[] = "newest_first";

  // Default max recent downloads to show.
  static constexpr int kDefaultMaxRecentDownloads = 20;

  // Minimum value for max recent downloads.
  static constexpr int kMinRecentDownloads = 5;

  // Maximum value for max recent downloads.
  static constexpr int kMaxRecentDownloadsLimit = 100;

  // Default: show download speed.
  static constexpr bool kDefaultShowDownloadSpeed = true;

  // Default: show file size.
  static constexpr bool kDefaultShowFileSize = true;

  // Default: show download progress bar.
  static constexpr bool kDefaultShowDownloadProgress = true;

  // Default display mode: list.
  static constexpr char kDefaultDownloadsDisplayMode[] = "list";

  // Default: prompt for download location.
  static constexpr bool kDefaultPromptForDownloadLocation = true;

  // Default: show safe browsing warnings.
  static constexpr bool kDefaultSafeBrowsingWarnings = true;

  // -- Observers ----------------------------------------------------------

  void AddObserver(AstraDownloadsObserver* observer);
  void RemoveObserver(AstraDownloadsObserver* observer);

  // -- Notification helpers (public for testing) -------------------------

  void NotifyDownloadStarted(int download_id);
  void NotifyDownloadUpdated(int download_id);
  void NotifyDownloadCompleted(int download_id);
  void NotifyDownloadFailed(int download_id, const std::string& error);
  void NotifyDownloadRemoved(int download_id);
  void NotifyAllDownloadsCleared();
  void NotifyDownloadsSettingsChanged();

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the DownloadManager for the associated profile.
  // Returns nullptr if the manager is not available.
  //
  // TODO(astra): Use DownloadManager::GetForBrowserContext() when building
  // against the full Chromium source tree.  In the overlay, we return
  // nullptr as a placeholder.
  //
  // Chromium owner: content::DownloadManager
  //   (content/public/browser/download_manager.h)
  // Chromium factory: DownloadManagerFactory (or via BrowserContext)
  //
  // Patch point: None needed — standard DownloadManager access.
  download::DownloadManager* GetDownloadManager() const;

  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // Helper to clamp max recent downloads to valid range.
  static int ClampMaxRecentDownloads(int value);

  // The profile this helper is associated with.  Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for download state changes.
  base::ObserverList<AstraDownloadsObserver> observers_;

  // Tracks whether we're currently observing the download manager.
  // TODO(astra): Flip this to true once DownloadManager::Observer
  // integration is implemented.
  bool is_observing_manager_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_DOWNLOADS_HELPER_H_
