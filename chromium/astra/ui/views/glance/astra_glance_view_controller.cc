#include "astra/ui/views/glance/astra_glance_view_controller.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Helper to convert a pointer to a string identifier.
// This is only valid within a single browser process session.
// TODO(astra): Replace with a proper stable tab identifier.
template <typename T>
std::string PointerToId(T* ptr) {
  std::ostringstream oss;
  oss << static_cast<const void*>(ptr);
  return oss.str();
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraGlanceViewController::AstraGlanceViewController(BrowserView* browser_view)
    : browser_view_(browser_view) {}

AstraGlanceViewController::~AstraGlanceViewController() {
  // Notify observers of shutdown before tearing down.
  NotifyNewObserversShutdown();

  // Ensure clean teardown.
  if (IsGlanceVisible()) {
    HideGlance();
  }

  // Cancel any pending timers.
  show_delay_timer_.Stop();
  auto_hide_timer_.Stop();
  hide_delay_timer_.Stop();
}

// =========================================================================
// Show / hide
// =========================================================================

void AstraGlanceViewController::ShowGlance(content::WebContents* for_contents,
                                            const gfx::Rect& anchor,
                                            Source source) {
  // Cancel any pending delayed show.
  show_delay_timer_.Stop();

  ShowGlanceInternal(for_contents, anchor, source);
}

void AstraGlanceViewController::ShowGlanceForURL(const GURL& url,
                                                  const gfx::Rect& anchor,
                                                  Source source) {
  // Cancel any pending delayed show.
  show_delay_timer_.Stop();

  ShowGlanceForURLInternal(url, anchor, source);
}

void AstraGlanceViewController::ShowGlanceForURLDelayed(const GURL& url,
                                                        const gfx::Rect& anchor,
                                                        base::TimeDelta delay,
                                                        Source source) {
  // If already visible, just update — don't delay.
  if (IsGlanceVisible()) {
    ShowGlanceForURL(url, anchor, source);
    return;
  }

  // Store pending show data.
  pending_url_ = url;
  pending_anchor_ = anchor;
  pending_source_ = source;
  pending_is_url_mode_ = true;
  pending_contents_ = nullptr;

  // Start the delay timer.
  show_delay_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AstraGlanceViewController::OnShowDelayTimerFired,
                     base::Unretained(this)));
}

void AstraGlanceViewController::ShowGlanceDelayed(content::WebContents* for_contents,
                                                   const gfx::Rect& anchor,
                                                   base::TimeDelta delay,
                                                   Source source) {
  // If already visible, just update — don't delay.
  if (IsGlanceVisible()) {
    ShowGlance(for_contents, anchor, source);
    return;
  }

  // Store pending show data.
  pending_contents_ = for_contents;
  pending_anchor_ = anchor;
  pending_source_ = source;
  pending_is_url_mode_ = false;
  pending_url_ = GURL();

  // Start the delay timer.
  show_delay_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AstraGlanceViewController::OnShowDelayTimerFired,
                     base::Unretained(this)));
}

void AstraGlanceViewController::OnShowDelayTimerFired() {
  if (pending_is_url_mode_) {
    ShowGlanceForURLInternal(pending_url_, pending_anchor_, pending_source_);
  } else if (pending_contents_) {
    ShowGlanceInternal(pending_contents_, pending_anchor_, pending_source_);
  }

  // Clear pending state.
  pending_url_ = GURL();
  pending_contents_ = nullptr;
  pending_anchor_ = gfx::Rect();
  pending_source_ = Source::kUnknown;
  pending_is_url_mode_ = false;
}

void AstraGlanceViewController::ShowGlanceInternal(content::WebContents* for_contents,
                                                    const gfx::Rect& anchor,
                                                    Source source) {
  if (!browser_view_ || !for_contents) {
    return;
  }

  // If already showing a glance, hide it first.
  if (IsGlanceVisible()) {
    HideGlance();
  }

  mode_ = Mode::kTab;
  source_ = source;
  glance_contents_ = for_contents;

  // Determine source tab ID — for tab glance from the sidebar, the source
  // is the tab itself.  For link hover, it's the tab containing the link.
  // TODO(astra): Determine what "source tab" means for tab glance.
  //   It might be the tab from which the user triggered the peek.
  source_tab_id_ = std::string();

  // Write glance metadata to the WebContents.
  WriteGlanceMetadata(for_contents, source_tab_id_);

  // Start observing loading state.
  StartObservingContents(for_contents);

  // Get the anchor view for the bubble.
  // TODO(astra): Use the actual sidebar item view as the anchor, not the
  // browser view itself.  For tab glance from the sidebar, the anchor view
  // should be the sidebar item corresponding to the tab.
  views::View* anchor_view = GetAnchorView();
  if (!anchor_view) {
    // Clean up.
    ClearGlanceMetadata(for_contents);
    StopObservingContents();
    glance_contents_ = nullptr;
    mode_ = Mode::kNone;
    return;
  }

  // Create and show the glance bubble.
  glance_widget_ = AstraGlanceView::ShowBubble(anchor_view, anchor, this);
  if (glance_widget_) {
    glance_view_ = static_cast<AstraGlanceView*>(
        glance_widget_->widget_delegate());
  }

  // Attach the WebContents to the glance view.
  if (glance_view_) {
    glance_view_->SetWebContents(for_contents);
    UpdateViewFromContents();
  }

  // TODO(astra): Handle the "showing same WebContents in two places" problem.
  // See class-level TODO(astra).  Currently we just set the pointer and let the
  // view handle the embedding.

  // Update tab index tracking.
  // TODO(astra): Determine the actual tab index from TabStripModel.
  if (browser_view_ && browser_view_->browser()) {
    TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
    if (tab_strip) {
      tab_index_ = tab_strip->GetIndexOfWebContents(for_contents);
    }
  }

  NotifyGlanceShown();
  NotifyNewObserversGlanceShown(tab_index_);
}

