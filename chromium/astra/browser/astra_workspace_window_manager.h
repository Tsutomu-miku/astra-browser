#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_WINDOW_MANAGER_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_WINDOW_MANAGER_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

class Browser;
class PrefService;
class Profile;

namespace astra {

class AstraWorkspaceWindowManagerObserver;

// Tile direction for window arrangement in a workspace.
enum class AstraTileDirection {
  kHorizontal,  // Windows side-by-side left-to-right
  kVertical,    // Windows top-to-bottom
  kGrid,        // Windows in a grid (rows and columns)
};

// Behavior for new browser windows.
enum class AstraNewWindowBehavior {
  kDefaultWorkspace,  // New windows always open in the default workspace
  kActiveWorkspace,   // New windows open in the currently active workspace
  kNewWorkspace,      // New windows always create a new workspace
  kAskUser,           // Ask the user where to place new windows
};

// Manages the mapping between workspaces and browser windows.
//
// This is the central coordinator for multi-window workspace support.  It observes
// Chromium's BrowserList to track browser window creation and destruction,
// and maintains the workspace-to-windows projection.
//
// Chromium subsystems reused:
//   - BrowserList (chrome/browser/ui/browser_list.h) — owns all Browser windows.
//   - BrowserListObserver — notification of window add/remove events.
//   - Browser (chrome/browser/ui/browser.h) — window object we attach metadata to.
//
// Patch points:
//   - None needed for now — uses public BrowserListObserver API.
//   - Browser::SupportsUserData for attaching AstraWindowFeatures.
//
// Design decisions:
//   - Window-based approach (Arc-style): each workspace has one or more windows.
//     Switching workspaces shows/hides entire windows rather than moving tabs
//     between windows.
//   - Workspace metadata lives on AstraWindowFeatures (per-Browser user data).
//   - The manager is a profile-scoped singleton-like helper that projects
//     (accessed via factory or static methods).
//
// TODO(astra): Make this a proper ProfileKeyedService if we need lifecycle
// management and per-profile instance tracking.  For now it's a lightweight helper that operates
// uses BrowserList and filters by profile.
// Chromium pattern: similar to BrowserList::GetInstance() with profile filtering.
class AstraWorkspaceWindowManager : public BrowserListObserver {
 public:
  // Returns the singleton instance.
  //
  // TODO(astra): Consider making this a ProfileKeyedService instead of a singleton.
  // For now, a singleton is fine since BrowserList is a singleton and window tracking is
  // process-wide.  If we need per-profile isolation, migrate to KeyedService.
  static AstraWorkspaceWindowManager* GetInstance();

  AstraWorkspaceWindowManager(const AstraWorkspaceWindowManager&) = delete;
  AstraWorkspaceWindowManager& operator=(const AstraWorkspaceWindowManager&) = delete;
  ~AstraWorkspaceWindowManager() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraWorkspaceWindowManagerObserver* observer);
  void RemoveObserver(AstraWorkspaceWindowManagerObserver* observer);

  // -- Window-to-workspace query ----------------------------------------------

  // Returns all browser windows belonging to |workspace_id| in |profile|.
  // Returns an empty vector if no windows are found.
  //
  // Chromium owner: BrowserList — we iterate all browsers and filter
  // by profile + workspace_id.
  std::vector<Browser*> GetWindowsForWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // Returns all browser windows in |workspace_id| for |profile|, ordered by
  // each window's order_index (ascending).
  //
  // This is the ordered variant of GetWindowsForWorkspace.
  // Chromium owner: BrowserList — we iterate and sort by AstraWindowFeatures order.
  std::vector<Browser*> GetWindowsInWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // Returns the workspace ID that |browser| belongs to.
  // Returns "default" if the browser has no AstraWindowFeatures or no
  // workspace_id set.
  std::string GetWorkspaceForWindow(Browser* browser) const;

  // Returns the number of windows in |workspace_id| for |profile|.
  size_t GetWindowCount(Profile* profile,
                       const std::string& workspace_id) const;

  // Returns the number of windows in |workspace_id| for |profile|.
  // Alias/convenience name for GetWindowCount.
  size_t GetWindowCountInWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // Returns the total number of tabs across all windows in |workspace_id|
  // for |profile|.  Tabs are counted from each Browser's TabStripModel.
  //
  // Chromium owner: TabStripModel — we read tab count from each window.
  size_t GetTabCount(Profile* profile,
                    const std::string& workspace_id) const;

