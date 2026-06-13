#include "astra/ui/views/astra_browser_view.h"

#include <memory>
#include <utility>

#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_screenshot_service.h"
#include "astra/browser/astra_theme_service.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/sidebar/astra_sidebar_view.h"
#include "astra/ui/views/split_view/astra_split_view_controller.h"
#include "astra/ui/views/glance/astra_glance_view_controller.h"
#include "astra/ui/views/workspace/astra_workspace_overview_controller.h"
#include "astra/ui/views/workspace/astra_workspace_import_export_dialog.h"
#include "astra/ui/views/profiles/astra_profile_menu_controller.h"
#include "astra/ui/views/screenshot/astra_screenshot_capture_bubble.h"
#include "astra/ui/views/screenshot/astra_screenshot_region_overlay.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_controller.h"
#include "astra/ui/views/newtab/astra_new_tab_bubble.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/open_url_params.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "url/gurl.h"

namespace astra {

AstraBrowserView::AstraBrowserView(BrowserView* browser_view)
    : browser_view_(browser_view) {}

AstraBrowserView::~AstraBrowserView() {
  // Ensure clean teardown even if Uninstall() was not called.
  Uninstall();
}

// -- Observers -------------------------------------------------------------

void AstraBrowserView::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraBrowserView::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// -- Install / Uninstall ---------------------------------------------------

void AstraBrowserView::Install() {
  if (!browser_view_ || sidebar_view_) {
    return;
  }

  // TODO(astra): Insert the sidebar into BrowserView's layout at the correct
  // position in the view hierarchy. The correct insertion point depends on
  // the BrowserView layout manager:
  //
  //   chrome/browser/ui/views/frame/browser_view_layout.h
  //   (BrowserViewLayoutManager or NonClientFrameView-dependent layout)
  //
  // The sidebar should live at the same level as the contents container
  // (browser_view_->contents_container() or similar), to the left of the
  // main web content area. The tab strip and toolbar span the full width.
  //
  // Proper approach:
  //   1. Get the root view or contents_container's parent
  //   2. Insert sidebar at index 0 (leftmost)
  //   3. Update the layout manager to reserve sidebar width
  //
  // For the overlay skeleton, we use AddChildView as a placeholder. The
  // real integration requires a tiny Chromium patch to BrowserView that
  // delegates layout coordination into AstraBrowserView.
  //
  // Chromium owner: BrowserView (chrome/browser/ui/views/frame/browser_view.h)
  // Patch file: chrome/browser/ui/views/frame/browser_view.cc.patch

  sidebar_view_ =
      browser_view_->AddChildView(std::make_unique<AstraSidebarView>(
          browser_view_->browser()));

  // Wire up TabStripModelObserver — the sidebar observes Chromium's
  // TabStripModel directly for reactive updates. This is the primary
  // update path, replacing manual UpdateFromModel() calls.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  if (browser_view_->browser() &&
      browser_view_->browser()->tab_strip_model()) {
    sidebar_view_->StartObservingTabStrip(
        browser_view_->browser()->tab_strip_model());
  }

  // Subscribe to Astra command events for sidebar toggle etc.
  AstraCommandDelegate::AddObserver(this);

  // Start observing Astra services for reactive UI updates.
  StartObservingServices();

  installed_ = true;

  // Notify observers that installation is complete.
  for (Observer& observer : observers_) {
    observer.OnAstraBrowserViewInstalled();
  }
}

void AstraBrowserView::Uninstall() {
  if (sidebar_view_) {
    AstraCommandDelegate::RemoveObserver(this);

    // Stop observing services.  ScopedObservation will clean up on
    // destruction, but we do it explicitly here to avoid any callbacks
    // during teardown.
    theme_service_observation_.Reset();
    focus_mode_service_observation_.Reset();
    workspace_service_observation_.Reset();

    // Stop observing the tab strip before the sidebar view is destroyed.
    // The sidebar's base::ScopedObservation would also clean up on
    // destruction, but we do it explicitly here for clarity and to ensure
    // no observer callbacks fire during teardown.
    sidebar_view_->StopObservingTabStrip();

    // Close all open bubbles before removing the sidebar.
    CloseAllBubbles();

    // Remove from the BrowserView hierarchy. The view is owned by the
    // parent View, so we remove it and release the raw pointer.
    // TODO(astra): Use the proper removal method once we know the exact
    // insertion point in BrowserView's layout. With AddChildView, the
    // parent owns the child and RemoveChildViewT deletes it.
    if (browser_view_ && browser_view_->GetIndexOf(sidebar_view_).has_value()) {
      browser_view_->RemoveChildViewT(sidebar_view_);
    }
    sidebar_view_ = nullptr;
    installed_ = false;

    // Notify observers that uninstall is complete.
    for (Observer& observer : observers_) {
      observer.OnAstraBrowserViewUninstalled();
    }
  }
}

void AstraBrowserView::UpdateSidebar() {
  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }
}

void AstraBrowserView::ShowSidebar() {
  if (!sidebar_view_) {
    return;
  }
  if (!IsSidebarVisible()) {
    sidebar_view_->SetVisible(true);
    // TODO(astra): Trigger BrowserView layout reflow after visibility change.
    // The correct approach is to call InvalidateLayout() on the parent or
    // use the BrowserView layout manager's relayout mechanism.
    if (browser_view_) {
      browser_view_->Layout();
    }
    // Notify observers of the visibility change.
    for (Observer& observer : observers_) {
      observer.OnSidebarVisibilityChanged(true);
    }
  }
}

void AstraBrowserView::HideSidebar() {
  if (!sidebar_view_) {
    return;
  }
  if (IsSidebarVisible()) {
    sidebar_view_->SetVisible(false);
    // TODO(astra): Trigger BrowserView layout reflow after visibility change.
    if (browser_view_) {
      browser_view_->Layout();
    }
    // Notify observers of the visibility change.
    for (Observer& observer : observers_) {
      observer.OnSidebarVisibilityChanged(false);
    }
  }
}

void AstraBrowserView::ToggleSidebar() {
  if (IsSidebarVisible()) {
    HideSidebar();
  } else {
    ShowSidebar();
  }
}

bool AstraBrowserView::IsSidebarVisible() const {
  return sidebar_view_ && sidebar_view_->GetVisible();
}

void AstraBrowserView::ToggleSidebarPin() {
  sidebar_pinned_ = !sidebar_pinned_;
  // TODO(astra): When unpinned, the sidebar should auto-hide when the
  // mouse leaves and show on hover. This requires an overlay-style sidebar
  // presentation rather than an in-layout sidebar.
  //
  // Chromium pattern to follow: chrome/browser/ui/views/side_panel/ — the
  // side panel has both pin modes and uses a slide-in overlay when
  // unpinned. Astra sidebar can follow a similar pattern but on the left.
}

// =========================================================================
// Command palette
// =========================================================================

void AstraBrowserView::ShowCommandPalette() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // If already open, just focus the search field.
  if (command_palette_widget_) {
    command_palette_widget_->Show();
    command_palette_widget_->Activate();
    return;
  }

  // Determine the anchor view.
  // TODO(astra): Anchor to the actual location bar / omnibox instead of
  // the toolbar or sidebar.  The omnibox is the natural anchor for a
  // command palette since it's already a text input at the top of the window.
  // Chromium component: LocationBar / OmniboxView
  // (chrome/browser/ui/views/location_bar/location_bar_view.h)
  //
  // For now, anchor to the BrowserView itself (top-left corner).
  // This is a reasonable default that positions the palette near the top.
  views::View* anchor = browser_view_;

  // Show the palette bubble.  The widget is owned by the native widget
  // system; we just keep a raw pointer to track its existence.
  bool was_any_bubble_open = IsAnyBubbleOpen();
  command_palette_widget_ = AstraCommandPaletteBubble::ShowBubble(
      anchor, browser_view_->browser(), this);

  if (command_palette_widget_) {
    command_palette_widget_->AddObserver(this);
    if (!was_any_bubble_open) {
      NotifyBubbleOpened();
    }
  }
}