void AstraGlanceViewController::ShowGlanceForURLInternal(const GURL& url,
                                                          const gfx::Rect& anchor,
                                                          Source source) {
  if (!browser_view_ || !url.is_valid()) {
    return;
  }

  // If already showing a glance, hide it first.
  if (IsGlanceVisible()) {
    HideGlance();
  }

  mode_ = Mode::kUrl;
  source_ = source;

  // Create a temporary WebContents for URL preview.
  owned_glance_contents_ = CreateTemporaryWebContents(url);
  if (!owned_glance_contents_) {
    mode_ = Mode::kNone;
    return;
  }

  glance_contents_ = owned_glance_contents_.get();

  // For URL glance, source_tab_id identifies the tab that triggered the
  // preview (e.g., the tab containing the hovered link).
  source_tab_id_ = std::string();  // TODO(astra): Get from active tab.
  if (delegate_) {
    content::WebContents* active = delegate_->GetActiveWebContents();
    if (active) {
      source_tab_id_ = GetSourceTabId(active);
    }
  }

  // Write glance metadata.
  WriteGlanceMetadata(glance_contents_, source_tab_id_);

  // Start observing loading state.
  StartObservingContents(glance_contents_);

  // Get the anchor view.
  views::View* anchor_view = GetAnchorView();
  if (!anchor_view) {
    // Clean up.
    ClearGlanceMetadata(glance_contents_);
    StopObservingContents();
    owned_glance_contents_.reset();
    glance_contents_ = nullptr;
    mode_ = Mode::kNone;
    return;
  }

  // Create and show the glance bubble.
  glance_widget_ = AstraGlanceView::ShowBubble(anchor_view, anchor, this);
  if (glance_widget_) {
    glance_view_ = static_cast<AstraGlanceView*>(
        glance_widget_->widget_delegate());
  }

  // Attach the WebContents to the glance view.
  if (glance_view_) {
    glance_view_->SetWebContents(glance_contents_);
    // Set loading state since we're about to load.
    glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
    UpdateViewFromContents();
  }

  // Start loading the URL.
  // TODO(astra): Use the correct load URL parameters including transition type,
  // referrer, etc.  For a preview, we might want to mark this as a preview
  // navigation so it doesn't affect history.
  //
  // Chromium pattern: NavigationController::LoadURLParams
  // (content/public/browser/navigation_controller.h).
  //
  // TODO(astra): Consider whether URL glance should add to the back/forward
  // history of the temporary WebContents.  It probably should for the mini
  // toolbar back/forward buttons to work, but it shouldn't pollute the main
  // browsing session's history.
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PAGE_TRANSITION_AUTO_BOOKMARK;
  owned_glance_contents_->GetController().LoadURLWithParams(params);

  NotifyGlanceShown();
  NotifyNewObserversGlanceShown(tab_index_);
}

void AstraGlanceViewController::HideGlance() {
  // Cancel any pending delayed show.
  show_delay_timer_.Stop();

  // Cancel auto-hide timer.
  auto_hide_timer_.Stop();

  // Cancel hide delay timer.
  hide_delay_timer_.Stop();

  if (!IsGlanceVisible()) {
    return;
  }

  // Stop observing before tearing down.
  StopObservingContents();

  // Clear glance metadata from the WebContents.
  if (glance_contents_) {
    ClearGlanceMetadata(glance_contents_);
  }

  // Play exit animation, then close the widget.
  // TODO(astra): Actually wait for the animation to complete before closing.
  //   For now, we close immediately.
  if (glance_view_) {
    glance_view_->PlayExitAnimation(base::DoNothing());
  }

  // Close the widget.  This triggers OnWidgetDestroying on the glance view,
  // which detaches the WebContents view.  Then the widget deletes the view,
  // which calls OnGlanceViewDestroyed() on us.
  //
  // We do it this way (Close then clear pointers in the callback) to be
  // consistent with Views' ownership model — the widget owns the view delegate.
  if (glance_widget_) {
    glance_widget_->Close();
    // The callbacks from closing will clear our raw_ptrs.
    // But we need to make sure we don't try to use them after close.
    glance_widget_ = nullptr;
    glance_view_ = nullptr;
  }

  // Handle WebContents based on mode.
  if (mode_ == Mode::kTab) {
    // Tab glance: restore the WebContents to its normal tab position.
    if (glance_contents_) {
      RestoreTabWebContents(glance_contents_);
    }
    glance_contents_ = nullptr;
  } else if (mode_ == Mode::kUrl) {
    // URL glance: destroy the temporary WebContents.
    // Unless it was promoted — PromoteToTab() releases ownership.
    if (owned_glance_contents_) {
      owned_glance_contents_.reset();
    }
    glance_contents_ = nullptr;
  }

  Mode old_mode = mode_;
  int old_tab_index = tab_index_;
  mode_ = Mode::kNone;
  source_ = Source::kUnknown;
  source_tab_id_.clear();
  tab_index_ = -1;

  if (old_mode != Mode::kNone) {
    NotifyGlanceHidden();
    NotifyNewObserversGlanceHidden();
  }
}

bool AstraGlanceViewController::IsGlanceVisible() const {
  return mode_ != Mode::kNone && glance_contents_ != nullptr;
}

// =========================================================================
// Display mode
// =========================================================================

void AstraGlanceViewController::SetDisplayMode(AstraGlanceView::DisplayMode mode) {
  if (!glance_view_) {
    return;
  }
  glance_view_->SetDisplayMode(mode);

  bool is_expanded = (mode == AstraGlanceView::DisplayMode::kExpanded);
  if (is_expanded) {
    NotifyGlanceExpanded();
  }
  NotifyNewObserversGlanceExpanded(is_expanded);
}

void AstraGlanceViewController::ToggleDisplayMode() {
  if (!glance_view_) {
    return;
  }
  glance_view_->ToggleDisplayMode();
}

// =========================================================================
// Promotion to full tab
// =========================================================================

bool AstraGlanceViewController::PromoteToTab() {
  if (mode_ != Mode::kUrl || !owned_glance_contents_ || !browser_view_) {
    // Tab glance is already a tab — no promotion needed.
    // Or we're not in a valid state for promotion.
    return false;
  }

  Browser* browser = browser_view_->browser();
  if (!browser) {
    return false;
  }

  TabStripModel* tab_strip = browser->tab_strip_model();
  if (!tab_strip) {
    return false;
  }

  // Clear glance metadata before adding to tab strip — once it's a real
  // tab, it should not be marked as a glance tab.
  ClearGlanceMetadata(owned_glance_contents_.get());

  // Add the WebContents to the tab strip.
  // TODO(astra): Use the correct TabStripModel insertion method.
  // The exact API depends on whether we want to add next to the current tab,
  // at the end, etc.
  //
  // Chromium API: TabStripModel::AddWebContents() or
  // TabStripModel::AppendWebContents().
  // We release ownership to TabStripModel.
  //
  // Reference: chrome/browser/ui/tabs/tab_strip_model.h
  //
  // TODO(astra): Choose the correct insertion index and add type.
  // For a "promote from glance" action, inserting next to the active tab
  // or at the end of the tab strip are reasonable options.
  int add_types = AddTabTypes::ADD_ACTIVE;  // TODO(astra): correct add type
  tab_strip->AppendWebContents(std::move(owned_glance_contents_),
                               /*foreground=*/true);
  // owned_glance_contents_ is now null (moved from).

  NotifyGlancePinnedAsTab();

  // Close the glance view.
  HideGlance();

  return true;
}

// =========================================================================
// Observer management
// =========================================================================

void AstraGlanceViewController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraGlanceViewController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AstraGlanceViewController::NotifyGlanceShown() {
  for (auto& observer : observers_) {
    observer.OnGlanceShown();
  }
}

void AstraGlanceViewController::NotifyGlanceHidden() {
  for (auto& observer : observers_) {
    observer.OnGlanceHidden();
  }
}

void AstraGlanceViewController::NotifyGlanceExpanded() {
  for (auto& observer : observers_) {
    observer.OnGlanceExpanded();
  }
}

