# Roadmap

## Phase 0: Architecture Reset

**Status: complete.**

Decision to move from Electron/CEF prototype to direct Chromium product layer.

- [x] Remove CEF/CMake/native-shell scaffold.
- [x] Add direct Chromium overlay under `chromium/astra/`.
- [x] Accept ADR-0009 for direct Chromium architecture.
- [x] Add architecture guardrails (`AGENTS.md`, engineering standards).
- [x] Archive legacy Electron prototype docs under `docs/legacy/`.
- [x] Establish `pnpm check:architecture` guard.

Exit criteria met: no CEF/Electron runtime in active architecture.

---

## Phase 1: Chromium Checkout Integration

**Goal:** Build Chromium with the Astra overlay present.

**Status: in progress (skeleton complete, 4 patches implemented as .patch files, architecture expanded).**

Sub-tasks:

- [x] Define overlay directory structure (`app/`, `browser/`, `ui/views/`).
- [x] Create `BUILD.gn` skeleton for `//astra:astra_browser`.
- [x] Define Astra class stubs: `AstraWorkspaceService`, `AstraTabFeatures`,
  `AstraCommandDelegate`, `AstraBrowserView`, `AstraSidebarView`.
- [x] Document patch queue in `chromium/astra/patches/README.md`.
- [x] Write ADR-0009 through ADR-0013 covering all core architectural decisions.
- [x] Expand architecture with common layer (`astra/common/`).
- [x] Add color system (`astra/ui/color/`) with AstraColorMixer.
- [x] Add DevTools integration (`astra/ui/views/devtools/`).
- [x] Add accessibility utilities (`astra/ui/accessibility/`).
- [x] Add patch 0001: register `AstraBrowserMainExtraParts` (.patch file created).
- [x] Add patch 0002: install `AstraBrowserView` (.patch file created).
- [x] Add patch 0003: forward command IDs (.patch file created).
- [x] Add patch 0004: include `//astra` in build graph (.patch file created).
- [x] Define 11 patches total (0001-0011) in the patch queue.
- [x] Write ADR-0014 through ADR-0028 covering additional features (notes,
  tab stacks, reading list, screenshots, new tab page, etc.).
- [ ] Create `scripts/chromium-bootstrap.sh` with depot_tools and `fetch chromium`.
- [ ] Implement overlay sync script: `chromium/astra/` -> `chromium/src/astra`.
- [ ] Get `autoninja -C out/astra_Debug chrome` to link with `//astra`.
- [ ] Add architecture smoke test verifying `//astra:astra_browser` in build graph.
- [ ] Verify `pnpm check:architecture` passes on CI.
- [ ] Unit test coverage for common layer, color system, devtools views.

Exit criteria:

- `autoninja -C out/astra_Debug chrome` passes.
- Patch queue is documented and minimal (11 patches total).
- No CEF/Electron/CMake runtime code is introduced.

---

## Phase 2: BrowserView Integration

**Goal:** Show an Astra sidebar inside Chromium's desktop UI without replacing
Chrome browser ownership.

**Status: skeleton complete (views defined, not yet wired into Chromium build).**

Sub-tasks:

- [x] Define `AstraBrowserMainExtraParts` stub with service registration hooks.
- [x] Define `AstraBrowserView` stub as a controller that augments `BrowserView`.
- [x] Define `AstraSidebarView` and all sidebar section views as Views widgets.
- [x] Define split view, glance, command palette, and other UI views as stubs.
- [x] Define DevTools integration views (toolbar, workspace panel, coordinator).
- [x] Define color system with AstraColorMixer and color IDs.
- [ ] Add patch 0002: install `AstraBrowserView` after `BrowserView` construction (patch defined, not yet applied in working build).
- [ ] Implement `AstraBrowserView` as a controller that augments `BrowserView`.
- [ ] Wire sidebar into `BrowserView`'s layout (left side, fixed width).
- [ ] Keep toolbar, content area, DevTools, WebUI, downloads, and profiles
      as Chromium-owned.
