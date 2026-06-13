# Technical Debt

This document catalogs technical debt across the Astra browser codebase, sorted by
severity. Each item includes description, impact, recommended fix, related files,
and estimated effort.

---

## Critical (would cause compile/link errors)

### TD-001: No verified Chromium build

**Description:** The entire `//astra` codebase has never been compiled inside a real
Chromium checkout. All BUILD.gn files contain TODOs noting that deps need `gn check`
verification. There may be missing includes, wrong API signatures, type mismatches,
or link errors throughout.

**Impact:** The entire codebase is unverified. Any assumption about API compatibility
with Chromium could be wrong. First integration attempt could reveal days or weeks
of compilation errors.

**Recommended fix:**
1. Bootstrap a Chromium checkout with `depot_tools`.
2. Sync the `//astra` overlay into `chromium/src/astra/`.
3. Apply patches 0001-0015.
4. Run `gn gen out/astra_Debug` and fix any GN errors.
5. Run `autoninja -C out/astra_Debug astra_browser` and fix compilation errors.
6. Add this as a CI check.

**Related files:** All `.cc` and `.h` files in `chromium/astra/`
**Estimated effort:** 2-4 weeks (high uncertainty)

---

### TD-002: Browser layer depends on ui/color (upward dependency)

**Description:** `astra/browser/BUILD.gn` includes `//astra/ui/color` in deps. This
violates the documented architecture where `browser` and `ui/color` are peer layers
both depending on `common`. The only user is `AstraThemeService`.

**Impact:** Architecture debt. If unaddressed, it creates a precedent for upward
dependencies and makes the dependency graph harder to reason about. In the short
term it compiles fine; in the long term it makes refactoring harder.

**Recommended fix:** Move `AstraThemeService` from `astra/browser/` to
`astra/ui/color/`, since theme/color management logically belongs in the color
system layer. Alternatively, extract the color types needed by `AstraThemeService`
into `astra/common` and keep the service in browser.

**Related files:**
- `chromium/astra/browser/BUILD.gn:122`
- `chromium/astra/browser/astra_theme_service.cc`
- `chromium/astra/browser/astra_theme_service.h`
- `chromium/astra/browser/astra_theme_service_factory.cc`
- `chromium/astra/browser/astra_theme_service_factory.h`

**Estimated effort:** 0.5-1 day

---

### TD-003: Empty test targets won't compile

**Description:** Multiple test targets have empty `sources = []` lists:
- `astra_app_tests` (app layer)
- `astra_app_browsertests` (app layer)
- `astra_ui_color_tests` (color layer)
- `astra_ui_views_tests` (views layer)
- `astra_ui_views_interactive_uitests` (views layer)
- `astra_ui_webui_tests` (webui layer)
- `astra_browser_browsertests` (browser layer)

Empty source sets may cause linker warnings or errors depending on the GN toolchain
configuration. More importantly, they represent unfulfilled test commitments.

**Impact:** Build warnings/errors when these targets are built. Gives false sense of
test coverage (targets exist but contain no tests).

**Recommended fix:**
- Remove empty test targets that have no plan for near-term implementation, OR
- Add at least one test per target as a placeholder, OR
- Mark them with `assert(is_chromeos, "Not implemented")` style guards.

**Related files:**
- `chromium/astra/app/BUILD.gn:84-96`, `107-120`
- `chromium/astra/ui/color/BUILD.gn:90-103`
- `chromium/astra/ui/views/BUILD.gn:214-227`, `239-254`
- `chromium/astra/ui/webui/BUILD.gn:84-97`
- `chromium/astra/browser/BUILD.gn:220-233`

**Estimated effort:** 1-2 days (for placeholder tests across all targets)

---

## High (architecture issues, missing key functionality)

### TD-004: No session restore metadata integration

**Description:** `AstraTabFeatures` stores per-tab metadata (workspace_id, favorite
state, split view config, etc.) but there is no mechanism to persist this across
browser restarts. Patch 0006 documents the session restore patch point, but the
`AstraSessionRestoreHelper` implementation is stub-level.

**Impact:** All Astra tab metadata is lost on browser restart. Workspace assignments,
favorites, split view state, and tab stack membership all reset to defaults. This
makes workspaces and favorites essentially useless for real usage.

