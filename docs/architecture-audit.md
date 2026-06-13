# Architecture Audit

## Executive Summary

**Overall Architecture Health Score: 5.5 / 10**

The Astra browser has a well-conceived architecture built on sound Chromium integration
principles (projection pattern, tiny patches, reuse of Chromium subsystems). The layer
structure is clearly documented and mostly followed in practice. However, the codebase is
at an early skeleton-to-partial implementation stage with significant gaps in wiring,
testing, and integration.

### Strengths

1. **Clean layer separation** -- The common / browser / ui/views / app layering is
   well-defined and mostly respected. Browser services use `ProfileKeyedService` and
   `WebContentsUserData` patterns correctly.
2. **Strong projection philosophy** -- Astra adds metadata to Chromium-owned state
   rather than duplicating it. `TabStripModel`, `WebContents`, `Profile`, history,
   downloads, passwords, extensions, and DevTools all remain Chromium-owned.
3. **Comprehensive patch documentation** -- 15 patches are documented with both `.md`
   descriptions and `.patch` diff files, including rationale, alternatives, and risks.
4. **Consistent naming and style** -- `Astra` class prefix, `astra_` file prefix,
   Chromium-style C++ with `base::ObserverList`, `raw_ptr`, `ProfileKeyedServiceFactory`.
5. **Good observer/separation pattern** -- `AstraCommandDelegate::Observer` cleanly
   decouples browser-layer logic from UI-layer presentation, mirroring Chromium's
   Browser / BrowserView split.

### Top 5 Issues

1. **No end-to-end build verification** -- The code has never been compiled in a real
   Chromium checkout. BUILD.gn files contain TODOs noting that deps need `gn check`
   verification. This is the single biggest risk -- there could be compile errors,
   missing includes, or link issues throughout the codebase.
2. **Zero test execution** -- 16 test files exist (12 browser, 1 common, 2 devtools
   views, 1 accessibility) but none have been run against real Chromium test
   infrastructure. The accessibility unit test is the most substantial but relies on
   `ViewsTestBase` that is not available in this overlay repo.
3. **Browser layer depends on ui/color** -- `astra/browser/BUILD.gn` includes
   `"//astra/ui/color"` in deps, which is an upward dependency. The documented
   architecture shows `ui/color` and `browser` as peers both depending on `common`.
   `AstraThemeService` is the only user of this dependency.
4. **Massive TODO surface** -- 1,351 `TODO(astra)` occurrences across 250 files.
   Many files are essentially header-only stubs with implementation TODOs. The
   sidebar view alone has 69 TODOs in its `.cc` file.
5. **No integration wiring** -- The patches are documented and .patch files exist, but
   there is no verified Chromium checkout, no overlay sync script, no bootstrap
   script, and no CI that validates the build. All 15 patches are marked "planned"
   status.

---

## Layer Architecture

### Documented Dependency Graph

From `docs/ARCHITECTURE.md`:

```
                     Chromium subsystems
      (//base, //skia, //ui/gfx, //ui/color, etc.)
                ▲                ▲
                │                │
           astra/common   astra/ui/color
                ▲    ▲          ▲
                │    │          │
           astra/browser │      │
                ▲    │          │
                │    └─────── astra/ui/views
                │                 ▲
           astra/app           astra/ui/views/devtools

  Chromium patch points ── call into any //astra layer
```

Key rules documented:
- `astra/common` is the bottom layer.
- `astra/browser` depends on `astra/common`, not on UI.
- `astra/ui/views` depends on `astra/browser` and `astra/common`.
- `astra/ui/color` depends on `astra/common` (or directly on Chromium types).
- `astra/app` depends on `astra/browser` and `astra/common`.

### Actual Dependency Graph (from BUILD.gn)

