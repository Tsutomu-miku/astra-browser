# ADR-0029: Common Layer

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

As the Astra codebase grows, type definitions, constants, and enums are shared
across multiple layers: `astra/app` (startup hooks), `astra/browser` (services
and metadata), and `astra/ui/views` (UI surfaces).

Without a dedicated common layer, several problems emerge:

- **Duplicate type definitions.** If workspace types are defined in
  `astra/browser` but also needed by `astra/ui/views`, the views layer would
  need to depend on the browser layer for types alone — pulling in unwanted
  service dependencies.
- **Circular dependency risk.** If browser-layer code needs UI-layer types
  (e.g., layout constants) and UI-layer code needs browser-layer types
  (e.g., metadata structs), a circular dependency could form.
- **No single source of truth.** Constants duplicated across layers drift
  over time (e.g., sidebar width defined in both browser prefs defaults and
  UI layout code).
- **Build complexity.** Layer boundaries are unclear when types are scattered
  across directories.

The Chromium codebase solves this with `//base` (fundamental types),
`//ui/gfx` (geometry), and layer-specific common directories. Astra needs
its own equivalent for product-level shared types.

## Decision

Astra creates a dedicated **common layer** at `astra/common/` that sits at
the bottom of the Astra dependency graph. All other Astra layers depend on
common; common depends only on `//base` and fundamental Chromium types.

**What goes in common:**

- Data-only structs used across layers (e.g., `AstraWorkspaceInfo`,
  `AstraSplitViewState`).
- Enums shared between browser and UI layers (e.g.,
  `AstraSplitViewOrientation`, `AstraWorkspaceAccentColor`).
- Integer / string constants shared across layers (e.g., command ID range
  boundaries, sidebar dimensions, animation durations, corner radii).
- Lightweight utility functions that operate on common types and have no
  dependencies on services or state (e.g., `IsAstraCommandId()`,
  `GetAstraAccentColor()`).
- Type aliases and opaque IDs (e.g., `AstraWorkspaceId`,
  `AstraFavoriteFolderId`).

**What does NOT go in common:**

- Stateful services or singletons.
- `ProfileKeyedService` implementations (those belong in `astra/browser`).
- `WebContentsUserData` (browser-layer metadata objects).
- UI views or controllers (those belong in `astra/ui/views`).
- Business logic or computation that depends on service state.
- Anything that depends on `astra/browser` or `astra/ui/views`.

**Dependency graph:**

```
astra/app    ----+
astra/browser  --+--> astra/common --> //base + //skia + //ui/gfx
astra/ui/views --+
astra/ui/color --+
```

**Direction rule:** common never depends on app, browser, or ui/views. The
dependency arrow points one way: everything depends on common.

**Current contents of common:**

| File | Purpose |
|------|---------|
| `astra_workspace_types.h` | Workspace ID, `AstraWorkspaceInfo` struct, accent color enum |
| `astra_tab_types.h` | Split view state, tab feature flags, favorite folder ID |
| `astra_command_constants.h` | Command ID range boundaries, accelerator string IDs |
| `astra_ui_constants.h` | Sidebar dimensions, spacing, corner radii, icon sizes, animation durations |

**Build target:** `//astra/common` is a `source_set` with visibility to
`//astra/*` and `//chrome/*` (for patch points). It depends on `//base`,
`//skia`, and `//ui/gfx` — the minimal set needed for data types.

## Consequences

Positive:

- **Clean dependency graph.** All layers share types through one common
  dependency, eliminating circular dependency risk.
- **Single source of truth.** Constants and types live in one place,
  preventing drift between layers.
- **Fast builds.** Common is a small, lightweight target with minimal deps.
  Changes to common rebuild only what depends on it.
- **Clear ownership boundary.** It is obvious where a new type or constant
  should live: if it is shared across layers, it goes in common.
- **Testable in isolation.** Common types and utilities can be unit-tested
  without bringing in browser services or UI views.

Negative:

- **Another layer to learn.** Developers need to understand the common vs.
  browser vs. views distinction.
- **Migration overhead.** As the codebase evolves, types may need to move
  between layers (e.g., promoting a browser-internal type to common when
  it becomes shared).
- **Risk of bloat.** Without discipline, common could accumulate too many
  types and become a dumping ground. The "no state, no services, no
  business logic" rule prevents this.

Neutral:

- Common is header-heavy (most files are `.h` with inline constexprs).
  This is intentional for a types-and-constants layer.
- The layer is small today (4 files) but will grow as Astra adds more
  shared types (e.g., note types, reading list types, tab stack types).

## Alternatives Considered

### No common layer — types live in browser layer, views depend on browser

Put all shared types in `astra/browser` and have `astra/ui/views` depend
on `astra/browser` for types.

- Rejected: Creates an undesired dependency direction. Views should depend
  on types, not on services. A browser dependency would pull in service
  code that views don't need and shouldn't know about. It also makes the
  "UI must not be a truth source" rule harder to enforce when views can
  access services directly.

### No common layer — duplicate types across layers

Define equivalent types in both browser and UI layers, converting between
them at the boundary.

- Rejected: Duplicate types drift over time, causing bugs. Converting
  between equivalent structs adds boilerplate and overhead. This was the
  state before the common layer was introduced, and it created friction
  during sidebar and split view implementation.

### Common as part of ui/color or another sub-layer

Embed common types inside another layer (e.g., `astra/ui/color` owns
color-related constants).

- Rejected: Constants like sidebar dimensions and workspace types are
  used across multiple UI and non-UI layers. Tucking them into a UI
  subdirectory would be misleading and create unexpected dependencies.

### A single large header with all constants

Put all constants in one big `astra_constants.h` file instead of splitting
by domain.

- Rejected: As the number of constants grows, a single file becomes
  unwieldy. Splitting by domain (workspace, tab, command, UI) makes it
  easier to find what you need and reduces rebuild impact.

## References

- **Chromium subsystems reused:** `//base`, `//skia`, `//ui/gfx`
- **Astra components:** `astra/common/` layer with types, enums, constants
- **Build target:** `//astra/common` (`chromium/astra/common/BUILD.gn`)
- **Related ADRs:** ADR-0009 (Direct Chromium Architecture), ADR-0032
  (Projection Pattern)
