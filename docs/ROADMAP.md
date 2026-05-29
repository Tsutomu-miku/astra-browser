# Roadmap And Progress

## Purpose

This document turns the broad "Arc/Zen-like Chromium browser" goal into prioritized, reviewable requirements. It also records current progress and product invariants that must not be guessed from implementation details.

README stays short. `docs/REQUIREMENTS.md` remains the full product requirement list. This file is the working roadmap for priority, sequencing, and acceptance criteria.

## Review Findings

The recent Favorites issue exposed a requirements gap:

- "Favorite" was not explicitly defined as a tab-like sidebar folder, so implementation treated it like a bookmark/quick entry.
- Acceptance criteria described what click should not do, but not what object identity it should preserve.
- Drag-and-drop was specified as a broad behavior, but not split into drag source, drag payload, drop target, visual feedback, and state mutation requirements.
- The project had many implemented features, but no priority map showing which ones are core browser semantics versus polish.

Going forward, product requirements that touch tabs must specify:

- The backing data identity: tab id, URL entry, workspace id, group id, or closed-tab index.
- The default click behavior.
- Modifier behavior for preview and split.
- Drag source and drop target behavior.
- Whether the action creates, selects, moves, mutates, or restores a tab.
- Regression tests that prove the distinction.

## Product Invariants

- A tab is the primary browsing object in a Space. Selecting a tab must preserve its Chromium page state whenever possible.
- A Space owns local tabs, pinned tabs, tab groups, and Space-local Favorites.
- Favorites are a tab-like Space section, not just bookmarks. A Favorite item should preserve a relationship to a real tab when it came from a tab.
- Clicking a Favorite should select its matching tab. It must not replace the current tab's URL.
- If a legacy or imported Favorite has no matching tab, opening it may create or recover a tab, but still must not mutate the current active tab's URL.
- Essentials are global quick entries. They remain separate from Space-local Favorites until the product explicitly upgrades them to tab-backed global objects.
- Pinned, Favorites, groups, and regular tabs are sidebar folders for browsing objects. Drag-and-drop moves or classifies tabs between these folders.
- URL equality is only a fallback for legacy data. New tab-originated Favorites should carry tab identity.
- Command palette, sidebar, start page, shortcuts, and context menus must share the same product semantics for the same object.

## Priority Levels

- P0: Core browser semantics. Wrong behavior here makes the browser feel broken or data-unsafe.
- P1: Experience completeness. Important for daily use, but the app remains understandable without it.
- P2: Expansion and long-term platform work.

## P0 Requirements

### P0-1 Browser Shell And Persistence

- Provide a Chromium-backed Electron browser shell.
- Keep app, preload, renderer, domain, store, and surface boundaries clear.
- Persist normalized browser state locally.
- Restore a valid session with at least one Space and one tab.
- Keep active Space webviews mounted across ordinary tab switching.
- Never leave the browser with zero workspaces or zero tabs in a workspace.

Progress: mostly implemented.

Acceptance:

- `pnpm check` passes.
- Starting the app shows a usable browser shell.
- Switching tabs preserves page state unless the tab is explicitly sleeping.

### P0-2 Tab Identity And Sidebar Semantics

- Regular tabs, pinned tabs, grouped tabs, and Favorites must preserve tab identity.
- Clicking a regular tab, pinned tab, grouped tab, or Favorite selects the corresponding tab.
- Clicking a Favorite must not navigate or replace the currently active tab.
- Adding a tab to Favorites records the tab relationship.
- Removing a Favorite does not close the tab unless a separate close action is used.

Progress: partially implemented. Favorite items carry optional `tabId`, opening paths select matching tabs first, and tab-backed Favorites now render through the shared sidebar tab row path with tab actions, tab split behavior, tab status badges, tab close cleanup, tab drag payloads, and tab accessibility labels while staying in the Favorites folder. Tab-backed Favorites no longer use a separate Favorite drag payload; moving them across Spaces follows the normal tab move path and preserves their Favorites folder membership in the destination Space. Sidebar, Start page, omnibox, command palette, and sidebar quick-entry fallback split paths now keep tab-backed Favorites on tab identity. Full cross-surface QA is still needed.

Small requirements:

- P0-2.1 Add `tabId` to tab-originated Favorites.
- P0-2.2 Sidebar Favorite click selects tab.
- P0-2.3 Start page Favorite click selects tab.
- P0-2.4 Command palette Favorite run selects tab.
- P0-2.5 Context-menu Open for Favorites selects tab.
- P0-2.6 Legacy Favorites without `tabId` use URL fallback without replacing active tab.
- P0-2.7 Tab-backed Favorites use tab context-menu and close shortcuts; legacy URL Favorites keep quick-entry behavior.
- P0-2.8 Tab-backed Favorites expose tab status and accessible labels while remaining in the Favorites folder.
- P0-2.9 Tab-backed Favorites open split view by tab identity; legacy URL Favorites open split view by URL.
- P0-2.10 Closing a tab-backed Favorite removes it from the Favorites folder; legacy URL Favorites survive matching tab closes.
- P0-2.11 Tab-backed Favorites render through the shared sidebar tab row path instead of a separate quick-entry row implementation.
- P0-2.12 Tab-backed Favorites reorder as normal tab rows inside the Favorites folder; URL-only legacy Favorites stay on the quick-entry fallback path.

### P0-3 Sidebar Drag And Drop

- Dragging tab rows must work reliably from the visible row, not a hidden or fragile nested target.
- Dragging tabs must support reordering among regular tabs.
- Dragging tabs into Favorites must classify the tab as a Favorite without losing tab identity.
- Dragging tabs into Pinned must pin the tab.
- Dragging pinned tabs back to Tabs must unpin them.
- Dragging tabs to groups, Spaces, New Space, and split targets must use consistent payload recovery.
- Dragging tabs should not show explicit target-region overlays; destinations accept drops directly, while reordering shows local insertion position only.

Progress: partially implemented. Native drag payloads, insertion indicators, quiet tab-drag destinations, empty folder-header drops, whole-row tab drag sources, and a shared tab-folder move action now cover Tabs, Pinned, groups, and Favorites. The domain folder move action owns moving tabs into Favorites and moving Favorite-backed tabs back out to Tabs, Pinned, or groups, so the UI no longer manually removes Favorites before moving tabs. Tabs folder drops now accept the same real tab payload path as the other sidebar folders, and tab-backed Favorites use that same tab payload when reordering or moving across Spaces. URL-only legacy Favorites remain a quick-entry fallback and Space/New Space legacy favorite drops recover from native payloads when React drag state has not synced yet. Real Electron manual QA is still required.

Small requirements:

- P0-3.1 Visible tab row is the drag source.
- P0-3.2 Tab row stays the tab item drop target; the inner button is not a competing native drag source.
- P0-3.3 Drag payload is recoverable from native `DataTransfer` when React state is missing, including tab rows, pinned tabs, Favorites, and Essentials.
- P0-3.4 Drop on Favorites adds tab-backed Favorite.
- P0-3.5 Drop on existing Favorite area is accepted.
- P0-3.6 Reorder regular tabs by before/after placement.
- P0-3.7 Reorder pinned tabs by horizontal placement.
- P0-3.8 Drag state clears after drop, cancel, or native drag end.
- P0-3.9 Dragging tabs does not render separate target-region sections or New Group/Ungroup target buttons.
- P0-3.10 Dragging a Favorite-backed tab into Tabs, Pinned, or a group removes it from the Favorites folder.
- P0-3.11 Tabs, Pinned, Favorites, and tab groups share one folder move path instead of separate pin/unpin/favorite-removal/drop-intent branches.
- P0-3.12 Empty Tabs, Pinned, and Favorites folders stay visible as ordinary headers and accept tab drops without special target UI.
- P0-3.13 Tabs folder drops accept the same real tab payloads as other sidebar folders; regular tab ordering remains row-level before/after placement.
- P0-3.14 Space and New Space Favorite drops accept native Favorite payloads even when React drag state is missing.

### P0-4 Spaces And Profiles

- Spaces must switch reliably.
- Spaces must be reorderable.
- Tabs, groups, closed tabs, and Favorites must move across Spaces with explicit actions.
- Each Space must use a stable Chromium profile partition.
- Space homepage and profile metadata must persist.

Progress: mostly implemented.

### P0-5 Packaging Baseline

- macOS, Windows, and Linux package scripts must produce distributable artifacts.
- macOS x64 and arm64 artifacts must be generated separately, not as a universal package by default.
- GitHub Release publishing must be controlled by CI release workflow, not local package scripts requiring `GH_TOKEN`.
- Linux package metadata must include maintainer or author email.

Progress: mostly implemented.

### P0-6 Memory Safety Baseline

- Active tabs, pinned tabs, and split-view tabs must be protected from bulk sleep.
- Sleeping tabs must unload webviews and wake on selection.
- Store and webview lifecycle code must not call webview methods before readiness.

Progress: partially implemented.

## P1 Requirements

### P1-1 Arc/Zen Sidebar Experience

- Sidebar sections should visually align with Arc-style hierarchy: Essentials, Pinned, Favorites, groups, regular tabs, recently closed.
- Hover affordances must not cover row titles.
- Section collapse, search reveal, and drag reveal must feel predictable.
- Keyboard focus navigation must work across sections and context menus.