void AstraBrowserView::HideCommandPalette() {
  if (command_palette_widget_) {
    command_palette_widget_->Close();
    // The widget will call OnCommandPaletteClosed() which clears the pointer.
    // We don't clear it here because Close() is asynchronous.
  }
}

bool AstraBrowserView::IsCommandPaletteOpen() const {
  return command_palette_widget_ != nullptr;
}

// =========================================================================
// Tab search
// =========================================================================

void AstraBrowserView::ShowTabSearch() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // If already open, just focus the search field.
  if (tab_search_widget_) {
    tab_search_widget_->Show();
    tab_search_widget_->Activate();
    return;
  }

  // Determine the anchor view.
  // TODO(astra): Anchor to the actual tab strip or a tab search button
  // in the toolbar / tab strip area.  The natural anchor for tab search
  // is the tab strip's tab search button (Chrome has one in the top-right
  // of the tab strip).
  // Chromium owner: TabSearchButton / TabSearchBubbleHost
  //   (chrome/browser/ui/views/tab_search/tab_search_button.h)
  //
  // For now, anchor to the BrowserView itself (top-left area).
  // This is a reasonable default near the tab strip.
  //
  // TODO(astra): Use bubble_anchor_util to compute a proper anchor point.
  // Chromium utility: chrome/browser/ui/views/bubble_anchor_util.h
  views::View* anchor = browser_view_;

  // Show the tab search bubble.  The widget is owned by the native widget
  // system; we just keep a raw pointer to track its existence.
  bool was_any_bubble_open = IsAnyBubbleOpen();
  tab_search_widget_ = AstraTabSearchBubble::ShowBubble(
      anchor, browser_view_->browser(), this);

  if (tab_search_widget_) {
    tab_search_widget_->AddObserver(this);
    if (!was_any_bubble_open) {
      NotifyBubbleOpened();
    }
  }
}

void AstraBrowserView::HideTabSearch() {
  if (tab_search_widget_) {
    tab_search_widget_->Close();
    // The widget will call OnTabSearchBubbleClosed() which clears the pointer.
    // We don't clear it here because Close() is asynchronous.
  }
}

bool AstraBrowserView::IsTabSearchOpen() const {
  return tab_search_widget_ != nullptr;
}

// =========================================================================
// Settings bubble
// =========================================================================

void AstraBrowserView::ShowSettings() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // If already open, just activate it.
  if (settings_widget_) {
    settings_widget_->Show();
    settings_widget_->Activate();
    return;
  }

  // Determine the anchor view.
  // TODO(astra): Anchor to the actual settings button in the sidebar or
  // toolbar.  The natural anchor for Astra settings is a settings gear
  // icon in the sidebar footer or the toolbar's settings button.
  // Chromium component: ToolbarView / AppMenuButton
  // (chrome/browser/ui/views/toolbar/toolbar_view.h)
  //
  // For now, anchor to the BrowserView itself (top-left area).
  // This is a reasonable default near the sidebar where a settings
  // button would typically live.
  views::View* anchor = browser_view_;

  // Show the settings bubble.  The widget is owned by the native widget
  // system; we just keep a raw pointer to track its existence.
  bool was_any_bubble_open = IsAnyBubbleOpen();
  settings_widget_ = AstraSettingsBubble::ShowBubble(
      anchor, browser_view_->browser(), this);

  if (settings_widget_) {
    settings_widget_->AddObserver(this);
    if (!was_any_bubble_open) {
      NotifyBubbleOpened();
    }
  }
}

void AstraBrowserView::HideSettings() {
  if (settings_widget_) {
    settings_widget_->Close();
    // The widget will call OnSettingsBubbleClosed() which clears the pointer.
    // We don't clear it here because Close() is asynchronous.
  }
}

bool AstraBrowserView::IsSettingsOpen() const {
  return settings_widget_ != nullptr;
}

// =========================================================================
// Workspace overview
// =========================================================================

void AstraBrowserView::ShowWorkspaceOverview() {
  if (!browser_view_) {
    return;
  }

  // Create the controller lazily on first show.
  if (!workspace_overview_controller_) {
    workspace_overview_controller_ =
        std::make_unique<AstraWorkspaceOverviewController>(browser_view_);
  }

  workspace_overview_controller_->Show();
}

void AstraBrowserView::HideWorkspaceOverview() {
  if (workspace_overview_controller_) {
    workspace_overview_controller_->Hide();
  }
}

bool AstraBrowserView::IsWorkspaceOverviewVisible() const {
  return workspace_overview_controller_ &&
         workspace_overview_controller_->IsVisible();
}

// =========================================================================
// Split view control
// =========================================================================

void AstraBrowserView::ToggleSplitView() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  if (split_view_controller_ && split_view_controller_->IsSplitViewActive()) {
    HideSplitView();
  } else {
    // Default to horizontal orientation for toggle.
    ShowSplitView(SplitViewOrientation::kHorizontal);
  }
}

void AstraBrowserView::ShowSplitView(SplitViewOrientation orientation) {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip || tab_strip->count() < 2) {
    // Need at least two tabs for split view.
    return;
  }

  // Use the active tab as primary and the next tab as secondary.
  // TODO(astra): In the full implementation, the partner tab should be
  // determined by AstraTabFeatures::split_view_partner_id(), or by the
  // user's explicit selection (e.g. drag a tab into the split area).
  // The "next tab" approach is a sensible default for the toggle command.
  //
  // Chromium subsystem: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  int active_index = tab_strip->active_index();
  int next_index = (active_index + 1) % tab_strip->count();

  content::WebContents* primary = tab_strip->GetWebContentsAt(active_index);
  content::WebContents* secondary = tab_strip->GetWebContentsAt(next_index);

  if (!primary || !secondary || primary == secondary) {
    return;
  }

  // Create the controller if it doesn't exist yet.
  if (!split_view_controller_) {
    split_view_controller_ =
        std::make_unique<AstraSplitViewController>(browser_view_);
  }

  // Read orientation from the active tab's metadata if available,
  // otherwise use the provided default.
  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(primary);
  if (features && features->is_in_split_view()) {
    orientation = features->split_view_orientation();
  }

  split_view_controller_->ShowSplitView(primary, secondary, orientation);
}

void AstraBrowserView::HideSplitView() {
  if (split_view_controller_) {
    split_view_controller_->HideSplitView();
  }
}

bool AstraBrowserView::IsSplitViewActive() const {
  return split_view_controller_ && split_view_controller_->IsSplitViewActive();
}

void AstraBrowserView::SwapSplitViews() {
  if (split_view_controller_) {
    split_view_controller_->SwapViews();
  }
}

// =========================================================================
// Glance / peek control
// =========================================================================

void AstraBrowserView::ShowGlance(content::WebContents* for_contents,
                                  const gfx::Rect& anchor) {
  if (!browser_view_ || !for_contents) {
    return;
  }

  // Create the controller lazily on first show.
  if (!glance_controller_) {
    glance_controller_ =
        std::make_unique<AstraGlanceViewController>(browser_view_);
  }

  glance_controller_->ShowGlance(for_contents, anchor);
}

