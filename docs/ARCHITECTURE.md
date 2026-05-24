# Architecture

The project is split around runtime boundaries rather than feature folders.

## Main Process

`src/main/main.js` is the Electron/Chromium host. It creates the native browser window, enables `webview`, blocks untrusted top-level shell navigation, routes popups to the OS browser, bridges Chromium download events to the renderer, handles Chromium session permission requests per profile partition, inspects profile storage usage, and clears Chromium session data globally or for selected workspace profile partitions on request.

## Preload

`src/main/preload.js` is the only bridge exposed to renderer code. It keeps `contextIsolation` enabled and publishes a minimal `window.astraShell` API for app version, download events, profile partition registration, profile storage inspection, permission prompts/rules, browsing-data clearing, and showing completed downloads in the file manager.

## Renderer

`src/renderer/stores` owns renderer state via Zustand. Store actions delegate browser mutations to domain actions, persist browser state, answer permission requests, and expose UI state such as panels, command palette state, and address input.

`src/renderer/hooks` is UI orchestration. It derives active workspace/tab view models, builds command and omnibox suggestions, connects webview refs, handles keyboard shortcuts, imports/exports browser state backups, and subscribes to Electron bridge events. It should stay thin enough that browser rules can be tested outside Electron.

`src/renderer/surfaces` contains React browser surfaces such as the sidebar, topbar, command palette, panels, permission prompts, find bar, and webview grid. Surface-level subcomponents live beside the parent surface when that improves readability, but extraction should clarify ownership rather than chase a line count.

`src/renderer/surfaces/sidebar/sidebarFiltering.ts` owns active-Space sidebar search rules for tabs, tab groups, pinned tabs, and favorites. The component keeps only the query state and rendering decisions.

`src/renderer/surfaces/webview` keeps current Space webviews mounted and hides inactive tabs instead of unmounting them. This preserves Chromium page state during ordinary tab switching while still rendering split view as the two visible webviews first. Sleeping tabs are the explicit exception: they remain in renderer state and the sidebar, but their hidden Chromium webview is omitted until selection wakes them.

`src/renderer/domain` contains pure browser-product rules: default state construction, state migration, startup behavior, URL normalization, homepage/search handling, tab lifecycle, selection/cycling, cleanup, grouping, duplication, mute, pin, and navigation state, ordering, workspace creation/deletion/ordering and transfer actions, workspace profile partitions, site permission rules, favorites, history management, recently closed tabs, workspace accents, selectors, and formatting helpers. `browser-core.ts`, `browser-actions.ts`, and `tab-actions.ts` are compatibility barrels; implementation lives in focused modules such as `browser-constants.ts`, `browser-factory.ts`, `navigation.ts`, `state-normalization.ts`, `tab-lifecycle-actions.ts`, `tab-cleanup-actions.ts`, `tab-group-actions.ts`, `tab-selection-actions.ts`, `tab-layout-actions.ts`, `tab-state-actions.ts`, `workspace-actions.ts`, `workspaceProfiles.ts`, `browsing-actions.ts`, and `settings-actions.ts`.

## Verification

`pnpm check` runs source validation, TypeScript typechecking, Vitest tests, and a Vite production build. The source validation checks required files, JS/MJS syntax, absence of checked-in TODO markers, and reports unusually large files as review warnings. Tests cover pure browser-core behavior without requiring Electron or a GUI runtime.
