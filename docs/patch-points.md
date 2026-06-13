# Chromium Patch Points Reference

This document is a comprehensive reference of all Chromium patch points used by
the Astra browser project. It complements the detailed patch descriptions in
`chromium/astra/patches/` by providing a high-level overview of all patches,
their categories, and their relationship to Astra's architecture.

## Overview

Astra integrates with Chromium through a set of **minimal, build-flag-gated
patches** that delegate immediately to `//astra` code. No product logic lives
in patched Chromium files.

All patches are guarded by `BUILDFLAG(IS_ASTRA_BRANDED)` so that the default
Chromium build is unaffected when Astra is not being built.

Patch categories (from most to least acceptable):

1. **Registration hooks** — Adding Astra to a list of things that already
   exists. Lowest risk.
2. **Observer / delegate hooks** — Adding Astra as an observer or delegate in
   an existing observer pattern.
3. **Conditional forward calls** — `if (astra) astra::DoThing()`. Small
   if-statements that call into Astra.
4. **Behavioral modifications** — Changing existing Chromium behavior for
   Astra. Avoided unless absolutely necessary.

See ADR-0021 (Direct Chromium Patch Strategy) for the full patch philosophy
and rules.

## Patch Table

| ID | File(s) | Purpose | Category | Size | Status | Detail |
|----|---------|---------|----------|------|--------|--------|
| 0001 | `chrome/browser/chrome_browser_main.cc` | Register `AstraBrowserMainExtraParts` in Chrome startup | Registration | ~5 lines | implemented (.patch) | [0001-browser-main-extra-parts.md](../chromium/astra/patches/0001-browser-main-extra-parts.md) |
| 0002 | `chrome/browser/ui/views/frame/browser_view.cc` | Install `AstraBrowserView` after `BrowserView` construction | Registration | ~10 lines | implemented (.patch) | [0002-browser-view-install.md](../chromium/astra/patches/0002-browser-view-install.md) |
| 0003 | `chrome/browser/ui/browser_command_controller.cc` | Forward Astra command IDs to `AstraCommandDelegate` | Conditional forward | ~10 lines | implemented (.patch) | [0003-command-forwarding.md](../chromium/astra/patches/0003-command-forwarding.md) |
| 0004 | `chrome/browser/BUILD.gn` | Include `//astra:astra_browser` in the build graph | Build config | ~3 lines | implemented (.patch) | [0004-build-gn-include.md](../chromium/astra/patches/0004-build-gn-include.md) |
| 0005 | `chrome/common/chrome_constants.cc` + theme files | Wire Astra branding constants and icons into Chromium | Registration | ~20 lines | documented | [0005-astra-branding.md](../chromium/astra/patches/0005-astra-branding.md) |
| 0006 | `chrome/browser/sessions/*` | Attach Astra tab metadata to session restore | Conditional forward | ~25 lines | documented | [0006-session-restore-metadata.md](../chromium/astra/patches/0006-session-restore-metadata.md) |
| 0007 | `chrome/browser/ui/views/accelerator_table.cc` | Merge Astra accelerators into Chrome's accelerator table | Registration | ~15 lines | documented | [0007-accelerator-table.md](../chromium/astra/patches/0007-accelerator-table.md) |
| 0008 | `chrome/app/chrome_command_ids.h` | Reserve Astra command ID range (60000+) | Registration | ~5 lines | documented | [0008-command-id-range.md](../chromium/astra/patches/0008-command-id-range.md) |
| 0009 | `chrome/browser/ui/views/profiles/profile_menu_view.cc` | Add workspace section to profile menu | Observer/delegate | ~15 lines | documented | [0009-profile-menu-workspaces.md](../chromium/astra/patches/0009-profile-menu-workspaces.md) |
| 0010 | `chrome/browser/ui/views/location_bar/location_bar_view.cc` | Add workspace decoration to omnibox location bar | Observer/delegate | ~25 lines | documented | [0010-location-bar-decoration.md](../chromium/astra/patches/0010-location-bar-decoration.md) |
| 0011 | `components/omnibox/browser/autocomplete_controller.cc` | Inject Astra suggestions into omnibox autocomplete | Observer/delegate | ~40 lines | documented | [0011-omnibox-astra-provider.md](../chromium/astra/patches/0011-omnibox-astra-provider.md) |

**Status legend:**
- **implemented (.patch)** — A `.patch` file exists and can be applied to a Chromium checkout.
- **documented** — Patch description exists in `chromium/astra/patches/` but no `.patch` file yet.

## Patches by Feature Area

### Startup & Build

| Patch | Feature |
|-------|---------|
| 0004 | Build system — makes `//astra` part of the Chrome build graph |
| 0005 | Branding — Astra product name, icons, constants |
| 0001 | Browser main — registers Astra's `BrowserMainExtraParts` |

These patches are the foundation. Without them, Astra code does not compile
or run.

### Command System

| Patch | Feature |
|-------|---------|
| 0008 | Command ID range reservation (60000+) |
| 0007 | Accelerator table — Astra keyboard shortcuts |
| 0003 | Command forwarding — routes Astra commands to `AstraCommandDelegate` |