void AstraGlanceViewController::NotifyGlancePinnedAsTab() {
  for (auto& observer : observers_) {
    observer.OnGlancePinnedAsTab();
  }
}

// =========================================================================
// Auto-hide
// =========================================================================

void AstraGlanceViewController::OnMouseEnteredGlance() {
  // Cancel any pending auto-hide.
  auto_hide_timer_.Stop();
}

void AstraGlanceViewController::OnMouseLeftGlance() {
  if (!auto_hide_on_mouse_leave_) {
    return;
  }

  // Start the auto-hide timer (hysteresis).
  if (!auto_hide_timer_.IsRunning()) {
    auto_hide_timer_.Start(
        FROM_HERE, auto_hide_delay_,
        base::BindOnce(&AstraGlanceViewController::OnAutoHideTimerFired,
                       base::Unretained(this)));
  }
}

void AstraGlanceViewController::OnAutoHideTimerFired() {
  // Mouse was gone long enough — hide the glance.
  HideGlance();
}

// =========================================================================
// AstraGlanceView::Delegate implementation
// =========================================================================

void AstraGlanceViewController::OnGlanceCloseRequested() {
  HideGlance();
}

void AstraGlanceViewController::OnGlancePromoteToTab() {
  // If we have a delegate, let it handle promotion.
  // Otherwise, do it ourselves via TabStripModel.
  if (mode_ == Mode::kUrl) {
    PromoteToTab();
  } else if (mode_ == Mode::kTab && delegate_ && glance_contents_) {
    // For tab glance, "promote" means focus the tab and close the glance.
    // TODO(astra): Activate the tab in TabStripModel and close the glance.
    HideGlance();
  }
}

void AstraGlanceViewController::OnGlanceViewDestroyed() {
  // The glance view is being destroyed — clear our raw pointers.
  // This can happen if the widget is closed by some other means
  // (e.g., clicking outside, Escape key, OS-level close).
  glance_view_ = nullptr;
  glance_widget_ = nullptr;

  // If we still think glance is visible, clean up the rest of state.
  if (mode_ != Mode::kNone) {
    StopObservingContents();

    // Clear metadata.
    if (glance_contents_) {
      ClearGlanceMetadata(glance_contents_);
    }

    // Handle WebContents.
    if (mode_ == Mode::kTab) {
      if (glance_contents_) {
        RestoreTabWebContents(glance_contents_);
      }
      glance_contents_ = nullptr;
    } else if (mode_ == Mode::kUrl) {
      owned_glance_contents_.reset();
      glance_contents_ = nullptr;
    }

    mode_ = Mode::kNone;
    source_ = Source::kUnknown;
    source_tab_id_.clear();
    tab_index_ = -1;

    NotifyGlanceHidden();
    NotifyNewObserversGlanceHidden();
  }
}

void AstraGlanceViewController::OnGlanceAddToFavorites() {
  if (!glance_contents_) {
    return;
  }

  if (delegate_) {
    delegate_->ToggleFavorite(glance_contents_);
    // Update the view state.
    if (glance_view_) {
      AstraTabFeatures* features =
          AstraTabFeatures::FromWebContents(glance_contents_);
      if (features) {
        glance_view_->SetIsFavorite(features->is_favorite());
      }
    }
  }
}

void AstraGlanceViewController::OnGlanceCopyURL() {
  if (!glance_contents_) {
    return;
  }

  GURL url = glance_contents_->GetVisibleURL();

  if (delegate_) {
    delegate_->CopyURLToClipboard(url);
  }
  // TODO(astra): If no delegate, copy to clipboard directly using
  //   ui::Clipboard.  Chromium pattern: ui::Clipboard::WriteText().
}

void AstraGlanceViewController::OnGlancePinTab() {
  if (!glance_contents_) {
    return;
  }

  if (mode_ == Mode::kUrl) {
    // For URL glance, "pin" means promote to tab first, then pin.
    // TODO(astra): Or we could just toggle the pinned flag on the
    //   temporary WebContents metadata.  But it doesn't make sense to
    //   pin a temporary glance tab.  Let's promote first.
    PromoteToTab();
    // After promotion, the WebContents is in TabStripModel.
    // TODO(astra): Pin it in TabStripModel.
    return;
  }

  if (delegate_) {
    delegate_->TogglePinned(glance_contents_);
    // Update the view state.
    if (glance_view_) {
      AstraTabFeatures* features =
          AstraTabFeatures::FromWebContents(glance_contents_);
      if (features) {
        glance_view_->SetIsPinned(features->sidebar_pinned());
      }
    }
  }
}

void AstraGlanceViewController::OnGlanceCloseTab() {
  if (!glance_contents_) {
    return;
  }

  // For URL glance: just hide (destroys the temporary WebContents).
  if (mode_ == Mode::kUrl) {
    HideGlance();
    return;
  }

  // For tab glance: close the actual tab via the delegate.
  if (delegate_) {
    // Save the pointer before hiding — HideGlance clears it.
    content::WebContents* contents = glance_contents_;
    HideGlance();
    delegate_->CloseTab(contents);
  }
}

void AstraGlanceViewController::OnGlanceToggleExpanded() {
  // The view already toggled; just notify observers.
  NotifyGlanceExpanded();
}

// =========================================================================
// content::WebContentsObserver implementation
// =========================================================================

void AstraGlanceViewController::DidStartLoading() {
  if (glance_view_) {
    glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
  }
}

void AstraGlanceViewController::DidStopLoading() {
  if (glance_view_) {
    // Only set to loaded if we're not in an error state.
    // DidFailLoad will set the error state, which takes precedence.
    // TODO(astra): We need a better way to track whether the last load
    //   succeeded or failed.  For now, assume success when loading stops.
    if (glance_view_->loading_state() != AstraGlanceView::LoadingState::kError) {
      glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
    }
  }
}

void AstraGlanceViewController::PrimaryPageChanged(content::Page& page) {
  // Primary page changed — update title, URL, etc.
  UpdateViewFromContents();
}

void AstraGlanceViewController::DidFailLoad(content::RenderFrameHost* render_frame_host,
                                             const GURL& validated_url,
                                             int error_code) {
  // Only handle main frame failures.
  // TODO(astra): Check if this is the main frame.
  //   render_frame_host->IsInPrimaryMainFrame() in newer Chromium versions.
  if (glance_view_) {
    glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kError);
    // TODO(astra): Set a user-friendly error message based on error_code.
    //   Chromium pattern: net::ErrorToString() or localized error strings.
    glance_view_->SetErrorMessage(u"Failed to load preview");
  }
}

void AstraGlanceViewController::TitleWasSet(content::NavigationEntry* entry) {
  if (glance_view_ && entry) {
    glance_view_->SetTitleText(entry->GetTitle());
  }
}

// =========================================================================
// WebContents observation helpers
// =========================================================================

void AstraGlanceViewController::StartObservingContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  // Observe the new WebContents.
  content::WebContentsObserver::Observe(web_contents);
}

