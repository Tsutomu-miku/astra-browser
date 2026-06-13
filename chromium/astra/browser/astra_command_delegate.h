#ifndef ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_
#define ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

class Browser;
class PrefService;
class Profile;

namespace content {
class WebContents;
}

namespace astra {

// =========================================================================
// Command ID range
// =========================================================================
//
// Astra-specific command IDs start at 60000.  This avoids collisions with
// Chromium's built-in command IDs, which live in the range defined by
// chrome::CommandId / IDC_FIRST (0) through IDC_LAST (~59999).
//
// The patch point in chrome/browser/ui/browser_command_controller.cc checks
// whether a command ID falls in the Astra range and, if so, forwards
// execution to AstraCommandDelegate instead of handling it through the
// standard Chrome command table.
//
// === BOUNDARY RULE (non-negotiable) ===
//   Standard browser commands (new tab, back, forward, reload, close, zoom,
//   find, downloads, history, settings, etc.) MUST NOT be added here.  They
//   go through Chrome's BrowserCommandController unchanged.  Only
//   Astra-specific product commands belong in this enum.
//
//   If you find yourself wanting to add a command that Chrome already has,
//   stop.  Use the Chrome command ID and route it through the existing
//   command infrastructure.  The whole point of this delegate is to extend,
//   not replace, Chrome's command system.
// =========================================================================

enum AstraCommandId {
  // Sentinel: first Astra command ID.
  kAstraCommandFirst = 60000,

  // -- Sidebar -----------------------------------------------------------
  kAstraCommandToggleSidebar = kAstraCommandFirst,
  kAstraCommandToggleSidebarPin,

  // -- Workspaces --------------------------------------------------------
  kAstraCommandNewWorkspace,
  kAstraCommandNextWorkspace,
  kAstraCommandPreviousWorkspace,
  kAstraCommandRenameWorkspace,
  kAstraCommandDeleteWorkspace,
  kAstraCommandMoveTabToNextWorkspace,
  kAstraCommandMoveTabToPreviousWorkspace,
  kAstraCommandShowAllWorkspaces,

  // -- Multi-window workspaces -------------------------------------------
  kAstraCommandNewWindowInWorkspace,
  kAstraCommandMoveWindowToNextWorkspace,
  kAstraCommandMoveWindowToPreviousWorkspace,

  // -- Tab features ------------------------------------------------------
  kAstraCommandToggleTabFavorite,

  // -- Recently closed tabs ---------------------------------------------
  // Reopen the most recently closed tab (restore last closed tab).
  //
  // NOTE: Chromium already provides IDC_RESTORE_TAB
  //   (chrome/browser/ui/commands/command_ids.h)
  //   for restoring the most recently closed tab.  Astra reuses Chromium's
  //   built-in TabRestoreService for the actual restore logic.
  //
  //   This Astra command provides the same functionality but routes through
  //   the Astra command delegate for sidebar integration (e.g. command
  //   palette listing, sidebar section highlighting on restore).
  //
  //   TODO(astra): Evaluate whether to keep this as a separate command or
  //     just use Chromium's IDC_RESTORE_TAB directly.  The Astra version is
  //     useful for command palette and sidebar integration, but it's also
  //     a duplicate of standard Chrome functionality.
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  // Chromium command: IDC_RESTORE_TAB
  //   (chrome/browser/ui/commands/command_ids.h)
  kAstraCommandReopenClosedTab,

  // Restore all recently closed tabs.
  //
  // This is an Astra-specific command — Chromium does not have a built-in
  // "restore all" command for recently closed tabs.
  //
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  kAstraCommandRestoreAllClosedTabs,

  // NOTE: There is no kAstraCommandMuteTab because Chromium already
  //   provides IDC_MUTE_TAB (chrome/browser/ui/commands/command_ids.h)
  //   for muting the active tab.  Astra reuses Chromium's built-in
  //   mute command rather than creating its own.
  //
  //   Per-tab mute in the sidebar audio indicator calls
  //   content::WebContents::SetAudioMuted() directly and does not need
  //   a command ID.
  //
  //   TODO(astra): Verify IDC_MUTE_TAB works correctly when building
  //     against the full Chromium tree.  The sidebar's audio indicator
  //     uses direct WebContents API calls, but keyboard accelerators
  //     for mute should go through Chromium's command system.
  // Chromium owner: BrowserCommandController
  //   (chrome/browser/ui/browser_command_controller.h)
  // Chromium command: IDC_MUTE_TAB (chrome/browser/ui/commands/command_ids.h)

