# ADR-0016: Glance / Peek Overlay Model

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

ADR-0013 established that Glance (also called Peek) displays a `WebContents` as a temporary preview. This ADR focuses on the overlay presentation model: how glance appears as a bubble, how it relates to the tab strip, and the two modes of operation.

Glance serves two use cases:
1. **Tab glance** -- Hovering or clicking on a sidebar tab shows a live preview of that tab without switching to it. The tab already exists in `TabStripModel`.
2. **URL glance** -- Hovering or clicking a link shows a preview of the URL. The `WebContents` is created temporarily and may be promoted to a real tab.

Key questions:
- Is glance a panel, a bubble, or a side pane?
- Where does the temporary `WebContents` for URL glance come from?
- How does promotion from glance to full tab work?
- What is the ownership model for the glance view and contents?

## Decision

Glance is a **bubble overlay** (`views::BubbleDialogDelegateView` subclass) that hosts a `WebContents` view. It is an ephemeral presentation layer, not a content area layout change.

**Two glance modes:**

1. **Tab glance (existing tab preview)**
   - Uses an existing `WebContents` from `TabStripModel`.
   - The `WebContents` view is temporarily shown in the glance bubble.
   - The original tab stays in `TabStripModel` throughout.
   - When dismissed, the view returns to its normal position.
   - `AstraTabFeatures::is_glance_tab` is set to true on the tab.

2. **URL glance (link preview)**
   - Creates a temporary `WebContents` owned by the glance controller.
   - Loads the target URL into it.
   - The temporary `WebContents` is marked with `is_glance_tab=true` and `glance_source_tab_id`.
   - On close, the `WebContents` is destroyed unless the user promotes it.
   - On promote, the temporary `WebContents` is transferred to `TabStripModel` as a new tab.

**Controller ownership:**
- `AstraGlanceViewController` (one per browser window) manages glance lifecycle.
- The controller owns the temporary `WebContents` for URL glance mode (via `unique_ptr`).
- For tab glance mode, the controller holds a `raw_ptr` to the existing `WebContents`.
- The glance bubble widget is owned by the Views widget system; the controller is notified of destruction.

**Promotion to tab:**
- URL glance can be promoted to a full tab via "Open in tab" action.
- Promotion transfers `WebContents` ownership from the controller to `TabStripModel`.
- After promotion, `is_glance_tab` is cleared and the tab behaves normally.
- Tab glance promotion is a no-op (already a tab).

**Bubble anchoring:**
- Glance is anchored to the triggering element (sidebar item, link, etc.).
- The bubble appears adjacent to the anchor, consistent with Chrome's bubble UX patterns.
- Bubble size is configurable but has minimum and maximum bounds.

## Consequences

Positive:

- Ephemeral: glance does not disturb the tab strip or content area layout.
- Two modes share the same view and controller, reducing code duplication.
- Promotion is a clean ownership transfer, not a page reload.
- Uses standard Chrome bubble patterns (`BubbleDialogDelegateView`), consistent with other browser UI.
- Tab glance gives a live preview (not a screenshot), so interactive content works.

Negative:

- Showing the same `WebContents` in two places simultaneously (tab strip + glance bubble) may not be supported by `WebContentsView` on all platforms. May require view reparenting or a mirror view approach.
- URL glance creates a real renderer process, which is heavier than a screenshot preview. This is the right tradeoff for interactivity but uses more memory.
- The glance bubble is a separate widget, which can feel disconnected from the content area on some platforms.
- Focus management between the main browser and the glance bubble requires care.

Neutral:

- Glance is a per-window concept, not per-tab. Only one glance is active per browser window at a time.

## Alternatives Considered

### Side panel (like Chrome's side panel)
Show glance as a side panel that slides in from the edge.

- Rejected: A side panel is more permanent and takes up permanent space. Glance is a quick preview that should be dismissable with one click or a click elsewhere. A bubble better matches the "peek" semantics.

### Screenshot / thumbnail preview (like Chrome tab hover cards)
Show a static screenshot of the tab instead of a live `WebContents`.

- Rejected: A static preview cannot show interactive content, video, or up-to-date state. The product requirement is for "live peek" similar to Edge's peek feature.

### Iframe in a bubble
Embed the page as an `<iframe>` inside a WebUI bubble.

- Rejected: Iframes have different security origins, cannot navigate independently, and do not support full DevTools. Also rejected for split view for the same reasons.

## References

- **Chromium subsystems reused:** `views::BubbleDialogDelegateView`, `content::WebContents`, `TabStripModel`, `Widget`
- **Astra components:** `AstraGlanceViewController`, `AstraGlanceView`, `AstraTabFeatures` (glance metadata)
- **Patch points:** None required for basic implementation (Astra-owned UI). Future `WebContentsView` mirroring may need a content-layer patch.
