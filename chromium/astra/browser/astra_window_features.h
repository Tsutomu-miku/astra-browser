#ifndef ASTRA_BROWSER_ASTRA_WINDOW_FEATURES_H_
#define ASTRA_BROWSER_ASTRA_WINDOW_FEATURES_H_

#include <string>
#include <utility>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/supports_user_data.h"
#include "ui/gfx/geometry/rect.h"

class Browser;

namespace astra {

// =========================================================================
// AstraWindowObserver
// =========================================================================
//
// Observer interface for Astra window feature events.
//
// All methods have default empty implementations — observers override only
// the events they care about.  This follows the same pattern as
// AstraAccessibilityObserver and AstraPipObserver.
//
// Events cover both global window lifecycle (created, closing) and per-window
// state changes (workspace, sidebar, split view).  Every event callback
// receives the AstraWindowFeatures* for the window it relates to.
//
// Chromium owner: BrowserListObserver
//   (chrome/browser/ui/browser_list_observer.h) for browser window lifecycle.
// Astra only adds metadata-level observation on top of Chromium's lifecycle.
// =========================================================================

class AstraWindowObserver : public base::CheckedObserver {
 public:
  // Called when a new browser window with Astra features is created.
  // Fired from AstraWindowFeatures::GetOrCreateForBrowser() when a new
  // instance is created for a Browser.
  virtual void OnWindowCreated(AstraWindowFeatures* window) {}

  // Called when a browser window with Astra features is being closed
  // (destructor of AstraWindowFeatures).
  virtual void OnWindowClosing(AstraWindowFeatures* window) {}

  // Called when a window's workspace assignment changes.
  // |old_workspace_id| and |new_workspace_id| are the before/after values.
  virtual void OnWindowWorkspaceChanged(AstraWindowFeatures* window,
                                        const std::string& old_workspace_id,
                                        const std::string& new_workspace_id) {}

  // Called when a window's sidebar visibility changes.
  virtual void OnWindowSidebarVisibilityChanged(AstraWindowFeatures* window,
                                                bool visible) {}

  // Called when a window's sidebar pinned state changes.
  virtual void OnWindowSidebarPinnedChanged(AstraWindowFeatures* window,
                                            bool pinned) {}

  // Called when a window's sidebar width changes.
  virtual void OnWindowSidebarWidthChanged(AstraWindowFeatures* window,
                                           int width) {}

  // Called when a window's split view active state changes.
  virtual void OnWindowSplitViewStateChanged(AstraWindowFeatures* window,
                                             bool active) {}

  // Called when any window-level Astra feature changes.
  // Catch-all notification for observers that just need to refresh their UI.
  virtual void OnWindowFeaturesChanged(AstraWindowFeatures* window) {}

 protected:
  ~AstraWindowObserver() override = default;
};

// =========================================================================
// AstraWindowFeatures
// =========================================================================
//
// Astra-only metadata attached to a Chromium-owned Browser window.
//
// This class stores ONLY product metadata that Chromium does not track.
// Do NOT mirror window bounds, tab count, profile, or other state that
// Browser already owns — those belong to Browser, TabStripModel, and
// BrowserList.
//
// Persistence model:
//   Per-window Astra metadata does NOT persist via PrefService directly.
//   Instead, it survives across browser restarts through Chromium's
//   SessionService (session restore) + window restore mechanisms.
//   Chromium owns session restore; Astra attaches extra data to each
//   window's session entry via a session restore patch point.
//
//   Default window BEHAVIORS (e.g. "new windows open maximized") DO persist
//   via PrefService as profile-level preferences.  See astra_prefs.h for
//   the window-related pref keys and defaults.
//
//   TODO(astra): Attach AstraWindowFeatures data to Chromium's session restore
//   pipeline so per-window metadata (workspace_id, placement, etc.) survives
//   browser restart and window restore.
//   Patch point: chrome/browser/sessions/base_session_service.cc or
//   sessions::SessionWindow serialization.
//   Chromium component: sessions / SessionService / SessionRestore.
//
// Attachment model:
//   Uses Browser's SupportsUserData base class, which is the standard
//   Chromium pattern for attaching arbitrary data to Browser objects.
//   This is analogous to WebContentsUserData for WebContents but uses
//   the more general SupportsUserData interface.
//
//   TODO(astra): Consider adding a dedicated BrowserUserData<> helper
//   template (similar to WebContentsUserData) for cleaner attachment.
//   Chromium owner: Browser (chrome/browser/ui/browser.h).
//   For now we use SupportsUserData directly.
//
// Cross-window operations:
//   Static methods like GetWindowCount(), GetActiveWindow(), and
//   TileWindowsInWorkspace() operate across all browser windows.  They
//   delegate to Chromium's BrowserList for window iteration and use
//   AstraWindowFeatures::FromBrowser() to access Astra metadata.
//   Chromium owns the actual window list — Astra only adds metadata and
//   convenience helpers.
//
// Observers:
//   Each AstraWindowFeatures instance has its own observer list for
//   per-window state change notifications.  There is also a static global
//   observer list for window lifecycle events (window created, window
//   closing) that are not tied to any single instance.
// =========================================================================

class AstraWindowFeatures : public base::SupportsUserData::Data {
 public:
  ~AstraWindowFeatures() override;

