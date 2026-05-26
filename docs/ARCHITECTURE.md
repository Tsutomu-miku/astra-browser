# Architecture

The project is split around runtime boundaries rather than feature folders.

## Main Process

`src/main/main.js` is the Electron/Chromium host. It creates the native browser window, enables `webview`, blocks untrusted top-level shell navigation, routes popups to the OS browser, bridges Chromium download events to the renderer, handles Chromium session permission requests per profile partition, inspects profile storage usage, and clears Chromium session data globally or for selected workspace profile partitions on request.

## Preload

`src/main/preload.js` is the only bridge exposed to renderer code. It keeps `contextIsolation` enabled and publishes a minimal `window.astraShell` API for app version, download events, profile partition registration, profile storage inspection, permission prompts/rules, browsing-data clearing, and showing completed downloads in the file manager.

## Renderer

`src/renderer/stores` owns renderer state via Zustand. Store actions delegate browser mutations to domain actions, persist browser state, answer permission requests, and expose UI state such as panels, command palette state, and address input.

`src/renderer/hooks` is UI orchestration. It derives active workspace/tab view models, builds command and omnibox suggestions, connects webview refs, handles keyboard shortcuts, imports/exports browser state backups, and subscribes to Electron bridge events. It should stay thin enough that browser rules can be tested outside Electron.

`src/renderer/common` contains reusable renderer interaction helpers that are not browser state rules and are not owned by one surface. List navigation, shared shortcut target ordering, and similar cross-surface UI utilities belong here so command palette, omnibox, sidebar, and future panels do not duplicate the same behavior.

`src/renderer/surfaces` contains React browser surfaces such as the sidebar, topbar, command palette, panels, permission prompts, find bar, and webview grid. Surface-level subcomponents live beside the parent surface when that improves readability, but extraction should clarify ownership rather than chase a line count.

Compact mode is renderer-owned UI state. It does not alter persisted browser data; it composes the existing collapsible sidebar with a floating topbar and compact sidebar address field so the content grid can reclaim the full viewport while browser chrome remains available on hover or focus.

`src/renderer/surfaces/sidebar/sidebarFiltering.ts` owns active-Space sidebar search rules for tabs, tab groups, pinned tabs, and favorites. The component keeps only the query state and rendering decisions.

Essentials are global quick entries stored on `BrowserState`, while favorites remain scoped to a workspace. The sidebar renders Essentials above Space-local tab controls so core pages stay available across Spaces without changing the active workspace profile model.

`src/renderer/surfaces/webview` keeps current Space webviews mounted and hides inactive tabs instead of unmounting them. This preserves Chromium page state during ordinary tab switching while still rendering split view webviews first, up to four visible panes. Sleeping tabs are the explicit exception: they remain in renderer state and the sidebar, but their hidden Chromium webview is omitted until selection wakes them.

Two-pane split view keeps its resize ratio as local renderer UI state. The ratio is intentionally not persisted with browser tabs, while the clamp helper is pure and tested so keyboard and pointer resizing share the same pane bounds.

Split layout mode is also renderer-owned UI state. Keyboard shortcuts and commands can switch visible split tabs between horizontal, vertical, and grid arrangements without changing the underlying tab or workspace model.

`src/renderer/surfaces/glance` owns temporary page previews. Glance uses the current workspace Chromium partition but is not part of persisted tab state until the user opens it as a tab or adds it to split view.

`src/renderer/surfaces/start` renders internal browser pages such as `astra://newtab`. These pages are part of the product shell and do not use Electron webviews, so startup search, global Essentials, favorites, and recent Space history remain responsive even before any external page is loaded.

`src/renderer/platform/webviewLifecycle.ts` owns the renderer-side contract for Electron webview readiness. A webview is registered with the controller only after `dom-ready`, so store and controller actions never call Chromium webview methods on a detached or not-yet-ready element. Lifecycle bugs should be fixed at this boundary rather than hidden by catch-and-ignore compatibility code.

`src/renderer/domain` contains pure browser-product rules: default state construction, state migration, startup behavior, URL normalization, homepage/search handling, tab lifecycle, selection/cycling, cleanup, grouping, duplication, mute, pin, and navigation state, ordering, workspace creation/deletion/ordering and transfer actions, workspace profile partitions, site permission rules, favorites, history management, recently closed tabs, workspace accents, selectors, and formatting helpers. `browser-core.ts`, `browser-actions.ts`, and `tab-actions.ts` are compatibility barrels; implementation lives in focused modules such as `browser-constants.ts`, `browser-factory.ts`, `navigation.ts`, `state-normalization.ts`, `tab-lifecycle-actions.ts`, `tab-cleanup-actions.ts`, `tab-group-actions.ts`, `tab-selection-actions.ts`, `tab-layout-actions.ts`, `tab-state-actions.ts`, `workspace-actions.ts`, `workspaceProfiles.ts`, `browsing-actions.ts`, and `settings-actions.ts`.

## Verification

`pnpm check` runs source validation, TypeScript typechecking, Vitest tests, and a Vite production build. The source validation checks required files, JS/MJS syntax, absence of checked-in TODO markers, and reports unusually large files as review warnings. Tests cover pure browser-core behavior without requiring Electron or a GUI runtime.