| Layer | Depends on | Notes |
|-------|-----------|-------|
| `astra/common` | `//base`, `//skia`, `//ui/gfx`, `//astra/build` | Correct -- bottom layer. |
| `astra/build` | `//build:buildflag` | Build config; depends on no other Astra layer. |
| `astra/ui/color` | `//base`, `//skia`, `//ui/color`, `//ui/gfx`, `//ui/native_theme`, `//astra/build` | Correct -- no browser or views dependency. |
| `astra/browser` | `//astra/ui/color`, `//base`, `//chrome/browser`, `//chrome/browser/profiles`, `//components/*`, `//content/public/browser`, `//astra/build` | **ISSUE:** Depends on `//astra/ui/color` which is a peer layer, not below. |
| `astra/ui/views` | `//astra/browser`, `//astra/ui/accessibility`, `//base`, `//chrome/browser/*`, `//components/*`, `//ui/*`, `//skia`, `//url`, `//astra/build` | Correct -- depends on browser. |
| `astra/ui/views/devtools` | `//astra/browser`, `//base`, `//chrome/*`, `//ui/*`, `//skia`, `//url` | Correct -- depends on browser. |
| `astra/ui/webui` | `//astra/browser`, `//astra/app/resources`, `//base`, `//chrome/*`, `//components/*`, `//content/public/browser`, `//net`, `//services/network`, `//skia`, `//ui/*`, `//url`, `//astra/build` | Correct -- depends on browser. |
| `astra/ui/accessibility` | (deps inferred from .h: `//ui/accessibility`, `//ui/views`) | Not listed as public dep of views in BUILD.gn; views lists it in `deps`. |
| `astra/app` | `//astra/browser`, `//base`, `//chrome/browser`, `//content/public/browser`, `//third_party/blink`, `//ui/base/accelerators`, `//ui/events`, `//ui/views`, `//url`, `//astra/build` | **ISSUE:** Depends on `//ui/views` transitively? Actually listed in `deps`, which means app depends on views. Documented architecture says app depends on browser/common only. |
| `astra/app/resources` | (Grit target, no code deps) | Resource compilation. |

### Dependency Direction Violations

1. **Browser → ui/color (upward)** (`chromium/astra/browser/BUILD.gn:122`)
   - `astra_browser` source_set includes `"//astra/ui/color"` in its `deps`.
   - Only `AstraThemeService` (`chromium/astra/browser/astra_theme_service.cc`) uses it.
   - This breaks the documented architecture where `browser` and `ui/color` are peer
     layers both above `common`.
   - **Severity:** Medium. Only one file uses it; could be refactored to move theme
     service to `ui/color` or move color types needed by browser down to `common`.

2. **App → ui/views (questionable)** (`chromium/astra/app/BUILD.gn:49`)
   - `astra/app` includes `"//ui/views"` in its `deps`.
   - The documented architecture shows `app` depending on `browser` and `common`, not
     directly on views.
   - **Severity:** Low. May be needed for accelerator table integration. Should be
     audited against actual `#include`s.

3. **Views → ui/accessibility (correct direction but thin layer)**
   - `astra/ui/views` depends on `//astra/ui/accessibility`.
   - The accessibility layer is a utility layer, not a lower layer. This is acceptable
     as a horizontal utility, but it's not documented in the architecture diagram.

---

## Module Inventory

### Core Modules

| Module | Files (.cc/.h) | Maturity | Notes |
|--------|--------------|----------|-------|
| `astra/common` | 5 (2 cc / 3 h) + 1 test | **Partial** | Workspace types, command constants, tab types, UI constants. Basic types defined. Test file referenced but sources list only has it in test target. |
| `astra/build` | 2 (0 cc / 1 h + 1 gni) | **Skeleton** | Buildflags header + GN config. Manually maintained; TODO to use `buildflag_header.gni` template. |
| `astra/browser` | 96 (54 cc / 42 h) + 13 tests | **Partial** | 18+ services defined. Workspace, favorite, tab features, command delegate have substantial implementation. Many services are stub/skeleton. |
| `astra/ui/color` | 5 (2 cc / 3 h) + 0 tests | **Partial** | Color IDs, color mixer, theme utils. Color mixer implementation exists. No unit tests yet (test target is empty). |
| `astra/ui/accessibility` | 3 (2 cc / 1 h) + 1 test | **Partial** | Utility functions for accessible names, roles, states, focus management, live regions. Has substantial unit test with both standalone and ViewsTestBase-based tests. |
| `astra/ui/views` | 149 (75 cc / 74 h) + 0 tests | **Skeleton → Partial** | Very large module with many sub-components. Sidebar (61 files) is most developed. Many views have headers + basic .cc but heavy TODO density. |
| `astra/ui/views/devtools` | 6 (3 cc / 3 h) + 2 tests | **Skeleton** | Toolbar, workspace panel, integration coordinator. Has unit test files. |
| `astra/ui/webui` | 7 (3 cc / 4 h) + 0 tests | **Skeleton** | New tab handler, WebUI config, constants. Web infrastructure stubbed out. |
| `astra/app` | 16 (7 cc / 9 h) + 0 tests | **Skeleton** | Main delegate, content browser client, feature list, brand, accelerators. Startup hooks stubbed. |
| `astra/app/resources` | 7 files | **Skeleton** | GRD file, strings, and newtab HTML/CSS/JS. Basic placeholder content. |
| `astra/patches` | 31 files (15 .md + 15 .patch + README + template) | **Planned** | All 15 patches documented with both description and diff files. Status is "planned" -- not applied to a working Chromium checkout. |
| `astra/test` | 1 file (README) | **Skeleton** | Just a README placeholder. |

