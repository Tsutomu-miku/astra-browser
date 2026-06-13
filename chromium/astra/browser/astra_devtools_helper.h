#ifndef ASTRA_BROWSER_ASTRA_DEVTOOLS_HELPER_H_
#define ASTRA_BROWSER_ASTRA_DEVTOOLS_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace content {
class WebContents;
}  // namespace content

class Browser;
class PrefService;
class Profile;

namespace astra {

// =========================================================================
// DevTools dock state
// =========================================================================
//
// Mirrors the dock positions supported by Chromium's DevToolsWindow.
// Astra does not own these states — they are projections of Chromium's
// DevToolsWindow dock side enum.  We define our own enum so that the
// Astra browser layer does not depend on DevTools headers directly.
//
// Chromium owner: DevToolsWindow / DevToolsDockSide
//   (chrome/browser/devtools/devtools_window.h)
//   (chrome/browser/devtools/devtools_window.cc)
// Chromium enum: devtools::mojom::DockSide or DevToolsWindow::DockSide
// =========================================================================
enum class AstraDevToolsDockState {
  kBottom,    // Docked to the bottom of the browser window.
  kRight,     // Docked to the right side of the browser window.
  kLeft,      // Docked to the left side of the browser window.
  kUndocked,  // Undocked — DevTools in a separate window.
  kMinimized, // Minimized — docked but collapsed to a small bar.
};

// =========================================================================
// AstraDevToolsPanelInfo — registered Astra DevTools panel metadata
// =========================================================================
//
// Describes an Astra-specific DevTools panel that can be shown alongside
// the standard Chromium DevTools panels.  Panels are registered by Astra
// features (workspace inspector, tab stack viewer, etc.) and surfaced
// through the DevTools window.
//
// This is projection metadata only — the actual panel content and
// rendering is owned by Chromium's DevTools framework (via extension
// panels or WebUI panels).  Astra only tracks which panels are available
// and their presentation metadata.
//
// Chromium owner: chrome.devtools.panels extension API
//   (chrome/common/extensions/api/devtools_panels.json)
// Chromium owner: DevToolsUIBindings / DevToolsWindow
//   (chrome/browser/devtools/devtools_window.h)
struct AstraDevToolsPanelInfo {
  // Stable identifier for the panel (e.g. "workspace-inspector").
  std::string id;

  // Human-readable display name shown in the DevTools tab strip.
  std::u16string name;

  // Optional icon identifier for the panel tab.
  std::string icon;

  // Whether the panel is currently enabled/visible in DevTools.
  bool enabled = true;

  // Whether this is a built-in Astra panel (vs. user/extension added).
  bool is_builtin = false;
};

// =========================================================================
// AstraDevToolsHelper — bridge between Astra and Chromium DevTools
// =========================================================================
//
// Helper class that wraps Chromium's DevTools subsystem and exposes an
// Astra-friendly API.  All DevTools state and window management is owned
// by Chromium — this helper is a thin adapter that translates between
// Astra types and Chromium's DevToolsWindow / DevToolsManager APIs.
//
// This is NOT a ProfileKeyedService.  DevTools is per-tab/per-WebContents
// state managed by Chromium's DevToolsWindow and DevToolsAgentHost.
// Astra only projects that state and provides convenience entry points.
//
// Astra-specific presentation preferences (default dock state, default
// panel, auto-open, Astra panel visibility) are persisted via the
// profile's PrefService.  These are purely presentation concerns — they
// never affect the underlying DevTools engine owned by Chromium.
//
// Chromium subsystems reused:
//   - DevToolsWindow   — DevTools window creation, dock state, lifecycle
//   - DevToolsManager  — global DevTools instance management
//   - DevToolsAgentHost — per-WebContents DevTools agent
//   - chrome/browser/devtools/ — Chrome DevTools UI layer
//
// Chromium patch points:
//   - None required for basic operations — DevToolsWindow has public
//     static methods (ToggleDevToolsWindow, etc.) that can be called
//     directly.
//   - For dock state querying: may require a small patch to expose
//     DevToolsWindow::GetDockSide() or similar public method.
//   - For Astra panel injection: may require a small patch to
//     DevToolsWindow to register custom panels.
//
// TODO(astra): Proper DevToolsWindow integration for toggle and dock state.
//   The current skeleton uses static helper methods that delegate to
//   Chromium's DevToolsWindow API.  Some methods may require a small
//   Chromium patch to expose internals (e.g., reading current dock side).
// =========================================================================

class AstraDevToolsHelper {
 public:
  // =======================================================================
  // Observer interface for DevTools state changes
  // =======================================================================
  //
  // Astra UI surfaces (sidebar, command palette, workspace overview)
  // implement this observer to react to DevTools lifecycle and state
  // changes.  All observer methods have empty default implementations so
  // observers only need to override the events they care about.
  //
  // The browser layer never depends on Views code.
  //
  // Chromium owner: DevToolsWindow — these events would be observed by
  //   patching DevToolsWindow's lifecycle methods (Create, Close,
  //   SetDockSide) to notify Astra's observer list.
  class Observer : public base::CheckedObserver {
   public:
    // Called when a DevTools window is opened for a tab.
    // |web_contents| is the inspected tab's WebContents.
    virtual void OnDevToolsWindowOpened(content::WebContents* web_contents) {}

