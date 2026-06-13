#include "astra/ui/views/tab_hover/astra_tab_hover_peek_controller.h"

#include <memory>
#include <utility>

#include "astra/ui/views/glance/astra_glance_view_controller.h"
#include "astra/ui/views/tab_hover/astra_tab_hover_preview_view.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabHoverPeekController::AstraTabHoverPeekController() {
  // Observe our own model so that any external changes to model state
  // get reflected in the view (e.g., settings changed from a settings page).
  model_.AddObserver(this);
}

AstraTabHoverPeekController::~AstraTabHoverPeekController() {
  // Stop observing the model before it's destroyed.
  model_.RemoveObserver(this);

  // Notify observers before tearing down.
  for (Observer& observer : observers_) {
    observer.OnPeekControllerDestroyed();
  }

  HideAll();
}

// =========================================================================
// Observer management
// =========================================================================

void AstraTabHoverPeekController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraTabHoverPeekController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Hover lifecycle
// =========================================================================

void AstraTabHoverPeekController::OnHoverStarted(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    content::WebContents* web_contents,
    Source source) {
  if (!anchor_view || !web_contents) {
    return;
  }

  // If we're already showing something for this same source and same
  // WebContents, just update the position — no need to re-show.
  if (state_ != State::kIdle && hover_contents_ == web_contents) {
    anchor_view_ = anchor_view;
    anchor_rect_ = anchor_rect;
    // Restart the hide timer if it was running (hover re-entered).
    if (hide_timer_.IsRunning()) {
      hide_timer_.Stop();
    }
    return;
  }

  // Hide any existing preview/glance first.
  HideAll();

  // Set up new hover state.
  state_ = State::kPending;
  source_ = source;
  anchor_view_ = anchor_view;
  anchor_rect_ = anchor_rect;
  hover_contents_ = web_contents;

  // Update model with tab data.
  UpdateTabDataFromWebContents();

  // Start the hover delay timer using settings from the model.
  const auto& settings = model_.settings();
  hover_show_timer_.Start(FROM_HERE, settings.hover_show_delay, this,
                          &AstraTabHoverPeekController::OnHoverShowTimerFired);
}

void AstraTabHoverPeekController::OnHoverEnded() {
  // Cancel pending show timer.
  hover_show_timer_.Stop();

  // If we're in a shown state, start the hide delay timer.
  if (state_ == State::kPreviewShown || state_ == State::kPeekMode ||
      state_ == State::kGlanceShown) {
    const auto& settings = model_.settings();
    hide_timer_.Start(FROM_HERE, settings.hover_hide_delay, this,
                      &AstraTabHoverPeekController::OnHideTimerFired);
  }
}

void AstraTabHoverPeekController::OnHoverMoved(const gfx::Point& /*location*/) {
  // TODO(astra): Update anchor position if the preview is already shown.
  // For sidebar items, the anchor is the whole item so we don't need to
  // update on mouse movement within the item.  For tab strip tabs, we
  // might want to re-anchor if the mouse moves to a different tab.
  //
  // Chromium pattern: TabHoverCardController::UpdateCard or
  // TabHoverCardController::UpdateTargetTab handles tab-to-tab transitions.
}

// =========================================================================
// Keyboard activation
// =========================================================================

bool AstraTabHoverPeekController::ActivatePeekFromKeyboard(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    content::WebContents* web_contents) {
  if (!anchor_view || !web_contents) {
    return false;
  }

  // If already showing, just update.
  if ((state_ == State::kPreviewShown || state_ == State::kPeekMode) &&
      hover_contents_ == web_contents) {
    return false;
  }

  // Hide any existing preview/glance.
  HideAll();

  // Set up state.
  state_ = State::kPending;
  source_ = Source::kKeyboard;
  anchor_view_ = anchor_view;
  anchor_rect_ = anchor_rect;
  hover_contents_ = web_contents;

  // Update model with tab data.
  UpdateTabDataFromWebContents();

  // Show immediately — no hover delay for keyboard activation.
  ShowPreview();

  return true;
}

bool AstraTabHoverPeekController::DismissPeekFromKeyboard() {
  if (state_ == State::kIdle) {
    return false;
  }

  HideAll();
  return true;
}

bool AstraTabHoverPeekController::ExpandFromKeyboard() {
  if (state_ != State::kPreviewShown && state_ != State::kPeekMode) {
    return false;
  }

  ExpandToGlance();
  return true;
}

