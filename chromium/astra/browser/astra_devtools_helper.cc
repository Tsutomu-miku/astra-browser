#include "astra/browser/astra_devtools_helper.h"

#include "base/logging.h"
#include "base/observer_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Built-in Astra DevTools panels.
// These are the standard Astra panels that ship with the browser.
const char kPanelWorkspaceInspector[] = "workspace-inspector";
const char kPanelTabStackViewer[] = "tab-stack-viewer";
const char kPanelFavoriteManager[] = "favorite-manager";
const char kPanelFocusMode[] = "focus-mode";

}  // namespace

// =========================================================================
// Query
// =========================================================================

bool AstraDevToolsHelper::IsDevToolsOpenForTab(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }

  // Query Chromium's DevTools system to check if DevTools is open.
  //
  // Chromium provides several ways to check:
  //   1. DevToolsWindow::FindForInspectedWebContents(web_contents)
  //      — returns the DevToolsWindow* if one exists for this tab.
  //   2. DevToolsManager::GetInstance()->GetDevToolsAgentHostFor(
  //        web_contents) — returns the DevToolsAgentHost, which has
  //      IsAttached() to check if a client is connected.
  //   3. web_contents->GetDelegate() or web_contents->GetUserData()
  //      — DevToolsWindow stores itself as WebContentsUserData.
  //
  // TODO(astra): Implement using the correct Chromium DevTools API.
  //   The exact method depends on which DevTools headers are available
  //   in the public API surface.  Options:
  //
  //   a) Use DevToolsWindow::FindForInspectedWebContents(web_contents)
  //      from chrome/browser/devtools/devtools_window.h (Chrome layer).
  //      This is the most direct approach but requires Chrome-level
  //      headers.
  //
  //   b) Use content::DevToolsAgentHost::GetOrCreateFor(web_contents)
  //      and check IsAttached() from content/public/browser/.
  //      This is more generic (works in content layer) but may not
  //      distinguish between docked and undocked windows.
  //
  //   c) Use web_contents->GetUserData with the DevToolsWindow key.
  //      DevToolsWindow uses WebContentsUserData to attach itself to
  //      the inspected WebContents (or its own devtools WebContents).
  //
  // For the overlay skeleton, we return false as a placeholder.
  // The real implementation will call into Chromium's DevToolsWindow.
  //
  // Chromium owner: DevToolsWindow / DevToolsManager
  //   (chrome/browser/devtools/devtools_window.h)
  //   (content/browser/devtools/devtools_manager.h)
  //
  // TODO(astra): Proper DevToolsWindow integration for toggle and dock state.
  //   We need to determine which DevTools APIs are available as public
  //   headers and wire up the actual checks.  Some APIs may require a
  //   small Chromium patch to expose them to the overlay.
  DVLOG(1) << "AstraDevToolsHelper::IsDevToolsOpenForTab: placeholder";
  return false;
}

AstraDevToolsDockState AstraDevToolsHelper::GetDevToolsDockState(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return AstraDevToolsDockState::kBottom;
  }

  // Read the current dock state from Chromium's DevToolsWindow.
  //
  // Chromium stores dock state in:
  //   - DevToolsWindow::dock_side() — the current dock side enum
  //   - PrefService — persisted preference for default dock side
  //     (devtools.preferences.currentDockState or similar)
  //
  // TODO(astra): Implement dock state querying from DevToolsWindow.
  //   The DevToolsWindow class has a dock_side() method internally,
  //   but it may not be public.  Options:
  //
  //   a) Patch DevToolsWindow to expose GetDockSide() as public static
  //      or instance method (tiny patch).
  //   b) Read from the DevTools preferences in PrefService:
  //      prefs->GetDict("devtools.preferences")->FindString("currentDockState")
  //   c) Use DevToolsWindow::GetInSessionDockSide() if available.
  //
  // For the overlay skeleton, return the default dock state from
  // Astra prefs (or kBottom if no profile is available).
  //
  // Chromium owner: DevToolsWindow::dock_side_
  //   (chrome/browser/devtools/devtools_window.h)
  // Chromium pref: devtools.preferences.currentDockState
  //   (chrome/browser/devtools/devtools_window.cc)
  DVLOG(1) << "AstraDevToolsHelper::GetDevToolsDockState: placeholder";
  return AstraDevToolsDockState::kBottom;
}

