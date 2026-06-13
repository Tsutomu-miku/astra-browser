#include "astra/ui/views/command_palette/astra_command_palette_model.h"

#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Command type names
// =========================================================================

const char16_t* GetCommandTypeName(AstraCommandType type) {
  switch (type) {
    case AstraCommandType::kAction:
      return u"Action";
    case AstraCommandType::kNavigation:
      return u"Navigation";
    case AstraCommandType::kWorkspace:
      return u"Workspace";
    case AstraCommandType::kSetting:
      return u"Setting";
    case AstraCommandType::kBookmark:
      return u"Bookmark";
    case AstraCommandType::kHistory:
      return u"History";
    case AstraCommandType::kTab:
      return u"Tab";
    case AstraCommandType::kExtension:
      return u"Extension";
  }
  return u"";
}

// =========================================================================
// Category labels
// =========================================================================

const char16_t* GetCategoryLabel(AstraCommandCategory category) {
  switch (category) {
    case AstraCommandCategory::kTabs:
      return u"Tabs";
    case AstraCommandCategory::kNavigation:
      return u"Navigation";
    case AstraCommandCategory::kWorkspaces:
      return u"Workspaces";
    case AstraCommandCategory::kBookmarks:
      return u"Bookmarks";
    case AstraCommandCategory::kHistory:
      return u"History";
    case AstraCommandCategory::kActions:
      return u"Actions";
    case AstraCommandCategory::kView:
      return u"View";
    case AstraCommandCategory::kTools:
      return u"Tools";
    case AstraCommandCategory::kSettings:
      return u"Settings";
    case AstraCommandCategory::kHelp:
      return u"Help";
  }
  return u"";
}

const char* GetCategoryIconName(AstraCommandCategory category) {
  switch (category) {
    case AstraCommandCategory::kTabs:
      return "tab";
    case AstraCommandCategory::kNavigation:
      return "arrow_forward";
    case AstraCommandCategory::kWorkspaces:
      return "workspace";
    case AstraCommandCategory::kBookmarks:
      return "bookmark";
    case AstraCommandCategory::kHistory:
      return "history";
    case AstraCommandCategory::kActions:
      return "bolt";
    case AstraCommandCategory::kView:
      return "visibility";
    case AstraCommandCategory::kTools:
      return "build";
    case AstraCommandCategory::kSettings:
      return "settings";
    case AstraCommandCategory::kHelp:
      return "help";
  }
  return "help";
}

