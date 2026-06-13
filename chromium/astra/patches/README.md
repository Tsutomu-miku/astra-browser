# Chromium Patch Queue

This directory documents every patch Astra applies to the Chromium source tree.
Each patch has two files:
- **`.patch`** — a unified diff that can be applied with `git apply` or `patch -p1`
- **`.md`** — detailed description: what changes, why, alternatives, risks

## Philosophy

**Tiny patches, delegate to `//astra`.**

Every Chromium patch should follow these rules:

- **Minimal change.** A patch should add as few lines as possible to Chromium
  source files. Ideally 1-10 lines per file.
- **Delegate immediately.** Patch code should call into `//astra` and do
  nothing else. No product logic, no state, no computation in patched files.
- **Build-flag gated.** All Astra-specific code paths in Chromium files are
  guarded by `BUILDFLAG(IS_ASTRA_BRANDED)` or equivalent, so upstream Chromium
  behavior is unchanged when the flag is off.
- **Documented.** Every patch has an entry in the table below and a detail
  file (`NNNN-patch-name.md`) explaining the change, the reason, and
  alternative approaches considered.
- **Reviewable.** Each patch must be small enough to review in a single
  sitting. If a patch grows beyond ~50 lines, split it or reconsider whether
  the logic belongs in `//astra` instead.

## Patch Table

Patches are numbered in suggested application order. Size estimates are
approximate line counts in Chromium files.

| ID | File | Purpose | Size | Requirement | Status | Detail | Patch |
|----|------|---------|------|-------------|--------|--------|-------|
| 0001 | `chrome/browser/chrome_browser_main.cc` | Register `AstraBrowserMainExtraParts` in Chrome startup | ~5 lines | **Hard** | planned | [0001-browser-main-extra-parts.md](0001-browser-main-extra-parts.md) | [0001-browser-main-extra-parts.patch](0001-browser-main-extra-parts.patch) |
| 0002 | `chrome/browser/ui/views/frame/browser_view.cc` | Install `AstraBrowserView` after `BrowserView` construction | ~15 lines | **Hard** | planned | [0002-browser-view-install.md](0002-browser-view-install.md) | [0002-browser-view-install.patch](0002-browser-view-install.patch) |
| 0003 | `chrome/browser/ui/browser_command_controller.cc` | Forward Astra command IDs to `AstraCommandDelegate` | ~10 lines | **Hard** | planned | [0003-command-forwarding.md](0003-command-forwarding.md) | [0003-command-forwarding.patch](0003-command-forwarding.patch) |
| 0004 | `chrome/browser/BUILD.gn` + `build/config/chrome_build.gni` | Include `//astra:astra_browser` in the build graph | ~10 lines | **Hard** | planned | [0004-build-gn-include.md](0004-build-gn-include.md) | [0004-build-gn-include.patch](0004-build-gn-include.patch) |
| 0005 | `chrome/common/chrome_constants.cc` + theme | Wire Astra branding constants and icons into Chromium | ~20 lines | Nice-to-have | planned | [0005-astra-branding.md](0005-astra-branding.md) | [0005-astra-branding.patch](0005-astra-branding.patch) |
| 0006 | `chrome/browser/sessions/*` | Attach Astra tab metadata to session restore | ~25 lines | Nice-to-have | planned | [0006-session-restore-metadata.md](0006-session-restore-metadata.md) | [0006-session-restore-metadata.patch](0006-session-restore-metadata.patch) |
| 0007 | `chrome/browser/ui/views/accelerator_table.cc` | Merge Astra accelerators into Chrome's accelerator table | ~15 lines | **Hard** | planned | [0007-accelerator-table.md](0007-accelerator-table.md) | [0007-accelerator-table.patch](0007-accelerator-table.patch) |
| 0008 | `chrome/app/chrome_command_ids.h` | Reserve Astra command ID range (60000+) | ~5 lines | **Hard** | planned | [0008-command-id-range.md](0008-command-id-range.md) | [0008-command-id-range.patch](0008-command-id-range.patch) |
| 0009 | `chrome/browser/ui/views/profiles/profile_menu_view.cc` | Add workspace section to profile menu | ~15 lines | Nice-to-have | planned | [0009-profile-menu-workspaces.md](0009-profile-menu-workspaces.md) | [0009-profile-menu-workspaces.patch](0009-profile-menu-workspaces.patch) |
| 0010 | `chrome/browser/ui/views/location_bar/location_bar_view.cc` | Add workspace decoration to omnibox location bar | ~25 lines | Nice-to-have | planned | [0010-location-bar-decoration.md](0010-location-bar-decoration.md) | [0010-location-bar-decoration.patch](0010-location-bar-decoration.patch) |
| 0011 | `components/omnibox/browser/autocomplete_controller.cc` | Inject Astra suggestions into omnibox autocomplete | ~40 lines | Nice-to-have | planned | [0011-omnibox-astra-provider.md](0011-omnibox-astra-provider.md) | [0011-omnibox-astra-provider.patch](0011-omnibox-astra-provider.patch) |
| 0012 | `chrome/browser/ui/color/chrome_color_mixers.cc` | Add Astra ColorProvider mixer for product color tokens | ~10 lines | **Hard** | planned | [0012-color-mixer-integration.md](0012-color-mixer-integration.md) | [0012-color-mixer-integration.patch](0012-color-mixer-integration.patch) |
| 0013 | `chrome/browser/chrome_content_browser_client.cc` | Install Astra ContentBrowserClient hooks (web prefs, URL policy) | ~15 lines | Nice-to-have | planned | [0013-content-browser-client-hooks.md](0013-content-browser-client-hooks.md) | [0013-content-browser-client-hooks.patch](0013-content-browser-client-hooks.patch) |
| 0014 | `chrome/app/chrome_main_delegate.cc` | Pre-sandbox and early startup hooks for Astra | ~15 lines | Nice-to-have | planned | [0014-chrome-main-delegate-hooks.md](0014-chrome-main-delegate-hooks.md) | [0014-chrome-main-delegate-hooks.patch](0014-chrome-main-delegate-hooks.patch) |
| 0015 | `chrome/browser/devtools/devtools_window.h` | Expose DevTools dock state for Astra command enabled/disabled | ~10 lines | Nice-to-have | planned | [0015-devtools-dock-state.md](0015-devtools-dock-state.md) | [0015-devtools-dock-state.patch](0015-devtools-dock-state.patch) |

