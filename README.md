# Astra Browser

A Chromium-based browser shell inspired by Zen and Arc. The first implementation uses Electron so the browser engine is Chromium while the product shell can move quickly.

## Current Features

- Vertical sidebar with workspace switcher.
- Drag and drop workspace buttons to reorder Spaces.
- Create and delete workspaces from settings or the command palette.
- Workspace profiles map Spaces to separate persistent Chromium partitions.
- Each Space has its own homepage for new tabs and startup reset.
- Multiple tabs per workspace.
- Current Space tabs keep their Chromium webviews mounted so tab switches preserve page state.
- Background tabs can be put to sleep to unload hidden Chromium webviews and reduce memory pressure.
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
- Global Essentials visible across Spaces for core pages.
- Workspace favorites for quick access, including Ctrl+D or Cmd+D to favorite the current page.
- Sidebar search filters tabs, tab groups, pinned tabs, and favorites inside the active Space with Arrow/Home/End selection, Enter open, and Alt+Enter Glance preview.
- Internal new tab page with Space-aware search, favorites, and recent history.
- Address bar with search fallback.
- Address bar suggestions for open tabs, Essentials, workspace favorites, and browsing history, with Arrow/Home/End keyboard selection and Alt+Enter split opening.
- Compact mode sidebar address field with the same tab, favorite, history, navigation suggestions, keyboard selection, and Alt+Enter split opening as the top bar.
- Back, forward, reload, hard reload, new tab, and close controls, with Zen-style Alt+Left/Right, Ctrl+[/], Ctrl+R, and Ctrl+Shift+R navigation shortcuts.
- Back and forward availability is tracked per tab from Chromium webview navigation state.
- Split view mode with up to four webviews, drag-to-split from the sidebar, and explicit tab-to-split commands.
- Resizable two-pane split view for horizontal and vertical layouts with pointer and keyboard controls.
- Zen-style split layout shortcuts for horizontal, vertical, grid, and unsplit-all flows.
- Glance preview overlay for quickly checking tabs, favorites, and history without replacing the active tab, tracking preview navigation with open, split, backdrop-close, and Escape-close actions.
- Command palette with workspace switching, tab cycling, tab grouping, duplicate tabs, sleeping tabs, mute controls, tab cleanup actions, cross-workspace tab moves, tab actions, data clearing, split view, and history search.
- Command palette entries for open tabs, Essentials, favorites, history, and reopening recently closed tabs.
- Command palette can directly open URLs or search typed queries, while exact command-name matches execute ahead of search fallback.
- Keyboard-first command palette selection with Arrow keys, Home/End, Enter, and Escape.
- Browser keyboard shortcuts for tabs, including Zen-style Alt+1-8 direct tab selection and Alt+9 last-tab selection, Ctrl+Alt+Q/E Space cycling, split view, command palette, and closed-tab restore.
- Diagnostics shortcuts: F12 and Ctrl+Shift+I toggle the application DevTools in development and packaged builds.
- Collapsible sidebar focus mode and Zen-style compact mode that hides browser chrome until hover, briefly peeks chrome after tab changes, and supports persistent floating sidebar and toolbar shortcuts.
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
- Settings panel for global homepage, search engine, workspace homepage, workspace name, workspace accent, and profile display name.
- Startup behavior can restore the previous session or open the homepage in each Space.
- Settings can export and import a JSON backup of Spaces, tabs, Essentials, favorites, history, permissions, and settings.
- Local tab state persisted in `localStorage`.

## Run

```bash
pnpm install
pnpm check
pnpm dev
pnpm package:win
pnpm package:mac
pnpm package:linux
```

`pnpm dev` starts Vite for the React/TypeScript renderer and launches Electron against that dev server. `pnpm start` builds the renderer first and then launches Electron against `dist/renderer`.
`pnpm package:win`, `pnpm package:mac`, and `pnpm package:linux` create Electron Builder artifacts in `release/`. `pnpm package:all` invokes all configured platform targets from one command when the host platform supports them.

## Troubleshooting

If a packaged build opens to a blank window, press `F12` or `Ctrl+Shift+I` while the Astra window is focused to open the application DevTools. Renderer load failures, renderer crashes, and page console messages are also forwarded to the Electron process log.

## CI Artifacts

GitHub Actions runs `pnpm check` on pushes and pull requests to `main`, then uploads:

- `astra-browser-renderer`: the compiled Vite renderer bundle from `dist/renderer`.
- `astra-browser-runtime`: renderer output plus Electron main/preload files and package metadata.
- `astra-browser-windows`: Windows portable `.exe` packages produced by Electron Builder on `windows-latest`.
- `astra-browser-macos`: macOS `.dmg` and `.zip` packages produced by Electron Builder on `macos-latest`.
- `astra-browser-linux`: Linux `.AppImage`, `.deb`, and `.tar.gz` packages produced by Electron Builder on `ubuntu-latest`.

## Releases

Tags matching the package version, such as `v0.1.0`, trigger the release workflow. The workflow builds Windows, macOS, and Linux packages, then publishes all platform artifacts as GitHub Release assets.

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
