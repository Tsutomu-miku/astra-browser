# Requirements

## Product Intent

Astra Browser should feel like a Chromium-based browser shaped by Zen and Arc interaction patterns: vertical organization, low-friction Spaces, compact chrome, strong keyboard access, rich split-view workflows, and fast preview/navigation surfaces. Electron is acceptable for the current stage because it embeds Chromium and lets the product shell move quickly.

## Documentation Rules

- Keep README focused on orientation, setup, scripts, packaging, and links.
- Keep priority, progress, and requirement splitting in `docs/ROADMAP.md`.
- Keep product requirements in this file.
- Keep engineering direction and structure in `docs/PROJECT_SPEC.md`.
- Keep runtime architecture details in `docs/ARCHITECTURE.md`.
- When a new user-facing feature lands, update the relevant requirement here instead of expanding README.

## Priority And Identity Rules

Detailed P0/P1/P2 priority, current progress, and requirement split guidance live in `docs/ROADMAP.md`.

For tab-related features, requirements must specify object identity. In particular, Space Favorites are tab-like entries: clicking a Favorite should select its matching tab and must not replace the current active tab's URL. URL-only Favorites are legacy/import fallback data and should recover or create a tab instead of mutating the active tab.

## Current Scope

### Browser Shell And Spaces

- The app must provide a Chromium-backed desktop browser shell.
- The app must organize browsing around vertical Spaces.
- Users must be able to create, delete, rename, recolor, and reorder Spaces.
- The workspace strip must expose a direct new-Space action without opening Settings.
- The workspace strip must expose direct Space management for settings, switching, creating, renaming, recoloring, and deleting from a Space context menu.
- Workspace strip context menus must restore focus to their triggering Space button when closed by Escape or a menu action.
- The workspace strip must support Arrow, Home, and End keyboard focus navigation across Spaces, New Space, and the sidebar toggle without stealing editing keys from Space menu fields.
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
- The sidebar footer controls must support ArrowLeft, ArrowRight, Home, and End keyboard focus navigation with explicit labels for icon-only actions.
- Sleeping tabs must expose a direct wake action from the sidebar tab context menu.
- Active tabs, split-view tabs, and pinned tabs must be protected from bulk sleep.
- Manually sleeping the current tab must move focus to a neighboring tab first and wake that fallback if needed.
- Users must be able to close tabs directly from the sidebar with recently closed recovery.
- The sidebar must expose a compact recently closed section for restoring the active Space's latest closed tabs.
- Sidebar recently closed rows must support Alt-click Glance preview and Shift-click split opening.
- Sidebar recently closed rows must support context menus for restore, restoring into another Space, Glance preview, split opening, and copying page details.
- Sidebar recently closed rows must be draggable onto Space buttons or the New Space button to restore them into the target Space.
- Sidebar recently closed rows must expose accessible labels with restore position and dragging state.
- Users must be able to duplicate the active tab.
- Users must be able to reorder sidebar tabs by drag and drop.
- Sidebar tab drag-and-drop must remain reliable across tab rows, Pinned, tab groups, Essentials, Favorites, Spaces, New Space, and Split drop targets even when drag state is recovered from the native drag payload instead of React state.
- Sidebar Essential and Favorite reordering must also recover from native drag payloads instead of depending only on React drag state.
- Sidebar drag reordering must show a before/after insertion indicator for tabs, pinned tabs, Essentials, favorites, and Spaces.
- Sidebar tab drag should avoid explicit target-region overlays; valid destinations should accept drops directly, and reordered items should show only the local before/after insertion indicator.
- Users must be able to move tabs between Spaces from the command palette or by dragging onto Space buttons.
- Users must be able to drag sidebar tabs onto the New Space button to create a Space from that tab.
- Sidebar Space context menus must support switching, renaming, recoloring, deleting, and keyboard opening via `ContextMenu` or `Shift+F10`.
- Sidebar Space rail buttons must expose active and drop-target state in accessible labels, including the New Space drop target.
- Users must be able to move tabs between Spaces from the sidebar tab context menu.
- Sidebar tab context menus must support opening, split-view targeting, duplication, pinning, muting, and closing.
- Sidebar tab context menus must support copying the tab URL and title without selecting the tab.
- Sidebar tab context menus must support adding or removing a tab from Space favorites and global Essentials.
- Sidebar tab rows and pinned tab buttons must support middle-click closing without selecting the tab first.
- Focused sidebar tab rows and pinned tab buttons must support Delete and Backspace to close without selecting the tab first.
- Focused sidebar tab rows must reveal close controls and action affordances without requiring pointer hover.
- Tab-backed Sidebar Favorites must use tab-level context menus and close shortcuts; legacy URL Favorites must keep quick-entry context menus.
- Tab-backed Sidebar Favorites must render and behave through the shared tab row path; Favorites is their folder, not a separate tab implementation.
- Tab-backed Sidebar Favorites must expose tab-level active, split, muted, and sleeping state in accessible labels and compact badges.
- Tab-backed Sidebar Favorites must send the matching tab to split view instead of creating a duplicate URL split; legacy URL Favorites may open a URL split.
- Tab-backed Favorites from Start page, omnibox, and command palette split actions must send the matching tab to split view instead of creating a duplicate URL split.
- Sidebar quick-entry fallback menus must also split tab-backed Favorites by tab identity when a matching tab exists.
- Closing a tab-backed Favorite must close the tab and remove that tab from the Favorites folder; legacy URL Favorites must not be removed just because a matching URL tab closes.
- Sidebar tab rows must expose compact visual status badges for split-view, muted, and sleeping states.
- Pinned tab buttons must expose compact visual status badges for split-view, muted, and sleeping states.
- Sidebar tab rows and icon-only pinned tab buttons must expose accessible labels that include active and status state.
- Pinned tabs must participate in sidebar drag-and-drop reordering.
- Users must be able to close other tabs, close tabs to the left, and close tabs to the right from command palette and sidebar tab context menu flows.
- Users must be able to cycle tabs with keyboard shortcuts or the command palette.
- Users must be able to sleep the current tab from the command palette when another tab can receive focus.