int AstraDevToolsHelper::GetOpenDevToolsCount() {
  // Count the number of currently open DevTools windows.
  //
  // In Chromium, this can be done by:
  //   1. Iterating all DevToolsAgentHost instances via DevToolsManager
  //      and counting those with IsAttached() == true.
  //   2. Iterating all Browser windows and checking for DevTools windows.
  //   3. Using DevToolsWindow::GetDevToolsWindowCount() if available.
  //
  // TODO(astra): Implement with actual DevToolsManager iteration.
  //   The exact approach depends on which APIs are publicly available.
  //   For the overlay skeleton, we return 0 as a placeholder.
  //
  // Chromium owner: DevToolsManager / DevToolsAgentHost
  //   (content/browser/devtools/devtools_manager.h)
  //   (content/public/browser/devtools_agent_host.h)
  DVLOG(1) << "AstraDevToolsHelper::GetOpenDevToolsCount: placeholder";
  return 0;
}

// =========================================================================
// Control
// =========================================================================

void AstraDevToolsHelper::ToggleDevTools(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  // Delegate to Chromium's DevToolsWindow::ToggleDevToolsWindow().
  //
  // Chromium's standard DevTools toggle flow:
  //   1. BrowserCommandController handles IDC_DEV_TOOLS
  //   2. Calls DevToolsWindow::ToggleDevToolsWindow(browser)
  //   3. Which finds or creates the DevToolsWindow for the active tab
  //
  // TODO(astra): Wire to actual DevToolsWindow::ToggleDevToolsWindow.
  //   The signature is typically:
  //     static void ToggleDevToolsWindow(
  //         Browser* browser,
  //         DevToolsToggleAction action,
  //         const std::string& panel = std::string());
  //
  //   Or there may be a version that takes WebContents directly.
  //
  //   For the overlay skeleton, we simulate the toggle by logging.
  //   In the real build, this calls into Chromium:
  //
  //     #include "chrome/browser/devtools/devtools_window.h"
  //     DevToolsWindow::ToggleDevToolsWindow(
  //         browser, DevToolsToggleAction::kToggle);
  //
  //   But since we only have WebContents here (not Browser), we may
  //   need to find the Browser from the WebContents via
  //   chrome/browser/ui/tab_helpers.h or similar.
  //
  // Chromium owner: DevToolsWindow::ToggleDevToolsWindow
  //   (chrome/browser/devtools/devtools_window.h)
  //
  // TODO(astra): Proper DevToolsWindow integration for toggle and dock state.
  //   Need to figure out the correct API for toggling per-WebContents
  //   vs per-Browser.  Chrome's DevToolsWindow has both per-browser
  //   and per-web-contents access patterns.
  DVLOG(1) << "AstraDevToolsHelper::ToggleDevTools: placeholder";
}

void AstraDevToolsHelper::OpenDevTools(content::WebContents* web_contents,
                                       AstraDevToolsDockState dock_state) {
  if (!web_contents) {
    return;
  }

  // Open DevTools with the specified dock state.
  //
  // In Chromium, this is done via:
  //   DevToolsWindow::OpenDevToolsWindow(web_contents, dock_side)
  // or
  //   DevToolsWindow::CreateDevToolsBrowser(...) for undocked.
  //
  // TODO(astra): Wire to actual DevToolsWindow open API.
  //   The exact API depends on whether we want to target a specific
  //   WebContents or use Browser-level DevTools.
  //
  //   Approximate Chromium call:
  //
  //     DevToolsWindow::OpenDevToolsWindow(
  //         web_contents,
  //         DevToolsDockSide::kDockedToBottom,  // or right/left/undocked
  //         DevToolsOpenedByAction::kOther);
  //
  //   For now, this is a placeholder.
  //
  // Chromium owner: DevToolsWindow::OpenDevToolsWindow
  //   (chrome/browser/devtools/devtools_window.h)
  DVLOG(1) << "AstraDevToolsHelper::OpenDevTools: placeholder, state="
           << DockStateToString(dock_state);
}

