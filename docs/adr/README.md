# Architecture Decision Records

Active ADRs describe the direct Chromium architecture only. Historical
Electron/CEF ADRs are archived under `docs/legacy/electron-prototype/adr/`.

## Index

| 编号 | 标题 | 状态 |
| --- | --- | --- |
| 0009 | [Direct Chromium Architecture](0009-direct-chromium-architecture.md) | Accepted |
| 0010 | [Workspace as Metadata Projection](0010-workspace-as-metadata-projection.md) | Accepted |
| 0011 | [Sidebar Projection Model](0011-sidebar-projection-model.md) | Accepted |
| 0012 | [Command Delegation Strategy](0012-command-delegation-strategy.md) | Accepted |
| 0013 | [Split View Architecture](0013-split-view-architecture.md) | Accepted |
| 0014 | [Favorite Folders as Tab Metadata](0014-favorite-folders-as-tab-metadata.md) | Accepted |
| 0015 | [Split View WebContents Layout](0015-split-view-webcontents-layout.md) | Accepted |
| 0016 | [Glance / Peek Overlay Model](0016-glance-peek-overlay-model.md) | Accepted |
| 0017 | [Command Palette Design](0017-command-palette-design.md) | Accepted |
| 0018 | [Workspace Import / Export Format](0018-workspace-import-export-format.md) | Accepted |
| 0019 | [Focus Mode (Distraction-Free)](0019-focus-mode-distraction-free.md) | Accepted |
| 0020 | [Tab Suspend / Memory Saver](0020-tab-suspend-memory-saver.md) | Accepted |
| 0021 | [Direct Chromium Patch Strategy](0021-direct-chromium-patch-strategy.md) | Accepted |
| 0022 | [Theme / Color System](0022-theme-color-system.md) | Accepted |
| 0023 | [Omnibox Astra Actions](0023-omnibox-astra-actions.md) | Accepted |
| 0024 | [Notes Feature Architecture](0024-notes-feature-architecture.md) | Accepted |
| 0025 | [Tab Stack / Tree Organization](0025-tab-stack-tree-organization.md) | Accepted |
| 0026 | [Reading List Integration](0026-reading-list-integration.md) | Accepted |
| 0027 | [Screenshot Capture Architecture](0027-screenshot-capture-architecture.md) | Accepted |
| 0028 | [New Tab Page Design](0028-new-tab-page-design.md) | Accepted |
| 0029 | [Common Layer](0029-common-layer.md) | Accepted |
| 0030 | [Color System — AstraColorMixer Approach](0030-color-system-astracolormixer.md) | Accepted |
| 0031 | [DevTools Integration](0031-devtools-integration.md) | Accepted |
| 0032 | [Projection Pattern](0032-projection-pattern.md) | Accepted |
| 0033 | [Agent-Based Development Workflow](0033-agent-based-development-workflow.md) | Accepted |

## By Category

### Architecture (Foundation)

- **[0009] Direct Chromium Architecture** -- Why we build on Chromium directly, not Electron or CEF.
- **[0021] Direct Chromium Patch Strategy** -- How we integrate with Chromium: overlay directory, tiny patches, delegation pattern.
- **[0029] Common Layer** -- Shared types and constants at the bottom of the dependency graph.
- **[0032] Projection Pattern** -- UI layers project state; no truth in UI. Core architectural principle.
- **[0033] Agent-Based Development Workflow** -- Directory-ownership agent model for parallel development.

### Core Product Model

- **[0010] Workspace as Metadata Projection** -- Workspaces are metadata on tabs, not separate tab models.
- **[0014] Favorite Folders as Tab Metadata** -- Favorites are per-tab flags + folder hierarchy service.
- **[0018] Workspace Import / Export Format** -- JSON schema for workspace serialization.
- **[0024] Notes Feature Architecture** -- URL-linked notes via ProfileKeyedService + sidebar projection.
- **[0025] Tab Stack / Tree Organization** -- Hierarchical tab stacks as WebContentsUserData metadata.

### UI / Presentation

- **[0011] Sidebar Projection Model** -- The sidebar projects state from `TabStripModel` + Astra metadata.
- **[0013] Split View Architecture** -- Split view uses real `WebContents` owned by `TabStripModel`.
- **[0015] Split View WebContents Layout** -- Views-level layout: `views::SplitView` with two WebContents views.
- **[0016] Glance / Peek Overlay Model** -- Glance is a bubble overlay with two modes (tab peek, URL peek).
- **[0022] Theme / Color System** -- Extends Chromium's `ColorProvider` with Astra color IDs and accent colors.
- **[0030] Color System — AstraColorMixer Approach** -- Implementation details: color IDs, mixer function, accent palette derivation.
- **[0031] DevTools Integration** -- Native Views panels and toolbar injected via Chromium patch points.
- **[0028] New Tab Page Design** -- Views-based NTP with workspace cards and shortcuts.

### Commands & Input

- **[0012] Command Delegation Strategy** -- Standard Chrome commands + Astra commands (ID range 60000+).
- **[0017] Command Palette Design** -- Unified fuzzy-search palette over Chrome + Astra commands.
- **[0023] Omnibox Astra Actions** -- Provider pattern for Astra omnibox suggestions (commands, workspace switch, tab search).

### Productivity Features

- **[0019] Focus Mode (Distraction-Free)** -- Hides distractions + optional site blocking, built on content settings.
- **[0020] Tab Suspend / Memory Saver** -- Astra policy layer on top of Chromium's tab discard engine.
- **[0026] Reading List Integration** -- Reuses Chromium's ReadingListModel, projected in sidebar.
- **[0027] Screenshot Capture Architecture** -- Builds on Chromium capture APIs with Astra metadata service.

## Rules

- New ADRs must use `NNNN-short-kebab.md`.
- ADRs in this directory are active guidance.
- Historical or superseded decisions must move to `docs/legacy/`.
- If a change proposes Electron, CEF, CMake browser builds, or duplicated Chrome
  services, it needs an ADR before implementation.
