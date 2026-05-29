# Architecture

The project is split around runtime boundaries rather than feature folders.

## Main Process

`src/main/main.js` is the Electron/Chromium host entry. It creates the native browser window, enables `webview`, blocks untrusted top-level shell navigation, routes popups to the OS browser, and owns the Chromium session bridge for downloads plus per-profile permission requests. `src/main/ipcHandlers.js` owns renderer IPC contracts for app version, diagnostics, profile storage inspection, browsing-data clearing, permission decisions/rules, and native file actions. `src/main/diagnostics.js` owns DevTools shortcuts and failure diagnostics.

## Preload

`src/main/preload.js` is the only bridge exposed to renderer code. It keeps `contextIsolation` enabled and publishes a minimal `window.astraShell` API for app version, download events, profile partition registration, profile storage inspection, permission prompts/rules, browsing-data clearing, opening completed downloads, and showing completed downloads in the file manager.

## Renderer

`src/renderer/stores` owns renderer state via Zustand. Store actions delegate browser mutations to domain actions, persist browser state, answer permission requests, and expose UI state such as panels, command palette state, and address input.

`src/renderer/app` is React-only application orchestration. `App.tsx` mounts the browser surfaces, while `app/controller` derives active workspace/tab view models, wires shared command and omnibox models into components, connects webview refs, and subscribes to Electron bridge events. Files in this folder should call React hooks or compose hook state; pure helpers belong in `common`, `domain`, `platform`, or the owning surface's `model` folder.

`src/renderer/common` contains reusable renderer interaction helpers that are not browser state rules and are not owned by one surface. List navigation, the shared omnibox suggestion/action model, keyboard shortcut parsing, shared shortcut target ordering, and similar cross-surface UI utilities belong here so command palette, omnibox, sidebar, and future panels do not duplicate the same behavior.

`src/renderer/platform` contains renderer-side adapters for runtime APIs that are outside pure browser state rules. Webview lifecycle registration, localStorage persistence, and browser state import/export helpers live here instead of in app controller hooks or domain modules.

`src/renderer/surfaces` contains React browser surfaces such as the sidebar, topbar, command palette, panels, permission prompts, find bar, and webview grid. Surface-level subcomponents live under `components`, while one-surface pure rules live under `model`. Extraction should clarify ownership rather than chase a line count.

Compact mode is renderer-owned UI state. It does not alter persisted browser data; it composes the existing collapsible sidebar with a floating topbar and compact sidebar address field so the content grid can reclaim the full viewport while browser chrome remains available on hover or focus.

Sidebar sizing is renderer-owned UI state. Width clamping and keyboard resize rules live in `src/renderer/common/layout` so the store, sidebar handle, and tests share one range without coupling store code to a sidebar surface module.

Compact chrome peeking is isolated in a controller hook so tab changes, shortcuts, and the top-edge peek target all share the same temporary reveal timer without leaking timer state into browser domain logic.

`src/renderer/surfaces/sidebar/sidebarFiltering.ts` owns active-Space sidebar search rules for tabs, tab groups, pinned tabs, and favorites. The component keeps only the query state and rendering decisions.

Essentials are global quick entries stored on `BrowserState`, while favorites remain scoped to a workspace. The sidebar renders Essentials above Space-local tab controls so core pages stay available across Spaces without changing the active workspace profile model.

`src/renderer/surfaces/webview` keeps current Space webviews mounted and hides inactive tabs instead of unmounting them. This preserves Chromium page state during ordinary tab switching while still rendering split view webviews first, up to four visible panes. Sleeping tabs are the explicit exception: they remain in renderer state and the sidebar, but their hidden Chromium webview is omitted until selection wakes them.

Two-pane split view keeps its resize ratio as local renderer UI state. The ratio is intentionally not persisted with browser tabs, while the clamp helper is pure and tested so keyboard and pointer resizing share the same pane bounds.

Split layout mode is also renderer-owned UI state. Keyboard shortcuts and commands can switch visible split tabs between horizontal, vertical, and grid arrangements without changing the underlying tab or workspace model. Split panes can be promoted to the active tab through a domain action, while `surfaces/webview/components` owns the webview lifecycle and split-pane controls.

`src/renderer/surfaces/glance` owns temporary page previews. Glance uses the current workspace Chromium partition but is not part of persisted tab state until the user opens it as a tab or adds it to split view.

Glance header controls and webview rendering live under `surfaces/glance/components`; navigation state is read through a small helper so optional Electron webview methods stay isolated from the visual component.

`src/renderer/surfaces/start` renders internal browser pages such as `astra://newtab`. These pages are part of the product shell and do not use Electron webviews, so startup search, global Essentials, favorites, and recent Space history remain responsive even before any external page is loaded. Start search reuses the shared omnibox suggestion/action model, while its visual pieces live under `surfaces/start/components`. Start entry modifier intent is separated from rendering so tile and history entries share the same open, Glance preview, and split-open behavior.

`src/renderer/surfaces/sidebar` owns vertical navigation, Space switching, tab organization, and sidebar search. Sidebar search intent resolution is separated from rendering so keyboard and pointer modifiers can share the same preview, split-open, and normal-open rules.

Command palette entries expose optional preview and split runners. Command-specific building, search, selection, and modifier intent live under `src/renderer/surfaces/command/model` so the surface owns its own rules while keeping the visual component focused.

`src/renderer/platform/webviewLifecycle.ts` owns the renderer-side contract for Electron webview readiness. A webview is registered with the controller only after `dom-ready`, so store and controller actions never call Chromium webview methods on a detached or not-yet-ready element. Lifecycle bugs should be fixed at this boundary rather than hidden by catch-and-ignore compatibility code.

`src/renderer/domain` contains pure browser-product rules: default state construction, state migration, startup behavior, URL normalization, homepage/search handling, tab lifecycle, selection/cycling, cleanup, grouping, duplication, mute, pin, navigation state, workspace creation/deletion/ordering and transfer actions, workspace profile partitions, site permission rules, favorites, history management, recently closed tabs, workspace accents, selectors, and formatting helpers. UI code imports shared browser primitives from `domain/browser`, store-facing mutations from `domain/actions`, and narrower implementation modules only for local domain work or focused tests.

## Verification

`pnpm check` runs source validation, TypeScript typechecking, Vitest tests, and a Vite production build. The source validation checks required files, JS/MJS syntax, absence of checked-in TODO markers, and reports unusually large files as review warnings. Tests cover pure browser domain behavior without requiring Electron or a GUI runtime.
