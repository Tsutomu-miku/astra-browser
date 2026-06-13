# ADR-0028: New Tab Page Design

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra includes a custom New Tab Page (NTP) that replaces or augments Chromium's default NTP. The Astra NTP shows workspace cards, shortcuts, and other Astra-specific content, providing quick access to the user's workspaces and favorite sites.

Key architectural questions:
- Is the NTP a Views surface or a WebUI page?
- Does it replace Chromium's NTP entirely or augment it?
- Where does NTP content live (service layer, chrome layer, UI layer)?
- How does it relate to workspaces, shortcuts, and other Astra features?

## Decision

The Astra New Tab Page is implemented as a **Views-based page** (`AstraNewTabView`) that appears as the content of a new tab, replacing Chromium's default NTP WebUI for branded builds. The NTP content is backed by `AstraNewTabPageService`, a profile-scoped service that provides NTP content data.

**Why Views over WebUI:**
- Performance: Views renders faster than WebUI for a page that opens frequently.
- Integration: Direct access to browser services without IPC overhead.
- Consistency: Matches the sidebar and other Astra UI surfaces, which are Views-based.
- Theming: Uses the same `ColorProvider` system as the rest of the browser chrome.

**Service layer (AstraNewTabPageService):**
- Profile-scoped service that provides NTP content data.
- Manages: shortcut list, workspace cards, recently closed tabs, most visited sites.
- Sources data from:
  - `AstraWorkspaceService` — workspace list and active workspace.
  - Chromium's `MostVisitedService` or `TopSites` — most visited sites.
  - `TabRestoreService` / recently closed — recently closed tabs.
  - `PrefService` — NTP preferences (layout, shortcuts).
- Observer pattern for UI to react to content changes.
- Does not store data itself — it aggregates from other services.

**UI layer (AstraNewTabView):**
- A `views::View` subclass that renders the NTP content.
- Shows:
  - Workspace cards (click to switch to workspace).
  - Shortcut tiles (most visited / user-defined).
  - Recently closed section.
  - Google search / omnibox (optional — may redirect to the real omnibox).
- Reads from `AstraNewTabPageService` for content.
- Dispatches commands for actions (open workspace, add shortcut, etc.).
- UI is never the source of truth — all content comes from services.

**Integration with new tab creation:**
- When a new tab is created, if the URL is `chrome://newtab` (or equivalent), the content area shows `AstraNewTabView` instead of the default WebUI NTP.
- This requires a patch to `BrowserView` or the new tab creation path to swap in the Astra NTP view for branded builds.
- Non-branded builds use Chromium's default NTP unchanged.

**Relationship to Chromium NTP:**
- Astra does not reimplement the underlying NTP infrastructure (most visited, suggestions, etc.).
- Astra reuses Chromium data sources (`TopSites`, `MostVisitedService`) and wraps them in its own service.
- If Astra NTP is disabled or unavailable, Chromium's default NTP is used as a fallback.

**Shortcuts:**
- Shortcut tiles show frequently visited sites or user-pinned sites.
- Data comes from Chromium's top sites / most visited system.
- Astra adds workspace shortcuts as a separate section.

## Consequences

Positive:
- Fast, native-feeling NTP with direct browser service access.
- Consistent with Astra's overall Views-based UI approach.
- Reuses Chromium data sources (top sites, recently closed) — no reimplementation.
- Clean service/UI separation: service aggregates data, UI renders it.
- Theming via `ColorProvider` integrates with Chrome themes.

Negative:
- Replacing the NTP requires a Chromium patch point to intercept new tab creation.
- A Views-based NTP cannot use web technologies (HTML/CSS/JS) for rich content, which may limit future features like interactive widgets.
- Maintenance: as Chromium's NTP evolves, Astra's NTP may diverge in features.
- No built-in "custom links" or "Google doodle" support — those are WebUI features in Chrome's NTP.

Neutral:
- The NTP is a separate Views surface, not part of the sidebar.
- Workspace cards on the NTP are a projection of workspace state — the truth source is `AstraWorkspaceService`.

## Alternatives Considered

### WebUI-based NTP (chrome://astra-newtab)
Build the NTP as a WebUI page with HTML/CSS/JS, similar to Chromium's default NTP.

- Rejected: Adds IPC overhead for every content update, requires a WebUI data source, and is slower to render than Views. Also inconsistent with Astra's overall direction of Views-based browser chrome. The NTP is core browser chrome that should feel fast and native.

### Chrome NTP with Astra customization overlay
Keep Chromium's default NTP and add an Astra overlay (e.g., workspace cards on top).

- Rejected: Layering Astra UI on top of Chrome's WebUI NTP would be architecturally messy — mixing WebUI and Views layers with complex z-ordering and event handling. It's cleaner to replace the NTP entirely for Astra-branded builds.

### NTP as a sidebar panel
Put the new tab content in the sidebar instead of a full-page view.

- Rejected: The NTP is a full-page experience that replaces the main content area. A sidebar panel would be too narrow for workspace cards and shortcuts. The NTP is the primary destination when opening a new tab.

### Reuse Chrome's NTP entirely (no custom NTP)
Use Chromium's default NTP as-is, with no Astra customization.

- Considered: The default NTP works well and has many features. However, workspace integration and Astra-specific content on the NTP are important product features. A custom NTP provides better integration with Astra's workspace model and overall product identity.

## References

- **Chromium subsystems reused:** `TopSites` / `MostVisitedService`, `TabRestoreService` (recently closed), `ColorProvider`, `Views`, `ProfileKeyedService`
- **Astra components:** `AstraNewTabPageService`, `AstraNewTabView`, `AstraNewTabBubble`, `AstraNtpShortcutView`, `AstraNtpWorkspaceCard`, `AstraWorkspaceService` (data source)
- **Patch points:** New tab creation / NTP URL handling (`chrome/browser/ui/browser_navigator.cc` or `BrowserView::ShowNTPOptions`), content area view swap
- **Related ADRs:** ADR-0010 (Workspace as Metadata Projection), ADR-0022 (Theme / Color System)
