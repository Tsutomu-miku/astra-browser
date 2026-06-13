# ADR-0010: Workspace as Metadata Projection

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra offers Arc-style Spaces (workspaces) as a core product feature. Users can
organize tabs into named workspaces and switch between them. The question is how
to implement this on top of Chromium without reimplementing tab ownership or
session management.

Chromium owns `TabStripModel`, `WebContents` lifecycle, session restore, and tab
navigation. Any workspace implementation that reparents tabs or manages its own
tab collection would duplicate Chromium infrastructure and diverge from session
restore, crash recovery, and extension APIs.

Several approaches exist in Chromium-adjacent products: separate `Browser`
windows per workspace (Arc on macOS uses this pattern via NSWindow), tab group
metadata within a single `TabStripModel`, or full custom tab models.

## Decision

Workspaces (Spaces) are **metadata projections** over Chromium's `TabStripModel`,
not separate tab containers.

- `AstraWorkspaceService` is a `ProfileKeyedService` that owns workspace
  metadata: workspace list, order, active workspace, display name, accent color.
  It does not own any tabs.
- Each `content::WebContents` carries an `AstraTabFeatures`
  (`WebContentsUserData`) object that stores a `workspace_id` string and other
  Astra-only tab metadata.
- The sidebar and tab strip UI project the current workspace by filtering
  `TabStripModel` entries whose `workspace_id` matches the active workspace.
- Switching workspace is a **UI projection change**: the active workspace ID
  changes in `AstraWorkspaceService`, and the sidebar/tab strip re-render to
  show only the tabs in that workspace. All `WebContents` remain alive in
  `TabStripModel`.
- Session restore uses Chromium's native session service. Astra workspace
  metadata is persisted alongside via profile prefs or `TabStripModel` group
  metadata, and re-applied after session restore.

No tab reparenting, no moving `WebContents` between models, no separate
`Browser` instances per workspace.

## Consequences

Positive:

- Full reuse of Chromium `TabStripModel`, session restore, crash recovery, and
  tab lifecycle management.
- Extensions and WebUI that enumerate tabs see all tabs, matching Chromium
  semantics.
- Workspace switching is cheap: no `WebContents` teardown or re-creation.
- Metadata-only changes are easy to test and reason about.
- Favorites, split view, and glance features can compose on the same metadata
  model.

Negative:

- Memory scales with total tabs across all workspaces, not just the active one.
  This is a deliberate tradeoff for simplicity and correctness; if memory
  becomes a concern, tab discarding (a Chromium feature) can be tuned.
- The top `TabStripModel` still shows all tabs; Astra hides it in favor of the
  sidebar projection. The model itself remains the truth source.
- Extensions that expect visible tabs to equal the full tab strip may show
  unexpected entries; this is mitigated by the Astra UI being the primary
  surface.

## Alternatives Considered

### Separate Browser per workspace
Each workspace gets its own `Browser` and `TabStripModel`, with `WebContents`
moved between them on switch.

- Rejected: Tab reparenting is fragile in Chromium. Moving `WebContents` between
  `Browser` instances breaks session state, devtools attachment, and extension
  tab IDs. Arc on macOS achieves this via Cocoa window layering, not Chromium
  tab reparenting.

### TabStripModel grouping (tab groups feature)
Use Chromium's native tab groups feature, where each workspace is a group.

- Rejected: Tab groups are designed for in-strip grouping, not workspace-scale
  separation. Only one group set exists per strip, and the UI is horizontal-tab
  oriented. Workspaces have different semantics (full-window projection,
  independent naming, color theming). However, tab group metadata storage may be
  a useful persistence mechanism for Astra workspace IDs.

### Custom tab model
Build a parallel `AstraTabModel` that owns `WebContents` and drives its own
strip.

- Rejected: Duplicates `TabStripModel` entirely. Session restore, extension
  APIs, DevTools, and all tab-related Chromium features would need re-adapters.
  This violates the prime directive of reusing Chromium.