void AstraBrowserView::ShowGlanceForURL(const GURL& url,
                                        const gfx::Rect& anchor) {
  if (!browser_view_ || !url.is_valid()) {
    return;
  }

  // Create the controller lazily on first show.
  if (!glance_controller_) {
    glance_controller_ =
        std::make_unique<AstraGlanceViewController>(browser_view_);
  }

  glance_controller_->ShowGlanceForURL(url, anchor);
}

void AstraBrowserView::HideGlance() {
  if (glance_controller_) {
    glance_controller_->HideGlance();
  }
}

bool AstraBrowserView::IsGlanceVisible() const {
  return glance_controller_ && glance_controller_->IsGlanceVisible();
}

// =========================================================================
// Workspace menu / profile area
// =========================================================================

void AstraBrowserView::ShowWorkspaceMenu() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // Create the controller lazily on first show.
  if (!profile_menu_controller_) {
    profile_menu_controller_ =
        std::make_unique<AstraProfileMenuController>(browser_view_->browser());
  }

  // TODO(astra): Anchor the menu to the actual avatar toolbar button
  // from BrowserView.  Currently we anchor to the BrowserView itself
  // at the top-right corner as a placeholder.
  //
  // Chromium owner: AvatarToolbarButton
  //   (chrome/browser/ui/views/toolbar/avatar_toolbar_button.h)
  // Patch point: BrowserView::avatar_button() or
  //   ToolbarView::avatar_button() — get the anchor view from there.
  views::View* anchor = browser_view_;

  profile_menu_controller_->ShowWorkspaceMenu(anchor);
}

void AstraBrowserView::HideWorkspaceMenu() {
  if (profile_menu_controller_) {
    profile_menu_controller_->HideWorkspaceMenu();
  }
}

bool AstraBrowserView::IsWorkspaceMenuVisible() const {
  return profile_menu_controller_ &&
         profile_menu_controller_->IsWorkspaceMenuShowing();
}

// =========================================================================
// Workspace import / export dialog
// =========================================================================

void AstraBrowserView::ShowImportExportDialog(
    AstraWorkspaceImportExportDialog::Mode mode) {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // If already open, just activate it.
  if (import_export_widget_) {
    import_export_widget_->Show();
    import_export_widget_->Activate();
    return;
  }

  // Determine the anchor view.
  // TODO(astra): Anchor to the actual button or menu item that triggered
  // the dialog. For now, anchor to the BrowserView itself (top-left area).
  views::View* anchor = browser_view_;

  // Get the profile from the browser.
  Profile* profile = browser_view_->browser()->profile();
  if (!profile) {
    return;
  }

  // Show the dialog bubble.
  // ShowBubble returns the bubble delegate; we get the widget from it.
  bool was_any_bubble_open = IsAnyBubbleOpen();
  AstraWorkspaceImportExportDialog* dialog =
      AstraWorkspaceImportExportDialog::ShowBubble(anchor, profile, mode);
  if (dialog) {
    import_export_widget_ = dialog->GetWidget();
    if (import_export_widget_) {
      import_export_widget_->AddObserver(this);
      if (!was_any_bubble_open) {
        NotifyBubbleOpened();
      }
    }
  }
}

void AstraBrowserView::HideImportExportDialog() {
  if (import_export_widget_) {
    import_export_widget_->Close();
    // The widget will call back to clear the pointer.
    // TODO(astra): Wire up the dialog closed callback to clear
    // import_export_widget_, similar to how command palette does it.
  }
}

bool AstraBrowserView::IsImportExportDialogVisible() const {
  return import_export_widget_ != nullptr;
}

// =========================================================================
// Focus mode
// =========================================================================

void AstraBrowserView::ToggleFocusMode() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // Toggle focus mode through the service, which is the truth source.
  // The service notifies observers, and we update the UI via our
  // AstraFocusModeServiceObserver overrides (OnFocusModeEntered /
  // OnFocusModeExited).
  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  AstraFocusModeService* focus_service =
      AstraFocusModeService::GetForProfile(profile);
  if (focus_service) {
    focus_service->ToggleFocusMode();
  }

  // Create the controller lazily if it doesn't exist yet.
  // The controller handles the UI presentation of focus mode.
  if (!focus_mode_controller_) {
    focus_mode_controller_ =
        std::make_unique<AstraFocusModeController>(browser_view_);
  }
}

bool AstraBrowserView::IsFocusModeActive() const {
  // Read the current state from the focus mode controller, which reads
  // from the service (truth source). The controller may be null if focus
  // mode has never been activated in this window.
  if (!focus_mode_controller_) {
    return false;
  }
  return focus_mode_controller_->IsFocusModeActive();
}

// =========================================================================
// New tab page
// =========================================================================

void AstraBrowserView::ShowNewTabPage() {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // If already open, just activate it.
  if (new_tab_widget_) {
    new_tab_widget_->Show();
    new_tab_widget_->Activate();
    return;
  }

  // The NTP is shown as a full-window overlay / large bubble.
  // TODO(astra): Consider using a full-window Widget instead of a
  // bubble for the new tab page, to get a cleaner full-viewport
  // presentation. The bubble pattern is used for consistency with
  // other Astra surfaces, but the NTP is more of a full-page overlay.
  //
  // Chromium pattern reference: TabSwitcher / TabSearch full-page
  //   overlays use WidgetDelegateView + fullscreen/MAXIMIZE state.

  // Show the new tab page bubble.
  // TODO(astra): Anchor the NTP properly — it should fill the content
  // area or be centered as a large card in the browser window.
  bool was_any_bubble_open = IsAnyBubbleOpen();
  new_tab_widget_ = AstraNewTabBubble::ShowBubble(
      browser_view_, browser_view_->browser(), this);

  if (new_tab_widget_) {
    new_tab_widget_->AddObserver(this);
    if (!was_any_bubble_open) {
      NotifyBubbleOpened();
    }
  }
}

void AstraBrowserView::HideNewTabPage() {
  if (new_tab_widget_) {
    new_tab_widget_->Close();
    // TODO(astra): Wire up the bubble closed callback to clear new_tab_widget_.
  }
}

bool AstraBrowserView::IsNewTabPageVisible() const {
  return new_tab_widget_ != nullptr;
}

// DEPRECATED: The sidebar now observes TabStripModel directly.
// This method is kept for compatibility — it triggers a full rebuild.
// Primary update path: TabStripModelObserver on AstraSidebarView.
void AstraBrowserView::OnTabStripModelChanged() {
  UpdateSidebar();
}

// =========================================================================
// AstraCommandDelegate::Observer implementation
// =========================================================================
//
// The command delegate uses a global observer list, so every
// AstraBrowserView instance receives all command notifications.  In a
// multi-window scenario, each instance determines whether to react.
//
// TODO(astra): Scope command execution to the active browser window.
// The current global observer list means all windows react to every
// command.  The fix is to either:
//   1. Pass Browser* through the observer interface, or
//   2. Move observers to a per-Browser helper object.
// Option (2) is preferred and matches Chromium's BrowserCommandController
// pattern (one controller per Browser).
// =========================================================================

void AstraBrowserView::OnToggleSidebar() {
  // TODO(astra): Only toggle if this is the active browser window.
  ToggleSidebar();
}

void AstraBrowserView::OnToggleSidebarPin() {
  ToggleSidebarPin();
}