void AstraDevToolsHelper::CloseDevTools(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  // Close DevTools for |web_contents|.
  //
  // In Chromium, DevToolsWindow::CloseWindow() or CloseDevToolsWindow().
  //
  // TODO(astra): Wire to actual DevToolsWindow close API.
  //   Find the DevToolsWindow for the WebContents and call Close().
  //
  //   Approximate Chromium call:
  //
  //     DevToolsWindow* window =
  //         DevToolsWindow::FindForInspectedWebContents(web_contents);
  //     if (window) window->CloseWindow();
  //
  // Chromium owner: DevToolsWindow::CloseWindow
  //   (chrome/browser/devtools/devtools_window.h)
  DVLOG(1) << "AstraDevToolsHelper::CloseDevTools: placeholder";
}

void AstraDevToolsHelper::SetDevToolsDockState(
    content::WebContents* web_contents,
    AstraDevToolsDockState dock_state) {
  if (!web_contents) {
    return;
  }

  // Set the dock state for DevTools.
  //
  // In Chromium, DevToolsWindow has a SetDockSide() method.
  // If DevTools is not open, this opens it with the specified side.
  //
  // TODO(astra): Wire to actual DevToolsWindow::SetDockSide.
  //
  //   Approximate Chromium call:
  //
  //     DevToolsWindow* window =
  //         DevToolsWindow::FindForInspectedWebContents(web_contents);
  //     if (window) {
  //       window->SetDockSide(ToChromiumDockSide(dock_state));
  //     } else {
  //       OpenDevTools(web_contents, dock_state);
  //     }
  //
  // Chromium owner: DevToolsWindow::SetDockSide
  //   (chrome/browser/devtools/devtools_window.h)
  DVLOG(1) << "AstraDevToolsHelper::SetDevToolsDockState: placeholder, state="
           << DockStateToString(dock_state);
}

void AstraDevToolsHelper::CloseAllDevTools() {
  // Close all open DevTools windows.
  //
  // In Chromium, this can be done by:
  //   1. Iterating all DevToolsAgentHost instances and detaching clients.
  //   2. Iterating all Browser windows and closing DevTools for each tab.
  //   3. Using DevToolsManager to force-close all DevToolsWindows.
  //
  // TODO(astra): Implement with actual DevToolsManager/BrowserList
  //   iteration.  For the overlay skeleton, this is a no-op placeholder.
  //
  // Chromium owner: DevToolsManager / BrowserList
  //   (content/browser/devtools/devtools_manager.h)
  //   (chrome/browser/ui/browser_list.h)
  DVLOG(1) << "AstraDevToolsHelper::CloseAllDevTools: placeholder";
}

// =========================================================================
// Browser-level helpers
// =========================================================================

void AstraDevToolsHelper::ToggleDevToolsForBrowser(Browser* browser) {
  if (!browser || !browser->tab_strip_model()) {
    return;
  }

  content::WebContents* active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!active_contents) {
    return;
  }

  // For the per-browser toggle, we can also use Chromium's
  // DevToolsWindow::ToggleDevToolsWindow(browser) directly, which
  // operates on the active tab and handles focus correctly.
  //
  // TODO(astra): Use Browser-level DevTools toggle when available.
  //   Chrome's DevToolsWindow has both Browser-level and
  //   WebContents-level APIs.  The Browser-level version is preferred
  //   for command dispatch because it matches the user's expectation
  //   (F12 / Ctrl+Shift+I toggles DevTools for the current browser).
  //
  //   For now, delegate to the WebContents version.
  ToggleDevTools(active_contents);
}

