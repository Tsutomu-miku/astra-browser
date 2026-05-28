# Requirements

## Product Intent

Astra Browser should feel like a Chromium-based browser shaped by Zen and Arc interaction patterns: vertical organization, low-friction Spaces, compact chrome, strong keyboard access, rich split-view workflows, and fast preview/navigation surfaces. Electron is acceptable for the current stage because it embeds Chromium and lets the product shell move quickly.

## Documentation Rules

- Keep README focused on orientation, setup, scripts, packaging, and links.
- Keep product requirements in this file.
- Keep engineering direction and structure in `docs/PROJECT_SPEC.md`.
- Keep runtime architecture details in `docs/ARCHITECTURE.md`.
- When a new user-facing feature lands, update the relevant requirement here instead of expanding README.

## Current Scope

### Browser Shell And Spaces

- The app must provide a Chromium-backed desktop browser shell.
- The app must organize browsing around vertical Spaces.
- Users must be able to create, delete, rename, recolor, and reorder Spaces.
- The workspace strip must expose a direct new-Space action without opening Settings.
- The workspace strip must expose direct Space management for switching, renaming, recoloring, and deleting from a Space context menu.
- The topbar Space pill must expose direct Space settings, creation, and deletion from a context menu.
- The workspace strip must show per-Space tab counts for quick scanning.
- The workspace strip must support wheel cycling between Spaces.
- Each Space must have a stable persistent Chromium profile partition.
- Each Space must support its own homepage.
- Startup behavior must support restoring the previous session or opening configured Space homepages.
- Browser state must persist locally in `localStorage`.
- Browser state must be exportable and importable as a JSON backup.

### Tabs And Sidebar

- Each Space must support multiple tabs.
- The current Space's tabs must keep their Chromium webviews mounted so tab switching preserves page state.
- Background tabs must be able to sleep, unloading hidden webviews to reduce memory use.
- Memory Saver must be able to automatically sleep idle background tabs while protecting active, split-view, and pinned tabs.
- Settings must expose a Memory Saver summary and a direct sleep-inactive-tabs action.
- The sidebar footer must expose a compact Memory Saver status and direct sleep action.
- Sleeping tabs must expose a direct wake action from the sidebar tab context menu.
- Active tabs, split-view tabs, and pinned tabs must be protected from bulk sleep.
- Users must be able to close tabs directly from the sidebar with recently closed recovery.
- The sidebar must expose a compact recently closed section for restoring the active Space's latest closed tabs.
- Sidebar recently closed rows must support Alt-click Glance preview and Shift-click split opening.
- Sidebar recently closed rows must support context menus for restore, Glance preview, split opening, and copying page details.
- Users must be able to duplicate the active tab.
- Users must be able to reorder sidebar tabs by drag and drop.
- Users must be able to move tabs between Spaces from the command palette or by dragging onto Space buttons.
- Users must be able to drag sidebar tabs onto the New Space button to create a Space from that tab.
- Users must be able to move tabs between Spaces from the sidebar tab context menu.
- Sidebar tab context menus must support opening, split-view targeting, duplication, pinning, muting, and closing.
- Sidebar tab context menus must support copying the tab URL and title without selecting the tab.
- Sidebar tab context menus must support adding or removing a tab from Space favorites and global Essentials.
- Sidebar tab rows and pinned tab buttons must support middle-click closing without selecting the tab first.
- Focused sidebar tab rows and pinned tab buttons must support Delete and Backspace to close without selecting the tab first.
- Sidebar tab rows must expose compact visual status badges for split-view, muted, and sleeping states.
- Pinned tab buttons must expose compact visual status badges for split-view, muted, and sleeping states.
- Pinned tabs must participate in sidebar drag-and-drop reordering.
- Users must be able to close other tabs, close tabs to the left, and close tabs to the right from command palette and sidebar tab context menu flows.
- Users must be able to cycle tabs with keyboard shortcuts or the command palette.
- Users must be able to sleep the current tab from the command palette when another tab can receive focus.

### Groups, Pins, Essentials, And Favorites