void AstraGlanceViewController::StopObservingContents() {
  if (content::WebContentsObserver::web_contents()) {
    content::WebContentsObserver::Observe(nullptr);
  }
}

void AstraGlanceViewController::UpdateViewFromContents() {
  if (!glance_view_ || !glance_contents_) {
    return;
  }

  // Title.
  std::u16string title = glance_contents_->GetTitle();
  if (title.empty()) {
    // Fall back to the URL if no title.
    title = base::UTF8ToUTF16(glance_contents_->GetVisibleURL().spec());
  }
  glance_view_->SetTitleText(title);

  // URL.
  glance_view_->SetURLText(
      base::UTF8ToUTF16(glance_contents_->GetVisibleURL().spec()));

  // Favicon.
  // TODO(astra): Get the favicon from FaviconService and set it on the view.
  //   Chromium owner: chrome/browser/favicon/favicon_service.h.
  //   Astra could cache favicons on AstraTabFeatures for quick access.

  // Security state.
  // TODO(astra): Determine security state from the WebContents.
  //   Chromium owner: content::SSLStatus, security_state::SecurityLevel.
  //   Use NavigationEntry::GetSSL() or web_contents->GetSecurityLevel().

  // Loading state.
  if (glance_contents_->IsLoading()) {
    glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
  } else {
    glance_view_->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
  }

  // Favorite state.
  AstraTabFeatures* features =
      AstraTabFeatures::FromWebContents(glance_contents_);
  if (features) {
    glance_view_->SetIsFavorite(features->is_favorite());
    glance_view_->SetIsPinned(features->sidebar_pinned());
  }
}

// =========================================================================
// Private helpers
// =========================================================================

content::BrowserContext* AstraGlanceViewController::GetBrowserContext() {
  if (!browser_view_ || !browser_view_->browser()) {
    return nullptr;
  }
  return browser_view_->browser()->profile();
}

views::View* AstraGlanceViewController::GetAnchorView() {
  // TODO(astra): Return the appropriate anchor view for the glance bubble.
  //
  // For sidebar-triggered glance: the sidebar item view that was hovered.
  // For link hover glance: the WebContents view (or a specific rect).
  //
  // For now, use the browser view itself as a fallback anchor.
  // In practice, the anchor should be more specific.
  //
  // Chromium pattern: BubbleDialogDelegateView anchors to a views::View*.
  // We can also use SetAnchorRect() for anchoring to a specific rect.
  return browser_view_;
}

std::unique_ptr<content::WebContents>
AstraGlanceViewController::CreateTemporaryWebContents(const GURL& url) {
  // TODO(astra): Create a real WebContents using Chromium's WebContents::Create.
  //
  // The correct way to create a WebContents in Chrome:
  //   content::WebContents::CreateParams params(browser_context);
  //   params.initially_hidden = true;
  //   std::unique_ptr<content::WebContents> web_contents =
  //       content::WebContents::Create(params);
  //
  // Or use chrome-specific helpers like:
  //   TabHelpers::CreateWebContents() — but that's tied to tab strip.
  //
  // For a glance/peek WebContents, we probably want:
  //   - No tab strip affiliation initially.
  //   - Its own WebContentsDelegate (or shared with the browser).
  //   - Initially hidden / off-screen.
  //   - The same profile/browser context as the parent browser.
  //
  // Chromium subsystem: content::WebContents (content/public/browser/).
  // Reference pattern: side panel WebContents creation in
  //   chrome/browser/ui/views/side_panel/side_panel_content_view.cc
  //
  // TODO(astra): Set up a WebContentsDelegate for the glance WebContents.
  // The delegate handles navigation actions, context menus, etc.
  // For glance, we may want to restrict some behaviors (e.g., no popups,
  // no downloads, no permission prompts) or route them to the main browser.
  //
  // Chromium owner: content::WebContentsDelegate
  //   (content/public/browser/web_contents_delegate.h).

  content::BrowserContext* browser_context = GetBrowserContext();
  if (!browser_context) {
    return nullptr;
  }

  content::WebContents::CreateParams params(browser_context);
  params.initially_hidden = true;
  // TODO(astra): Set initial size to match the glance view's content area.
  params.initial_size = gfx::Size(
      AstraGlanceView::kExpandedSize.width(),
      AstraGlanceView::kExpandedSize.height() - AstraGlanceView::kHeaderHeight
          - AstraGlanceView::kActionBarHeight
          - AstraGlanceView::kStatusBarHeight);

  // TODO(astra): This is a placeholder.  The actual WebContents::Create
  // requires linking against content/browser.  In a real Chromium build,
  // this would work.  For the skeleton, we return nullptr to indicate
  // "not yet implemented" and let the view show a placeholder.
  //
  // Uncomment the following line in the real Chromium build:
  // return content::WebContents::Create(params);
  return nullptr;
}

void AstraGlanceViewController::WriteGlanceMetadata(
    content::WebContents* web_contents,
    const std::string& source_tab_id) {
  if (!web_contents) {
    return;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);
  features->set_is_glance_tab(true);
  features->set_glance_source_tab_id(source_tab_id);
}

void AstraGlanceViewController::ClearGlanceMetadata(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (features) {
    features->set_is_glance_tab(false);
    features->set_glance_source_tab_id(std::string());
  }
}

void AstraGlanceViewController::RestoreTabWebContents(
    content::WebContents* web_contents) {
  // TODO(astra): Restore the WebContents view to its normal position in the
  // tab strip's content area.
  //
  // This is the reverse of whatever we did to show the WebContents in the
  // glance bubble.  The exact mechanism depends on the embedding approach:
  //
  // Option 1 — view reparenting:
  //   If we reparented the WebContentsView from the contents container to
  //   the glance bubble, we need to reparent it back.
  //
  // Option 2 — dual view (if possible):
  //   If WebContents can be shown in two places simultaneously, we just
  //   hide or remove the glance view copy.
  //
  // Option 3 — preview WebContents:
  //   If we created a separate preview WebContents, we just destroy it.
  //   (But then it wouldn't be "the same tab" — it's a preview.)
  //
  // Chromium subsystem: BrowserView + TabStripModel active tab handling.
  // Reference: chrome/browser/ui/views/frame/browser_view.cc —
  //   TabStripModelObserver::OnActiveTabChanged() swaps the WebContents
  //   view in the contents container.
  //
  // Patch point: May need a small patch to BrowserView or contents container
  // to support temporarily detaching/replacing a WebContents view.

  // Placeholder: nothing to do in the skeleton.
  // In a real implementation, this would ensure the WebContents view is
  // back in its normal position in the tab strip's content area.
}

std::string AstraGlanceViewController::GetSourceTabId(
    content::WebContents* web_contents) {
  // TODO(astra): Use a proper stable identifier.
  // For now, use the pointer address as a session-scoped ID.
  return PointerToId(web_contents);
}

// =========================================================================
// Pinning (glance pinned open)
// =========================================================================