void AstraBrowserView::OnOpenCommandPalette() {
  // TODO(astra): Only show if this is the active browser window.
  // The command delegate broadcasts to all observers; each browser view
  // should check if it's the active window before reacting.
  ShowCommandPalette();
}

void AstraBrowserView::OnOpenTabSearch() {
  // TODO(astra): Only show if this is the active browser window.
  // The command delegate broadcasts to all observers; each browser view
  // should check if it's the active window before reacting.
  //
  // Tab search reads from Chromium's TabStripModel (truth source) and
  // projects the results into the search bubble.  No state is stored in
  // the UI layer.
  ShowTabSearch();
}

void AstraBrowserView::OnOpenGlance() {
  // TODO(astra): Only show if this is the active browser window.
  //
  // For the OpenGlance command, we preview the active tab (or a URL,
  // depending on context).  The exact behavior depends on what triggered
  // the command:
  //   - Keyboard shortcut: could glance the current link under cursor, or
  //     peek at the next tab, or show the bookmark bar item under cursor.
  //   - Sidebar hover: glances the hovered tab.
  //   - Link hover: glances the linked URL.
  //
  // For the command-based trigger, we default to showing a URL glance for
  // the active tab's URL (as a demo), or peeking at the next tab.
  //
  // TODO(astra): Determine the correct default behavior for the OpenGlance
  // command.  Options:
  //   1. Peek at the next tab in the tab strip (tab glance mode).
  //   2. Show a URL preview of the link under the mouse cursor (URL glance).
  //   3. Toggle a "glance mode" where hovering tabs shows previews.
  //
  // For now, we implement a demo that shows a URL glance for a sample URL
  // anchored near the sidebar.  The real behavior should be driven by the
  // context that triggered the command.
  //
  // Chromium subsystem: depends on the trigger source.
  // For link hover: content::RenderViewHost / WebContents hover state.
  // For sidebar hover: AstraSidebarView mouse events.

  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip || tab_strip->count() < 1) {
    return;
  }

  // Demo: show a tab glance for the active tab, anchored to a dummy position.
  // In real usage, the anchor would come from the sidebar item or link.
  content::WebContents* active_contents = tab_strip->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  // Anchor to a position near the top-left (sidebar area).
  // TODO(astra): Compute the actual anchor based on context.
  gfx::Rect anchor(0, 100, 1, 1);

  ShowGlance(active_contents, anchor);
}

void AstraBrowserView::OnShowAllWorkspaces() {
  // Show the workspace overview overlay.
  //
  // The overview is a full-window presentation layer that shows all
  // workspaces as cards with tab previews.  It reads workspace data from
  // AstraWorkspaceService and tab data from TabStripModel.
  //
  // Architecture:
  //   - Truth source: AstraWorkspaceService (workspace metadata) +
  //     TabStripModel (tab state) + Chromium thumbnails (future).
  //   - Controller: AstraWorkspaceOverviewController — manages the widget
  //     and dispatches user actions to services.
  //   - View: AstraWorkspaceOverviewView — pure presentation, no state.
  //
  // Chromium subsystems reused: TabStripModel, Widget, Views framework.
  // Patch point: This is triggered from the kAstraCommandShowAllWorkspaces
  //   command, which flows through the BrowserCommandController patch in
  //   chrome/browser/ui/browser_command_controller.cc.
  ShowWorkspaceOverview();
}

void AstraBrowserView::OnOpenSettings() {
  // Show the Astra settings bubble.
  //
  // The settings page is a Views-based surface that reads and writes
  // Astra preferences directly through PrefService.  No state is stored
  // in the settings view itself — it's purely a projection of pref values.
  //
  // Architecture:
  //   - Truth source: PrefService (sidebar, split view, general settings)
  //                   + AstraWorkspaceService (workspace metadata).
  //   - View: AstraSettingsPageView — pure presentation, no state.
  //   - Bubble: AstraSettingsBubble — BubbleDialogDelegateView wrapper.
  //
  // Chromium subsystems reused: PrefService, BubbleDialogDelegateView,
  //   ToggleButton, Slider, Combobox.
  // Patch point: kAstraCommandOpenSettings flows through the
  //   BrowserCommandController patch in
  //   chrome/browser/ui/browser_command_controller.cc.
  //
  // TODO(astra): Only show if this is the active browser window.
  // The command delegate broadcasts to all observers; each browser view
  // should check if it's the active window before reacting.
  ShowSettings();
}

void AstraBrowserView::OnSwitchWorkspaceMenu() {
  // Show the workspace switcher menu in the profile menu area.
  //
  // The workspace menu shows the current workspace and a list of all
  // workspaces for quick switching.  It's designed to be shown from the
  // avatar button / profile menu area since that's already a
  // user/identity-oriented location.
  //
  // Architecture:
  //   - Truth source: AstraWorkspaceService (ProfileKeyedService).
  //   - Controller: AstraProfileMenuController — manages the menu view
  //     and handles user actions (switch workspace, new workspace).
  //   - View: AstraProfileMenuWorkspaces — pure presentation.
  //
  // Chromium subsystems reused: Profile menu concept, AvatarToolbarButton.
  // Patch point: kAstraCommandSwitchWorkspaceMenu flows through the
  //   BrowserCommandController patch in
  //   chrome/browser/ui/browser_command_controller.cc.
  //
  // TODO(astra): Integrate more deeply with the profile menu.  The
  // current approach shows a standalone bubble from the avatar area.
  // A better integration would embed the workspace section directly
  // into the profile menu via the ProfileMenuView::BuildBody() patch.
  // See patch documentation:
  //   chromium/astra/patches/0008-profile-menu-workspaces.md
  ShowWorkspaceMenu();
}

void AstraBrowserView::OnExportWorkspaces() {
  // Show the workspace export dialog.
  //
  // The export dialog shows a summary of workspaces and provides the
  // exported JSON for copy-paste or file save.
  //
  // Architecture:
  //   - Truth source: AstraWorkspaceService + TabStripModel (Chromium).
  //   - Export logic: AstraWorkspaceImportExport (browser layer).
  //   - View: AstraWorkspaceImportExportDialog (views layer).
  //
  // Chromium subsystems reused: BrowserList, TabStripModel, views framework.
  // TODO(astra): Proper dialog anchoring — the dialog should be anchored
  // to the button or menu item that triggered it, not the whole BrowserView.
  ShowImportExportDialog(AstraWorkspaceImportExportDialog::Mode::kExport);
}

void AstraBrowserView::OnImportWorkspaces() {
  // Show the workspace import dialog.
  //
  // The import dialog lets the user paste JSON or load from a file,
  // validates the data, and shows a preview before confirming import.
  //
  // Architecture:
  //   - Import logic: AstraWorkspaceImportExport (browser layer).
  //   - View: AstraWorkspaceImportExportDialog (views layer).
  //   - State changes: workspaces created, tabs opened.
  //
  // Chromium subsystems reused: TabStripModel, Browser, views framework.
  // TODO(astra): Proper dialog anchoring.
  ShowImportExportDialog(AstraWorkspaceImportExportDialog::Mode::kImport);
}

void AstraBrowserView::OnWorkspaceNavigateRequested(int direction) {
  // Workspace navigation requested (from keyboard shortcut, etc.).
  // In incognito mode, this is the primary navigation path since the
  // command delegate does not call AstraWorkspaceService::ActivateWorkspace
  // directly (to avoid modifying the shared/original profile state).
  //
  // The sidebar handles the actual navigation, including the incognito
  // logic (local active workspace vs. service-based).
  //
  // See AstraIncognitoHandler::DoesWorkspaceActivationAffectService.
  if (sidebar_view_) {
    sidebar_view_->NavigateWorkspace(direction);
  }
}