    // Called when a DevTools window is closed for a tab.
    // |web_contents| is the inspected tab's WebContents.
    virtual void OnDevToolsWindowClosed(content::WebContents* web_contents) {}

    // Called when the DevTools dock state changes for a tab.
    // |web_contents| is the inspected tab's WebContents.
    // |dock_state| is the new dock state.
    virtual void OnDevToolsDockStateChanged(
        content::WebContents* web_contents,
        AstraDevToolsDockState dock_state) {}

    // Called when the Astra DevTools panel visibility changes.
    // |visible| is true if the Astra panel is now shown, false if hidden.
    virtual void OnAstraPanelVisibilityChanged(bool visible) {}

    // Called when any DevTools presentation setting changes
    // (default dock state, default panel, auto-open, etc.).
    // Use this for catch-all updates (e.g., full UI refresh).
    virtual void OnDevToolsSettingsChanged() {}

   protected:
    ~Observer() override = default;
  };

  AstraDevToolsHelper() = delete;
  AstraDevToolsHelper(const AstraDevToolsHelper&) = delete;
  AstraDevToolsHelper& operator=(const AstraDevToolsHelper&) = delete;
  ~AstraDevToolsHelper() = delete;

  // -- Query --------------------------------------------------------------

  // Returns true if DevTools is currently open for |web_contents|.
  //
  // This queries Chromium's DevToolsWindow system to determine if a
  // DevTools instance is attached to the given WebContents.
  //
  // Chromium owner: DevToolsWindow / DevToolsManager
  //   (chrome/browser/devtools/devtools_window.h)
  //   Implementation checks DevToolsWindow::GetInstanceForInspectedWebContents
  //   or DevToolsManager::GetDevToolsAgentHostFor.
  static bool IsDevToolsOpenForTab(content::WebContents* web_contents);

  // Returns the current dock state for |web_contents|'s DevTools.
  // If DevTools is not open, returns the default dock state from prefs
  // (or kBottom if no pref is set).
  //
  // Chromium owner: DevToolsWindow::GetDockSide()
  //   (chrome/browser/devtools/devtools_window.h)
  //
  // TODO(astra): Dock state querying may require a small Chromium patch
  //   to expose DevToolsWindow::GetDockSide() as a public method, or to
  //   add a static helper that returns the dock side for a given
  //   WebContents.  The DevToolsWindow class has this information
  //   internally but may not expose it publicly.
  static AstraDevToolsDockState GetDevToolsDockState(
      content::WebContents* web_contents);

  // Returns the number of currently open DevTools windows across all
  // browser windows and tabs.
  //
  // Chromium owner: DevToolsManager — tracks all DevToolsAgentHost
  //   instances.  Count can be derived from iterating attached hosts.
  //   (content/browser/devtools/devtools_manager.h)
  static int GetOpenDevToolsCount();

  // -- Control ------------------------------------------------------------