void AstraGlanceViewController::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;

  // If pinned, cancel any pending auto-hide.
  if (pinned) {
    auto_hide_timer_.Stop();
  }

  // Update the view if visible.
  if (glance_view_) {
    // TODO(astra): Update view pin button state.
    // For now, the view doesn't have a pin button for the glance itself.
  }

  NotifyGlancePinnedChanged(pinned);
  NotifyNewObserversGlancePinned(pinned);
}

void AstraGlanceViewController::TogglePinned() {
  SetPinned(!is_pinned_);
}

// =========================================================================
// Size presets
// =========================================================================

bool AstraGlanceViewController::ApplySizePreset(SizePreset preset) {
  if (!glance_widget_) {
    return false;
  }

  gfx::Size new_size = GetPresetSize(preset);
  gfx::Rect bounds = glance_widget_->GetWindowBoundsInScreen();
  if (bounds.size() == new_size) {
    return false;
  }

  bounds.set_size(new_size);
  glance_widget_->SetBounds(bounds);

  // Remember the size if that feature is enabled.
  if (GetRememberSize()) {
    last_size_ = new_size;
  }

  NotifyGlanceResized(new_size);
  NotifyNewObserversGlanceSizeChanged(new_size);
  return true;
}

gfx::Size AstraGlanceViewController::GetPresetSize(SizePreset preset) const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    switch (preset) {
      case SizePreset::kSmall:
        return kFallbackSmallSize;
      case SizePreset::kMedium:
        return kFallbackExpandedSize;
      case SizePreset::kLarge:
        return kFallbackLargeSize;
    }
    return kFallbackExpandedSize;
  }

  switch (preset) {
    case SizePreset::kSmall:
      return gfx::Size(
          prefs->GetInteger(prefs::kPrefGlanceSmallWidth),
          prefs->GetInteger(prefs::kPrefGlanceSmallHeight));
    case SizePreset::kMedium:
      return gfx::Size(
          prefs->GetInteger(prefs::kPrefGlanceExpandedWidth),
          prefs->GetInteger(prefs::kPrefGlanceExpandedHeight));
    case SizePreset::kLarge:
      return gfx::Size(
          prefs->GetInteger(prefs::kPrefGlanceLargeWidth),
          prefs->GetInteger(prefs::kPrefGlanceLargeHeight));
  }
  return kFallbackExpandedSize;
}

AstraGlanceViewController::SizePreset
AstraGlanceViewController::GetCurrentSizePreset() const {
  if (!glance_widget_) {
    return SizePreset::kMedium;
  }

  gfx::Size current_size = glance_widget_->GetWindowBoundsInScreen().size();
  gfx::Size small_size = GetPresetSize(SizePreset::kSmall);
  gfx::Size medium_size = GetPresetSize(SizePreset::kMedium);
  gfx::Size large_size = GetPresetSize(SizePreset::kLarge);

  // Find the closest preset by area difference.
  int small_diff = std::abs(current_size.GetArea() - small_size.GetArea());
  int medium_diff = std::abs(current_size.GetArea() - medium_size.GetArea());
  int large_diff = std::abs(current_size.GetArea() - large_size.GetArea());

  if (small_diff <= medium_diff && small_diff <= large_diff) {
    return SizePreset::kSmall;
  } else if (medium_diff <= large_diff) {
    return SizePreset::kMedium;
  } else {
    return SizePreset::kLarge;
  }
}

// =========================================================================
// Position presets
// =========================================================================

void AstraGlanceViewController::SetPosition(Position position) {
  if (position_ == position) {
    return;
  }
  position_ = position;

  // TODO(astra): Update the bubble arrow position if the view is visible.
  // This would require recreating the bubble or updating its arrow.
  // For now, we just track the state.

  NotifyGlanceSourceChanged(Source::kUnknown);  // Not actually source change
  // TODO(astra): Add OnGlancePositionChanged observer method if needed.
}

void AstraGlanceViewController::CyclePosition() {
  Position next;
  switch (position_) {
    case Position::kLeft:
      next = Position::kTop;
      break;
    case Position::kTop:
      next = Position::kRight;
      break;
    case Position::kRight:
      next = Position::kBottom;
      break;
    case Position::kBottom:
      next = Position::kLeft;
      break;
  }
  SetPosition(next);
}

// static
std::string AstraGlanceViewController::PositionToString(Position position) {
  switch (position) {
    case Position::kLeft:
      return "left";
    case Position::kRight:
      return "right";
    case Position::kTop:
      return "top";
    case Position::kBottom:
      return "bottom";
  }
  return "right";
}

// static
AstraGlanceViewController::Position
AstraGlanceViewController::StringToPosition(const std::string& str) {
  if (str == "left") return Position::kLeft;
  if (str == "right") return Position::kRight;
  if (str == "top") return Position::kTop;
  if (str == "bottom") return Position::kBottom;
  return Position::kRight;  // Default
}

// =========================================================================
// Recent glance history
// =========================================================================

void AstraGlanceViewController::AddToRecentGlances(const GURL& url) {
  if (!url.is_valid()) {
    return;
  }

  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return;
  }

  const base::Value::List& existing = prefs->GetList(prefs::kPrefGlanceRecentUrls);
  base::Value::List new_list;
  std::string url_spec = url.spec();

  // Add the new URL at the front.
  new_list.Append(url_spec);

  int max_count = prefs->GetInteger(prefs::kPrefGlanceRecentMaxCount);

  // Copy existing entries, skipping duplicates.
  for (const auto& item : existing) {
    if (item.is_string() && item.GetString() != url_spec) {
      new_list.Append(item.GetString());
    }
    if (static_cast<int>(new_list.size()) >= max_count) {
      break;
    }
  }

  prefs->SetList(prefs::kPrefGlanceRecentUrls, std::move(new_list));
}

std::vector<GURL> AstraGlanceViewController::GetRecentGlances() const {
  std::vector<GURL> result;

  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return result;
  }

  const base::Value::List& list = prefs->GetList(prefs::kPrefGlanceRecentUrls);
  for (const auto& item : list) {
    if (item.is_string()) {
      GURL url(item.GetString());
      if (url.is_valid()) {
        result.push_back(std::move(url));
      }
    }
  }

  return result;
}

void AstraGlanceViewController::ClearRecentGlances() {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return;
  }
  prefs->SetList(prefs::kPrefGlanceRecentUrls, base::Value::List());
}

// =========================================================================
// Presentation settings — getters and setters
// =========================================================================
//
// All settings are persisted via PrefService.
// Setters notify observers via OnGlanceSettingsChanged().

AstraGlanceView::DisplayMode
AstraGlanceViewController::GetDefaultDisplayMode() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return AstraGlanceView::DisplayMode::kExpanded;
  }
  std::string mode = prefs->GetString(prefs::kPrefGlanceDefaultDisplayMode);
  return (mode == "compact") ? AstraGlanceView::DisplayMode::kCompact
                              : AstraGlanceView::DisplayMode::kExpanded;
}