void AstraBrowserView::OnSplitViewStateChanged() {
  // When split view state changes, read the current state from the active
  // tab's AstraTabFeatures and update the presentation accordingly.
  //
  // The command delegate has already updated the metadata (toggle,
  // orientation, swap).  We read that metadata and apply it to the UI.
  //
  // Chromium subsystem reused: TabStripModel + WebContents pair.
  // Astra metadata: is_in_split_view, split_view_orientation, split_view_ratio.

  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip || tab_strip->count() < 1) {
    return;
  }

  content::WebContents* active_contents = tab_strip->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(active_contents);
  if (!features) {
    // No Astra metadata on this tab — ensure split view is off.
    if (IsSplitViewActive()) {
      HideSplitView();
    }
    return;
  }

  if (features->is_in_split_view()) {
    // Activate or update split view.
    if (!split_view_controller_) {
      split_view_controller_ =
          std::make_unique<AstraSplitViewController>(browser_view_);
    }

    if (!split_view_controller_->IsSplitViewActive()) {
      // Need to find the partner tab.
      // TODO(astra): Use split_view_partner_id() to find the partner by
      // Astra tab identity instead of just using the next tab.  For now,
      // use the next tab in the strip as a reasonable default.
      ShowSplitView(features->split_view_orientation());
    } else {
      // Already active — update orientation/ratio from metadata.
      split_view_controller_->SetOrientation(features->split_view_orientation());
      split_view_controller_->SetSplitRatio(features->split_view_ratio());
    }
  } else {
    // Deactivate split view.
    if (IsSplitViewActive()) {
      HideSplitView();
    }
  }
}

// =========================================================================
// AstraCommandDelegate::Observer: favorite folders
// =========================================================================

void AstraBrowserView::OnFavoriteFoldersChanged() {
  // Favorite folder membership or structure changed. Refresh the sidebar
  // to update the favorite folders section.
  //
  // Truth source: AstraFavoriteService (ProfileKeyedService).
  // UI projection: sidebar's favorite folders section.
  //
  // TODO(astra): Use a more targeted update (e.g. just the favorites
  //   section) instead of a full sidebar rebuild. For now, a full update
  //   is fine since the sidebar is relatively cheap to rebuild.
  //
  // Chromium owner: AstraFavoriteService (astra/browser/astra_favorite_service.h)
  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }
}

// =========================================================================
// AstraCommandDelegate::Observer: recently closed tabs
// =========================================================================

void AstraBrowserView::OnRecentlyClosedTabRestored() {
  // A recently closed tab was restored. Update the sidebar's recently
  // closed section to reflect the change.
  //
  // Truth source: sessions::TabRestoreService (Chromium).
  // UI projection: sidebar's recently closed section.
  //
  // TODO(astra): Consider making the sidebar observe TabRestoreService
  //   directly, similar to how it observes TabStripModel. This would
  //   provide more granular updates than the full rebuild here.
  //
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }
}

void AstraBrowserView::OnAllRecentlyClosedTabsRestored() {
  // All recently closed tabs were restored at once.
  // Same as OnRecentlyClosedTabRestored but for bulk restore.
  //
  // TODO(astra): Optimize for bulk restore — the sidebar could show
  //   a loading state or animate the transition. For now, a full
  //   rebuild is sufficient.
  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }
}

// =========================================================================
// AstraCommandDelegate::Observer: search settings
// =========================================================================

void AstraBrowserView::OnOpenSearchSettings() {
  // Open search engine settings. Currently this opens Chrome's native
  // search settings page in a new tab, since search engine state is
  // fully owned by Chromium's TemplateURLService.
  //
  // TODO(astra): Consider two options:
  //   Option A: Open the Astra settings bubble scrolled to the search
  //     settings section (for quick access without leaving the page).
  //   Option B: Open Chrome's search settings WebUI (chrome://settings/searchEngines)
  //     for full management capabilities.
  //   Currently using Option B as the primary path, with Option A as
  //   a future enhancement for the Astra settings bubble.
  //
  // Chromium owner: TemplateURLService
  //   (components/search_engines/template_url_service.h)
  // Chromium WebUI: chrome://settings/searchEngines
  //
  // TODO(astra): Only open if this is the active browser window.
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // Open Chrome's search settings page in a new tab.
  // TODO(astra): Use the proper URL for search settings.
  //   The exact URL may vary by Chromium version.
  GURL settings_url("chrome://settings/searchEngines");
  browser_view_->browser()->OpenURL(content::OpenURLParams(
      settings_url, content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false));
}

// =========================================================================
// AstraCommandDelegate::Observer: extensions panel
// =========================================================================

void AstraBrowserView::OnToggleExtensionsPanel() {
  // Toggle the extensions panel in the sidebar.
  //
  // The extensions panel shows installed extensions and their actions.
  // It is a section within the sidebar, managed by AstraSidebarView.
  //
  // Truth source: ExtensionService (Chromium) + sidebar state.
  // UI projection: sidebar extensions section.
  //
  // TODO(astra): The extensions panel is a sidebar section. This method
  //   should call a method on AstraSidebarView to toggle the extensions
  //   section visibility or scroll it into view. For now, we just
  //   ensure the sidebar is visible, which is a reasonable fallback.
  //
  // Chromium owner: ExtensionService
  //   (extensions/browser/extension_service.h)
  //   ExtensionsToolbarContainer
  //   (chrome/browser/ui/views/extensions/extensions_toolbar_container.h)

  if (!IsSidebarVisible()) {
    ShowSidebar();
  }
  // TODO(astra): Scroll to or expand the extensions section in the sidebar.
  //   This requires a method on AstraSidebarView like
  //   ToggleExtensionsSection() or ScrollToSection(kExtensions).
}

// =========================================================================
// AstraCommandDelegate::Observer: omnibox command mode
// =========================================================================

void AstraBrowserView::OnFocusOmniboxCommandMode() {
  // Focus the omnibox and put it in "command mode" — a mode where
  // typing @-prefixed commands or special characters triggers command
  // palette-like behavior directly from the address bar.
  //
  // This is an alternative entry point to the command palette that
  // leverages the existing omnibox infrastructure.
  //
  // Truth source: OmniboxController / LocationBarView (Chromium).
  // UI projection: omnibox focus + command mode state.
  //
  // TODO(astra): Implement proper omnibox command mode. Options:
  //   Option A: Focus the omnibox and insert a trigger character
  //     (e.g. '?' or '@') to enter command mode.
  //   Option B: Open the command palette instead (simpler, reuses
  //     existing command palette infrastructure).
  //   Option C: Use Chromium's side search or omnibox pedal system.
  //
  //   Currently using Option B as a fallback — open the command
  //   palette, which provides a similar command entry experience.
  //
  // Chromium owner: LocationBarView / OmniboxView
  //   (chrome/browser/ui/views/location_bar/location_bar_view.h)
  // Patch point: OmniboxView::SetFocusAndSelection() or
  //   LocationBarView::FocusLocation() to focus the omnibox.
  ShowCommandPalette();
}

// =========================================================================
// AstraCommandDelegate::Observer: focus mode
// =========================================================================