bool AstraTabHoverPeekController::ToggleMuteFromKeyboard() {
  if (state_ == State::kIdle || state_ == State::kPending) {
    return false;
  }

  ToggleMute();
  return true;
}

// =========================================================================
// Peek mode control
// =========================================================================

void AstraTabHoverPeekController::EnterPeekMode() {
  if (state_ != State::kPreviewShown) {
    return;
  }

  if (!model_.settings().enable_peek_mode) {
    return;
  }

  state_ = State::kPeekMode;
  model_.StartPeek();

  if (preview_view_) {
    preview_view_->SetPeekMode(true);
  }

  NotifyPeekModeStarted();
}

void AstraTabHoverPeekController::ExitPeekMode() {
  if (state_ != State::kPeekMode) {
    return;
  }

  state_ = State::kPreviewShown;
  model_.EndPeek();

  if (preview_view_) {
    preview_view_->SetPeekMode(false);
  }

  NotifyPeekModeEnded();
}

// =========================================================================
// Preview / glance control
// =========================================================================

void AstraTabHoverPeekController::ExpandToGlance() {
  if (state_ != State::kPreviewShown && state_ != State::kPeekMode) {
    return;
  }

  // Stop the auto-expand and peek timers since we're expanding now.
  auto_expand_timer_.Stop();
  peek_timer_.Stop();

  // Exit peek mode before expanding to glance.
  if (state_ == State::kPeekMode) {
    model_.EndPeek();
    NotifyPeekModeEnded();
  }

  // Hide the preview card first.
  HidePreview();

  // Show the glance view.
  ShowGlance();
}

void AstraTabHoverPeekController::CollapseToPreview() {
  if (state_ != State::kGlanceShown) {
    return;
  }

  // Hide the glance.
  HideGlance();

  // Re-show the preview card.
  ShowPreview();
}

void AstraTabHoverPeekController::HideAll() {
  // Stop all timers.
  hover_show_timer_.Stop();
  hide_timer_.Stop();
  auto_expand_timer_.Stop();
  peek_timer_.Stop();

  // Hide preview if visible.
  bool was_preview = state_ == State::kPreviewShown || state_ == State::kPeekMode;
  bool was_peek = state_ == State::kPeekMode;
  HidePreview();

  // Hide glance if visible.
  bool was_glance = state_ == State::kGlanceShown;
  HideGlance();

  // Reset state.
  state_ = State::kIdle;
  anchor_view_ = nullptr;
  anchor_rect_ = gfx::Rect();
  hover_contents_ = nullptr;

  // Reset model hover state.
  if (model_.is_hover_shown()) {
    model_.HideHover();
  }

  // Notify observers if we were showing something.
  if (was_peek) {
    NotifyPeekModeEnded();
  }
  if (was_preview || was_glance) {
    NotifyPreviewHidden();
  }
}

// =========================================================================
// Preview image management
// =========================================================================

void AstraTabHoverPeekController::StartPreviewImageLoading() {
  if (!model_.settings().show_preview_image) {
    return;
  }

  model_.SetPreviewImageLoadingState(
      AstraTabHoverImageLoadingState::kLoading);

  if (preview_view_) {
    preview_view_->SetThumbnailLoadingState(
        AstraTabHoverImageLoadingState::kLoading);
  }

  NotifyPreviewImageLoading();
}

void AstraTabHoverPeekController::SetPreviewImage(
    const gfx::ImageSkia& image,
    const gfx::Size& dimensions) {
  model_.SetPreviewImage(image, dimensions);

  if (preview_view_ && !image.isNull()) {
    preview_view_->SetThumbnail(image);
    preview_view_->SetThumbnailLoadingState(
        AstraTabHoverImageLoadingState::kLoaded);
  }

  NotifyPreviewImageLoaded();
}

void AstraTabHoverPeekController::PreviewImageLoadFailed() {
  model_.SetPreviewImageLoadingState(
      AstraTabHoverImageLoadingState::kFailed);

  if (preview_view_) {
    preview_view_->SetThumbnailLoadingState(
        AstraTabHoverImageLoadingState::kFailed);
  }
}

void AstraTabHoverPeekController::ClearPreviewImage() {
  model_.ClearPreviewImage();

  if (preview_view_) {
    preview_view_->SetThumbnailPlaceholder();
    preview_view_->SetThumbnailLoadingState(
        AstraTabHoverImageLoadingState::kNotLoaded);
  }
}

