#ifndef ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_MODEL_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_MODEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

#include "astra/browser/astra_downloads_helper.h"

namespace astra {

// =========================================================================
// AstraDownloadsBubbleItem — presentation data for a bubble download item
// =========================================================================
//
// Data for a single download entry as shown in the downloads bubble.
// This is a projection of AstraDownloadItem from the downloads helper,
// with bubble-specific presentation fields added.
struct AstraDownloadsBubbleItem {
  std::string id;
  std::u16string filename;
  GURL url;
  int64_t total_bytes = -1;
  int64_t received_bytes = 0;
  int64_t speed_bytes_per_sec = 0;
  AstraDownloadState state = AstraDownloadState::kInProgress;
  base::Time start_time;
  base::Time end_time;
  base::TimeDelta time_remaining;
  std::string mime_type;
  bool is_dangerous = false;
  std::string danger_type;
  gfx::ImageSkia icon;
};

// Sort order for bubble download items.
enum class AstraDownloadsBubbleSortOrder {
  kNewestFirst,    // Most recent downloads first
  kOldestFirst,    // Oldest downloads first
  kLargestFirst,   // Largest file size first
  kSmallestFirst,  // Smallest file size first
  kName,           // Alphabetical by filename
};

// =========================================================================
// AstraDownloadsBubbleObserver — observer interface
// =========================================================================
//
// Observer for downloads bubble model changes.  All methods have empty
// default implementations so observers only override what they need.
class AstraDownloadsBubbleObserver : public base::CheckedObserver {
 public:
  // Called when the list of downloads shown in the bubble changes.
  virtual void OnDownloadsChanged(AstraDownloadsBubbleModel* model) {}

  // Called when a specific download is updated (progress, state, etc.).
  virtual void OnDownloadUpdated(AstraDownloadsBubbleModel* model,
                                 const std::string& download_id) {}

  // Called when a new download starts.
  virtual void OnDownloadStarted(AstraDownloadsBubbleModel* model,
                                 const std::string& download_id) {}

  // Called when a download completes.
  virtual void OnDownloadCompleted(AstraDownloadsBubbleModel* model,
                                   const std::string& download_id) {}

  // Called when the active download count changes (badge update).
  virtual void OnActiveCountChanged(AstraDownloadsBubbleModel* model,
                                    int active_count) {}

  // Called when the model is shutting down.
  virtual void OnDownloadsBubbleModelShutdown(
      AstraDownloadsBubbleModel* model) {}

 protected:
  ~AstraDownloadsBubbleObserver() override = default;
};

// =========================================================================
// AstraDownloadsBubbleModel — model for the downloads bubble
// =========================================================================
//
// Model that manages download data for the downloads toolbar bubble.
// Observes AstraDownloadsHelper for download changes and projects the
// data into bubble-specific presentation format.
//
// The model is separate from the bubble view so the toolbar button can
// observe the model for badge updates even when the bubble is closed.
//
// Truth source: AstraDownloadsHelper (which projects Chromium's
// DownloadManager).
//
// TODO(astra): Wire to AstraDownloadsHelper observer pattern for
//   real-time updates.  Currently operates on pull/snapshot model.
//   Chromium owner: content::DownloadManager
//   Patch point: None — uses public DownloadManager::Observer interface.
// =========================================================================

class AstraDownloadsBubbleModel : public AstraDownloadsObserver {
 public:
  explicit AstraDownloadsBubbleModel(AstraDownloadsHelper* helper);
  ~AstraDownloadsBubbleModel() override;

  AstraDownloadsBubbleModel(const AstraDownloadsBubbleModel&) = delete;
  AstraDownloadsBubbleModel& operator=(const AstraDownloadsBubbleModel&) =
      delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(AstraDownloadsBubbleObserver* observer);
  void RemoveObserver(AstraDownloadsBubbleObserver* observer);

  // -- Download list access -----------------------------------------------

  // Get all downloads to display in the bubble (active + recent completed).
  // Downloads are sorted according to the current sort order.
  std::vector<AstraDownloadsBubbleItem> GetDisplayDownloads() const;

  // Get the number of downloads displayed in the bubble.
  size_t GetDisplayCount() const;

  // Get a specific download by ID.  Returns nullptr if not found.
  const AstraDownloadsBubbleItem* GetDownload(const std::string& id) const;

  // -- Active count (for badge) -------------------------------------------

  // Number of currently active (in-progress) downloads.
  int GetActiveDownloadCount() const;

  // True if there are any active downloads.
  bool HasActiveDownloads() const;

  // Total number of downloads (all states).
  int GetTotalDownloadCount() const;

  // -- Sorting and filtering ----------------------------------------------

  // Set the sort order for displayed downloads.
  void SetSortOrder(AstraDownloadsBubbleSortOrder order);
  AstraDownloadsBubbleSortOrder GetSortOrder() const { return sort_order_; }