void AstraBrowserView::OnToggleFocusMode() {
  // Toggle focus mode on/off. This is the command-driven path
  // (keyboard shortcut, command palette).
  //
  // Truth source: AstraFocusModeService (ProfileKeyedService).
  // UI projection: sidebar hidden, toolbar minimized, indicator shown.
  //
  // TODO(astra): Route through the service instead of directly toggling.
  //   The command delegate should call AstraFocusModeService::Toggle(),
  //   which notifies observers, and the focus mode controller should
  //   observe the service and update UI accordingly.
  //
  //   Currently we call ToggleFocusMode() directly on the controller,
  //   which in turn updates the service state and applies UI changes.
  ToggleFocusMode();
}

// =========================================================================
// AstraCommandDelegate::Observer: screenshots
// =========================================================================

void AstraBrowserView::OnScreenshotVisible() {
  // Capture the visible area (viewport) of the current tab.
  //
  // The capture is performed by AstraScreenshotService. When the
  // capture completes, the service notifies its observers, and the
  // capture bubble is shown with the result.
  //
  // Truth source: AstraScreenshotService + Chromium capture APIs.
  // UI projection: capture bubble showing the result + action buttons.
  //
  // TODO(astra): Observe AstraScreenshotService to show the capture
  //   bubble when the bitmap is ready. Currently the service does
  //   the capture (stub) but the UI doesn't react to completion yet.
  //   A future iteration should:
  //   1. Make AstraBrowserView implement AstraScreenshotServiceObserver
  //   2. OnScreenshotTaken → show capture bubble
  //   3. OnScreenshotFailed → show error toast or bubble
  //
  // TODO(astra): Only capture if this is the active browser window.
  //   The command delegate broadcasts to all observers; each browser
  //   view should check if it's the active window before reacting.
  //
  // Chromium owner: content::WebContents (visible area capture)
  // Astra service: AstraScreenshotService

  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  content::WebContents* active_contents =
      browser_view_->browser()->tab_strip_model()->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  // Initiate capture through the screenshot service.
  // TODO(astra): Get the service from the profile and call
  //   CaptureVisibleArea(). For now, we create a placeholder
  //   capture bubble directly as a UI demo.
  //
  // In the full implementation:
  //   AstraScreenshotService::GetForBrowserContext(profile)
  //     ->CaptureVisibleArea(web_contents);
  //   Service notifies observers on completion.
  //   Observer (AstraBrowserView) shows capture bubble.

  // TODO(astra): Remove this demo bubble and wire through the service.
  //   For now, show an empty capture bubble as a placeholder.
  if (!screenshot_capture_widget_) {
    SkBitmap placeholder_bitmap;
    placeholder_bitmap.allocN32Pixels(800, 600);
    placeholder_bitmap.eraseColor(SK_ColorLTGRAY);
    bool was_any_bubble_open = IsAnyBubbleOpen();
    screenshot_capture_widget_ = AstraScreenshotCaptureBubble::ShowBubble(
        browser_view_, browser_view_->browser(), placeholder_bitmap,
        AstraScreenshotType::kVisibleArea, gfx::Rect(0, 0, 800, 600),
        this);
    if (screenshot_capture_widget_) {
      screenshot_capture_widget_->AddObserver(this);
      if (!was_any_bubble_open) {
        NotifyBubbleOpened();
      }
    }
  }
}

void AstraBrowserView::OnScreenshotFullPage() {
  // Capture the full page (entire scrollable document) of the current tab.
  //
  // Full-page capture requires scrolling and stitching multiple captures
  // or using a dedicated full-page capture API.
  //
  // Chromium owner: chrome/browser/screenshot/ (if available) or
  //   content::ScreenshotManager for full-page capture.
  //
  // TODO(astra): Implement full-page capture through AstraScreenshotService.
  //   See OnScreenshotVisible() for the overall architecture notes.
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  content::WebContents* active_contents =
      browser_view_->browser()->tab_strip_model()->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  // TODO(astra): Call AstraScreenshotService::CaptureFullPage().
  //   For now, show a placeholder bubble to demonstrate the UI flow.
  if (!screenshot_capture_widget_) {
    SkBitmap placeholder_bitmap;
    placeholder_bitmap.allocN32Pixels(800, 1200);
    placeholder_bitmap.eraseColor(SK_ColorLTGRAY);
    bool was_any_bubble_open = IsAnyBubbleOpen();
    screenshot_capture_widget_ = AstraScreenshotCaptureBubble::ShowBubble(
        browser_view_, browser_view_->browser(), placeholder_bitmap,
        AstraScreenshotType::kFullPage, gfx::Rect(0, 0, 800, 1200),
        this);
    if (screenshot_capture_widget_) {
      screenshot_capture_widget_->AddObserver(this);
      if (!was_any_bubble_open) {
        NotifyBubbleOpened();
      }
    }
  }
}

void AstraBrowserView::OnScreenshotRegion() {
  // Start a region screenshot capture. Shows the region selection
  // overlay so the user can drag a rectangle to select the capture area.
  //
  // The region overlay is a full-window tinted overlay with draggable
  // selection handles. When the user confirms the selection, the
  // capture is performed and the result is shown in the capture bubble.
  //
  // Truth source: AstraScreenshotService + user-selected region.
  // UI projection: region overlay → capture bubble.
  //
  // TODO(astra): Make AstraBrowserView implement
  //   AstraScreenshotRegionOverlay::Delegate to receive the selected
  //   region and initiate the capture.
  //
  // Chromium owner: No direct Chromium equivalent — Astra-specific
  //   region selection UI. The capture itself uses WebContents APIs.
  //
  // TODO(astra): Only show if this is the active browser window.

  if (!browser_view_) {
    return;
  }

  // If already active, just bring it to front.
  if (region_overlay_widget_) {
    region_overlay_widget_->Show();
    region_overlay_widget_->Activate();
    return;
  }

  // Show the region selection overlay.
  // TODO(astra): Implement the full region capture flow:
  //   1. User selects region
  //   2. AstraBrowserView::OnRegionSelected() is called
  //   3. Service captures the region
  //   4. Capture bubble shows the result
  bool was_any_bubble_open = IsAnyBubbleOpen();
  region_overlay_widget_ = AstraScreenshotRegionOverlay::ShowOverlay(
      browser_view_->GetWidget(), this);

  if (region_overlay_widget_) {
    region_overlay_widget_->AddObserver(this);
    if (!was_any_bubble_open) {
      NotifyBubbleOpened();
    }
  }
}

// =========================================================================
// AstraCommandDelegate::Observer: new tab page
// =========================================================================

void AstraBrowserView::OnOpenNewTabPage() {
  // Open the Astra new tab page.
  //
  // The Astra NTP is a Views-based surface that shows workspace
  // shortcuts, most visited sites, and quick actions. It is shown as
  // a full-window overlay / large bubble rather than as a WebUI page.
  //
  // Truth source: AstraWorkspaceService + Chrome's most visited sites.
  // UI projection: AstraNewTabBubble (BubbleDialogDelegateView pattern).
  //
  // TODO(astra): Integrate with Chrome's new tab creation path so that
  //   pressing Ctrl/Cmd+T or clicking the new tab button shows the
  //   Astra NTP instead of Chrome's default NTP.
  //   Chromium owner: Browser::NewTab()
  //   (chrome/browser/ui/browser.cc)
  //   Patch point: NewTab() method — check for Astra NTP flag and show
  //   the Astra NTP bubble instead of navigating to chrome://newtab.
  //
  // TODO(astra): Only show if this is the active browser window.
  ShowNewTabPage();
}

// =========================================================================
// AstraCommandPaletteBubble::Delegate implementation
// =========================================================================

