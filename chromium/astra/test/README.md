# Astra Test Infrastructure

This directory describes the test infrastructure for Astra Chromium code.

Test files live alongside the code they test (per-layer `*_unittest.cc` and
`*_browsertest.cc` files).  This directory serves as documentation and a
place for shared test helpers.

## Test Structure

Astra follows Chromium's test layering model.  Each layer has its own test
target and uses the appropriate Chromium test harness.

### Layer 1: Unit Tests (`*_unittest.cc`)

Fast, lightweight tests that don't require a full browser process.

| Layer | Test Target | Harness | Example |
|-------|-------------|---------|---------|
| `//astra/browser` | `astra_browser_unit_tests` | `TestingProfile` + gtest | Workspace service CRUD, tab features defaults, command identity |
| `//astra/app` | `astra_app_tests` | gtest + base test support | Feature flag defaults, main delegate registration |
| `//astra/ui/views` | `astra_ui_views_tests` | `views_test_support` + `ViewsTestBase` | Sidebar item rendering, workspace switcher layout |

Unit tests verify:
- Service logic in isolation
- Data model invariants
- Observer notification patterns
- API contracts and edge cases

### Layer 2: Browser Tests (`*_browsertest.cc`)

Tests that require a real Browser, TabStripModel, and WebContents.

| Layer | Test Target | Harness |
|-------|-------------|---------|
| `//astra/browser` | `astra_browser_browsertests` (future) | `InProcessBrowserTest` |

Browser tests verify:
- Integration with Chromium Browser, TabStripModel, and WebContents
- ProfileKeyedService factory wiring
- Command execution end-to-end
- Tab metadata persistence across navigation

### Layer 3: Interactive UI Tests (`*_interactive_uitest.cc`)

Tests that drive the Views UI through user interaction simulation.

| Layer | Test Target | Harness |
|-------|-------------|---------|
| `//astra/ui/views` | `astra_ui_views_interactive_uitests` (future) | `InteractiveViewsTestApi` + `InProcessBrowserTest` |

Interactive UI tests verify:
- Sidebar visibility and layout
- Workspace switcher click behavior
- Split view resize and orientation changes
- Command palette keyboard navigation

### Layer 4: End-to-End Tests (E2E)

Full browser tests with real user scenarios.  These use Chromium's
`browser_tests` infrastructure or platform-specific test frameworks.

E2E tests verify:
- Full user workflows (create workspace, add tab, switch, etc.)
- Cross-feature interactions
- Performance and stability regressions

## Chromium Test Harnesses

Astra reuses Chromium's test infrastructure rather than building custom test
frameworks.  Each harness maps to a specific Chromium subsystem:

### `//testing/gtest`
Google Test framework.  All tests use TEST_F, TEST, ASSERT_*, and EXPECT_*.

### `//base/test:test_support`
Base test utilities including `TaskEnvironment` for posting tasks and
`ScopedFeatureList` for feature flag overrides.

### `//chrome/test:test_support` / `TestingProfile`
Test-only Profile implementation for profile-keyed service tests.
Provides PrefService, KeyedService factories, and profile lifecycle.
Used by `AstraWorkspaceService` and `AstraFavoriteService` tests.

### `//content/public/test:test_support`
Content layer test utilities including `TestWebContentsFactory` and
`WebContentsTester`.  Used by `AstraTabFeatures` tests (WebContentsUserData).

### `//ui/views:views_test_support`
Views test utilities including `ViewsTestBase` and widget test helpers.
Used by sidebar and split view widget tests.

### `//chrome/test/base:in_process_browser_test`
Full browser test harness that starts a real Browser with TabStripModel,
WebContents, and profile.  Used for browser_tests and interactive_ui_tests.

## Running Tests

Tests are built with GN/Ninja, same as the rest of Chromium.

### Build all Astra tests

```bash
autoninja -C out/astra_Debug astra_tests
```

### Build a specific test target

```bash
autoninja -C out/astra_Debug astra_browser_unit_tests
autoninja -C out/astra_Debug astra_ui_views_tests
```

### Run unit tests

```bash
out/astra_Debug/unit_tests --gtest_filter="Astra*"
```

Or run the standalone test binary (if built as a separate target):

```bash
out/astra_Debug/astra_browser_unit_tests
```

### Run browser tests

```bash
out/astra_Debug/browser_tests --gtest_filter="Astra*"
```

