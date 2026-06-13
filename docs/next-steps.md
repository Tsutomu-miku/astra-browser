# Next Steps / Roadmap Update

This document outlines the top priorities for the next development phase, based on the
architecture audit, technical debt inventory, and completeness matrix.

---

## Top 10 Priorities

### 1. Get a working Chromium build with Astra overlay

**Priority: Critical**
**Effort: 2-4 weeks**
**Dependencies: None (foundational)**

Bootstrap a real Chromium checkout, sync the `//astra` overlay, apply all 15 patches,
and get `autoninja -C out/astra_Debug chrome` to compile and link.

Why this is #1:
- Everything else depends on this being verified
- Currently 0% confidence that the code compiles
- All TODOs, all tests, all features are unverified
- This will reveal the true state of the codebase

Deliverables:
- `scripts/chromium-bootstrap.sh` -- installs depot_tools, fetches Chromium
- `scripts/sync-astra-overlay.sh` -- syncs `chromium/astra/` to `chromium/src/astra/`
- `scripts/apply-astra-patches.sh` -- applies all patches in order
- Verified green build: `autoninja -C out/astra_Default chrome` succeeds
- List of compilation errors fixed

---

### 2. Sidebar integration end-to-end

**Priority: Critical**
**Effort: 1-2 weeks**
**Dependencies: #1 (working build)**

Get the `AstraSidebarView` actually showing inside `BrowserView` with real tab data.

Why this is #2:
- The sidebar is the primary Astra UI surface
- It's the most developed module (61 files)
- Validates the core architectural assumption (projection pattern)
- Once sidebar works, many other features can piggyback on the integration

Deliverables:
- Patch 0002 validated and working (BrowserView install)
- `AstraBrowserView::Install()` creates sidebar view in correct location
- Sidebar shows tab list from `TabStripModel`
- `TabStripModelObserver` fires correctly and updates sidebar
- Clicking a tab in sidebar activates it
- Basic browser test: sidebar visible in branded builds

---

### 3. Workspace service validation + TabStripModel projection

**Priority: High**
**Effort: 1-2 weeks**
**Dependencies: #1, #2**

Validate that `AstraWorkspaceService` works end-to-end with real profiles and that
the sidebar correctly projects tabs by workspace.

Deliverables:
- `AstraWorkspaceService` persists correctly through `PrefService`
- Workspace CRUD works from command palette/keyboard shortcuts
- Sidebar filters tabs by active workspace
- Workspace switching is a projection change (no tab reparenting)
- Multi-window workspace behavior works correctly
- `AstraWorkspaceWindowManager` properly tracks window-workspace association

---

### 4. Fix the browser → ui/color dependency issue

**Priority: High**
**Effort: 0.5-1 day**
**Dependencies: None**

Fix the architectural violation where `astra/browser` depends on `astra/ui/color`.

Deliverables:
- Move `AstraThemeService` to `astra/ui/color/`, OR
- Move color types needed by browser down to `astra/common`
- Updated architecture diagram if needed
- Updated BUILD.gn files

---

### 5. Session restore metadata persistence

**Priority: High**
**Effort: 3-5 days**
**Dependencies: #1 (working build)**

Implement patch 0006 and `AstraSessionRestoreHelper` so that Astra tab metadata
(workspace_id, favorite state, split view config) survives browser restarts.

Deliverables:
- Patch 0006 validated and working
- `AstraSessionMetadata` serialization/deserialization working
- `AstraSessionRestoreHelper` properly hooks into Chromium session restore
- Workspace assignment persists across restarts
- Favorite state persists across restarts
- Browser test: metadata round-trips through session restore

---

### 6. Keyed service factory registration pipeline

**Priority: High**
**Effort: 2-3 days**
**Dependencies: #1 (working build)**

Wire all Astra `ProfileKeyedServiceFactory` instances into Chromium's factory
registration system so services are created at the right time in the profile
lifecycle.

Deliverables:
- `RegisterAstraProfileKeyedServices()` called at correct startup point
- All 10+ factory classes registered
- `DependsOn()` relationships correct
- Prefs registered at profile creation time
- Incognito redirect behavior verified
- Browser test: services exist after profile initialization

---

### 7. Command system end-to-end

**Priority: High**
**Effort: 2-3 days**
**Dependencies: #1, #2**

Verify that the command system works end-to-end: accelerator → command controller
→ patch → `AstraCommandDelegate` → service or UI.

Deliverables:
- Patch 0003 (command forwarding) validated
- Patch 0007 (accelerator table) validated
- Patch 0008 (command ID range) validated
- Keyboard shortcuts work: toggle sidebar, next/prev workspace, etc.
- `IsCommandEnabled()` returns correct values
- Observer notifications fire from command execution

---

### 8. Color system integration

**Priority: Medium**
**Effort: 1-2 days**
**Dependencies: #1 (working build)**

Integrate `AstraColorMixer` into Chromium's ColorProvider system via patch 0012.

Deliverables:
- Patch 0012 validated and working
- All `kColorAstra*` color IDs are registered
- Light/dark mode produces correct colors
- Accent color derivation works
- Sidebar uses Astra color IDs
- Unit tests for color mixer

---

### 9. Unit test validation and expansion

**Priority: Medium**
**Effort: 1-2 weeks**
**Dependencies: #1 (working build)**

Get existing tests compiling, fix failures, and fill in gaps.

Deliverables:
- All 16 existing test files compile
- Tests pass in Chromium's `unit_tests` suite
- Tests added for color system (currently empty)
- Tests added for views layer (currently empty)
- Test infrastructure integrated into CI
- High-priority services have good test coverage