void AstraGlanceViewController::SetDefaultDisplayMode(
    AstraGlanceView::DisplayMode mode) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  std::string mode_str = (mode == AstraGlanceView::DisplayMode::kCompact)
                             ? "compact"
                             : "expanded";
  if (prefs->GetString(prefs::kPrefGlanceDefaultDisplayMode) == mode_str) {
    return;
  }
  prefs->SetString(prefs::kPrefGlanceDefaultDisplayMode, mode_str);
  NotifyGlanceSettingsChanged();
}

base::TimeDelta AstraGlanceViewController::GetShowDelay() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return kDefaultShowDelay;
  }
  return base::Milliseconds(prefs->GetInteger(prefs::kPrefGlanceShowDelayMs));
}

void AstraGlanceViewController::SetShowDelay(base::TimeDelta delay) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  int ms = static_cast<int>(delay.InMilliseconds());
  ms = std::clamp(ms, 0, 5000);  // Clamp to reasonable range
  if (prefs->GetInteger(prefs::kPrefGlanceShowDelayMs) == ms) {
    return;
  }
  prefs->SetInteger(prefs::kPrefGlanceShowDelayMs, ms);
  NotifyGlanceSettingsChanged();
}

base::TimeDelta AstraGlanceViewController::GetAutoHideDelay() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return auto_hide_delay_;
  }
  return base::Milliseconds(
      prefs->GetInteger(prefs::kPrefGlanceAutoHideDelayMs));
}

void AstraGlanceViewController::SetAutoHideDelay(base::TimeDelta delay) {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    auto_hide_delay_ = delay;
    return;
  }

  int ms = static_cast<int>(delay.InMilliseconds());
  ms = std::clamp(ms, 0, 10000);  // Clamp to reasonable range
  if (prefs->GetInteger(prefs::kPrefGlanceAutoHideDelayMs) == ms) {
    return;
  }
  prefs->SetInteger(prefs::kPrefGlanceAutoHideDelayMs, ms);
  NotifyGlanceSettingsChanged();
}

gfx::Size AstraGlanceViewController::GetCompactSize() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return kFallbackCompactSize;
  }
  return gfx::Size(
      prefs->GetInteger(prefs::kPrefGlanceCompactWidth),
      prefs->GetInteger(prefs::kPrefGlanceCompactHeight));
}

void AstraGlanceViewController::SetCompactSize(const gfx::Size& size) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  int width = std::clamp(size.width(), 200, 1000);
  int height = std::clamp(size.height(), 150, 800);

  if (prefs->GetInteger(prefs::kPrefGlanceCompactWidth) == width &&
      prefs->GetInteger(prefs::kPrefGlanceCompactHeight) == height) {
    return;
  }
  prefs->SetInteger(prefs::kPrefGlanceCompactWidth, width);
  prefs->SetInteger(prefs::kPrefGlanceCompactHeight, height);
  NotifyGlanceSettingsChanged();
}

gfx::Size AstraGlanceViewController::GetExpandedSize() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return kFallbackExpandedSize;
  }
  return gfx::Size(
      prefs->GetInteger(prefs::kPrefGlanceExpandedWidth),
      prefs->GetInteger(prefs::kPrefGlanceExpandedHeight));
}

void AstraGlanceViewController::SetExpandedSize(const gfx::Size& size) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  int width = std::clamp(size.width(), 200, 1200);
  int height = std::clamp(size.height(), 200, 900);

  if (prefs->GetInteger(prefs::kPrefGlanceExpandedWidth) == width &&
      prefs->GetInteger(prefs::kPrefGlanceExpandedHeight) == height) {
    return;
  }
  prefs->SetInteger(prefs::kPrefGlanceExpandedWidth, width);
  prefs->SetInteger(prefs::kPrefGlanceExpandedHeight, height);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetShowActionBar() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceShowActionBar);
}

void AstraGlanceViewController::SetShowActionBar(bool show) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceShowActionBar) == show) return;
  prefs->SetBoolean(prefs::kPrefGlanceShowActionBar, show);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetShowStatusBar() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceShowStatusBar);
}

void AstraGlanceViewController::SetShowStatusBar(bool show) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceShowStatusBar) == show) return;
  prefs->SetBoolean(prefs::kPrefGlanceShowStatusBar, show);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetShowResizeHandle() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceShowResizeHandle);
}

void AstraGlanceViewController::SetShowResizeHandle(bool show) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceShowResizeHandle) == show) return;
  prefs->SetBoolean(prefs::kPrefGlanceShowResizeHandle, show);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetHoverPeekEnabled() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled);
}

void AstraGlanceViewController::SetHoverPeekEnabled(bool enabled) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled) == enabled) return;
  prefs->SetBoolean(prefs::kPrefGlanceHoverPeekEnabled, enabled);
  NotifyGlanceSettingsChanged();
}

AstraGlanceViewController::Position
AstraGlanceViewController::GetDefaultPosition() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return Position::kRight;
  }
  return StringToPosition(
      prefs->GetString(prefs::kPrefGlanceDefaultPosition));
}

void AstraGlanceViewController::SetDefaultPosition(Position position) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  std::string pos_str = PositionToString(position);
  if (prefs->GetString(prefs::kPrefGlanceDefaultPosition) == pos_str) {
    return;
  }
  prefs->SetString(prefs::kPrefGlanceDefaultPosition, pos_str);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetPinnedByDefault() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return false;
  }
  return prefs->GetBoolean(prefs::kPrefGlancePinnedByDefault);
}

void AstraGlanceViewController::SetPinnedByDefault(bool pinned) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlancePinnedByDefault) == pinned) return;
  prefs->SetBoolean(prefs::kPrefGlancePinnedByDefault, pinned);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetRememberSize() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceRememberSize);
}

void AstraGlanceViewController::SetRememberSize(bool remember) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceRememberSize) == remember) return;
  prefs->SetBoolean(prefs::kPrefGlanceRememberSize, remember);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetAnimationsEnabled() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceAnimationsEnabled);
}

void AstraGlanceViewController::SetAnimationsEnabled(bool enabled) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceAnimationsEnabled) == enabled) return;
  prefs->SetBoolean(prefs::kPrefGlanceAnimationsEnabled, enabled);
  NotifyGlanceSettingsChanged();
}

bool AstraGlanceViewController::GetShowSettingsButton() const {
  PrefService* prefs = GetPrefService();
  if (!prefs) {
    return true;
  }
  return prefs->GetBoolean(prefs::kPrefGlanceShowSettingsButton);
}

void AstraGlanceViewController::SetShowSettingsButton(bool show) {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;
  if (prefs->GetBoolean(prefs::kPrefGlanceShowSettingsButton) == show) return;
  prefs->SetBoolean(prefs::kPrefGlanceShowSettingsButton, show);
  NotifyGlanceSettingsChanged();
}

// =========================================================================
// Utility methods
// =========================================================================