  // Toggles DevTools for |web_contents|.
  // If DevTools is open, closes it.  If closed, opens it with the
  // last-used dock state (or the default if no history).
  //
  // This delegates to Chromium's DevToolsWindow::ToggleDevToolsWindow().
  //
  // Chromium owner: DevToolsWindow::ToggleDevToolsWindow
  //   (chrome/browser/devtools/devtools_window.h)
  static void ToggleDevTools(content::WebContents* web_contents);

  // Opens DevTools for |web_contents| with the specified dock state.
  // If DevTools is already open, changes the dock state to |dock_state|.
  //
  // Chromium owner: DevToolsWindow::OpenDevToolsWindow / SetDockSide
  //   (chrome/browser/devtools/devtools_window.h)
  //
  // TODO(astra): Verify the exact API for opening DevTools with a
  //   specific dock state.  Chromium's DevToolsWindow has
  //   OpenDevToolsWindow() which takes a DockSide parameter, or the
  //   dock side can be set after creation via SetDockSide().
  static void OpenDevTools(content::WebContents* web_contents,
                           AstraDevToolsDockState dock_state);

  // Closes DevTools for |web_contents|.
  // No-op if DevTools is not open.
  //
  // Chromium owner: DevToolsWindow::CloseWindow / CloseDevToolsWindow
  static void CloseDevTools(content::WebContents* web_contents);

  // Sets the dock state for |web_contents|'s DevTools.
  // If DevTools is not open, opens it with the given dock state.
  //
  // Chromium owner: DevToolsWindow::SetDockSide
  //   (chrome/browser/devtools/devtools_window.h)
  static void SetDevToolsDockState(content::WebContents* web_contents,
                                   AstraDevToolsDockState dock_state);

  // Closes all open DevTools windows across all browser windows and tabs.
  //
  // Chromium owner: DevToolsManager — can iterate all DevToolsAgentHost
  //   instances and force-close their associated DevToolsWindows.
  //   (content/browser/devtools/devtools_manager.h)
  //   (chrome/browser/devtools/devtools_window.h)
  static void CloseAllDevTools();

  // -- Browser-level helpers ----------------------------------------------

  // Toggles DevTools for the active tab of |browser|.
  // Convenience wrapper that gets the active WebContents from the
  // browser's TabStripModel and calls ToggleDevTools().
  //
  // Chromium owner: TabStripModel::GetActiveWebContents
  //   (chrome/browser/ui/tabs/tab_strip_model.h)
  static void ToggleDevToolsForBrowser(Browser* browser);

  // Returns true if DevTools is open for the active tab of |browser|.
  static bool IsDevToolsOpenForActiveTab(Browser* browser);

  // Opens DevTools for the active tab of |browser| with the given dock
  // state.  Convenience wrapper around OpenDevTools().
  static void OpenDevToolsForBrowser(Browser* browser,
                                     AstraDevToolsDockState dock_state);

  // Closes DevTools for the active tab of |browser|.
  static void CloseDevToolsForBrowser(Browser* browser);

  // -- Astra DevTools panel -----------------------------------------------

  // Opens the Astra-specific DevTools panel (workspace/favorite management).
  // This is an optional advanced feature that adds an Astra panel to the
  // DevTools window for browser-level product features.
  //
  // Implementation approach:
  //   - Option A: Use a DevTools extension (chrome.devtools.panels API).
  //   - Option B: Use a custom WebUI panel registered via a patch.
  //   - Option C: Use a Views-based toolbar alongside the DevTools dock.
  //
  // Currently implemented as a no-op placeholder.
  //
  // TODO(astra): Implement Astra DevTools panel / extension.
  //   This could be built as a Chrome DevTools extension (using the
  //   chrome.devtools.panels API) or as a native Views panel that
  //   attaches to the DevTools dock area.
  //
  // Chromium owner: DevToolsWindow / DevToolsUIBindings
  //   (chrome/browser/devtools/devtools_window.h)
  // Chromium extension API: chrome.devtools.panels
  //   (chrome/common/extensions/api/devtools_panels.json)
  static void OpenAstraDevToolsPanel(content::WebContents* web_contents);

