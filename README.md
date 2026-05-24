# Astra Browser

A Chromium-based browser shell inspired by Zen and Arc. The first implementation uses Electron so the browser engine is Chromium while the product shell can move quickly.

## Current Features

- Vertical sidebar with workspace switcher.
- Drag and drop workspace buttons to reorder Spaces.
- Create and delete workspaces from settings or the command palette.
- Workspace profiles map Spaces to separate persistent Chromium partitions.
- Multiple tabs per workspace.
- Collapsible, named, and color-coded tab groups for organizing workspace tabs.
- Direct sidebar tab closing with recently closed recovery.
- Sidebar tab context menu for opening, split-view targeting, duplication, pinning, muting, and closing.
- Close other tabs from the command palette while preserving recent closed recovery.
- Close tabs to the left from the command palette.
- Close tabs to the right from the command palette.
- Drag and drop sidebar tabs to reorder them.
- Duplicate the active tab from the command palette.
- Cycle tabs with Ctrl+Tab, Ctrl+Shift+Tab, or the command palette.
- Move tabs between workspaces from the command palette or by dragging onto workspace buttons.
- Mute or unmute the active tab with Chromium webview audio controls.
- Pinned tabs per workspace for favorite pages.
- Workspace favorites for quick access.
- Address bar with search fallback.
- Address bar suggestions for open tabs, workspace favorites, and browsing history.
- Back, forward, reload, new tab, and close controls.
- Back and forward availability is tracked per tab from Chromium webview navigation state.
- Split view mode with two webviews side by side and explicit tab-to-split commands.
- Command palette with workspace switching, tab cycling, tab grouping, duplicate tabs, mute controls, tab cleanup actions, cross-workspace tab moves, tab actions, data clearing, split view, and history search.
- Command palette entries for open tabs, favorites, history, and reopening recently closed tabs.
- Command palette can directly open URLs or search typed queries.
- Browser keyboard shortcuts for tabs, workspace switching, split view, command palette, and closed-tab restore.
- Collapsible sidebar focus mode.
- Address identity indicator for loading, host, and basic security state.
- Site information panel with per-origin permission controls.
- Site permission decisions are scoped to the current workspace Chromium profile.
- Settings can inspect Chromium profile storage usage per Space.
- Settings can clear Chromium cache/storage and renderer-owned permissions/history for a single Space profile.
- Chromium permission prompts for camera, microphone, location, notifications, and similar web capabilities.
- Clear browsing data from settings or the command palette across workspace Chromium profiles.
- Find in page via Chromium webview search.
- Per-tab page zoom controls using Chromium webview zoom.
- Searchable browsing history drawer with single-entry removal, clearing, and per-workspace closed-tab restore.
- Chromium download tracking with a downloads drawer.
- Settings panel for homepage, search engine, workspace name, workspace accent, and profile display name.
- Startup behavior can restore the previous session or open the homepage in each Space.
- Settings can export and import a JSON backup of Spaces, tabs, favorites, history, permissions, and settings.
- Local tab state persisted in `localStorage`.

## Run

```bash
pnpm install
pnpm check
pnpm dev
```

`pnpm dev` starts Vite for the React/TypeScript renderer and launches Electron against that dev server. `pnpm start` builds the renderer first and then launches Electron against `dist/renderer`.

On minimal Linux environments, Electron also requires native desktop libraries such as GTK, ATK, NSS, and X11/Wayland support. If `pnpm start` fails before opening a window with a missing shared library, install the corresponding system package and rerun the command.

For Debian/Ubuntu-style environments, the common packages are:

```bash
sudo apt-get install -y libatk1.0-0 libatk-bridge2.0-0 libcups2 libcairo2 libgtk-3-0 libpango-1.0-0 libxdamage1 libgbm1 libxkbcommon0 libatspi2.0-0
```

## Architecture

- `src/main/main.js` creates the Chromium-backed application window, keeps external popups outside the shell, and owns Chromium session permission requests.
- `src/main/preload.js` exposes a small, isolated API surface to the renderer.
- `src/renderer/domain` contains testable browser state helpers, focused action modules with stable barrels, and pure formatting/navigation/permission rules.
- `src/renderer/stores` contains the Zustand browser store and typed renderer actions.
- `src/renderer/hooks` owns UI orchestration around active webviews, shortcuts, and Electron bridge subscriptions.
- `src/renderer/surfaces` contains React browser UI grouped by product surface.
- `src/renderer/styles` defines the Zen/Arc-style layout split by surface.
- `tests/browser-core.test.ts` covers state migration, URL normalization, favorite detection, and formatting helpers.

## Direction

This scaffold keeps browser-product features in the renderer and Chromium-window wiring in the main process. Next milestones are per-profile cleanup options by data type, richer session restore controls, and native Chromium source integration if Electron is no longer sufficient.