- Users must be able to create named, color-coded, collapsible tab groups.
- Sidebar tab group headers must support context menus for collapsing, sleeping, duplicating, renaming, recoloring, moving to another Space, closing, and ungrouping the whole group.
- Users must be able to drag sidebar tab groups onto Space buttons to move the whole group between Spaces.
- Users must be able to drag sidebar tab groups onto the New Space button to create a Space from that group.
- Sidebar tab context menus must support creating a new group from a tab, moving tabs to existing groups, and ungrouping grouped tabs.
- Users must be able to drag a regular sidebar tab onto a New Group target to create a tab group.
- Users must be able to drag a grouped sidebar tab onto an Ungroup target to remove it from its group.
- Users must be able to pin tabs per Space.
- Users must be able to drag sidebar tabs into Pinned to pin them in the active Space.
- Users must have global Essentials visible across Spaces for core pages.
- Clicking a global Essential from the sidebar must navigate the current tab instead of creating a new tab.
- Users must be able to drag sidebar tabs into Essentials to save them as global quick entries.
- Users must be able to reorder global Essentials directly from the sidebar with drag and drop.
- Users must have Space-local favorites for quick access.
- Clicking a Space favorite from the sidebar must navigate the current tab instead of creating a new tab.
- Users must be able to drag sidebar tabs into Space favorites to save them for quick access.
- Users must be able to reorder Space favorites directly from the sidebar with drag and drop.
- Sidebar Essentials and favorites must support context menus for opening, Glance preview, split opening, and removing the entry.
- Opening an Essential or favorite from its sidebar context menu must navigate the current tab instead of creating a new tab.
- Sidebar Essentials and favorites context menus must support copying the entry URL and title.
- Sidebar Essentials, pinned tabs, favorites, and tabs must be visually separated into scannable sections.
- Sidebar sections must be individually collapsible, while filtering must reveal matching contents even when a section was collapsed.
- Sidebar drag targets must temporarily reveal collapsed Essentials, Pinned, Favorites, and Tabs sections when dragging relevant tabs or quick entries.
- Sidebar quick entries must show active-page state when the current tab matches an Essential or favorite.
- Users must be able to toggle the current page as a favorite with `Ctrl+D`, `Cmd+D`, `Ctrl+Shift+D`, or `Cmd+Shift+D`.
- Sidebar search must filter global Essentials plus tabs, tab groups, pinned tabs, and favorites inside the active Space.
- Sidebar search must navigate Essential and favorite matches in the current tab unless preview or split modifiers are used.
- Sidebar search must support Arrow, Home, End, Enter, Alt+Enter preview, and Shift+Enter split-open flows.
- Sidebar search must visually hint Alt preview and Shift split actions while filtering.
- Sidebar items must support Alt-click Glance preview and Shift-click split opening.
- Sidebar tab and favorite rows must reveal Alt preview and Shift split hints on hover or keyboard focus.
- Pinned tab buttons must reveal Alt preview and Shift split hints on hover or keyboard focus.

### Navigation, Address Bar, And Omnibox

- The address bar must support direct URLs and search fallback.
- The address bar must be focusable with `Ctrl+L`, `Cmd+L`, `Ctrl+J`, `Cmd+J`, and `Alt+D`.
- The address identity control must expose a context menu for site information, copying page details, Glance preview, and split opening.
- Address bar suggestions must include open tabs, Essentials, workspace favorites, and browsing history.
- Address bar suggestions must support Arrow, Home, End, Enter, and Alt+Enter split opening.
- Address bar suggestions must visually hint Alt split opening where suggestions are shown.
- Compact mode must expose a sidebar address field with the same suggestion and keyboard behavior as the top bar.
- Back and forward availability must be tracked per tab from Chromium webview navigation state.
- Back, forward, reload, stop-loading, and hard reload must be available through controls and keyboard shortcuts where appropriate.
- Navigation shortcuts must include `Alt+Left`, `Alt+Right`, `Alt+Home`, `Ctrl+[`, `Cmd+[`, `Ctrl+]`, `Cmd+]`, `Ctrl+R`, `Cmd+R`, `Ctrl+Shift+R`, and `Cmd+Shift+R`.

### Keyboard Shortcuts