  // Returns the total number of tabs in |workspace_id| for |profile|.
  // Alias/convenience name for GetTabCount.
  size_t GetTabCountInWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // Returns the currently active/focused window in |workspace_id| for
  // |profile|.  Returns nullptr if the workspace has no windows or no
  // active window.
  //
  // The "active" window is the one whose BrowserWindow is currently active
  // (has focus).
  //
  // Chromium owner: BrowserList + BrowserWindow::IsActive().
  Browser* GetActiveWindowInWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // -- Window manipulation ----------------------------------------------

  // Moves |browser| to |workspace_id|.
  //
  // If the browser is incognito, this is a no-op (workspace is read-only).
  //
  // TODO(astra): When moving a window between workspaces, we should also
  // update all tabs in the window's TabStripModel to the new workspace?
  // Current design: window workspace_id is the primary truth for window switching,
  // but per-tab workspace_id determines tab membership.
  // For the window's tabs may differ from the window's workspace_id (e.g. tabs from
  // different workspaces can be in the same window).
  // For simplicity and Arc-style: window = workspace container.
  void MoveWindowToWorkspace(Browser* browser,
                          const std::string& workspace_id);

  // Moves all tabs from |source_workspace_id| to |target_workspace_id|
  // for |profile|.
  //
  // Windows in the source workspace are NOT automatically closed — they
  // remain in the source workspace but become empty.
  // Use CloseAllWindowsInWorkspace() separately if you also want to close
  // the source workspace's windows.
  //
  // Note: This updates per-tab workspace_id metadata via AstraTabFeatures.
  // Actual tab moves between windows are owned by Chromium's TabStripModel.
  //
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h).
  void MoveAllTabsToWorkspace(Profile* profile,
                              const std::string& source_workspace_id,
                              const std::string& target_workspace_id);

  // Creates a new browser window in |workspace_id| for |profile|.
  // Returns the new Browser*, or nullptr on failure.
  //
  // Chromium owner: Browser::Create() — we create a new Browser window with
  // the appropriate profile and type, then assign it to the workspace.
  //
  // TODO(astra): Use Browser::Create() with Browser::CreateParams.
  // Chromium component: Browser::CreateParams.
  Browser* CreateNewWindowInWorkspace(Profile* profile,
                                     const std::string& workspace_id);

  // -- Window ordering ---------------------------------------------------

  // Reorders the windows in |workspace_id| for |profile| to match the order
  // of |ordered_windows|.  All windows currently in the workspace must be
  // included in the list.
  //
  // Returns true if the reorder was successful.
  //
  // Window ordering is stored as order_index on AstraWindowFeatures.
  // Chromium owner: BrowserList — we only change Astra metadata, not the
  // underlying BrowserList ordering.
  bool ReorderWindowsInWorkspace(
      Profile* profile,
      const std::string& workspace_id,
      const std::vector<Browser*>& ordered_windows);

  // -- Bulk operations ---------------------------------------------------

  // Closes all windows in |workspace_id| for |profile|.
  //
  // Chromium owner: Browser::window()->Close() — we delegate actual window
  // closing to Chromium's window management.  The manager just iterates
  // and requests close for each window.
  //
  // Note: This is a request — window closing is asynchronous and the user
  // may cancel (e.g., "beforeunload" handlers).
  void CloseAllWindowsInWorkspace(Profile* profile,
                                  const std::string& workspace_id);

  // Moves all windows from |source_workspace_id| to |target_workspace_id|
  // for |profile|.  After this call, |source_workspace_id| will have zero
  // windows.
  //
  // This is equivalent to calling MoveWindowToWorkspace for every window
  // in the source workspace.
  void MoveAllWindowsToWorkspace(
      Profile* profile,
      const std::string& source_workspace_id,
      const std::string& target_workspace_id);

  // -- Stats -------------------------------------------------------------

  // Returns the total number of browser windows for |profile| across all
  // workspaces.
  //
  // Chromium owner: BrowserList — we iterate and count.
  size_t GetTotalWindowCount(Profile* profile) const;

  // Returns the total number of tabs across all windows for |profile|.
  //
  // Chromium owner: BrowserList + TabStripModel.
  size_t GetTotalTabCount(Profile* profile) const;

  // Returns the average number of tabs per window across all workspaces
  // for |profile|.  Returns 0 if there are no windows.
  double GetAverageTabsPerWindow(Profile* profile) const;

  // Returns the average number of tabs per window in |workspace_id| for
  // |profile|.  Returns 0 if the workspace has no windows.
  double GetAverageTabsPerWindowInWorkspace(
      Profile* profile,
      const std::string& workspace_id) const;

  // -- Hibernation -------------------------------------------------------

  // Hibernates |workspace_id| for |profile|.  Hibernated workspaces have
  // their tabs unloaded from memory to save resources.  Window metadata
  // and workspace assignment are preserved.
  //
  // Chromium owner: TabStripModel + WebContents (tab discarding).
  // This is a projection — we mark the workspace as hibernated on each
  // window's AstraWindowFeatures.
  //
  // TODO(astra): Integrate with Chromium's tab discarding system.
  // Chromium component: TabStripModel::DiscardWebContentsAt or
  // memory::TabManager.
  // For now, sets the hibernation flag on AstraWindowFeatures and
  // notifies observers.
  void HibernateWorkspace(Profile* profile,
                          const std::string& workspace_id);

  // Wakes up (de-hibernates) |workspace_id| for |profile|.  Tabs in the
  // workspace are made available again.
  //
  // Chromium owner: TabStripModel — tabs reload when activated.
  // This is the reverse of HibernateWorkspace.
  void WakeUpWorkspace(Profile* profile,
                       const std::string& workspace_id);

  // Returns true if |workspace_id| for |profile| is currently hibernated.
  // A workspace is considered hibernated if all of its windows have the
  // hibernated flag set.
  bool IsWorkspaceHibernated(Profile* profile,
                             const std::string& workspace_id) const;

  // -- Window arrangement ---------------------------------------------------

  // Tiles all windows in |workspace_id| for |profile| using the given
  // |direction|.  Windows are resized and positioned to fill the available
  // screen space.
  //
  // Chromium owner: BrowserWindow — actual window positioning is done by
  //   Chromium's window system.  We compute the layout and call SetBounds.
  void TileWindows(Profile* profile,
                   const std::string& workspace_id,
                   AstraTileDirection direction);

  // Convenience: tiles windows horizontally (side by side).
  void TileWindowsHorizontal(Profile* profile,
                             const std::string& workspace_id);

  // Convenience: tiles windows vertically (top to bottom).
  void TileWindowsVertical(Profile* profile,
                           const std::string& workspace_id);

  // Convenience: tiles windows in a grid layout.
  void TileWindowsGrid(Profile* profile,
                       const std::string& workspace_id);

  // Stacks all windows in |workspace_id| for |profile|: same position,
  // same size.  All windows get the same bounds as the first window.
  void StackWindows(Profile* profile,
                    const std::string& workspace_id);

  // Cascades all windows in |workspace_id| for |profile|: each window
  // is offset from the previous one by the cascade offset setting.
  void CascadeWindows(Profile* profile,
                      const std::string& workspace_id);

  // Maximizes all windows in |workspace_id| for |profile|.
  void MaximizeAllWindows(Profile* profile,
                          const std::string& workspace_id);

  // Minimizes all windows in |workspace_id| for |profile|.
  void MinimizeAllWindows(Profile* profile,
                          const std::string& workspace_id);

  // Restores all windows in |workspace_id| for |profile| to their
  // normal (non-maximized, non-minimized) state.
  void RestoreAllWindows(Profile* profile,
                         const std::string& workspace_id);

  // -- Focus cycling --------------------------------------------------------

  // Returns the currently focused/active window in |workspace_id| for
  // |profile|.  Returns nullptr if no window is focused.
  //
  // This is a convenience alias for GetActiveWindowInWorkspace.
  Browser* GetFocusedWindow(Profile* profile,
                            const std::string& workspace_id) const;

  // Focuses (activates) the next window in |workspace_id| for |profile|.
  // Returns the newly focused window, or nullptr if no windows.
  Browser* FocusNextWindow(Profile* profile,
                           const std::string& workspace_id);

  // Focuses (activates) the previous window in |workspace_id| for |profile|.
  // Returns the newly focused window, or nullptr if no windows.
  Browser* FocusPreviousWindow(Profile* profile,
                               const std::string& workspace_id);

  // -- New window behavior --------------------------------------------------

  // Returns the new window behavior for |profile|.
  AstraNewWindowBehavior GetNewWindowBehavior(Profile* profile) const;

  // Sets the new window behavior for |profile|.
  void SetNewWindowBehavior(Profile* profile,
                            AstraNewWindowBehavior behavior);

  // -- Workspace switching helpers ------------------------------------------

  // Switches to the next workspace in order.  Wraps around from last to first.
  void SwitchToNextWorkspace(Profile* profile);

  // Switches to the previous workspace in order.  Wraps around from first
  // to last.
  void SwitchToPreviousWorkspace(Profile* profile);

  // Switches to the workspace at the given |index|.  No-op if index is
  // out of range.
  void SwitchToWorkspaceAtIndex(Profile* profile, size_t index);

  // Returns the number of workspaces for |profile|.
  // Delegates to AstraWorkspaceService.
  size_t GetWorkspaceCount(Profile* profile) const;

  // Returns the index of the currently active workspace.
  size_t GetActiveWorkspaceIndex(Profile* profile) const;

  // Returns the index of the workspace with |workspace_id|, or 0 if not found.
  size_t GetWorkspaceIndex(Profile* profile,
                           const std::string& workspace_id) const;

  // Returns the N most recently used workspace IDs for |profile|.
  std::vector<std::string> GetRecentWorkspaces(Profile* profile,
                                               size_t max_count) const;

  // -- Workspace switch animation -------------------------------------------

  // Returns the workspace switch animation duration in milliseconds.
  int GetSwitchAnimationDurationMs(Profile* profile) const;

  // Sets the workspace switch animation duration in milliseconds.
  void SetSwitchAnimationDurationMs(Profile* profile, int duration_ms);

  // -- Saved window state ---------------------------------------------------

  // Saves the current window state (bounds, state) for all windows in
  // |workspace_id| to AstraWindowFeatures saved state.
  void SaveAllWindowState(Profile* profile,
                          const std::string& workspace_id);

  // Restores all windows in |workspace_id| from their saved state.
  void RestoreAllWindowState(Profile* profile,
                             const std::string& workspace_id);

  // Returns true if there is saved window state for |workspace_id|.
  bool HasSavedWindowState(Profile* profile,
                           const std::string& workspace_id) const;

  // Clears the saved window state for |workspace_id|.
  void ClearSavedWindowState(Profile* profile,
                             const std::string& workspace_id);

  // -- Settings (PrefService-based) -----------------------------------------

  // Returns whether window positions/sizes are remembered per workspace.
  bool GetRememberPlacement(Profile* profile) const;

  // Sets whether window positions/sizes are remembered per workspace.
  void SetRememberPlacement(Profile* profile, bool remember);

  // Returns whether new windows open in the active workspace.
  bool GetNewInActiveWorkspace(Profile* profile) const;

  // Sets whether new windows open in the active workspace.
  void SetNewInActiveWorkspace(Profile* profile, bool in_active);

  // Returns the tile gap (spacing between tiled windows) in pixels.
  int GetTileGap(Profile* profile) const;

  // Sets the tile gap in pixels.
  void SetTileGap(Profile* profile, int gap_px);

  // Returns the tile padding (space around the tile area) in pixels.
  int GetTilePadding(Profile* profile) const;

  // Sets the tile padding in pixels.
  void SetTilePadding(Profile* profile, int padding_px);

  // Returns the cascade offset (x/y offset for cascaded windows) in pixels.
  int GetCascadeOffset(Profile* profile) const;

  // Sets the cascade offset in pixels.
  void SetCascadeOffset(Profile* profile, int offset_px);

  // Returns whether auto-tiling is enabled.
  bool GetAutoTile(Profile* profile) const;

  // Sets whether auto-tiling is enabled.
  void SetAutoTile(Profile* profile, bool auto_tile);

  // Returns the default workspace ID for new windows.
  std::string GetDefaultWorkspaceId(Profile* profile) const;

  // Sets the default workspace ID for new windows.
  void SetDefaultWorkspaceId(Profile* profile,
                             const std::string& workspace_id);

  // -- Workspace indicator position -----------------------------------------

  // Position of the workspace indicator UI element.
  enum class IndicatorPosition {
    kTopLeft,
    kTopCenter,
    kTopRight,
    kBottomLeft,
    kBottomCenter,
    kBottomRight,
  };

  // Returns the workspace indicator position.
  IndicatorPosition GetWorkspaceIndicatorPosition(Profile* profile) const;

  // Sets the workspace indicator position.
  void SetWorkspaceIndicatorPosition(Profile* profile,
                                     IndicatorPosition position);

  // -- Workspace switching ----------------------------------------------

  // Switches to |workspace_id| in |profile|.
  // Shows all windows belonging to the workspace and hides all other
  // windows for the same profile.
  //
  // This is the Arc-style workspace switching model: each workspace has its own set
  // of windows, and switching shows/hides them as a group.
  //
  // Chromium owner: Browser window state (show/hide, minimize/restore).
  // We use Browser window show/hide (or minimize/restore) for visibility.
  //
  // TODO(astra): On some platforms window hide/show may not be the best approach
  // best.  Alternative: minimize/restore.  We save window bounds for now;
  // We use saved_bounds_ on AstraWindowFeatures for position recall.
  void SwitchToWorkspace(Profile* profile,
                          const std::string& workspace_id);

  // Returns the currently active workspace ID for |profile|.
  // The "active" workspace is the one whose windows are currently visible.
  //
  // Note: this is determined by checking which workspace has at least one visible window.
  // If multiple workspaces have visible windows (mixed state), returns the
  // workspace with the most visible windows or the first found.
  //
  // TODO(astra): Track active workspace explicitly rather than deriving it from window visibility.
  // For now we derive it, but explicit tracking would be more reliable.
  std::string GetActiveWorkspaceId(Profile* profile) const;

  // -- BrowserListObserver ----------------------------------------------

  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

 private:
  AstraWorkspaceWindowManager();

  // Helper: checks if |browser| belongs to |profile|.
  bool BrowserBelongsToProfile(Browser* browser, Profile* profile) const;

  // Saves the current window state (bounds, minimized, maximized) to
  // AstraWindowFeatures before hiding the window.
  void SaveWindowState(Browser* browser);

  // Restores a window from saved state (bounds, minimized, maximized).
  void RestoreWindowState(Browser* browser);

  // Helper: assigns a default order index to a newly added window.
  // Places it at the end of its workspace's window list.
  void AssignDefaultOrderIndex(Browser* browser);

  // Helper: notifies observers that window count changed for a workspace.
  void NotifyWindowCountChanged(Profile* profile,
                                const std::string& workspace_id);

  base::ObserverList<AstraWorkspaceWindowManagerObserver> observers_;
};

