#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_DOWNLOAD_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_DOWNLOAD_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageButton;
class ProgressBar;
}  // namespace views

namespace astra {

// States a download item can be in. Mirrors download::DownloadItem::DownloadState
// but is presentation-facing so the view doesn't need to depend on
// components/download/public/common directly.
//
// Chromium owner: download::DownloadItem::DownloadState
//   (components/download/public/common/download_item.h)
enum class AstraDownloadState {
  kInProgress,
  kComplete,
  kPaused,
  kFailed,
  kCancelled,
  kInterrupted,
};

// Delegate interface for AstraDownloadItemView actions.
// Implemented by the parent AstraSidebarDownloadsView.
class AstraDownloadItemDelegate {
 public:
  virtual ~AstraDownloadItemDelegate() = default;

  // Called when the user clicks the download item (primary action).
  virtual void OnDownloadItemClicked(const std::string& download_id) = 0;

  // Called when the user clicks the cancel button.
  virtual void OnDownloadCancelRequested(const std::string& download_id) = 0;

  // Called when the user clicks the pause button.
  virtual void OnDownloadPauseRequested(const std::string& download_id) = 0;

  // Called when the user clicks the resume button.
  virtual void OnDownloadResumeRequested(const std::string& download_id) = 0;

  // Called when the user clicks the "open" button (completed downloads).
  virtual void OnDownloadOpenRequested(const std::string& download_id) = 0;

  // Called when the user clicks the "show in folder" button.
  virtual void OnDownloadShowInFolderRequested(const std::string& download_id) = 0;
};

// A single download item row in the sidebar downloads section.
// Shows an icon, filename, status text, and (for active downloads) a
// progress bar and action buttons.
//
// This is a pure presentation view — it does not own download state.
// Data is projected from Chromium's DownloadManager by the parent
// AstraSidebarDownloadsView.
//
// TODO(astra): Replace placeholder icon with real file-type icon via
//   chrome/browser/ui/views/download/download_item_view.h icon utilities.
//   Chromium owner: DownloadItemView
//     (chrome/browser/ui/views/download/download_item_view.h)
class AstraDownloadItemView : public AstraSidebarItemView {
 public:
  AstraDownloadItemView(const std::string& download_id,
                        const std::u16string& filename,
                        AstraDownloadState state,
                        int64_t received_bytes,
                        int64_t total_bytes);
  AstraDownloadItemView(const AstraDownloadItemView&) = delete;
  AstraDownloadItemView& operator=(const AstraDownloadItemView&) = delete;
  ~AstraDownloadItemView() override;

  // -- Download info ------------------------------------------------------

  // Set all download info at once.
  void SetDownloadInfo(const GURL& url,
                       const std::u16string& filename,
                       int64_t total_bytes);

  // Get the filename.
  const std::u16string& GetFilename() const { return filename_; }

  // Get the source URL.
  const GURL& GetUrl() const { return url_; }

  // -- Progress -----------------------------------------------------------

  // Set the download progress (0.0 to 1.0).
  void SetProgress(double progress);
  double GetProgress() const { return progress_; }

  // -- State --------------------------------------------------------------

  // Set the current download state.
  void SetState(AstraDownloadState state);
  AstraDownloadState GetState() const { return state_; }

  // -- Bytes --------------------------------------------------------------

  // Set bytes received so far.
  void SetBytesReceived(int64_t bytes);
  int64_t GetBytesReceived() const { return received_bytes_; }

  // Set total bytes of the download.
  void SetTotalBytes(int64_t bytes);
  int64_t GetTotalBytes() const { return total_bytes_; }

  // -- Time remaining -----------------------------------------------------

  // Set estimated time remaining.
  void SetTimeRemaining(base::TimeDelta remaining);
  base::TimeDelta GetTimeRemaining() const { return time_remaining_; }

  // -- Dangerous download -------------------------------------------------

  // Set whether this download is marked as dangerous.
  void SetIsDangerous(bool dangerous);
  bool IsDangerous() const { return is_dangerous_; }

  // -- Action availability ------------------------------------------------

  // Whether the download can be resumed.
  bool CanResume() const;

  // Whether the download can be paused.
  bool CanPause() const;

  // Whether the download can be cancelled.
  bool CanCancel() const;

  // -- Visibility controls ------------------------------------------------

  // Show or hide the progress bar.
  void ShowProgressBar(bool show);

  // Show or hide the pause button.
  void ShowPauseButton(bool show);

  // Show or hide the cancel button.
  void ShowCancelButton(bool show);

  // Show or hide the open button.
  void ShowOpenButton(bool show);

  // Show or hide the "show in folder" button.
  void ShowInFolderButton(bool show);

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraDownloadItemDelegate* delegate) {
    delegate_ = delegate;
  }

  const std::string& download_id() const { return download_id_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Update the status text (size, speed, state description).
  void UpdateStatusText();

  // Update the progress bar visibility and value.
  void UpdateProgressBar();

  // Update action button visibility based on state and hover.
  void UpdateActionButtonsVisibility();

  // Update the icon based on file type / state.
  void UpdateIcon();

  // Format byte count as a human-readable string (e.g. "1.2 MB").
  std::u16 FormatBytes(int64_t bytes) const;

  // Format time remaining as a human-readable string.
  std::u16 FormatTimeRemaining() const;

  // Get a short status string for the current download state.
  std::u16 GetStateLabel() const;

  // Button action handlers.
  void OnPauseButtonPressed();
  void OnCancelButtonPressed();
  void OnResumeButtonPressed();
  void OnOpenButtonPressed();
  void OnShowInFolderButtonPressed();

  // Stable identifier for the download this item represents.
  std::string download_id_;

  // Current projected state. Updated by the parent via setters.
  AstraDownloadState state_;
  int64_t received_bytes_ = 0;
  int64_t total_bytes_ = 0;
  double progress_ = 0.0;
  base::TimeDelta time_remaining_;
  bool is_dangerous_ = false;

  // Filename and source URL.
  std::u16string filename_;
  GURL url_;

  // Visibility flags.
  bool show_progress_bar_ = true;
  bool show_pause_button_ = true;
  bool show_cancel_button_ = true;
  bool show_open_button_ = true;
  bool show_in_folder_button_ = true;

  // Action delegate. Not owned.
  raw_ptr<AstraDownloadItemDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ProgressBar> progress_bar_ = nullptr;
  raw_ptr<views::ImageButton> pause_button_ = nullptr;
  raw_ptr<views::ImageButton> cancel_button_ = nullptr;
  raw_ptr<views::ImageButton> resume_button_ = nullptr;
  raw_ptr<views::ImageButton> open_button_ = nullptr;
  raw_ptr<views::ImageButton> show_in_folder_button_ = nullptr;

  // Hover state for showing/hiding action buttons.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_DOWNLOAD_ITEM_VIEW_H_
