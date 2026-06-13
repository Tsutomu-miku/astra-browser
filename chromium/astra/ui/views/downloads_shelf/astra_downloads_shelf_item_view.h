#ifndef ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_downloads_helper.h"

namespace views {
class ImageView;
class Label;
class ProgressBar;
}  // namespace views

namespace astra {

// =========================================================================
// AstraDownloadsShelfItemDelegate — delegate for shelf item actions
// =========================================================================
class AstraDownloadsShelfItemDelegate {
 public:
  virtual ~AstraDownloadsShelfItemDelegate() = default;

  // Called when the item is clicked (primary action).
  virtual void OnDownloadClicked(int download_id) = 0;

  // Called when the pause button is pressed.
  virtual void OnPauseDownload(int download_id) = 0;

  // Called when the resume button is pressed.
  virtual void OnResumeDownload(int download_id) = 0;

  // Called when the cancel button is pressed.
  virtual void OnCancelDownload(int download_id) = 0;

  // Called when the "show in folder" button is pressed.
  virtual void OnShowInFolder(int download_id) = 0;

  // Called when the dismiss button is pressed (removes from shelf).
  virtual void OnDismissItem(int download_id) = 0;
};

// =========================================================================
// AstraDownloadsShelfItemView — single download item in the shelf
// =========================================================================
//
// A single download "chip" in the downloads shelf.  Compact horizontal
// layout showing file name, progress, and action buttons.
//
// Layout (left to right):
//   - File type icon
//   - File name (elided)
//   - Progress bar (thin, at bottom)
//   - Action buttons (on hover: pause/resume, show in folder, dismiss)
//
// Chromium pattern: DownloadItemView
//   (chrome/browser/ui/views/download/download_item_view.h)
// =========================================================================

class AstraDownloadsShelfItemView : public views::View {
 public:
  AstraDownloadsShelfItemView(int download_id, const std::u16string& filename);
  ~AstraDownloadsShelfItemView() override;

  AstraDownloadsShelfItemView(const AstraDownloadsShelfItemView&) = delete;
  AstraDownloadsShelfItemView& operator=(
      const AstraDownloadsShelfItemView&) = delete;

  // -- Data setters -------------------------------------------------------

  void SetFilename(const std::u16string& filename);
  void SetURL(const GURL& url);
  void SetState(AstraDownloadState state);
  void SetProgress(double progress);
  void SetBytes(int64_t received, int64_t total);
  void SetSpeed(int64_t bytes_per_sec);
  void SetDanger(bool is_dangerous);

  // -- Accessors ----------------------------------------------------------

  int download_id() const { return download_id_; }
  AstraDownloadState state() const { return state_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraDownloadsShelfItemDelegate* delegate) {
    delegate_ = delegate;
  }

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
  // Build child views.
  void BuildLayout();

  // Update the status text and progress.
  void UpdateDisplay();

  // Update action button visibility based on state and hover.
  void UpdateActionButtons();

  // Update icon based on file type / state.
  void UpdateIcon();

  // Update colors from theme.
  void UpdateColors();

  // Button handlers.
  void OnPausePressed();
  void OnResumePressed();
  void OnCancelPressed();
  void OnShowInFolderPressed();
  void OnDismissPressed();

  // Format helpers.
  std::u16 FormatBytes(int64_t bytes) const;
  std::u16 FormatSpeed(int64_t bytes_per_sec) const;
  std::u16 GetStateLabel() const;

  // Download identifier.
  int download_id_ = -1;

  // Download data.
  std::u16string filename_;
  GURL url_;
  AstraDownloadState state_ = AstraDownloadState::kInProgress;
  double progress_ = 0.0;
  int64_t received_bytes_ = 0;
  int64_t total_bytes_ = -1;
  int64_t speed_bytes_per_sec_ = 0;
  bool is_dangerous_ = false;

  // Hover state.
  bool is_hovered_ = false;

  // Delegate.
  raw_ptr<AstraDownloadsShelfItemDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::View> text_container_ = nullptr;
  raw_ptr<views::Label> filename_label_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::ProgressBar> progress_bar_ = nullptr;
  raw_ptr<views::View> action_container_ = nullptr;
  raw_ptr<views::ImageButton> pause_button_ = nullptr;
  raw_ptr<views::ImageButton> resume_button_ = nullptr;
  raw_ptr<views::ImageButton> cancel_button_ = nullptr;
  raw_ptr<views::ImageButton> show_in_folder_button_ = nullptr;
  raw_ptr<views::ImageButton> dismiss_button_ = nullptr;

  // -- Constants ----------------------------------------------------------

  static constexpr int kIconSize = 20;
  static constexpr int kItemWidth = 180;
  static constexpr int kItemHeight = 36;
  static constexpr int kProgressBarHeight = 2;
  static constexpr int kIconTextSpacing = 8;
  static constexpr int kHorizontalPadding = 8;
  static constexpr int kActionButtonSize = 20;
  static constexpr int kActionButtonSpacing = 2;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_ITEM_VIEW_H_
