# Astra Browser

Astra Browser is a Chromium-based desktop browser shell inspired by Zen and Arc. It uses Electron for the current implementation so the product shell can iterate quickly while still running pages in Chromium.

## Status

This repository is an active prototype. The current focus is matching Zen-style browser workflows: vertical Spaces, compact chrome, keyboard-first navigation, split views, Glance previews, profile-aware browsing data, and fast access to history, downloads, commands, favorites, and Essentials.

## Documentation

- [Requirements](docs/REQUIREMENTS.md): product scope, functional requirements, keyboard expectations, and quality requirements.
- [Roadmap and progress](docs/ROADMAP.md): P0/P1/P2 priorities, current progress, requirement split rules, and acceptance criteria.
- [Project spec](docs/PROJECT_SPEC.md): product goal, engineering goals, reference structure, and file split heuristics.
- [Architecture](docs/ARCHITECTURE.md): runtime boundaries between Electron main, preload, renderer app orchestration, surfaces, state, and domain logic.
- [Code structure](docs/CODE_STRUCTURE.md): current directory ownership map for day-to-day navigation.

## Highlights

- Chromium-backed Electron shell with isolated main/preload/renderer boundaries.
- Zen/Arc-style vertical sidebar, Spaces, Essentials, workspace favorites, pinned tabs, and compact mode.
- Keep-alive webviews for current Space tabs, explicit sleeping for inactive tabs, and per-workspace Chromium profiles.
- Keyboard-first browser controls, command palette, omnibox suggestions, sidebar search, history, downloads, and find-in-page.
- Glance previews and split view layouts with horizontal, vertical, and grid modes.
- Settings for homepages, startup behavior, search engine, profile storage, permissions, data clearing, and state backup import/export.
- Multi-platform packaging for Windows, macOS, and Linux with GitHub Release publishing.

## Quick Start

```bash
pnpm install
pnpm check
pnpm dev
```

`pnpm dev` starts Vite for the React/TypeScript renderer and launches Electron against the local dev server.

## Scripts

```bash
pnpm dev
pnpm start
pnpm check
pnpm test
pnpm build
pnpm package:win
pnpm package:mac
pnpm package:mac:arm64
pnpm package:mac:x64
pnpm package:mac:all
pnpm package:linux
pnpm package:all
```

- `pnpm start` builds the renderer and launches Electron against `dist/renderer`.
- `pnpm check` runs source validation, TypeScript, tests, and production build.
- `pnpm package:win`, `pnpm package:mac`, and `pnpm package:linux` clean `release/`, create Electron Builder artifacts, and remove unpacked staging directories after distributable files are written.
- `pnpm package:mac` builds the current macOS architecture; `pnpm package:mac:arm64` and `pnpm package:mac:x64` build one macOS architecture at a time.
- `pnpm package:mac:all` runs x64 and arm64 macOS package builds as separate Electron Builder invocations for local release checks.
- `pnpm package:all` invokes supported platform targets from one command while still building macOS x64 and arm64 artifacts in separate Electron Builder runs.
- Release artifacts keep only the supported Electron locales and discard Electron Builder's unpacked `.app` staging folders, blockmaps, and local update manifests to avoid confusing large intermediate output with distributable packages.
- Package scripts build artifacts only; GitHub Release publishing is handled by the release workflow after artifacts are uploaded.

## Releases

Tags matching the package version, such as `v0.1.9`, trigger the release workflow. The workflow runs the quality gate, builds Windows portable executables, macOS DMG/ZIP packages, and Linux AppImage/DEB/tar.gz packages, then publishes the platform artifacts as GitHub Release assets.

## Repository Layout

```text
src/main              Electron host process and Chromium session integration
src/renderer/app      React app shell, controller hooks, and side-effect orchestration
src/renderer/common   Cross-surface interaction models and shared renderer helpers
src/renderer/domain   Testable browser state, navigation, history, profile, and tab rules
src/renderer/platform Renderer adapters for browser runtime and persistence boundaries
src/renderer/stores   Zustand renderer state and typed actions
src/renderer/surfaces React browser UI grouped by product surface
src/renderer/styles   Surface-oriented CSS
tests                 Unit tests for domain and UI orchestration helpers
docs                  Requirements, architecture, and project specs
```

## Troubleshooting

If a packaged build opens to a blank window, press `F12` or `Ctrl+Shift+I` while the Astra window is focused to open the application DevTools. Renderer load failures, renderer crashes, and page console messages are also forwarded to the Electron process log.

On minimal Linux environments, Electron also requires native desktop libraries such as GTK, ATK, NSS, and X11/Wayland support. If `pnpm start` fails before opening a window with a missing shared library, install the corresponding system package and rerun the command.

For Debian/Ubuntu-style environments, the common packages are:

```bash
sudo apt-get install -y libatk1.0-0 libatk-bridge2.0-0 libcups2 libcairo2 libgtk-3-0 libpango-1.0-0 libxdamage1 libgbm1 libxkbcommon0 libatspi2.0-0
```