  // Set the maximum number of items to show in the bubble.
  void SetMaxDisplayItems(int max);
  int GetMaxDisplayItems() const { return max_display_items_; }

  // Whether to show completed downloads in the bubble.
  void SetShowCompleted(bool show);
  bool GetShowCompleted() const { return show_completed_; }

  // Whether to show cancelled/failed downloads in the bubble.
  void SetShowFailed(bool show);
  bool GetShowFailed() const { return show_failed_; }

  // -- Download actions ---------------------------------------------------
  //
  // These delegate to AstraDownloadsHelper, which in turn delegates to
  // Chromium's DownloadManager.

  void PauseDownload(const std::string& id);
  void ResumeDownload(const std::string& id);
  void CancelDownload(const std::string& id);
  void RemoveDownload(const std::string& id);
  void OpenDownload(const std::string& id);
  void ShowDownloadInFolder(const std::string& id);
  void RetryDownload(const std::string& id);

  void PauseAllDownloads();
  void ResumeAllDownloads();
  void ClearCompletedDownloads();

  // -- Refresh ------------------------------------------------------------

  // Refresh download data from the underlying helper.
  void Refresh();

  // -- Presentation settings ----------------------------------------------

  // Whether to show file size in the item view.
  bool show_file_size() const { return show_file_size_; }
  void set_show_file_size(bool show);

  // Whether to show download speed in the item view.
  bool show_speed() const { return show_speed_; }
  void set_show_speed(bool show);

  // Whether to show time remaining in the item view.
  bool show_time_remaining() const { return show_time_remaining_; }
  void set_show_time_remaining(bool show);

  // Whether to show a progress ring on the toolbar button.
  bool show_progress_ring() const { return show_progress_ring_; }
  void set_show_progress_ring(bool show);

  // Whether to show the active download count badge.
  bool show_badge() const { return show_badge_; }
  void set_show_badge(bool show);

  // -- Utility ------------------------------------------------------------

  // Format byte count as human-readable string.
  static std::u16 FormatBytes(int64_t bytes);

  // Format download speed as human-readable string.
  static std::u16 FormatSpeed(int64_t bytes_per_sec);

  // Format time remaining as human-readable string.
  static std::u16 FormatTimeRemaining(base::TimeDelta remaining);

  // Calculate progress as 0.0 to 1.0.
  static double CalculateProgress(int64_t received, int64_t total);

  // -- Constants ----------------------------------------------------------

  static constexpr int kDefaultMaxDisplayItems = 5;
  static constexpr int kMinDisplayItems = 1;
  static constexpr int kMaxDisplayItems = 20;

  // -- AstraDownloadsObserver ---------------------------------------------

  void OnDownloadStarted(int download_id) override;
  void OnDownloadUpdated(int download_id) override;
  void OnDownloadCompleted(int download_id) override;
  void OnDownloadFailed(int download_id, const std::string& error) override;
  void OnDownloadRemoved(int download_id) override;
  void OnAllDownloadsCleared() override;
  void OnDownloadsSettingsChanged() override;

 private:
  // Convert an AstraDownloadItem to a bubble item.
  static AstraDownloadsBubbleItem ToBubbleItem(const AstraDownloadItem& item);

  // Apply filters to produce the display list.
  std::vector<AstraDownloadsBubbleItem> GetFilteredDownloads() const;

  // Apply sort order to a list of items.
  void SortItems(std::vector<AstraDownloadsBubbleItem>& items) const;

  // Static sort comparator.
  static bool CompareItems(const AstraDownloadsBubbleItem& a,
                           const AstraDownloadsBubbleItem& b,
                           AstraDownloadsBubbleSortOrder order);

  // Notify observers that downloads changed.
  void NotifyDownloadsChanged();

  // Notify observers that a specific download was updated.
  void NotifyDownloadUpdated(const std::string& id);

  // Notify observers that the active count changed.
  void NotifyActiveCountChanged();

  // Helper to convert int download_id to string.
  static std::string IdToString(int download_id);

  // The downloads helper we observe and project from.  Not owned.
  raw_ptr<AstraDownloadsHelper> helper_ = nullptr;

  // Observers.
  base::ObserverList<AstraDownloadsBubbleObserver> observers_;

  // Display settings.
  AstraDownloadsBubbleSortOrder sort_order_ =
      AstraDownloadsBubbleSortOrder::kNewestFirst;
  int max_display_items_ = kDefaultMaxDisplayItems;
  bool show_completed_ = true;
  bool show_failed_ = false;

  // Presentation settings.
  bool show_file_size_ = true;
  bool show_speed_ = true;
  bool show_time_remaining_ = true;
  bool show_progress_ring_ = true;
  bool show_badge_ = true;

  // Cached active count (for change detection).
  mutable int cached_active_count_ = 0;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_MODEL_H_