// Observer interface for AstraWorkspaceWindowManager.
//
// UI layers (workspace switcher, workspace overview) should observe this
// manager to update their presentation when window-workspace mappings
// change.  UI must never be the source of truth — this manager is.
//
// All observer methods have default empty implementations so that derived
// classes can override only what they need.
class AstraWorkspaceWindowManagerObserver : public base::CheckedObserver {
 public:
  ~AstraWorkspaceWindowManagerObserver() override = default;

  // Called when a window is added to a workspace (new window or
  // window moved between workspaces).
  virtual void OnWindowAddedToWorkspace(Browser* browser,
                                        const std::string& workspace_id) {}

  // Called when a window is removed from a workspace (window closed
  // or window moved to another workspace).
  virtual void OnWindowRemovedFromWorkspace(Browser* browser,
                                           const std::string& workspace_id) {}

  // Called when the active workspace changes (via SwitchToWorkspace).
  virtual void OnActiveWorkspaceChanged(const std::string& old_id,
                                         const std::string& new_id) {}

  // Called when the number of windows in a workspace changes.
  // This fires after OnWindowAddedToWorkspace / OnWindowRemovedFromWorkspace.
  virtual void OnWindowCountChanged(const std::string& workspace_id,
                                    size_t new_count) {}

  // Called when a workspace's hibernation state changes.
  virtual void OnWorkspaceHibernationChanged(const std::string& workspace_id,
                                             bool is_hibernated) {}

  // Called when windows in a workspace are rearranged (tiled, stacked,
  // cascaded, etc.).
  virtual void OnWindowsArranged(const std::string& workspace_id,
                                 AstraTileDirection direction) {}

  // Called when the focused window changes within a workspace.
  virtual void OnWindowFocusChanged(const std::string& workspace_id,
                                    Browser* focused_window) {}

  // Called when all windows in a workspace are maximized, minimized,
  // or restored.
  virtual void OnAllWindowsStateChanged(const std::string& workspace_id,
                                        bool is_maximized,
                                        bool is_minimized) {}
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_WINDOW_MANAGER_H_