  // -- Favorites / favorite folders --------------------------------------
  kAstraCommandCreateFavoriteFolder,
  kAstraCommandRenameFavoriteFolder,
  kAstraCommandDeleteFavoriteFolder,
  kAstraCommandMoveFavoriteToFolder,
  kAstraCommandToggleFavoriteFolderExpanded,

  // -- Split view --------------------------------------------------------
  kAstraCommandToggleSplitView,
  kAstraCommandSplitViewVertical,
  kAstraCommandSplitViewHorizontal,
  kAstraCommandSwapSplitViews,

  // -- Glance / Peek -----------------------------------------------------
  kAstraCommandOpenGlance,

  // -- Command palette ---------------------------------------------------
  kAstraCommandOpenCommandPalette,

  // -- Tab search --------------------------------------------------------
  kAstraCommandOpenTabSearch,

  // -- Settings ----------------------------------------------------------
  kAstraCommandOpenSettings,

  // -- Search engines / search settings -----------------------------------
  // Opens Chrome's search engine settings (chrome://settings/searchEngines).
  //
  // Search engine state is fully owned by Chromium's TemplateURLService.
  // This command provides quick access to Chrome's search engine management
  // from the Astra command palette and settings.
  //
  // Chromium owner: TemplateURLService
  //   (components/search_engines/template_url_service.h)
  // Chromium WebUI: chrome://settings/searchEngines
  //   (chrome/browser/resources/settings/search_engines_page/)
  //
  // TODO(astra): Consider whether this should open the search section of
  //   Astra settings (with default engine selector) instead of jumping
  //   directly to Chrome settings.  Option A (Astra UI) vs Option B
  //   (delegate to Chrome).  Currently follows Option B.
  kAstraCommandOpenSearchSettings,

  // -- Profile menu / workspace avatar -----------------------------------
  kAstraCommandSwitchWorkspaceMenu,

  // -- Extensions panel --------------------------------------------------
  kAstraCommandToggleExtensionsPanel,

  // -- Workspace import / export -----------------------------------------
  kAstraCommandExportWorkspaces,
  kAstraCommandImportWorkspaces,

  // -- Omnibox / address bar ---------------------------------------------
  kAstraCommandFocusOmniboxCommandMode,

  // -- Focus mode --------------------------------------------------------
  kAstraCommandToggleFocusMode,

  // -- Screenshot / screen capture --------------------------------------
  kAstraCommandScreenshotVisible,
  kAstraCommandScreenshotFullPage,
  kAstraCommandScreenshotRegion,

  // -- DevTools ----------------------------------------------------------
  kAstraCommandToggleDevTools,
  kAstraCommandDevToolsDockBottom,
  kAstraCommandDevToolsDockRight,
  kAstraCommandDevToolsDockLeft,
  kAstraCommandDevToolsUndock,
  kAstraCommandOpenAstraDevToolsPanel,

  // -- New tab page ------------------------------------------------------
  // Opens the Astra-branded new tab page as a Views-based overlay.
  // This is an alternative to Chrome's built-in WebUI NTP for Astra users.
  //
  // Chromium owner: NewTabPageUI
  //   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
  // Patch point: chrome/browser/new_tab_page/new_tab_page_url_handler.cc
  //   — to redirect chrome://newtab to the Astra NTP.
  //
  // TODO(astra): Proper integration with Chrome's new tab creation path
  // so that pressing Ctrl/Cmd+T or clicking the new tab button shows the
  // Astra NTP instead of Chrome's WebUI NTP.
  kAstraCommandOpenNewTabPage,

