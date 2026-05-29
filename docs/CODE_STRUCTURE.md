# Code Structure

This map is the working guide for where new code should live.

```text
src/main
  Electron host process, Chromium sessions, IPC handlers, native shell integrations
  diagnostics.js   DevTools shortcuts and renderer failure diagnostics
  ipcHandlers.js   Native IPC contracts for profile data, downloads, permissions,
                   diagnostics, and shell file actions
  main.js          Window lifecycle, webContents policy, and Chromium session bridge

src/renderer/common
  Cross-surface interaction models and reusable renderer helpers
  navigation/    Shared list navigation math and navigation control state
  omnibox/       Address/start-page suggestion and submit rules
  shortcuts/     Keyboard parsing and shared shortcut target ordering

src/renderer/app
  React application composition and stateful renderer orchestration
  App.tsx        Browser shell layout and surface mounting
  controller/    Thin browser controller composition plus focused hooks for address
                 focus, action facades, shortcut routing, commands, omnibox,
                 compact chrome behavior, and Electron event subscriptions

src/renderer/domain
  Pure browser product rules. Keep the root as a small public API surface.
  actions.ts          Store-facing aggregate for browser, tab, Space, data, and
                      permission mutations
  browser/            Public browser model entry plus state shape, migrations,
                      navigation, selectors, URL identity, formatting, zoom,
                      and immutable update helpers
  browsing/           History, downloads, and navigation mutations
  permissions/        Site permission rules and settings mutations
  tabs/               Public tab action entry plus lifecycle, grouping, split view,
                      selection, layout, cleanup, and tab utilities
  workspaces/         Space/workspace actions and Chromium partition/profile mapping

src/renderer/platform
  Renderer-side adapters for APIs outside pure product rules
  persistence/   localStorage state persistence and import/export backup helpers

src/renderer/common
  Cross-surface renderer interaction and layout helpers
  layout/        Shared UI sizing/clamping rules used by store state and surfaces

src/renderer/stores
  Zustand state container and typed action facade
  browserStore.ts       Runtime state implementation and side effects
  browserStoreTypes.ts  Store contract, UI state enums, and action signatures

src/renderer/surfaces
  React UI grouped by product surface
  command/       Command palette component and command model
  glance/        Temporary preview panel
  panels/        Settings, history, downloads, and site info drawers
  sidebar/       Spaces, tabs, Essentials, search, and tab context menu
    components/chrome      Sidebar shell controls, footer, address, and search box
    components/tabs        Tab lists, groups, status rows, and tab context menu
    components/workspaces  Vertical Space strip and Space management menu
    model                  Sidebar-only state derivation and menu/search rules
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
- Keep `useBrowserController` as a composition layer; move action facades, shortcut
  routing, DOM focus, and side-effect subscriptions into focused controller hooks.
- Put one-surface rules in that surface's `model` folder.
- Put React subcomponents in that surface's `components` folder. When a surface grows,
  split components by visual/behavioral area instead of keeping every component flat.
- Put domain implementation in a business subfolder (`domain/tabs`, `domain/workspaces`,
  `domain/permissions`, etc.) instead of adding new prefixed files at `domain/` root.
- Import cross-domain browser primitives from `domain/browser` in UI code; import
  store-facing mutations from `domain/actions`. Use narrower subfolder paths only
  for local domain implementation or focused tests.
- Put localStorage, Electron webview lifecycle, and other runtime adapters under `platform`.
- Keep browser state transitions and invariants in `domain`.
