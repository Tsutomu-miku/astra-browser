#ifndef ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_VIEW_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"

namespace views {
class ImageButton;
class Label;
class ScrollView;
class View;
}  // namespace views

namespace astra {

class AstraDownloadsBubbleModel;
class AstraDownloadsBubbleItemView;
struct AstraDownloadsBubbleItem;

// =========================================================================
// AstraDownloadsBubbleDelegate — delegate for bubble-level actions
// =========================================================================
//
// Delegate interface for actions that affect the bubble as a whole.
// Implemented by browser-level code (e.g. AstraBrowserView).
class AstraDownloadsBubbleDelegate {
 public:
  virtual ~AstraDownloadsBubbleDelegate() = default;

  // Called when the user clicks "Show all downloads" (opens the full
  // downloads page or sidebar downloads section).
  virtual void OnShowAllDownloadsRequested() = 0;

  // Called when the user clicks "Clear all" (clears completed downloads).
  virtual void OnClearAllDownloadsRequested() = 0;

  // Called when the bubble is about to close.
  virtual void OnDownloadsBubbleClosing() {}

  // Called to open the downloads settings.
  virtual void OnDownloadsSettingsRequested() {}
};

// =========================================================================
// AstraDownloadsBubbleView — the downloads bubble view
// =========================================================================
//
// A bubble dialog that shows recent downloads, with progress bars and
// action buttons for each download.  Similar to Chrome's download bubble
// that appears when clicking the download icon in the toolbar.
//
// Layout (top to bottom):
//   1. Header bar — "Downloads" title, active count badge, settings button
//   2. Scrollable content area — list of download items
//   3. Empty state view (shown when no downloads)
//   4. Footer bar — "Show all downloads" link, "Clear all" button
//
// The view observes AstraDownloadsBubbleModel for data changes.
//
// Chromium pattern: views::BubbleDialogDelegateView
//   (ui/views/bubble/bubble_dialog_delegate_view.h)
//
// TODO(astra): Add animation for new download items appearing.
//   Chromium pattern: views::Animation / gfx::SlideAnimation
// =========================================================================

class AstraDownloadsBubbleView
    : public views::BubbleDialogDelegateView,
      public AstraDownloadsBubbleItemDelegate {
 public:
  // Creates and shows the downloads bubble anchored to |anchor_view|.
  // |anchor_rect| is in |anchor_view|'s coordinate system.
  // Returns the widget owning the bubble (owned by the widget system).
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   const gfx::Rect& anchor_rect,
                                   AstraDownloadsBubbleModel* model,
                                   AstraDownloadsBubbleDelegate* delegate);

  ~AstraDownloadsBubbleView() override;

  AstraDownloadsBubbleView(const AstraDownloadsBubbleView&) = delete;
  AstraDownloadsBubbleView& operator=(const AstraDownloadsBubbleView&) = delete;

  // -- Model and delegate -------------------------------------------------

  void SetModel(AstraDownloadsBubbleModel* model);
  AstraDownloadsBubbleModel* model() { return model_; }

  void set_delegate(AstraDownloadsBubbleDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- View refresh -------------------------------------------------------

  // Refresh all download items from the model.
  void RefreshFromModel();

  // Update a single download item.
  void UpdateDownloadItem(const std::string& download_id);

  // Add a new download item (prepended to the list).
  void AddDownloadItem(const struct AstraDownloadsBubbleItem& item);

  // Remove a download item by ID.
  void RemoveDownloadItem(const std::string& download_id);

  // -- Bubble state -------------------------------------------------------

  // Show the empty state (no downloads).
  void ShowEmptyState();

  // Hide the empty state and show the download list.
  void HideEmptyState();

  // Update the active count badge in the header.
  void UpdateActiveCountBadge();

  // -- BubbleDialogDelegateView overrides ---------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- AstraDownloadsBubbleItemDelegate ----------------------------------

  void OnDownloadItemClicked(const std::string& download_id) override;
  void OnPauseDownload(const std::string& download_id) override;
  void OnResumeDownload(const std::string& download_id) override;
  void OnCancelDownload(const std::string& download_id) override;
  void OnOpenDownload(const std::string& download_id) override;
  void OnShowDownloadInFolder(const std::string& download_id) override;
  void OnRetryDownload(const std::string& download_id) override;
  void OnRemoveDownload(const std::string& download_id) override;

 private:
  AstraDownloadsBubbleView(views::View* anchor_view,
                           const gfx::Rect& anchor_rect,
                           AstraDownloadsBubbleModel* model,
                           AstraDownloadsBubbleDelegate* delegate);

  // BubbleDialogDelegateView:
  void Init() override;

  // Build the header bar.
  void BuildHeader();

  // Build the scrollable content area.
  void BuildContentArea();

  // Build the empty state view.
  void BuildEmptyState();

  // Build the footer bar.
  void BuildFooter();

  // Rebuild all download items from the model.
  void RebuildItems();

  // Find an item view by download ID.
  AstraDownloadsBubbleItemView* FindItemView(
      const std::string& download_id) const;

  // Create a new item view from a bubble item.
  std::unique_ptr<AstraDownloadsBubbleItemView> CreateItemView(
      const struct AstraDownloadsBubbleItem& item);

  // Update colors from the color provider.
  void UpdateColors();

  // Update the header title and badge.
  void UpdateHeader();

  // Button handlers.
  void OnSettingsButtonPressed();
  void OnClearAllButtonPressed();
  void OnShowAllButtonPressed();

  // The model providing download data.  Not owned.
  raw_ptr<AstraDownloadsBubbleModel> model_ = nullptr;

  // Delegate for bubble-level actions.  Not owned.
  raw_ptr<AstraDownloadsBubbleDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> active_count_badge_ = nullptr;
  raw_ptr<views::ImageButton> settings_button_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::View> empty_state_view_ = nullptr;
  raw_ptr<views::View> footer_ = nullptr;
  raw_ptr<views::LabelButton> show_all_button_ = nullptr;
  raw_ptr<views::LabelButton> clear_all_button_ = nullptr;

  // Item views in display order (owned by items_container_).
  std::vector<raw_ptr<AstraDownloadsBubbleItemView>> item_views_;

  // -- Layout constants ---------------------------------------------------

  static constexpr int kBubbleWidth = 360;
  static constexpr int kMaxBubbleHeight = 480;
  static constexpr int kHeaderHeight = 40;
  static constexpr int kFooterHeight = 36;
  static constexpr int kItemSpacing = 1;
  static constexpr int kHorizontalPadding = 0;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_BUBBLE_VIEW_H_
