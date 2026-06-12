# Roadmap

## Phase 0: Architecture Reset

Status: complete.

- Remove CEF/CMake/native-shell scaffold.
- Add direct Chromium overlay under `chromium/astra/`.
- Accept ADR-0009 for direct Chromium.
- Add architecture guardrails and legacy doc archive.

## Phase 1: Chromium Checkout Integration

Goal: build Chromium with the Astra overlay present.

- Bootstrap Chromium with depot_tools.
- Sync `chromium/astra/` to `chromium/src/astra`.
- Add the smallest Chromium patch to include `//astra`.
- Build `chrome` with GN/Ninja.
- Add an architecture smoke test that verifies `//astra:astra_browser` is in the
  generated build graph.

Exit criteria:

- `autoninja -C out/astra_Debug chrome` passes.
- Patch queue is documented and minimal.
- No CEF/Electron/CMake runtime code is introduced.

## Phase 2: BrowserView Integration

Goal: show an Astra sidebar inside Chromium's desktop UI without replacing
Chrome browser ownership.

- Register `AstraBrowserMainExtraParts`.
- Construct `AstraBrowserView` from a Chromium `BrowserView` patch point.
- Render `AstraSidebarView` from Chromium `TabStripModel` plus Astra metadata.
- Keep toolbar, content area, DevTools, WebUI, downloads, and profiles
  Chromium-owned.

Exit criteria:

- A Chromium build launches with visible Astra sidebar shell.
- New tab, close tab, navigation, DevTools, history, downloads, and extensions
  still use Chrome infrastructure.

## Phase 3: Workspace Semantics

Goal: implement Astra Spaces as product metadata.

- Implement `AstraWorkspaceService` as a `ProfileKeyedService`.
- Attach workspace/favorite/split metadata through `AstraTabFeatures`.
- Project Chromium tabs into sidebar sections without moving tab ownership out of
  `TabStripModel`.
- Migrate P0 tab identity behavior from legacy tests into Chromium tests.

Exit criteria:

- Space switching is metadata projection, not separate browser runtime state.
- Favorites preserve tab identity.
- Split/Glance use Chromium WebContents.

## Phase 4: Product Parity

Goal: make the direct Chromium build usable as the main Astra shell.

- Command palette delegates standard work to Chrome commands.
- Settings/history/downloads/passwords/extensions use Chrome WebUI/components
  first.
- Implement Split/Glance Views polish.
- Add session metadata persistence through Chromium profile/session mechanisms.

Exit criteria:

- Daily navigation workflows run in direct Chromium.
- Legacy Electron prototype is no longer needed for core dogfooding.

## Phase 5: Legacy Retirement

Goal: remove the old prototype after direct Chromium is the working product.

- Delete or archive `src/` runtime code.
- Replace Electron packaging with Chromium signing/notarization/release flow.
- Move final tests to Chromium unit/browser/ui suites.

Exit criteria:

- No Electron dependency in production build.
- Release artifacts are Chromium-built Astra packages.