  // Creates the user data for |browser| if it does not already exist,
  // then returns a pointer to it.
  // Fires OnWindowCreated on global observers the first time an instance
  // is created for a given Browser.
  static AstraWindowFeatures* GetOrCreateForBrowser(Browser* browser);

  // Returns the AstraWindowFeatures for |browser|, or nullptr if none
  // has been created yet.
  static AstraWindowFeatures* FromBrowser(Browser* browser);

  // Resets all Astra window metadata to defaults. Used when a Browser
  // is repurposed (e.g., session restore) and old product metadata should
  // not carry over.
  void Reset();

  // -- Per-window observers -----------------------------------------------

  // Adds an observer for this specific window's feature changes.
  // Observers added here receive per-window events: workspace changes,
  // sidebar changes, split view changes, etc.
  void AddObserver(AstraWindowObserver* observer);
  void RemoveObserver(AstraWindowObserver* observer);

  // -- Global observers ----------------------------------------------------

  // Adds a global observer that receives events from all windows,
  // including window lifecycle events (created, closing).
  static void AddGlobalObserver(AstraWindowObserver* observer);
  static void RemoveGlobalObserver(AstraWindowObserver* observer);

  // -- Workspace ----------------------------------------------------------

  const std::string& workspace_id() const { return workspace_id_; }
  void set_workspace_id(std::string workspace_id);

  bool IsInDefaultWorkspace() const { return workspace_id_ == "default"; }

  // -- Window ordering ---------------------------------------------------

  // Order index of this window within its workspace.
  // Used by AstraWorkspaceWindowManager for ordered window lists and
  // ReorderWindowsInWorkspace.  Lower index = earlier in the list.
  //
  // This is Astra-level presentation metadata — Chromium's BrowserList
  // does not track workspace-specific window ordering.
  size_t order_index() const { return order_index_; }
  void set_order_index(size_t index) { order_index_ = index; }

  // -- Sidebar state per window -------------------------------------------
  //
  // Per-window sidebar state.  These are runtime values that start from
  // profile-level defaults (astra_prefs.h) and can be overridden per window.
  // Per-window state is not persisted via PrefService — it survives through
  // session restore metadata attachment.

  // Whether the sidebar is visible in this window.
  bool sidebar_visible() const { return sidebar_visible_; }
  void set_sidebar_visible(bool visible);

  // Whether the sidebar is pinned open in this window.
  // When unpinned, the sidebar slides in as an overlay on hover.
  bool sidebar_pinned() const { return sidebar_pinned_; }
  void set_sidebar_pinned(bool pinned);

  // Width of the sidebar in pixels for this window.
  int sidebar_width() const { return sidebar_width_; }
  void set_sidebar_width(int width);

  // Toggles sidebar visibility.  Returns the new visibility state.
  bool ToggleSidebar();

  // -- Split view state per window ----------------------------------------
  //
  // Per-window split view state.  Split view divides a single browser
  // window into two side-by-side web content panes.
  //
  // Chromium owner: TabStripModel + content::WebContents for the actual
  //   split view rendering.  Astra adds metadata and presentation state.
  // TODO(astra): Wire split view to Chromium's tab side-by-side feature
  //   or implement as a split WebView arrangement.

  // Whether split view is currently active in this window.
  bool split_view_active() const { return split_view_active_; }
  void set_split_view_active(bool active);