**Recommended fix:**
1. Implement `AstraSessionRestoreHelper` to serialize/deserialize Astra metadata
   into Chromium's session storage.
2. Implement patch 0006 to hook into session restore pipeline.
3. Add browser tests verifying metadata round-trips through session restore.

**Related files:**
- `chromium/astra/browser/astra_session_restore_helper.cc`
- `chromium/astra/browser/astra_session_restore_helper.h`
- `chromium/astra/browser/astra_session_metadata.cc`
- `chromium/astra/browser/astra_session_metadata.h`
- `chromium/astra/patches/0006-session-restore-metadata.md`

**Estimated effort:** 3-5 days

---

### TD-005: No keyed service factory registration pipeline

**Description:** Astra defines 10+ `ProfileKeyedServiceFactory` classes but there
is no verified integration with Chromium's profile keyed service factory system.
`RegisterAstraProfileKeyedServices()` in `astra_workspace_service.cc` is a stub
with an empty body, and the patch point for factory registration is noted but
not wired up.

**Impact:** Services may not be created at the right time in the profile lifecycle.
Prefs may not be registered. Incognito redirect behavior may not work correctly.
Lazy creation via `GetForProfile()` may work for some services but could cause
ordering issues.

**Recommended fix:**
1. Wire `RegisterAstraProfileKeyedServices()` into Chromium's factory registration
   (either via patch or via `BrowserMainExtraParts::PreProfileInit()`).
2. Ensure all factory `DependsOn()` relationships are correct.
3. Test that services are created in the right order and work correctly with
   incognito profile redirects.

**Related files:**
- `chromium/astra/browser/astra_keyed_service_factories.cc`
- `chromium/astra/browser/astra_keyed_service_factories.h`
- `chromium/astra/browser/astra_workspace_service.cc:601-617`
- `chromium/astra/app/astra_browser_main_extra_parts.cc`

**Estimated effort:** 2-3 days

---

### TD-006: BrowserView integration patch not validated

