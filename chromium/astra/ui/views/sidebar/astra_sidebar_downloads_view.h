#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "astra/ui/views/sidebar/astra_download_item_view.h"
#include "content/public/browser/download_manager.h"
#include "ui/views/view.h"

namespace views {
class Label;
class View;
}  // namespace views

namespace astra {

// Sidebar section that displays active and recent downloads.
//
// This is a presentation-only view: it projects Chromium download state
// (from content::DownloadManager) into a vertical list. It never stores
// download truth state — the DownloadManager is the single source of truth.
//
// Display order:
//   1. Active downloads (in-progress) — with live progress bars
//   2. Recent completed downloads — up to kMaxRecentItems
//
// Each download item shows: filename, progress (for active), size/status,
// and a cancel button (visible on hover for active downloads).
//
// Click behavior:
//   - Completed download: opens the file (delegates to DownloadItem::OpenDownload)
//   - Active download: shows the download in the shelf/page (delegates to Chrome)
//   - "Show all" link at bottom: opens chrome://downloads
//
// Implements content::DownloadManager::Observer to receive live download
// updates from Chromium's download subsystem.
//
// TODO(astra): Consider using AllDownloadsNotifier instead of direct
// DownloadManager observation. AllDownloadsNotifier handles off-the-record
// profiles and provides a unified view across profiles.
// Chromium owner: AllDownloadsNotifier (chrome/browser/download/all_downloads_notifier.h)
// Chromium owner: DownloadShelf (chrome/browser/ui/views/download/download_shelf_view.h)
class AstraSidebarDownloadsView : public views::View,
                                  public content::DownloadManager::Observer,
                                  public AstraDownloadItemDelegate {
 public:
  // Maximum number of recent (completed) downloads shown in the sidebar.
  static constexpr size_t kMaxRecentItems = 5;

  explicit AstraSidebarDownloadsView(content::DownloadManager* download_manager);
  AstraSidebarDownloadsView(const AstraSidebarDownloadsView&) = delete;
  AstraSidebarDownloadsView& operator=(const AstraSidebarDownloadsView&) = delete;
  ~AstraSidebarDownloadsView() override;

  // Refresh the downloads list from the underlying DownloadManager.
  // Full rebuild — used for initial sync and when incremental updates
  // are not possible.
  void RefreshFromManager();

  // Set section visibility and update layout accordingly.
  void SetSectionVisible(bool visible);

  // -- content::DownloadManager::Observer --------------------------------

  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadUpdated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadRemoved(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

  // -- AstraDownloadItemDelegate -----------------------------------------

  void OnDownloadItemClicked(const std::string& download_id) override;
  void OnDownloadCancelRequested(const std::string& download_id) override;

  // -- views::View -------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Find the item view for a given download ID, or nullptr if not found.
  AstraDownloadItemView* FindItemView(const std::string& download_id) const;

  // Create a download item view from a DownloadItem.
  std::unique_ptr<AstraDownloadItemView> CreateItemView(
      download::DownloadItem* item);

  // Get the display string for the download state (used in status text).
  // Maps download::DownloadItem::DownloadState to our presentation enum.
  static AstraDownloadState MapDownloadState(
      download::DownloadItem::DownloadState state);

  // Generate a stable string ID for a download item.
  // Uses the download's GUID if available, otherwise the integer ID as a string.
  static std::string GetDownloadId(download::DownloadItem* item);

  // Called when the "Show all downloads" link is clicked.
  void OnShowAllClicked();

  // Sort and rebuild the items list from the current DownloadManager state.
  // Active downloads come first, then completed downloads (most recent first).
  void RebuildItems();

  // Update the visibility of the "Show all" link based on item count.
  void UpdateShowAllVisibility();

  // The Chromium DownloadManager we observe and project.
  // Not owned — we observe it and clear our pointer in ManagerGoingDown.
  raw_ptr<content::DownloadManager> download_manager_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::Label> show_all_label_ = nullptr;

  // Tracks whether we're currently observing the download manager.
  bool is_observing_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DOWNLOADS_VIEW_H_
