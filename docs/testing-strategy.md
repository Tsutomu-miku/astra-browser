# Testing Strategy

This document outlines the testing strategy for the Astra browser codebase, including
what's tested, what's missing, test coverage priorities, and what Chromium test
harnesses to use.

---

## Current Test Landscape

### Test Inventory

| Test Target | Files | Status | Harness |
|-------------|-------|--------|---------|
| `astra_common_unittests` | `astra/common/astra_common_unittest.cc` | Skeleton (referenced but may be stub) | `//base/test:test_support` + gtest |
| `astra_browser_unit_tests` | 12 test files | Source files exist, never compiled/run | `TestingProfile` + `content/public/test:test_support` |
| `astra_app_tests` | 0 files | Empty target | N/A |
| `astra_ui_color_tests` | 0 files | Empty target | N/A |
| `astra_ui_views_tests` | 0 files | Empty target | N/A |
| `astra_devtools_views_tests` | 2 test files | Source files exist, never run | `views::test::ViewsTestBase` |
| `astra_ui_accessibility_tests` | `astra_accessibility_utils_unittest.cc` | Substantial test file, never compiled | `views::test::ViewsTestBase` + gtest |
| `astra_ui_webui_tests` | 0 files | Empty target | N/A |
| `astra_browser_browsertests` | 0 files | Empty target | `InProcessBrowserTest` |
| `astra_app_browsertests` | 0 files | Empty target | `InProcessBrowserTest` |
| `astra_ui_views_interactive_uitests` | 0 files | Empty target | `InteractiveViewsTestApi` + `InProcessBrowserTest` |

### What's Currently Tested (in theory -- unvalidated)

From test source files that exist:

1. **Common layer** -- workspace types, command ID range checks, split view state,
   tab feature flags, UI constants
   - File: `chromium/astra/common/astra_common_unittest.cc`
   - 6 test categories planned (per BUILD.gn comment)

2. **Browser layer unit tests** -- 12 test files:
   - `astra_command_delegate_unittest.cc` -- command identity + observer pattern
   - `astra_favorite_service_unittest.cc` -- favorite folder CRUD
   - `astra_focus_mode_service_unittest.cc` -- focus mode state + blocklist
   - `astra_memory_saver_service_unittest.cc` -- tab suspend + settings
   - `astra_note_service_unittest.cc` -- note CRUD + workspace association
   - `astra_pip_service_unittest.cc` -- PiP state + size presets
   - `astra_reading_list_service_unittest.cc` -- reading list projection
   - `astra_screenshot_service_unittest.cc` -- screenshot capture + observers
   - `astra_session_restore_helper_unittest.cc` -- session metadata bridge
   - `astra_tab_features_unittest.cc` -- WebContentsUserData operations
   - `astra_workspace_import_export_unittest.cc` -- JSON validation + round-trip
   - `astra_workspace_service_unittest.cc` -- workspace CRUD + persistence

3. **Accessibility utils** -- 30+ test cases:
   - Null view safety (7 tests)
   - Property key defaults (6 tests)
   - Standalone view tests: name, role, focus, disabled, focus children
   - ApplyAstraAccessibleProperties tests: description, role, pressed, selected,
     expanded, live region, multiple properties
   - Keyboard navigation: arrow keys, home/end, enter/space, key release,
     unrecognized keys, null callbacks
   - Theme detection: high contrast, reduced motion
   - Many ViewsTestBase tests are commented out (need real Chromium headers)

4. **DevTools views** -- 2 test files:
   - `astra_devtools_toolbar_unittest.cc`
   - `astra_devtools_workspace_panel_unittest.cc`

### What's Missing

- **Zero browser tests** -- no end-to-end testing with real browser windows
- **Zero interactive UI tests** -- no testing of views inside real BrowserView
- **Zero WebUI tests** -- no testing of WebUI pages and message handlers
- **Zero color system tests** -- no tests for color mixer, color IDs, or accents
- **Zero views layer tests** -- sidebar, split view, command palette, etc.
  have no unit tests
- **No test execution** -- all test files are source-only; none have been
  compiled or run
- **No CI integration** -- no automated test runs on PRs or merges
- **No coverage tracking** -- no code coverage measurement

---

## Unit Tests

### What should be unit-tested

**High priority (should exist today):**

1. **Color system**
   - All `kColorAstra*` IDs are registered by the mixer
   - Light/dark mode produces expected color values
   - Accent color derivation produces valid contrast ratios (WCAG AA)
   - No color ID collisions with Chromium's built-in color space
   - `GetAstraAccentColor()` returns correct SkColor for each enum value