PrefService* AstraGlanceViewController::GetPrefService() const {
  if (!browser_view_ || !browser_view_->browser()) {
    return nullptr;
  }
  Profile* profile = browser_view_->browser()->profile();
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

void AstraGlanceViewController::ResetSettingsToDefaults() {
  PrefService* prefs = GetPrefService();
  if (!prefs) return;

  prefs->ClearPref(prefs::kPrefGlanceDefaultDisplayMode);
  prefs->ClearPref(prefs::kPrefGlanceShowDelayMs);
  prefs->ClearPref(prefs::kPrefGlanceAutoHideDelayMs);
  prefs->ClearPref(prefs::kPrefGlanceCompactWidth);
  prefs->ClearPref(prefs::kPrefGlanceCompactHeight);
  prefs->ClearPref(prefs::kPrefGlanceExpandedWidth);
  prefs->ClearPref(prefs::kPrefGlanceExpandedHeight);
  prefs->ClearPref(prefs::kPrefGlanceShowActionBar);
  prefs->ClearPref(prefs::kPrefGlanceShowStatusBar);
  prefs->ClearPref(prefs::kPrefGlanceShowResizeHandle);
  prefs->ClearPref(prefs::kPrefGlanceHoverPeekEnabled);
  prefs->ClearPref(prefs::kPrefGlanceDefaultPosition);
  prefs->ClearPref(prefs::kPrefGlanceSizePreset);
  prefs->ClearPref(prefs::kPrefGlancePinnedByDefault);
  prefs->ClearPref(prefs::kPrefGlanceRememberSize);
  prefs->ClearPref(prefs::kPrefGlanceRecentMaxCount);
  prefs->ClearPref(prefs::kPrefGlanceShowSettingsButton);
  prefs->ClearPref(prefs::kPrefGlanceAnimationsEnabled);
  prefs->ClearPref(prefs::kPrefGlanceSmallWidth);
  prefs->ClearPref(prefs::kPrefGlanceSmallHeight);
  prefs->ClearPref(prefs::kPrefGlanceLargeWidth);
  prefs->ClearPref(prefs::kPrefGlanceLargeHeight);

  NotifyGlanceSettingsChanged();
}

// =========================================================================
// Additional notification helpers
// =========================================================================

void AstraGlanceViewController::NotifyGlanceModeChanged(Mode mode) {
  for (auto& observer : observers_) {
    observer.OnGlanceModeChanged(mode);
  }
}

void AstraGlanceViewController::NotifyGlanceResized(const gfx::Size& new_size) {
  for (auto& observer : observers_) {
    observer.OnGlanceResized(new_size);
  }
}

void AstraGlanceViewController::NotifyGlanceSourceChanged(Source source) {
  for (auto& observer : observers_) {
    observer.OnGlanceSourceChanged(source);
  }
}

void AstraGlanceViewController::NotifyGlancePinnedChanged(bool pinned) {
  for (auto& observer : observers_) {
    observer.OnGlancePinnedChanged(pinned);
  }
}

void AstraGlanceViewController::NotifyGlanceSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnGlanceSettingsChanged();
  }
}

// =========================================================================
// Tab-index-based show/hide API
// =========================================================================

void AstraGlanceViewController::ShowGlance(int tab_index,
                                            const gfx::Point& anchor_point) {
  // TODO(astra): Look up the WebContents for the tab at tab_index from
  //   TabStripModel and show glance for it.  For now, we update state and
  //   notify observers.
  //
  // Chromium subsystem: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h).
  // Patch point: The controller needs access to Browser's TabStripModel.

  tab_index_ = tab_index;
  anchor_position_ = anchor_point;

  if (browser_view_ && browser_view_->browser()) {
    TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
    if (tab_strip && tab_index >= 0 && tab_index < tab_strip->count()) {
      content::WebContents* contents =
          tab_strip->GetWebContentsAt(tab_index);
      if (contents) {
        gfx::Rect anchor_rect(anchor_point, gfx::Size());
        ShowGlanceInternal(contents, anchor_rect, Source::kSidebarHover);
        return;
      }
    }
  }

  // Fallback: just notify observers that glance was shown.
  NotifyNewObserversGlanceShown(tab_index);
}

void AstraGlanceViewController::HideGlanceAfterDelay() {
  if (is_pinned_) {
    // Pinned glances don't auto-dismiss.
    return;
  }

  hide_delay_timer_.Start(
      FROM_HERE, base::Milliseconds(dismiss_delay_ms_),
      base::BindOnce(&AstraGlanceViewController::OnHideDelayTimerFired,
                     base::Unretained(this)));
}

void AstraGlanceViewController::CancelHide() {
  hide_delay_timer_.Stop();
}

bool AstraGlanceViewController::IsVisible() const {
  return IsGlanceVisible();
}

bool AstraGlanceViewController::IsAnimating() const {
  return is_animating_;
}

int AstraGlanceViewController::GetTabIndex() const {
  return tab_index_;
}

void AstraGlanceViewController::OnHideDelayTimerFired() {
  HideGlance();
}

// =========================================================================
// Content type
// =========================================================================

void AstraGlanceViewController::SetContentType(AstraGlanceContentType type) {
  if (content_type_ == type) {
    return;
  }
  content_type_ = type;

  // Update the view if visible.
  if (glance_view_) {
    glance_view_->SetContentType(type);
  }

  NotifyNewObserversGlanceContentTypeChanged(type);
}

AstraGlanceContentType AstraGlanceViewController::GetContentType() const {
  return content_type_;
}

// =========================================================================
// Sizing
// =========================================================================

void AstraGlanceViewController::SetSize(const gfx::Size& size) {
  // Clamp to min/max.
  int width = std::clamp(size.width(), min_size_.width(), max_size_.width());
  int height = std::clamp(size.height(), min_size_.height(), max_size_.height());
  gfx::Size clamped_size(width, height);

  if (glance_widget_) {
    gfx::Rect bounds = glance_widget_->GetWindowBoundsInScreen();
    if (bounds.size() == clamped_size) {
      return;
    }
    bounds.set_size(clamped_size);
    glance_widget_->SetBounds(bounds);
  }

  last_size_ = clamped_size;

  NotifyNewObserversGlanceSizeChanged(clamped_size);
}

gfx::Size AstraGlanceViewController::GetSize() const {
  if (glance_widget_) {
    return glance_widget_->GetWindowBoundsInScreen().size();
  }
  return last_size_;
}

void AstraGlanceViewController::SetSizePreset(AstraGlanceSize preset) {
  if (size_preset_ == preset) {
    return;
  }
  size_preset_ = preset;

  gfx::Size new_size;
  switch (preset) {
    case AstraGlanceSize::kSmall:
      new_size = gfx::Size(320, 240);
      break;
    case AstraGlanceSize::kMedium:
      new_size = gfx::Size(560, 420);
      break;
    case AstraGlanceSize::kLarge:
      new_size = gfx::Size(720, 540);
      break;
    case AstraGlanceSize::kExtraLarge:
      new_size = gfx::Size(900, 680);
      break;
  }

  SetSize(new_size);
}

AstraGlanceSize AstraGlanceViewController::GetSizePreset() const {
  return size_preset_;
}

