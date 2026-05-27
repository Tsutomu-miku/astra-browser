# Code Structure

This map is the working guide for where new code should live.

```text
src/main
  Electron host process, Chromium sessions, IPC handlers, native shell integrations

src/renderer/common
  Cross-surface interaction models and reusable renderer helpers
  navigation/    Shared list navigation math
  omnibox/       Address/start-page suggestion and submit rules
  shortcuts/     Keyboard parsing and shared shortcut target ordering

src/renderer/app
  React application composition and stateful renderer orchestration
  App.tsx        Browser shell layout and surface mounting
  controller/    React hooks that bind store state, side effects, commands, omnibox,
                 compact chrome behavior, and Electron event subscriptions

src/renderer/domain
  Pure browser product rules. Keep the root as a small public API surface.
  browser-core.ts     Stable aggregate for state shape, browser primitives, and helpers
  browser-actions.ts  Stable aggregate for store actions
  tab-actions.ts      Stable aggregate for tab actions
  browser/            State shape, migrations, navigation, selectors, URL identity,
                      formatting, zoom, and immutable update helpers
  browsing/           History, downloads, and navigation mutations
  permissions/        Site permission rules and settings mutations
  tabs/               Tab lifecycle, grouping, split view, selection, layout, cleanup
  workspaces/         Space/workspace actions and Chromium partition/profile mapping

src/renderer/platform
  Renderer-side adapters for APIs outside pure product rules
  persistence/   localStorage state persistence and import/export backup helpers

src/renderer/stores
  Zustand state container and typed action facade

src/renderer/surfaces
  React UI grouped by product surface
  command/       Command palette component and command model
  glance/        Temporary preview panel
  panels/        Settings, history, downloads, and site info drawers
  sidebar/       Spaces, tabs, Essentials, search, and tab context menu
  start/         React-rendered internal new-tab/start page
  topbar/        Main navigation and address controls
  webview/       Chromium webview grid, lifecycle, and split-pane UI

src/renderer/styles
  CSS split by surface and shared layout primitives

tests
  Unit tests for domain, common models, surface models, and platform helpers
```

## Placement Rules

- Put reusable UI interaction logic in `common` when at least two surfaces use it.
- Put app-level React orchestration in `app/controller`.
- Put one-surface rules in that surface's `model` folder.
- Put React subcomponents in that surface's `components` folder.
- Put domain implementation in a business subfolder (`domain/tabs`, `domain/workspaces`,
  `domain/permissions`, etc.) instead of adding new prefixed files at `domain/` root.
- Import cross-domain browser primitives from `domain/browser-core` in UI code; use the
  narrower subfolder path only for local domain implementation or focused tests.
- Put localStorage, Electron webview lifecycle, and other runtime adapters under `platform`.
- Keep browser state transitions and invariants in `domain`.