// =========================================================================
// Media state management
// =========================================================================

void AstraTabHoverPeekController::UpdateMediaState(
    AstraTabHoverMediaState state) {
  model_.SetMediaState(state);

  if (preview_view_) {
    preview_view_->SetMediaState(state);
  }
}

void AstraTabHoverPeekController::ToggleMute() {
  bool new_muted = !model_.tab_data().is_muted;
  model_.SetMuted(new_muted);

  if (preview_view_) {
    preview_view_->SetMuted(new_muted);
  }

  NotifyMuteToggled();
}

// =========================================================================
// Timer callbacks
// =========================================================================

void AstraTabHoverPeekController::OnHoverShowTimerFired() {
  DCHECK_EQ(state_, State::kPending);
  DCHECK(hover_contents_);
  DCHECK(anchor_view_);

  ShowPreview();
}

void AstraTabHoverPeekController::OnHideTimerFired() {
  // Hide everything when the hide delay expires.
  HideAll();
}

void AstraTabHoverPeekController::OnAutoExpandTimerFired() {
  DCHECK(state_ == State::kPreviewShown || state_ == State::kPeekMode);

  // Auto-expand to full glance.
  ExpandToGlance();

  // TODO(astra): Consider whether to auto-expand by default.
  // Edge's peek feature requires an explicit click, while some browsers
  // auto-expand on long hover.  We should align with product design.
}

void AstraTabHoverPeekController::OnPeekTimerFired() {
  DCHECK_EQ(state_, State::kPreviewShown);

  // Enter peek mode (expanded preview).
  EnterPeekMode();
}

// =========================================================================
// Show / hide helpers
// =========================================================================

void AstraTabHoverPeekController::ShowPreview() {
  DCHECK(anchor_view_);
  DCHECK(hover_contents_);
  DCHECK(!preview_widget_);

  // Check if hover cards are enabled.
  if (!model_.settings().show_tab_hover_cards) {
    state_ = State::kIdle;
    return;
  }

  // Create and show the preview bubble.
  preview_widget_ = AstraTabHoverPreviewView::ShowBubble(
      anchor_view_, anchor_rect_, hover_contents_, this);

  if (preview_widget_) {
    preview_view_ = static_cast<AstraTabHoverPreviewView*>(
        preview_widget_->widget_delegate());

    // Observe the widget so we know when it's destroyed.
    preview_widget_->AddObserver(this);

    state_ = State::kPreviewShown;

    // Update model hover state.
    model_.ShowHover();

    // Update view from model state.
    UpdateViewFromModel();

    // Start the peek activation timer if peek mode is enabled.
    if (model_.settings().enable_peek_mode) {
      peek_timer_.Start(FROM_HERE, model_.settings().peek_activation_delay,
                        this,
                        &AstraTabHoverPeekController::OnPeekTimerFired);
    }

    // Start the auto-expand timer if enabled.
    // TODO(astra): Make auto-expand configurable.  For now, it's always on
    // with a longer delay to give users time to read the preview.
    if (kAutoExpandDelay > base::TimeDelta()) {
      auto_expand_timer_.Start(
          FROM_HERE, kAutoExpandDelay, this,
          &AstraTabHoverPeekController::OnAutoExpandTimerFired);
    }

    NotifyPreviewShown();
  } else {
    // Failed to create preview — reset state.
    state_ = State::kIdle;
  }
}

void AstraTabHoverPeekController::HidePreview() {
  if (preview_widget_) {
    // Stop observing before closing to avoid reentrancy.
    preview_widget_->RemoveObserver(this);
    preview_widget_->Close();
    preview_widget_ = nullptr;
    preview_view_ = nullptr;
  }
}

