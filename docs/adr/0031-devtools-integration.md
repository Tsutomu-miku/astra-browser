# ADR-0031: DevTools Integration

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra needs DevTools integration for two purposes:

1. **Developer debugging of Astra features.** Developers building Astra
   need to inspect workspace metadata, tab features, and other Astra state
   alongside the standard DevTools panels.
2. **Astra-specific DevTools UX enhancements.** Astra may want to add
   toolbar buttons or panels that integrate with Astra product features
   (e.g., focus mode toggle from DevTools, workspace inspector).

The core question is how to extend DevTools without forking it or
reimplementing it. Chromium's DevTools is a massive, complex subsystem —
the entire frontend, protocol, agent host, window management, and dock
infrastructure are Chromium-owned and must remain so.

Possible approaches:

- **Chrome DevTools extension API** (`chrome.devtools.panels`) — register
  a custom panel via an extension.
- **WebUI custom panel** — add a WebUI-based panel via a Chromium patch.
- **Native Views panel** — add a Views-based panel via a Chromium patch.
- **Toolbar injection only** — add buttons to the DevTools toolbar without
  full panels.

## Decision

Astra uses **native Views extensions** injected into Chromium's DevTools
window via small patch points. Chromium owns the full DevTools experience;
Astra only adds:

1. A **toolbar strip** (`AstraDevToolsToolbar`) with Astra-specific
   shortcut buttons (Focus Mode, Workspace Inspector).
2. A **Workspace Inspector panel** (`AstraDevToolsWorkspacePanel`) that
   shows Astra workspace and tab metadata for debugging.
3. An **integration coordinator** (`AstraDevToolsIntegration`) that
   manages lifecycle and wires Astra services to DevTools UI.

All three components live in `astra/ui/views/devtools/`.

### Architecture

```
Chromium DevTools (fully reused)
  DevToolsWindow — hosts the window, dock, all standard panels
  DevToolsUIBindings — panel registration and protocol binding
  DevToolsAgentHost — per-tab DevTools agent
  All standard panels (Elements, Console, Sources, etc.) — unchanged

Astra DevTools extensions (native Views, injected via patches)
  AstraDevToolsIntegration — coordinator / lifecycle manager
    ├── AstraDevToolsToolbar — extra toolbar buttons
    │     ├── Focus Mode button
    │     └── Workspace Inspector button
    └── AstraDevToolsWorkspacePanel — native Views debug panel
          ├── Current workspace info
          ├── Workspace list
          └── Tab Astra metadata
```

### Coordinator Pattern

`AstraDevToolsIntegration` is not a View subclass. It is a coordinator /
controller object that:

- Creates and owns the toolbar and panel views.
- Bridges Astra services (workspace service, tab features, focus mode)
  to the DevTools UI components.
- Handles DevTools lifecycle events (open, close, inspected tab change).
- Provides an observer interface for other Astra components that need
  to react to DevTools panel state changes.

The coordinator is instantiated by the DevToolsWindow patch point and
destroyed when the DevTools window closes. It holds raw pointers to
Astra services (looked up from the profile), not owning any state itself.

### Native Views Panel (Not WebUI)

The Workspace Inspector is a native `views::View` subclass, not a WebUI
or extension panel. Reasons:

- **Direct C++ service access.** The panel reads from
  `AstraWorkspaceService` and `AstraTabFeatures` directly without
  needing a JS bridge or Mojo bindings.
- **Performance.** Native Views update synchronously without IPC overhead.
- **No extra extension overhead.** No need to bundle a built-in extension
  or manage extension lifecycle.
- **Consistent with Astra's other UI.** All Astra browser chrome uses
  Views; DevTools panels should follow the same pattern.

### Patch Points

Four patch points integrate Astra DevTools extensions with Chromium:

| # | Location | Purpose | Type |
|---|----------|---------|------|
| 1 | `chrome/browser/devtools/devtools_window.cc` (constructor) | Create `AstraDevToolsIntegration` instance | Registration hook |
| 2 | `chrome/browser/devtools/devtools_window.cc` (toolbar construction) | Inject `AstraDevToolsToolbar` into toolbar area | Observer / delegate hook |
| 3 | `chrome/browser/devtools/devtools_ui_bindings.cc` | Register Workspace Inspector as a native panel | Registration hook |
| 4 | `chrome/browser/devtools/devtools_window.cc` (inspected tab change) | Notify coordinator of inspected tab change | Observer hook |

All patches are build-flag gated with `BUILDFLAG(IS_ASTRA_BRANDED)` and
delegate immediately to Astra code. No product logic lives in patched files.

### Browser-Layer Helper

`AstraDevToolsHelper` (in `astra/browser/`) provides a static helper API
for basic DevTools operations (toggle, open, close, dock state) that wraps
Chromium's `DevToolsWindow` API. This is the projection layer — it does
not own DevTools state, it provides an Astra-friendly interface to
Chromium-owned DevTools state.