  // Returns whether the Astra DevTools panel is currently visible/enabled.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This is a presentation preference — it controls whether the Astra
  // panel tab is shown in DevTools.  The actual panel content and
  // rendering is owned by Chromium's DevTools framework.
  static bool IsAstraPanelVisible(Profile* profile);

  // Sets whether the Astra DevTools panel is visible.
  // Fires OnAstraPanelVisibilityChanged and OnDevToolsSettingsChanged
  // observer notifications.
  static void SetAstraPanelVisible(Profile* profile, bool visible);

  // Toggles the Astra DevTools panel visibility.
  // Returns the new visibility state.
  static bool ToggleAstraPanelVisible(Profile* profile);

  // -- Astra panel registration -------------------------------------------

  // Registers an Astra DevTools panel with the given metadata.
  // Returns true if the panel was registered successfully.
  //
  // Registered panels are shown as additional tabs in the DevTools
  // window.  This is an Astra-level registry — actual panel injection
  // into Chromium's DevToolsWindow would require a patch.
  //
  // In the overlay skeleton, panels are tracked in-memory for UI
  // projection purposes.
  //
  // TODO(astra): Wire panel registration to Chromium's DevTools panel
  //   extension API or a custom WebUI panel injection point.
  // Chromium owner: chrome.devtools.panels (extension API)
  //   (chrome/common/extensions/api/devtools_panels.json)
  // Patch point: DevToolsWindow::Create or DevToolsUIBindings could be
  //   patched to inject Astra panels.
  static bool RegisterAstraPanel(const AstraDevToolsPanelInfo& panel_info);

  // Unregisters an Astra DevTools panel by ID.
  // Returns true if a panel was found and removed.
  static bool UnregisterAstraPanel(const std::string& panel_id);

  // Returns the list of all registered Astra DevTools panels.
  static std::vector<AstraDevToolsPanelInfo> GetRegisteredPanels();

  // Returns the number of registered Astra DevTools panels.
  static size_t GetRegisteredPanelCount();

  // Returns info for the panel with the given |panel_id|, or an empty
  // AstraDevToolsPanelInfo (empty id) if not found.
  static AstraDevToolsPanelInfo GetPanelById(const std::string& panel_id);

  // -- DevTools settings (Astra-specific, persisted via PrefService) -------

  // Returns the default dock state for new DevTools windows.
  //
  // Persisted via PrefService.  Default: kBottom.
  //
  // This is a presentation preference — it controls what dock state
  // DevTools opens with by default.  The actual docking is handled by
  // Chromium's DevToolsWindow.
  //
  // Chromium owner: DevToolsWindow — uses prefs for default dock side
  //   (devtools.preferences.currentDockState or similar).
  //   This Astra pref complements the Chromium pref by allowing
  //   Astra-specific defaults.
  static AstraDevToolsDockState GetDefaultDockState(Profile* profile);

  // Sets the default dock state for new DevTools windows.
  // Fires OnDevToolsSettingsChanged observer notification.
  static void SetDefaultDockState(Profile* profile,
                                  AstraDevToolsDockState dock_state);

  // Returns the ID of the default panel shown when DevTools opens.
  //
  // Persisted via PrefService.  Default: empty string (uses Chromium's
  // default panel, typically the Elements panel).
  //
  // This is a presentation preference — it controls which panel is
  // activated by default when DevTools opens.
  static std::string GetDefaultPanel(Profile* profile);

  // Sets the default panel ID for new DevTools windows.
  // Fires OnDevToolsSettingsChanged observer notification.
  static void SetDefaultPanel(Profile* profile, const std::string& panel_id);

  // Returns whether DevTools auto-opens for new tabs.
  //
  // Persisted via PrefService.  Default: false.
  //
  // When true, Astra automatically opens DevTools for newly created tabs.
  // This is useful for development workflows.
  //
  // Chromium owner: DevToolsWindow — there is no native auto-open feature
  //   for regular tabs; Astra would implement this by observing
  //   TabStripModel changes and calling OpenDevTools().
  static bool GetAutoOpenDevTools(Profile* profile);

  // Sets whether DevTools auto-opens for new tabs.
  // Fires OnDevToolsSettingsChanged observer notification.
  static void SetAutoOpenDevTools(Profile* profile, bool auto_open);

