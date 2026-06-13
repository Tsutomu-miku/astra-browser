# ADR-0032: Projection Pattern

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra is a Chromium overlay product. Chromium owns all core browser state:
tab ownership, profiles, navigation, history, downloads, passwords,
extensions, DevTools, and more. Astra adds product semantics on top —
workspaces, favorites, sidebar projection, split view metadata, glance,
focus mode — without replacing any Chromium subsystem.

This creates a fundamental architectural pattern: Astra UI and services
**project** Chromium state through the lens of Astra product metadata.
This pattern appears repeatedly across the codebase:

- The sidebar projects `TabStripModel` filtered by workspace and
  decorated with favorite/pinned/split metadata.
- Workspace switching is a projection change, not a tab ownership change.
- Split view projects two `WebContents` side-by-side without reparenting.
- The command palette projects Chrome commands + Astra commands into a
  single searchable list.
- DevTools panels project Astra service state for inspection.

This ADR formalizes the projection pattern as the core architectural
principle that unifies all Astra UI and service design.

## Decision

Astra uses the **projection pattern** as its primary architectural
approach. UI layers and presentation services project Chromium and Astra
state into product-specific views. No UI code or presentation layer is
ever a source of truth.

### The Three Layers

The projection pattern consists of three layers, with strict dependency
direction from bottom to top:

```
Layer 3: Presentation / UI (projection)
  Views, sidebar, split view, glance, command palette, DevTools panels
  - Reads from truth sources
  - Renders / projects state
  - Dispatches commands
  - Never owns state

Layer 2: Astra Product Metadata (truth)
  ProfileKeyedService implementations, WebContentsUserData
  - WorkspaceService, FavoriteService, TabFeatures, FocusModeService
  - Owns Astra-specific product state
  - Persists to prefs / user data
  - Provides observer interfaces

Layer 1: Chromium Subsystems (truth)
  TabStripModel, WebContents, Profile, extensions, history, etc.
  - Owns all core browser state
  - Provides observer patterns for change notification
  - Not modified by Astra (except tiny patch points)
```

**Dependency direction:** Layer 3 depends on layers 2 and 1. Layer 2
depends on layer 1. Layer 1 never depends on layers 2 or 3.

### Observer Pattern for Reactive Updates

All truth sources (both Chromium and Astra) provide observer interfaces.
Projection layers subscribe to observers and refresh their projection
when underlying state changes.

Examples:

- `TabStripModelObserver` — sidebar refreshes when tabs are added/removed/moved.
- `AstraWorkspaceService::Observer` — sidebar refreshes when workspace
  list or active workspace changes.
- `views::View::OnThemeChanged()` — views repaint when color provider
  (theme) changes.

The update flow is:

```
State change in truth source
    |
    v
Observer notification
    |
    v
Projection layer rebuilds / patches its view
    |
    v
UI reflects new state
```

Projection layers never mutate state directly to "stay in sync" — they
always react to observer notifications from the truth source.

### Command Dispatch for Mutations

Projection layers (UI) never mutate state directly. When a user action
should change state, the UI dispatches a command that flows down to the
appropriate truth source:

```
User action (click, keystroke, drag)
    |
    v
UI view captures input
    |
    v
Command dispatch (Chrome command or Astra command)
    |
    v
Truth source applies change
    |
    v
Observer notification fires
    |
    v
UI projection updates (reactively)
```

This ensures a unidirectional data flow: state changes always originate
from truth sources and propagate upward to projections via observers.
UI never mutates state directly and then reflects its own mutation —
it always reflects what the truth source tells it.

### Projection Rules

Every projection layer must follow these rules:

1. **No state ownership.** A projection may cache data for rendering
   efficiency, but it must never be the authoritative source of that data.
   If the projection were destroyed and recreated, it should look identical.

2. **Read-only access to truth sources.** Projections read from truth
   sources but only modify state through well-defined command / mutation
   APIs.

3. **Reactive to observers.** Projections update in response to observer
   notifications, not by polling or by mutating themselves after commands.

4. **Pure function of inputs.** Given the same truth source state, a
   projection always produces the same visual output. No hidden state.

5. **Single source per data field.** Each piece of data displayed in a
   projection comes from exactly one truth source. If the same data
   appears in multiple projections, they all read from the same source.

### Common Projection Patterns in Astra