### Views Sub-modules

| Sub-module | Files | Maturity | Notes |
|-----------|-------|----------|-------|
| sidebar | 61 (30 cc / 31 h) | **Partial** | Most developed views sub-module. 11+ section views, drag & drop, item views. Heavy TODO density (69 in sidebar_view.cc alone). |
| settings | 10 (5 cc / 5 h) | **Skeleton** | Page view, section view, search box, search settings view, bubble. |
| profiles | 12 (6 cc / 6 h) | **Skeleton** | Profile menu controller, workspaces, avatar button, header/footer views. |
| workspace | 8 (4 cc / 4 h) | **Skeleton** | Card view, overview controller/view, import/export dialog. |
| newtab | 8 (4 cc / 4 h) | **Skeleton** | New tab view, bubble, shortcut view, workspace card. |
| command_palette | 8 (4 cc / 4 h) | **Skeleton** | Bubble, item view, model, view. |
| focus_mode | 4 (2 cc / 2 h) | **Skeleton** | Controller and indicator. Also has menu bubble (4 files). |
| screenshot | 4 (2 cc / 2 h) | **Skeleton** | Capture bubble and region overlay. |
| tab_search | 4 (2 cc / 2 h) | **Skeleton** | Bubble and item view. |
| tab_hover | 4 (2 cc / 2 h) | **Skeleton** | Peek controller and preview view. |
| split_view | 4 (2 cc / 2 h) | **Skeleton** | View and controller. |
| glance | 4 (2 cc / 2 h) | **Skeleton** | View and view controller. |
| omnibox | 2 (1 cc / 1 h) | **Skeleton** | Location bar decoration. |
| pip | 2 (1 cc / 1 h) | **Skeleton** | PiP controls view. |

### Browser Services Inventory

| Service | Type | Maturity |
|---------|------|----------|
| `AstraWorkspaceService` | `ProfileKeyedService` | **Partial** -- Full CRUD, persistence via PrefService, observers |
| `AstraFavoriteService` | `ProfileKeyedService` | **Partial** -- Folder CRUD, move operations, observers |
| `AstraFocusModeService` | `ProfileKeyedService` | **Skeleton** -- Toggle, basic state |
| `AstraMemorySaverService` | `ProfileKeyedService` | **Skeleton** -- Tab suspend stubs |
| `AstraNoteService` | `ProfileKeyedService` | **Skeleton** -- Note CRUD stubs |
| `AstraReadingListService` | `ProfileKeyedService` | **Skeleton** -- Reading list projection stubs |
| `AstraScreenshotService` | `ProfileKeyedService` | **Skeleton** -- Capture stubs |
| `AstraPipService` | `ProfileKeyedService` | **Skeleton** -- PiP state stubs |
| `AstraNewTabPageService` | `ProfileKeyedService` | **Skeleton** -- NTP data stubs |
| `AstraTabStackService` | `ProfileKeyedService` | **Skeleton** -- Stack CRUD stubs |
| `AstraThemeService` | `ProfileKeyedService` | **Skeleton** -- Theme color stubs |
| `AstraAccessibilityService` | `ProfileKeyedService` | **Skeleton** -- Accessibility stubs |
| `AstraTabFeatures` | `WebContentsUserData` | **Partial** -- Rich metadata, many fields |
| `AstraWindowFeatures` | Per-window | **Skeleton** |
| `AstraCommandDelegate` | Static / global | **Partial** -- Full command enum, execution switch, observers |
| `AstraSessionRestoreHelper` | Helper | **Skeleton** |
| `AstraSessionMetadata` | Helper | **Skeleton** |
| `AstraWorkspaceWindowManager` | Singleton helper | **Skeleton** |
| `AstraRecentTabsHelper` | Helper | **Skeleton** |
| `AstraDevToolsHelper` | Helper | **Skeleton** |
| `AstraExtensionHelper` | Helper | **Skeleton** |
| `AstraPasswordHelper` | Helper | **Skeleton** |
| `AstraSearchEngineHelper` | Helper | **Skeleton** |
| `AstraIncognitoHandler` | Helper | **Skeleton** |
| `AstraOmniboxManager` / `AstraOmniboxProvider` / `AstraOmniboxAction` | Omnibox | **Skeleton** |
| `astra_prefs` | Pref registration | **Partial** -- 20+ pref keys defined with defaults |