bool AstraDevToolsHelper::IsDevToolsOpenForActiveTab(Browser* browser) {
  if (!browser || !browser->tab_strip_model()) {
    return false;
  }

  content::WebContents* active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  return IsDevToolsOpenForTab(active_contents);
}

void AstraDevToolsHelper::OpenDevToolsForBrowser(Browser* browser,
                                                 AstraDevToolsDockState dock_state) {
  if (!browser || !browser->tab_strip_model()) {
    return;
  }

  content::WebContents* active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  OpenDevTools(active_contents, dock_state);
}

void AstraDevToolsHelper::CloseDevToolsForBrowser(Browser* browser) {
  if (!browser || !browser->tab_strip_model()) {
    return;
  }

  content::WebContents* active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  CloseDevTools(active_contents);
}

// =========================================================================
// Astra DevTools panel
// =========================================================================

void AstraDevToolsHelper::OpenAstraDevToolsPanel(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  // Open the Astra-specific DevTools panel.
  //
  // This is an optional advanced feature.  Implementation approaches:
  //
  // Option A — DevTools Extension (recommended for initial implementation):
  //   - Build a Chrome extension that uses the chrome.devtools.panels API
  //     to add an "Astra" panel to DevTools.
  //   - The panel can access tab information and workspace data via
  //     chrome.runtime messaging with the background page.
  //   - Pros: uses standard extension APIs, no Chromium patches needed.
  //   - Cons: limited to extension API surface, harder to access
  //     Astra-only services.
  //
  // Option B — Custom WebUI panel (more powerful):
  //   - Patch DevToolsWindow to add a custom panel type.
  //   - Build a WebUI page (chrome://astra-devtools) that provides
  //     workspace management and favorite tools.
  //   - Pros: full access to browser internals, native performance.
  //   - Cons: requires more Chromium patching.
  //
  // Option C — Views toolbar alongside DevTools dock:
  //   - Add a Views-based toolbar that appears next to the DevTools
  //     dock area (above or beside it).
  //   - The toolbar provides quick access to Astra features (workspace
  //     switcher, favorite toggle, notes) while debugging.
  //   - Pros: native UI, easy access to Astra services.
  //   - Cons: doesn't integrate into the DevTools panel tabs.
  //
  // Currently a placeholder — no-op.
  //
  // TODO(astra): Implement Astra DevTools panel or toolbar integration.
  //   Decide between extension, WebUI, and Views approaches.
  //   Option C (Views toolbar) is most aligned with Astra's architecture
  //   because it reuses Chromium Views and integrates with Astra services
  //   directly.  It can be implemented as a view that attaches to the
  //   DevTools dock area.
  //
  // Chromium owner: DevToolsDockSide, DevToolsWindow
  //   (chrome/browser/devtools/devtools_window.h)
  // Patch point: DevToolsWindow::SetDockSide or DevToolsWindow::Create
  //   could be patched to inject Astra's toolbar view alongside the
  //   DevTools web view.
  DVLOG(1) << "AstraDevToolsHelper::OpenAstraDevToolsPanel: placeholder";
}

bool AstraDevToolsHelper::IsAstraPanelVisible(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultDevToolsAstraPanelVisible;
  }
  return prefs->GetBoolean(prefs::kPrefDevToolsAstraPanelVisible);
}

void AstraDevToolsHelper::SetAstraPanelVisible(Profile* profile,
                                               bool visible) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  bool old_value = prefs->GetBoolean(prefs::kPrefDevToolsAstraPanelVisible);
  if (old_value == visible) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDevToolsAstraPanelVisible, visible);
  NotifyAstraPanelVisibilityChanged(visible);
  NotifyDevToolsSettingsChanged();
}

bool AstraDevToolsHelper::ToggleAstraPanelVisible(Profile* profile) {
  bool current = IsAstraPanelVisible(profile);
  SetAstraPanelVisible(profile, !current);
  return !current;
}

// =========================================================================
// Astra panel registration
// =========================================================================