These patches enable Astra's command system. The command ID range ensures no
collisions with Chrome's built-in commands. The accelerator table patch
registers Astra's keyboard shortcuts. The command forwarding patch routes
Astra-range commands to Astra's command delegate.

### Browser UI

| Patch | Feature |
|-------|---------|
| 0002 | BrowserView installation — `AstraBrowserView` augments `BrowserView` |
| 0009 | Profile menu — workspace section in avatar menu |
| 0010 | Location bar — workspace decoration in omnibox |

These patches inject Astra UI into Chrome's browser window. The BrowserView
installation is the primary integration point for Astra's sidebar and other
browser chrome additions.

### Omnibox

| Patch | Feature |
|-------|---------|
| 0011 | Omnibox Astra provider — injects Astra suggestions into autocomplete |

This patch adds Astra-specific omnibox suggestions (workspace switch, tab
search, Astra commands) to the Chrome omnibox.

### Session & Persistence

| Patch | Feature |
|-------|---------|
| 0006 | Session restore metadata — attaches Astra tab metadata to session restore |

This patch ensures Astra metadata (workspace ID, favorite state, split view
config) survives session restore and browser restart.

## Planned Future Patches

The following patches are anticipated but not yet scoped or documented:

- **TabStripModel observer hook** — For sidebar projection updates. May not
  be needed if `AstraBrowserView` can attach a `TabStripModelObserver` after
  construction via patch 0002.
- **Profile keyed service factory registration** — Currently handled by
  `AstraBrowserMainExtraParts::PreProfileInit()` in patch 0001. May need a
  separate patch if additional registration points are required.
- **WebContentsUserData attachment point** — For `AstraTabFeatures`. May be
  handled by `TabStripModelObserver` in the browser layer rather than a
  Chromium patch.
- **Side panel integration** — For Astra sidebar surfaces that want to use
  Chrome's side panel framework. Currently Astra builds its own sidebar
  outside the side panel system.
- **Color mixer registration** — For `AstraColorMixer` integration with
  Chromium's ColorProvider pipeline. Natural patch point:
  `chrome/browser/ui/color/chrome_color_mixers.cc`.
- **DevTools integration** — For `AstraDevToolsIntegration` (toolbar injection,
  custom panel registration). Patch points in
  `chrome/browser/devtools/devtools_window.cc` and `devtools_ui_bindings.cc`.

## Patch Lifecycle

### Adding a New Patch

1. **Identify the hook point.** Find the right place in Chromium to inject
   Astra code. Prefer registration hooks and observer patterns over behavioral
   modifications.
2. **Write the patch detail file.** Copy `PATCH_TEMPLATE.md` to
   `NNNN-patch-name.md` and fill in all sections. Include exact lines,
   surrounding context, rationale, alternatives, and rebase risks.
3. **Add to the table.** Update `chromium/astra/patches/README.md` and
   `docs/patch-points.md` with the new patch.
4. **Implement in Chromium.** Apply the change in a local Chromium checkout
   and verify it builds.
5. **Generate .patch file.** Export the diff as a `.patch` file in
   `chromium/astra/patches/`.
6. **Review.** Patch review checks: tininess, delegation, gating, alternatives.

### Rebase Workflow

When updating Chromium to a new revision:

1. **Apply patches in order.** Start from a clean Chromium checkout.
2. **Resolve conflicts one by one.** For each conflict:
   - If trivial (line number shift), rebase and continue.
   - If substantive, try to move logic to `//astra` to eliminate the patch.
   - If the patch point no longer exists, find a new hook.
   - If no reasonable hook exists, escalate to an ADR.
3. **Update .patch files.** Regenerate `.patch` files from the rebased changes.
4. **Update detail files.** Update patch detail files with new context and
   any rebase notes.
5. **Run tests.** Verify the build passes and all Astra features work.

See `chromium/astra/patches/README.md` for the full conflict resolution
strategy.

## Architecture Principles

All patches follow these principles (from ADR-0021):

- **Minimal:** Each patch should be 1-50 lines. Larger patches mean logic
  should move to `//astra`.
- **Delegation only:** Patches call into `//astra` and do nothing else.
  No product logic in patched files.
- **Build-flag gated:** All Astra code paths in Chromium files are guarded
  by `BUILDFLAG(IS_ASTRA_BRANDED)`.
- **Documented:** Every patch has a detail file explaining what, why, and
  alternatives.
- **Numbered:** Patches are numbered sequentially in suggested application
  order.

## Related Documentation

- **ADR-0021: Direct Chromium Patch Strategy** — Philosophical foundation for
  the patch approach.
- **`chromium/astra/patches/README.md`** — Full patch queue with table and
  process.
- **`chromium/astra/patches/NNNN-*.md`** — Individual patch detail files.
- **`chromium/astra/BUILD.gn`** — Build targets and patch point list.
- **`docs/ARCHITECTURE.md`** — Overall architecture diagram showing how
  patches connect to the Astra overlay.
