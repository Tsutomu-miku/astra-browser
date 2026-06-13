# ADR-0015: Split View WebContents Layout

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

ADR-0013 established that Split View uses real `WebContents` instances owned by `TabStripModel`. This ADR focuses on the Views-level layout decision: how the two `WebContents` views are arranged in the browser content area.

The content area of `BrowserView` normally hosts a single active `WebContents` view. Split view requires showing two `WebContents` side-by-side (or top-bottom) with a draggable splitter between them.

Key questions:
- What Views container manages the two panes?
- How are `WebContents` views obtained and embedded?
- How does the split layout integrate with `BrowserView`'s existing content area?
- Who manages the splitter and resize logic?

## Decision

Split view uses `views::SplitView` (or an `AstraSplitView` subclass) as the container, with each pane hosting one `WebContents` view. The split view is inserted into the content area of `BrowserView`, replacing the normal single-tab contents container.

**Layout structure:**

```
BrowserView
  └── contents_container_
        └── AstraSplitView (inserted by AstraBrowserView)
              ├── [primary WebContents view]
              ├── splitter handle
              └── [secondary WebContents view]
```

**AstraSplitViewController** manages the layout lifecycle:
- On `ShowSplitView()`, the controller obtains both `WebContents` views, creates an `AstraSplitView`, inserts it into the content area, and reparents both WebContents views into the split panes.
- On `HideSplitView()`, the controller removes the split view, restores the primary `WebContents` view to its normal position, and destroys the split container.
- The controller also manages split ratio, orientation (horizontal/vertical), and view swapping.

**WebContents view embedding:**
- Each `WebContents` has an associated native view accessible via `content::WebContentsView`.
- In desktop Chrome, this view is embedded in the Views hierarchy through `views::WebView` or a similar container.
- The split view reparents these existing views rather than creating new `WebContents`.
- Only the *view* is reparented; `WebContents` ownership remains with `TabStripModel`.

**State persistence:**
- Split state (active, partner_id, ratio, orientation) is stored on `AstraTabFeatures` for both participating tabs.
- Session restore recovers tab content via Chromium; split metadata is reapplied from `AstraTabFeatures` persistence.

## Consequences

Positive:

- Reuses Chromium's `views::SplitView` (or a thin subclass) for splitter logic, drag handling, and layout.
- Both panes have full `WebContents` functionality: navigation, DevTools, extensions, zoom, find-in-page.
- `WebContents` ownership stays with `TabStripModel` -- no new ownership model.
- The split view is a Views-level concern; browser logic (commands, tab strip, etc.) is unaffected.
- Orientation switching (horizontal/vertical) is a layout property, not a content change.

Negative:

- Reparenting `WebContents` views between containers can be tricky on some platforms (especially macOS where `WebContentsView` has native view hierarchies).
- BrowserView's content area layout assumes a single active tab; inserting a split view requires coordination with `BrowserView` internals (patch point: `chrome/browser/ui/views/frame/browser_view.cc`).
- The standard tab strip shows both split tabs as separate entries; the sidebar must visually indicate split state.
- Layout complexity increases: find bar, infobars, and other content-area overlays need to target the correct pane.

Neutral:

- The splitter is a Views control, not a native control, for consistent cross-platform behavior.

## Alternatives Considered

### Custom split container instead of views::SplitView
Build a custom `views::View` subclass with two children and a draggable divider.

- Rejected: `views::SplitView` already provides this functionality and is used elsewhere in Chromium (e.g., for split settings pages). Reusing it is simpler and follows Chromium patterns.

### Side-by-side Browser objects
Use two `Browser` windows side-by-side in a single top-level window.

- Rejected: Each `Browser` has its own tab strip, toolbar, and state. Split view shares one toolbar and one tab strip -- it is a layout variation, not two separate browsers.

### WebContents-level split (nested WebContents)
Create a special "split" `WebContents` that internally manages two renderers.

- Rejected: `WebContents` is designed for one page. Modifying it for split view would be a large change to content layer, far from the product UX layer where this feature belongs.

## References

- **Chromium subsystems reused:** `views::SplitView`, `content::WebContentsView`, `BrowserView` contents container, `TabStripModel`
- **Astra components:** `AstraSplitViewController`, `AstraSplitView`, `AstraTabFeatures`, `AstraBrowserView`
- **Patch points:** BrowserView content area (`chrome/browser/ui/views/frame/browser_view.cc` -- install AstraBrowserView hook, see patch 0002)