  // Sentinel: one past the last Astra command ID.
  kAstraCommandLast,
};

// =========================================================================
// Command categories
// =========================================================================
//
// Logical grouping of Astra commands for the command palette, keyboard
// shortcut settings, and other UI surfaces that need to present commands
// organized by function.
//
// Categories are purely organizational — they do not affect execution
// semantics or command routing.
enum class AstraCommandCategory {
  kWorkspace = 0,   // Workspace management (create, switch, rename, etc.)
  kTab,             // Tab manipulation (favorites, closed tabs, devtools)
  kView,            // View / UI presentation (sidebar, split view, glance, NTP)
  kNavigation,      // Navigation-related (omnibox, command palette, tab search)
  kTools,           // Tools and utilities (screenshot, settings, extensions)
};

// =========================================================================
// Command metadata
// =========================================================================
//
// Human-readable information about an Astra command.  Used by the command
// palette, keyboard shortcut UI, and other surfaces that need to display
// information about available commands.
//
// The metadata is static — it describes the command itself, not its runtime
// state (enabled/disabled is computed separately via IsCommandEnabled).
struct AstraCommandInfo {
  int command_id = -1;
  std::string name;
  std::string description;
  AstraCommandCategory category = AstraCommandCategory::kTools;
};

// =========================================================================
// Command bridge between Chrome and Astra
// =========================================================================
//
// AstraCommandDelegate is the entry point for all Astra-specific commands.
// It sits behind a small patch in Chrome's BrowserCommandController that
// forwards any command ID in the Astra range (60000+) to this delegate.
//
// Architecture:
//
//   Chromium command input           Chromium patch             Astra layer
//   (accelerators, menus, ...)       (tiny forwarder)           (product logic)
//   --------------------    -----------------------------    ----------------
//   IDC_NEW_TAB            ->   handled normally by   ->   (not our problem)
//   IDC_BACK               ->   BrowserCommandController
//   ...
//   kAstraCommandToggle*   ->   IsAstraCommand()?     ->   ExecuteCommand()
//                              yes: forward to Astra       -> services + observers
//
// Three categories of commands:
//
//   1. Workspace commands — routed to AstraWorkspaceService
//      (ProfileKeyedService obtained from the browser's profile via
//      AstraWorkspaceServiceFactory).
//
//   2. Tab metadata commands — read/write AstraTabFeatures
//      (WebContentsUserData attached to the active WebContents).
//
//   3. UI commands — dispatched to registered Observer implementations.
//      The browser layer never depends on Views directly.
//
// Command delegation policy:
//   - Standard Chrome command IDs (0–59999): handled entirely by Chrome's
//     BrowserCommandController.  Never intercepted or reimplemented here.
//   - Astra command IDs (60000+): forwarded from the BrowserCommandController
//     patch point to AstraCommandDelegate.  Handled by Astra product logic.
//   - The patch is a two-line forwarder: if (IsAstraCommand(id)) return
//     AstraCommandDelegate::ExecuteCommand(browser, id).
//   - AstraCommandDelegate never handles a standard Chrome command.
//     BrowserCommandController never handles an Astra command.
//
// TODO(astra): Register Astra accelerators in the Chromium accelerator
// table.  Patch point: chrome/browser/ui/views/accelerator_table.cc
// Add Astra command IDs with their keybindings to kAcceleratorMap or a
// separate Astra accelerator table that gets merged at build time.
// The command palette keybinding (e.g. Ctrl/Cmd+Shift+P or Ctrl/Cmd+K)
// should be registered there — the accelerator system routes directly to
// BrowserCommandController, which then forwards to Astra via the patch.
// =========================================================================

class AstraCommandDelegate {
 public:
  // =======================================================================
  // Observer interface for UI-layer command handling
  // =======================================================================
  //
  // Commands that require UI interaction (sidebar toggling, command palette,
  // workspace overview, etc.) are dispatched through this observer interface
  // rather than having the browser layer depend directly on Views code.
  //
  // The UI layer (e.g. AstraBrowserView in astra/ui/views) implements this
  // interface and registers with the delegate.  This follows the same
  // separation pattern as Chromium's Browser / BrowserView split:
  //   - Browser-layer logic is model-only (services, metadata).
  //   - UI concerns live in the views layer and react through observer
  //     interfaces.
  //   - Browser code never depends on views code.
  //
  // Inherits from base::CheckedObserver so observers can safely be removed
  // while iteration is in progress (e.g. an observer unregisters itself
  // inside a notification callback).
  //
  // All observer methods have default empty implementations so that
  // subclasses can override only the methods they care about.
  //
  // TODO(astra): Consider whether this should be per-Browser or global.
  // Currently the delegate uses a global observer list because it is
  // stateless and all-static.  If we later need per-window command routing
  // (e.g. which browser window the command palette should open in), we may
  // want to move the observer list onto a per-Browser helper object.
  // Chromium patch point: chrome/browser/ui/browser.h — add an Astra command
  // delegate member, analogous to BrowserCommandController.
  // =======================================================================
  class Observer : public base::CheckedObserver {
   public:
    // Sidebar visibility should toggle.
    virtual void OnToggleSidebar() {}

    // Sidebar pin state should toggle.
    virtual void OnToggleSidebarPin() {}

    // The command palette should open.
    virtual void OnOpenCommandPalette() {}

    // The tab search bubble should open.
    virtual void OnOpenTabSearch() {}

    // Glance / peek mode should open.
    virtual void OnOpenGlance() {}

    // The all-workspaces overview view should show.
    virtual void OnShowAllWorkspaces() {}

    // Workspace navigation was requested (next or previous).
    // |direction| is +1 for next, -1 for previous.
    virtual void OnWorkspaceNavigateRequested(int direction) {}