The helper defines its own `AstraDevToolsDockState` enum so that the
browser layer does not need to include DevTools headers directly,
avoiding unwanted dependencies.

### Truth Sources

The DevTools UI components are pure projections. They read from:

- `AstraWorkspaceService` — workspace list, active workspace, accent colors
- `AstraTabFeatures` (`WebContentsUserData`) — per-tab Astra metadata
- `TabStripModel` — tab counts per workspace
- Chromium DevTools state — dock state, open/closed, inspected tab

DevTools UI never stores state, never mutates state directly (it dispatches
commands), and refreshes from services when notified of changes.

## Consequences

Positive:

- **Full DevTools reuse.** All standard DevTools panels, protocol, and
  features are used as-is. No reimplementation, no maintenance burden.
- **Native performance.** Views-based panels are fast and integrate
  tightly with C++ services.
- **Clean separation.** Astra DevTools code lives in its own directory
  and is clearly identifiable as Astra product code.
- **Minimal patches.** Four small patch points, all registration or
  observer hooks. No behavioral changes to DevTools.
- **Extensible.** New Astra DevTools panels or toolbar buttons follow
  the same pattern — add a view, wire it through the coordinator, add
  a patch point if needed.
- **Debuggable.** The workspace inspector panel gives Astra developers
  visibility into product state without adding logging or breakpoints.

Negative:

- **Patch maintenance.** Each patch point must be rebased when Chromium
  updates. DevTools internals change relatively frequently.
- **Native panel complexity.** Adding a native Views panel to DevTools
  is more complex than using the extension API, because DevTools' panel
  infrastructure is designed for WebUI panels, not native Views.
- **Limited to Astra-branded builds.** The panels only appear in
  Astra-branded Chromium builds, not in stock Chrome.
- **No extension compatibility.** Since these are native panels, they
  cannot be installed via the Chrome Web Store or used by extensions.

Neutral:

- The DevTools integration is primarily a developer tool / debug aid.
  It is not a user-facing feature. The Workspace Inspector panel is
  analogous to Chrome's internal `about://net-internals` or other
  developer-only pages.
- The toolbar has Focus Mode and Workspace Inspector buttons today, but
  more buttons can be added as Astra features grow.

## Alternatives Considered

### DevTools Extension API (`chrome.devtools.panels`)

Build Astra's DevTools features as a built-in extension using the
`chrome.devtools.panels` API. Panels would be HTML/JS WebUI surfaces.

- Rejected: Requires a JS ↔ C++ bridge for every piece of Astra state
  that the panel needs to display. This adds significant IPC overhead
  and complexity compared to native Views panels that access C++ services
  directly. Also requires bundling a built-in extension with its own
  lifecycle management.

### WebUI Panel (via DevToolsUIBindings patch)

Add a WebUI panel (like the standard DevTools panels) via a Chromium
patch, with Mojo bindings to Astra services.

- Rejected: Similar to the extension approach but with a C++ patch
  instead of an extension. Still requires building a full WebUI frontend
  and Mojo bindings, which is more work than native Views for a
  debug panel. WebUI is better suited for user-facing feature pages
  (settings, history), not internal debug tools.

### No DevTools integration (use logging / external tools)

Skip custom DevTools panels entirely and use logging, `chrome://inspect`,
and external tools for Astra debugging.

- Rejected: The workspace inspector panel is a major productivity boost
  for Astra developers. Being able to see Astra tab metadata alongside
  standard DevTools panels speeds up debugging significantly. The
  incremental cost of adding Views panels via small patches is worth
  the developer productivity gain.

### DevTools as a separate extension installable by users

Provide Astra DevTools as a downloadable Chrome extension.

- Rejected: Astra DevTools panels need deep access to internal Astra
  state that is not exposed through extension APIs. A user-installable
  extension could not read `AstraWorkspaceService` or
  `AstraTabFeatures` data.

## References

- **Chromium subsystems reused:** `DevToolsWindow`, `DevToolsUIBindings`,
  `DevToolsAgentHost`, `DevToolsManager`, `chrome/browser/devtools/`
- **Astra components:** `AstraDevToolsIntegration` (coordinator),
  `AstraDevToolsToolbar`, `AstraDevToolsWorkspacePanel`,
  `AstraDevToolsHelper` (browser-layer projection)
- **Source location:** `chromium/astra/ui/views/devtools/`
- **Build target:** `//astra/ui/views/devtools` (`astra_devtools`)
- **Patch points:** `devtools_window.cc` (3 hooks),
  `devtools_ui_bindings.cc` (1 hook)
- **Related ADRs:** ADR-0021 (Direct Chromium Patch Strategy),
  ADR-0032 (Projection Pattern)