Progress: partially implemented. Row action hints reserve stable inline space, pinned tab hints reveal inside the icon tile, status badges, list controls, Space rail controls, sidebar chrome controls, and sidebar menu swatches avoid native title tooltips, hover/focus no longer overlays or squeezes row titles, and close controls reveal on keyboard focus without using absolute overlays.

### P1-2 Split View And Glance

- Split view supports horizontal, vertical, and grid layouts.
- Tabs, tab-backed Favorites, Essentials, history, and recently closed rows can be sent to split view without duplicating existing tab-backed pages.
- Glance previews can open, split, close, navigate, reload, and copy URL.

Progress: partially implemented. Sidebar, Start page, omnibox, command palette, and sidebar quick-entry fallback split paths now keep tab-backed Favorites on tab identity while URL-only entries still use URL split.

### P1-3 Command Palette And Omnibox

- Command palette, omnibox, sidebar search, and start search share object semantics.
- Open-tab entries select tabs.
- Favorite entries select matching tabs.
- Essentials navigate current tab unless later changed to tab-backed Essentials.
- History entries open a tab or preview/split based on modifier.

Progress: partially implemented.

### P1-4 Settings, Permissions, History, Downloads

- Settings should manage global and Space-scoped browser data.
- Site permissions must be profile-scoped.
- History, downloads, find, and zoom must cover daily browser workflows.

Progress: partially implemented.

### P1-5 Documentation And QA

- Requirements must include priority, object identity, default action, modifiers, drag behavior, and tests for new browser interactions.
- Every user-facing feature should update docs and add regression tests.
- Manual QA scenarios should exist for Electron-only behavior such as drag-and-drop and packaging.

Progress: started.

## P2 Requirements

- Native Chromium UI integration or lower-level Chromium embedding if Electron becomes limiting.
- Extension marketplace compatibility.
- Account sync.
- Cross-device profile sync.
- Advanced tab history across windows.
- Browser automation/export APIs.
- Plugin or theme ecosystem.
- Mobile support.

## Current Progress Summary

| Area | Status | Notes |
| --- | --- | --- |
| Electron Chromium shell | Mostly done | Electron shell, preload bridge, renderer surfaces exist. |
| Spaces | Mostly done | Creation, switching, reordering, profiles, settings are present. |
| Regular tabs | Mostly done | Lifecycle, selection, closing, duplication, sleeping, webviews are present. |
| Favorites as tabs | Partial | `tabId` exists, opening semantics are fixed, and Favorite-backed tabs are excluded from other sidebar tab folders. |
| Drag-and-drop | Partial | Workspace drag works; tab drag uses a native payload from the whole visible tab row; Favorite-backed drags can move back into tab folders; needs manual Electron QA. |
| Pinned tabs | Partial | Pin/unpin and drag behavior exist; should be reviewed with Favorite-as-tab model. |
| Tab groups | Partial | Grouping, context menus, drag targets exist; needs integrated DnD QA. |
| Essentials | Partial | Global quick entries exist; semantics intentionally remain URL-entry based for now. |
| Split and Glance | Partial | Core interactions exist; tab-backed Favorites use tab identity across sidebar, Start, omnibox, and command palette split paths; needs integrated QA. |
| Command palette | Partial | Broad command coverage exists; object semantics need continued alignment. |
| Memory management | Partial | Sleeping and Memory Saver exist; lifecycle QA should continue. |
| Packaging | Mostly done | Multi-platform scripts and release workflow exist; artifact size should keep being monitored. |
| Documentation | Partial | Requirements were broad; this roadmap is the new prioritization baseline. |

## Requirement Split Template

Use this shape for large user requests:

```text
ID:
Priority:
Object identity:
Default action:
Modifier actions:
Drag source:
Drop targets:
State mutation:
Visual feedback:
Accessibility:
Tests:
Manual QA:
```

Example:

```text
ID: P0-2.2 Sidebar Favorite click selects tab
Priority: P0
Object identity: Favorite.tabId, with URL fallback for legacy data
Default action: selectTab(tabId)
Modifier actions: Alt opens Glance, Shift opens split
Drag source: Favorite row for Favorite reordering or moving between Spaces
Drop targets: Favorites list, Space buttons, New Space, Split
State mutation: no tab URL mutation on default click
Visual feedback: current page state when active tab matches Favorite
Accessibility: label includes Favorite and current page state
Tests: component test, command/start/context menu parity tests
Manual QA: click Favorite while another tab is active and confirm active tab id changes rather than URL replacement
```