namespace {

// =========================================================================
// Chrome command index
// =========================================================================
//
// Curated list of the most useful Chrome commands for the command palette.
// Not exhaustive — just the ~40 commands users are most likely to invoke
// from a quick launcher.
//
// TODO(astra): Replace this hardcoded list with a dynamic enumeration of
// Chrome commands, ideally by querying the accelerator table or
// CommandUpdater at runtime.  Patch point:
// chrome/browser/ui/browser_command_controller.h — expose a way to iterate
// all registered command IDs and their labels.
//
// TODO(astra): Localize display names and descriptions.  These strings
// are currently in English hardcoded form; they should come from Chrome's
// string resources (IDS_* in chrome/app/generated_resources.grd) for
// Chrome commands, and from Astra's own string bundle for Astra commands.
// =========================================================================

struct ChromeCommandEntry {
  int command_id;
  const char16_t* title;
  const char16_t* description;
  const char16_t* shortcut;
  AstraCommandType type;
  AstraCommandCategory category;
  const char* icon_name;
};

const ChromeCommandEntry kChromeCommands[] = {
    // -- Tabs & windows ----------------------------------------------------
    {IDC_NEW_TAB, u"New Tab", u"Open a new tab", u"⌘T",
     AstraCommandType::kTab, AstraCommandCategory::kTabs, "new_tab"},
    {IDC_NEW_WINDOW, u"New Window", u"Open a new browser window", u"⌘N",
     AstraCommandType::kTab, AstraCommandCategory::kTabs, "new_window"},
    {IDC_NEW_INCOGNITO_WINDOW,
     u"New Incognito Window",
     u"Open a new window in incognito mode",
     u"⇧⌘N",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "incognito"},
    {IDC_CLOSE_TAB, u"Close Tab", u"Close the current tab", u"⌘W",
     AstraCommandType::kTab, AstraCommandCategory::kTabs, "close"},
    {IDC_CLOSE_WINDOW,
     u"Close Window",
     u"Close the current window",
     u"⇧⌘W",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "close_window"},
    {IDC_SELECT_TAB_0, u"Switch to Tab 1", u"Jump to the first tab", u"⌘1",
     AstraCommandType::kTab, AstraCommandCategory::kTabs, "tab_1"},
    {IDC_SELECT_NEXT_TAB,
     u"Next Tab",
     u"Switch to the next tab",
     u"⌘⇥",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "arrow_right"},
    {IDC_SELECT_PREVIOUS_TAB,
     u"Previous Tab",
     u"Switch to the previous tab",
     u"⇧⌘⇥",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "arrow_left"},
    {IDC_MOVE_TAB_TO_NEW_WINDOW,
     u"Move Tab to New Window",
     u"Tear off the current tab into a new window",
     u"",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "move_tab"},
    {IDC_DUPLICATE_TAB,
     u"Duplicate Tab",
     u"Open the current page in a new tab",
     u"",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "duplicate"},
    {IDC_PIN_TAB, u"Pin Tab", u"Pin or unpin the current tab", u"",
     AstraCommandType::kTab, AstraCommandCategory::kTabs, "pin"},
    {IDC_MUTE_TAB_SITE,
     u"Mute Tab",
     u"Mute or unmute the current tab",
     u"",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "mute"},

    // -- Navigation --------------------------------------------------------
    {IDC_BACK, u"Back", u"Go back to the previous page", u"⌘[",
     AstraCommandType::kNavigation, AstraCommandCategory::kNavigation, "back"},
    {IDC_FORWARD, u"Forward", u"Go forward to the next page", u"⌘]",
     AstraCommandType::kNavigation, AstraCommandCategory::kNavigation,
     "forward"},
    {IDC_RELOAD, u"Reload", u"Reload the current page", u"⌘R",
     AstraCommandType::kNavigation, AstraCommandCategory::kNavigation,
     "reload"},
    {IDC_RELOAD_BYPASSING_CACHE,
     u"Hard Reload",
     u"Reload without using cached content",
     u"⇧⌘R",
     AstraCommandType::kNavigation,
     AstraCommandCategory::kNavigation,
     "reload_hard"},
    {IDC_HOME, u"Home", u"Go to the home page", u"",
     AstraCommandType::kNavigation, AstraCommandCategory::kNavigation, "home"},
    {IDC_STOP, u"Stop", u"Stop loading the current page", u"⎋",
     AstraCommandType::kAction, AstraCommandCategory::kNavigation, "stop"},
    {IDC_FOCUS_LOCATION,
     u"Focus Location Bar",
     u"Move focus to the address bar",
     u"⌘L",
     AstraCommandType::kNavigation,
     AstraCommandCategory::kNavigation,
     "search"},
    {IDC_FOCUS_SEARCH,
     u"Focus Search",
     u"Move focus to the search box",
     u"⌘K",
     AstraCommandType::kNavigation,
     AstraCommandCategory::kNavigation,
     "search"},

    // -- Zoom & view -------------------------------------------------------
    {IDC_ZOOM_PLUS, u"Zoom In", u"Increase page zoom level", u"⌘+",
     AstraCommandType::kAction, AstraCommandCategory::kView, "zoom_in"},
    {IDC_ZOOM_MINUS, u"Zoom Out", u"Decrease page zoom level", u"⌘-",
     AstraCommandType::kAction, AstraCommandCategory::kView, "zoom_out"},
    {IDC_ZOOM_NORMAL, u"Actual Size", u"Reset zoom to 100%", u"⌘0",
     AstraCommandType::kAction, AstraCommandCategory::kView, "zoom_reset"},
    {IDC_FULLSCREEN,
     u"Enter Full Screen",
     u"Enter or exit full screen mode",
     u"⌃⌘F",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "fullscreen"},
    {IDC_TOGGLE_FULLSCREEN_TOOLBAR,
     u"Toggle Fullscreen Toolbar",
     u"Show or hide the toolbar in full screen",
     u"⇧⌘F",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "menu"},

    // -- Find & edit -------------------------------------------------------
    {IDC_FIND, u"Find", u"Find text on the page", u"⌘F",
     AstraCommandType::kAction, AstraCommandCategory::kActions, "find"},
    {IDC_FIND_NEXT, u"Find Next", u"Jump to the next match", u"⌘G",
     AstraCommandType::kAction, AstraCommandCategory::kActions, "arrow_down"},
    {IDC_FIND_PREVIOUS,
     u"Find Previous",
     u"Jump to the previous match",
     u"⇧⌘G",
     AstraCommandType::kAction,
     AstraCommandCategory::kActions,
     "arrow_up"},
    {IDC_FIND_STOP, u"Stop Find", u"Dismiss the find bar", u"⎋",
     AstraCommandType::kAction, AstraCommandCategory::kActions, "close"},

    // -- Tools & pages -----------------------------------------------------
    {IDC_SHOW_BOOKMARKS_BAR,
     u"Toggle Bookmarks Bar",
     u"Show or hide the bookmarks bar",
     u"⇧⌘B",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "bookmark_bar"},
    {IDC_BOOKMARK_PAGE,
     u"Bookmark This Page",
     u"Add or remove a bookmark for the current page",
     u"⌘D",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "bookmark"},
    {IDC_BOOKMARK_ALL_TABS,
     u"Bookmark All Tabs",
     u"Bookmark all open tabs in the current window",
     u"⇧⌘D",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "bookmark"},
    {IDC_BOOKMARK_MANAGER,
     u"Bookmark Manager",
     u"Open the bookmark manager",
     u"⌥⌘B",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "bookmark_manager"},

    // -- History -----------------------------------------------------------
    {IDC_SHOW_HISTORY,
     u"History",
     u"Open the history page",
     u"⌘Y",
     AstraCommandType::kHistory,
     AstraCommandCategory::kHistory,
     "history"},
    {IDC_SHOW_RECENTLY_CLOSED,
     u"Recently Closed",
     u"View recently closed tabs and windows",
     u"",
     AstraCommandType::kHistory,
     AstraCommandCategory::kHistory,
     "history"},
    {IDC_RESTORE_TAB,
     u"Reopen Closed Tab",
     u"Restore the most recently closed tab",
     u"⇧⌘T",
     AstraCommandType::kHistory,
     AstraCommandCategory::kHistory,
     "restore_tab"},
    {IDC_CLEAR_BROWSING_DATA,
     u"Clear Browsing Data",
     u"Clear browsing history, cookies, and cache",
     u"⇧⌘⌫",
     AstraCommandType::kAction,
     AstraCommandCategory::kHistory,
     "clear_data"},
    {IDC_SHOW_DOWNLOADS,
     u"Downloads",
     u"Open the downloads page",
     u"⇧⌘J",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "download"},
    {IDC_OPTIONS, u"Settings", u"Open browser settings", u"⌘,",
     AstraCommandType::kSetting, AstraCommandCategory::kSettings, "settings"},
    {IDC_MANAGE_EXTENSIONS,
     u"Extensions",
     u"Manage installed extensions",
     u"",
     AstraCommandType::kExtension,
     AstraCommandCategory::kTools,
     "extension"},
    {IDC_PASSWORD_MANAGER,
     u"Password Manager",
     u"Open the password manager",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "password"},
    {IDC_TASK_MANAGER,
     u"Task Manager",
     u"Open the browser task manager",
     u"⇧⎋",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "task_manager"},

    // -- DevTools ----------------------------------------------------------
    {IDC_DEV_TOOLS,
     u"Developer Tools",
     u"Toggle the developer tools",
     u"⌥⌘I",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "devtools"},
    {IDC_DEV_TOOLS_CONSOLE,
     u"JavaScript Console",
     u"Toggle the JavaScript console",
     u"⌥⌘J",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "console"},
    {IDC_DEV_TOOLS_ELEMENTS,
     u"Inspect Elements",
     u"Inspect the current page elements",
     u"⇧⌘C",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "inspect"},
    {IDC_VIEW_SOURCE,
     u"View Source",
     u"View the page source code",
     u"⌥⌘U",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "code"},
    {IDC_DEV_TOOLS_INSPECT,
     u"Inspect",
     u"Inspect an element on the page",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "inspect"},

    // -- Printing & sharing ------------------------------------------------
    {IDC_PRINT, u"Print", u"Print the current page", u"⌘P",
     AstraCommandType::kAction, AstraCommandCategory::kActions, "print"},
    {IDC_SAVE_PAGE, u"Save Page As", u"Save the current page to disk", u"⌘S",
     AstraCommandType::kAction, AstraCommandCategory::kActions, "save"},

    // -- Settings ----------------------------------------------------------
    {IDC_PRIVACY_SETTINGS,
     u"Privacy and Security",
     u"Open privacy and security settings",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "privacy"},
    {IDC_CONTENT_SETTINGS,
     u"Site Settings",
     u"Manage site permissions and content settings",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "site_settings"},
    {IDC_SEARCH_ENGINE_SETTINGS,
     u"Search Engine Settings",
     u"Manage default search engines",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "search"},
    {IDC_EXTENSIONS_SETTINGS,
     u"Extensions Settings",
     u"Manage extensions and their details",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "extension"},

    // -- Help & about ------------------------------------------------------
    {IDC_ABOUT,
     u"About Chrome",
     u"View version information and updates",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kHelp,
     "info"},
    {IDC_HELP_PAGE,
     u"Help",
     u"Open the help center",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kHelp,
     "help"},
    {IDC_REPORT_BUG,
     u"Report an Issue",
     u"Send feedback or report a bug",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kHelp,
     "feedback"},
};

// =========================================================================
// Astra command index
// =========================================================================
//
// Display metadata for all Astra-specific commands.  Maps each
// AstraCommandId enum value to display metadata.
//
// TODO(astra): Read shortcuts from a proper accelerator registration
// instead of hardcoding.  Patch point:
// chrome/browser/ui/views/accelerator_table.cc — register Astra command
// IDs with their keybindings in kAcceleratorMap or a parallel table.
// =========================================================================

struct AstraCommandEntry {
  int command_id;
  const char16_t* title;
  const char16_t* description;
  const char16_t* shortcut;
  AstraCommandType type;
  AstraCommandCategory category;
  const char* icon_name;
};

const AstraCommandEntry kAstraCommands[] = {
    // -- Sidebar -----------------------------------------------------------
    {kAstraCommandToggleSidebar,
     u"Toggle Sidebar",
     u"Show or hide the Astra sidebar",
     u"⌘\\",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "sidebar"},
    {kAstraCommandToggleSidebarPin,
     u"Toggle Sidebar Pin",
     u"Pin the sidebar open or make it auto-hide",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "pin"},

    // -- Workspaces --------------------------------------------------------
    {kAstraCommandNewWorkspace,
     u"New Workspace",
     u"Create a new workspace",
     u"⌃⌘N",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "add_workspace"},
    {kAstraCommandNextWorkspace,
     u"Next Workspace",
     u"Switch to the next workspace",
     u"⌃⌘→",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_right"},
    {kAstraCommandPreviousWorkspace,
     u"Previous Workspace",
     u"Switch to the previous workspace",
     u"⌃⌘←",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_left"},
    {kAstraCommandRenameWorkspace,
     u"Rename Workspace",
     u"Rename the current workspace",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "edit"},
    {kAstraCommandDeleteWorkspace,
     u"Delete Workspace",
     u"Delete the current workspace",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "delete"},
    {kAstraCommandMoveTabToNextWorkspace,
     u"Move Tab to Next Workspace",
     u"Move the current tab to the next workspace",
     u"⇧⌃⌘→",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_right"},
    {kAstraCommandMoveTabToPreviousWorkspace,
     u"Move Tab to Previous Workspace",
     u"Move the current tab to the previous workspace",
     u"⇧⌃⌘←",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_left"},
    {kAstraCommandShowAllWorkspaces,
     u"Show All Workspaces",
     u"Open the workspace overview",
     u"⇧⌘W",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "workspace_grid"},
    {kAstraCommandNewWindowInWorkspace,
     u"New Window in Workspace",
     u"Open a new window in the current workspace",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "new_window"},
    {kAstraCommandMoveWindowToNextWorkspace,
     u"Move Window to Next Workspace",
     u"Move the current window to the next workspace",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_right"},
    {kAstraCommandMoveWindowToPreviousWorkspace,
     u"Move Window to Previous Workspace",
     u"Move the current window to the previous workspace",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "arrow_left"},
    {kAstraCommandExportWorkspaces,
     u"Export Workspaces",
     u"Export workspaces to a file",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "download"},
    {kAstraCommandImportWorkspaces,
     u"Import Workspaces",
     u"Import workspaces from a file",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "upload"},
    {kAstraCommandSwitchWorkspaceMenu,
     u"Switch Workspace Menu",
     u"Open the workspace switcher menu",
     u"",
     AstraCommandType::kWorkspace,
     AstraCommandCategory::kWorkspaces,
     "menu"},

    // -- Tab features ------------------------------------------------------
    {kAstraCommandToggleTabFavorite,
     u"Toggle Favorite",
     u"Add or remove the current tab from favorites",
     u"⌘D",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "star"},
    {kAstraCommandCreateFavoriteFolder,
     u"Create Favorite Folder",
     u"Create a new favorite folder",
     u"",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "folder"},
    {kAstraCommandRenameFavoriteFolder,
     u"Rename Favorite Folder",
     u"Rename the selected favorite folder",
     u"",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "edit"},
    {kAstraCommandDeleteFavoriteFolder,
     u"Delete Favorite Folder",
     u"Delete the selected favorite folder",
     u"",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "delete"},
    {kAstraCommandMoveFavoriteToFolder,
     u"Move Favorite to Folder",
     u"Move the current favorite to a different folder",
     u"",
     AstraCommandType::kBookmark,
     AstraCommandCategory::kBookmarks,
     "move"},

    // -- Split view --------------------------------------------------------
    {kAstraCommandToggleSplitView,
     u"Toggle Split View",
     u"Enable or disable split view for the current tab",
     u"⌘\\",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "split_view"},
    {kAstraCommandSplitViewVertical,
     u"Split View Vertical",
     u"Arrange split view vertically",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "split_vertical"},
    {kAstraCommandSplitViewHorizontal,
     u"Split View Horizontal",
     u"Arrange split view horizontally",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "split_horizontal"},
    {kAstraCommandSwapSplitViews,
     u"Swap Split Views",
     u"Swap the left and right split views",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "swap"},

    // -- Glance ------------------------------------------------------------
    {kAstraCommandOpenGlance,
     u"Open Glance",
     u"Open a quick glance preview",
     u"⇧Space",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "glance"},

    // -- Command palette ---------------------------------------------------
    {kAstraCommandOpenCommandPalette,
     u"Command Palette",
     u"Open the command palette",
     u"⌘⇧P",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "command_palette"},

    // -- Tab search --------------------------------------------------------
    {kAstraCommandOpenTabSearch,
     u"Tab Search",
     u"Search through open tabs",
     u"",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "search"},

    // -- Settings ----------------------------------------------------------
    {kAstraCommandOpenSettings,
     u"Astra Settings",
     u"Open Astra settings",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "settings"},
    {kAstraCommandOpenSearchSettings,
     u"Search Settings",
     u"Manage search engine settings",
     u"",
     AstraCommandType::kSetting,
     AstraCommandCategory::kSettings,
     "search"},

    // -- Extensions panel --------------------------------------------------
    {kAstraCommandToggleExtensionsPanel,
     u"Toggle Extensions Panel",
     u"Show or hide the extensions panel",
     u"",
     AstraCommandType::kExtension,
     AstraCommandCategory::kView,
     "extension"},

    // -- Focus mode --------------------------------------------------------
    {kAstraCommandToggleFocusMode,
     u"Toggle Focus Mode",
     u"Enter or exit focus mode",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kView,
     "focus"},

    // -- Screenshot --------------------------------------------------------
    {kAstraCommandScreenshotVisible,
     u"Screenshot Visible",
     u"Capture a screenshot of the visible area",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kActions,
     "screenshot"},
    {kAstraCommandScreenshotFullPage,
     u"Screenshot Full Page",
     u"Capture a screenshot of the entire page",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kActions,
     "screenshot"},
    {kAstraCommandScreenshotRegion,
     u"Screenshot Region",
     u"Capture a screenshot of a selected region",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kActions,
     "screenshot"},

    // -- DevTools ----------------------------------------------------------
    {kAstraCommandToggleDevTools,
     u"Toggle DevTools",
     u"Toggle developer tools",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "devtools"},
    {kAstraCommandOpenAstraDevToolsPanel,
     u"Astra DevTools Panel",
     u"Open the Astra DevTools panel",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "devtools"},
    {kAstraCommandDevToolsDockBottom,
     u"Dock DevTools to Bottom",
     u"Dock developer tools to the bottom of the window",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "dock_bottom"},
    {kAstraCommandDevToolsDockRight,
     u"Dock DevTools to Right",
     u"Dock developer tools to the right side",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "dock_right"},
    {kAstraCommandDevToolsDockLeft,
     u"Dock DevTools to Left",
     u"Dock developer tools to the left side",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "dock_left"},
    {kAstraCommandDevToolsUndock,
     u"Undock DevTools",
     u"Undock developer tools into a separate window",
     u"",
     AstraCommandType::kAction,
     AstraCommandCategory::kTools,
     "undock"},

    // -- New tab page ------------------------------------------------------
    {kAstraCommandOpenNewTabPage,
     u"Astra New Tab",
     u"Open the Astra new tab page",
     u"",
     AstraCommandType::kTab,
     AstraCommandCategory::kTabs,
     "new_tab"},

    // -- Omnibox -----------------------------------------------------------
    {kAstraCommandFocusOmniboxCommandMode,
     u"Focus Omnibox (Command Mode)",
     u"Focus the address bar in command mode",
     u"",
     AstraCommandType::kNavigation,
     AstraCommandCategory::kNavigation,
     "command"},

    // -- Recently closed ---------------------------------------------------
    {kAstraCommandReopenClosedTab,
     u"Reopen Closed Tab (Astra)",
     u"Restore the most recently closed tab",
     u"",
     AstraCommandType::kHistory,
     AstraCommandCategory::kHistory,
     "restore_tab"},
    {kAstraCommandRestoreAllClosedTabs,
     u"Restore All Closed Tabs",
     u"Restore all recently closed tabs",
     u"",
     AstraCommandType::kHistory,
     AstraCommandCategory::kHistory,
     "restore"},
};

// Returns a category weight multiplier for ranking.
// Higher values make commands in that category rank higher.
double GetCategoryWeightInternal(AstraCommandCategory category) {
  switch (category) {
    case AstraCommandCategory::kTabs:
      return 0.2;
    case AstraCommandCategory::kWorkspaces:
      return 0.3;
    case AstraCommandCategory::kActions:
      return 0.15;
    case AstraCommandCategory::kView:
      return 0.1;
    case AstraCommandCategory::kBookmarks:
      return 0.05;
    case AstraCommandCategory::kHistory:
      return 0.0;
    case AstraCommandCategory::kNavigation:
      return 0.0;
    case AstraCommandCategory::kTools:
      return 0.0;
    case AstraCommandCategory::kSettings:
      return -0.1;
    case AstraCommandCategory::kHelp:
      return -0.2;
  }
  return 0.0;
}

}  // namespace

// =========================================================================
// Fuzzy match implementation
// =========================================================================
//
// Checks whether all characters of |query| appear in order in |text|.
// Case-insensitive.  Returns true if it's a fuzzy match.
//
// For example:
//   "nt" fuzzy matches "New Tab" (N -> T)
//   "nwt" fuzzy matches "New Window" (N -> W -> T)
//   "abc" does NOT fuzzy match "acb" (wrong order)
// =========================================================================

// static
bool AstraCommandPaletteModel::FuzzyMatch(const std::u16string& query,
                                            const std::u16string& text) {
  if (query.empty()) {
    return true;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string text_lower = base::ToLowerASCII(text);

  size_t query_idx = 0;
  for (size_t i = 0; i < text_lower.size() && query_idx < query_lower.size();
       ++i) {
    if (text_lower[i] == query_lower[query_idx]) {
      ++query_idx;
    }
  }

  return query_idx == query_lower.size();
}

// =========================================================================
// Match ranges
// =========================================================================
//
// Returns the ranges of |text| that match |query|.  For exact and
// substring matches, this is a single range.  For fuzzy matches, this
// is multiple non-contiguous ranges.
// =========================================================================

// static
std::vector<gfx::Range> AstraCommandPaletteModel::GetMatchRanges(
    const std::u16string& query,
    const std::u16string& text) {
  std::vector<gfx::Range> ranges;

  if (query.empty()) {
    return ranges;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string text_lower = base::ToLowerASCII(text);

  // Try exact / substring match first (single range).
  size_t pos = text_lower.find(query_lower);
  if (pos != std::u16string::npos) {
    ranges.emplace_back(static_cast<uint32_t>(pos),
                        static_cast<uint32_t>(pos + query_lower.size()));
    return ranges;
  }

  // Try fuzzy match (multiple ranges, one per matched character).
  size_t query_idx = 0;
  size_t range_start = std::u16string::npos;
  for (size_t i = 0; i < text_lower.size() && query_idx < query_lower.size();
       ++i) {
    if (text_lower[i] == query_lower[query_idx]) {
      if (range_start == std::u16string::npos) {
        range_start = i;
      }
      ++query_idx;
    } else if (range_start != std::u16string::npos) {
      // End of a contiguous match run.
      ranges.emplace_back(static_cast<uint32_t>(range_start),
                          static_cast<uint32_t>(i));
      range_start = std::u16string::npos;
    }
  }

  // Don't forget the last range.
  if (range_start != std::u16string::npos && query_idx == query_lower.size()) {
    // We need to find where the last matching character was.
    // Re-scan from the end of the last range.
    // Actually, we need to handle this more carefully.
    // For simplicity with fuzzy matching, collect individual character ranges.
    ranges.clear();
    query_idx = 0;
    for (size_t i = 0; i < text_lower.size() && query_idx < query_lower.size();
         ++i) {
      if (text_lower[i] == query_lower[query_idx]) {
        ranges.emplace_back(static_cast<uint32_t>(i),
                            static_cast<uint32_t>(i + 1));
        ++query_idx;
      }
    }
  }

  // Only return ranges if the full query matched.
  if (query_idx == query_lower.size()) {
    return ranges;
  }
  return {};
}

// =========================================================================
// Acronym matching
// =========================================================================
//
// Checks if |query| matches as an acronym of |text|.
// An acronym match means each character of the query is the first letter
// of a word in the text, in order.
//
// Examples:
//   "nt" matches "New Tab" (N + T)
//   "dt" matches "Developer Tools" (D + T)
//   "nt" does NOT match "Navigation" (only one word)
// =========================================================================

// static
bool AstraCommandPaletteModel::IsAcronymMatch(const std::u16string& query,
                                              const std::u16string& text) {
  if (query.empty()) {
    return true;
  }
  if (text.empty()) {
    return false;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string text_lower = base::ToLowerASCII(text);

  size_t query_idx = 0;
  bool at_word_start = true;

  for (size_t i = 0; i < text_lower.size() && query_idx < query_lower.size();
       ++i) {
    char16_t c = text_lower[i];

    if (c == ' ' || c == '-' || c == '_' || c == '/') {
      at_word_start = true;
      continue;
    }

    if (at_word_start) {
      if (c == query_lower[query_idx]) {
        ++query_idx;
      }
      at_word_start = false;
    }
  }

  return query_idx == query_lower.size();
}

// =========================================================================
// Word boundary matching
// =========================================================================
//
// Checks if |query| matches on word boundaries of |text|.
// A word boundary match means each word of the query matches the start
// of a word in the text, in order.
//
// Examples:
//   "new tab" matches "New Tab"
//   "dev tool" matches "Developer Tools"
//   "tab new" does NOT match "New Tab" (wrong order)
// =========================================================================

// static
bool AstraCommandPaletteModel::IsWordBoundaryMatch(
    const std::u16string& query,
    const std::u16string& text) {
  if (query.empty()) {
    return true;
  }
  if (text.empty()) {
    return false;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string text_lower = base::ToLowerASCII(text);

  // Split query into words.
  std::vector<std::u16string> query_words;
  size_t start = 0;
  for (size_t i = 0; i <= query_lower.size(); ++i) {
    if (i == query_lower.size() || query_lower[i] == ' ') {
      if (i > start) {
        query_words.push_back(query_lower.substr(start, i - start));
      }
      start = i + 1;
    }
  }

  if (query_words.empty()) {
    return true;
  }

  // Walk through text word starts, checking each query word matches
  // the start of a text word, in order.
  size_t word_idx = 0;
  bool at_word_start = true;

  for (size_t i = 0; i < text_lower.size() && word_idx < query_words.size();
       ++i) {
    char16_t c = text_lower[i];

    if (c == ' ' || c == '-' || c == '_' || c == '/') {
      at_word_start = true;
      continue;
    }

    if (at_word_start) {
      // Check if the current query word matches at this position.
      const auto& word = query_words[word_idx];
      if (i + word.size() <= text_lower.size() &&
          text_lower.substr(i, word.size()) == word) {
        ++word_idx;
        i += word.size() - 1;  // Skip the matched word.
      }
      at_word_start = false;
    }
  }

  return word_idx == query_words.size();
}

// =========================================================================
// Relevance scoring
// =========================================================================
//
// Score tiers (higher = better match):
//   - Exact match on title:       2000
//   - Prefix match on title:      1000
//   - Substring match on title:   500 - pos
//   - Exact match on description: 300
//   - Prefix match on description: 200
//   - Substring match on desc:    100 - pos
//   - Fuzzy match on title:       50 + bonus for contiguous chars
//   - Fuzzy match on description: 25
//
// Multipliers:
//   - Category weight:            * (1.0 + weight)
//   - Recent command boost:       + 300
//   - Usage count boost:          + use_count * 5
//
// Higher scores win.  Returns a negative value if no match.
// =========================================================================

double AstraCommandPaletteModel::ComputeRelevanceScore(
    const std::u16string& query,
    const AstraCommandItem& item) const {
  if (query.empty()) {
    // Empty query: commands get a base score.  Recent commands get a boost.
    double base_score = 10.0;
    if (item.is_recent) {
      base_score += 500.0;
    }
    // Apply category weight as a multiplier on the base.
    double cat_weight = GetCategoryWeightInternal(item.category);
    base_score *= (1.0 + cat_weight);
    // Add usage count boost.
    base_score += item.use_count * 5.0;
    return base_score;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string title_lower = base::ToLowerASCII(item.title);
  std::u16string desc_lower = base::ToLowerASCII(item.description);

  double score = -1.0;

  // Exact match on title — highest relevance.
  if (title_lower == query_lower) {
    score = 2000.0;
  }
  // Prefix match on title — very high relevance.
  else if (base::StartsWith(title_lower, query_lower)) {
    score = 1000.0;
  }
  // Substring match on title — medium relevance.
  else if (title_lower.find(query_lower) != std::u16string::npos) {
    size_t pos = title_lower.find(query_lower);
    score = 500.0 - static_cast<double>(pos);
  }
  // Fuzzy match on title — lower relevance.
  else if (enable_fuzzy_search_ && FuzzyMatch(query, item.title)) {
    score = 50.0;
  }
  // Exact match on description — medium-high relevance.
  else if (desc_lower == query_lower) {
    score = 300.0;
  }
  // Prefix match on description — medium relevance.
  else if (base::StartsWith(desc_lower, query_lower)) {
    score = 200.0;
  }
  // Substring match on description — lower relevance.
  else if (desc_lower.find(query_lower) != std::u16string::npos) {
    size_t pos = desc_lower.find(query_lower);
    score = 100.0 - static_cast<double>(pos);
  }
  // Fuzzy match on description — lowest relevance.
  else if (enable_fuzzy_search_ && FuzzyMatch(query, item.description)) {
    score = 25.0;
  }

  // Search in command IDs if enabled.
  if (search_in_command_ids_ && score < 0) {
    std::u16string id_str = base::NumberToString16(item.command_id);
    if (id_str.find(query_lower) != std::u16string::npos) {
      score = 10.0;
    }
  }

  if (score < 0) {
    return -1.0;
  }

  // Apply category weight multiplier.
  double cat_weight = GetCategoryWeightInternal(item.category);
  score *= (1.0 + cat_weight);

  // Apply word boundary match bonus on title (if the query matches at
  // the start of words in the title, give a boost).
  if (score >= 0 && !query_lower.empty() &&
      IsWordBoundaryMatch(query, item.title)) {
    score += 100.0;
  }

  // Apply acronym match bonus on title.
  if (score >= 0 && !query_lower.empty() && query_lower.size() >= 2 &&
      IsAcronymMatch(query, item.title)) {
    score += 150.0;
  }

  // Apply recent command boost.
  if (item.is_recent) {
    score += 300.0;
  }

  // Apply usage count boost.
  score += item.use_count * 5.0;

  return score;
}

// =========================================================================
// Construction
// =========================================================================

AstraCommandPaletteModel::AstraCommandPaletteModel() {
  BuildCommandIndex();
  UpdateResults();
}

AstraCommandPaletteModel::~AstraCommandPaletteModel() {
  // Notify new-style observers of shutdown.
  for (auto& observer : observers_) {
    observer.OnCommandPaletteModelShutdown(this);
  }
}

// =========================================================================
// Command index building
// =========================================================================

void AstraCommandPaletteModel::BuildCommandIndex() {
  commands_.clear();

  // Add Chrome commands.
  BuildChromeCommands(commands_);

  // Add Astra commands.
  BuildAstraCommands(commands_);

  // Add dynamic workspace commands (if any workspaces exist).
  if (workspace_count_ > 0) {
    for (size_t i = 0;
         i < std::min(workspace_count_, kMaxWorkspaceCommands); ++i) {
      AstraCommandItem item;
      // Use a special ID range for dynamic workspace commands.
      // These aren't real command IDs — they are handled specially by
      // the view/bubble delegate.
      // TODO(astra): Use proper Astra command IDs for workspace switching
      // with a parameter, or create a command ID per workspace slot.
      // Chromium patch point: chrome/browser/ui/browser_command_controller.h
      item.command_id = kAstraCommandFirst + 1000 + static_cast<int>(i);
      item.title =
          u"Switch to Workspace " + base::NumberToString16(i + 1);
      item.description = u"Jump directly to workspace " +
                         base::NumberToString16(i + 1);
      item.shortcut_text =
          (i < 9) ? (u"⌃" + base::NumberToString16(i + 1)) : u"";
      item.type = AstraCommandType::kWorkspace;
      item.category = AstraCommandCategory::kWorkspaces;
      item.icon_name = "workspace_" + std::to_string(i + 1);
      item.workspace_id = "workspace_" + std::to_string(i + 1);
      item.is_astra = true;
      item.is_dynamic_workspace = true;
      commands_.push_back(std::move(item));
    }
  }

  // Apply use counts and recent status from tracking data.
  std::unordered_set<int> recent_set(recently_used_ids_.begin(),
                                      recently_used_ids_.end());
  for (auto& cmd : commands_) {
    cmd.is_recent = (recent_set.count(cmd.command_id) > 0);
  }
}

// static
void AstraCommandPaletteModel::BuildChromeCommands(
    std::vector<AstraCommandItem>& out) {
  for (const auto& entry : kChromeCommands) {
    AstraCommandItem item;
    item.command_id = entry.command_id;
    item.title = entry.title;
    item.description = entry.description;
    item.shortcut_text = entry.shortcut;
    item.type = entry.type;
    item.category = entry.category;
    item.icon_name = entry.icon_name;
    item.is_astra = false;
    item.is_dynamic_workspace = false;
    out.push_back(std::move(item));
  }
}

// static
void AstraCommandPaletteModel::BuildAstraCommands(
    std::vector<AstraCommandItem>& out) {
  for (const auto& entry : kAstraCommands) {
    AstraCommandItem item;
    item.command_id = entry.command_id;
    item.title = entry.title;
    item.description = entry.description;
    item.shortcut_text = entry.shortcut;
    item.type = entry.type;
    item.category = entry.category;
    item.icon_name = entry.icon_name;
    item.is_astra = true;
    item.is_dynamic_workspace = false;
    out.push_back(std::move(item));
  }
}

// =========================================================================
// Query / search
// =========================================================================

void AstraCommandPaletteModel::SetQuery(const std::u16string& query) {
  if (query_ == query) {
    return;
  }
  query_ = query;

  // Notify legacy observers of search text change.
  for (auto& observer : legacy_observers_) {
    observer.OnSearchTextChanged(query_);
  }

  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }

  // Auto-execute single result if enabled.
  if (auto_execute_single_result_ && results_.size() == 1) {
    ExecuteCommand(0);
  }
}

void AstraCommandPaletteModel::UpdateResults() {
  struct ScoredItem {
    const AstraCommandItem* item;
    double score;
    size_t original_index;
  };

  std::vector<ScoredItem> scored;
  scored.reserve(commands_.size());

  // Build a set of recently used command IDs for O(1) lookup.
  std::unordered_set<int> recent_set(recently_used_ids_.begin(),
                                      recently_used_ids_.end());

  for (size_t i = 0; i < commands_.size(); ++i) {
    const auto& cmd = commands_[i];

    // Skip commands that don't pass the category filter.
    if (!IsCategoryVisible(cmd.category)) {
      continue;
    }

    double score = ComputeRelevanceScore(query_, cmd);
    if (score >= 0) {
      scored.push_back({&cmd, score, i});
    }
  }

  // Sort by the chosen sort mode.
  if (sort_by_relevance_) {
    // Sort by relevance (highest first), stable by original index as tiebreaker.
    std::sort(scored.begin(), scored.end(),
              [](const ScoredItem& a, const ScoredItem& b) {
                if (a.score != b.score) {
                  return a.score > b.score;
                }
                return a.original_index < b.original_index;
              });
  } else {
    // Sort by usage count (most used first), then by relevance as tiebreaker.
    std::sort(scored.begin(), scored.end(),
              [](const ScoredItem& a, const ScoredItem& b) {
                if (a.item->use_count != b.item->use_count) {
                  return a.item->use_count > b.item->use_count;
                }
                if (a.score != b.score) {
                  return a.score > b.score;
                }
                return a.original_index < b.original_index;
              });
  }

  // Truncate to max visible results.
  size_t max_results = std::min(max_search_results_, kMaxResults);
  if (scored.size() > max_results) {
    scored.resize(max_results);
  }

  // Build the flat result list in score order.
  results_.clear();
  results_.reserve(scored.size());
  for (const auto& scored_item : scored) {
    AstraCommandItem item = *scored_item.item;
    item.relevance_score = scored_item.score;
    results_.push_back(std::move(item));
  }

  // Group results by category (preserving score order within groups).
  // Category order is determined by first appearance in the score-sorted
  // list, which means highest-scoring categories come first.
  BuildResultGroups();

  // Rebuild the flat list from groups so results_ is in group order.
  // This ensures selection indices match the visual order when section
  // headers are inserted by category.
  results_.clear();
  for (const auto& group : result_groups_) {
    for (const auto& item : group.items) {
      results_.push_back(item);
    }
  }

  // Update selection.
  if (results_.empty()) {
    selected_index_ = -1;
  } else if (selected_index_ < 0 ||
             selected_index_ >= static_cast<int>(results_.size())) {
    selected_index_ = 0;
  }
}

void AstraCommandPaletteModel::BuildResultGroups() {
  result_groups_.clear();

  if (results_.empty()) {
    return;
  }

  // Group results by category, preserving the order of items within each
  // category.  Categories appear in the order of their first item in the
  // sorted results list (i.e. highest-scoring category first).
  std::map<AstraCommandCategory, size_t> category_to_group_index;

  for (const auto& item : results_) {
    auto it = category_to_group_index.find(item.category);
    if (it == category_to_group_index.end()) {
      // New category — add a new group.
      result_groups_.push_back({item.category, {}});
      category_to_group_index[item.category] = result_groups_.size() - 1;
    }
    size_t group_idx = category_to_group_index[item.category];
    result_groups_[group_idx].items.push_back(item);
  }
}

double AstraCommandPaletteModel::GetCategoryWeight(
    AstraCommandCategory category) const {
  return GetCategoryWeightInternal(category);
}

// =========================================================================
// Full command index accessors
// =========================================================================

size_t AstraCommandPaletteModel::GetCommandCount() const {
  return commands_.size();
}

// =========================================================================
// Search API
// =========================================================================

std::vector<AstraCommandItem> AstraCommandPaletteModel::SearchCommands(
    const std::u16string& query) const {
  std::vector<AstraCommandItem> results;

  if (query.empty()) {
    return GetDefaultCommands();
  }

  std::vector<std::pair<double, const AstraCommandItem*>> scored;

  for (const auto& cmd : commands_) {
    if (!IsCategoryVisible(cmd.category)) {
      continue;
    }

    double score = ComputeRelevanceScore(query, cmd);
    if (score >= 0) {
      scored.push_back({score, &cmd});
    }
  }

  // Sort by score (highest first).
  if (sort_by_relevance_) {
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) {
                return a.first > b.first;
              });
  } else {
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) {
                if (a.second->use_count != b.second->use_count) {
                  return a.second->use_count > b.second->use_count;
                }
                return a.first > b.first;
              });
  }

  // Truncate.
  size_t max_results = std::min(max_search_results_, kMaxResults);
  if (scored.size() > max_results) {
    scored.resize(max_results);
  }

  // Build result list.
  for (const auto& pair : scored) {
    AstraCommandItem item = *pair.second;
    item.relevance_score = pair.first;
    results.push_back(std::move(item));
  }

  return results;
}

// =========================================================================
// Recent commands
// =========================================================================

std::vector<AstraCommandItem> AstraCommandPaletteModel::GetRecentCommands(
    int max_count) const {
  std::vector<AstraCommandItem> results;

  size_t max = (max_count > 0) ? static_cast<size_t>(max_count)
                               : max_recent_commands_;
  max = std::min(max, kMaxRecentlyUsed);
  max = std::min(max, recently_used_ids_.size());

  // Build a map from command_id to command for quick lookup.
  std::unordered_map<int, const AstraCommandItem*> id_to_cmd;
  for (const auto& cmd : commands_) {
    id_to_cmd[cmd.command_id] = &cmd;
  }

  for (size_t i = 0; i < max; ++i) {
    int cmd_id = recently_used_ids_[i];
    auto it = id_to_cmd.find(cmd_id);
    if (it != id_to_cmd.end()) {
      AstraCommandItem item = *it->second;
      item.is_recent = true;
      results.push_back(std::move(item));
    }
  }

  return results;
}

void AstraCommandPaletteModel::RecordCommandUse(int command_id) {
  // Find the command in the index.
  int idx = FindCommandIndex(command_id);
  if (idx >= 0) {
    commands_[idx].use_count++;
    commands_[idx].is_recent = true;
  }

  // Update recently used list.
  auto it = std::find(recently_used_ids_.begin(), recently_used_ids_.end(),
                       command_id);
  if (it != recently_used_ids_.end()) {
    recently_used_ids_.erase(it);
  }
  recently_used_ids_.insert(recently_used_ids_.begin(), command_id);

  // Trim to max size.
  if (recently_used_ids_.size() > kMaxRecentlyUsed) {
    recently_used_ids_.resize(kMaxRecentlyUsed);
  }

  // Re-rank results.
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

void AstraCommandPaletteModel::ClearRecentCommands() {
  if (recently_used_ids_.empty()) {
    return;
  }

  // Clear is_recent flag on all commands.
  for (auto& cmd : commands_) {
    cmd.is_recent = false;
  }

  recently_used_ids_.clear();

  // Re-rank results.
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Default commands
// =========================================================================

std::vector<AstraCommandItem> AstraCommandPaletteModel::GetDefaultCommands()
    const {
  std::vector<AstraCommandItem> results;

  // Start with recent commands.
  if (show_recent_section_) {
    auto recent = GetRecentCommands(static_cast<int>(max_recent_commands_));
    for (auto& cmd : recent) {
      results.push_back(std::move(cmd));
    }
  }

  // Add all other commands with base scores, sorted by category weight.
  std::unordered_set<int> seen_ids;
  for (const auto& cmd : results) {
    seen_ids.insert(cmd.command_id);
  }

  std::vector<AstraCommandItem> others;
  for (const auto& cmd : commands_) {
    if (seen_ids.count(cmd.command_id) > 0) {
      continue;
    }
    if (!IsCategoryVisible(cmd.category)) {
      continue;
    }
    others.push_back(cmd);
  }

  // Sort by category weight (highest first), then by use count,
  // then by index.
  std::sort(others.begin(), others.end(),
            [this](const AstraCommandItem& a, const AstraCommandItem& b) {
              double weight_a = GetCategoryWeightInternal(a.category);
              double weight_b = GetCategoryWeightInternal(b.category);
              if (weight_a != weight_b) {
                return weight_a > weight_b;
              }
              if (a.use_count != b.use_count) {
                return a.use_count > b.use_count;
              }
              return false;
            });

  // Cap total results.
  size_t max_results = std::min(max_search_results_, kMaxResults);
  for (const auto& cmd : others) {
    if (results.size() >= max_results) {
      break;
    }
    results.push_back(cmd);
  }

  return results;
}

// =========================================================================
// Command access
// =========================================================================

const AstraCommandItem* AstraCommandPaletteModel::GetCommandAt(int index) const {
  if (index < 0 || index >= static_cast<int>(results_.size())) {
    return nullptr;
  }
  return &results_[index];
}

const AstraCommandItem* AstraCommandPaletteModel::GetSelectedItem() const {
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(results_.size())) {
    return nullptr;
  }
  return &results_[selected_index_];
}

// =========================================================================
// Commands by type
// =========================================================================

std::vector<AstraCommandItem> AstraCommandPaletteModel::GetCommandsByType(
    AstraCommandType type) const {
  std::vector<AstraCommandItem> results;
  for (const auto& cmd : commands_) {
    if (cmd.type == type) {
      results.push_back(cmd);
    }
  }
  return results;
}

// =========================================================================
// Dynamic commands
// =========================================================================

void AstraCommandPaletteModel::AddCommand(const AstraCommandItem& command) {
  // Check for duplicate — if the command ID already exists, update it.
  int idx = FindCommandIndex(command.command_id);
  if (idx >= 0) {
    commands_[idx] = command;
  } else {
    commands_.push_back(command);
  }

  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

bool AstraCommandPaletteModel::RemoveCommand(int command_id) {
  int idx = FindCommandIndex(command_id);
  if (idx < 0) {
    return false;
  }

  commands_.erase(commands_.begin() + idx);

  // Also remove from recently used.
  auto it = std::find(recently_used_ids_.begin(), recently_used_ids_.end(),
                       command_id);
  if (it != recently_used_ids_.end()) {
    recently_used_ids_.erase(it);
  }

  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }

  return true;
}

// =========================================================================
// Ranking
// =========================================================================

void AstraCommandPaletteModel::UpdateRanking() {
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Selection
// =========================================================================

void AstraCommandPaletteModel::SetSelectedIndex(int index) {
  int old_index = selected_index_;

  if (results_.empty()) {
    selected_index_ = -1;
  } else {
    selected_index_ =
        std::max(0, std::min(static_cast<int>(results_.size()) - 1, index));
  }

  if (selected_index_ != old_index) {
    for (auto& observer : legacy_observers_) {
      observer.OnSelectionChanged();
    }
  }
}

void AstraCommandPaletteModel::MoveSelection(int delta) {
  if (results_.empty()) {
    return;
  }

  int count = static_cast<int>(results_.size());
  int new_index = selected_index_ + delta;

  // Wrap around at edges (like most command palettes do).
  if (new_index < 0) {
    new_index = count - 1;
  } else if (new_index >= count) {
    new_index = 0;
  }

  SetSelectedIndex(new_index);
}

void AstraCommandPaletteModel::SelectNextGroup() {
  if (result_groups_.size() <= 1) {
    return;
  }

  int current_group = FindGroupIndexForResult(selected_index_);
  int next_group = current_group + 1;
  if (next_group >= static_cast<int>(result_groups_.size())) {
    next_group = 0;  // Wrap around.
  }

  int first_in_group = GetFirstResultInGroup(next_group);
  if (first_in_group >= 0) {
    SetSelectedIndex(first_in_group);
  }
}

void AstraCommandPaletteModel::SelectPrevGroup() {
  if (result_groups_.size() <= 1) {
    return;
  }

  int current_group = FindGroupIndexForResult(selected_index_);
  int prev_group = current_group - 1;
  if (prev_group < 0) {
    prev_group = static_cast<int>(result_groups_.size()) - 1;  // Wrap around.
  }

  int first_in_group = GetFirstResultInGroup(prev_group);
  if (first_in_group >= 0) {
    SetSelectedIndex(first_in_group);
  }
}

// =========================================================================
// Command execution
// =========================================================================

void AstraCommandPaletteModel::ExecuteCommand(int index) {
  if (index < 0 || index >= static_cast<int>(results_.size())) {
    return;
  }

  const auto& item = results_[index];

  // Record usage.
  RecordCommandUse(item.command_id);

  // Notify new-style observers that a command was executed.
  for (auto& observer : observers_) {
    observer.OnCommandExecuted(this, item.command_id);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnCommandExecutionRequested(item.command_id, item.is_astra);
  }
}

// =========================================================================
// Workspace commands
// =========================================================================

void AstraCommandPaletteModel::UpdateWorkspaceCommands(size_t workspace_count) {
  if (workspace_count_ == workspace_count) {
    return;
  }
  workspace_count_ = workspace_count;
  BuildCommandIndex();
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Observers — new style
// =========================================================================

void AstraCommandPaletteModel::AddObserver(
    AstraCommandPaletteObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraCommandPaletteModel::RemoveObserver(
    AstraCommandPaletteObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Observers — legacy
// =========================================================================

void AstraCommandPaletteModel::AddObserver(
    AstraCommandPaletteModelObserver* observer) {
  legacy_observers_.AddObserver(observer);
}

void AstraCommandPaletteModel::RemoveObserver(
    AstraCommandPaletteModelObserver* observer) {
  legacy_observers_.RemoveObserver(observer);
}

// =========================================================================
// Helpers
// =========================================================================

int AstraCommandPaletteModel::FindCommandIndex(int command_id) const {
  for (size_t i = 0; i < commands_.size(); ++i) {
    if (commands_[i].command_id == command_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int AstraCommandPaletteModel::FindRecentIndex(int command_id) const {
  for (size_t i = 0; i < recently_used_ids_.size(); ++i) {
    if (recently_used_ids_[i] == command_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int AstraCommandPaletteModel::FindGroupIndexForResult(int result_index) const {
  if (result_index < 0 || result_index >= static_cast<int>(results_.size())) {
    return -1;
  }

  int flat_index = 0;
  for (size_t g = 0; g < result_groups_.size(); ++g) {
    size_t group_size = result_groups_[g].items.size();
    if (result_index < flat_index + static_cast<int>(group_size)) {
      return static_cast<int>(g);
    }
    flat_index += static_cast<int>(group_size);
  }
  return -1;
}

int AstraCommandPaletteModel::GetFirstResultInGroup(int group_index) const {
  if (group_index < 0 ||
      group_index >= static_cast<int>(result_groups_.size())) {
    return -1;
  }
  if (result_groups_[group_index].items.empty()) {
    return -1;
  }

  int flat_index = 0;
  for (int g = 0; g < group_index; ++g) {
    flat_index += static_cast<int>(result_groups_[g].items.size());
  }
  return flat_index;
}

// =========================================================================
// Category filter
// =========================================================================

void AstraCommandPaletteModel::SetCategoryFilter(
    const std::set<AstraCommandCategory>& categories) {
  if (category_filter_ == categories) {
    return;
  }
  category_filter_ = categories;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

bool AstraCommandPaletteModel::IsCategoryVisible(
    AstraCommandCategory category) const {
  if (category_filter_.empty()) {
    return true;  // Empty filter = show all.
  }
  return category_filter_.count(category) > 0;
}

void AstraCommandPaletteModel::ClearCategoryFilter() {
  if (category_filter_.empty()) {
    return;
  }
  category_filter_.clear();
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — max_search_results
// =========================================================================

void AstraCommandPaletteModel::set_max_search_results(size_t max) {
  if (max_search_results_ == max) {
    return;
  }
  max_search_results_ = max;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — max_recent_commands
// =========================================================================

void AstraCommandPaletteModel::set_max_recent_commands(size_t max) {
  if (max_recent_commands_ == max) {
    return;
  }
  max_recent_commands_ = max;

  // No immediate results change needed — affects GetRecentCommands only.
  // But it may affect default commands display.
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — show_descriptions
// =========================================================================

void AstraCommandPaletteModel::set_show_descriptions(bool show) {
  if (show_descriptions_ == show) {
    return;
  }
  show_descriptions_ = show;
  // Presentation-only change — notify model change so views can update.

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — show_shortcuts
// =========================================================================

void AstraCommandPaletteModel::set_show_shortcuts(bool show) {
  if (show_shortcuts_ == show) {
    return;
  }
  show_shortcuts_ = show;
  // Presentation-only change.

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnCommandListChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — enable_fuzzy_search
// =========================================================================

void AstraCommandPaletteModel::set_enable_fuzzy_search(bool enable) {
  if (enable_fuzzy_search_ == enable) {
    return;
  }
  enable_fuzzy_search_ = enable;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — search_in_command_ids
// =========================================================================

void AstraCommandPaletteModel::set_search_in_command_ids(bool enable) {
  if (search_in_command_ids_ == enable) {
    return;
  }
  search_in_command_ids_ = enable;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — sort_by_relevance
// =========================================================================

void AstraCommandPaletteModel::set_sort_by_relevance(bool sort_by_relevance) {
  if (sort_by_relevance_ == sort_by_relevance) {
    return;
  }
  sort_by_relevance_ = sort_by_relevance;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Settings — auto_execute_single_result
// =========================================================================

void AstraCommandPaletteModel::set_auto_execute_single_result(
    bool auto_execute) {
  auto_execute_single_result_ = auto_execute;
  // No immediate effect — affects SetQuery behavior.
}

// =========================================================================
// Legacy settings (aliases)
// =========================================================================

void AstraCommandPaletteModel::set_max_visible_commands(size_t max) {
  set_max_search_results(max);
}

void AstraCommandPaletteModel::set_show_recent_section(bool show) {
  if (show_recent_section_ == show) {
    return;
  }
  show_recent_section_ = show;
  UpdateResults();

  // Notify new-style observers.
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }

  // Notify legacy observers.
  for (auto& observer : legacy_observers_) {
    observer.OnModelChanged();
  }
}

// =========================================================================
// Bulk operations
// =========================================================================

size_t AstraCommandPaletteModel::ExecuteAllVisible() {
  size_t count = results_.size();
  for (size_t i = 0; i < count; ++i) {
    const auto& item = results_[i];
    RecordCommandUse(item.command_id);
    for (auto& observer : legacy_observers_) {
      observer.OnCommandExecutionRequested(item.command_id, item.is_astra);
    }
    for (auto& observer : observers_) {
      observer.OnCommandExecuted(this, item.command_id);
    }
  }
  return count;
}

size_t AstraCommandPaletteModel::ExecuteFirstN(size_t count) {
  size_t actual = std::min(count, results_.size());
  for (size_t i = 0; i < actual; ++i) {
    const auto& item = results_[i];
    RecordCommandUse(item.command_id);
    for (auto& observer : legacy_observers_) {
      observer.OnCommandExecutionRequested(item.command_id, item.is_astra);
    }
    for (auto& observer : observers_) {
      observer.OnCommandExecuted(this, item.command_id);
    }
  }
  return actual;
}

// =========================================================================
// Palette lifecycle notifications
// =========================================================================

void AstraCommandPaletteModel::NotifyPaletteOpened() {
  for (auto& observer : legacy_observers_) {
    observer.OnPaletteOpened();
  }
}

void AstraCommandPaletteModel::NotifyPaletteClosed() {
  for (auto& observer : legacy_observers_) {
    observer.OnPaletteClosed();
  }
}

void AstraCommandPaletteModel::NotifyCommandExecuted(int command_id,
                                                      bool is_astra) {
  for (auto& observer : legacy_observers_) {
    observer.OnCommandExecuted(command_id, is_astra);
  }
}

// =========================================================================
// Persistence via PrefService
// =========================================================================
//
// Presentation settings and recent command history persist through
// PrefService so they survive browser restarts.
//
// Chromium subsystem: PrefService (components/prefs/pref_service.h)
// Astra owns: command palette presentation prefs, recent command list.
// =========================================================================

void AstraCommandPaletteModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  // Load presentation settings.
  int max_visible = prefs->GetInteger(prefs::kPrefCommandPaletteMaxVisible);
  if (max_visible > 0) {
    max_search_results_ = static_cast<size_t>(max_visible);
  }

  show_descriptions_ = prefs->GetBoolean(prefs::kPrefCommandPaletteShowDescriptions);
  show_shortcuts_ = prefs->GetBoolean(prefs::kPrefCommandPaletteShowShortcuts);
  show_recent_section_ =
      prefs->GetBoolean(prefs::kPrefCommandPaletteShowRecentSection);

  // Load recent commands from the command delegate's recent list pref.
  const base::Value::List& recent_list =
      prefs->GetList(prefs::kPrefCommandRecentList);
  recently_used_ids_.clear();
  for (const auto& val : recent_list) {
    if (val.is_int()) {
      recently_used_ids_.push_back(val.GetInt());
    }
  }
  // Cap at kMaxRecentlyUsed.
  if (recently_used_ids_.size() > kMaxRecentlyUsed) {
    recently_used_ids_.resize(kMaxRecentlyUsed);
  }

  // Update is_recent flags on commands.
  std::unordered_set<int> recent_set(recently_used_ids_.begin(),
                                      recently_used_ids_.end());
  for (auto& cmd : commands_) {
    cmd.is_recent = (recent_set.count(cmd.command_id) > 0);
  }

  // Recompute results with loaded settings.
  UpdateResults();
}

void AstraCommandPaletteModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  // Save presentation settings.
  prefs->SetInteger(prefs::kPrefCommandPaletteMaxVisible,
                    static_cast<int>(max_search_results_));
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowDescriptions,
                    show_descriptions_);
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowShortcuts, show_shortcuts_);
  prefs->SetBoolean(prefs::kPrefCommandPaletteShowRecentSection,
                    show_recent_section_);

  // Save recent commands to the command delegate's recent list pref.
  base::Value::List recent_list;
  for (int cmd_id : recently_used_ids_) {
    recent_list.Append(cmd_id);
  }
  prefs->SetList(prefs::kPrefCommandRecentList, std::move(recent_list));
}

}  // namespace astra