---

## Dependency Direction Check

### Verified Correct Dependencies

- ✅ `common` depends on no other Astra layer -- bottom of graph.
- ✅ `views` depends on `browser` (public_deps) -- correct direction.
- ✅ `devtools/views` depends on `browser` (public_deps) -- correct direction.
- ✅ `webui` depends on `browser` -- correct direction.
- ✅ `app` depends on `browser` (public_deps) -- correct direction.
- ✅ No layer depends on `app` (patch points only) -- correct, app is top of graph.

### Issues Found

1. **`browser` → `ui/color` (upward dependency)**
   - File: `chromium/astra/browser/BUILD.gn:122`
   - `deps = [ "//astra/ui/color", ... ]`
   - Violates: browser and ui/color should be peer layers.
   - User: `AstraThemeService` (`chromium/astra/browser/astra_theme_service.cc`)
   - Recommendation: Move `AstraThemeService` to `astra/ui/color/` since theme/color
     concerns belong in the color system layer, or move color types needed by browser
     down to `astra/common`.

2. **`app` → `ui/views` (potential upward dependency)**
   - File: `chromium/astra/app/BUILD.gn:49`
   - `deps = [ "//ui/views", ... ]`
   - Note: This is `//ui/views` (Chromium's views), NOT `//astra/ui/views`. So it's
     a Chromium dependency, not an Astra layer dependency. This is acceptable since
     accelerator registration involves views-level types.
   - Recommendation: Add a comment clarifying this is Chromium's views, not Astra's.

3. **`browser` → many `//chrome/*` deps**
   - `//chrome/browser`, `//chrome/browser/profiles`, etc.
   - This is expected -- browser services need to integrate with Chrome's profile
     and keyed service infrastructure.
   - The architecture is a "Chromium product layer" so depending on `//chrome/browser`
     is correct.

4. **`ui/views` → `//astra/ui/accessibility`**
   - Accessibility utilities are a horizontal concern, not a vertical layer.
   - This is fine, but the accessibility layer should be documented in the
     architecture diagram.

---

## Public API Surface

What's exposed to Chromium patch points (from `//astra` public headers):

### Build Config Layer
- `astra_buildflags.h` -- `BUILDFLAG(IS_ASTRA_BRANDED)`, `BUILDFLAG(ENABLE_ASTRA_*)`
  - Patch point: Any Chromium file gating Astra-specific code.

### App Layer (Startup Hooks)
- `AstraMainDelegate` -- entry point for `ChromeMainDelegate` patching
  - Patch point: `chrome/app/chrome_main_delegate.cc`
- `AstraContentBrowserClient` -- content browser client hooks
  - Patch point: `chrome/browser/chrome_content_browser_client.cc`
- `AstraBrowserMainExtraParts` -- browser main lifecycle hooks
  - Patch point: `chrome/browser/chrome_browser_main.cc`
- `AstraFeatureList` -- Astra product feature flags
  - Patch point: `chrome/browser/chrome_feature_list_creator.cc` (or similar)
- `AstraAcceleratorTable` / `AstraAcceleratorRegistrar`
  - Patch point: `chrome/browser/ui/views/accelerator_table.cc`

### Browser Layer (Services)
- `AstraCommandDelegate` -- command execution + enabled state
  - Patch point: `chrome/browser/ui/browser_command_controller.cc`
  - Command IDs: `kAstraCommandFirst` (60000) through `kAstraCommandLast`
- `AstraWorkspaceServiceFactory` / `AstraFavoriteServiceFactory` / etc.
  - Patch point: Profile keyed service factory registration
- `AstraTabFeatures` -- `WebContentsUserData` for per-tab metadata
  - No patch point needed (uses public `WebContentsUserData` API)

### UI Layer (Views Integration)
- `AstraBrowserView` -- installs Astra UI into BrowserView
  - Patch point: `chrome/browser/ui/views/frame/browser_view.cc`
- `AstraColorMixer` / `AddAstraColorMixer()` -- color system integration
  - Patch point: `chrome/browser/ui/color/chrome_color_mixers.cc`
- `AstraLocationBarDecoration` -- omnibox workspace indicator
  - Patch point: `chrome/browser/ui/views/location_bar/location_bar_view.cc`
- `AstraProfileMenuWorkspaces` -- workspace section in profile menu
  - Patch point: `chrome/browser/ui/views/profiles/profile_menu_view.cc`
- `AstraDevToolsIntegration` -- DevTools toolbar + panel
  - Patch point: `chrome/browser/devtools/devtools_window.cc`

### Common Layer (Shared Types)
- `AstraWorkspaceId`, `AstraWorkspaceInfo`, `AstraWorkspaceAccentColor`
- `AstraCommandId` enum range, `IsAstraCommandId()`
- UI constants (dimensions, spacing)
- Tab types (split view state, etc.)

---

## Patch Point Analysis

### Summary

| Metric | Count |
|--------|-------|
| Total documented patches | 15 |
| Hard requirement patches | 7 (0001, 0002, 0003, 0004, 0007, 0008, 0012) |
| Nice-to-have patches | 8 (0005, 0006, 0009, 0010, 0011, 0013, 0014, 0015) |
| Patches with .patch files | 15 |
| Patches with .md descriptions | 15 |
| Status of all patches | "planned" (not applied to working build) |

### Patch Categories

| Category | Count | Patches | Risk |
|----------|-------|---------|------|
| **Build / infrastructure** | 2 | 0004 (BUILD.gn include), 0008 (command ID range) | **Low** -- Pure build config / constants. |
| **Startup / lifecycle** | 3 | 0001 (browser main parts), 0013 (content browser client), 0014 (chrome main delegate) | **Medium** -- Touches early startup; bugs can cause crashes on launch. |
| **Browser UI integration** | 4 | 0002 (BrowserView install), 0009 (profile menu), 0010 (location bar), 0015 (DevTools dock state) | **Medium-High** -- UI patches can be fragile across Chromium rebases. |
| **Command system** | 3 | 0003 (command forwarding), 0007 (accelerator table), 0011 (omnibox provider) | **Medium** -- Command dispatch is critical path. |
| **Branding / theme** | 2 | 0005 (branding), 0012 (color mixer) | **Low** -- Cosmetic; can be disabled without breaking functionality. |
| **Session / data** | 1 | 0006 (session restore metadata) | **Medium** -- Data persistence patches must not corrupt session state. |

### Risk Assessment

**Overall patch risk: Medium**

Mitigating factors:
- All patches are build-flag gated with `BUILDFLAG(IS_ASTRA_BRANDED)`.
- All patches follow the "tiny patch, delegate to //astra" pattern.
- Each patch has documented rationale, alternatives, and risks.
- Patch files are reviewed as part of the patch queue process.

Risk factors:
- **No patches have been applied to a real Chromium build**, so there is no
  empirical evidence that they apply cleanly or work correctly.
- **No rebase testing** -- patch stability across Chromium version updates is unknown.
- **No conflict resolution strategy validated** -- the process is documented but
  untested against real upstream changes.
- **UI patches tend to be fragile** -- Chromium's `BrowserView`, `LocationBarView`,
  and `ProfileMenuView` change frequently.

### Patch Size Estimates

From `chromium/astra/patches/README.md`:

| Patch | Estimated Size |
|-------|---------------|
| 0001 browser-main-extra-parts | ~5 lines |
| 0002 browser-view-install | ~15 lines |
| 0003 command-forwarding | ~10 lines |
| 0004 build-gn-include | ~10 lines |
| 0005 astra-branding | ~20 lines |
| 0006 session-restore-metadata | ~25 lines |
| 0007 accelerator-table | ~15 lines |
| 0008 command-id-range | ~5 lines |
| 0009 profile-menu-workspaces | ~15 lines |
| 0010 location-bar-decoration | ~25 lines |
| 0011 omnibox-astra-provider | ~40 lines |
| 0012 color-mixer-integration | ~10 lines |
| 0013 content-browser-client-hooks | ~15 lines |
| 0014 chrome-main-delegate-hooks | ~15 lines |
| 0015 devtools-dock-state | ~10 lines |

Total: ~225 lines across all 15 patches. Average ~15 lines per patch.
**This is excellent -- very small patch surface.**

### Patches That Could Be Eliminated (per documentation)

| Patch | Cleaner Alternative | Likelihood |
|-------|---------------------|-----------|
| 0002 BrowserView install | `BrowserViewObserver` or widget lifecycle API | Low -- Chromium unlikely to add this. |
| 0003 Command forwarding | `CommandUpdater::RegisterCommandHandler()` | Low -- Chrome command system is static. |
| 0007 Accelerator table | `FocusManager` runtime registration | Medium -- partially possible today. |
| 0011 Omnibox provider | Extension omnibox API or provider plugin system | Low -- extensions API has limitations. |
| 0012 Color mixer | Dynamic `ColorProvider::AddMixer()` | Low -- ColorProvider is constructed statically. |
| 0015 DevTools dock state | Public `DevToolsWindow::dock_side()` API | Medium -- simple API surface addition. |