  // Split view orientation: "horizontal" (side by side) or "vertical"
  // (top and bottom).
  const std::string& split_view_orientation() const {
    return split_view_orientation_;
  }
  void set_split_view_orientation(std::string orientation);

  // Split view ratio: 0.0 to 1.0, representing the size of the first pane
  // relative to the total window size.  0.5 means equal split.
  double split_view_ratio() const { return split_view_ratio_; }
  void set_split_view_ratio(double ratio);

  // Toggles split view on/off.  Returns the new active state.
  bool ToggleSplitView();

  // -- Window placement (saved state for workspace recall) ----------------

  // Saved window bounds (screen coordinates). Used for multi-monitor
  // workspace restore — each workspace's windows remember their monitor
  // positions across workspace switches and session restores.
  //
  // Chromium owns the actual window bounds (Browser window state).
  // These stored bounds are used for workspace-switch recall: when
  // switching back to a workspace, windows are restored to their saved
  // positions on their original monitors.
  //
  // TODO(astra): Integrate with display::Display for multi-monitor
  // positioning.  Chromium component: display::Screen / display::Display.
  const gfx::Rect& saved_bounds() const { return saved_bounds_; }
  void set_saved_bounds(const gfx::Rect& bounds) { saved_bounds_ = bounds; }

  // Whether the window was minimized when its workspace was switched away.
  // When switching back to the workspace, the window is restored to its
  // previous state (minimized or not).
  bool is_minimized() const { return is_minimized_; }
  void set_is_minimized(bool minimized) { is_minimized_ = minimized; }

  // Whether the window was maximized when its workspace was switched away.
  bool is_maximized() const { return is_maximized_; }
  void set_is_maximized(bool maximized) { is_maximized_ = maximized; }

  // Whether the window was in fullscreen mode when its workspace was
  // switched away.
  bool is_fullscreen() const { return is_fullscreen_; }
  void set_is_fullscreen(bool fullscreen) { is_fullscreen_ = fullscreen; }

  // -- Window state queries -----------------------------------------------

  // Returns true if the window is in normal (non-minimized, non-maximized,
  // non-fullscreen) state according to saved workspace recall state.
  bool IsWindowStateNormal() const {
    return !is_minimized_ && !is_maximized_ && !is_fullscreen_;
  }

  // -- Fullscreen helpers -------------------------------------------------

  // Toggles the fullscreen metadata state for this window.
  // Returns the new fullscreen state.
  //
  // Note: this toggles the Astra metadata.  The actual Chromium fullscreen
  // toggle is owned by Browser::ToggleFullscreenMode().  This method is for
  // workspace-switch recall and Astra UI projection.
  //
  // Chromium owner: Browser::ToggleFullscreenMode()
  //   (chrome/browser/ui/browser.h)
  bool ToggleFullscreen();

  // -- Incognito compatibility -------------------------------------------

  // Returns true if this window's workspace assignment is read-only.
  // In incognito mode, windows always belong to the default workspace
  // and cannot be moved to other workspaces.
  //
  // See AstraIncognitoHandler for the design rationale.
  // Chromium owner: Profile::IsOffTheRecord()
  bool workspace_read_only() const { return workspace_read_only_; }

  // -- Hibernation --------------------------------------------------------

  // Whether this window is in a hibernated state.
  // Hibernated windows have their tabs unloaded to save memory.
  // The window itself remains open but its WebContents are discarded.
  //
  // This is Astra-level presentation metadata — actual tab discarding is
  // done by Chromium's TabStripModel / memory manager.
  //
  // TODO(astra): Integrate with Chromium's tab discarding / memory saver.
  // Chromium component: TabStripModel::DiscardWebContentsAt.
  bool is_hibernated() const { return is_hibernated_; }
  void set_is_hibernated(bool hibernated) { is_hibernated_ = hibernated; }

  // Key used to identify this data type in SupportsUserData.
  static const void* UserDataKey();

  // -- Window management (static, uses BrowserList) -----------------------
  //
  // Cross-window convenience helpers.  These iterate Chromium's BrowserList
  // and project Astra metadata from each window.
  //
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  //   BrowserList owns the actual list of all browser windows.
  //   These methods are Astra convenience wrappers that filter/query by
  //   Astra metadata (e.g. workspace).