    // Split view state has changed.
    virtual void OnSplitViewStateChanged() {}

    // Favorite folders or favorite membership has changed.
    virtual void OnFavoriteFoldersChanged() {}

    // A recently closed tab was restored.
    // The UI may use this to highlight the recently closed section or
    // update the sidebar's recently closed list.
    //
    // Chromium owner: sessions::TabRestoreService
    //   (chrome/browser/sessions/tab_restore_service.h)
    //
    // TODO(astra): Consider whether this notification is needed, or if
    //   the UI should just listen to TabRestoreServiceObserver directly.
    virtual void OnRecentlyClosedTabRestored() {}

    // All recently closed tabs were restored.
    virtual void OnAllRecentlyClosedTabsRestored() {}

    // The Astra settings page / bubble should open.
    virtual void OnOpenSettings() {}

    // Chrome's search engine settings page should open.
    //
    // Search engine state is fully owned by Chromium's TemplateURLService.
    // This command provides quick access to Chrome's search engine management
    // UI from the Astra command palette.
    //
    // Chromium owner: TemplateURLService
    //   (components/search_engines/template_url_service.h)
    // Chromium WebUI: chrome://settings/searchEngines
    //   (chrome/browser/resources/settings/search_engines_page/)
    // Chromium dialog: SearchEngineDialog
    //   (chrome/browser/ui/search_engines/search_engine_dialog.h)
    //
    // TODO(astra): Consider whether this should open the search section of
    //   the Astra settings page instead, or show a SearchEngineDialog modal.
    virtual void OnOpenSearchSettings() {}

    // The workspace switcher menu should be shown.
    virtual void OnSwitchWorkspaceMenu() {}

    // The extensions panel in the sidebar should be toggled.
    virtual void OnToggleExtensionsPanel() {}

    // The workspace export dialog should open.
    virtual void OnExportWorkspaces() {}

    // The workspace import dialog should open.
    virtual void OnImportWorkspaces() {}

    // The omnibox should be focused in command mode.
    virtual void OnFocusOmniboxCommandMode() {}

    // Focus mode should be toggled on/off.
    virtual void OnToggleFocusMode() {}

    // Screenshot: capture the visible area.
    virtual void OnScreenshotVisible() {}

    // Screenshot: capture the full page.
    virtual void OnScreenshotFullPage() {}

    // Screenshot: capture a user-selected region.
    virtual void OnScreenshotRegion() {}

    // The Astra new tab page should be shown.
    //
    // This is a UI-level command that opens the Astra-branded new tab page
    // as a Views-based overlay / bubble.  It provides an alternative to
    // Chrome's WebUI new tab page for Astra users.
    //
    // Chromium owner: NewTabPageUI
    //   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
    // Patch point: chrome/browser/new_tab_page/new_tab_page_url_handler.cc
    //   — to redirect chrome://newtab to a custom Astra page.
    //
    // TODO(astra): Proper NTP integration with tab creation — the new tab
    // should show the Astra NTP instead of Chrome's default NTP.
    virtual void OnOpenNewTabPage() {}

    // ---------------------------------------------------------------------
    // General command lifecycle notifications
    // ---------------------------------------------------------------------

    // Called after any Astra command is successfully executed.
    // |command_id| is the ID of the command that was executed.
    // This is a catch-all notification useful for the command palette,
    // recent commands tracking UI, and analytics.
    virtual void OnCommandExecuted(int command_id) {}

    // Called when the recent commands list changes.
    // Observers can use GetRecentCommands() to get the updated list.
    virtual void OnRecentCommandsChanged() {}

    // Called when command aliases are added or removed.
    virtual void OnCommandAliasesChanged() {}

   protected:
    ~Observer() override = default;
  };

  AstraCommandDelegate() = delete;
  AstraCommandDelegate(const AstraCommandDelegate&) = delete;
  AstraCommandDelegate& operator=(const AstraCommandDelegate&) = delete;

  // -- Command identity --------------------------------------------------

  // Returns true if |command_id| falls in the Astra command ID range.
  static bool IsAstraCommand(int command_id);

  // Returns true if this delegate has a handler for |command_id|.
  static bool SupportsCommand(int command_id);

  // -- Execution ---------------------------------------------------------

  // Executes |command_id| in the context of |browser|.
  // Returns true if the command was recognized and handled.
  static bool ExecuteCommand(Browser* browser, int command_id);

  // -- Enabled state -----------------------------------------------------

  // Returns whether |command_id| is currently enabled for |browser|.
  static bool IsCommandEnabled(Browser* browser, int command_id);