- The browser must support keyboard-first tab, Space, command, split, navigation, zoom, find, history, downloads, favorite, mute, and address workflows.
- Direct tab selection must support Zen-style `Alt+1` through `Alt+8`, ordered by Essentials, pinned tabs, then regular tabs.
- Last-tab selection must support `Alt+9`.
- Space cycling must support `Ctrl+Alt+Q` and `Ctrl+Alt+E`.
- Split layout shortcuts must support horizontal, vertical, grid, and unsplit-all flows.
- The command palette must open by keyboard.
- Find next and previous must be available with `Ctrl+G`, `Cmd+G`, `Ctrl+Shift+G`, and `Cmd+Shift+G`.
- Recently closed tab restore must be available by keyboard.
- Current-tab mute must be available with `Ctrl+M` or `Cmd+M`.
- History must be available with `Ctrl+H` or `Cmd+H`.
- Downloads must be available with `Ctrl+Shift+Y` or `Cmd+Shift+Y`.
- Diagnostics shortcuts must include `F12` and `Ctrl+Shift+I` for application DevTools in development and packaged builds.
- Application DevTools must also be available from the command palette for keyboard-first diagnostics.

### Compact Mode And Chrome

- The app must support collapsible sidebar focus mode.
- Sidebar width must be toggleable with `Alt+B`.
- The app must support Zen-style compact mode that hides browser chrome until hover or focus.
- Collapsed and compact sidebars must reveal their tab controls on hover or keyboard focus.
- Compact mode must be toggleable with `Ctrl+Alt+C` or `Cmd+Alt+C`.
- Compact mode must briefly peek browser chrome after tab changes.
- Compact mode must expose a top-edge peek target for temporarily showing the toolbar.
- Compact mode must expose command palette actions for temporarily peeking the floating toolbar and sidebar.
- Compact mode must expose a visible control for pinning or unpinning the floating toolbar.
- Compact mode must expose a visible control for pinning or unpinning the floating sidebar.
- Compact mode must support persistent floating sidebar and floating toolbar shortcuts.
- Compact mode must leave content as the primary visual surface.

### Split View And Glance

- Split view must support up to four visible webviews.
- Users must be able to drag tabs to split view from the sidebar.
- The sidebar Split control must highlight as a drop target when a tab can be sent to split view.
- Users must be able to send explicit tabs or URLs into split view.
- Users must be able to make a secondary split pane active without closing split view.
- Horizontal and vertical two-pane split views must be resizable by pointer and keyboard.
- Split layout mode must support horizontal, vertical, and grid layouts.
- Split layout switching must be directly available from the sidebar while split view is active.
- Split pane overlay controls must show the pane identity, active pane state, and active split layout.
- Glance must preview tabs, favorites, and history without replacing the active tab.
- Glance preview navigation must support open, split, backdrop-close, and Escape-close actions.
- Glance preview actions must support copying the current preview URL without opening it as a tab.
- Glance previews must support in-preview back, forward, reload, and stop-loading controls.

### Command Palette

- The command palette must support workspace switching, tab cycling, tab grouping, duplicate tabs, sleeping tabs, mute controls, tab cleanup actions, cross-workspace tab moves, tab actions, data clearing, split view, and history search.
- The command palette must summarize Memory Saver state for sleep-inactive-tabs actions.
- The command palette must expose state-aware Memory Saver auto-sleep toggles and delay presets.
- The command palette must expose page tools such as find in page and site information.
- The command palette must expose page navigation tools for back, forward, reload, stop-loading, and hard reload.
- The command palette must expose split-pane focus commands while split view is active.
- The command palette must include entries for open tabs, Essentials, favorites, history, and recently closed tabs.
- Recently closed command palette entries must support direct restore, Glance preview, and split opening.
- The command palette must identify active and sleeping open-tab entries so keyboard navigation has clear tab state.
- The command palette must directly open URLs or search typed queries.
- The command palette must preview typed URLs or searches in Glance with `Alt+Enter`.
- Exact command-name matches must execute ahead of search fallback.
- Command palette selection must support Arrow, Home, End, Enter, Alt+Enter preview, Shift+Enter split opening, and Escape.
- Command palette entries must visually hint when Alt preview or Shift split actions are available.
- Command palette entries must expose compact visual type badges for fast keyboard scanning.
- Command palette chrome commands must use state-aware labels for sidebar, compact mode, and pinned floating chrome actions.
- Command palette page commands must support copying the current URL and current page title.
- Command palette entries for keyboard-accessible browser actions must show their shortcut hints.

### New Tab And Start Surface

