#ifndef ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_TOOLBAR_BUTTON_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_TOOLBAR_BUTTON_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "ui/views/controls/button/image_button.h"

#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"

namespace views {
class ImageView;
class Label;
class View;
}  // namespace views

namespace astra {

class AstraDownloadsBubbleModel;
class AstraDownloadsBubbleDelegate;

// =========================================================================
// AstraDownloadsToolbarButton — toolbar download button
// =========================================================================
//
// A toolbar button that shows the downloads bubble when clicked.
// Features:
//   - Download icon
//   - Badge showing the number of active downloads
//   - Progress ring around the icon indicating overall progress
//   - Animates when a download completes
//   - Shows different states: idle, downloading, completed, error
//
// The button observes AstraDownloadsBubbleModel for download count changes
// and updates its badge and progress indicator accordingly.
//
// Chromium pattern: ToolbarButton / ImageButton in the browser toolbar.
//   (chrome/browser/ui/views/toolbar/toolbar_button.h)
//
// TODO(astra): Integrate with Chromium's toolbar view.
//   Chromium owner: ToolbarView (chrome/browser/ui/views/toolbar/)
//   Patch point: ToolbarView::Init() — add the download button to the
//   trailing toolbar button row.
// =========================================================================

class AstraDownloadsToolbarButton
    : public views::ImageButton,
      public AstraDownloadsBubbleObserver {
 public:
  // Button state enum for different visual states.
  enum class State {
    kIdle,          // No active downloads
    kDownloading,   // Active downloads in progress
    kCompleted,     // All downloads completed (brief state)
    kError,         // A download failed
  };

  explicit AstraDownloadsToolbarButton(AstraDownloadsBubbleModel* model);
  ~AstraDownloadsToolbarButton() override;

  AstraDownloadsToolbarButton(const AstraDownloadsToolbarButton&) = delete;
  AstraDownloadsToolbarButton& operator=(
      const AstraDownloadsToolbarButton&) = delete;

  // -- Model binding ------------------------------------------------------

  void SetModel(AstraDownloadsBubbleModel* model);
  AstraDownloadsBubbleModel* model() { return model_; }

  // -- Bubble management --------------------------------------------------

  // Show the downloads bubble anchored to this button.
  void ShowBubble(AstraDownloadsBubbleDelegate* delegate);

  // Hide the downloads bubble if it's showing.
  void HideBubble();

  // Returns true if the downloads bubble is currently showing.
  bool IsBubbleShowing() const;

  // -- State --------------------------------------------------------------

  State GetState() const { return state_; }

  // -- Badge --------------------------------------------------------------

  // Set the active download count displayed in the badge.
  void SetActiveCount(int count);
  int GetActiveCount() const { return active_count_; }

  // Set overall download progress (0.0 to 1.0) for the progress ring.
  void SetOverallProgress(double progress);
  double GetOverallProgress() const { return overall_progress_; }

  // -- Display options ----------------------------------------------------

  void SetShowBadge(bool show);
  bool GetShowBadge() const { return show_badge_; }

  void SetShowProgressRing(bool show);
  bool GetShowProgressRing() const { return show_progress_ring_; }

  // -- Animation ----------------------------------------------------------

  // Play the "download complete" animation (pulse + badge flash).
  void PlayCompleteAnimation();

  // Play the "download started" animation (bounce + badge appear).
  void PlayStartAnimation();

  // -- views::ImageButton -------------------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnThemeChanged() override;
  void PaintButtonContents(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // -- AstraDownloadsBubbleObserver ---------------------------------------

  void OnDownloadsChanged(AstraDownloadsBubbleModel* model) override;
  void OnDownloadUpdated(AstraDownloadsBubbleModel* model,
                         const std::string& download_id) override;
  void OnDownloadStarted(AstraDownloadsBubbleModel* model,
                         const std::string& download_id) override;
  void OnDownloadCompleted(AstraDownloadsBubbleModel* model,
                           const std::string& download_id) override;
  void OnActiveCountChanged(AstraDownloadsBubbleModel* model,
                            int active_count) override;
  void OnDownloadsBubbleModelShutdown(
      AstraDownloadsBubbleModel* model) override;

 private:
  // Calculate overall download progress from the model.
  double CalculateOverallProgress() const;

  // Update the button's visual state from the model.
  void UpdateStateFromModel();

  // Update the badge text and visibility.
  void UpdateBadge();

  // Update the icon based on current state.
  void UpdateIcon();

  // Update the progress ring.
  void UpdateProgressRing();

  // Update the tooltip text.
  void UpdateTooltip();

  // Paint the progress ring around the icon.
  void PaintProgressRing(gfx::Canvas* canvas);

  // Paint the badge showing active count.
  void PaintBadge(gfx::Canvas* canvas);

  // Button click handler.
  void OnButtonPressed();

  // Animation timer callbacks.
  void OnCompleteAnimationTick();
  void OnStartAnimationTick();
  void OnAnimationFinished();

  // The model providing download state.  Not owned.
  raw_ptr<AstraDownloadsBubbleModel> model_ = nullptr;

  // The bubble widget (if showing).  Not owned.
  raw_ptr<views::Widget> bubble_widget_ = nullptr;

  // Current button state.
  State state_ = State::kIdle;

  // Active download count (for badge).
  int active_count_ = 0;

  // Overall download progress (0.0 to 1.0).
  double overall_progress_ = 0.0;

  // Display options.
  bool show_badge_ = true;
  bool show_progress_ring_ = true;

  // Hover state.
  bool is_hovered_ = false;

  // Animation state.
  bool animating_complete_ = false;
  bool animating_start_ = false;
  base::OneShotTimer animation_timer_;
  int animation_frame_ = 0;

  // Scoped observation for model changes.
  base::ScopedObservation<AstraDownloadsBubbleModel,
                          AstraDownloadsBubbleObserver>
      model_observation_{this};

  // -- Constants ----------------------------------------------------------

  static constexpr int kButtonSize = 36;
  static constexpr int kIconSize = 20;
  static constexpr int kBadgeSize = 16;
  static constexpr int kBadgeFontSize = 10;
  static constexpr int kProgressRingStrokeWidth = 2;
  static constexpr int kProgressRingInset = 2;
  static constexpr base::TimeDelta kCompleteAnimationDuration =
      base::Milliseconds(600);
  static constexpr base::TimeDelta kStartAnimationDuration =
      base::Milliseconds(300);
  static constexpr int kAnimationFrameCount = 12;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_ASTRA_DOWNLOADS_TOOLBAR_BUTTON_H_