2. **Workspace service**
   - CRUD operations (add, rename, delete, reorder)
   - Default workspace invariant (always exists, cannot be deleted)
   - Active workspace switching
   - Persistence round-trip (save to prefs, load back)
   - Observer notifications fire correctly
   - Incognito behavior (redirected to original profile)

3. **Tab features**
   - `WebContentsUserData` attachment and retrieval
   - All metadata field getters/setters
   - `Reset()` clears all state
   - `ToggleFavorite()` flips state
   - Incognito read-only favorite behavior
   - Split view state transitions
   - Stack parent/child relationships

4. **Command delegate**
   - `IsAstraCommand()` range check boundaries
   - `ExecuteCommand()` for each command category
   - `IsCommandEnabled()` for all commands in various states
   - Observer registration/notification
   - Incognito command enable/disable rules

5. **Favorite service**
   - Folder CRUD (add, rename, delete, reorder)
   - Root folder invariant
   - Favorite move between folders
   - Folder expanded state toggling
   - Observer notifications

6. **Accessibility utils**
   - All utility functions with real views
   - `ApplyAstraAccessibleProperties()` end-to-end
   - Keyboard navigation helpers
   - Focus management with nested views

**Medium priority:**

7. **Session metadata** -- serialization/deserialization of Astra tab metadata
8. **Workspace import/export** -- JSON format validation, round-trip, error cases
9. **Focus mode service** -- toggle, duration, blocklist matching
10. **Memory saver service** -- suspend eligibility, timeout logic, observer pattern
11. **Note service** -- note CRUD, workspace association, persistence
12. **Reading list service** -- projection of Chromium ReadingListModel
13. **Screenshot service** -- capture types, observer notifications
14. **PiP service** -- state tracking, size presets

**Low priority:**

15. **Omnibox provider** -- suggestion generation, scoring
16. **New tab page service** -- shortcut data, workspace cards
17. **Theme service** -- accent color, light/dark mode
18. **Tab stack service** -- stack CRUD, tab membership

### Chromium test harnesses to use

| Test Type | Harness | Header | Target Dep |
|-----------|---------|--------|------------|
| Pure unit (no Chromium objects) | gtest/gmock | `testing/gtest/include/gtest/gtest.h` | `//testing/gtest`, `//testing/gmock` |
| Base utilities | `base::test::TaskEnvironment` | `base/test/task_environment.h` | `//base/test:test_support` |
| Profile-keyed services | `TestingProfile` | `chrome/test/base/testing_profile.h` | `//chrome/test:test_support` |
| WebContents / content | `content::TestWebContents` or `RenderViewHostTestHarness` | `content/public/test/test_web_contents.h` | `//content/public/test:test_support` |
| Views widgets | `views::test::ViewsTestBase` | `ui/views/test/views_test_base.h` | `//ui/views:views_test_support` |
| Color provider | `ui::ColorProvider` test utils | `ui/color/color_provider.h` | `//ui/color` |

### Unit test integration point

Tests should be added to Chromium's `unit_tests` test suite via a patch:
- Patch file: `chrome/test/BUILD.gn`
- Add `//astra:astra_tests` as a dependency of the `unit_tests` target
- Or use a separate `astra_unittests` target that can be run independently

---

## Browser Tests

### What needs browser tests

Browser tests use `InProcessBrowserTest` to spin up a real browser process with
real profiles, tab strips, and WebContents. They test integration between
Astra services and Chromium subsystems.

**High priority:**

1. **Startup / lifecycle**
   - Astra services are created at profile initialization
   - `AstraBrowserMainExtraParts` hooks fire in correct order
   - Default workspace exists on fresh profile
   - Services are accessible via factory `GetForProfile()`

2. **Sidebar integration**
   - `AstraSidebarView` is created and visible in branded builds
   - Sidebar has correct initial width and position
   - Sidebar updates when tabs are added/removed/moved
   - Workspace switching updates sidebar projection

3. **Workspace end-to-end**
   - Creating a workspace adds it to the service and sidebar
   - Deleting a workspace reassigns tabs to default
   - Workspace state persists across browser restart (session restore)
   - Multi-window workspace behavior

4. **Tab metadata survival**
   - Astra tab metadata survives navigation
   - Metadata survives tab discarding / restore
   - Metadata round-trips through session restore (patch 0006)

5. **Command integration**
   - Astra keyboard shortcuts trigger correct behavior
   - Commands route through `BrowserCommandController` → patch → delegate
   - Command enabled/disabled state matches UI state

**Medium priority:**