### Groups, Pins, Essentials, And Favorites

- Users must be able to create named, color-coded, collapsible tab groups.
- Users must be able to reorder tab groups directly from the sidebar with drag and drop.
- Sidebar tab group headers must support context menus for collapsing, sleeping, duplicating, renaming, recoloring, moving to another Space or a new Space, closing, and ungrouping the whole group.
- Sidebar tab group toggles must support `ArrowLeft` to collapse and `ArrowRight` to expand without stealing editing keys from group name fields.
- Sidebar tab group toggles must participate in the same Arrow, Home, and End keyboard focus navigation as other primary sidebar items.
- Users must be able to drag sidebar tab groups onto Space buttons to move the whole group between Spaces.
- Users must be able to drag sidebar tab groups onto the New Space button to create a Space from that group.
- Sidebar tab context menus must support creating a new group from a tab, moving tabs to existing groups, and ungrouping grouped tabs.
- Tab grouping and ungrouping should stay available through tab context menus and commands; dragging tabs should not create extra New Group or Ungroup target regions in the sidebar.
- Users must be able to pin tabs per Space.
- Users must be able to drag sidebar tabs into Pinned to pin them in the active Space.
- Users must be able to drag pinned tabs back into the regular Tabs section to unpin and place them near the drop target.
- Dropping a pinned tab onto empty space inside the regular Tabs section must unpin it to the end of the regular list.
- Tabs, Pinned, Favorites, and tab groups must be treated as sidebar folders: moving a tab into a folder preserves tab identity and uses the same domain tab-folder move path.
- Moving a tab into Favorites must create or update its tab-backed Favorite entry; moving a Favorite-backed tab into Tabs, Pinned, or a group must remove it from the Favorites folder in that same move action.
- Empty Tabs, Pinned, and Favorites folders must remain visible as ordinary sidebar folder headers when not filtering, accept tab drops on the folder itself, and avoid separate "drop target" regions or labels.
- The regular Tabs folder should accept the same real tab payloads as other sidebar folders and place dropped tabs at the folder end; ordinary tab reordering should happen on tab rows with before/after placement.
- Users must have global Essentials visible across Spaces for core pages.
- Clicking a global Essential from the sidebar must navigate the current tab instead of creating a new tab.
- Users must be able to drag sidebar tabs into Essentials to save them as global quick entries.
- Users must be able to reorder global Essentials directly from the sidebar with drag and drop.
- Users must have Space-local favorites for quick access.
- Clicking a Space favorite from the sidebar must select the matching tab when one exists, and must not replace the current active tab's URL.
- Users must be able to drag sidebar tabs into Space favorites to place those tabs in the Favorites section while preserving tab identity.
- Users must be able to reorder Space favorites directly from the sidebar with drag and drop.
- Space favorite reordering must work across mixed tab-backed and legacy URL Favorite rows.
- Users must be able to drag Space favorites onto Space buttons or the New Space button to move them between Spaces.
- Sidebar Essentials and favorites must support context menus for opening, Glance preview, split opening, and removing the entry.
- Sidebar Space favorite context menus must support moving favorites to another Space or to a new Space.
- Opening an Essential from its sidebar context menu must navigate the current tab instead of creating a new tab.
- Opening a Space favorite from its sidebar context menu must select the matching tab when one exists, and must not replace the current active tab's URL.
- Sidebar Essentials and favorites context menus must support copying the entry URL and title.
- Sidebar Essentials and favorites must expose accessible labels with kind, current-page, search selection, dragging, and drop-target state.
- Sidebar tab, pinned tab, tab group, recently closed tab, Essential, and favorite context menus must open from the keyboard with `ContextMenu` or `Shift+F10`.
- Sidebar context menus must focus the first action when opened and support Arrow, Home, and End keyboard navigation.
- Sidebar context menus must restore focus to their triggering sidebar item when closed by Escape or a menu action.
- Sidebar primary items and section headers must support Arrow, Home, and End keyboard focus navigation without stealing editing keys from text fields.
- Sidebar Essentials, pinned tabs, favorites, and tabs must be visually separated into scannable sections.
- Sidebar sections must be individually collapsible, while filtering must reveal matching contents even when a section was collapsed.
- Sidebar collapsible section headers must support `ArrowLeft` to collapse and `ArrowRight` to expand.
- Dragging a tab should not temporarily reveal collapsed sidebar sections just to advertise drop targets; collapsed sections remain under user control unless search filtering needs to reveal matches.
- Sidebar quick entries must show active-page state when the current tab matches an Essential or favorite.
- Users must be able to toggle the current page as a favorite with `Ctrl+D`, `Cmd+D`, `Ctrl+Shift+D`, or `Cmd+Shift+D`.
- Sidebar search must filter global Essentials plus tabs, tab groups, pinned tabs, and favorites inside the active Space.
- Sidebar search must navigate Essential matches in the current tab unless preview or split modifiers are used.
- Sidebar search must select matching Favorite tabs unless preview or split modifiers are used.
- Sidebar search must support Arrow, Home, End, Enter, Alt+Enter preview, and Shift+Enter split-open flows.
- Sidebar search keyboard selection must keep the active result scrolled into view.
- Sidebar search must visually hint Alt preview and Shift split actions while filtering.
- Sidebar search clearing must keep focus in the search input and expose an explicit clear-button label.
- Sidebar items must support Alt-click Glance preview and Shift-click split opening.
- Focused sidebar tabs, pinned tabs, quick entries, and recently closed rows must support Enter, Alt+Enter Glance preview, and Shift+Enter split opening.
- Sidebar tab and favorite rows must reveal Alt preview and Shift split hints on hover or keyboard focus.
- Sidebar row hover hints must not cover tab or favorite titles; they should reserve inline space or stay inside their tile.
- Sidebar tab, pinned tab, Essential, and favorite rows must avoid native browser title tooltips that cover the side panel while scanning or dragging.
- Sidebar Space and tab group menu swatches must avoid native browser title tooltips and use accessible labels instead.
- Pinned tab buttons must reveal Alt preview and Shift split hints on hover or keyboard focus.