bool AstraDevToolsHelper::RegisterAstraPanel(
    const AstraDevToolsPanelInfo& panel_info) {
  if (panel_info.id.empty()) {
    return false;
  }

  auto& panels = GetRegisteredPanelsInternal();

  // Check if a panel with this ID already exists.
  for (const auto& panel : panels) {
    if (panel.id == panel_info.id) {
      return false;
    }
  }

  panels.push_back(panel_info);
  DVLOG(1) << "AstraDevToolsHelper::RegisterAstraPanel: registered panel="
           << panel_info.id;
  return true;
}

bool AstraDevToolsHelper::UnregisterAstraPanel(const std::string& panel_id) {
  if (panel_id.empty()) {
    return false;
  }

  auto& panels = GetRegisteredPanelsInternal();
  for (auto it = panels.begin(); it != panels.end(); ++it) {
    if (it->id == panel_id) {
      panels.erase(it);
      DVLOG(1) << "AstraDevToolsHelper::UnregisterAstraPanel: unregistered panel="
               << panel_id;
      return true;
    }
  }
  return false;
}

std::vector<AstraDevToolsPanelInfo> AstraDevToolsHelper::GetRegisteredPanels() {
  return GetRegisteredPanelsInternal();
}

size_t AstraDevToolsHelper::GetRegisteredPanelCount() {
  return GetRegisteredPanelsInternal().size();
}

AstraDevToolsPanelInfo AstraDevToolsHelper::GetPanelById(
    const std::string& panel_id) {
  if (panel_id.empty()) {
    return AstraDevToolsPanelInfo();
  }

  const auto& panels = GetRegisteredPanelsInternal();
  for (const auto& panel : panels) {
    if (panel.id == panel_id) {
      return panel;
    }
  }
  return AstraDevToolsPanelInfo();
}

// =========================================================================
// DevTools settings (persisted via PrefService)
// =========================================================================

AstraDevToolsDockState AstraDevToolsHelper::GetDefaultDockState(
    Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return DockStateFromString(prefs::kDefaultDevToolsDefaultDockState);
  }
  std::string state_str = prefs->GetString(prefs::kPrefDevToolsDefaultDockState);
  return DockStateFromString(state_str);
}

void AstraDevToolsHelper::SetDefaultDockState(Profile* profile,
                                              AstraDevToolsDockState dock_state) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  std::string state_str = DockStateToString(dock_state);
  std::string old_value = prefs->GetString(prefs::kPrefDevToolsDefaultDockState);
  if (old_value == state_str) {
    return;
  }

  prefs->SetString(prefs::kPrefDevToolsDefaultDockState, state_str);
  NotifyDevToolsSettingsChanged();
}

std::string AstraDevToolsHelper::GetDefaultPanel(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultDevToolsDefaultPanel;
  }
  return prefs->GetString(prefs::kPrefDevToolsDefaultPanel);
}

void AstraDevToolsHelper::SetDefaultPanel(Profile* profile,
                                          const std::string& panel_id) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  std::string old_value = prefs->GetString(prefs::kPrefDevToolsDefaultPanel);
  if (old_value == panel_id) {
    return;
  }

  prefs->SetString(prefs::kPrefDevToolsDefaultPanel, panel_id);
  NotifyDevToolsSettingsChanged();
}

bool AstraDevToolsHelper::GetAutoOpenDevTools(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultDevToolsAutoOpen;
  }
  return prefs->GetBoolean(prefs::kPrefDevToolsAutoOpen);
}

void AstraDevToolsHelper::SetAutoOpenDevTools(Profile* profile,
                                              bool auto_open) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  bool old_value = prefs->GetBoolean(prefs::kPrefDevToolsAutoOpen);
  if (old_value == auto_open) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDevToolsAutoOpen, auto_open);
  NotifyDevToolsSettingsChanged();
}

bool AstraDevToolsHelper::ToggleAutoOpenDevTools(Profile* profile) {
  bool current = GetAutoOpenDevTools(profile);
  SetAutoOpenDevTools(profile, !current);
  return !current;
}