void AstraTabHoverPeekController::ShowGlance() {
  DCHECK(anchor_view_);
  DCHECK(hover_contents_);
  DCHECK(!glance_controller_);

  // Get the BrowserView from the anchor view's widget.
  // TODO(astra): Get BrowserView from the anchor view hierarchy.
  // The glance controller needs a BrowserView to access the tab strip model
  // and profile.
  //
  // For the skeleton, we create a placeholder glance controller with
  // a null BrowserView — it will gracefully no-op.
  //
  // Chromium pattern: BrowserView is accessible via
  // BrowserView::GetBrowserViewForNativeWindow() or from the widget.

  class BrowserView* browser_view = GetBrowserViewFromAnchor();

  glance_controller_ = std::make_unique<AstraGlanceViewController>(
      browser_view  // BrowserView* — will be resolved from anchor_view_
                    // in a real build.
  );

  if (glance_controller_) {
    // Compute the anchor rect for the glance view.
    // The glance is typically larger than the preview and positioned
    // relative to the same anchor.
    gfx::Rect glance_anchor = anchor_rect_;
    if (glance_anchor.IsEmpty()) {
      glance_anchor = gfx::Rect(anchor_view_->size());
    }

    glance_controller_->ShowGlance(hover_contents_, glance_anchor);

    if (glance_controller_->IsGlanceVisible()) {
      state_ = State::kGlanceShown;
      NotifyExpandedToGlance();
    } else {
      // Glance failed to show — clean up.
      glance_controller_.reset();
      state_ = State::kIdle;
    }
  }
}

void AstraTabHoverPeekController::HideGlance() {
  if (glance_controller_) {
    glance_controller_->HideGlance();
    glance_controller_.reset();
  }
}

// =========================================================================
// AstraTabHoverPreviewView::Delegate overrides
// =========================================================================

void AstraTabHoverPeekController::OnPeekRequested() {
  // User clicked the "Peek" button on the preview card.
  ExpandToGlance();
}

void AstraTabHoverPeekController::OnCloseRequested() {
  // User clicked the close button on the preview card.
  NotifyCloseRequested();
  HideAll();
}

void AstraTabHoverPeekController::OnPreviewViewDestroyed() {
  // The preview view is being destroyed.  Clear our pointers.
  preview_view_ = nullptr;
  // Don't reset preview_widget_ here — OnWidgetDestroying handles that.
}

void AstraTabHoverPeekController::OnMuteToggled() {
  // User clicked the mute button on the preview card.
  ToggleMute();
}

// =========================================================================
// views::WidgetObserver overrides
// =========================================================================

void AstraTabHoverPeekController::OnWidgetDestroying(views::Widget* widget) {
  if (widget == preview_widget_) {
    widget->RemoveObserver(this);
    preview_widget_ = nullptr;
    preview_view_ = nullptr;

    // If we were in preview or peek state, go back to idle.
    if (state_ == State::kPreviewShown || state_ == State::kPeekMode) {
      bool was_peek = state_ == State::kPeekMode;
      state_ = State::kIdle;
      model_.HideHover();

      if (was_peek) {
        NotifyPeekModeEnded();
      }
      NotifyPreviewHidden();
    }
  }
}

// =========================================================================
// AstraTabHoverModelObserver overrides
// =========================================================================

void AstraTabHoverPeekController::OnHoverShown() {
  // Model says hover was shown — view should already be updated.
}

void AstraTabHoverPeekController::OnHoverHidden() {
  // Model says hover was hidden — view should already be updated.
}

void AstraTabHoverPeekController::OnPreviewImageChanged() {
  // Model image state changed — update the view if it exists.
  if (preview_view_) {
    const auto& img_state = model_.preview_image_state();
    preview_view_->SetThumbnailLoadingState(img_state.loading_state);
  }
}

void AstraTabHoverPeekController::OnTabDataChanged() {
  // Model tab data changed — update the view if it exists.
  if (preview_view_) {
    const auto& tab_data = model_.tab_data();
    preview_view_->SetTitleText(tab_data.title);
    preview_view_->SetDomainText(
        AstraTabHoverModel::FormatDomainFromUrl(tab_data.url));
    preview_view_->SetMediaState(tab_data.media_state);
    preview_view_->SetMuted(tab_data.is_muted);
    preview_view_->SetTabIndex(tab_data.tab_index);
  }
}

void AstraTabHoverPeekController::OnPeekModeChanged() {
  // Model peek mode changed — update the view if it exists.
  if (preview_view_) {
    preview_view_->SetPeekMode(model_.peek_state().is_peeking);
    preview_view_->SetPreviewSize(model_.peek_state().peek_size);
  }
}

void AstraTabHoverPeekController::OnHoverSettingsChanged() {
  // Settings changed — update the view if it exists.
  if (preview_view_) {
    UpdateViewFromModel();
  }
}

// =========================================================================
// Helpers
// =========================================================================

