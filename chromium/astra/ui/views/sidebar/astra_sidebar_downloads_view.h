#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "content/public/browser/download_manager.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_download_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"

namespace astra {

// =========================================================================
// AstraDownloadItemInfo — presentation data for a download item
// =========================================================================
//
// A lightweight data struct representing a download projected from
// Chromium's DownloadManager.  This is presentation-facing — it carries
// only the fields needed for display and interaction.
//
// Truth source: download::DownloadItem (components/download/public/common/)
//
// TODO(astra): Wire to DownloadManager for real download data.
//   Chromium owner: content::DownloadManager (content/public/browser/)
struct AstraDownloadItemInfo {
  // Unique identifier for the download (GUID or numeric ID as string).
  std::string id;

  // Display filename.
  std::u16string filename;

  // Source URL of the download.
  GURL url;

  // Total bytes expected (0 if unknown).
  int64_t total_bytes = 0;

  // Bytes received so far.
  int64_t received_bytes = 0;

  // Current download state.
  AstraDownloadState state = AstraDownloadState::kInProgress;

  // Current download speed in bytes per second.
  int64_t speed_bytes_per_sec = 0;

  // Estimated time remaining.
  base::TimeDelta time_remaining;

  // Time the download started.
  base::Time start_time;

  // Time the download completed/failed (base::Time() if in progress).
  base::Time end_time;

  // Local file path (after download completes).
  base::FilePath file_path;

  // MIME type of the download.
  std::string mime_type;

  // Whether the download is marked as dangerous.
  bool is_dangerous = false;

  // Type of danger (if dangerous).
  enum class DangerType {
    kNone,
    kDangerousFile,
    kDangerousUrl,
    kDangerousContent,
    kUncommonContent,
    kPotentiallyUnwanted,
  };

  DangerType danger_type = DangerType::kNone;

  // Whether the user has already been prompted about this download.
  bool has_prompted = false;
};

// Sort order for downloads list.
enum class AstraDownloadSortBy {
  kNewestFirst,    // Most recent first (default)
  kOldestFirst,    // Oldest first
  kLargestFirst,   // Largest file size first
  kSmallestFirst,  // Smallest file size first
  kName,           // Alphabetical by filename
};

// =========================================================================
// AstraSidebarDownloadsDelegate — action callbacks for downloads section
// =========================================================================
//
// Delegate interface for user actions originating in the downloads section.
// The downloads section view is pure presentation — it never mutates
// Chromium's DownloadManager directly.
class AstraSidebarDownloadsDelegate {
 public:
  virtual ~AstraSidebarDownloadsDelegate() = default;

  // Called when a download item is clicked (primary action).
  virtual void OnDownloadClicked(const std::string& download_id) = 0;

  // Called when a download item is right-clicked (context menu).
  virtual void OnDownloadRightClicked(const std::string& download_id,
                                      const gfx::Point& point) = 0;

  // Called when the user pauses a download.
  virtual void OnPauseDownload(const std::string& download_id) = 0;

  // Called when the user resumes a download.
  virtual void OnResumeDownload(const std::string& download_id) = 0;

  // Called when the user cancels a download.
  virtual void OnCancelDownload(const std::string& download_id) = 0;

  // Called when the user opens a completed download.
  virtual void OnOpenDownload(const std::string& download_id) = 0;

  // Called when the user wants to show the download in its folder.
  virtual void OnShowDownloadInFolder(const std::string& download_id) = 0;

  // Called when the user retries a failed/cancelled download.
  virtual void OnRetryDownload(const std::string& download_id) = 0;

  // Called when the user removes a download from the list.
  virtual void OnRemoveDownload(const std::string& download_id) = 0;

