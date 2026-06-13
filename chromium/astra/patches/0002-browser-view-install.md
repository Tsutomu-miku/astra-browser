# 0002 — Install AstraBrowserView in BrowserView

**Patch ID:** 0002
**File:** `chrome/browser/ui/views/frame/browser_view.cc`
(and `chrome/browser/ui/views/frame/browser_view.h`)
**Size estimate:** ~15 lines total across 2 files
**Status:** planned
**Astra component:** `astra/ui/views/astra_browser_view.h`

## Context

`BrowserView` is Chromium's main window view class. It owns the toolbar,
tab strip, omnibox, and all other browser UI elements. It implements
`views::WidgetDelegate`, `views::AcceleratorTarget`, and `BrowserWindow`.

Astra augments `BrowserView` with Astra-specific surfaces (sidebar,
workspace switcher, etc.) without replacing the entire desktop UI stack.
The install happens after `BrowserView` construction so the existing
toolbar, tab strip, omnibox, and all Chromium-owned UI remain intact.

This cannot be done from `//astra` alone because Astra UI surfaces must
be added to `BrowserView`'s view hierarchy and observe `BrowserView`'s
lifecycle. A small patch instantiates `AstraBrowserView` and calls
`Install()` on it.

## Change

### In the header (`browser_view.h`):

#### Before

```cpp
class BrowserView : public views::WidgetDelegateView,
                    public BrowserWindow,
                    ... {
  // ... member variables ...
  raw_ptr<TabStripRegionView> tab_strip_region_view_ = nullptr;
  raw_ptr<SidePanelView> side_panel_view_ = nullptr;
  // ... more members ...
};
```

#### After

Add a forward declaration and a member variable:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
namespace astra {
class AstraBrowserView;
}
#endif

class BrowserView : public views::WidgetDelegateView,
                    public BrowserWindow,
                    ... {
  // ... member variables ...
  raw_ptr<TabStripRegionView> tab_strip_region_view_ = nullptr;
  raw_ptr<SidePanelView> side_panel_view_ = nullptr;

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra product UI layer — adds sidebar, workspace switcher, etc.
  // Owned by BrowserView; installed after construction.
  std::unique_ptr<astra::AstraBrowserView> astra_browser_view_;
#endif

  // ... more members ...
};
```

### In the source (`browser_view.cc`):

#### Before

```cpp
// Include at the top of the file.
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
// ... other includes ...
```

```cpp
// Inside BrowserView::Init() or BrowserView::Show(),
// after the view hierarchy is set up but before the widget is shown.
void BrowserView::Init() {
  // ... setup toolbar, tab strip, omnibox, etc. ...
  // View hierarchy is complete.
}
```

#### After

Include at the top of the file, guarded by the build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/ui/views/astra_browser_view.h"
#endif
```

Inside `BrowserView::Init()` or `BrowserView::Show()`, after the main view
hierarchy has been constructed:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Install Astra product UI surfaces (sidebar, workspace switcher, etc.).
  // This must happen after the core Chromium view hierarchy is constructed
  // so that AstraBrowserView can observe and augment existing views.
  if (!astra_browser_view_) {
    astra_browser_view_ = std::make_unique<astra::AstraBrowserView>(this);
    astra_browser_view_->Install();
  }
#endif
```

## Rationale

**Why patch BrowserView?**
- It is the root of the browser window's view hierarchy.
- All browser UI surfaces (toolbar, tab strip, omnibox, side panel)
  are children of BrowserView.
- Astra's UI surfaces need to be part of this hierarchy to receive
  events, participate in layout, and access the Browser object.

**Why a separate AstraBrowserView class?**
- Keeps all Astra UI logic in `//astra`, not in Chromium code.
- The patch is tiny — just instantiation and Install() call.
- Easy to disable: turn off the build flag and all Astra UI is gone.
- Clean separation: Chromium owns the window, Astra adds surfaces.

**What is AstraBrowserView?**
- It is a controller, not a replacement for BrowserView.
- It adds child views (sidebar, workspace switcher) to the existing
  BrowserView hierarchy.
- It observes BrowserView state (tab strip model, profile, etc.).
- BrowserView remains the root view and widget delegate.

**What `//astra` code does this delegate to?**
- `astra/ui/views/astra_browser_view.h` — main Astra UI controller.
- `astra/ui/views/sidebar/astra_sidebar_view.h` — sidebar UI surface.
- `astra/ui/views/profiles/astra_workspace_avatar_button.h` — workspace badge.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `astra/build/astra_buildflags.h`
- **Default:** off (plain Chromium builds have no Astra UI)
- All Astra includes, members, and code are behind this flag.

## Alternatives Considered

1. **Build a separate browser window** — Create our own window class
   instead of extending BrowserView. Rejected: massive duplication of
   Chromium's window logic, and we want to reuse Chromium's UI, not
   replace it.

2. **Use a separate widget for the sidebar** — Show the sidebar as a
   separate top-level window. Rejected: Poor UX (two windows instead
   of one), complex focus management, doesn't feel integrated.

3. **Patch every view individually** — Add Astra views directly into
   each Chromium view (tab strip, toolbar, etc.). Rejected: Too many
   patch points, high maintenance cost. Better to have one entry point
   (AstraBrowserView) that installs all Astra UI.

4. **Use the SidePanel API** — Chromium has a side panel feature.
   Could we use that for the sidebar? Considered, but the Astra sidebar
   is more than a side panel — it replaces the tab strip and has its
   own navigation model. The side panel API is too limited.

## Risks & Rebase Concerns

- **Medium risk.** BrowserView is a large, complex class that changes
  frequently. The exact location of the Install() call may need to
  shift with Chromium refactors.
- **Mitigation:** The patch is tiny (~15 lines) and the concept is
  simple — call Install() after the view hierarchy is built. Even if
  the exact line changes, the patch is easy to rebase.
- **Member variable:** Adding a member to BrowserView is slightly
  more invasive than just adding code to a method. But it's still
  small — just one unique_ptr member.
- **Graceful degradation:** If the patch fails to apply, Astra UI
  simply won't appear. All Chromium functionality continues to work.
  No crash, no data loss.

## Related

- ADR: `docs/adr/0009-direct-chromium-architecture.md`
- Related patches: 0001 (browser main extra parts — startup),
  0009 (profile menu workspaces), 0010 (location bar decoration)
- Astra source: `astra/ui/views/astra_browser_view.h`
- Astra source: `astra/ui/views/sidebar/astra_sidebar_view.h`
