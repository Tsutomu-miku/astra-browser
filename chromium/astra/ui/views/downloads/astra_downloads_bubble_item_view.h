#ifndef ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_downloads_helper.h"
#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"

namespace views {
class ImageView;
class Label;
class ProgressBar;
}  // namespace views

namespace astra {

// =========================================================================
// AstraDownloadsBubbleItemDelegate — action callbacks for bubble item
// =========================================================================
//
// Delegate interface for user actions on a download item in the bubble.
// Implemented by the parent bubble view.
class AstraDownloadsBubbleItemDelegate {
 public:
  virtual ~AstraDownloadsBubbleItemDelegate() = default;

  // Called when the download item is clicked (primary action).
  virtual void OnDownloadItemClicked(const std::string& download_id) = 0;

  // Called when the pause button is pressed.
  virtual void OnPauseDownload(const std::string& download_id) = 0;

  // Called when the resume button is pressed.
  virtual void OnResumeDownload(const std::string& download_id) = 0;

  // Called when the cancel button is pressed.
  virtual void OnCancelDownload(const std::string& download_id) = 0;

  // Called when the open button is pressed.
  virtual void OnOpenDownload(const std::string& download_id) = 0;

  // Called when the "show in folder" button is pressed.
  virtual void OnShowDownloadInFolder(const std::string& download_id) = 0;

  // Called when the retry button is pressed (for failed downloads).
  virtual void OnRetryDownload(const std::string& download_id) = 0;

  // Called when the remove button is pressed.
  virtual void OnRemoveDownload(const std::string& download_id) = 0;
};

// =========================================================================
// AstraDownloadsBubbleItemView — single download item in the bubble
// =========================================================================
//
// A single download entry in the downloads bubble.  Shows:
//   - File type icon (left)
//   - Filename (top line)
//   - Status text (bottom line: size, speed, time remaining, or state)
//   - Progress bar (for in-progress downloads)
//   - Action buttons (right side, shown on hover)
//
// Layout (left to right):
//   [icon] [filename]        [action buttons]
//          [status / progress]
//
// This is a pure presentation view — it does not own download state.
// Data is set by the parent bubble view via setter methods.
//
// TODO(astra): Use real file type icons from Chromium's resource bundle.
//   Chromium owner: chrome/browser/ui/views/download/download_item_view.h
//   Chromium pattern: GetIconForDangerousDownload / GetFileIcon
// =========================================================================

class AstraDownloadsBubbleItemView : public views::View {
 public:
  explicit AstraDownloadsBubbleItemView(const std::string& download_id);
  ~AstraDownloadsBubbleItemView() override;

  AstraDownloadsBubbleItemView(const AstraDownloadsBubbleItemView&) = delete;
  AstraDownloadsBubbleItemView& operator=(
      const AstraDownloadsBubbleItemView&) = delete;

  // -- Download data setters ----------------------------------------------

  void SetFilename(const std::u16string& filename);
  void SetURL(const GURL& url);
  void SetState(AstraDownloadState state);
  void SetProgress(double progress);
  void SetBytes(int64_t received, int64_t total);
  void SetSpeed(int64_t bytes_per_sec);
  void SetTimeRemaining(base::TimeDelta remaining);
  void SetIsDangerous(bool dangerous);
  void SetDangerType(const std::string& danger_type);

  // Update all data at once.
  void UpdateFromItem(const struct AstraDownloadsBubbleItem& item);

  // -- Accessors ----------------------------------------------------------

  const std::string& download_id() const { return download_id_; }
  AstraDownloadState state() const { return state_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraDownloadsBubbleItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- Display options ----------------------------------------------------

  void SetShowProgress(bool show);
  void SetShowSpeed(bool show);
  void SetShowTimeRemaining(bool show);
  void SetShowFileSize(bool show);

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;

 private:
  // Build the child views.
  void BuildLayout();

  // Update the status text based on current state.
  void UpdateStatusText();

  // Update the progress bar visibility and value.
  void UpdateProgressBar();

  // Update action button visibility based on state and hover.
  void UpdateActionButtons();

  // Update the icon based on file type / state.
  void UpdateIcon();

  // Update colors from the color provider.
  void UpdateColors();

  // Get a human-readable state label.
  std::u16 GetStateLabel() const;

  // Format bytes as a human-readable string.
  std::u16 FormatBytes(int64_t bytes) const;

  // Format speed as a human-readable string.
  std::u16 FormatSpeed(int64_t bytes_per_sec) const;

  // Format time remaining as a human-readable string.
  std::u16 FormatTimeRemaining(base::TimeDelta remaining) const;

  // Button handlers.
  void OnPauseButtonPressed();
  void OnResumeButtonPressed();
  void OnCancelButtonPressed();
  void OnOpenButtonPressed();
  void OnShowInFolderButtonPressed();
  void OnRetryButtonPressed();
  void OnRemoveButtonPressed();

  // Stable identifier for this download.
  std::string download_id_;

  // Current download state and data.
  AstraDownloadState state_ = AstraDownloadState::kInProgress;
  std::u16string filename_;
  GURL url_;
  int64_t received_bytes_ = 0;
  int64_t total_bytes_ = -1;
  double progress_ = 0.0;
  int64_t speed_bytes_per_sec_ = 0;
  base::TimeDelta time_remaining_;
  bool is_dangerous_ = false;
  std::string danger_type_;

  // Display options.
  bool show_progress_ = true;
  bool show_speed_ = true;
  bool show_time_remaining_ = true;
  bool show_file_size_ = true;

  // Hover state.
  bool is_hovered_ = false;

  // Delegate for action callbacks.  Not owned.
  raw_ptr<AstraDownloadsBubbleItemDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::View> text_container_ = nullptr;
  raw_ptr<views::Label> filename_label_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::ProgressBar> progress_bar_ = nullptr;
  raw_ptr<views::View> action_container_ = nullptr;
  raw_ptr<views::ImageButton> pause_button_ = nullptr;
  raw_ptr<views::ImageButton> resume_button_ = nullptr;
  raw_ptr<views::ImageButton> cancel_button_ = nullptr;
  raw_ptr<views::ImageButton> open_button_ = nullptr;
  raw_ptr<views::ImageButton> show_in_folder_button_ = nullptr;
  raw_ptr<views::ImageButton> retry_button_ = nullptr;
  raw_ptr<views::ImageButton> remove_button_ = nullptr;

  // -- Layout constants ---------------------------------------------------

  static constexpr int kIconSize = 32;
  static constexpr int kItemHeight = 56;
  static constexpr int kProgressBarHeight = 3;
  static constexpr int kHorizontalPadding = 12;
  static constexpr int kIconTextSpacing = 12;
  static constexpr int kTextActionSpacing = 8;
  static constexpr int kActionButtonSize = 28;
  static constexpr int kActionButtonSpacing = 4;
  static constexpr int kFilenameLineHeight = 16;
  static constexpr int kStatusLineHeight = 14;
  static constexpr int kTextProgressSpacing = 4;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_ITEM_VIEW_H_