  // Called when the user requests clearing all downloads.
  virtual void OnClearAllDownloadsRequested() = 0;
};

// =========================================================================
// AstraSidebarDownloadsView — downloads sidebar section
// =========================================================================
//
// Sidebar section that displays active and recent downloads.
// Extends AstraSidebarSectionView for common section chrome and adds
// download-specific presentation logic.
//
// This is a presentation-only view: it projects Chromium download state
// (from content::DownloadManager) into a vertical list. It never stores
// download truth state — the DownloadManager is the single source of truth.
//
// Display order:
//   1. Active downloads (in-progress) — with live progress bars
//   2. Recent completed downloads
//
// Implements content::DownloadManager::Observer to receive live download
// updates from Chromium's download subsystem.
//
// TODO(astra): Consider using AllDownloadsNotifier instead of direct
//   DownloadManager observation. AllDownloadsNotifier handles off-the-record
//   profiles and provides a unified view across profiles.
//   Chromium owner: AllDownloadsNotifier
//     (chrome/browser/download/all_downloads_notifier.h)
class AstraSidebarDownloadsView
    : public AstraSidebarSectionView,
      public content::DownloadManager::Observer,
      public AstraDownloadItemDelegate {
 public:
  // Maximum number of recent (completed) downloads shown.
  static constexpr size_t kMaxRecentItems = 5;

  explicit AstraSidebarDownloadsView(
      content::DownloadManager* download_manager);
  ~AstraSidebarDownloadsView() override;

  AstraSidebarDownloadsView(const AstraSidebarDownloadsView&) = delete;
  AstraSidebarDownloadsView& operator=(
      const AstraSidebarDownloadsView&) = delete;

  // -- Delegate ------------------------------------------------------------

  void set_delegate(AstraSidebarDownloadsDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarDownloadsDelegate* delegate() const { return delegate_; }

  // -- Download data projection -------------------------------------------

  // Set the full list of downloads. Replaces all existing items.
  void SetDownloads(const std::vector<AstraDownloadItemInfo>& downloads);

  // Get the total number of downloads.
  int GetDownloadCount() const;

  // Get download info at the given index.
  AstraDownloadItemInfo GetDownloadAt(int index) const;

  // Add a download item.
  void AddDownload(const AstraDownloadItemInfo& download);

  // Remove the download at the given index.
  void RemoveDownload(int index);

  // Clear all downloads from the display (presentation only).
  void ClearAllDownloads();

  // Clear only completed downloads from the display.
  void ClearCompletedDownloads();

  // Update the download at the given index with new info.
  void UpdateDownload(int index, const AstraDownloadItemInfo& download);

  // -- Selection -----------------------------------------------------------

  // Set the selected download by index. -1 clears selection.
  void SetSelectedDownload(int index);
  int GetSelectedIndex() const { return selected_index_; }
  void ClearSelection();

  // -- Category visibility -------------------------------------------------

  // Set whether to show in-progress downloads.
  void SetShowInProgress(bool show);
  bool GetShowInProgress() const { return show_in_progress_; }

  // Set whether to show completed downloads.
  void SetShowCompleted(bool show);
  bool GetShowCompleted() const { return show_completed_; }

  // Set whether to show cancelled downloads.
  void SetShowCancelled(bool show);
  bool GetShowCancelled() const { return show_cancelled_; }

  // Get counts by category.
  int GetInProgressCount() const;
  int GetCompletedCount() const;
  int GetCancelledCount() const;

  // Get total downloaded size across all completed downloads.
  int64_t GetTotalDownloadedSize() const;

  // -- Download actions (delegate to service) ------------------------------

  // Pause the download at the given index.
  void PauseDownload(int index);

  // Resume the download at the given index.
  void ResumeDownload(int index);

  // Cancel the download at the given index.
  void CancelDownload(int index);

  // Open the downloaded file (for completed downloads).
  void OpenDownload(int index);

  // Show the downloaded file in its folder.
  void ShowDownloadInFolder(int index);

  // Retry a failed/cancelled download.
  void RetryDownload(int index);

  // -- Sorting -------------------------------------------------------------

  // Set the sort order for downloads.
  void SetSortBy(AstraDownloadSortBy sort_by);
  AstraDownloadSortBy GetSortBy() const { return sort_by_; }

  // -- Search --------------------------------------------------------------

  // Filter downloads by filename search query.
  void SearchDownloads(const std::u16string& query);

  // Get the number of downloads matching the current search.
  int GetSearchResultsCount() const;

  // -- Display options -----------------------------------------------------

  // Set whether to always show progress bars (even for completed downloads).
  void SetAlwaysShowProgress(bool show);
  bool GetAlwaysShowProgress() const { return always_show_progress_; }

  // Set whether to show file size in the item view.
  void SetShowFileSize(bool show);
  bool GetShowFileSize() const { return show_file_size_; }

  // Set whether to show download speed in the item view.
  void SetShowSpeed(bool show);
  bool GetShowSpeed() const { return show_speed_; }

  // Set whether to show estimated time remaining.
  void SetShowTimeRemaining(bool show);
  bool GetShowTimeRemaining() const { return show_time_remaining_; }

  // Get overall progress across all in-progress downloads (0.0 to 1.0).
  double GetOverallProgress() const;

  // Get the number of currently active (in-progress) downloads.
  int GetActiveDownloadCount() const;

  // -- Auto-open -----------------------------------------------------------

  // Set whether downloads auto-open when completed.
  void SetAutoOpenDownloads(bool auto_open);
  bool GetAutoOpenDownloads() const { return auto_open_downloads_; }

  // -- Manager integration -------------------------------------------------

  // Refresh the downloads list from the underlying DownloadManager.
  void RefreshFromManager();

  // Set section visibility.
  void SetSectionVisible(bool visible);

  // -- content::DownloadManager::Observer -----------------------------------

  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadUpdated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadRemoved(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

  // -- AstraDownloadItemDelegate -------------------------------------------

  void OnDownloadItemClicked(const std::string& download_id) override;
  void OnDownloadCancelRequested(const std::string& download_id) override;
  void OnDownloadPauseRequested(const std::string& download_id) override;
  void OnDownloadResumeRequested(const std::string& download_id) override;
  void OnDownloadOpenRequested(const std::string& download_id) override;
  void OnDownloadShowInFolderRequested(
      const std::string& download_id) override;

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 protected:
  // AstraSidebarSectionView overrides.
  void OnSearchQueryChanged(const std::u16string& query) override;
  void OnShowMoreClicked() override;
  void OnMoreButtonClicked() override;

 private:
  // Build the child views and layout.
  void BuildDownloadsLayout();

  // Find the item view for a given download ID.
  AstraDownloadItemView* FindItemView(const std::string& download_id) const;

  // Find the index of a download by ID. Returns -1 if not found.
  int FindDownloadIndex(const std::string& download_id) const;

  // Create a download item view from info struct.
  std::unique_ptr<AstraDownloadItemView> CreateItemView(
      const AstraDownloadItemInfo& info);

  // Generate a stable string ID for a download item.
  static std::string GetDownloadId(download::DownloadItem* item);

  // Called when the "Show all downloads" link is clicked.
  void OnShowAllClicked();

  // Sort and rebuild the items list from the current downloads_ vector.
  void RebuildItems();

  // Apply current sort order to downloads_.
  void ApplySortOrder();

  // Apply current filters (category + search) to determine visible items.
  void ApplyFilters();

  // Update the visibility of the "Show all" link.
  void UpdateShowAllVisibility();

  // Static sort comparator.
  static bool CompareDownloads(const AstraDownloadItemInfo& a,
                               const AstraDownloadItemInfo& b,
                               AstraDownloadSortBy sort_by);

  // The Chromium DownloadManager we observe and project.
  raw_ptr<content::DownloadManager> download_manager_ = nullptr;

  // Action delegate.
  raw_ptr<AstraSidebarDownloadsDelegate> delegate_ = nullptr;

  // Cached download data (projection from DownloadManager).
  std::vector<AstraDownloadItemInfo> downloads_;

  // Selection state.
  int selected_index_ = -1;

  // Category visibility.
  bool show_in_progress_ = true;
  bool show_completed_ = true;
  bool show_cancelled_ = false;

  // Display options.
  AstraDownloadSortBy sort_by_ = AstraDownloadSortBy::kNewestFirst;
  bool always_show_progress_ = false;
  bool show_file_size_ = true;
  bool show_speed_ = true;
  bool show_time_remaining_ = true;
  bool auto_open_downloads_ = false;

  // Tracks whether we're currently observing the download manager.
  bool is_observing_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_
