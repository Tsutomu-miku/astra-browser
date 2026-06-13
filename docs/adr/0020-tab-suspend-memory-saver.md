# ADR-0020: Tab Suspend / Memory Saver

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Browsers with many tabs consume significant memory. Chromium already has a tab discard system (`resource_coordinator::TabManager`) that automatically discards inactive tabs when system memory is low. Astra can leverage this system but needs to add Astra-specific policy and UI.

Key questions:
- Does Astra implement its own tab suspend, or reuse Chromium's?
- What policy determines which tabs to suspend (Astra-specific criteria)?
- How is suspended state reflected in the Astra sidebar?
- How does this interact with workspaces and favorite tabs?

## Decision

Astra builds on **Chromium's existing tab discard mechanism** (`resource_coordinator::TabManager`) with an Astra policy layer (`AstraMemorySaverService`) that adds Astra-specific rules and UI integration.

**Chromium subsystems reused:**
- `resource_coordinator::TabManager` -- the core discard engine that decides when and which tabs to discard based on memory pressure, tab activity, and tab importance.
- `TabStripModel` -- tab state and discard status.
- `LifecycleUnit` / `LifecycleManager` -- the underlying lifecycle framework.
- Chrome's "Memory Saver" mode UI in settings and tab strip (where applicable).

**AstraMemorySaverService:**
- Profile-scoped service that configures Chromium's tab discard with Astra-specific policy.
- Adds Astra-specific heuristics:
  - **Workspace-aware discard:** Tabs in inactive workspaces are eligible for earlier discard (lower importance score).
  - **Favorite protection:** Favorite tabs have higher discard priority (less likely to be discarded) because they are user-curated.
  - **Split view protection:** Both tabs in an active split view are treated as active and are not discarded.
  - **Glance tab handling:** URL glance temporary tabs are immediately eligible for discard since they are ephemeral.
- Provides API to query memory state: suspended tab count, memory savings estimate.
- Observer pattern for UI updates.

**Policy integration:**
- Astra service registers with the tab manager as a policy provider or observer.
- Adjusts tab importance scores based on Astra metadata (workspace active state, favorite status, split view state).
- Does not replace the discard algorithm -- only influences scoring.
- Patch point: `resource_coordinator::TabManager` or `TabLifecycleUnit` to hook in Astra policy.

**UI integration:**
- Sidebar shows a "suspended" indicator (e.g., faded icon) for discarded tabs.
- Sidebar re-reads discard state from `TabStripModel` / `WebContents` rather than maintaining its own state.
- Hovering a suspended tab shows a tooltip explaining that the tab is suspended and will reload on activation.
- A "memory saver" status indicator in the toolbar shows savings and allows toggling the feature.

**Tab reactivation:**
- Clicking a suspended tab in the sidebar reloads it -- standard Chromium behavior.
- No special Astra reactivation logic needed; it just works through `TabStripModel`.
- Astra metadata (workspace, favorite, split) is preserved across discard because it lives on `AstraTabFeatures` which persists through session/restore cycle.

## Consequences

Positive:

- Leverages Chromium's mature, tested discard engine instead of building a new one.
- Chromium's discard already handles all the hard parts: memory pressure detection, renderer process lifecycle, tab state serialization, reload on activation.
- Astra policy layer is thin -- it only adjusts priorities, not the core algorithm.
- Discard state is in Chromium's `TabStripModel`, so the sidebar projects it like any other tab state.
- All Chromium tab features (session restore, DevTools, extensions) work with discarded tabs.

Negative:

- Astra policy injection requires a patch to `resource_coordinator::TabManager` or a related component. The exact patch point depends on how tab importance scoring works in the current Chromium version.
- Memory savings estimates are approximate; exact byte counts depend on Chromium internals.
- Favorite and workspace-aware discard policy adds some complexity to the discard decision path.
- If the tab manager API changes significantly between Chromium versions, the Astra patch may need rework.

Neutral:

- The feature is opt-in (can be enabled/disabled in settings), consistent with Chrome's Memory Saver mode.
- Astra does not add a separate "suspend tab now" user action by default; discard is automatic. Manual suspend can be added later.

## Alternatives Considered

### Full Astra tab suspend implementation
Build a complete tab suspend system in Astra code that hibernates tabs to disk.

- Rejected: Massive duplication of Chromium's discard system. Chromium's tab manager already handles process lifecycle, memory pressure, coordination with system resource managers (e.g., Android's LMK), and cross-platform considerations. Reimplementing this would be a huge effort and likely less reliable.

### Workspace-level hibernation
Suspend entire workspaces as a unit (e.g., "hibernate Work workspace").

- Considered as a future enhancement. The current approach (individual tab discard with workspace-aware policy) is simpler and builds on existing Chromium mechanisms. Workspace-level hibernation can be added later as a higher-level feature that triggers discard for all tabs in a workspace.

### Browser-level "sleeping tabs" like Edge
Implement a "sleeping tabs" feature similar to Microsoft Edge.

- Rejected: Chromium's tab discard is essentially this feature already. Edge's sleeping tabs are built on the same Chromium discard infrastructure. Astra should use the same underlying mechanism rather than building a parallel system.

## References

- **Chromium subsystems reused:** `resource_coordinator::TabManager`, `LifecycleUnit` / `LifecycleManager`, `TabStripModel`, `WebContents::WasDiscarded()`, Memory Saver settings
- **Astra components:** `AstraMemorySaverService` (planned), `AstraTabFeatures` (metadata for policy), `AstraSidebarView` (UI projection)
- **Patch points:** Tab manager policy hook (`chrome/browser/resource_coordinator/tab_manager.h` or `components/performance_manager/`), profile keyed service registration