// =========================================================================
// Shortcut helpers
// =========================================================================

std::string AstraDevToolsHelper::GetDevToolsToggleShortcutText() {
  // Returns the platform-appropriate DevTools toggle shortcut text.
  //
  // On macOS: "Cmd + Opt + I"
  // On Windows/Linux: "Ctrl + Shift + I" or "F12"
  //
  // In a full Chromium build, we would query the accelerator manager
  // for IDC_DEV_TOOLS to get the actual shortcut.  For the overlay
  // skeleton, we return a generic description.
  //
  // Chromium owner: BrowserCommandController / AcceleratorManager
  //   (chrome/browser/ui/browser_command_controller.h)
  //   (ui/base/accelerators/accelerator_manager.h)
#if defined(OS_APPLE)
  return "⌘⌥I (F12)";
#else
  return "Ctrl+Shift+I (F12)";
#endif
}

std::string AstraDevToolsHelper::GetDevToolsOpenShortcutText(
    AstraDevToolsDockState state) {
  // Returns shortcut text for opening DevTools in a specific dock state.
  //
  // In Chromium, there are separate commands for each dock state:
  //   - IDC_DEV_TOOLS — toggle (last used state)
  //   - IDC_DEV_TOOLS_UNDOCKED — open undocked
  //   - etc.
  //
  // For the overlay skeleton, return a generic description.
  switch (state) {
    case AstraDevToolsDockState::kBottom:
      return "Dock to bottom";
    case AstraDevToolsDockState::kRight:
      return "Dock to right";
    case AstraDevToolsDockState::kLeft:
      return "Dock to left";
    case AstraDevToolsDockState::kUndocked:
      return "Undocked (separate window)";
    case AstraDevToolsDockState::kMinimized:
      return "Minimized";
  }
  return "Toggle DevTools";
}

// =========================================================================
// Dock state string conversion
// =========================================================================

std::string AstraDevToolsHelper::DockStateToString(
    AstraDevToolsDockState state) {
  switch (state) {
    case AstraDevToolsDockState::kBottom:
      return "bottom";
    case AstraDevToolsDockState::kRight:
      return "right";
    case AstraDevToolsDockState::kLeft:
      return "left";
    case AstraDevToolsDockState::kUndocked:
      return "undocked";
    case AstraDevToolsDockState::kMinimized:
      return "minimized";
  }
  return "bottom";  // Default fallback.
}

AstraDevToolsDockState AstraDevToolsHelper::DockStateFromString(
    const std::string& state) {
  if (state == "bottom") {
    return AstraDevToolsDockState::kBottom;
  }
  if (state == "right") {
    return AstraDevToolsDockState::kRight;
  }
  if (state == "left") {
    return AstraDevToolsDockState::kLeft;
  }
  if (state == "undocked") {
    return AstraDevToolsDockState::kUndocked;
  }
  if (state == "minimized") {
    return AstraDevToolsDockState::kMinimized;
  }
  // Default to bottom for unknown values (matches Chromium default).
  return AstraDevToolsDockState::kBottom;
}

// =========================================================================
// Observers
// =========================================================================

void AstraDevToolsHelper::AddObserver(Observer* observer) {
  if (!observer) {
    return;
  }
  GetObservers().AddObserver(observer);
}

void AstraDevToolsHelper::RemoveObserver(Observer* observer) {
  if (!observer) {
    return;
  }
  GetObservers().RemoveObserver(observer);
}

void AstraDevToolsHelper::NotifyDevToolsWindowOpened(
    content::WebContents* web_contents) {
  for (auto& observer : GetObservers()) {
    observer.OnDevToolsWindowOpened(web_contents);
  }
}

void AstraDevToolsHelper::NotifyDevToolsWindowClosed(
    content::WebContents* web_contents) {
  for (auto& observer : GetObservers()) {
    observer.OnDevToolsWindowClosed(web_contents);
  }
}