| Projection | Truth Sources | Pattern |
|------------|--------------|---------|
| Sidebar tab list | `TabStripModel` + `AstraTabFeatures` + `AstraWorkspaceService` | Filter + decorate |
| Workspace switcher | `AstraWorkspaceService` | List projection |
| Split view | `TabStripModel` (two WebContents) + `AstraTabFeatures` (split state) | Layout projection |
| Glance / peek | `WebContents` + `AstraTabFeatures` | Preview projection |
| Command palette | `BrowserCommandController` + `AstraCommandDelegate` | Unified search projection |
| DevTools workspace panel | `AstraWorkspaceService` + `AstraTabFeatures` | Debug projection |
| Focus mode indicator | `AstraFocusModeService` | Status projection |

## Consequences

Positive:

- **Single source of truth.** No dual-state bugs. There is never a question
  of "which model is correct" — the Chromium or Astra service is always
  the authority.
- **Extensibility.** New projections can be added without touching truth
  sources. New features can compose existing projections in new ways.
- **Testability.** Truth sources can be tested independently of UI.
  Projections can be tested with mock truth sources.
- **Resilience.** UI crashes or bugs cannot corrupt state. If a projection
  breaks, the underlying data is safe.
- **Consistency.** Every Astra UI surface follows the same pattern, making
  the codebase predictable and easy to navigate.
- **Chromium-aligned.** This pattern matches how Chromium itself is
  structured: models own state, views project state, commands flow down.
  Astra's pattern is a natural extension of Chromium's MVC-like approach.

Negative:

- **More layers.** There is an indirection between state and presentation.
  Developers need to understand the pattern to trace from UI back to
  the truth source.
- **Observer boilerplate.** Each projection needs observer registration,
  notification handlers, and refresh logic. This adds some boilerplate
  compared to direct state mutation.
- **Performance considerations.** Rebuilding a projection on every state
  change can be expensive for large datasets (e.g., hundreds of tabs).
  In practice, tab counts are small enough that O(n) rebuilds are fine,
  but this requires vigilance.
- **Coordination across projections.** When multiple projections display
  related data, they all update independently via observers. This is
  correct but may cause multiple repaints for a single state change.

Neutral:

- The pattern is not a formal framework or base class. It is a
  convention that all Astra code follows, enforced by code review and
  architecture checks. There is no `AstraProjectionView` base class.
- Some projections are pure UI (sidebar view), while others are
  service-level projections (e.g., `AstraDevToolsHelper` as a projection
  of DevTools state). The pattern applies at multiple levels.

## Alternatives Considered

### MVVM (Model-View-ViewModel)

Formalize the projection layer as ViewModels that sit between models
and views, with explicit binding.

- Considered: MVVM is a well-understood pattern. However, Chromium's
  Views framework does not use MVVM natively, and adding it would be
  a significant departure from Chromium patterns. The projection pattern
  is lighter-weight and more aligned with how Chromium code is structured
  (views observe models directly).

### MVP (Model-View-Presenter)

Use presenter classes that mediate between models and views.

- Considered: Similar to MVVM above. The coordinator pattern used in
  Astra (e.g., `AstraDevToolsIntegration`) is a lightweight form of
  presenter, but we have not formalized it as a universal pattern.
  Adding formal presenters for every projection would increase
  boilerplate without proportional benefit.

### Direct state ownership by UI

Let UI components own state and mutate it directly, with services
reading from UI.

- Rejected: Violates the fundamental architecture principle that UI
  must not be a source of truth. Creates dual-state bugs, makes testing
  harder, and diverges from Chromium patterns. This was explicitly
  rejected in earlier ADRs (ADR-0010, ADR-0011).

### Unidirectional data flow / Flux / Redux

Use a central store with actions and reducers, with views subscribing
to store changes.

- Rejected: Over-engineered for Astra's needs. Chromium already has
  its own model-observer pattern that works well. Adding a Flux-like
  architecture would be a large departure from Chromium conventions
  and would not integrate naturally with existing Chromium services.
  The observer-based projection pattern achieves similar unidirectional
  flow within Chromium's existing framework.

## References

- **Chromium subsystems reused:** `TabStripModelObserver`,
  `content::WebContentsUserData`, `ProfileKeyedService`,
  `views::View::OnThemeChanged()`, observer pattern throughout Chromium
- **Astra components:** Sidebar projection, workspace projection, split
  view projection, command palette projection, DevTools panel projection
- **Source locations:** `chromium/astra/browser/` (truth sources),
  `chromium/astra/ui/views/` (projection layers),
  `chromium/astra/common/` (shared types for projections)
- **Related ADRs:** ADR-0010 (Workspace as Metadata Projection),
  ADR-0011 (Sidebar Projection Model), ADR-0013 (Split View Architecture),
  ADR-0029 (Common Layer)
