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

## Execution Policy

Work should move by priority bands, not by whichever small bug is easiest to isolate.

1. Finish P0 requirements until the browser's core object model, drag/drop semantics, memory safety, persistence, and packaging are reliable enough for daily dogfooding.
2. Then focus P1 requirements in large experience batches: Arc/Zen sidebar polish, iconography, interaction states, Split/Glance workflows, command/omnibox parity, and daily-use settings/history/downloads.
3. Defer P2 work until the active P0/P1 batch has no known blocking gaps. P2 items should be fixed in batches, not as one-off distractions.

When a P0/P1 implementation reveals architecture that cannot support the target behavior cleanly, prefer a contained refactor over stacking compatibility branches.

## Current Execution Batches

### Batch 1: P0 Stabilization

Status: active.

- P0-2 Tab identity and cross-surface Favorite semantics.
- P0-3 Sidebar drag-and-drop semantics and Electron manual QA.
- P0-6 Memory safety baseline, especially webview sleep/wake lifecycle and protected tabs.
- P0-5 Packaging baseline, including per-arch macOS/Windows artifacts and Linux metadata.

Exit criteria:

- Full automated suite and production build pass.
- Manual Electron QA covers tab reorder, folder moves, Space/New Space drops, Split drops, sleep/wake, and package artifact smoke checks.
- No known P0 behavior requires a user workaround.

### Batch 2: P1 Arc/Zen Experience

Status: next.

- Sidebar visual hierarchy: spacing, density, section rhythm, quiet active state, restrained typography, configurable chrome accent, collapsed state, and recently closed placement.
- Iconography: real site favicons for tabs where available, consistent symbolic icons for actions and status, clear favicon fallbacks, no placeholder-looking controls in primary chrome.
- Interaction states: hover, pressed, focus-visible, drag, split-target, sleeping, muted, active, search-selected, and disabled states must feel deliberate, quiet, and not cover row titles.
- Compact chrome: floating sidebar/topbar reveal, pin/unpin controls, address field behavior, and content-first layout.
- Split and Glance: visible affordances, no duplicate tab-backed splits, predictable pane focus, and polished controls.
- Command palette, omnibox, sidebar search, and start page: same object semantics and modifier hints.

Exit criteria:

- Sidebar can be scanned and operated without native tooltips, overlapping hints, or ambiguous icons.
- Keyboard and pointer interactions expose the same primary, preview, and split actions.
- Targeted visual/state tests cover the main sidebar controls and row variants.

### Batch 3: P1 Daily Browser Completeness

Status: queued.

- Settings, permissions, history, downloads, find, zoom, and browsing data flows.
- Profile-scoped permission/storage clarity.
- Command palette coverage for daily browser operations.

### Batch 4: P2 Expansion

Status: deferred.

- Native Chromium integration, extensions, sync, advanced history, browser automation APIs, plugin/theme ecosystem, and mobile support.

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

Progress: partially implemented. Favorite items carry optional `tabId`, opening paths select matching tabs first, and tab-backed Favorites now render through the shared sidebar tab row path with tab actions, tab split behavior, tab status badges, tab close cleanup, tab drag payloads, and tab accessibility labels while staying in the Favorites folder. Tab-backed Favorites no longer use or synthesize a separate Favorite drag payload; moving them across folders or Spaces follows the normal tab move path and preserves their Favorites folder membership when appropriate. Sidebar, Start page, omnibox, command palette, keyboard number shortcuts, and sidebar quick-entry fallback split paths now keep tab-backed Favorites on tab identity. Full cross-surface QA is still needed.

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

Progress: partially implemented. Native drag payloads, insertion indicators, quiet tab-drag destinations, empty folder-header drops, whole-row tab drag sources, and a shared tab-folder move action now cover Tabs, Pinned, groups, and Favorites. The domain folder move action owns moving tabs into Favorites and moving Favorite-backed tabs back out to Tabs, Pinned, or groups, so the UI no longer manually removes Favorites before moving tabs. Sidebar folder drops for Tabs, Pinned, and Favorites now share one tab-folder handler instead of branching through quick-entry Favorite logic. Tabs folder drops now accept the same real tab payload path as the other sidebar folders, and tab-backed Favorites use that same tab payload when reordering or moving across Spaces. Space rail buttons, New Space, and Split now expose tab drags as drop-target state instead of only accepting the drop silently. Pinned tab rows and tab group headers now ignore unrelated payloads instead of swallowing them as tab drops, and tab group reorder recovers from native group payloads when React drag state has not synced yet. Drag source resolution is centralized in `sidebarDragSources` so Space, New Space, Split, tab folders, groups, Essentials, and legacy Favorites read native payloads through one model instead of duplicating `text/*` keys. Space and New Space drops now derive a single drop intent before dispatching, so legacy Favorite moves are no longer a separate wrapper path around tab/group/closed-tab handling. URL-only legacy Favorites remain a quick-entry fallback and Space/New Space legacy favorite drops recover from native payloads when React drag state has not synced yet. Real Electron manual QA is still required.

Small requirements:

- P0-3.1 Visible tab row is the drag source.
- P0-3.2 Tab row stays the tab item drop target; the inner button is not a competing native drag source.
- P0-3.3 Drag payload is recoverable from native `DataTransfer` when React state is missing, including tab rows, pinned tabs, tab groups, Favorites, and Essentials.
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
- P0-3.15 Sidebar folder drops for Tabs, Pinned, and Favorites are handled as the same tab-folder operation; quick-entry drag code must not synthesize tab payloads for Favorites.
- P0-3.16 Space rail buttons and New Space expose drop-target state for tab drags as well as groups, closed tabs, and legacy Favorites.
- P0-3.17 Sidebar drop targets read tab, group, Essential, Favorite, closed-tab, and workspace payload identities through a shared source model rather than hard-coded `DataTransfer` keys in components.
- P0-3.18 Space and New Space drops derive a shared intent before dispatch so tab, group, closed-tab, workspace, and legacy Favorite moves are not split across competing component branches.

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
- Windows x64 and arm64 artifacts must be generated separately, not from one mixed-architecture builder invocation.
- GitHub Release publishing must be controlled by CI release workflow, not local package scripts requiring `GH_TOKEN`.
- Linux package metadata must include maintainer or author email.

Progress: mostly implemented. macOS and Windows now expose per-architecture local package scripts and CI release matrices, with all-architecture commands invoking x64 and arm64 builds separately.

### P0-6 Memory Safety Baseline

- Active tabs, pinned tabs, and split-view tabs must be protected from bulk sleep.
- Sleeping tabs must unload webviews and wake on selection.
- Store and webview lifecycle code must not call webview methods before readiness.

Progress: partially implemented. Memory Saver sleep protection is now centralized in a shared tab sleep policy used by domain actions, settings/sidebar summaries, and tab-group context-menu availability. Manual inactive sleep, automatic idle sleep, and group sleep all use the same releasable-tab definition, and no-op sleep requests now preserve the existing state object when every candidate is protected or already sleeping.

Small requirements:

- P0-6.1 Active, pinned, and split-view tabs share one protected-tab policy for Memory Saver.
- P0-6.2 Manual inactive sleep and automatic idle sleep use the same releasable-tab policy.
- P0-6.3 Tab-group sleep uses the same releasable-tab policy and remains a no-op when all group tabs are protected.
- P0-6.4 Settings/sidebar Memory Saver counts are derived from the same policy as the sleep actions.
- P0-6.5 Sleeping a tab clears loading and navigation affordances before the webview is released.
- P0-6.6 Sleeping tabs wake on tab selection and split opening.
- P0-6.7 Webview lifecycle calls are gated by readiness and Electron manual QA.

## P1 Requirements

### P1-1 Arc/Zen Sidebar Experience

- Sidebar sections should visually align with Arc-style hierarchy: Essentials, Pinned, Favorites, groups, regular tabs, recently closed, with restrained font weights.
- Sidebar iconography should use real site favicons for tabs where available, consistent symbolic action/status icons, clear favicon fallbacks, and restrained primary chrome controls.
- Hover affordances must not cover row titles.
- Hover, pressed, focus-visible, active, selected, drag, drop-target, sleeping, muted, split, and disabled states must be visually distinct and quiet, without border-heavy active rows.
- Section collapse, search reveal, and drag reveal must feel predictable.
- Keyboard focus navigation must work across sections and context menus.

Progress: partially implemented. Row action hints reserve stable inline space, pinned tab hints reveal inside the icon tile, status badges, list controls, Space rail controls, sidebar chrome controls, and sidebar menu swatches avoid native title tooltips, hover/focus no longer overlays or squeezes row titles, and close controls reveal on keyboard focus without using absolute overlays. Collapsed sidebar sections now stay collapsed during tab, Essential, and Favorite drags; search filtering is the only automatic section reveal path. Current tab folders now render before Recently Closed so the sidebar scans as Essentials, Pinned, Favorites, Tabs, then recovery. Long sidebar folders auto-scroll near the top and bottom edges while dragging tabs, quick entries, groups, or recently closed rows, and keyboard focus navigation scrolls the newly focused row into view. Sidebar context-menu keyboard navigation also keeps newly focused menu actions visible. Tab group keyboard context menus open from the group toggle while group-name editing fields keep text-editing context menu keys. Essential context-menu Open now navigates the active tab instead of selecting or creating a tab. Tab rows, pinned tabs, and recently closed tabs now persist Electron page favicons when available and share a sidebar icon renderer with internal-page, local-file, web, unknown-page, loading, and sleeping fallbacks. Favicons are cached by site origin so same-site tabs, Favorites, and recently closed rows can reuse known icons before the webview emits a fresh favicon event. Active and selected sidebar states now use neutral row fills and restrained text weight instead of accent-tinted borders, heavy type, or multi-layer inset shadows, with the global chrome background reduced to a quieter neutral palette. Global settings now expose a chrome accent mode so the default UI can stay neutral while users can opt into matching the current Space accent.

Small requirements:

- P1-1.1 Sidebar section rhythm, density, and typography align with Arc/Zen-style vertical browsing.
- P1-1.2 Primary sidebar and footer icons are consistent, discoverable, and do not rely on text labels inside cramped controls.
- P1-1.3 Real site favicons and fallback icons distinguish tabs, pinned tabs, Essentials, Favorites, groups, recently closed rows, and internal pages.
- P1-1.4 Row action hints reserve space and never obscure title text.
- P1-1.5 Active, search-selected, split, muted, sleeping, hover, focus, pressed, disabled, dragging, and drop-target states are visually distinct without border-heavy active rows.
- P1-1.6 Collapsed and compact sidebar reveal behavior is predictable and content-first.
- P1-1.7 Sidebar visual QA includes desktop and narrow widths for text fitting and icon clarity.
- P1-1.8 Chrome accent defaults to neutral and can optionally follow the active Space accent.

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