**Description:** Patch 0002 (BrowserView install) is documented but not applied to
a working build. `AstraBrowserView` is designed as a coordinator object that installs
into `BrowserView`, but the actual layout integration (how the sidebar fits into
`BrowserView`'s existing layout) has not been tested.

**Impact:** The entire UI layer may not fit into `BrowserView`'s layout hierarchy.
The sidebar may not render correctly, may conflict with existing side panel UI,
or may cause layout thrashing.

**Recommended fix:**
1. Apply patch 0002 in a real Chromium checkout.
2. Implement `AstraBrowserView::Install()` to add the sidebar to `BrowserView`'s
   layout correctly.
3. Handle resize, show/hide, and interaction with Chrome's side panel.
4. Add browser tests verifying sidebar visibility.

**Related files:**
- `chromium/astra/ui/views/astra_browser_view.cc`
- `chromium/astra/ui/views/astra_browser_view.h`
- `chromium/astra/patches/0002-browser-view-install.md`

**Estimated effort:** 3-5 days

---

### TD-007: 1,351 TODO(astra) comments across 250 files

**Description:** The codebase contains 1,351 `TODO(astra)` occurrences across 250
source files. This represents a very high density of incomplete implementation,
averaging 5.4 TODOs per source file.

**Impact:**
- Many files are effectively stubs with TODO-filled implementations.
- Hard to estimate true implementation completeness.
- High risk of missing functionality when first building.
- TODOs in critical paths could cause runtime crashes or silent failures.

**Recommended fix:**
1. Categorize TODOs by severity (critical / high / medium / low).
2. Prioritize and batch TODO cleanup per module.
3. Adopt a policy that TODOs must reference a tracking issue or specific
   Chromium owner / patch point.

**Related files:** All source files in `chromium/astra/`
**Estimated effort:** Ongoing (many person-weeks total)

---

### TD-008: No TabStripModelObserver integration validated

**Description:** `AstraSidebarView` implements `TabStripModelObserver` and declares
`StartObservingTabStrip()` / `StopObservingTabStrip()`, but the actual observation
wiring into `BrowserView` is not validated. The sidebar relies on tab change
notifications to update its presentation.

**Impact:** Sidebar may show stale data or not update when tabs change. Full rebuilds
via `UpdateFromModel()` may work but be slow. Incremental updates (insert/remove/move)
may not be implemented.

**Recommended fix:**
1. Wire `TabStripModel` observation through `AstraBrowserView` to sidebar.
2. Implement incremental update handlers instead of full rebuilds.
3. Test that sidebar updates correctly for add, remove, move, activate, pin,
   and tab change events.

**Related files:**
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.h:162-187`
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.cc` (69 TODOs)
- `chromium/astra/ui/views/astra_browser_view.h:311-323`

**Estimated effort:** 2-3 days

---

### TD-009: No overlay sync or Chromium bootstrap scripts

**Description:** The codebase lives in `chromium/astra/` as an overlay, but there
are no scripts to:
- Bootstrap a Chromium checkout (`depot_tools`, `fetch chromium`)
- Sync the `chromium/astra/` overlay into `chromium/src/astra/`
- Apply all patches in order
- Rebase patches after Chromium version updates

**Related files:**
- `chromium/astra/patches/README.md:173-174` (references `apply-astra-patches.sh` as TODO)
- `scripts/` directory

**Impact:** Developers cannot easily set up a working environment. No repeatable
build process. No CI possible.

**Recommended fix:**
1. Create `scripts/chromium-bootstrap.sh` that installs depot_tools and runs
   `fetch chromium`.
2. Create `scripts/sync-astra-overlay.sh` that copies/symlinks `chromium/astra/`
   into `chromium/src/astra/`.
3. Create `scripts/apply-astra-patches.sh` that applies all patches in order.
4. Document the full developer setup flow.

**Estimated effort:** 2-3 days

---

## Medium (UX gaps, incomplete features)

### TD-010: Sidebar full-rebuild on every tab change

**Description:** `AstraSidebarView::UpdateFromModel()` does a full rebuild of all
sidebar sections. The `TabStripModelObserver` handlers are not implemented to do
incremental updates. Every tab add/remove/move triggers a complete sidebar rebuild.

**Impact:** Poor performance with many tabs. Layout flicker. Excessive CPU usage
for common operations like opening/closing tabs.

**Recommended fix:** Implement incremental updates in each `TabStripModelObserver`
method using `AstraSidebarSectionView::InsertItemAt()` / `RemoveItemAt()`.

**Related files:**
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.h:78-83`
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.cc`

**Estimated effort:** 2-3 days

---

### TD-011: Workspace deletion doesn't reassign orphan tabs

**Description:** When a workspace is deleted, tabs that were in that workspace
should be reassigned to the default workspace. `AstraWorkspaceService::DeleteWorkspace()`
has a TODO noting this is not implemented because the service does not own
`TabStripModel` and cannot iterate tabs directly.

**Impact:** Deleting a workspace leaves tabs "stranded" -- their `workspace_id`
still refers to a deleted workspace. They would disappear from the sidebar
projection but still exist in `TabStripModel`.

**Recommended fix:** Either:
1. Add a helper that iterates all `Browser` windows' `TabStripModel` and updates
   `AstraTabFeatures::workspace_id()` for affected tabs, OR
2. Have `AstraSidebarView` treat tabs in deleted workspaces as belonging to the
   default workspace (projection-level fallback).

**Related files:**
- `chromium/astra/browser/astra_workspace_service.cc:212-229`
- `chromium/astra/browser/astra_workspace_window_manager.cc`

**Estimated effort:** 1 day

---

### TD-012: No drag-and-drop implementation

**Description:** `AstraSidebarView` declares drag-and-drop delegate interfaces
(`AstraSidebarItemDragDelegate`, `AstraSidebarSectionDropDelegate`) and has
drag state variables (`is_dragging_`, `current_drag_data_`, etc.), but the
actual drag implementation appears to be incomplete.

**Impact:** Users cannot drag tabs between workspaces, into favorites, or
between sidebar sections. Major UX gap for the sidebar's core interaction model.

**Recommended fix:**
1. Implement `OnMouseDragged()`, `OnMouseReleased()`, and drag ghost rendering.
2. Implement section-level drop validation and execution.
3. Wire drag operations to service-layer mutations (move tab to workspace,
   add to favorites, reorder, etc.).

**Related files:**
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.h:247-250`
- `chromium/astra/ui/views/sidebar/astra_sidebar_view.h:251-263`
- `chromium/astra/ui/views/sidebar/astra_sidebar_drag_types.h`
- `chromium/astra/ui/views/sidebar/astra_sidebar_drop_indicator_view.h`

**Estimated effort:** 3-5 days

---

### TD-013: Split view has no WebContents layout implementation

**Description:** `AstraSplitView` and `AstraSplitViewController` are declared
with interfaces but the actual layout of two `WebContents` views side-by-side
is likely stub-level. The `AstraTabFeatures::split_view_partner_id` uses strings
instead of proper tab/WebContents references.

**Impact:** Split view is a flagship feature but currently just metadata. No actual
two-pane layout exists.

**Recommended fix:**
1. Implement `AstraSplitView` as a views container that holds two `WebView`
   or `WebContentsView` children.
2. Implement `AstraSplitViewController` to manage the split view lifecycle,
   partner tab pairing, and resize.
3. Integrate with `BrowserView`'s content area layout.
4. Wire split view commands through `AstraCommandDelegate`.

**Related files:**
- `chromium/astra/ui/views/split_view/astra_split_view.cc`
- `chromium/astra/ui/views/split_view/astra_split_view.h`
- `chromium/astra/ui/views/split_view/astra_split_view_controller.cc`
- `chromium/astra/ui/views/split_view/astra_split_view_controller.h`

**Estimated effort:** 4-7 days

---

### TD-014: No actual DevTools integration

**Description:** `AstraDevToolsIntegration`, `AstraDevToolsToolbar`, and
`AstraDevToolsWorkspacePanel` are declared as views components but there is no
verified patch point for injecting them into `DevToolsWindow`. Patch 0015 only
exposes dock state, not full toolbar/panel integration.

**Impact:** DevTools extensions are a non-functional feature. The Astra DevTools
toolbar and workspace panel cannot appear in the DevTools window.

**Recommended fix:**
1. Design the integration approach (native Views injection vs. WebUI extension
   vs. DevTools protocol).
2. Implement patch 0015 expanded with toolbar/panel hooks.
3. Implement `AstraDevToolsIntegration` lifecycle (creation, tab switching,
   dock state changes).

**Related files:**
- `chromium/astra/ui/views/devtools/astra_devtools_integration.cc`
- `chromium/astra/ui/views/devtools/astra_devtools_toolbar.cc`
- `chromium/astra/ui/views/devtools/astra_devtools_workspace_panel.cc`
- `chromium/astra/patches/0015-devtools-dock-state.md`

**Estimated effort:** 3-5 days

---

### TD-015: Buildflags are manually maintained

**Description:** `astra_buildflags.h` is a manually written header instead of
being generated from GN using `buildflag_header.gni`. The BUILD.gn file notes
this as a TODO.

**Impact:**
- Risk of header and GN defines getting out of sync.
- Doesn't follow Chromium's standard buildflag pattern.
- `BUILDFLAG()` macro may not work correctly with manual defines.

**Recommended fix:** Replace manual header with proper `buildflag_header()` target
using `//build/buildflag_header.gni`.

**Related files:**
- `chromium/astra/build/BUILD.gn:18-19`
- `chromium/astra/build/astra_buildflags.h`

**Estimated effort:** 0.5-1 day

---

### TD-016: Command palette has no actual command listing

**Description:** `AstraCommandPaletteModel` and `AstraCommandPaletteView` exist
but the model likely does not actually enumerate Chrome commands or Astra commands
with search/filter capabilities.

**Impact:** Command palette is non-functional -- it can be shown but has no items.

**Recommended fix:**
1. Implement command model that enumerates both Chrome commands (via
   `BrowserCommandController` or command IDs) and Astra commands.
2. Implement fuzzy search/filter.
3. Wire up keyboard navigation and execution.

**Related files:**
- `chromium/astra/ui/views/command_palette/astra_command_palette_model.cc`
- `chromium/astra/ui/views/command_palette/astra_command_palette_view.cc`
- `chromium/astra/ui/views/command_palette/astra_command_palette_bubble.cc`

**Estimated effort:** 2-3 days

---

## Low (code quality, naming consistency)

### TD-017: Inner classes lack Astra prefix

**Description:** 37 inner/nested classes like `Observer`, `Delegate`,
`QuickActionButton`, `ScoredItem`, `ChromeCommandEntry`, `WorkspaceIndicatorButton`,
`FocusModeBadge` don't have `Astra` prefixes. These are flagged by the architecture
check as warnings.

**Impact:** Mostly cosmetic / consistency issue. Inner `Observer` and `Delegate`
classes are idiomatic Chromium pattern and are fine as-is. But some classes
(`ChromeCommandEntry`, `QuickActionButton`, `WorkspaceIndicatorButton`) could
collide with similarly-named types in Chromium headers.

**Recommended fix:**
- Keep `Observer` and `Delegate` as-is (Chromium idiom).
- Rename standalone inner types like `ChromeCommandEntry`, `ScoredItem`,
  `QuickActionButton`, `WorkspaceIndicatorButton`, `FocusModeBadge` to have
  `Astra` prefixes, OR accept them as implementation details inside already-
  prefixed outer classes.

**Related files:** 37 files flagged by `pnpm check:architecture`
**Estimated effort:** 1-2 days

---

### TD-018: Workspace ID type inconsistency

**Description:** Workspace IDs use `std::string` everywhere instead of a dedicated
ID type. The common layer defines `AstraWorkspaceId = std::string` but the browser
layer's `AstraWorkspace` struct uses `std::string id` directly. The TODO notes
potential migration to `base::Uuid`.

**Impact:** Type safety is weak -- any string can be passed as a workspace ID.
Risk of mixing up workspace IDs with other string IDs (folder IDs, stack IDs).

**Recommended fix:** Either:
1. Migrate to `base::UnguessableToken` or `base::Uuid` for strong typing, OR
2. At minimum, use `using AstraWorkspaceId = std::string` consistently across
   all layers instead of bare `std::string`.

**Related files:**
- `chromium/astra/common/astra_workspace_types.h:17-21`
- `chromium/astra/browser/astra_workspace_service.h:33`

**Estimated effort:** 1-2 days

---

### TD-019: Command ID upper bound drift risk

**Description:** `kAstraCommandLast` in `astra/common/astra_command_constants.h`
is hardcoded to `kAstraCommandFirst + 500`, while the actual last command is
defined in `astra/browser/astra_command_delegate.h` as `kAstraCommandLast`
(enum value). These two could drift apart.

**Impact:** If the enum grows beyond 500 entries and the common layer constant
is not updated, range checks in the common/app/ui layers would be wrong.
Commands in the gap would not be recognized as Astra commands.

**Recommended fix:** Generate both from a single source, OR add a static_assert
in the browser layer that verifies the enum's `kAstraCommandLast` fits within
the common layer's range.

**Related files:**
- `chromium/astra/common/astra_command_constants.h:40-44`
- `chromium/astra/browser/astra_command_delegate.h:201`

**Estimated effort:** 0.5 days

---

### TD-020: Static counters for ID generation

**Description:** `AstraCommandDelegate::ExecuteCommand` uses a `static int` counter
for generating workspace IDs and folder IDs (`workspace_counter`, `folder_counter`).
These are not persisted across sessions and will collide after restart.

**Impact:** Workspace and folder IDs are not stable across browser restarts.
After restart, new workspaces would get IDs like "workspace-1" again, potentially
conflicting with saved workspace data.

**Recommended fix:** Use `base::UnguessableToken::Create()` or `base::GenerateGUID()`
for unique IDs. Or at minimum use a timestamp + random component.

**Related files:**
- `chromium/astra/browser/astra_command_delegate.cc:69-73` (workspace counter)
- `chromium/astra/browser/astra_command_delegate.cc:356-357` (folder counter)

**Estimated effort:** 0.5 days

---

### TD-021: Inconsistent use of forward declarations vs includes

**Description:** Some headers use forward declarations (`class Browser;`,
`class TabStripModel;`) correctly while others include heavy Chromium headers
that could be forward-declared. This increases compilation time.

**Impact:** Slower builds. Coupling between headers.

**Recommended fix:** Audit includes across all headers. Replace includes with
forward declarations where possible (types only used as pointers/references
in function signatures).

**Related files:** All `.h` files in `chromium/astra/`
**Estimated effort:** 1-2 days

---

### TD-022: No CI pipeline

**Description:** No CI configuration is visible for building, testing, or
validating the codebase. The `check:architecture` script exists but is only
run manually.

**Impact:** No automated quality gates. Regressions can be introduced silently.
No build verification on PRs.

**Recommended fix:**
1. Add GitHub Actions or similar CI.
2. Run `pnpm check:architecture` on every PR.
3. Once Chromium build is working, add a build step.
4. Run available unit tests.

**Related files:**
- `.github/` directory
- `scripts/check-architecture.mjs`

**Estimated effort:** 1-2 days (for basic CI)

---

### TD-023: `favorite_read_only_` logic in wrong place

**Description:** The incognito read-only guard for favorites is in
`AstraTabFeatures::set_is_favorite()` rather than in the service layer or
command delegate. This means direct callers can bypass the check if they
set the flag through other paths.

**Impact:** Inconsistent enforcement of incognito restrictions. If another
code path sets `is_favorite` without going through `set_is_favorite()`,
the incognito guard is bypassed.

**Recommended fix:** Move incognito policy enforcement to `AstraFavoriteService`
or `AstraCommandDelegate`, with `AstraIncognitoHandler` as the centralized
policy source.

**Related files:**
- `chromium/astra/browser/astra_tab_features.h:94-99`
- `chromium/astra/browser/astra_incognito_handler.h`
- `chromium/astra/browser/astra_incognito_handler.cc`

**Estimated effort:** 0.5-1 day

---

### TD-024: Duplicate "recently closed" concept

**Description:** Both `kAstraCommandReopenClosedTab` (Astra command) and
`IDC_RESTORE_TAB` (Chrome command) do the same thing. The code comments note
this is a duplicate but useful for command palette integration.

**Impact:** Confusion about which command to use. Two code paths for the same
functionality. Risk of divergence.

**Recommended fix:** Decide on a single approach. Either:
1. Remove the Astra version and use Chrome's `IDC_RESTORE_TAB` directly,
   adding sidebar highlighting via a `TabRestoreServiceObserver`.
2. Keep the Astra version but make it clearly only for sidebar/palette
   integration, with a comment explaining the duplication rationale.

**Related files:**
- `chromium/astra/browser/astra_command_delegate.h:69-86`
- `chromium/astra/browser/astra_command_delegate.cc:298-322`

**Estimated effort:** 0.5 days

---

### TD-025: Many helper services could be consolidated

**Description:** Several "helper" services (`AstraExtensionHelper`,
`AstraPasswordHelper`, `AstraSearchEngineHelper`, `AstraRecentTabsHelper`,
`AstraDevToolsHelper`) wrap Chromium subsystems with thin projections. Some
are barely more than a single method call.

**Impact:** Proliferation of tiny classes adds cognitive overhead. Each has
its own header and source file, factory pattern boilerplate, etc.

**Recommended fix:** Consolidate related helpers, or convert them to free
functions in namespaces if they don't need state. Evaluate whether each
helper earns its existence or if the UI layer can call Chromium APIs directly.

**Related files:**
- `chromium/astra/browser/astra_extension_helper.h`
- `chromium/astra/browser/astra_password_helper.h`
- `chromium/astra/browser/astra_search_engine_helper.h`
- `chromium/astra/browser/astra_recent_tabs_helper.h`
- `chromium/astra/browser/astra_devtools_helper.h`

**Estimated effort:** 1-2 days (refactoring + review)

---

## Summary

| Severity | Count | Total Estimated Effort |
|----------|-------|----------------------|
| Critical | 3 | ~2.5-5 weeks |
| High | 6 | ~2.5-4 weeks |
| Medium | 7 | ~2-3 weeks |
| Low | 9 | ~1-2 weeks |
| **Total** | **25** | **~8-14 weeks** to address all debt |

The critical and high items alone represent 5-9 weeks of work, mostly centered
around getting the first working Chromium build and validating the core
architecture assumptions.
