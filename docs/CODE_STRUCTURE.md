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

src/renderer/domain
  Pure browser product rules: state shape, migrations, tabs, workspaces, history,
  permissions, navigation, split state, selectors, formatting

src/renderer/hooks
  React hooks only: controller composition, effects, compact chrome peeking,
  omnibox state wiring, profile storage polling

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
- Put one-surface rules in that surface's `model` folder.
- Put React subcomponents in that surface's `components` folder.
- Keep `hooks` for functions that call React hooks.
- Put localStorage, Electron webview lifecycle, and other runtime adapters under `platform`.
- Keep browser state transitions and invariants in `domain`.