**Planned future patches (not yet scoped / not yet created):**

- New tab page redirect (`chrome/browser/new_tab_page/new_tab_page_url_handler.cc`)
  — redirect `chrome://newtab` to the Astra NTP or Views-based NTP surface.
  Can also be handled by `Browser::NewTab()` in `chrome/browser/ui/browser.cc`.

- TabStripModel observer hook for sidebar projection updates
  — may not need a patch if AstraSidebarView can observe TabStripModel directly
  via the public `TabStripModelObserver` interface and `AddObserver()`.

- Profile keyed service factory registration (currently handled by
  `AstraBrowserMainExtraParts::PreProfileInit()` in patch 0001).

- WebContentsUserData attachment point for AstraTabFeatures
  — may not need a patch if `WebContentsUserData<T>::CreateForWebContents()`
  works from `//astra` code without Chromium modifications.

- Side panel integration for Astra sidebar surfaces
  — Chromium's `SidePanelRegistry` could be used instead of a custom sidebar.

- Test suite integration (`chrome/test/BUILD.gn`)
  — add `astra_tests` to the appropriate test suite targets (unit_tests,
  browser_tests, interactive_ui_tests).

## Hard Requirements vs Nice-to-Have

### Hard requirements (must apply for Astra to function)

These patches are essential for the core Astra product experience. Without them,
Astra UI surfaces cannot be created, commands cannot be dispatched, or the
build will fail.

- **0001 — Browser main extra parts**: Registers Astra's startup hooks.
  Without this, no Astra services or keyed-service factories are initialized.

- **0002 — Browser view install**: Creates `AstraBrowserView` and installs
  all Astra UI surfaces (sidebar, command palette, etc.) into the browser
  window. Without this, there is no Astra UI.

- **0003 — Command forwarding**: Routes Astra-range command IDs through
  Chrome's command controller to `AstraCommandDelegate`. Without this,
  keyboard shortcuts and menu items for Astra commands do nothing.

- **0004 — Build GN include**: Pulls `//astra` into the build graph.
  Without this, no Astra code is compiled and all other patches will
  cause build failures (missing headers).

- **0007 — Accelerator table**: Registers Astra keyboard shortcuts with
  Chrome's FocusManager. Without this, Astra commands are only reachable
  via the command palette or programmatic calls.

- **0008 — Command ID range**: Reserves the 60000+ command ID range.
  Without this, Astra command IDs may collide with Chrome's built-in
  commands or dynamic label IDs.

- **0012 — Color mixer**: Registers Astra color tokens with the
  ColorProvider. Without this, Views code that calls
  `GetColor(kColorAstra*)` will fail (colors not registered).

### Nice-to-have (optional, can defer)

These patches enhance the product but are not essential for a functional
Astra build. They can be applied later or skipped for development builds.

- **0005 — Branding**: Changes product name from "Chromium" to "Astra Browser".
  Can ship without it; the product will just say "Chromium" everywhere.

- **0006 — Session restore metadata**: Saves/restores Astra tab metadata
  (workspace ID, favorite state, split view config) across browser restarts.
  Without this, tabs lose their Astra metadata on restart (workspace
  assignment resets to default, favorites are lost, etc.). Functionality
  still works during the session.

- **0009 — Profile menu workspaces**: Adds workspace switching to the
  profile menu. Without this, workspaces are only accessible via the
  sidebar and keyboard shortcuts.

- **0010 — Location bar decoration**: Adds workspace accent indicator
  to the omnibox. Purely cosmetic / UX polish.

- **0011 — Omnibox Astra provider**: Injects Astra suggestions (workspace
  switch, tab search, commands) into the omnibox autocomplete. Without
  this, the command palette is the only way to discover Astra commands
  by typing.

- **0013 — Content browser client hooks**: Web preference overrides and
  URL policy for Astra surfaces. Only needed if we have Astra-specific
  web views with custom requirements.