---

### 10. Split view implementation

**Priority: Medium**
**Effort: 4-7 days**
**Dependencies: #1, #2**

Implement actual split view with two `WebContents` laid out side by side.

Deliverables:
- `AstraSplitView` renders two WebContents side by side
- `AstraSplitViewController` manages split state and partner pairing
- Split view commands work (toggle, vertical/horizontal, swap)
- Resizable divider
- Integrates with `BrowserView` content area layout
- Session restore preserves split view state

---

## Quick Wins (Low Effort, High Impact)

### QW-1: Replace static counters with proper ID generation

**Effort: 0.5 days**
**Impact: Medium** -- Fixes a correctness bug in workspace/folder ID generation.

Replace `static int` counters in `AstraCommandDelegate` with `base::UnguessableToken`
or `base::Uuid` for stable unique IDs.

### QW-2: Fix command ID upper bound drift risk

**Effort: 0.5 days**
**Impact: Low-Medium** -- Prevents a subtle bug as command count grows.

Add a `static_assert` in `astra_command_delegate.h` that verifies
`kAstraCommandLast` enum value is within the common layer's range.

### QW-3: Convert buildflags to proper buildflag_header.gni

**Effort: 0.5-1 day**
**Impact: Medium** -- Follows Chromium patterns, reduces manual maintenance.

Replace manually maintained `astra_buildflags.h` with generated header using
`buildflag_header.gni` template.

### QW-4: Add CI with architecture check

**Effort: 1 day**
**Impact: Medium** -- Catches architecture violations early.

Set up GitHub Actions to run `pnpm check:architecture` and `git diff --check`
on every PR.

### QW-5: Clean up empty test targets

**Effort: 0.5 days**
**Impact: Low** -- Reduces build warnings and false sense of coverage.

Either remove empty test targets or add placeholder tests. Update documentation
to reflect actual test coverage.

### QW-6: Add `astra/common` to `astra/browser` public_deps

**Effort: 0.25 days**
**Impact: Low-Medium** -- Clarifies dependency graph.

The browser BUILD.gn currently has `//astra/ui/color` in deps but doesn't explicitly
list `//astra/common` in public_deps (it's pulled in through color). Add it explicitly.

### QW-7: Document accessibility layer in architecture diagram

**Effort: 0.25 days**
**Impact: Low** -- Improves documentation accuracy.

Add `astra/ui/accessibility` to the architecture diagram as a horizontal utility
layer.

---

## Dependency Graph

```
Phase 0 (Foundation)
  1. Working Chromium build  [critical, 2-4w]
      │
      ├── 4. Fix browser→ui/color dep  [high, 0.5-1d]  (can be parallel)
      │
      ├── 6. Keyed service registration  [high, 2-3d]
      │
      ├── 8. Color system integration  [medium, 1-2d]
      │
      └── 9. Unit test validation  [medium, 1-2w]

Phase 1 (Core UX)
  2. Sidebar integration  [critical, 1-2w]
      │
      ├── 3. Workspace projection  [high, 1-2w]
      │
      ├── 7. Command system  [high, 2-3d]
      │
      └── 5. Session restore metadata  [high, 3-5d]

Phase 2 (Feature Parity)
  10. Split view  [medium, 4-7d]
```

---

## Sequencing Recommendations

### Sprint 1 (2 weeks)
- **Goal:** Get a compiling, running Chromium build with Astra overlay
- **Tasks:** #1 (working build), #4 (fix dep issue), QW-3 (buildflags), QW-4 (CI)
- **Exit criteria:** `autoninja` green, architecture check in CI

### Sprint 2 (2 weeks)
- **Goal:** Visible sidebar with tab data in Chromium browser window
- **Tasks:** #2 (sidebar integration), #6 (service registration), #7 (command system)
- **Exit criteria:** Sidebar visible, tabs update live, keyboard shortcuts work

### Sprint 3 (2 weeks)
- **Goal:** Workspaces functional end-to-end
- **Tasks:** #3 (workspace projection), #5 (session restore metadata)
- **Exit criteria:** Workspace CRUD works, tabs filter by workspace, persistence works

### Sprint 4 (2 weeks)
- **Goal:** Test infrastructure and quality
- **Tasks:** #9 (unit test validation), #8 (color system integration), all QW items
- **Exit criteria:** Test suite running in CI, color system integrated, quick wins done

### Sprint 5+ (2+ weeks)
- **Goal:** Feature expansion
- **Tasks:** #10 (split view), command palette implementation, focus mode,
  reading list, notes, screenshot capture
- **Exit criteria:** Feature parity with Phase 4 of roadmap

---

## Risk Management

### Highest risks

1. **Chromium build may reveal many compilation errors** -- the codebase has
   never been compiled. Expect 1-2 weeks of fixing compilation issues.

2. **Patches may not apply cleanly** -- all 15 patches are "planned" status.
   Some may need rework to match the actual Chromium codebase layout.

3. **BrowserView integration may be harder than expected** -- fitting the sidebar
   into Chromium's existing layout (side panel, toolbar, bookmarks bar) may
   require more substantial patches than anticipated.

4. **TabStripModelObserver may not work as expected** -- the sidebar relies on
   observing TabStripModel for updates, but the exact integration points and
   threading behavior need verification.

### Mitigation strategies

- Start with the smallest possible integration (patch 0001 + compile check)
  before attempting full sidebar integration.
- Use `gn check` early to catch dependency issues.
- Prioritize getting a debug build running over optimized builds.
- Keep patches tiny; if a patch grows, move logic to `//astra`.
- Write tests alongside implementation to catch regressions early.
