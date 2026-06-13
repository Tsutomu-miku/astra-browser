#ifndef ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_
#define ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_observation.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/views/widget/widget_observer.h"
#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_theme_service.h"
#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/command_palette/astra_command_palette_bubble.h"
#include "astra/ui/views/settings/astra_settings_bubble.h"
#include "astra/ui/views/tab_search/astra_tab_search_bubble.h"
#include "astra/ui/views/newtab/astra_new_tab_bubble.h"
#include "astra/ui/views/screenshot/astra_screenshot_capture_bubble.h"
#include "astra/ui/views/screenshot/astra_screenshot_region_overlay.h"
#include "astra/ui/views/workspace/astra_workspace_import_export_dialog.h"

class BrowserView;

namespace astra {

class AstraSidebarView;
class AstraSplitViewController;
class AstraWorkspaceOverviewController;
class AstraGlanceViewController;
class AstraProfileMenuController;
class AstraWorkspaceImportExportDialog;
class AstraFocusModeController;
class AstraScreenshotCaptureBubble;
class AstraScreenshotRegionOverlay;
class AstraNewTabBubble;
enum class AstraScreenshotType;

// Identifies each bubble/widget type managed by AstraBrowserView.
//
// Used by utility methods like CloseAllBubbles(), GetOpenBubbleCount(),
// and observer notifications like OnAnyBubbleOpened() / OnAllBubblesClosed().
enum class BubbleType {
  kCommandPalette = 0,
  kSettings = 1,
  kTabSearch = 2,
  kImportExport = 3,
  kScreenshotCapture = 4,
  kRegionOverlay = 5,
  kNewTab = 6,
};

// Controller that augments Chrome's BrowserView instead of replacing the whole
// desktop UI stack. This keeps toolbar, tab strip, WebUI, DevTools, download
// UI, and profile plumbing on Chromium rails.
//
// AstraBrowserView is NOT a View subclass. It is a coordinator object that
// installs Astra UI surfaces (sidebar, command palette, etc.) into an existing
// BrowserView and wires them to Chromium models and command infrastructure.
//
// Chromium owns: BrowserView, TabStripModel, WebContents, toolbar,
//                content area, all browser commands.
// Astra adds:   sidebar view, workspace switcher, split view UI,
//                command palette.
//
// Implements AstraCommandDelegate::Observer to receive UI-level command
// notifications from the command delegate. This follows Chromium's
// Browser / BrowserView separation pattern: browser-layer logic is model-only;
// UI concerns live in the views layer and react through observer interfaces.
//
// Implements AstraCommandPaletteBubble::Delegate to receive command execution
// requests from the command palette and dispatch them through the appropriate
// channel: Chrome commands go through BrowserCommandController, Astra commands
// go through AstraCommandDelegate.
//
// TODO(astra): Integrate with BrowserView's lifecycle via a patch point in
// chrome/browser/ui/views/frame/browser_view.cc. The patch should create an
// AstraBrowserView instance in BrowserView::Init() and destroy it in
// ~BrowserView().
class AstraBrowserView : public views::WidgetObserver,
                         public AstraThemeServiceObserver,
                         public AstraFocusModeServiceObserver,
                         public AstraWorkspaceServiceObserver,
                         public AstraCommandDelegate::Observer,
                         public AstraCommandPaletteBubble::Delegate,
                         public AstraSettingsBubble::Delegate,
                         public AstraTabSearchBubble::Delegate,
                         public AstraNewTabBubble::Delegate,
                         public AstraScreenshotCaptureBubble::Delegate,
                         public AstraScreenshotRegionOverlay::Delegate {
 public:
  // Observer interface for AstraBrowserView state changes.
  //
  // Other components can observe AstraBrowserView to react to UI state
  // changes such as sidebar visibility, focus mode toggles, workspace
  // switches, and bubble open/close events.
  //
  // All methods have empty default implementations so observers only need
  // to override the ones they care about.
  class Observer : public base::CheckedObserver {
   public:
    // Called when AstraBrowserView finishes installing its UI into the
    // BrowserView (Install() completes successfully).
    virtual void OnAstraBrowserViewInstalled() {}

    // Called when AstraBrowserView removes its UI from the BrowserView
    // (Uninstall() completes).
    virtual void OnAstraBrowserViewUninstalled() {}

    // Called when sidebar visibility changes.
    // |visible| is true if the sidebar is now visible, false if hidden.
    virtual void OnSidebarVisibilityChanged(bool visible) {}

    // Called when focus mode is toggled on or off.
    // |active| is true if focus mode is now active.
    virtual void OnFocusModeToggled(bool active) {}

    // Called when the active workspace switches.
    // |workspace_id| is the ID of the newly active workspace.
    virtual void OnWorkspaceSwitched(const std::string& workspace_id) {}

    // Called when the first bubble opens (transition from 0 to 1).
    virtual void OnAnyBubbleOpened() {}

    // Called when the last bubble closes (transition from 1 to 0).
    virtual void OnAllBubblesClosed() {}

   protected:
    ~Observer() override = default;
  };

  explicit AstraBrowserView(BrowserView* browser_view);
  AstraBrowserView(const AstraBrowserView&) = delete;
  AstraBrowserView& operator=(const AstraBrowserView&) = delete;
  ~AstraBrowserView() override;

  // -- Observers ---------------------------------------------------------

  // Adds an observer to receive state change notifications.
  void AddObserver(Observer* observer);

  // Removes an observer.
  void RemoveObserver(Observer* observer);

  // Installs Astra UI into the BrowserView's layout.
  // Safe to call multiple times — subsequent calls are no-ops.
  void Install();

  // Removes all Astra UI from the BrowserView.
  // Safe to call even if Install() was never called.
  void Uninstall();

  // Refreshes sidebar content from underlying models.
  void UpdateSidebar();

  // -- Sidebar visibility control ----------------------------------------

  void ShowSidebar();
  void HideSidebar();
  void ToggleSidebar();
  bool IsSidebarVisible() const;

  // Sidebar pin state (always visible vs auto-hide).
  // TODO(astra): Implement pin behavior — currently just tracks the flag.
  void ToggleSidebarPin();
  bool IsSidebarPinned() const { return sidebar_pinned_; }

  // -- Command palette ---------------------------------------------------

  // Show the command palette bubble, anchored to the toolbar area.
  void ShowCommandPalette();

  // Hide the command palette if it's open.
  void HideCommandPalette();

  // Returns true if the command palette is currently open.
  bool IsCommandPaletteOpen() const;

  // -- Tab search --------------------------------------------------------

  // Show the tab search bubble, anchored to the tab strip or toolbar area.
  void ShowTabSearch();

  // Hide the tab search bubble if it's open.
  void HideTabSearch();

  // Returns true if the tab search bubble is currently open.
  bool IsTabSearchOpen() const;

  // -- Settings ----------------------------------------------------------

  // Show the Astra settings bubble, anchored to the sidebar or toolbar.
  void ShowSettings();

  // Hide the settings bubble if it's open.
  void HideSettings();

  // Returns true if the settings bubble is currently open.
  bool IsSettingsOpen() const;

  // -- Workspace overview ------------------------------------------------

  // Show the all-workspaces overview overlay.
  void ShowWorkspaceOverview();

  // Hide the workspace overview if it's open.
  void HideWorkspaceOverview();

  // Returns true if the workspace overview is currently visible.
  bool IsWorkspaceOverviewVisible() const;

  // Accessor for the workspace overview controller (may be null if not
  // created yet).  The controller is created lazily on first Show.
  AstraWorkspaceOverviewController* workspace_overview_controller() {
    return workspace_overview_controller_.get();
  }
  const AstraWorkspaceOverviewController* workspace_overview_controller()
      const {
    return workspace_overview_controller_.get();
  }

  // -- Workspace import / export -----------------------------------------

  // Show the workspace import/export dialog.
  //
  // |mode| determines whether the dialog shows export or import UI.
  // The dialog is anchored to the toolbar or sidebar area.
  //
  // Truth source: AstraWorkspaceService + AstraWorkspaceImportExport.
  // UI layer: AstraWorkspaceImportExportDialog (BubbleDialogDelegateView).
  //
  // TODO(astra): Proper dialog anchoring — the dialog should be anchored
  //   to the button or menu item that triggered it.
  void ShowImportExportDialog(
      AstraWorkspaceImportExportDialog::Mode mode);

  // Hide the import/export dialog if it's open.
  void HideImportExportDialog();

  // Returns true if the import/export dialog is currently open.
  bool IsImportExportDialogVisible() const;

  // -- Split view control ------------------------------------------------

  // Toggle split view on/off for the active tab.  When activating, uses
  // the next tab in TabStripModel as the split partner.
  void ToggleSplitView();

  // Show split view with a specific primary/secondary pair and orientation.
  void ShowSplitView(SplitViewOrientation orientation);

  // Hide split view and restore single-tab layout.
  void HideSplitView();

  // Returns true if split view is currently active.
  bool IsSplitViewActive() const;

  // Swap the primary and secondary views in split view.
  void SwapSplitViews();

  // Accessor for the split view controller (may be null if not installed).
  AstraSplitViewController* split_view_controller() {
    return split_view_controller_.get();
  }
  const AstraSplitViewController* split_view_controller() const {
    return split_view_controller_.get();
  }

  // -- Glance / peek control ---------------------------------------------

  // Show a glance preview of |for_contents| anchored near the sidebar item
  // or link that triggered it.
  // |anchor| is in the anchor view's coordinate space.
  void ShowGlance(content::WebContents* for_contents,
                  const gfx::Rect& anchor);

  // Show a glance preview for the given |url|, creating a temporary
  // WebContents for the preview.
  void ShowGlanceForURL(const GURL& url, const gfx::Rect& anchor);

  // Hide the glance if it's visible.
  void HideGlance();

  // Returns true if the glance view is currently visible.
  bool IsGlanceVisible() const;

  // Accessor for the glance view controller (may be null if not created).
  // The controller is created lazily on first ShowGlance.
  AstraGlanceViewController* glance_controller() {
    return glance_controller_.get();
  }
  const AstraGlanceViewController* glance_controller() const {
    return glance_controller_.get();
  }

  // -- Workspace menu / profile area -------------------------------------

  // Show the workspace switcher menu, anchored near the avatar button
  // in the top-right of the toolbar.
  //
  // TODO(astra): Anchor to the actual avatar toolbar button when
  // integrated with BrowserView.  Currently the anchor is computed
  // from the BrowserView bounds as a placeholder.
  // Chromium owner: AvatarToolbarButton
  //   (chrome/browser/ui/views/toolbar/avatar_toolbar_button.h)
  void ShowWorkspaceMenu();

  // Hide the workspace menu if it's open.
  void HideWorkspaceMenu();

  // Returns true if the workspace menu is currently visible.
  bool IsWorkspaceMenuVisible() const;

  // Accessor for the profile menu controller (may be null if not
  // created).  The controller is created lazily on first use.
  AstraProfileMenuController* profile_menu_controller() {
    return profile_menu_controller_.get();
  }
  const AstraProfileMenuController* profile_menu_controller() const {
    return profile_menu_controller_.get();
  }

  // -- Focus mode ---------------------------------------------------------

  // Toggles focus mode on/off. Focus mode hides distractions (sidebar,
  // toolbar clutter) and shows a subtle focus mode indicator.
  //
  // Truth source: AstraFocusModeService (profile-scoped keyed service).
  // This method is the UI entry point — it calls through to the service
  // and the service notifies observers of state changes.
  //
  // Chromium subsystems reused:
  //   - PrefService (default duration + blocklist persistence).
  //   - FullscreenController / ImmersiveModeController (future).
  //   - HostContentSettingsMap (future, for site blocking).
  void ToggleFocusMode();

  // Returns true if focus mode is currently active for this window.
  // This reads the state from the service — it's a projection, not truth.
  bool IsFocusModeActive() const;

  // Accessor for the focus mode controller (may be null if focus mode
  // has never been activated in this window).
  //
  // The controller is created lazily on first focus mode activation.
  // It's owned by AstraBrowserView via unique_ptr.
  AstraFocusModeController* focus_mode_controller() {
    return focus_mode_controller_.get();
  }
  const AstraFocusModeController* focus_mode_controller() const {
    return focus_mode_controller_.get();
  }

  // -- New tab page -------------------------------------------------------

  // Show the Astra new tab page as a full-window overlay / bubble.
  //
  // The new tab page shows workspace shortcuts, frequently visited sites,
  // and quick actions. It is an alternative to Chrome's WebUI NTP.
  //
  // Truth source: AstraWorkspaceService + Chrome's most visited sites.
  // UI layer: AstraNewTabBubble (BubbleDialogDelegateView pattern).
  //
  // TODO(astra): Integrate with Chrome's new tab creation path so that
  //   pressing Ctrl/Cmd+T or clicking the new tab button shows the Astra
  //   NTP instead of the Chrome NTP.
  //   Chromium owner: NewTabPageUI / Browser::NewTab()
  //   Patch point: chrome/browser/ui/browser.cc — NewTab() method.
  void ShowNewTabPage();

  // Hide the new tab page if it's open.
  void HideNewTabPage();

  // Returns true if the new tab page is currently visible.
  bool IsNewTabPageVisible() const;

  // -- Bubble utilities ---------------------------------------------------

  // Closes all open bubbles and palettes.
  // Safe to call even if no bubbles are open.
  void CloseAllBubbles();

  // Returns true if any bubble is currently open.
  bool IsAnyBubbleOpen() const;

  // Returns the number of currently open bubbles.
  int GetOpenBubbleCount() const;

  // -- Convenience accessors ----------------------------------------------

  // Returns the active WebContents from the browser's tab strip model.
  // Returns nullptr if there is no active tab.
  content::WebContents* GetActiveWebContents() const;

  // Returns the Browser associated with this browser view.
  // Returns nullptr if the browser view is null.
  Browser* GetBrowser() const;

  // Returns the Profile associated with this browser view.
  // Returns nullptr if the browser or profile is not available.
  Profile* GetProfile() const;

  // -- Accessors ---------------------------------------------------------

  // Accessor for the sidebar view (may be null if not installed).
  AstraSidebarView* sidebar_view() { return sidebar_view_; }
  const AstraSidebarView* sidebar_view() const { return sidebar_view_; }

  // -- Tab strip model notifications -------------------------------------

  // DEPRECATED: The sidebar now observes TabStripModel directly via
  // TabStripModelObserver (see AstraSidebarView). This method is kept as
  // a compatibility fallback — it triggers a full sidebar rebuild.
  //
  // Primary update path: AstraSidebarView observes TabStripModel directly
  // via base::ScopedObservation, receiving granular change notifications.
  // This old push path is only used when the observer is not yet wired up
  // through the Chromium BrowserView patch point.
  //
  // TODO(astra): Remove this method once the TabStripModelObserver path is
  // fully wired through the Chromium BrowserView patch and is proven stable.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  void OnTabStripModelChanged();

  // -- AstraCommandDelegate::Observer --------------------------------------

  void OnToggleSidebar() override;
  void OnToggleSidebarPin() override;
  void OnOpenCommandPalette() override;
  void OnOpenTabSearch() override;
  void OnOpenGlance() override;
  void OnShowAllWorkspaces() override;
  void OnSplitViewStateChanged() override;
  void OnFavoriteFoldersChanged() override;
  void OnOpenSettings() override;
  void OnSwitchWorkspaceMenu() override;
  void OnExportWorkspaces() override;
  void OnImportWorkspaces() override;
  void OnWorkspaceNavigateRequested(int direction) override;
  void OnFocusOmniboxCommandMode() override;
  void OnToggleFocusMode() override;

  // -- AstraCommandDelegate::Observer: recently closed tabs ---------------

  // Called when a recently closed tab is restored. The sidebar's
  // recently closed section should refresh its data.
  //
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  //
  // TODO(astra): Consider whether this notification is needed or if the
  //   UI should observe TabRestoreService directly. Currently routed
  //   through the command delegate for convenience.
  void OnRecentlyClosedTabRestored() override;

  // Called when all recently closed tabs are restored at once.
  void OnAllRecentlyClosedTabsRestored() override;

  // -- AstraCommandDelegate::Observer: search settings --------------------

  // Called to open search engine settings. This can either show the
  // search section of the Astra settings bubble or open Chrome's
  // search engine settings WebUI.
  //
  // Chromium owner: TemplateURLService
  //   (components/search_engines/template_url_service.h)
  // Chromium WebUI: chrome://settings/searchEngines
  //
  // TODO(astra): Decide whether to open Astra settings search section
  //   or Chrome's native search settings page. Currently opens Chrome
  //   settings via NavigateToURL.
  void OnOpenSearchSettings() override;

  // -- AstraCommandDelegate::Observer: extensions panel -------------------

  // Called to toggle the extensions panel in the sidebar.
  //
  // The extensions panel shows installed extensions and their actions.
  // State (visibility, pinning) is owned by the sidebar / Chrome's
  // extensions system, not by this view.
  //
  // Chromium owner: ExtensionService / ExtensionsToolbarContainer
  //   (chrome/browser/extensions/extension_service.h)
  //   (chrome/browser/ui/views/extensions/extensions_toolbar_container.h)
  //
  // TODO(astra): The extensions panel is a sidebar section managed by
  //   AstraSidebarView. This method delegates to the sidebar, similar to
  //   how OnToggleSidebar works. Consider moving this logic entirely to
  //   the sidebar's observer.
  void OnToggleExtensionsPanel() override;

  // -- AstraCommandDelegate::Observer: screenshots ------------------------

  // Called to capture a screenshot of the visible area.
  //
  // The capture is initiated through AstraScreenshotService, which
  // performs the actual capture asynchronously. When complete, the
  // service notifies AstraScreenshotServiceObserver, and the capture
  // bubble is shown.
  //
  // Chromium owner: content::WebContents (visible area capture)
  //   chrome/browser/screenshot/ (full page, if available)
  // Astra service: AstraScreenshotService (ProfileKeyedService)
  //
  // TODO(astra): Observe AstraScreenshotService to show the capture
  //   bubble when the bitmap is ready. Currently the service does the
  //   capture stubs but the UI doesn't react to completion yet.
  void OnScreenshotVisible() override;

  // Called to capture a full-page screenshot.
  void OnScreenshotFullPage() override;

  // Called to start a region capture screenshot. Shows the region
  // selection overlay; the actual capture happens when the user
  // confirms the selection.
  //
  // The region overlay is a full-window transparent overlay that lets
  // the user drag a rectangle to select the capture area.
  void OnScreenshotRegion() override;

  // -- AstraCommandDelegate::Observer: new tab page -----------------------

  // Called to open the Astra new tab page.
  //
  // The Astra NTP is a Views-based surface that shows workspace
  // shortcuts, most visited sites, and quick actions. It is shown as
  // a full-window overlay rather than as a WebUI page.
  //
  // Chromium owner: NewTabPageUI / Browser::NewTab()
  //   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
  // Patch point: chrome/browser/ui/browser.cc — NewTab() method.
  //
  // TODO(astra): Integrate with Chrome's new tab creation so that
  //   Ctrl/Cmd+T and the new tab button show the Astra NTP.
  void OnOpenNewTabPage() override;

  // -- AstraCommandPaletteBubble::Delegate --------------------------------

  void OnCommandPaletteExecute(int command_id, bool is_astra) override;
  void OnCommandPaletteClosed() override;

  // -- AstraSettingsBubble::Delegate -------------------------------------

  void OnSettingsBubbleClosed() override;

  // -- AstraTabSearchBubble::Delegate -----------------------------------

  void OnTabSearchBubbleClosed() override;

  // -- AstraNewTabBubble::Delegate ---------------------------------------

  void OnNewTabBubbleClosed() override;

  // -- AstraScreenshotCaptureBubble::Delegate ----------------------------

  void OnScreenshotSave() override;
  void OnScreenshotCopy() override;
  void OnScreenshotEdit() override;
  void OnScreenshotDelete() override;
  void OnScreenshotBubbleClosed() override;

  // -- AstraScreenshotRegionOverlay::Delegate ----------------------------

  void OnRegionSelected(const gfx::Rect& region) override;
  void OnRegionSelectionCancelled() override;

  // -- views::WidgetObserver ---------------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;

  // -- AstraThemeServiceObserver -----------------------------------------

  void OnThemeChanged() override;
  void OnAccentColorChanged(SkColor new_accent_color) override;
  void OnThemePresetChanged(AstraThemePreset preset) override;

  // -- AstraFocusModeServiceObserver -------------------------------------

  void OnFocusModeEntered(base::TimeDelta duration) override;
  void OnFocusModeExited() override;

  // -- AstraWorkspaceServiceObserver -------------------------------------

  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override;

 private:
  friend class AstraBrowserViewTest;  // For unit tests to access internals.

  // Helper: starts observing the relevant services if a profile is
  // available.  Called from Install() after the browser view is confirmed.
  // Observations are automatically cleaned up by ScopedObservation dtor.
  void StartObservingServices();

  // Helper: clears a widget pointer and notifies observers if this was
  // the last open bubble.  Used by OnWidgetDestroying() and the
  // bubble-closed delegate callbacks.
  void ClearWidgetPointer(views::Widget* widget);

  // Helper: returns true if the given widget pointer is one of the
  // tracked bubble widgets (non-null and matches a known member).
  bool IsTrackedBubbleWidget(views::Widget* widget) const;

  // Helper: fires OnAnyBubbleOpened() if this is the first bubble.
  void NotifyBubbleOpened();

  // Helper: fires OnAllBubblesClosed() if no bubbles are open.
  void NotifyBubbleClosed();

  // Helper: refreshes all Astra UI colors when the theme changes.
  void RefreshAllUiColors();

  raw_ptr<BrowserView> browser_view_;
  raw_ptr<AstraSidebarView> sidebar_view_ = nullptr;
  bool installed_ = false;
  bool sidebar_pinned_ = true;

  // Observers of this AstraBrowserView instance.
  base::ObserverList<Observer> observers_;

  // Service observations.
  // These are automatically cleaned up by ScopedObservation's destructor.
  base::ScopedObservation<AstraThemeService, AstraThemeServiceObserver>
      theme_service_observation_{this};
  base::ScopedObservation<AstraFocusModeService, AstraFocusModeServiceObserver>
      focus_mode_service_observation_{this};
  base::ScopedObservation<AstraWorkspaceService, AstraWorkspaceServiceObserver>
      workspace_service_observation_{this};

  // Split view controller (created on demand).
  // Uses a unique_ptr because the controller is owned by AstraBrowserView
  // and should be destroyed with it.  The split view widget itself is
  // owned by the Views hierarchy.
  std::unique_ptr<AstraSplitViewController> split_view_controller_;

  // Command palette widget.  Nullptr when the palette is closed.
  // The widget is owned by the native widget system; we just keep a
  // raw pointer to know if it's open and to close it programmatically.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally (e.g. user clicks outside).
  raw_ptr<views::Widget> command_palette_widget_ = nullptr;

  // Settings bubble widget.  Nullptr when the bubble is closed.
  // The widget is owned by the native widget system; we just keep a
  // raw pointer to know if it's open and to close it programmatically.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally.
  raw_ptr<views::Widget> settings_widget_ = nullptr;

  // Tab search bubble widget.  Nullptr when the bubble is closed.
  // The widget is owned by the native widget system; we just keep a
  // raw pointer to know if it's open and to close it programmatically.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally (e.g. user clicks outside).
  raw_ptr<views::Widget> tab_search_widget_ = nullptr;

  // Workspace overview controller (created on demand).
  // Manages the full-window workspace overview overlay.
  // Owned by AstraBrowserView via unique_ptr.
  std::unique_ptr<AstraWorkspaceOverviewController>
      workspace_overview_controller_;

  // Glance / peek controller (created on demand).
  // Manages the glance preview bubble and its WebContents.
  // Owned by AstraBrowserView via unique_ptr.
  std::unique_ptr<AstraGlanceViewController> glance_controller_;

  // Profile menu / workspace avatar controller (created on demand).
  // Manages workspace switching in the profile menu / avatar area.
  // Owned by AstraBrowserView via unique_ptr.
  //
  // TODO(astra): Decide where to place the workspace indicator in the
  // toolbar.  Options:
  //   1. Replace AvatarToolbarButton with AstraWorkspaceAvatarButton
  //      (which wraps the avatar and adds workspace badge).
  //   2. Add AstraWorkspaceAvatarButton as a separate button next to
  //      the avatar button in the toolbar.
  //   3. Embed the workspace section directly into the profile menu
  //      via the ProfileMenuView patch point.
  //
  // Chromium owner: ToolbarView / AvatarToolbarButton
  //   (chrome/browser/ui/views/toolbar/toolbar_view.h,
  //    chrome/browser/ui/views/toolbar/avatar_toolbar_button.h)
  // Patch point: ToolbarView::Init() — insert the workspace avatar
  //   button into the toolbar view hierarchy.
  std::unique_ptr<AstraProfileMenuController> profile_menu_controller_;

  // Focus mode controller (created on demand).
  // Manages focus mode UI changes: sidebar hiding, toolbar minimization,
  // and the floating focus mode indicator widget.
  //
  // Truth source: AstraFocusModeService (profile-scoped keyed service).
  // The controller is the projection layer — it never stores truth state.
  // It reads focus mode state from the service and applies UI changes.
  //
  // Chromium subsystems reused:
  //   - FullscreenController (for potential immersive mode).
  //   - ImmersiveModeController (for toolbar auto-hide).
  //   - Widget / Views framework (for the indicator).
  //
  // TODO(astra): Use Chromium's immersive mode / fullscreen APIs for
  // toolbar auto-hide instead of manual visibility toggling.
  // Chromium owner: ImmersiveModeController
  //   (chrome/browser/ui/views/frame/immersive_mode_controller.h)
  // Patch point: BrowserView::immersive_mode_controller().
  std::unique_ptr<AstraFocusModeController> focus_mode_controller_;

  // Import/export dialog widget.  Nullptr when the dialog is closed.
  // The widget is owned by the native widget system; we just keep a
  // raw pointer to know if it's open and to close it programmatically.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally.
  raw_ptr<views::Widget> import_export_widget_ = nullptr;

  // Screenshot capture bubble widget.  Nullptr when the bubble is closed.
  // Shown after a screenshot capture completes.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally.
  raw_ptr<views::Widget> screenshot_capture_widget_ = nullptr;

  // Screenshot region selection overlay widget.  Nullptr when the
  // overlay is not active.  Shown for region screenshot capture.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally.
  raw_ptr<views::Widget> region_overlay_widget_ = nullptr;

  // New tab page bubble widget.  Nullptr when the NTP is closed.
  // We observe the widget via views::WidgetObserver to clear the pointer
  // when the widget is destroyed externally.
  raw_ptr<views::Widget> new_tab_widget_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_ASTRA_BROWSER_VIEW_H_