  // Toggles auto-open DevTools.  Returns the new state.
  static bool ToggleAutoOpenDevTools(Profile* profile);

  // -- Shortcut helpers ---------------------------------------------------

  // Returns a human-readable description of the DevTools toggle shortcut.
  // On most platforms this is "F12" or "Ctrl+Shift+I" / "Cmd+Opt+I".
  //
  // This is a convenience method for UI surfaces that want to display
  // the shortcut hint.  The actual shortcut registration is owned by
  // Chromium's accelerator system.
  //
  // Chromium owner: BrowserCommandController / AcceleratorManager
  //   (chrome/browser/ui/browser_command_controller.h)
  //   (ui/base/accelerators/accelerator_manager.h)
  static std::string GetDevToolsToggleShortcutText();

  // Returns the shortcut description for opening DevTools in a specific
  // dock state.  Useful for command palette or settings UI.
  static std::string GetDevToolsOpenShortcutText(AstraDevToolsDockState state);

  // -- String conversion --------------------------------------------------

  // Converts an AstraDevToolsDockState to the corresponding Chromium
  // DevTools dock side string/identifier.
  //
  // The returned string matches Chromium's DevTools dock side enum names
  // (e.g., "bottom", "right", "left", "undocked").
  static std::string DockStateToString(AstraDevToolsDockState state);

  // Converts a Chromium DevTools dock side string to an Astra dock state
  // enum value.  Returns kBottom as default for unrecognized strings.
  static AstraDevToolsDockState DockStateFromString(const std::string& state);

  // -- Observers ---------------------------------------------------------

  // Registers an observer for DevTools state change notifications.
  //
  // TODO(astra): Wire up to DevToolsWindow lifecycle events via patches.
  //   Currently observers are registered but only notified by explicit
  //   calls to Notify* methods and by settings changes.  In a full
  //   Chromium build, patches to DevToolsWindow would trigger these
  //   notifications on actual window open/close/dock change events.
  static void AddObserver(Observer* observer);

  // Unregisters an observer.
  static void RemoveObserver(Observer* observer);

  // Notify all observers that a DevTools window has opened.
  static void NotifyDevToolsWindowOpened(content::WebContents* web_contents);

  // Notify all observers that a DevTools window has closed.
  static void NotifyDevToolsWindowClosed(content::WebContents* web_contents);

  // Notify all observers that the dock state has changed.
  static void NotifyDevToolsDockStateChanged(
      content::WebContents* web_contents,
      AstraDevToolsDockState dock_state);

  // Notify all observers that the Astra panel visibility has changed.
  static void NotifyAstraPanelVisibilityChanged(bool visible);

  // Notify all observers that DevTools presentation settings have changed.
  static void NotifyDevToolsSettingsChanged();

 private:
  // Returns the DevToolsWindow instance for |web_contents|, or nullptr
  // if DevTools is not open for that tab.
  //
  // This is an internal helper that accesses Chromium's DevToolsWindow
  // API.  The exact return type depends on how we integrate with
  // Chromium — in the overlay repo, we forward-declare and access via
  // the public API.
  //
  // Chromium owner: DevToolsWindow::GetInstanceForInspectedWebContents
  //   (chrome/browser/devtools/devtools_window.h)
  // TODO(astra): Determine the correct DevToolsWindow getter API.
  //   Chromium has DevToolsWindow::FindForInspectedWebContents or
  //   DevToolsManager::GetDevToolsAgentHostFor.  We need to verify
  //   which public API is available for checking if DevTools is open.
  static void* GetDevToolsWindowForContents(content::WebContents* web_contents);

  // Returns the static observer list.  Wrapped in a function to avoid
  // static initialization order issues.
  static base::ObserverList<Observer>& GetObservers();

  // Returns the static registered panels list.  Wrapped in a function to
  // avoid static initialization order issues.
  static std::vector<AstraDevToolsPanelInfo>& GetRegisteredPanelsInternal();

  // Helper to get the PrefService from a profile, with null checks.
  static PrefService* GetPrefs(Profile* profile);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_DEVTOOLS_HELPER_H_
