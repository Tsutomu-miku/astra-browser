#ifndef ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_VIEW_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

#include "astra/browser/astra_downloads_helper.h"
#include "astra/ui/views/downloads_shelf/astra_downloads_shelf_item_view.h"

namespace views {
class ImageButton;
class Label;
class ScrollView;
}  // namespace views

namespace astra {

class AstraDownloadsHelper;

// =========================================================================
// AstraDownloadsShelfView — downloads shelf (bottom bar)
// =========================================================================
//
// A horizontal bar that appears at the bottom of the browser window when
// downloads start or complete.  Shows download items with progress and
// action buttons.
//
// Similar to Chrome's download shelf:
//   - Appears from the bottom with a slide-in animation
//   - Shows each download as a "chip" with icon, name, progress, and actions
//   - Auto-hides after a delay (configurable)
//   - Has a close button to dismiss the shelf
//   - Has a "Show all" link that opens the downloads page
//
// Layout (left to right):
//   1. Download icon + "Downloads" label
//   2. Scrollable row of download chips
//   3. "Show all downloads" link
//   4. Close button
//
// The shelf observes AstraDownloadsHelper for download state changes.
//
// Chromium pattern: DownloadShelf / DownloadShelfView
//   (chrome/browser/ui/views/download/download_shelf.h)
//
// TODO(astra): Integrate with BrowserView via a patch.
//   Chromium owner: BrowserView (chrome/browser/ui/views/frame/browser_view.h)
//   Patch point: BrowserView::InitViews() — insert the download shelf
//   above the status bar or at the bottom of the contents area.
// =========================================================================

class AstraDownloadsShelfView
    : public views::View,
      public AstraDownloadsObserver,
      public AstraDownloadsShelfItemDelegate {
 public:
  // Delegate for shelf-level actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the shelf is closed by the user.
    virtual void OnShelfClosed() = 0;

    // Called when "Show all downloads" is clicked.
    virtual void OnShowAllDownloads() = 0;

    // Called to open a download item.
    virtual void OnOpenDownload(int download_id) = 0;

    // Called to show a download in its folder.
    virtual void OnShowDownloadInFolder(int download_id) = 0;
  };

  explicit AstraDownloadsShelfView(AstraDownloadsHelper* helper);
  ~AstraDownloadsShelfView() override;

  AstraDownloadsShelfView(const AstraDownloadsShelfView&) = delete;
  AstraDownloadsShelfView& operator=(const AstraDownloadsShelfView&) =
      delete;

  // -- Show / hide --------------------------------------------------------

  // Show the shelf (slide-in animation).
  void Show();

  // Hide the shelf (slide-out animation).
  void Hide();

  // Hide the shelf after a delay (auto-hide).
  void HideAfterDelay(base::TimeDelta delay);

  // Cancel a pending auto-hide.
  void CancelAutoHide();

  // Whether the shelf is currently visible.
  bool IsShelfVisible() const { return shelf_visible_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() const { return delegate_; }

  // -- Helper access ------------------------------------------------------

  void SetHelper(AstraDownloadsHelper* helper);
  AstraDownloadsHelper* helper() { return helper_; }

  // -- Display options ----------------------------------------------------

  // Whether to auto-hide the shelf after downloads complete.
  void SetAutoHide(bool auto_hide);
  bool GetAutoHide() const { return auto_hide_; }

  // Set the auto-hide delay.
  void SetAutoHideDelay(base::TimeDelta delay);
  base::TimeDelta GetAutoHideDelay() const { return auto_hide_delay_; }

  // Maximum number of items to show in the shelf.
  void SetMaxItems(int max_items);
  int GetMaxItems() const { return max_items_; }

  // -- AstraDownloadsObserver ---------------------------------------------

  void OnDownloadStarted(int download_id) override;
  void OnDownloadUpdated(int download_id) override;
  void OnDownloadCompleted(int download_id) override;
  void OnDownloadFailed(int download_id, const std::string& error) override;
  void OnDownloadRemoved(int download_id) override;
  void OnAllDownloadsCleared() override;
  void OnDownloadsSettingsChanged() override;

  // -- AstraDownloadsShelfItemDelegate --------------------------------------

  void OnDownloadClicked(int download_id) override;
  void OnPauseDownload(int download_id) override;
  void OnResumeDownload(int download_id) override;
  void OnCancelDownload(int download_id) override;
  void OnShowInFolder(int download_id) override;
  void OnDismissItem(int download_id) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the shelf layout.
  void BuildLayout();

  // Build the header (icon + label).
  void BuildHeader();

  // Build the scrollable items container.
  void BuildItemsContainer();

  // Build the trailing controls (show all + close).
  void BuildTrailingControls();

  // Rebuild all download items from the helper.
  void RebuildItems();

  // Add a new download item to the shelf.
  void AddDownloadItem(int download_id);

  // Remove a download item from the shelf.
  void RemoveDownloadItem(int download_id);

  // Update a download item's state.
  void UpdateDownloadItem(int download_id);

  // Check if all downloads are complete (for auto-hide logic).
  bool AllDownloadsComplete() const;

  // Update colors from the color provider.
  void UpdateColors();

  // Start the auto-hide timer if all downloads are complete.
  void MaybeStartAutoHide();

  // Auto-hide timer callback.
  void OnAutoHideTimerFired();

  // Button handlers.
  void OnShowAllClicked();
  void OnCloseClicked();

  // Animation callbacks (stub for now).
  void OnShowAnimationComplete();
  void OnHideAnimationComplete();

  // The downloads helper we observe.  Not owned.
  raw_ptr<AstraDownloadsHelper> helper_ = nullptr;

  // Delegate for shelf-level actions.  Not owned.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Whether the shelf is currently visible.
  bool shelf_visible_ = false;

  // Auto-hide settings.
  bool auto_hide_ = true;
  base::TimeDelta auto_hide_delay_ = base::Seconds(8);
  base::OneShotTimer auto_hide_timer_;

  // Maximum items to show.
  int max_items_ = 5;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_ = nullptr;
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::View> trailing_ = nullptr;
  raw_ptr<views::LabelButton> show_all_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // Download item views in display order.
  // TODO(astra): Create AstraDownloadsShelfItemView for individual items.
  std::vector<raw_ptr<views::View>> item_views_;

  // Scoped observation of the helper.
  base::ScopedObservation<AstraDownloadsHelper,
                          AstraDownloadsObserver>
      helper_observation_{this};

  // -- Constants ----------------------------------------------------------

  static constexpr int kShelfHeight = 48;
  static constexpr int kHeaderWidth = 100;
  static constexpr int kTrailingWidth = 120;
  static constexpr int kItemSpacing = 8;
  static constexpr int kHorizontalPadding = 12;
  static constexpr int kItemHeight = 36;
  static constexpr int kCloseButtonSize = 24;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_SHELF_ASTRA_DOWNLOADS_SHELF_VIEW_H_