  // -- Command metadata --------------------------------------------------

  // Returns metadata about |command_id| in |out_info|.
  // Returns true if |command_id| is a valid Astra command.
  static bool GetCommandInfo(int command_id, AstraCommandInfo* out_info);

  // Returns the human-readable name of |command_id|.
  // Returns an empty string if the command is not recognized.
  static std::string GetCommandName(int command_id);

  // Returns the description of |command_id|.
  // Returns an empty string if the command is not recognized.
  static std::string GetCommandDescription(int command_id);

  // Returns the category of |command_id|.
  // DCHECKs if the command is not recognized.
  static AstraCommandCategory GetCommandCategory(int command_id);

  // Returns all Astra command IDs in ascending order.
  static std::vector<int> GetAllCommandIds();

  // Returns all command IDs in the given category.
  static std::vector<int> GetCommandsByCategory(AstraCommandCategory category);

  // -- Recent commands ---------------------------------------------------

  // Returns the list of recently executed command IDs, most recent first.
  // The list is capped at the max recent commands setting.
  //
  // Persistence: stored per-profile via PrefService.
  // Chromium subsystem: PrefService (components/prefs/)
  static std::vector<int> GetRecentCommands(Profile* profile);

  // Clears the recent commands history.
  // Fires OnRecentCommandsChanged observer notification.
  static void ClearRecentCommands(Profile* profile);

  // Returns the maximum number of recent commands to remember.
  static int GetMaxRecentCommands(Profile* profile);

  // Sets the maximum number of recent commands to remember.
  // If the current list is longer than |max|, it is truncated.
  // Fires OnRecentCommandsChanged observer notification if the list changes.
  static void SetMaxRecentCommands(Profile* profile, int max);

  // -- Command aliases ---------------------------------------------------

  // Returns all aliases for |command_id|.
  //
  // Aliases provide alternate names for commands, useful for the command
  // palette fuzzy matching and keyboard shortcuts.
  static std::vector<std::string> GetCommandAliases(Profile* profile,
                                                    int command_id);

  // Adds an alias for |command_id|.
  // Returns true on success.  Returns false if |alias| is already used by
  // another command, or if |command_id| is not a valid Astra command.
  // Fires OnCommandAliasesChanged observer notification on success.
  //
  // Persistence: stored per-profile via PrefService.
  static bool AddCommandAlias(Profile* profile,
                              int command_id,
                              const std::string& alias);

  // Removes an alias.  No-op if the alias doesn't exist.
  // Fires OnCommandAliasesChanged observer notification if an alias was removed.
  static void RemoveCommandAlias(Profile* profile, const std::string& alias);

  // Returns the command ID for the given alias, or -1 if not found.
  static int GetCommandByAlias(Profile* profile, const std::string& alias);

  // Returns all registered aliases.
  static std::vector<std::string> GetAllAliases(Profile* profile);

  // -- Observers ---------------------------------------------------------

  // Registers an observer for UI-level command notifications.
  static void AddObserver(Observer* observer);

  // Unregisters an observer.
  static void RemoveObserver(Observer* observer);

 private:
  // Returns the active WebContents from |browser|'s tab strip.
  static content::WebContents* GetActiveWebContents(Browser* browser);

  // Returns the AstraWorkspaceService for |browser|'s profile.
  static class AstraWorkspaceService* GetWorkspaceService(Browser* browser);

  // Returns the AstraFavoriteService for |browser|'s profile.
  static class AstraFavoriteService* GetFavoriteService(Browser* browser);

  // Returns the AstraFocusModeService for |browser|'s profile.
  static class AstraFocusModeService* GetFocusModeService(Browser* browser);

  // Returns the AstraScreenshotService for |browser|'s profile.
  static class AstraScreenshotService* GetScreenshotService(Browser* browser);

  // Returns the static observer list.
  static base::ObserverList<Observer>& GetObservers();

  // Records a command execution in the recent commands history.
  // Called from ExecuteCommand after a command succeeds.
  // The command is moved to the front of the list if already present.
  static void RecordRecentCommand(Profile* profile, int command_id);

  // Helper: gets PrefService from a profile, handling null and incognito.
  static PrefService* GetPrefsForProfile(Profile* profile);

  // Notifies all observers that the recent commands list changed.
  static void NotifyRecentCommandsChanged();

  // Notifies all observers that command aliases changed.
  static void NotifyCommandAliasesChanged();

  // Notifies all observers that a command was executed.
  static void NotifyCommandExecuted(int command_id);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_COMMAND_DELEGATE_H_