6. **Split view integration** -- two WebContents laid out side by side
7. **Focus mode end-to-end** -- toggles, UI changes, site blocking
8. **Favorite persistence** -- favorites survive session restore
9. **Note service integration** -- notes persist across browser sessions
10. **Incognito behavior** -- workspace list shared, mutations disabled

### Chromium test harness

- **Harness:** `InProcessBrowserTest`
- **Header:** `chrome/test/base/in_process_browser_test.h`
- **Target dep:** `//chrome/test:test_support`
- **Test suite:** `browser_tests` (via `chrome/test/BUILD.gn`)

### Browser test integration point

Add `//astra:astra_browser_tests` to Chromium's `browser_tests` target
dependencies. Tests should be in `*_browsertest.cc` files.

---

## Interactive UI Tests

### What needs interactive UI tests

Interactive UI tests exercise views inside a real browser window using
`InteractiveViewsTestApi`. They simulate user interactions (clicks, typing,
drag-and-drop) and verify UI state changes.

**High priority:**

1. **Sidebar interactions**
   - Clicking a tab in the sidebar activates it
   - Clicking a workspace switches the active workspace
   - Sidebar sections can be expanded/collapsed
   - Drag-and-drop between sidebar sections
   - Keyboard navigation within sidebar sections

2. **Command palette**
   - Opening/closing the palette
   - Typing filters the command list
   - Selecting a command executes it
   - Recent commands appear at top

3. **Split view**
   - Activating split view shows two panes
   - Resizing the split divider works
   - Swapping panes exchanges contents
   - Closing split view returns to single tab

4. **Workspace overview**
   - Opening overview shows all workspaces
   - Clicking a workspace switches and closes overview
   - Workspace cards show correct tab/window counts

**Medium priority:**

5. **Focus mode indicator** -- visibility, countdown, click-to-dismiss
6. **Screenshot capture** -- region selection, capture result bubble
7. **Settings bubble** -- opening, closing, toggling preferences
8. **Tab search** -- search, filter, activate tab

### Chromium test harnesses

- **Harness:** `InteractiveViewsTestApi` + `InProcessBrowserTest`
- **Headers:**
  - `chrome/test/base/in_process_browser_test.h`
  - `chrome/test/interaction/interactive_browser_test.h`
  - `ui/views/interaction/interactive_views_test.h`
- **Target deps:**
  - `//chrome/test:interactive_ui_test_support`
  - `//ui/views:views_test_support`
- **Test suite:** `interactive_ui_tests` (via `chrome/test/BUILD.gn`)

### Interactive UI test integration point

Add `//astra:astra_interactive_ui_tests` to Chromium's
`interactive_ui_tests` target dependencies.

---

## WebUI Tests

### What needs WebUI tests

1. **Astra New Tab Page**
   - Page loads correctly at `chrome://newtab` or `astra://newtab`
   - Workspace cards render with correct data
   - Shortcut tiles are clickable
   - Message handler round-trips work correctly

2. **Astra Settings (if WebUI-based)**
   - Settings page loads
   - Pref bindings work correctly
   - Changes persist and update UI

### Chromium test harness

- **Harness:** `WebUIBrowserTest` or `content::WebUIMessageHandler` unit tests
- **Headers:**
  - `chrome/test/base/web_ui_browser_test.h`
  - `content/public/test/test_web_ui.h`
- **Target deps:**
  - `//chrome/test:test_support`
  - `//content/public/test:test_support`

---

## Test Coverage Priorities

### Phase 1: Foundation (highest priority)

Goal: Get tests compiling and running in a Chromium checkout.

1. **Set up test infrastructure**
   - Integrate `astra_tests` into Chromium's `unit_tests` suite
   - Get existing test files to compile
   - Fix any compilation errors
   - Verify tests can run and pass

2. **Common layer tests**
   - Verify `astra_common_unittest.cc` compiles and passes
   - Add tests for all enum value boundaries
   - Add tests for type defaults and invariants

3. **Color mixer tests**
   - Add unit tests for `AddAstraColorMixer()`
   - Verify all `kColorAstra*` IDs are registered
   - Light/dark mode produces different colors where expected
   - Accent color derivation meets contrast requirements

4. **Accessibility utils tests**
   - Uncomment ViewsTestBase-based tests
   - Verify standalone tests pass
   - Add integration tests for `ApplyAstraAccessibleProperties`

### Phase 2: Service layer

Goal: Comprehensive unit test coverage for all Astra services.