  // Returns the total number of browser windows.
  static size_t GetWindowCount();

  // Returns the most recently active browser window's Astra features,
  // or nullptr if there are no windows.
  static AstraWindowFeatures* GetActiveWindow();

  // Returns all windows that belong to the given workspace.
  // Windows are returned in the order they appear in BrowserList
  // (most-recently-active first, typically).
  static std::vector<AstraWindowFeatures*> GetWindowsByWorkspace(
      const std::string& workspace_id);

  // -- Window arrangement helpers (static) --------------------------------

  // Tiles all windows in the given workspace side by side (horizontal tile).
  // Windows are resized to equal widths and placed left-to-right.
  //
  // TODO(astra): Implement actual window positioning using views::Widget
  //   and display::Screen.  Currently this is a metadata-level helper that
  //   updates saved_bounds_ for each window.  The actual window movement
  //   would be done through Chromium's Widget/BrowserWindow API.
  //
  // Chromium owner: views::Widget (ui/views/widget/widget.h)
  // Chromium owner: display::Screen (ui/display/screen.h)
  static void TileWindowsInWorkspace(const std::string& workspace_id);

  // Stacks all windows in the given workspace: same position, same size.
  // All windows get the same bounds (first window's saved bounds or a
  // default).
  //
  // TODO(astra): Implement actual window stacking through Chromium's
  //   widget system.  Currently updates saved_bounds_ only.
  static void StackWindowsInWorkspace(const std::string& workspace_id);

  // -- Bulk operations ----------------------------------------------------

  // Closes all windows in the given workspace.
  //
  // Note: this is an Astra convenience helper.  The actual window closing
  // is done by Chromium's Browser / BrowserWindow API.
  //
  // Chromium owner: Browser::window()->Close()
  // Chromium owner: BrowserList
  //
  // TODO(astra): Implement using BrowserList + Browser::window()->Close().
  //   Currently this is a metadata-level helper stub.
  static void CloseAllWindowsInWorkspace(const std::string& workspace_id);

  // -- Test helpers --------------------------------------------------------

  // Creates a standalone AstraWindowFeatures instance for testing without
  // needing a real Browser object.  Used by unit tests.
  static std::unique_ptr<AstraWindowFeatures> CreateForTesting();

 private:
  // The user data key used with SupportsUserData.
  static const int kUserDataKey;

  // Returns the static global observer list.
  static base::ObserverList<AstraWindowObserver>& GetGlobalObservers();

  // Notifies global observers of window creation.
  void NotifyWindowCreated();

  // Notifies global observers of window closing.
  void NotifyWindowClosing();

  // Notifies both per-instance and global observers of a features change.
  void NotifyFeaturesChanged();

  // Private constructor — use GetOrCreateForBrowser() or CreateForTesting().
  explicit AstraWindowFeatures(Browser* browser);

  // Helper to clamp split view ratio to valid range [0.1, 0.9].
  static double ClampSplitRatio(double ratio);

  // Helper to clamp sidebar width to valid range [120, 600].
  static int ClampSidebarWidth(int width);

  // Per-instance observers for this window.
  base::ObserverList<AstraWindowObserver> observers_;

  // Workspace membership for this window.
  // All tabs in the window are typically in the same workspace, but the
  // source of truth is per-tab (AstraTabFeatures). This window-level
  // workspace_id is used for window visibility projection during workspace
  // switching and for new-tab workspace assignment.
  std::string workspace_id_ = "default";

  // Order index within the workspace's window list.
  size_t order_index_ = 0;

  // -- Sidebar runtime state ----------------------------------------------

  bool sidebar_visible_ = true;
  bool sidebar_pinned_ = true;
  int sidebar_width_ = 280;

  // -- Split view runtime state -------------------------------------------

  bool split_view_active_ = false;
  std::string split_view_orientation_ = "horizontal";
  double split_view_ratio_ = 0.5;

  // -- Saved window state for workspace-switch recall ---------------------

  gfx::Rect saved_bounds_;
  bool is_minimized_ = false;
  bool is_maximized_ = false;
  bool is_fullscreen_ = false;

  // Whether the window is hibernated (tabs unloaded).
  bool is_hibernated_ = false;

  // When true, workspace_id cannot be changed (incognito windows).
  bool workspace_read_only_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WINDOW_FEATURES_H_
