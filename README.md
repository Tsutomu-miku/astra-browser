# Astra Browser

Astra Browser is being rebuilt as a direct Chromium browser. The active target is
not Electron and not CEF: Astra lives as a product layer on top of Chromium's
`chrome`, `content`, `components`, and `ui/views` framework.

## Current Direction

- Build system: Chromium GN/Ninja.
- Product overlay: `chromium/astra/`, synced into a Chromium checkout as
  `chromium/src/astra`.
- UI framework: Chromium desktop `ui/views` and `BrowserView`.
- Browser primitives: reuse Chromium `Profile`, `Browser`, `TabStripModel`,
  `WebContents`, `NavigationController`, History, Downloads, Permissions,
  Password Manager, Autofill, Safe Browsing, Extensions, DevTools, WebUI, Policy,
  and Update.
- Astra-owned product semantics: Spaces, vertical sidebar organization,
  Favorites as a tab folder, Split/Glance, Astra command palette, and visual
  identity.

Legacy Electron prototype code remains under `src/` only as migration reference.
Do not add new product architecture there unless the task explicitly says it is
legacy prototype maintenance.

## Primary Docs

- [Direct Chromium refactor plan](docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Code structure](docs/CODE_STRUCTURE.md)
- [Engineering standards](docs/ENGINEERING_STANDARDS.md)
- [Roadmap](docs/ROADMAP.md)
- [ADR-0009](docs/adr/0009-direct-chromium-architecture.md)

Historical Electron/CEF docs are archived in
`docs/legacy/electron-prototype/`.

## Chromium Workflow

```bash
./scripts/chromium-bootstrap.sh
./scripts/build-chromium.sh Debug
```

The bootstrap step downloads Chromium and depot_tools, which is intentionally
large. The build script syncs `chromium/astra/` into `chromium/src/astra` and
builds Chromium's `chrome` target.

## Repository Layout

```text
chromium/astra/      Astra overlay for direct Chromium source builds
docs/                Active direct Chromium architecture and standards
docs/adr/            Active architecture decisions
docs/legacy/         Historical Electron prototype materials
scripts/             Chromium bootstrap/build plus repository checks
src/                 Legacy Electron prototype, migration reference only
tests/               Legacy prototype tests until migrated to Chromium tests
```

## Guardrails

Run the architecture guard before handing work to another agent:

```bash
pnpm check:architecture
```

This prevents CEF/CMake/native-shell scaffolds from reappearing in the active
tree and checks that `chromium/astra/` stays aligned with Chromium framework
boundaries.
