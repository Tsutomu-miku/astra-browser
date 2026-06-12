# Engineering Standards

## Prime Directive

Astra is a direct Chromium browser. Every architectural change must preserve
Chromium as the browser framework and keep Astra as a product layer.

If a task seems to require reimplementing a Chrome browser subsystem, pause and
write an ADR or patch-point note first.

## Hard Boundaries

### Do Not Add

- CEF, libcef, CefBrowser, CefClient, CefApp, or CEF setup scripts.
- Electron runtime work for new architecture.
- CMake browser build files.
- AppKit/WinUI/native shell code outside Chromium Views for the primary app.
- Product services that duplicate Chromium-owned services:
  `ProfileManager`, `DownloadManager`, `PermissionManager`, `HistoryService`,
  `ExtensionService`, `PasswordManager`, `AutofillService`,
  `SafeBrowsingService`, `DevToolsService`.

### Do Use

- `ProfileKeyedService` for profile-scoped Astra metadata.
- `content::WebContentsUserData` for per-tab Astra metadata.
- Chromium `Browser`, `TabStripModel`, and `BrowserView` for browser and UI
  ownership.
- Chrome command infrastructure for standard browser commands.
- Chrome WebUI/components for settings, history, downloads, extensions,
  passwords, and internal pages unless a product ADR says otherwise.

## Dependency Rules

```text
astra/app -> astra/browser -> Chromium chrome/content/components
astra/ui/views -> astra/browser + Chromium chrome/ui/views
Chromium patch point -> //astra delegating hook
```

Never invert this:

- `astra/browser` must not depend on Views.
- UI must not own durable browser state.
- Patch points must not contain Astra business logic.
- Legacy `src/` must not import or generate direct Chromium code.

## State Rules

- Chromium owns tab lifetime.
- Chromium owns navigation state.
- Chromium owns browser storage and profile isolation.
- Astra owns only metadata Chromium lacks: workspace id, favorite folder
  membership, split/glance presentation state, and sidebar projection hints.
- URL equality is never the primary identity for a tab-derived object.

## Patch Policy

Chromium patches should be tiny and boring:

- Add a build flag or registration hook.
- Include `//astra` headers only at the smallest necessary point.
- Delegate immediately to Astra code.
- Document each patch in `chromium/astra/patches/README.md` or a patch file.

Do not patch Chromium with product rules that can live in `//astra`.

## C++ Style

- Follow Chromium C++ style.
- Use `raw_ptr` for non-owning Chromium object fields when appropriate.
- Use Chromium factories and keyed services instead of global singletons.
- Keep constructors cheap.
- Prefer explicit `Profile*`, `Browser*`, or `content::WebContents*` parameters
  over hidden lookup.
- Include only what the file uses.
- Avoid generic utility layers until two real call sites exist.

## UI Style

- Build browser UI with Chromium Views.
- UI widgets read models and dispatch commands; they do not mutate product
  collections directly.
- Sidebar rows should be projections of Chromium tabs plus Astra metadata.
- Split/Glance must use Chromium-owned WebContents, not embedded secondary
  runtimes.

## Documentation Rules

- Active docs must describe direct Chromium only.
- Legacy Electron/CEF material belongs under `docs/legacy/electron-prototype/`.
- ADRs in `docs/adr/` are active decisions; deprecated or historical ADRs go to
  legacy.
- If a doc says to build with Electron, CEF, or CMake, it must be in legacy.

## Test Rules

- New Astra product semantics need Chromium unit/browser tests once the checkout
  is available.
- Legacy Vitest tests can serve as behavior references, not as final acceptance.
- Architecture guard checks must pass before handing work to another agent:

```bash
pnpm check:architecture
```

## Agent Handoff Rules

Every implementation handoff should state:

- Chromium subsystem being reused.
- Astra-owned metadata or UI surface being changed.
- Exact Chromium patch point, if any.
- Tests or smoke checks run.
- Any ADR needed before deeper implementation.