### Run interactive UI tests

```bash
out/astra_Debug/interactive_ui_tests --gtest_filter="Astra*"
```

## Current Test Files

### Unit tests

| File | Status | Notes |
|------|--------|-------|
| `astra/browser/astra_workspace_service_unittest.cc` | Complete | 25+ test cases.  Uses TestingProfile. |
| `astra/browser/astra_tab_features_unittest.cc` | Skeleton | 10 test cases.  Needs content test harness. |
| `astra/browser/astra_command_delegate_unittest.cc` | Partial | 12+ test cases.  Identity tests run; execution tests need Browser. |
| `astra/browser/astra_favorite_service_unittest.cc` | Complete | 25+ test cases.  Folder CRUD, hierarchy, observers. |
| `astra/browser/astra_note_service_unittest.cc` | Complete | 35+ test cases.  Note CRUD, search, URL/workspace filtering, observers. |
| `astra/browser/astra_workspace_import_export_unittest.cc` | Partial | 25+ test cases.  JSON validation, URL safety, limits. |
| `astra/ui/accessibility/astra_accessibility_utils_unittest.cc` | Skeleton | 20+ test cases.  Null safety, keyboard nav, theme detection. |

### TODO(astra): Not yet implemented

- `astra/browser/astra_focus_mode_service_unittest.cc` — focus session lifecycle, blocklist
- `astra/browser/astra_memory_saver_service_unittest.cc` — memory saver policy logic
- `astra/browser/astra_tab_stack_service_unittest.cc` — tab stack operations
- `astra/browser/astra_prefs_unittest.cc` — pref key registration and defaults
- `astra/browser/astra_pip_service_unittest.cc` — PiP state tracking
- `astra/browser/astra_screenshot_service_unittest.cc` — screenshot capture metadata
- `astra/browser/astra_reading_list_service_unittest.cc` — reading list projection
- `astra/app/astra_feature_list_unittest.cc` — feature flag defaults
- `astra/app/astra_browser_main_extra_parts_unittest.cc` — lifecycle hooks
- `astra/ui/views/astra_sidebar_view_unittest.cc` — sidebar view layout
- `astra/ui/views/astra_workspace_switcher_view_unittest.cc` — switcher widget
- `astra/ui/views/astra_split_view_unittest.cc` — split view widget
- `astra/browser/astra_workspace_service_browsertest.cc` — full browser integration
- `astra/browser/astra_command_delegate_browsertest.cc` — command execution E2E
- `astra/browser/astra_favorite_service_browsertest.cc` — favorite + tab integration
- `astra/browser/astra_note_service_browsertest.cc` — note persistence E2E
- `astra/browser/astra_workspace_import_export_browsertest.cc` — import/export E2E

## Test Guidelines

- **Prefix test files with `astra_`** and suffix with `_unittest.cc` or
  `_browsertest.cc` following Chromium conventions.
- **Use TEST_F** for test fixtures, **TEST** for standalone tests.
- **Use ASSERT_\*** for preconditions, **EXPECT_\*** for test assertions.
- **Follow Chromium C++ style** — `snake_case` for local variables and
  methods, `kCamelCase` for constants.
- **Use `TODO(astra):` format** for all todo comments and name the
  Chromium component or patch point they relate to.
- **Never reimplement test infrastructure** that Chromium already provides.
  Use `TestingProfile`, `WebContentsTester`, `ViewsTestBase`, etc.
- **UI tests must not be the source of truth** for product state.  They
  read from services and verify presentation.

## Adding a New Test

1. Create the test file in the appropriate layer directory.
2. Add the file to the `sources` list of the layer's test target in BUILD.gn.
3. If the test needs a new test dependency, add it to `deps`.
4. Verify the test compiles and passes with `autoninja` + test binary.
5. Run `pnpm check:architecture` to ensure no architectural violations.

## Integration with Chromium Test Suites

TODO(astra): Wire Astra test targets into Chromium's top-level test suites.

- `unit_tests` — add `astra_browser_unit_tests` and `astra_app_tests` sources
- `browser_tests` — add Astra browser test sources
- `interactive_ui_tests` — add Astra views interactive test sources

Patch point: `//chrome/test/BUILD.gn` test suite definitions.
The `astra_tests` aggregate target should be listed as a dependency of the
appropriate test suite.
