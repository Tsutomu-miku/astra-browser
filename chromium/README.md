# Astra Direct Chromium Overlay

This directory (`chromium/`) contains the Astra overlay that is synced into a
Chromium checkout as `chromium/src/astra/`. It is not a standalone project and
it is not built by CMake. The target build system is Chromium GN/Ninja.

## The Overlay Pattern

Astra is developed as an overlay on top of Chromium. Here is how it works:

```
astra-browser repo                       Chromium checkout
================                         ==================

chromium/astra/       --(sync)-->        src/astra/
  BUILD.gn                               BUILD.gn
  app/                                   app/
  browser/                               browser/
  ui/views/                              ui/views/
  patches/                               (patch tooling)

chromium/astra/patches/ --(apply)-->     src/chrome/... (small patches)
                                         src/build/...
                                         src/ui/...
```

The overlay is the primary source of Astra product code. It is developed in
this repository and copied into a Chromium checkout by build scripts. Chromium
source files receive tiny registration/delegation patches that call into
`//astra`.

This pattern keeps Astra code separate from Chromium code while allowing Astra
to use all of Chromium's framework directly. The Chromium checkout is the
build and runtime environment; the overlay is the product layer.

## Directory Layout

```
chromium/
  README.md             This file.
  astra/
    BUILD.gn            GN build entry point for Astra targets.
    app/                Startup hooks and Chromium patch helpers.
      astra_browser_main_extra_parts.*
      astra_content_browser_client.*
      astra_main_delegate.*
    browser/            Astra product metadata and services.
      astra_workspace_service.*   ProfileKeyedService for Spaces.
      astra_tab_features.*        WebContentsUserData for tab metadata.
      astra_command_delegate.*    Astra-only command dispatch.
    ui/views/           Views-based UI additions.
      astra_browser_view.*        BrowserView augmenting controller.
      sidebar/                    Vertical tab sidebar.
    patches/            Patch queue notes and templates.
      README.md
      PATCH_TEMPLATE.md
```

## Bootstrap

The bootstrap script fetches Chromium and sets up the overlay:

```bash
./scripts/chromium-bootstrap.sh
```

What it does:

1. Checks for `depot_tools` and installs it if missing.
2. Fetches Chromium source into `chromium/src/` (this takes a while).
3. Syncs `chromium/astra/` overlay into `chromium/src/astra/`.
4. Applies the patch queue from `chromium/astra/patches/`.
5. Generates initial GN args.

The Chromium checkout is **not** committed to this repository. It is a local
build dependency.

## Sync

After making changes to the overlay in this repository, sync them into the
Chromium checkout:

```bash
./scripts/sync-astra-overlay.sh   # TODO(astra): create this script
```

Sync is a one-way copy from `chromium/astra/` to `chromium/src/astra/`. It
overwrites files in the checkout but does not delete files. If you rename or
remove a file, clean the checkout manually or re-bootstrap.

## Build

Build the Astra-branded Chromium target:

```bash
# Debug build
./scripts/build-chromium.sh Debug

# Release build
./scripts/build-chromium.sh Release

# Direct GN/Ninja
cd chromium/src
gn gen out/astra_Debug --args='is_debug=true is_astra_branded=true'
autoninja -C out/astra_Debug chrome
```

The build target is `chrome`. The Astra overlay is linked into the Chrome
binary via the build flag and patches.

## Patch Philosophy

See `chromium/astra/patches/README.md` for the full patch policy. The short
version:

- **Patches are tiny.** Each patch changes 1-10 lines of Chromium code.
- **Patches delegate.** All product logic lives in `//astra`, never in patched
  Chromium files.
- **Patches are gated.** Every patch is behind `BUILDFLAG(IS_ASTRA_BRANDED)`
  so Chromium builds normally without the flag.
- **Patches are documented.** Each patch has a detail file explaining the
  what, why, and alternatives.

## Relationship to Chromium Source

Astra depends on Chromium as the browser framework. Astra is not a fork of
Chromium. The relationship is:

- **Chromium provides:** browser process, renderer processes, `WebContents`,
  `TabStripModel`, `Browser`, `BrowserView`, profiles, extensions, passwords,
  history, downloads, permissions, DevTools, WebUI, updater, policy, and all
  the web platform.
- **Astra provides:** workspace/Spaces metadata, sidebar projection UI,
  split/glance presentation, Astra-specific commands, and product visual
  identity.

Astra code is written against Chromium's public C++ APIs. When a needed hook
is not exposed, a small patch adds it. The goal is to minimize patches and
maximize reuse.

## Current Status

Architecture skeleton only. The overlay contains header and stub source files
that define the class structure and dependencies. Full implementation follows
in subsequent phases.

Chromium patch points listed in `chromium/astra/patches/README.md` need to be
applied inside the Chromium checkout by follow-up work.