std::u16string AstraTabHoverPeekController::GetPreviewTitle() const {
  if (!hover_contents_) {
    return std::u16string();
  }
  std::u16string title = hover_contents_->GetTitle();
  if (title.empty()) {
    // Fallback: show the URL host.
    GURL url = hover_contents_->GetVisibleURL();
    if (url.is_valid()) {
      return base::UTF8ToUTF16(url.host());
    }
  }
  return title;
}

std::u16string AstraTabHoverPeekController::GetPreviewUrl() const {
  if (!hover_contents_) {
    return std::u16string();
  }
  GURL url = hover_contents_->GetVisibleURL();
  return base::UTF8ToUTF16(url.spec());
}

BrowserView* AstraTabHoverPeekController::GetBrowserViewFromAnchor() const {
  // TODO(astra): Implement proper BrowserView lookup from the anchor view.
  //
  // The glance controller needs a BrowserView to access the tab strip model
  // and profile.  We can get it via:
  //
  //   views::Widget* top_widget = anchor_view_->GetWidget();
  //   if (!top_widget) return nullptr;
  //   BrowserView* browser_view =
  //       BrowserView::GetBrowserViewForNativeWindow(
  //           top_widget->GetNativeWindow());
  //   return browser_view;
  //
  // Alternatively, we could store a BrowserView* on the controller at
  // construction time, which is simpler if the controller is owned by
  // the BrowserView.
  //
  // Chromium owner: BrowserView (chrome/browser/ui/views/frame/browser_view.h)
  // Patch point: No patch needed — BrowserView is accessible via
  //   GetBrowserViewForNativeWindow() from a native window.
  //
  // For the skeleton, return nullptr — the glance controller will no-op.
  return nullptr;
}

void AstraTabHoverPeekController::UpdateViewFromModel() {
  if (!preview_view_) {
    return;
  }

  preview_view_->UpdateFromModel(model_);
}

void AstraTabHoverPeekController::UpdateTabDataFromWebContents() {
  if (!hover_contents_) {
    return;
  }

  AstraTabHoverTabData tab_data;
  tab_data.title = GetPreviewTitle();
  tab_data.url = hover_contents_->GetVisibleURL();
  // TODO(astra): Get actual favicon from FaviconService.
  tab_data.is_media_playing = hover_contents_->IsCurrentlyAudible();
  tab_data.media_state =
      hover_contents_->IsCurrentlyAudible()
          ? AstraTabHoverMediaState::kPlaying
          : AstraTabHoverMediaState::kNone;
  // TODO(astra): Get actual tab index from TabStripModel.
  tab_data.tab_index = -1;

  model_.SetTabData(tab_data);
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraTabHoverPeekController::NotifyPreviewShown() {
  for (Observer& observer : observers_) {
    observer.OnPeekPreviewShown(source_, hover_contents_);
  }
}

void AstraTabHoverPeekController::NotifyPreviewHidden() {
  for (Observer& observer : observers_) {
    observer.OnPeekPreviewHidden();
  }
}

void AstraTabHoverPeekController::NotifyExpandedToGlance() {
  for (Observer& observer : observers_) {
    observer.OnPeekExpandedToGlance();
  }
}

void AstraTabHoverPeekController::NotifyCollapsedFromGlance() {
  for (Observer& observer : observers_) {
    observer.OnPeekCollapsedFromGlance();
  }
}

void AstraTabHoverPeekController::NotifyPeekModeStarted() {
  for (Observer& observer : observers_) {
    observer.OnPeekModeStarted();
  }
}

void AstraTabHoverPeekController::NotifyPeekModeEnded() {
  for (Observer& observer : observers_) {
    observer.OnPeekModeEnded();
  }
}

void AstraTabHoverPeekController::NotifyPreviewImageLoading() {
  for (Observer& observer : observers_) {
    observer.OnPreviewImageLoading();
  }
}

void AstraTabHoverPeekController::NotifyPreviewImageLoaded() {
  for (Observer& observer : observers_) {
    observer.OnPreviewImageLoaded();
  }
}

void AstraTabHoverPeekController::NotifyCloseRequested() {
  for (Observer& observer : observers_) {
    observer.OnPeekCloseRequested();
  }
}

void AstraTabHoverPeekController::NotifyMuteToggled() {
  for (Observer& observer : observers_) {
    observer.OnPeekMuteToggled();
  }
}

}  // namespace astra