void AstraGlanceViewController::SetMinSize(const gfx::Size& size) {
  min_size_ = size;
  // If current size is below new minimum, resize up.
  if (glance_widget_) {
    gfx::Size current = GetSize();
    if (current.width() < min_size_.width() ||
        current.height() < min_size_.height()) {
      SetSize(gfx::Size(
          std::max(current.width(), min_size_.width()),
          std::max(current.height(), min_size_.height())));
    }
  }
}

gfx::Size AstraGlanceViewController::GetMinSize() const {
  return min_size_;
}

void AstraGlanceViewController::SetMaxSize(const gfx::Size& size) {
  max_size_ = size;
  // If current size is above new maximum, resize down.
  if (glance_widget_) {
    gfx::Size current = GetSize();
    if (current.width() > max_size_.width() ||
        current.height() > max_size_.height()) {
      SetSize(gfx::Size(
          std::min(current.width(), max_size_.width()),
          std::min(current.height(), max_size_.height())));
    }
  }
}

gfx::Size AstraGlanceViewController::GetMaxSize() const {
  return max_size_;
}

void AstraGlanceViewController::SetMaintainAspectRatio(bool maintain) {
  maintain_aspect_ratio_ = maintain;
}

bool AstraGlanceViewController::GetMaintainAspectRatio() const {
  return maintain_aspect_ratio_;
}

// =========================================================================
// Positioning
// =========================================================================

void AstraGlanceViewController::SetAnchorPosition(const gfx::Point& point) {
  anchor_position_ = point;
  // TODO(astra): Reposition the glance bubble when the anchor point changes.
  //   This would involve updating the bubble's anchor rect.
  //   Chromium pattern: BubbleDialogDelegateView::SetAnchorRect().
}

gfx::Point AstraGlanceViewController::GetAnchorPosition() const {
  return anchor_position_;
}

void AstraGlanceViewController::SetPlacement(AstraGlancePlacement placement) {
  if (placement_ == placement) {
    return;
  }
  placement_ = placement;
  // TODO(astra): Update the bubble arrow position when placement changes.
  //   For now, we just track the state.
}

AstraGlancePlacement AstraGlanceViewController::GetPlacement() const {
  return placement_;
}

void AstraGlanceViewController::SetOffset(int offset_px) {
  offset_px_ = offset_px;
  // TODO(astra): Apply the offset to the bubble position.
  //   The offset controls the distance between the anchor and the bubble.
}

int AstraGlanceViewController::GetOffset() const {
  return offset_px_;
}

// =========================================================================
// Trigger settings
// =========================================================================

void AstraGlanceViewController::SetTriggerMode(AstraGlanceTriggerMode mode) {
  trigger_mode_ = mode;
}

AstraGlanceTriggerMode AstraGlanceViewController::GetTriggerMode() const {
  return trigger_mode_;
}

void AstraGlanceViewController::SetHoverDelay(int delay_ms) {
  hover_delay_ms_ = std::max(0, delay_ms);
}

int AstraGlanceViewController::GetHoverDelay() const {
  return hover_delay_ms_;
}

void AstraGlanceViewController::SetDismissDelay(int delay_ms) {
  dismiss_delay_ms_ = std::max(0, delay_ms);
}

int AstraGlanceViewController::GetDismissDelay() const {
  return dismiss_delay_ms_;
}

// =========================================================================
// Hover trigger toggles
// =========================================================================

void AstraGlanceViewController::SetShowOnTabHover(bool show) {
  show_on_tab_hover_ = show;
}

bool AstraGlanceViewController::GetShowOnTabHover() const {
  return show_on_tab_hover_;
}

void AstraGlanceViewController::SetShowOnBookmarkHover(bool show) {
  show_on_bookmark_hover_ = show;
}

bool AstraGlanceViewController::GetShowOnBookmarkHover() const {
  return show_on_bookmark_hover_;
}

void AstraGlanceViewController::SetShowOnHistoryHover(bool show) {
  show_on_history_hover_ = show;
}

bool AstraGlanceViewController::GetShowOnHistoryHover() const {
  return show_on_history_hover_;
}

// =========================================================================
// Interaction actions
// =========================================================================

void AstraGlanceViewController::PinGlance() {
  SetPinned(true);
}

void AstraGlanceViewController::UnpinGlance() {
  SetPinned(false);
}

void AstraGlanceViewController::ExpandGlance() {
  if (!IsExpanded()) {
    SetExpanded(true);
  }
}

void AstraGlanceViewController::CollapseGlance() {
  if (IsExpanded()) {
    SetExpanded(false);
  }
}

bool AstraGlanceViewController::IsExpanded() const {
  if (glance_view_) {
    return glance_view_->IsExpanded();
  }
  // Default: expanded mode is the default.
  return true;
}

void AstraGlanceViewController::NavigateToTab() {
  if (tab_index_ < 0 || !browser_view_ || !browser_view_->browser()) {
    return;
  }
  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip || tab_index_ >= tab_strip->count()) {
    return;
  }
  // Activate the tab and close the glance.
  tab_strip->ActivateTabAt(tab_index_);
  HideGlance();
}

void AstraGlanceViewController::OpenInNewTab() {
  if (!delegate_ || !glance_contents_) {
    return;
  }
  delegate_->OpenURLInNewTab(glance_contents_->GetVisibleURL());
}

void AstraGlanceViewController::CloseTab() {
  OnGlanceCloseTab();
}

// =========================================================================
// AstraGlanceObserver management
// =========================================================================

void AstraGlanceViewController::AddObserver(AstraGlanceObserver* observer) {
  glance_observers_.AddObserver(observer);
}

void AstraGlanceViewController::RemoveObserver(AstraGlanceObserver* observer) {
  glance_observers_.RemoveObserver(observer);
}

// =========================================================================
// New observer notification helpers
// =========================================================================

void AstraGlanceViewController::NotifyNewObserversGlanceShown(int tab_index) {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceShown(this, tab_index);
  }
}

void AstraGlanceViewController::NotifyNewObserversGlanceHidden() {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceHidden(this);
  }
}

void AstraGlanceViewController::NotifyNewObserversGlancePinned(bool pinned) {
  for (auto& observer : glance_observers_) {
    observer.OnGlancePinned(this, pinned);
  }
}

void AstraGlanceViewController::NotifyNewObserversGlanceExpanded(bool expanded) {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceExpanded(this, expanded);
  }
}

void AstraGlanceViewController::NotifyNewObserversGlanceSizeChanged(
    const gfx::Size& new_size) {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceSizeChanged(this, new_size);
  }
}

void AstraGlanceViewController::NotifyNewObserversGlanceContentTypeChanged(
    AstraGlanceContentType type) {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceContentTypeChanged(this, type);
  }
}

void AstraGlanceViewController::NotifyNewObserversShutdown() {
  for (auto& observer : glance_observers_) {
    observer.OnGlanceViewControllerShutdown(this);
  }
}

}  // namespace astra