1. **Workspace service tests** -- CRUD, persistence, observers, incognito
2. **Tab features tests** -- all metadata fields, WebContentsUserData pattern
3. **Command delegate tests** -- all commands, enabled/disabled states, observers
4. **Favorite service tests** -- folders, move operations, persistence
5. **Session restore tests** -- metadata round-trip through session storage

### Phase 3: Integration

Goal: End-to-end validation through browser tests.

1. **Startup / service creation** -- services exist at profile init
2. **Sidebar integration** -- sidebar visible and updates with tab changes
3. **Workspace end-to-end** -- create/switch/delete with real TabStripModel
4. **Command routing** -- keyboard shortcuts reach Astra delegate

### Phase 4: Interactive UI

Goal: User interaction testing for key workflows.

1. **Sidebar interactions** -- click to activate, workspace switching
2. **Command palette** -- open, search, execute
3. **Split view** -- activate, resize, swap, close

---

## Test Infrastructure Needed

### Build system integration

- Patch `chrome/test/BUILD.gn` to include `//astra:astra_tests` in
  the `unit_tests` target deps
- Patch `chrome/test/BUILD.gn` for browser_tests and interactive_ui_tests
- Or create standalone `astra_unittests`, `astra_browsertests`, and
  `astra_interactive_uitests` targets that can be run independently

### CI integration

- Add test steps to CI pipeline
- Run `unit_tests --gtest_filter="Astra*"` on every PR
- Run browser tests on nightly builds
- Track test coverage trends

### Test support libraries

- `TestingProfile` is already available via `//chrome/test:test_support`
- `content::TestWebContents` for tab features testing
- `views::test::ViewsTestBase` for views unit tests
- Mock objects for services (e.g., `MockAstraWorkspaceService`)
  - Currently no mocks are defined; services are used directly

### Test data / fixtures

- Test workspace data sets
- Test tab configurations (many tabs, few tabs, all in one workspace, etc.)
- Mock WebContents with configurable state
- Test profile with pre-populated prefs

---

## Testing Principles

1. **Test the Astra layer, not Chromium** -- We test that Astra code correctly
   projects and extends Chromium state. We don't test Chromium's own functionality.

2. **Service tests use TestingProfile** -- Profile-keyed services should be
   tested with `TestingProfile` from `//chrome/test:test_support`, not with
   manual PrefService setup.

3. **UI tests verify projection, not truth** -- Views tests verify that UI
   correctly reflects service state and dispatches commands. They don't verify
   service logic (that's what service unit tests are for).

4. **Prefer browser tests over mocks for integration** -- When testing how
   Astra integrates with Chromium subsystems, use `InProcessBrowserTest`
   rather than mocking out Chromium objects.

5. **All tests must work in both branded and non-branded builds** -- Tests
   should verify that Astra features are correctly gated by `BUILDFLAG(IS_ASTRA_BRANDED)`.

6. **Test names follow Chromium convention** -- `TestSuiteName.TestName`
   with CamelCase names, using `TEST_F()` for fixture-based tests.

---

## Test File Locations

```
chromium/astra/
├── common/
│   └── astra_common_unittest.cc          [exists]
├── browser/
│   ├── astra_command_delegate_unittest.cc [exists]
│   ├── astra_favorite_service_unittest.cc [exists]
│   ├── astra_focus_mode_service_unittest.cc [exists]
│   ├── astra_memory_saver_service_unittest.cc [exists]
│   ├── astra_note_service_unittest.cc    [exists]
│   ├── astra_pip_service_unittest.cc     [exists]
│   ├── astra_reading_list_service_unittest.cc [exists]
│   ├── astra_screenshot_service_unittest.cc [exists]
│   ├── astra_session_restore_helper_unittest.cc [exists]
│   ├── astra_tab_features_unittest.cc    [exists]
│   ├── astra_workspace_import_export_unittest.cc [exists]
│   ├── astra_workspace_service_unittest.cc [exists]
│   └── [add browsertest files here]       [missing]
├── app/
│   └── [add app browsertest files here]  [missing]
├── ui/
│   ├── color/
│   │   └── astra_color_mixer_unittest.cc  [planned, missing source]
│   ├── accessibility/
│   │   └── astra_accessibility_utils_unittest.cc [exists]
│   ├── views/
│   │   ├── [per-view unit tests]        [missing]
│   │   └── devtools/
│   │       ├── astra_devtools_toolbar_unittest.cc [exists]
│   │       └── astra_devtools_workspace_panel_unittest.cc [exists]
│   └── webui/
│       └── [WebUI test files]            [missing]
└── test/
    └── README.md                          [placeholder only]
```