- [ ] Verify basic navigation (new tab, load page, close tab) works end-to-end.
- [ ] Add browser test: Astra sidebar is visible in branded builds.
- [ ] Add browser test: standard Chrome commands still work.

Exit criteria:

- A Chromium build launches with visible Astra sidebar shell.
- New tab, close tab, navigation, DevTools, history, downloads, and extensions
  still use Chrome infrastructure.
- Sidebar is a Views view, not a WebUI or HTML surface.

---

## Phase 3: Workspace Semantics

**Goal:** Implement Astra Spaces as product metadata over Chromium tab state.

**Status: service stubs defined, not yet wired into Chromium.**

Sub-tasks:

- [x] Define `AstraWorkspaceService` stub with workspace CRUD interface.
- [x] Define `AstraTabFeatures` `WebContentsUserData` stub with workspace_id.
- [x] Define common layer types: workspace types, tab types, command constants.
- [x] Define favorite service, focus mode service, and other product services.
- [x] Define session metadata and session restore helper stubs.
- [ ] Implement `AstraWorkspaceService` with workspace CRUD and persistence.
- [ ] Wire `AstraTabFeatures` creation on tab creation (via
      `TabStripModelObserver` or WebContents creation hook).
- [ ] Project Chromium tabs into sidebar sections by workspace filter.
- [ ] Implement workspace switching as UI projection change (no tab reparenting).
- [ ] Implement Favorites section in sidebar via `is_favorite` metadata.
- [ ] Implement Pinned section via Chromium's native pinned state + sidebar
      projection.
- [ ] Session restore bridge: restore workspace metadata after Chromium session
      restore.
- [ ] Migrate P0 tab identity behavior from legacy Electron tests into Chromium
      browser tests.
- [ ] ADR-0010 and ADR-0011 validation against implementation.

Exit criteria:

- Space switching is metadata projection, not separate browser runtime state.
- Favorites preserve tab identity across session restore.
- Sidebar projection updates reactively to `TabStripModel` changes.

---

## Phase 4: Product Parity

**Goal:** Make the direct Chromium build usable as the main Astra shell.

**Status: architecture defined, implementation in skeleton stage.**

Sub-tasks:

- [x] Define command ID range (60000+) and command delegate interface.
- [x] Define accelerator table with key Astra shortcuts.
- [x] Define command palette Views stubs.
- [x] Define split view and glance Views stubs.
- [x] Define notes, reading list, screenshot, PiP, and other feature stubs.
- [x] Define new tab page, profile menu, tab search, settings UI stubs.
- [x] Define omnibox provider and location bar decoration stubs.
- [x] Write ADRs for all major features (0014-0028).
- [ ] Implement all Astra commands: toggle sidebar, new workspace,
      next/previous workspace, add to favorites, toggle split view, etc.
- [ ] Implement command palette as a Views bubble that searches both Chrome
      and Astra commands.
- [ ] Implement Split View as a Views layout of two `WebContents`.
- [ ] Implement Glance/Peek as a temporary side `WebContents`.
- [ ] Settings/history/downloads/passwords/extensions use Chrome WebUI first,
      with Astra visual styling.
- [ ] Session metadata persistence through Chromium profile/session mechanisms.
- [ ] Add session restore browser tests for workspace + split state.
- [ ] Add unit tests for all Astra services and metadata objects.

Exit criteria:

- Daily navigation workflows run in direct Chromium build.
- Legacy Electron prototype is no longer needed for core dogfooding.

---

## Phase 5: Legacy Retirement

**Goal:** Remove the old prototype after direct Chromium is the working product.

**Status: not started.**

Sub-tasks:

- [ ] Archive or delete `src/` Electron runtime code.
- [ ] Remove Electron build and packaging dependencies.
- [ ] Replace Electron packaging with Chromium signing/notarization/release flow.
- [ ] Move final tests to Chromium unit/browser/ui test suites.
- [ ] Update documentation to remove legacy references.
- [ ] Final architecture review and guardrail tightening.

Exit criteria:

- No Electron dependency in production build.
- Release artifacts are Chromium-built Astra packages.
- All tests run against Chromium test infrastructure.

---

## Current Priorities

Derived from the architecture audit, tech debt inventory, and completeness matrix.
These are the top items to tackle next, in priority order.

### P0 — Critical

1. **Working Chromium build with Astra overlay**
   - Bootstrap a real Chromium checkout
   - Sync `//astra` overlay, apply patches
   - Get `autoninja -C out/astra_Debug chrome` to compile and link
   - *Effort: 2-4 weeks*

2. **Sidebar integration end-to-end**
   - `AstraSidebarView` showing inside `BrowserView` with real tab data
   - `TabStripModelObserver` fires correctly and updates sidebar
   - *Effort: 1-2 weeks, depends on #1*

### P1 — High

3. **Workspace service validation + TabStripModel projection**
   - `AstraWorkspaceService` persists correctly through `PrefService`
   - Sidebar correctly projects tabs by workspace
   - Workspace switching is a projection change (no tab reparenting)
   - *Effort: 1-2 weeks, depends on #1, #2*

4. **Fix browser → ui/color dependency issue**
   - Move `AstraThemeService` to `astra/ui/color/`, OR
   - Move color types needed by browser down to `astra/common`
   - *Effort: 0.5-1 day*

5. **Session restore metadata persistence**
   - Patch 0006 validated and working
   - `AstraSessionMetadata` serialization/deserialization
   - Workspace assignment and favorite state survive restarts
   - *Effort: 3-5 days, depends on #1*

6. **Keyed service factory registration pipeline**
   - Wire all Astra `ProfileKeyedServiceFactory` instances into Chromium
   - All 10+ factory classes registered at profile creation
   - *Effort: 2-3 days, depends on #1*

7. **Command system end-to-end**
   - Patches 0003, 0007, 0008 validated
   - Keyboard shortcuts work: toggle sidebar, next/prev workspace, etc.
   - *Effort: 2-3 days, depends on #1, #2*

### P2 — Medium

8. **Color system integration**
   - Integrate `AstraColorMixer` into Chromium's ColorProvider (patch 0012)
   - All `kColorAstra*` color IDs registered
   - *Effort: 1-2 days, depends on #1*

9. **Unit test validation and expansion**
   - All existing tests compile and pass in Chromium's `unit_tests` suite
   - Tests added for color system and views layer
   - *Effort: 1-2 weeks, depends on #1*

10. **Split view implementation**
    - `AstraSplitView` renders two WebContents side by side
    - Resizable divider, layout modes
    - *Effort: 4-7 days, depends on #1, #2*

### Quick Wins (Low Effort, High Impact)

- **QW-1:** Replace static counters with `base::UnguessableToken` for stable IDs
- **QW-2:** Add `static_assert` for command ID upper bound
- **QW-3:** Convert buildflags to proper `buildflag_header.gni`
- **QW-4:** Add CI with architecture check (GitHub Actions)
- **QW-5:** Clean up empty test targets
- **QW-6:** Add `astra/common` to `astra/browser` public_deps explicitly
- **QW-7:** Document accessibility layer in architecture diagram

### Sequencing

```
Sprint 1 (2 weeks): #1 (working build) + #4 (dep fix) + QW-3, QW-4
Sprint 2 (2 weeks): #2 (sidebar integration) + #6 (service registration) + #7 (command system)
Sprint 3 (2 weeks): #3 (workspace projection) + #5 (session restore metadata)
Sprint 4 (2 weeks): #9 (test validation) + #8 (color system) + all QW items
Sprint 5+ (2+ weeks): #10 (split view) + feature parity work
```
