# ADR-0025: Tab Stack / Tree Organization

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra supports tab stacking — a hierarchical organization feature where tabs can be grouped under a parent tab, forming a tree structure in the sidebar. This is similar to Arc Browser's tab stacking or Tree Style Tab extensions. The stack appears as a collapsible group in the sidebar, with child tabs nested under the parent.

Key architectural questions:
- Are stacks stored as metadata on tabs, or as a separate data model?
- How does this relate to Chromium's native tab groups feature?
- How does stacking interact with workspaces, favorites, and split view?
- Who owns the stack state — the sidebar UI or a service?

## Decision

Tab stacks are **metadata on tabs**, stored via `AstraTabFeatures` (`WebContentsUserData`). The `AstraTabStackService` provides helper methods for stack operations, but the truth source is per-tab metadata on `WebContents`. The sidebar projects the flat `TabStripModel` into a tree view based on stack metadata.

**Metadata model (AstraTabFeatures):**
- `is_in_stack` — true if this tab has a parent (is a child in a stack).
- `stack_parent_id` — opaque string ID of the parent tab. Empty if not stacked.
- `is_stack_collapsed` — whether the stack headed by this tab is collapsed. Only meaningful for parent tabs.
- Stack parent ID is stored as a string (opaque identifier) to avoid coupling to any specific tab handle type.

**Service layer (AstraTabStackService):**
- Profile-scoped helper service that provides stack operations:
  - `StackTab(parent_web_contents, child_web_contents)` — stack one tab onto another.
  - `UnstackTab(web_contents)` — remove a tab from its stack.
  - `GetStackChildren(parent_web_contents)` — get all child tabs of a parent.
  - `GetStackParent(child_web_contents)` — get the parent of a stacked tab.
  - `CollapseStack(parent_web_contents)` / `ExpandStack(parent_web_contents)` — toggle collapsed state.
- The service reads and writes `AstraTabFeatures` metadata; it does not own tab state.
- Stack operations do not reorder `TabStripModel` entries — stacking is purely metadata.

**UI projection (sidebar):**
- The sidebar reads stack metadata from `AstraTabFeatures` for all tabs in the active workspace.
- It projects the flat tab list into a tree structure with parent/child relationships.
- Collapsed stacks show only the parent tab; expanded stacks show parent + children.
- Drag-and-drop to stack/unstack dispatches commands that update `AstraTabFeatures` via the stack service.
- The sidebar is never the source of truth — it always reads from `TabStripModel` + `AstraTabFeatures`.

**Relationship to Chromium tab groups:**
- Chromium's native tab groups are flat (label/color based), not hierarchical.
- Astra stacks are hierarchical (tree/parent-child).
- Tab stacks and tab groups are independent features — a tab can be in both a stack and a group.
- `AstraTabStackService` does not use or depend on `TabGroupModel`.

**Persistence:**
- Stack metadata survives session restore via the same mechanism as other `AstraTabFeatures` metadata (session restore bridge).
- Stack parent IDs are stable across restarts because they are derived from tab identifiers that persist through session restore.

## Consequences

Positive:
- Builds on the existing `WebContentsUserData` pattern — no new ownership model.
- All Chromium tab features (session restore, extensions, DevTools, tab discarding) work unchanged.
- Stack state travels with the tab, not with a separate data store.
- The sidebar projection pattern is consistent with how favorites and workspaces work.
- Clean separation: service owns operations, UI owns presentation.

Negative:
- Building the tree projection requires iterating all tabs and building parent/child links. This is O(n) per update, acceptable for typical tab counts.
- Stack parent ID resolution requires mapping from string ID to `WebContents`, which needs an index or lookup table.
- Flat `TabStripModel` means extensions and Chrome UI see all tabs in a flat list, not the stacked tree. This is consistent with the overall sidebar-as-projection model.
- Drag-and-drop stacking requires coordinate transformation from sidebar visual tree order back to `TabStripModel` indices.

Neutral:
- Stacks are orthogonal to workspaces — a stack exists within a workspace, and switching workspace changes which stacks are visible.
- Stacks are orthogonal to favorites — a favorite tab can be a stack parent or child.

## Alternatives Considered

### Stacks as a separate service-owned data model
Build a dedicated `AstraTabStackService` that owns the stack tree structure, with its own persistence and observer pattern.

- Rejected: Duplicates the ownership pattern. Tabs are the truth source; stack metadata belongs on tabs. A separate data model would need to stay in sync with `TabStripModel` (tab add/remove/move), adding complexity without benefit. The current approach puts metadata where it belongs — on the `WebContents` — and uses the service only for operation logic.

### Reuse Chromium tab groups with sidebar tree projection
Map hierarchical stacks onto Chromium's flat tab groups, with the sidebar projecting a tree structure.

- Considered: Tab groups are a Chromium-native feature with their own UI and persistence. However, tab groups are flat (one level of grouping), not hierarchical. Emulating a tree on top of flat groups would be awkward and would not match the user's mental model. Also, tab groups have their own visual presentation in the horizontal tab strip, which would conflict with Astra's sidebar-only model.

### Sidebar-owned stack state
Let the sidebar view own the stack tree structure directly, with its own data model.

- Rejected: Violates the principle that UI is never the source of truth. If the sidebar is ever hidden, replaced, or recreated, stack state would be lost. Also, other features (command palette, keyboard shortcuts, workspace import/export) would not be able to access stack state.

### Browser-owned stacks (like Arc on macOS)
Use separate `Browser` windows or Cocoa window layering to implement stacks, similar to how Arc Browser handles tab stacks on macOS.

- Rejected: Tab reparenting between `Browser` instances is fragile in Chromium and breaks session state, DevTools attachment, and extension tab IDs. The metadata projection approach is simpler and more correct.

## References

- **Chromium subsystems reused:** `TabStripModel`, `content::WebContentsUserData`, `WebContents`, session restore
- **Astra components:** `AstraTabStackService`, `AstraTabFeatures` (stack metadata), `AstraSidebarView` (tree projection), `AstraSidebarStackHeaderView`, `AstraSidebarStackChildView`
- **Patch points:** Session restore metadata (same as other AstraTabFeatures data), profile keyed service registration
- **Related ADRs:** ADR-0010 (Workspace as Metadata Projection), ADR-0011 (Sidebar Projection Model), ADR-0014 (Favorite Folders as Tab Metadata)
