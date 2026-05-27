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
- Active tabs, split-view tabs, and pinned tabs must be protected from bulk sleep.
- Users must be able to close tabs directly from the sidebar with recently closed recovery.
- Users must be able to duplicate the active tab.
- Users must be able to reorder sidebar tabs by drag and drop.
- Users must be able to move tabs between Spaces from the command palette or by dragging onto Space buttons.
- Users must be able to move tabs between Spaces from the sidebar tab context menu.
- Sidebar tab context menus must support opening, split-view targeting, duplication, pinning, muting, and closing.
- Sidebar tab context menus must support adding or removing a tab from Space favorites and global Essentials.
- Sidebar tab rows must expose compact visual status badges for split-view, muted, and sleeping states.
- Users must be able to close other tabs, close tabs to the left, and close tabs to the right from command palette and sidebar tab context menu flows.
- Users must be able to cycle tabs with keyboard shortcuts or the command palette.

### Groups, Pins, Essentials, And Favorites

- Users must be able to create named, color-coded, collapsible tab groups.
- Sidebar tab context menus must support creating a new group from a tab, moving tabs to existing groups, and ungrouping grouped tabs.
- Users must be able to pin tabs per Space.
- Users must have global Essentials visible across Spaces for core pages.
- Users must have Space-local favorites for quick access.
- Sidebar Essentials, pinned tabs, favorites, and tabs must be visually separated into scannable sections.
- Sidebar quick entries must show active-page state when the current tab matches an Essential or favorite.
- Users must be able to toggle the current page as a favorite with `Ctrl+D`, `Cmd+D`, `Ctrl+Shift+D`, or `Cmd+Shift+D`.
- Sidebar search must filter global Essentials plus tabs, tab groups, pinned tabs, and favorites inside the active Space.
- Sidebar search must support Arrow, Home, End, Enter, Alt+Enter preview, and Shift+Enter split-open flows.
- Sidebar search must visually hint Alt preview and Shift split actions while filtering.
- Sidebar items must support Alt-click Glance preview and Shift-click split opening.

### Navigation, Address Bar, And Omnibox

- The address bar must support direct URLs and search fallback.
- The address bar must be focusable with `Ctrl+L`, `Cmd+L`, `Ctrl+J`, `Cmd+J`, and `Alt+D`.
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

### Compact Mode And Chrome

- The app must support collapsible sidebar focus mode.
- Sidebar width must be toggleable with `Alt+B`.
- The app must support Zen-style compact mode that hides browser chrome until hover or focus.
- Compact mode must be toggleable with `Ctrl+Alt+C` or `Cmd+Alt+C`.
- Compact mode must briefly peek browser chrome after tab changes.
- Compact mode must expose a top-edge peek target for temporarily showing the toolbar.
- Compact mode must support persistent floating sidebar and floating toolbar shortcuts.
- Compact mode must leave content as the primary visual surface.

### Split View And Glance

- Split view must support up to four visible webviews.
- Users must be able to drag tabs to split view from the sidebar.
- Users must be able to send explicit tabs or URLs into split view.
- Users must be able to make a secondary split pane active without closing split view.
- Horizontal and vertical two-pane split views must be resizable by pointer and keyboard.
- Split layout mode must support horizontal, vertical, and grid layouts.
- Glance must preview tabs, favorites, and history without replacing the active tab.
- Glance preview navigation must support open, split, backdrop-close, and Escape-close actions.
- Glance previews must support in-preview back, forward, and reload controls.

### Command Palette

- The command palette must support workspace switching, tab cycling, tab grouping, duplicate tabs, sleeping tabs, mute controls, tab cleanup actions, cross-workspace tab moves, tab actions, data clearing, split view, and history search.
- The command palette must include entries for open tabs, Essentials, favorites, history, and recently closed tabs.
- The command palette must directly open URLs or search typed queries.
- Exact command-name matches must execute ahead of search fallback.
- Command palette selection must support Arrow, Home, End, Enter, Alt+Enter preview, Shift+Enter split opening, and Escape.
- Command palette entries must visually hint when Alt preview or Shift split actions are available.

### New Tab And Start Surface

- The internal new tab page must be rendered by the React shell instead of a Chromium webview.
- The new tab page must support Space-aware search.
- The new tab page must show global Essentials plus favorites and recent history for the active Space.
- The new tab search must use the shared omnibox suggestion model so Essentials, favorites, open tabs, and history can be opened directly from the start surface.
- The new tab search must visually hint Alt split opening for suggestions.
- New tab quick entries must visually hint Alt preview and Shift split actions on hover or keyboard focus.
- New tab Essentials, favorites, and recent history entries must support Alt-click Glance preview and Shift-click split opening.

### Site Identity, Permissions, And Data

- The address area must show loading, host, and basic security state.
- The site information panel must expose per-origin permission controls.
- The site information panel must summarize connection security, active Space profile scope, and custom permission decisions.
- The site information panel must reset all custom permissions for the current origin and active Space profile.
- Permission decisions must be scoped to the current workspace Chromium profile.
- Chromium permission prompts must support camera, microphone, location, notifications, and similar web capabilities.
- Settings must inspect Chromium profile storage usage per Space.
- Settings must clear Chromium cache/storage and renderer-owned permissions/history for a single Space profile.
- Settings and command palette flows must clear browsing data across workspace Chromium profiles.

### History, Downloads, Find, And Zoom

- Find in page must use Chromium webview search and support next/previous match shortcuts.
- Per-tab zoom controls must use Chromium webview zoom.
- Browsing history must be searchable.
- History entries must support single-entry removal and full clearing.
- History must include per-workspace recently closed tab restore.
- Chromium downloads must be tracked in a downloads drawer that can be opened with `Ctrl+Shift+Y` or `Cmd+Shift+Y`.
- Completed downloads must support opening the file and revealing it in the file manager from the downloads drawer.

### Settings

- Settings must support global homepage, global search engine, workspace homepage, workspace name, workspace accent, and profile display name.
- Settings must support startup restore/homepage behavior.
- Settings must support state backup import/export.
- Settings must support browsing-data and profile cleanup controls.

### Packaging And Release

- The app must package with Electron Builder.
- Windows packaging must produce portable executables.
- macOS packaging must produce DMG and ZIP artifacts.
- Linux packaging must produce AppImage, DEB, and tar.gz artifacts.
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