void AstraBrowserView::OnCommandPaletteExecute(int command_id, bool is_astra) {
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  Browser* browser = browser_view_->browser();

  if (is_astra) {
    // Astra command — dispatch through AstraCommandDelegate.
    AstraCommandDelegate::ExecuteCommand(browser, command_id);
  } else {
    // Chrome command — dispatch through Chrome's command controller.
    //
    // TODO(astra): Verify the exact API for executing Chrome commands.
    // The primary entry point is Browser::ExecuteCommand() or
    // BrowserCommandController::ExecuteCommand().  We use
    // command_controller() as the most direct path, but the exact
    // method signature may differ slightly across Chromium versions.
    //
    // Chromium owner: BrowserCommandController
    // (chrome/browser/ui/browser_command_controller.h)
    //
    // TODO(astra): Handle disambiguation with
    // BrowserCommandController::IsCommandEnabled() before execution,
    // or let the controller handle it gracefully.
    browser->command_controller()->ExecuteCommand(command_id);
  }
}

void AstraBrowserView::OnCommandPaletteClosed() {
  // The widget is being closed — clear our raw pointer and stop observing.
  ClearWidgetPointer(command_palette_widget_);
}

// =========================================================================
// AstraSettingsBubble::Delegate implementation
// =========================================================================

void AstraBrowserView::OnSettingsBubbleClosed() {
  // The widget is being closed — clear our raw pointer and stop observing.
  ClearWidgetPointer(settings_widget_);
}

// =========================================================================
// AstraTabSearchBubble::Delegate implementation
// =========================================================================

void AstraBrowserView::OnTabSearchBubbleClosed() {
  // The widget is being closed — clear our raw pointer and stop observing.
  ClearWidgetPointer(tab_search_widget_);
}

// =========================================================================
// AstraNewTabBubble::Delegate implementation
// =========================================================================

void AstraBrowserView::OnNewTabBubbleClosed() {
  // The widget is being closed — clear our raw pointer and stop observing.
  ClearWidgetPointer(new_tab_widget_);
}

// =========================================================================
// AstraScreenshotCaptureBubble::Delegate implementation
// =========================================================================

void AstraBrowserView::OnScreenshotSave() {
  // User clicked "Save to downloads" in the capture bubble.
  // Route the save request through AstraScreenshotService.
  //
  // TODO(astra): Get the actual captured bitmap from the bubble and call
  //   AstraScreenshotService::SaveToDownloads(). Currently the bubble
  //   holds the bitmap; we need a way to pass it back to the service.
  //
  // Chromium subsystem: DownloadService / DownloadManager
  //   (chrome/browser/download/download_service.h)
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // TODO(astra): Get the bitmap from the bubble and save it.
  //   For now, this is a no-op placeholder.
}

void AstraBrowserView::OnScreenshotCopy() {
  // User clicked "Copy to clipboard" in the capture bubble.
  // Route the copy request through AstraScreenshotService.
  //
  // Chromium subsystem: ui/base/clipboard/clipboard.h
  //
  // TODO(astra): Get the bitmap from the bubble and call
  //   AstraScreenshotService::CopyToClipboard().
  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  // TODO(astra): Actual clipboard copy.
}

void AstraBrowserView::OnScreenshotEdit() {
  // User clicked "Edit" in the capture bubble.
  // Opens the image in the default image editor or an Astra-specific
  // screenshot annotation tool.
  //
  // TODO(astra): Implement screenshot editing. Options:
  //   Option A: Open in system default image editor.
  //   Option B: Build an in-app annotation toolbar (crop, draw, text).
  //   Option C: Use Chromium's share/edit system.
  //
  // Chromium owner: ShareManager / image editor (if available)
}

void AstraBrowserView::OnScreenshotDelete() {
  // User clicked "Delete" or the bubble was dismissed.
  // Close the capture bubble and discard the screenshot.
  //
  // The screenshot is not saved — it only exists in memory for the
  // duration of the bubble being shown.
  if (screenshot_capture_widget_) {
    screenshot_capture_widget_->Close();
    // OnScreenshotBubbleClosed() will clear the pointer.
  }
}

void AstraBrowserView::OnScreenshotBubbleClosed() {
  // The capture bubble is closing — clear our raw pointer and stop observing.
  ClearWidgetPointer(screenshot_capture_widget_);
}

// =========================================================================
// AstraScreenshotRegionOverlay::Delegate implementation
// =========================================================================

void AstraBrowserView::OnRegionSelected(const gfx::Rect& region) {
  // User confirmed a region selection.
  // Close the overlay, capture the selected region, and show the
  // capture bubble with the result.
  //
  // Truth source: AstraScreenshotService (does the actual capture).
  // UI projection: region overlay closes → capture bubble shows.

  // Close the region overlay first.
  if (region_overlay_widget_) {
    region_overlay_widget_->Close();
    // ClearWidgetPointer will be called from OnWidgetDestroying.
  }

  if (!browser_view_ || !browser_view_->browser()) {
    return;
  }

  content::WebContents* active_contents =
      browser_view_->browser()->tab_strip_model()->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  // Initiate region capture through the screenshot service.
  // TODO(astra): Call AstraScreenshotService::CaptureRegion() with
  //   the selected region. The service captures asynchronously and
  //   notifies observers on completion.
  //
  // For now, show a placeholder capture bubble as a UI demo.
  // TODO(astra): Remove placeholder and wire through service observer.
  if (!screenshot_capture_widget_) {
    SkBitmap placeholder_bitmap;
    placeholder_bitmap.allocN32Pixels(region.width(), region.height());
    placeholder_bitmap.eraseColor(SK_ColorLTGRAY);
    bool was_any_bubble_open = IsAnyBubbleOpen();
    screenshot_capture_widget_ = AstraScreenshotCaptureBubble::ShowBubble(
        browser_view_, browser_view_->browser(), placeholder_bitmap,
        AstraScreenshotType::kRegion, region, this);
    if (screenshot_capture_widget_) {
      screenshot_capture_widget_->AddObserver(this);
      if (!was_any_bubble_open) {
        NotifyBubbleOpened();
      }
    }
  }
}

void AstraBrowserView::OnRegionSelectionCancelled() {
  // User cancelled the region selection (Escape key, etc.).
  // Close the region overlay without capturing anything.
  if (region_overlay_widget_) {
    region_overlay_widget_->Close();
    // ClearWidgetPointer will be called from OnWidgetDestroying.
  }
}

// =========================================================================
// Bubble utility methods
// =========================================================================

void AstraBrowserView::CloseAllBubbles() {
  // Close each bubble widget.  Each Close() call will eventually trigger
  // OnWidgetDestroying, which clears the pointer and notifies observers.
  // We collect the widgets first to avoid modifying the set while iterating.
  if (command_palette_widget_) {
    command_palette_widget_->Close();
  }
  if (settings_widget_) {
    settings_widget_->Close();
  }
  if (tab_search_widget_) {
    tab_search_widget_->Close();
  }
  if (import_export_widget_) {
    import_export_widget_->Close();
  }
  if (screenshot_capture_widget_) {
    screenshot_capture_widget_->Close();
  }
  if (region_overlay_widget_) {
    region_overlay_widget_->Close();
  }
  if (new_tab_widget_) {
    new_tab_widget_->Close();
  }
}