### Navigation, Address Bar, And Omnibox

- The address bar must support direct URLs and search fallback.
- The address bar must be focusable with `Ctrl+L`, `Cmd+L`, `Ctrl+J`, `Cmd+J`, and `Alt+D`.
- The address identity control must expose a context menu for site information, copying page details, Glance preview, and split opening.
- Address bar suggestions must include open tabs, Essentials, workspace favorites, and browsing history.
- Address bar suggestions must support Arrow, Home, End, Enter, and Alt+Enter split opening.
- Address bar suggestions must visually hint Alt split opening where suggestions are shown.
- Compact mode must expose a sidebar address field with the same suggestion, keyboard behavior, and combobox/listbox accessibility semantics as the top bar.
- Address shortcuts in compact mode must reveal and focus the sidebar address field instead of targeting hidden top chrome.
- Back and forward availability must be tracked per tab from Chromium webview navigation state.
- Back, forward, reload, stop-loading, and hard reload must be available through controls and keyboard shortcuts where appropriate.
- Navigation shortcuts must include `Alt+Left`, `Alt+Right`, `Alt+Home`, `Ctrl+[`, `Cmd+[`, `Ctrl+]`, `Cmd+]`, `Ctrl+R`, `Cmd+R`, `Ctrl+Shift+R`, and `Cmd+Shift+R`.

