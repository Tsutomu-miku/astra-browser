#ifndef ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PEEK_CONTROLLER_H_
#define ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PEEK_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/ui/views/tab_hover/astra_tab_hover_model.h"

namespace content {
class WebContents;
}

namespace views {
class View;
class Widget;
}  // namespace views

namespace astra {

class AstraGlanceViewController;
class AstraTabHoverPreviewView;

// =========================================================================
// AstraTabHoverPeekController — manages peek/hover preview state
// =========================================================================
//
// Controls the lifecycle of tab peek previews triggered by hovering over a
// tab representation (sidebar item, tab strip tab, etc.).  After a hover
// delay, the controller shows either a lightweight preview card
// (AstraTabHoverPreviewView) or a full glance view (AstraGlanceView).
//
// The controller drives the model (AstraTabHoverModel) which owns all
// state, and updates the view to reflect model state.
//
// Architecture:
//   - Model owns truth state (AstraTabHoverModel).
//   - Controller drives model changes from user input and timers.
//   - View renders model state.
//   - UI never owns truth — model owns it.
//
// Two peek modes:
//
//   **Preview mode** — lightweight card with title, URL, and a "Peek" button.
//     - Shown first (after the initial hover delay).
//     - Fast to construct, low overhead.
//     - User can click "Peek" to expand to full glance.
//
//   **Glance mode** — full live WebContents preview via AstraGlanceView.
//     - Shown when the user clicks "Peek" on the preview card, or when
//       hover is held long enough (auto-expand, configurable).
//     - Uses the existing AstraGlanceViewController.
//
// Trigger sources:
//   - Sidebar item hover (AstraSidebarItemView).
//   - Tab strip tab hover (would be wired via a Chromium patch to
//     TabHoverCardController).
//   - Keyboard activation (e.g., pressing Space or a dedicated peek key).
//   - Link hover (future — URL glance mode).
//
// Hover timing model:
//   - Hover starts → start hover timer.
//   - After hover_show_delay → show preview card.
//   - If mouse leaves the source before the delay → cancel timer, no preview.
//   - If mouse leaves the source after preview is shown → hide after hide delay.
//   - "Peek" button click → expand preview to full glance.
//   - Hold hover long enough → auto-expand to peek mode (larger preview).
//
// Observer pattern:
//   - Observers are notified of peek state changes (shown, hidden, expanded,
//     collapsed).  UI components can observe to update their state (e.g.,
//     sidebar item highlight, focus indicators).
//
// Keyboard activation:
//   - The controller supports keyboard-triggered peek for accessibility.
//   - When a sidebar item or tab is focused, pressing a designated key
//     (e.g., Space, or Ctrl+Space) activates the peek preview.
//   - Additional keys: Escape to dismiss, Enter/Space to expand, M to mute.
//
// Chromium owner: TabHoverCardController
//   (chrome/browser/ui/tabs/tab_hover_card_controller.h).
// Chromium views: TabHoverCardBubbleView, TabHoverCardView
//   (chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h).
//
// TODO(astra): Integrate with Chromium's actual tab hover card system.
//   The real integration requires patching TabHoverCardController and
//   TabHoverCardBubbleView to add an Astra "Peek" button and delegate to
//   this controller.  File to patch:
//     chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.cc
//   Hook: after the hover card widget is created, add a "Peek" action
//   button that calls AstraTabHoverPeekController::ExpandToGlance().
//   Chromium owner: TabHoverCardController
//   (chrome/browser/ui/tabs/tab_hover_card_controller.h).
//
// TODO(astra): Consider adding a "peek on long hover" auto-expand mode
//   where holding hover on the preview card for an additional delay
//   automatically expands to full glance (like Edge's peek feature).
// =========================================================================

class AstraTabHoverPeekController
    : public views::WidgetObserver,
      public AstraTabHoverPreviewView::Delegate,
      public AstraTabHoverModelObserver {
 public:
  // The source that triggered the peek.  Used to determine anchor position
  // and for telemetry/metrics.
  enum class Source {
    kSidebarItem,   // Hovered a sidebar item.
    kTabStripTab,   // Hovered a tab in the tab strip (Chromium-patched).
    kKeyboard,      // Activated via keyboard.
    kLinkHover,     // Hovered a link (future).
  };

  // Current state of the peek controller.
  enum class State {
    kIdle,          // No active hover or preview.
    kPending,       // Hover started, waiting for the show delay.
    kPreviewShown,  // Lightweight preview card is visible.
    kPeekMode,      // Peek mode (expanded preview) is active.
    kGlanceShown,   // Full glance view is visible.
  };

  // Observer interface for peek state changes.
  // Implemented by UI components that need to react to peek state
  // (e.g., sidebar items, tab strip tabs, focus rings).
  class Observer : public base::CheckedObserver {
   public:
    // Called when the preview card is shown.
    // |source| indicates what triggered the peek.
    // |web_contents| is the tab being previewed.
    virtual void OnPeekPreviewShown(Source source,
                                    content::WebContents* web_contents) {}

    // Called when the preview card is hidden.
    virtual void OnPeekPreviewHidden() {}

    // Called when the peek expands to full glance mode.
    virtual void OnPeekExpandedToGlance() {}

    // Called when the peek collapses from glance back to preview.
    virtual void OnPeekCollapsedFromGlance() {}

    // Called when peek mode (expanded preview) is entered.
    virtual void OnPeekModeStarted() {}

    // Called when peek mode ends (returns to normal preview).
    virtual void OnPeekModeEnded() {}

    // Called when the preview image starts loading.
    virtual void OnPreviewImageLoading() {}

    // Called when the preview image finishes loading.
    virtual void OnPreviewImageLoaded() {}

    // Called when the close button on the preview is clicked.
    virtual void OnPeekCloseRequested() {}

    // Called when the mute toggle on the preview is clicked.
    virtual void OnPeekMuteToggled() {}

    // Called when the peek controller is destroyed.
    virtual void OnPeekControllerDestroyed() {}

   protected:
    ~Observer() override = default;
  };

  AstraTabHoverPeekController();
  ~AstraTabHoverPeekController() override;

  AstraTabHoverPeekController(const AstraTabHoverPeekController&) = delete;
  AstraTabHoverPeekController& operator=(const AstraTabHoverPeekController&) =
      delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Model access -------------------------------------------------------

  // Get the underlying model (read-only access for observers/tests).
  const AstraTabHoverModel& model() const { return model_; }
  AstraTabHoverModel& model() { return model_; }

  // -- Hover lifecycle ----------------------------------------------------

  // Called when the mouse enters a hover source (e.g., a sidebar item).
  // |anchor_view| is the view to anchor the preview bubble to.
  // |anchor_rect| is the rect within |anchor_view| to anchor to (can be
  // empty to use the whole view).
  // |web_contents| is the tab to preview (must outlive the peek).
  // |source| indicates what triggered the hover.
  // Starts the hover delay timer.  After the delay, the preview is shown.
  void OnHoverStarted(views::View* anchor_view,
                      const gfx::Rect& anchor_rect,
                      content::WebContents* web_contents,
                      Source source);

  // Called when the mouse leaves the hover source.
  // If the preview is pending (timer hasn't fired yet), cancels the timer.
  // If the preview is visible, hides it after the hide delay.
  // If glance is visible, hides it after the hide delay.
  void OnHoverEnded();

  // Called when the mouse moves within the hover source.
  // Updates the anchor position if needed.  Does not reset the hover timer.
  void OnHoverMoved(const gfx::Point& location);

  // -- Keyboard activation ------------------------------------------------

  // Activate peek preview via keyboard.
  // Shows the preview immediately (no hover delay) for the given tab.
  // |anchor_view| is the view to anchor to (e.g., the focused sidebar item).
  // |web_contents| is the tab to preview.
  // Returns true if peek was activated, false if it was already active.
  bool ActivatePeekFromKeyboard(views::View* anchor_view,
                                const gfx::Rect& anchor_rect,
                                content::WebContents* web_contents);

  // Dismiss peek preview via keyboard (e.g., Escape key).
  // Returns true if peek was dismissed, false if nothing was active.
  bool DismissPeekFromKeyboard();

  // Expand to peek mode via keyboard (e.g., Space or Enter key while peeked).
  // Returns true if expanded, false if not in a state that can expand.
  bool ExpandFromKeyboard();

  // Toggle mute via keyboard (e.g., M key).
  // Returns true if toggled, false if not in a valid state.
  bool ToggleMuteFromKeyboard();

  // -- Preview / glance control ------------------------------------------

  // Expand the current preview to peek mode (larger preview image).
  // No-op if no preview is currently shown.
  void EnterPeekMode();

  // Exit peek mode, returning to normal preview size.
  // No-op if not in peek mode.
  void ExitPeekMode();

  // Expand the current preview to a full glance view.
  // Called when the user clicks the "Peek" button on the preview card,
  // or when auto-expand triggers (long hover on the preview).
  // No-op if no preview is currently shown.
  void ExpandToGlance();

  // Collapse from glance back to preview mode.
  // No-op if glance is not shown.
  void CollapseToPreview();

  // Hide any visible preview or glance, and cancel any pending timers.
  void HideAll();

  // -- Preview image management --------------------------------------------

  // Start loading the preview image (sets loading state in model).
  // Notifies observers.
  void StartPreviewImageLoading();

  // Set the loaded preview image.
  // Updates model state and notifies observers.
  void SetPreviewImage(const gfx::ImageSkia& image,
                       const gfx::Size& dimensions);

  // Mark preview image loading as failed.
  void PreviewImageLoadFailed();

  // Clear the preview image.
  void ClearPreviewImage();

  // -- Media state management ----------------------------------------------

  // Update media state for the current tab.
  void UpdateMediaState(AstraTabHoverMediaState state);

  // Toggle mute state.
  void ToggleMute();

  // -- Accessors ----------------------------------------------------------

  State state() const { return state_; }
  Source source() const { return source_; }

  // The WebContents being previewed, or nullptr if no active peek.
  content::WebContents* hover_contents() { return hover_contents_; }
  const content::WebContents* hover_contents() const {
    return hover_contents_;
  }

  // The preview view widget, or nullptr if no preview is shown.
  views::Widget* preview_widget() { return preview_widget_; }

  // The glance controller, or nullptr if glance is not active.
  AstraGlanceViewController* glance_controller() {
    return glance_controller_.get();
  }

  // -- Configuration ------------------------------------------------------

  // Delay in milliseconds before showing the preview after hover start.
  // Matches Chromium's default tab hover card delay.
  static constexpr base::TimeDelta kHoverShowDelay = base::Milliseconds(500);

  // Delay in milliseconds before hiding the preview after hover end.
  static constexpr base::TimeDelta kHoverHideDelay = base::Milliseconds(300);

  // Delay in milliseconds before auto-expanding preview to glance on
  // continued hover.  Set to zero to disable auto-expand.
  // TODO(astra): Make this configurable via preferences or feature flags.
  static constexpr base::TimeDelta kAutoExpandDelay = base::Milliseconds(1500);

  // Delay in milliseconds before entering peek mode on continued hover.
  static constexpr base::TimeDelta kPeekActivationDelay =
      base::Milliseconds(800);

  // -- AstraTabHoverPreviewView::Delegate overrides -----------------------

  void OnPeekRequested() override;
  void OnCloseRequested() override;
  void OnPreviewViewDestroyed() override;
  void OnMuteToggled() override;

  // -- views::WidgetObserver overrides ------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;

  // -- AstraTabHoverModelObserver overrides --------------------------------

  void OnHoverShown() override;
  void OnHoverHidden() override;
  void OnPreviewImageChanged() override;
  void OnTabDataChanged() override;
  void OnPeekModeChanged() override;
  void OnHoverSettingsChanged() override;

 private:
  // Show the lightweight preview card.
  void ShowPreview();

  // Hide the preview card if it's visible.
  void HidePreview();

  // Show the full glance view (replaces preview).
  void ShowGlance();

  // Hide the glance view if it's visible.
  void HideGlance();

  // Timer callback: fires when the hover delay has elapsed.
  void OnHoverShowTimerFired();

  // Timer callback: fires when the hide delay has elapsed.
  void OnHideTimerFired();

  // Timer callback: fires when the auto-expand delay has elapsed.
  void OnAutoExpandTimerFired();

  // Timer callback: fires when the peek activation delay has elapsed.
  void OnPeekTimerFired();

  // Get the title text for the preview from the WebContents.
  std::u16string GetPreviewTitle() const;

  // Get the URL text for the preview from the WebContents.
  std::u16string GetPreviewUrl() const;

  // Look up BrowserView from the anchor view's widget hierarchy.
  // Used to construct the glance controller.
  // TODO(astra): Implement proper BrowserView lookup.
  //   views::Widget* top_widget = anchor_view_->GetWidget();
  //   BrowserView* browser_view =
  //       BrowserView::GetBrowserViewForNativeWindow(
  //           top_widget->GetNativeWindow());
  class BrowserView* GetBrowserViewFromAnchor() const;

  // Update the preview view from the current model state.
  void UpdateViewFromModel();

  // Update tab data in the model from the current WebContents.
  void UpdateTabDataFromWebContents();

  // Notify observers.
  void NotifyPreviewShown();
  void NotifyPreviewHidden();
  void NotifyExpandedToGlance();
  void NotifyCollapsedFromGlance();
  void NotifyPeekModeStarted();
  void NotifyPeekModeEnded();
  void NotifyPreviewImageLoading();
  void NotifyPreviewImageLoaded();
  void NotifyCloseRequested();
  void NotifyMuteToggled();

  // Current state.
  State state_ = State::kIdle;

  // Source that triggered the current hover/peek.
  Source source_ = Source::kSidebarItem;

  // The view to anchor the preview bubble to.
  // Not owned — must outlive the peek.
  raw_ptr<views::View> anchor_view_ = nullptr;

  // The anchor rect within |anchor_view_|.
  gfx::Rect anchor_rect_;

  // The WebContents being previewed.
  // Not owned — owned by TabStripModel or the glance controller (for URL mode).
  raw_ptr<content::WebContents> hover_contents_ = nullptr;

  // Timer for the initial hover delay before showing the preview.
  base::OneShotTimer hover_show_timer_;

  // Timer for the hide delay after hover ends.
  base::OneShotTimer hide_timer_;

  // Timer for auto-expanding preview to glance on long hover.
  base::OneShotTimer auto_expand_timer_;

  // Timer for entering peek mode on long hover.
  base::OneShotTimer peek_timer_;

  // The preview widget and view.
  // The widget is owned by the Views widget system.
  raw_ptr<views::Widget> preview_widget_ = nullptr;
  raw_ptr<AstraTabHoverPreviewView> preview_view_ = nullptr;

  // Glance view controller, created lazily when expanding to glance mode.
  std::unique_ptr<AstraGlanceViewController> glance_controller_;

  // The model — owns all state.
  AstraTabHoverModel model_;

  // Observers for peek state changes.
  base::ObserverList<Observer> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PEEK_CONTROLLER_H_