void AstraDevToolsHelper::NotifyDevToolsDockStateChanged(
    content::WebContents* web_contents,
    AstraDevToolsDockState dock_state) {
  for (auto& observer : GetObservers()) {
    observer.OnDevToolsDockStateChanged(web_contents, dock_state);
  }
}

void AstraDevToolsHelper::NotifyAstraPanelVisibilityChanged(bool visible) {
  for (auto& observer : GetObservers()) {
    observer.OnAstraPanelVisibilityChanged(visible);
  }
}

void AstraDevToolsHelper::NotifyDevToolsSettingsChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnDevToolsSettingsChanged();
  }
}

// =========================================================================
// Internal helpers
// =========================================================================

void* AstraDevToolsHelper::GetDevToolsWindowForContents(
    content::WebContents* web_contents) {
  // Internal helper to get the DevToolsWindow for a WebContents.
  //
  // In the real Chromium build, this would return DevToolsWindow*.
  // The void* return type is used here to avoid including DevTools
  // headers in the header file.
  //
  // Chromium owner: DevToolsWindow::FindForInspectedWebContents
  //   or DevToolsWindow::GetInstanceForInspectedWebContents
  //   (chrome/browser/devtools/devtools_window.h)
  //
  // TODO(astra): Implement with actual DevToolsWindow lookup.
  //   Will need to include chrome/browser/devtools/devtools_window.h
  //   and call the appropriate static getter method.
  if (!web_contents) {
    return nullptr;
  }

  DVLOG(1) << "AstraDevToolsHelper::GetDevToolsWindowForContents: placeholder";
  return nullptr;
}

base::ObserverList<AstraDevToolsHelper::Observer>&
AstraDevToolsHelper::GetObservers() {
  static base::ObserverList<Observer> observers;
  return observers;
}

std::vector<AstraDevToolsPanelInfo>&
AstraDevToolsHelper::GetRegisteredPanelsInternal() {
  // Static list of registered Astra DevTools panels.
  //
  // In a production build, these would be registered at startup by
  // various Astra features (workspace service, tab stack service, etc.).
  // For the overlay skeleton, we initialize with the built-in panels
  // on first access.
  //
  // TODO(astra): Move panel registration to feature-specific init
  //   code so each feature registers its own panel.  The helper
  //   should not hardcode panel names.
  static std::vector<AstraDevToolsPanelInfo> panels;
  static bool initialized = false;

  if (!initialized) {
    initialized = true;

    // Register built-in Astra panels.
    AstraDevToolsPanelInfo workspace_panel;
    workspace_panel.id = kPanelWorkspaceInspector;
    workspace_panel.name = u"Workspace Inspector";
    workspace_panel.icon = "workspace";
    workspace_panel.enabled = true;
    workspace_panel.is_builtin = true;
    panels.push_back(workspace_panel);

    AstraDevToolsPanelInfo tab_stack_panel;
    tab_stack_panel.id = kPanelTabStackViewer;
    tab_stack_panel.name = u"Tab Stack Viewer";
    tab_stack_panel.icon = "tab-stack";
    tab_stack_panel.enabled = true;
    tab_stack_panel.is_builtin = true;
    panels.push_back(tab_stack_panel);

    AstraDevToolsPanelInfo favorite_panel;
    favorite_panel.id = kPanelFavoriteManager;
    favorite_panel.name = u"Favorite Manager";
    favorite_panel.icon = "favorite";
    favorite_panel.enabled = true;
    favorite_panel.is_builtin = true;
    panels.push_back(favorite_panel);

    AstraDevToolsPanelInfo focus_panel;
    focus_panel.id = kPanelFocusMode;
    focus_panel.name = u"Focus Mode";
    focus_panel.icon = "focus";
    focus_panel.enabled = false;  // Disabled by default.
    focus_panel.is_builtin = true;
    panels.push_back(focus_panel);
  }

  return panels;
}

PrefService* AstraDevToolsHelper::GetPrefs(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

}  // namespace astra