- The internal new tab page must be rendered by the React shell instead of a Chromium webview.
- The new tab page must support Space-aware search.
- The new tab page must show global Essentials plus favorites and recent history for the active Space.
- The new tab search must use the shared omnibox suggestion model so Essentials, favorites, open tabs, and history can be opened directly from the start surface.
- The new tab search must visually hint Alt split opening for suggestions.
- New tab quick entries must visually hint Alt preview and Shift split actions on hover or keyboard focus.
- New tab Essentials, favorites, and recent history entries must support Alt-click Glance preview and Shift-click split opening.
- New tab Essentials and favorites must support context menus for opening, Glance preview, split opening, and removing the entry.
- New tab recent history entries must support context menus for opening, Glance preview, split opening, and removing the entry.

### Site Identity, Permissions, And Data

- The address area must show loading, host, and basic security state.
- The site information panel must expose per-origin permission controls.
- The site information panel must summarize connection security, active Space profile scope, and custom permission decisions.
- The site information panel must support copying the current site origin when an origin is available.
- The site information panel must reset all custom permissions for the current origin and active Space profile.
- Permission decisions must be scoped to the current workspace Chromium profile.
- Chromium permission prompts must support camera, microphone, location, notifications, and similar web capabilities.
- Settings must inspect Chromium profile storage usage per Space.
- Settings must clear Chromium cache/storage and renderer-owned permissions/history for a single Space profile.
- Settings and command palette flows must clear browsing data across workspace Chromium profiles.

### History, Downloads, Find, And Zoom

- Find in page must use Chromium webview search and support next/previous match shortcuts.
- Find in page must show current match counts and no-match feedback from Chromium search results.
- Per-tab zoom controls must use Chromium webview zoom.
- Browsing history must be searchable.
- History entries must support single-entry removal and full clearing.
- History entries must support Alt-click Glance preview, Shift-click split opening, and context menus for open, preview, split, and removal.
- History must include per-workspace recently closed tab restore.
- Chromium downloads must be tracked in a downloads drawer that can be opened with `Ctrl+Shift+Y` or `Cmd+Shift+Y`.
- Completed downloads must support opening the file and revealing it in the file manager from the downloads drawer.
- Download entries must support context menus for opening the file, revealing the file, and copying the file path.

### Settings

- Settings must support global homepage, global search engine, workspace homepage, workspace name, workspace accent, and profile display name.
- Settings must support startup restore/homepage behavior.
- Settings must support state backup import/export.
- Settings must support browsing-data and profile cleanup controls.
- Settings must group global, Space, data, and workspace management controls behind clear section navigation.

### Packaging And Release

- The app must package with Electron Builder.
- Windows packaging must produce portable executables.
- macOS packaging must produce DMG and ZIP artifacts.
- Linux packaging must produce AppImage, DEB, and tar.gz artifacts.
- Local package scripts must clean stale release output before building new artifacts.
- Default macOS packaging must build the current architecture, while release packaging must build x64 and arm64 artifacts as separate jobs and upload assets by architecture and package type.
- Local all-architecture macOS packaging must run x64 and arm64 as separate Electron Builder invocations instead of producing a universal package.
- Full local multi-platform packaging must also invoke macOS x64 and arm64 package builds separately so ZIP and DMG outputs stay architecture-specific.
- Package scripts must remove unpacked Electron Builder staging directories from `release/` after distributable artifacts are written.
- Application package metadata must include author email and Linux maintainer information for DEB builds.
- GitHub Releases must publish all platform artifacts for version-matching tags.

## Quality Requirements

- `pnpm check` must pass before a change is considered healthy.
- New browser-product rules should be testable without launching Electron whenever possible.
- Renderer UI should be grouped by product surface.
- Domain modules must remain independent of React, Electron, DOM APIs, and localStorage.
- Store actions should delegate browser rules to reusable domain actions.
- Shortcut parsing should remain tested as a pure function.
- Webview methods must only be used after the webview lifecycle owner has registered a ready element.
- Unexpected lifecycle/data errors should surface during development instead of being hidden by broad compatibility catch blocks.

## Non-Goals For The Current Stage

- Native Chromium source integration is not required while Electron is sufficient for product iteration.
- Browser extension marketplace compatibility is not required yet.
- Account sync is not required yet.
- Mobile browser support is not required yet.
