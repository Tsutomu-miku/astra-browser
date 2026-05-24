# Project Spec

## Product Goal

Build a Chromium-based browser shell inspired by Zen and Arc: vertical workspaces, per-workspace Chromium profiles, internal new tab page, per-workspace homepages, keep-alive Chromium webviews for current Space tabs, sleeping background tabs for memory control, workspace creation/deletion, drag-and-drop workspace ordering, compact tab management, sidebar tab search, sidebar tab context actions, named and color-coded collapsible tab groups, direct tab closing, keyboard tab cycling, close-other-tabs and close-left/right cleanup, tab duplication, per-tab mute, per-tab navigation state, drag-and-drop tab ordering, drag-and-drop cross-workspace tab moves, collapsible focus mode, page identity feedback, site information, per-origin permissions, profile storage inspection, global and per-profile browsing-data clearing, configurable startup/session restore, state backup import/export, find in page, per-tab zoom, favorites, explicit split-view tab targeting, keyboard-first navigation, omnibox suggestions, command palette with direct search/navigation, per-workspace recently closed tab recovery, searchable and editable history, downloads, and user settings.

Electron is acceptable for this stage because it embeds Chromium and lets the product shell iterate quickly. If the project later needs native Chromium UI integration, keep this shell architecture as the product prototype and migrate features behind stable domain contracts.

## Engineering Goals

- Use Vite, React, and TypeScript for renderer development.
- Keep Electron main/preload code separate from renderer UI.
- Keep browser-product rules in framework-independent domain modules.
- Keep React components presentational where possible.
- Use Zustand for renderer state management.
- Keep state transitions in typed store actions and domain actions, not scattered across component markup.
- Keep address-bar suggestion ranking in reusable, tested logic rather than component-only filtering.
- Keep tab lifecycle, tab selection/cycling, tab cleanup, grouping, duplication, mute state, pin state, ordering, and workspace-transfer behavior in reusable domain actions so command palette, shortcuts, context menus, and drag/drop sidebar controls remain consistent.
- Keep active Space webviews mounted across tab switches so Chromium page state, scroll position, and in-page session state are not lost by ordinary tab selection.
- Treat Electron `webview` methods as lifecycle-bound APIs: only expose a webview to controller/store code after the element has emitted `dom-ready`, and remove it when unmounted.
- Render internal browser pages such as `astra://newtab` in React surfaces instead of loading them into Chromium webviews.
- Keep sleeping-tab behavior explicit: sleeping a tab unloads its hidden webview, selecting it wakes it, and active/split/pinned tabs are protected from bulk sleep.
- Keep sidebar filtering in reusable, tested logic so tabs, groups, pinned tabs, and favorites share one matching rule.
- Keep workspace creation, deletion, switching, renaming, accent/homepage changes, and ordering in reusable domain actions, and never allow the state to have zero workspaces.
- Keep workspace profile identity stable so Electron webviews can use persistent Chromium partitions per Space.
- Inspect Chromium session cache and storage usage through the main process so profile diagnostics use the real browser runtime.
- Keep startup behavior explicit: restore previous tabs by default, or reset each Space to its own configured homepage when requested.
- Export browser state as normalized JSON and import backups through the same migration path used for local persistence.
- Split large domain action files by responsibility and keep `browser-actions.ts` / `tab-actions.ts` as stable import barrels for store/tests.
- Route Chromium permission requests through the isolated preload bridge and persist origin-level decisions in typed renderer state scoped by workspace profile.
- Clear Chromium session storage/cache for all workspace profile partitions through the main process while clearing renderer-owned history, downloads, and permission rules.
- Clear a single workspace profile through the matching Chromium partition while keeping unrelated Space history and permissions intact.
- Keep history management in domain actions so panel controls, command palette entries, and future shortcuts share the same behavior.
- Treat 300 lines per source file as a review signal, not a hard product goal. Split files when a boundary becomes clearer; keep cohesive files together when splitting would make the code harder to follow.
- Prefer typed interfaces over unstructured objects for browser state and bridge APIs.
- Make new behavior testable without launching Electron whenever possible.
- Do not add compatibility-style catch blocks that swallow runtime errors and leave unclear state behind. Prefer root-cause fixes that enforce the correct lifecycle, data invariant, or ownership boundary; let unexpected errors surface during development and diagnostics.

## Architecture Principles

Use well-known frontend projects as references for architecture principles, not as templates to copy blindly. The current baseline was checked against these GitHub directory structures:

- VS Code: `microsoft/vscode/src/vs` separates reusable base utilities, platform services, editor code, and workbench UI. Reference: https://github.com/microsoft/vscode/tree/main/src/vs
- Grafana: `grafana/grafana/public/app` keeps app shell, core, features, plugins, routes, store, and shared types separated. Reference: https://github.com/grafana/grafana/tree/main/public/app
- Appsmith: `appsmithorg/appsmith/app/client/src` separates actions, API, components, entities, hooks, pages, reducers, sagas, selectors, and widgets. Reference: https://github.com/appsmithorg/appsmith/tree/release/app/client/src
- Supabase Studio: `supabase/supabase/apps/studio` separates app routes, components, data access, hooks, lib, pages, state, styles, tests, and types. Reference: https://github.com/supabase/supabase/tree/master/apps/studio
- Cal.com / Cal.diy: `calcom/cal.com/apps/web` separates app, components, lib, modules, pages, server, styles, and tests. Reference: https://github.com/calcom/cal.com/tree/main/apps/web

- Follow VS Code-style layering for desktop app boundaries: native host capabilities live in the main process, preload exposes a narrow bridge, renderer code owns product UI, and pure domain logic stays testable without Electron.
- Follow large React app conventions from projects such as Next.js, Remix, and mature dashboard codebases: group UI by product surface, keep shared primitives explicit, and avoid dumping unrelated behavior into a global components folder.
- Follow Redux Toolkit/Zustand-style state discipline: stores expose typed actions, components call actions, and browser rules live in reusable domain functions rather than ad hoc component mutations.
- Follow TanStack-style separation for reusable behavior: put ranking, normalization, selectors, keyboard parsing, and state transitions in small modules that can be tested directly.
- Prefer stable public barrels only at module boundaries. Internal implementation files should stay focused and can change without forcing broad import churn.
- Optimize for readable ownership. A file is too large when it mixes unrelated reasons to change, hides domain rules in UI markup, or makes tests difficult; line count is only one clue.
- Do not split mechanically. A 320-line file with one clear responsibility can be healthier than five files that require jumping around to understand one behavior.

## Reference Structure

The project should follow the broad layering used by mature Electron/React projects:

- `src/main`: Electron host process, native window lifecycle, Chromium/session integrations.
- `src/main/preload.js`: narrow isolated bridge from Electron to renderer.
- `src/renderer/domain`: pure browser state, migration, URL, search, history, permissions, focused action modules, selectors, and formatting rules.
- `src/renderer/stores`: Zustand stores and typed renderer state actions.
- `src/renderer/hooks`: stateful React orchestration, side effects, keyboard/command/omnibox builders, and view-model glue.
- `src/renderer/surfaces`: UI grouped by browser surface such as sidebar, topbar, panels, command palette, permissions, find, and webview.
- `src/renderer/surfaces/start`: internal browser start/new-tab surfaces rendered by React.
- `src/renderer/styles`: global CSS split by layout and surface.
- `src/renderer/types`: ambient Electron/webview declarations.
- `tests`: unit tests for domain logic and future hook/component behavior.
- `docs`: architecture and product/engineering specs.

This mirrors the separation seen in large open source apps: runtime shell code is isolated from UI, UI is split into components, and business rules are testable without the app runtime.

## File Split Heuristics

Prefer splitting a file when at least one of these is true:

- It combines runtime IPC, store mutation, domain rules, and UI rendering in one place.
- A component contains repeated subcomponents that have their own props and state-independent rendering rules.
- A domain module has multiple independent action families, such as lifecycle, layout, cleanup, grouping, and state toggles.
- A helper can be tested directly and reused by command palette, omnibox, sidebar, or settings flows.
- The import list or test setup makes the file's ownership unclear.

Prefer keeping code together when:

- The logic has one cohesive reason to change.
- Extracting it would create a file that is just a pass-through wrapper.
- The reader would need to bounce across many tiny files to understand one simple interaction.

## Renderer Boundaries

- React components receive data and callbacks through props.
- Zustand stores own renderer state and expose typed actions.
- Hooks derive view models, connect webview refs, and subscribe to global side effects.
- Domain modules must not import React, Electron, DOM APIs, or localStorage.
- Components must not directly mutate browser state objects.
- Webview lifecycle logic belongs in a focused component or hook.
- Electron webview refs must be registered only after `dom-ready`; code outside the lifecycle owner should never need to guess whether a webview is attached.
- Main-process Chromium session callbacks must communicate with the renderer through preload IPC only.

## Verification Gates

`pnpm check` must pass before considering a change healthy:

- source file existence checks
- TypeScript typecheck
- Vite production build
- unit tests
- source hygiene checks, including TODO detection and soft warnings for unusually large files

Electron runtime launch is a separate gate because minimal Linux environments may lack GUI shared libraries.