### Keyboard Shortcuts

- The browser must support keyboard-first tab, Space, command, split, navigation, zoom, find, history, downloads, favorite, mute, and address workflows.
- Direct tab selection must support Zen-style `Alt+1` through `Alt+8`, ordered by Essentials, pinned tabs, then regular tabs.
- Number shortcuts targeting Essentials must navigate the current tab instead of opening a new tab.
- Last-tab selection must support `Alt+9` using the sidebar's visual tab order, including grouped-tab order and excluding tabs hidden inside collapsed groups.
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
- Expanded sidebar width must be resizable with a pointer drag handle and keyboard Arrow/Home/End controls.
- Resized sidebar width must persist locally and the resize handle must support double-click reset to the default width.
- The app must support Zen-style compact mode that hides browser chrome until hover or focus.
- Collapsed and compact sidebars must reveal their tab controls on hover or keyboard focus.
- Compact mode must be toggleable with `Ctrl+Alt+C` or `Cmd+Alt+C`.
- Compact mode must briefly peek browser chrome after tab changes.
- Compact mode must expose a top-edge peek target that holds the toolbar open while hovered or focused and supports temporary command-triggered peeks.
- Compact mode must expose separate command palette actions for temporarily peeking the floating toolbar or floating sidebar without forcing both chrome surfaces open.
- Compact mode must expose a visible control for pinning or unpinning the floating toolbar.
- Compact mode must expose a visible control for pinning or unpinning the floating sidebar.
- Compact mode must support persistent floating sidebar and floating toolbar shortcuts.
- Compact mode must leave content as the primary visual surface.

### Split View And Glance

- Split view must support up to four visible webviews.
- Users must be able to drag tabs, Essentials, Space favorites, and recently closed rows to split view from the sidebar.
- The sidebar Split control must highlight as a drop target when a sidebar item can be sent to split view.
- The sidebar Split control must send tab-backed Favorites by tab identity; URL-only sidebar entries may be sent by URL.
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
- Command palette Favorite entries must select matching tabs by tab identity or legacy URL fallback; they must not replace the current active tab's URL.
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
- New tab Favorites must select matching tabs by tab identity or legacy URL fallback; they must not replace the current active tab's URL.
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