bool AstraBrowserView::IsAnyBubbleOpen() const {
  return command_palette_widget_ != nullptr ||
         settings_widget_ != nullptr ||
         tab_search_widget_ != nullptr ||
         import_export_widget_ != nullptr ||
         screenshot_capture_widget_ != nullptr ||
         region_overlay_widget_ != nullptr ||
         new_tab_widget_ != nullptr;
}

int AstraBrowserView::GetOpenBubbleCount() const {
  int count = 0;
  if (command_palette_widget_) count++;
  if (settings_widget_) count++;
  if (tab_search_widget_) count++;
  if (import_export_widget_) count++;
  if (screenshot_capture_widget_) count++;
  if (region_overlay_widget_) count++;
  if (new_tab_widget_) count++;
  return count;
}

// =========================================================================
// Convenience accessors
// =========================================================================

content::WebContents* AstraBrowserView::GetActiveWebContents() const {
  if (!browser_view_ || !browser_view_->browser()) {
    return nullptr;
  }
  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip) {
    return nullptr;
  }
  return tab_strip->GetActiveWebContents();
}

Browser* AstraBrowserView::GetBrowser() const {
  if (!browser_view_) {
    return nullptr;
  }
  return browser_view_->browser();
}

Profile* AstraBrowserView::GetProfile() const {
  Browser* browser = GetBrowser();
  if (!browser) {
    return nullptr;
  }
  return browser->profile();
}

// =========================================================================
// views::WidgetObserver implementation
// =========================================================================

void AstraBrowserView::OnWidgetDestroying(views::Widget* widget) {
  // A widget we are observing is being destroyed.  Remove ourselves as
  // an observer and clear our pointer.
  if (!widget) {
    return;
  }

  widget->RemoveObserver(this);
  ClearWidgetPointer(widget);
}

// =========================================================================
// AstraThemeServiceObserver implementation
// =========================================================================

void AstraBrowserView::OnThemeChanged() {
  // Theme changed — refresh all Astra UI colors.
  RefreshAllUiColors();
}

void AstraBrowserView::OnAccentColorChanged(SkColor new_accent_color) {
  // Accent color changed — refresh all Astra UI colors.
  // The ColorProvider will also be updated, so views using color IDs
  // will update automatically.  We refresh here for any surfaces that
  // don't use the ColorProvider directly.
  RefreshAllUiColors();
}

void AstraBrowserView::OnThemePresetChanged(AstraThemePreset preset) {
  // Theme preset changed — colors will be updated via OnThemeChanged.
  // TODO(astra): If the preset directly affects any Astra UI surfaces
  //   beyond what ColorProvider handles, update them here.
}

// =========================================================================
// AstraFocusModeServiceObserver implementation
// =========================================================================

void AstraBrowserView::OnFocusModeEntered(base::TimeDelta duration) {
  // Focus mode started — update UI and notify observers.
  //
  // The focus mode controller independently observes the service and
  // handles its own UI (indicator, menu bubble, tab dimming).
  // AstraBrowserView handles sidebar visibility and observer notifications.

  // Create the controller lazily on first activation.
  if (!focus_mode_controller_ && browser_view_) {
    focus_mode_controller_ =
        std::make_unique<AstraFocusModeController>(browser_view_);
  }

  // Hide the sidebar in focus mode to reduce distractions.
  if (IsSidebarVisible()) {
    HideSidebar();
  }

  // Notify observers.
  for (Observer& observer : observers_) {
    observer.OnFocusModeToggled(true);
  }
}

void AstraBrowserView::OnFocusModeExited() {
  // Focus mode ended — update UI and notify observers.
  //
  // The focus mode controller independently observes the service and
  // handles its own UI teardown.

  // Restore sidebar visibility when focus mode ends.
  // TODO(astra): Remember the previous sidebar state before focus mode
  //   started, instead of always showing it.
  if (sidebar_view_ && !IsSidebarVisible()) {
    ShowSidebar();
  }

  // Notify observers.
  for (Observer& observer : observers_) {
    observer.OnFocusModeToggled(false);
  }
}

// =========================================================================
// AstraWorkspaceServiceObserver implementation
// =========================================================================

void AstraBrowserView::OnActiveWorkspaceChanged(const std::string& old_id,
                                                const std::string& new_id) {
  // Active workspace changed — refresh the sidebar.
  //
  // The profile menu controller independently observes the workspace
  // service and handles its own updates.

  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }

  // Notify observers.
  for (Observer& observer : observers_) {
    observer.OnWorkspaceSwitched(new_id);
  }
}

// =========================================================================
// Private helper methods
// =========================================================================

void AstraBrowserView::StartObservingServices() {
  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  // Observe theme service.
  AstraThemeService* theme_service = AstraThemeService::GetForProfile(profile);
  if (theme_service && !theme_service_observation_.IsObserving()) {
    theme_service_observation_.Observe(theme_service);
  }

  // Observe focus mode service.
  AstraFocusModeService* focus_service =
      AstraFocusModeService::GetForProfile(profile);
  if (focus_service && !focus_mode_service_observation_.IsObserving()) {
    focus_mode_service_observation_.Observe(focus_service);
  }

  // Observe workspace service.
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceService::GetForProfile(profile);
  if (workspace_service && !workspace_service_observation_.IsObserving()) {
    workspace_service_observation_.Observe(workspace_service);
  }
}

void AstraBrowserView::ClearWidgetPointer(views::Widget* widget) {
  if (!widget) {
    return;
  }

  bool was_last_bubble = (GetOpenBubbleCount() == 1);

  // Clear the matching widget pointer.
  if (widget == command_palette_widget_) {
    command_palette_widget_ = nullptr;
  } else if (widget == settings_widget_) {
    settings_widget_ = nullptr;
  } else if (widget == tab_search_widget_) {
    tab_search_widget_ = nullptr;
  } else if (widget == import_export_widget_) {
    import_export_widget_ = nullptr;
  } else if (widget == screenshot_capture_widget_) {
    screenshot_capture_widget_ = nullptr;
  } else if (widget == region_overlay_widget_) {
    region_overlay_widget_ = nullptr;
  } else if (widget == new_tab_widget_) {
    new_tab_widget_ = nullptr;
  } else {
    // Unknown widget — nothing to clear.
    return;
  }

  // If this was the last open bubble, notify observers.
  if (was_last_bubble) {
    NotifyBubbleClosed();
  }
}

bool AstraBrowserView::IsTrackedBubbleWidget(views::Widget* widget) const {
  return widget == command_palette_widget_ ||
         widget == settings_widget_ ||
         widget == tab_search_widget_ ||
         widget == import_export_widget_ ||
         widget == screenshot_capture_widget_ ||
         widget == region_overlay_widget_ ||
         widget == new_tab_widget_;
}

void AstraBrowserView::NotifyBubbleOpened() {
  for (Observer& observer : observers_) {
    observer.OnAnyBubbleOpened();
  }
}

void AstraBrowserView::NotifyBubbleClosed() {
  for (Observer& observer : observers_) {
    observer.OnAllBubblesClosed();
  }
}

void AstraBrowserView::RefreshAllUiColors() {
  // Refresh colors on all Astra UI surfaces.
  //
  // Most views use the ColorProvider and will update automatically when
  // the theme changes.  For surfaces that need explicit color updates,
  // we refresh them here.
  //
  // TODO(astra): Add explicit refresh calls for Astra UI surfaces that
  //   don't automatically pick up ColorProvider changes.
  //   For the focus mode controller, this would be handled via
  //   its own theme observation or ColorProvider integration.

  if (sidebar_view_) {
    sidebar_view_->UpdateFromModel();
  }
}

}  // namespace astra
