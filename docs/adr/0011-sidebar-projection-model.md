# ADR-0011: Sidebar Projection Model

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra features a vertical tab sidebar similar to Arc Browser and Zen Browser.
The sidebar shows tabs grouped by workspace, with favorites and pinned sections.
The key architectural question is whether the sidebar owns its own tab model or
projects data from Chromium's existing `TabStripModel`.

The sidebar is the primary tab management surface in Astra. It must support:
- Tab ordering, grouping, and drag-reorder
- Favorite/pinned sections
- Workspace filtering
- Tab state display (loading, muted, crashed, favicon)
- Tab commands (close, reload, new tab, duplicate, etc.)

## Decision

The sidebar is a **projection view**, not a tab model owner. It reads state
from three sources and dispatches commands back to Chromium's command
infrastructure:

**Data sources (read-only):**

1. **`TabStripModel`** -- tab order, count, `WebContents` references, selection
   state, pinned state, group membership.
2. **`AstraTabFeatures`** (`WebContentsUserData`) -- Astra metadata per tab:
   workspace ID, favorite folder membership, split/glance presentation flags.
3. **`AstraWorkspaceService`** (`ProfileKeyedService`) -- workspace list,
   active workspace, workspace ordering.

**Command dispatch:**

- Standard tab operations (new tab, close tab, reload, pin, mute, duplicate)
  are dispatched through `chrome::ExecuteCommand` on the `Browser` or directly
  to `TabStripModel` methods.
- Astra-specific sidebar commands (move to workspace, add to favorites, toggle
  split view) go through `AstraCommandDelegate`.
- The sidebar never directly mutates its own data model to reflect state
  changes; it reacts to observation notifications from `TabStripModelObserver`
  and `AstraWorkspaceService` observer.

**Reactive updates:**

- The sidebar implements `TabStripModelObserver` to receive tab insertion,
  removal, move, and metadata change notifications.
- On each change, the sidebar rebuilds or patches its projection based on the
  current workspace filter and Astra metadata.

## Consequences

Positive:

- Single source of truth: `TabStripModel` owns tab state. No risk of sidebar
  state drifting from Chromium state.
- All Chromium tab features (session restore, extensions, DevTools, tab
  discarding, prerendering) work without adaptation.
- The sidebar is trivially replacable or restylable; it contains no business
  logic.
- Sidebar crashes or bugs cannot corrupt tab state.

Negative:

- Every sidebar update must recompute the projection from `TabStripModel` +
  Astra metadata. This is O(n) per change, acceptable for typical tab counts.
- Drag and drop must be mapped from sidebar visual order back to
  `TabStripModel` indices accounting for the workspace filter. This is a
  coordinate transformation, not a state ownership concern.
- The standard Chromium horizontal tab strip must be hidden or styled away
  since the sidebar is the primary surface; the model underneath is shared.

## Alternatives Considered

### Custom tab model owned by sidebar
The sidebar maintains its own `AstraTabModel` with its own tab ordering, and
drives `TabStripModel` as a side effect.

- Rejected: Dual state. Both models would need to stay in sync, and the
  question of which is authoritative creates bugs. Extensions and Chrome UI
  surfaces that read `TabStripModel` directly would bypass sidebar state.

### Side panel WebUI
Implement the sidebar as a Chrome side panel WebUI (HTML/CSS/JS) that
communicates with a browser-side bridge.

- Rejected: WebUI side panels are designed for secondary content (bookmarks,
  reading list), not as the primary tab management surface. Performance for
  frequent tab updates and drag interactions would be inferior to Views. The
  primary browser chrome should use Views, matching Chromium desktop
  architecture.

### Chrome Side Panel API
Use the extensions Side Panel API to host the sidebar.

- Rejected: The Side Panel API is an extension surface, not a product browser
  UI surface. It would require bundling a built-in extension and going through
  extension IPC for every tab update. Wrong layer for core browser UI.