- **0014 — Chrome main delegate hooks**: Pre-sandbox and early startup
  hooks. Most Astra initialization happens in BrowserMainExtraParts
  (patch 0001), so this is only needed for things that must run before
  the sandbox is engaged.

- **0015 — DevTools dock state**: Exposes dock state querying for
  `AstraDevToolsHelper`. Without this, DevTools toggle still works
  (uses public `ToggleDevToolsWindow`), but dock-side commands may
  not show correct enabled/disabled state.

## Patches That Could Be Replaced

Some patches exist because no cleaner integration point exists in Chromium
today. If Chromium adds better APIs or extension points in the future,
these patches could be removed:

| Patch | Cleaner alternative | Notes |
|-------|---------------------|-------|
| 0002 (BrowserView install) | `BrowserViewObserver` or widget lifecycle API | If Chromium adds a browser window lifecycle observer, Astra could install UI without patching BrowserView directly. |
| 0003 (Command forwarding) | `CommandUpdater::RegisterCommandHandler()` callback | If Chrome's command system supported runtime command registration with callbacks, Astra could register commands dynamically instead of patching the dispatch path. |
| 0007 (Accelerator table) | `FocusManager` runtime accelerator registration API | Runtime registration is partially possible via `FocusManager::RegisterAccelerator()`, but menu tooltips and standard shortcut display would still need the table. |
| 0011 (Omnibox provider) | Extension omnibox API or provider plugin system | The extension `chrome.omnibox` API works but has limitations. A proper provider plugin system would be cleaner. |
| 0012 (Color mixer) | `ColorProvider::AddMixer()` from anywhere | If ColorProvider supported dynamic mixer registration (e.g., via a registry), Astra colors could self-register without a patch. |
| 0015 (DevTools dock state) | Public `DevToolsWindow::dock_side()` API | If Chromium makes dock state querying public, the patch is unnecessary. |

## How to Apply Patches

Patches are applied as part of the Chromium bootstrap and update flow:

```bash
# Bootstrap a fresh Chromium checkout and apply all patches
./scripts/chromium-bootstrap.sh

# Re-apply patches after a Chromium rebase
./scripts/apply-astra-patches.sh  # TODO(astra): create this script
```

Each patch is a human-readable description file (`.md`) and a machine-readable
diff file (`.patch`) in this directory. The `.patch` files use unified diff
format and can be applied with `git apply` or `patch -p1`.

The overlay sync (`chromium/astra/` -> `chromium/src/astra/`) happens before
patch application so that `//astra` headers and source files are available
when patches reference them.

## How to Review Patches

When reviewing a new or modified patch:

1. **Check the detail file.** Does it explain the what, why, and where?
2. **Verify tininess.** Is the patch under 50 lines? If not, can more logic
   move to `//astra`?
3. **Check gating.** Is the change behind a build flag or runtime check?
4. **Check delegation.** Does the patch call into `//astra` immediately, or
   does it contain product logic?
5. **Check alternatives.** Does the detail file explain why this patch point
   is the right one?
6. **Check the .patch file.** Is it valid unified diff format? Does it apply
   cleanly to the target Chromium revision?

## Conflict Resolution

When updating Chromium to a new revision, patches may conflict. The resolution
strategy is:

1. **Rebase cleanly first.** If the conflict is trivial (line number shift,
   nearby refactor), rebase the patch and update the detail file.
2. **Move logic to `//astra`.** If the conflict is substantive, see if the
   logic can move entirely into Astra code, eliminating the patch.
3. **Find a new patch point.** If the old patch point no longer exists, find
   a different hook in Chromium and update the patch ID and detail file.
4. **Escalate to ADR.** If no reasonable patch point exists and the feature
   cannot be implemented from `//astra`, write an ADR to discuss the approach
   before writing code.
5. **Never grow a patch to work around a conflict.** If the patch would need
   to expand significantly, step back and reconsider the architecture.

## Patch Detail Files

Each patch has two files:

- **`NNNN-patch-name.patch`** — A unified diff that can be applied with
  `git apply` or `patch -p1`. Contains the actual code changes.
- **`NNNN-patch-name.md`** — A detailed description following the
  `PATCH_TEMPLATE.md` template. Contains:
  - The exact lines to change in the Chromium file
  - Surrounding context (3-5 lines before and after)
  - The full rationale
  - Alternatives considered
  - Known risks and rebase concerns

## Adding a New Patch

1. Copy `PATCH_TEMPLATE.md` to `NNNN-patch-name.md`.
2. Fill in all sections of the .md file.
3. Generate the corresponding `.patch` file from a local Chromium checkout.
4. Add an entry to the table above.
5. Apply the change in a local Chromium checkout and verify it builds.
6. Submit for review.

## File Naming Convention

- **Number prefix:** Four-digit zero-padded sequence number (`0001-`, `0002-`, etc.)
- **Descriptive name:** Hyphenated lowercase name of the patch point
- **Extensions:** `.md` for description, `.patch` for the diff

Example: `0003-command-forwarding.md` and `0003-command-forwarding.patch`
