# ADR-0013: Split View Architecture

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra offers two side-by-side viewing features:

1. **Split View** -- two tabs displayed side-by-side in the main content area,
   each independently navigable. Both tabs are "real" tabs that exist in the
   tab strip.
2. **Glance / Peek** -- a temporary side panel that previews a link or page
   without switching the main tab. A glance can be promoted to a full tab.

Both features require displaying multiple web pages simultaneously. The
question is how to implement this without reimplementing WebContents ownership
or introducing nested browsing contexts.

Chromium's `content::WebContents` is the fundamental unit of web content. Each
`WebContents` has its own renderer process, navigation controller, session
history, and DevTools target. Displaying two pages side-by-side means arranging
two `WebContents` views in the UI.

## Decision

Split View and Glance are **Views layouts of Chromium WebContents**. Both
features use real, full `WebContents` instances owned by `TabStripModel`. No
iframes, no nested webviews, no secondary runtimes.

**Split View:**

- A Views container splits the content area horizontally (or vertically).
- Each pane hosts the `WebContents`' native view (via `views::WebView` or
  direct `content::WebContentsView` embedding).
- Both `WebContents` are regular tabs in `TabStripModel`. They are identified
  as "in split view" by `AstraTabFeatures` metadata.
- Activating a tab in split view selects the paired tab as the side pane.
- Split state is stored in `AstraTabFeatures` (`is_in_split_view()`) and
  resolved by `AstraBrowserView` at layout time.
- Both tabs have full renderer processes, full navigation stacks, and full
  DevTools support.

**Glance / Peek:**

- Glance is a temporary side panel that hosts a `WebContents`.
- The glance `WebContents` may either be:
  - An existing tab's `WebContents` (for "peek at this tab"), or
  - A newly created `WebContents` that is added to `TabStripModel` but marked
    as "glance only" until promoted.
- When a glance is promoted, it becomes a regular tab in `TabStripModel` (if
  not already).
- Glance metadata lives in `AstraTabFeatures`.
- The Views layout is managed by `AstraBrowserView`.

**Common principles:**

- All `WebContents` are owned by `TabStripModel`. Astra never owns
  `WebContents` directly.
- Both split view and glance are presentation choices, not content ownership
  models.
- Closing a split pane closes the underlying tab (normal Chromium behavior).
  Detaching removes the split metadata and restores single-tab layout.
- Session restore recovers tab content via Chromium session restore; split
  metadata is reapplied from `AstraTabFeatures` persistence.

## Consequences

Positive:

- Both tabs are fully alive: real processes, real navigation, real DevTools.
- No nested browsing context security issues (iframes have different security
  models than top-level browsing contexts).
- Extensions see both tabs as normal tabs and interact with them correctly.
- All Chromium web features (find in page, zoom, mute, media, print, etc.)
  work on both panes without adaptation.
- Tab identity is preserved: split is a view state, not a new kind of tab.
- Memory behavior is predictable: two tabs = two WebContents = normal Chromium
  memory profile.

Negative:

- Two `WebContents` = two renderer processes, which uses more memory than an
  iframe approach. This is the correct tradeoff for a product feature; split
  view is for real work, not previews.
- Layout complexity in `AstraBrowserView`. The browser view must coordinate
  with the existing `BrowserView` content area layout.
- The standard tab strip shows both split tabs as separate entries; the Astra
  sidebar must visually group or indicate split state.

## Alternatives Considered

### Iframe approach
Embed one page as an `<iframe>` inside another page, or inside a side panel.

- Rejected: Iframes run in the same renderer process as the parent page (in
  the default site-per-process model), have different security origins and
  permissions, cannot be navigated independently in the same way, and do not
  support full DevTools. They are a web content feature, not a browser tab
  feature.

### Picture-in-Picture (PiP)
Use the Picture-in-Picture API for the secondary pane.

- Rejected: PiP is for video, not general web content. It has a restricted
  document model and cannot navigate arbitrary URLs. Wrong tool for the job.

### Chrome tab groups side-by-side
Wait for or contribute to a Chromium tab groups side-by-side feature.

- Rejected: Chromium tab groups are a tab strip organization feature, not a
  split content area feature. There is no built-in split view in desktop
  Chrome. Implementing it at the Astra layer as a Views layout is the right
  place for product-specific UX.

### WebView in side panel
Use a secondary `WebContents` created and owned by the sidebar/panel, not by
`TabStripModel`.

- Rejected: Creates a `WebContents` not owned by `TabStripModel`, breaking
  session restore, extension tab enumeration, and tab management semantics.
  All `WebContents` should live in `TabStripModel` for consistency.
