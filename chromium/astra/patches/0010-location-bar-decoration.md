# 0010 — Add Astra workspace decoration to location bar

**Patch ID:** 0010
**File:** `chrome/browser/ui/views/location_bar/location_bar_view.cc`
(and `chrome/browser/ui/views/location_bar/location_bar_view.h`)
**Size estimate:** ~25 lines total across 2 files
**Status:** planned
**Astra component:** `astra/ui/views/omnibox/astra_location_bar_decoration.h`

## Context

The location bar (omnibox) in Chromium has a row of "decorations" — small
icon buttons on the leading and trailing edges (security icon, star/bookmark,
translate, site settings, etc.). Each decoration is a `views::View` subclass
that the `LocationBarView` manages and lays out.

Astra adds a workspace indicator decoration: a small colored dot on the
leading edge of the omnibox showing the current workspace's accent color.
Clicking it opens the command palette or workspace switcher.

This cannot be done from `//astra` alone because the decoration must be
added to the location bar's view hierarchy and participate in its layout.
A small patch adds the Astra decoration view to the existing decoration row.

**Chromium class:** `LocationBarView`
  (`chrome/browser/ui/views/location_bar/location_bar_view.h`)
  - The main omnibox / location bar view.
  - Manages leading and trailing decoration icon rows.
  - Handles layout and update of all decorations.

## Change

### In the header (`location_bar_view.h`)

Add a forward declaration and a member variable for the Astra decoration.

#### Before

```cpp
class LocationBarView : public views::View {
  // ...
 private:
  raw_ptr<StarView> star_view_ = nullptr;
  raw_ptr<TranslateIconView> translate_icon_view_ = nullptr;
  // ... other decoration members ...
};
```

#### After

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
namespace astra {
class AstraLocationBarDecorationView;
}
#endif

class LocationBarView : public views::View {
  // ...
 private:
  raw_ptr<StarView> star_view_ = nullptr;
  raw_ptr<TranslateIconView> translate_icon_view_ = nullptr;
  // ... other decoration members ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra workspace indicator decoration.
  raw_ptr<astra::AstraLocationBarDecorationView> astra_decoration_ = nullptr;
#endif
};
```

### In the source (`location_bar_view.cc`)

Include the Astra header and create the decoration in `Init()`.

#### Before

```cpp
// Includes at the top of the file.
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/star_view.h"
// ...
```

```cpp
// In LocationBarView::Init() or wherever decorations are created.
void LocationBarView::Init() {
  // ... create star_view_, translate_icon_view_, etc. ...
  // All decorations created and added to the view hierarchy.
}
```

#### After

Include at the top, guarded by build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/ui/views/omnibox/astra_location_bar_decoration.h"
#include "astra/browser/astra_workspace_service.h"
#endif
```

In `LocationBarView::Init()` or wherever decorations are created:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra workspace indicator decoration.
  astra_decoration_ = AddChildView(
      std::make_unique<astra::AstraLocationBarDecorationView>(
          astra::AstraLocationBarDecorationView::Edge::kLeading));
  astra_decoration_->SetProperty(views::kElementIdentifierKey,
                                 kLocationBarAstraDecorationElementId);
#endif
```

In the layout / update method (e.g., `Update()`, `RefreshContentViews()`),
update the decoration with the active workspace:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  if (astra_decoration_) {
    astra::AstraWorkspaceService* workspace_service =
        astra::AstraWorkspaceServiceFactory::GetForProfile(
            browser_->profile());
    if (workspace_service) {
      const auto& active = workspace_service->active_workspace();
      astra_decoration_->UpdateWorkspace(active.name, active.accent_color);
    }
    astra_decoration_->SetDecorationVisible(true);
  }
#endif
```

## Rationale

**Why the location bar?**
- The omnibox is the most prominent UI surface in a browser.
- Placing the workspace indicator in the omnibox gives it constant
  visibility and follows the Arc browser pattern where the current
  space is visible in the address bar area.
- Users expect context indicators near where they type and search.

**Why a decoration view?**
- Chromium's location bar has a well-defined decoration system.
- Adding a new decoration is the smallest, most idiomatic patch.
- Decorations participate in layout, hit testing, and accessibility.
- The pattern is used by star/bookmark, translate, site settings, etc.

**Leading edge vs trailing edge?**
- Leading (left side) is more visible and follows the pattern of the
  security icon as a "context indicator."
- Trailing would be less noticeable.
- The Astra decoration view supports both edges; the patch defaults to
  leading but can be changed.

**What `//astra` code does this delegate to?**
- `astra::AstraLocationBarDecorationView` — UI view with color dot + tooltip.
- `astra::AstraWorkspaceService` — source of truth for workspace name + color.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)**
- **Build flag defined in:** `astra/build/astra_buildflags.h`
- **Default:** off (no Astra decoration in plain Chromium builds)
- All Astra includes, members, and code are behind this flag.

## Alternatives Considered

1. **Add to toolbar instead of location bar** — Less visible, doesn't
   follow the Arc/Space pattern. The omnibox is the natural home for
   workspace context since that's where users type and search.

2. **Use an OmniboxPedal instead of a decoration** — Pedals appear in
   the suggestion dropdown, not in the omnibox itself. We need an
   always-visible indicator, not a suggestion-time action.

3. **Draw directly on the omnibox background** — Would require painting
   customizations and wouldn't participate in hit testing / accessibility.
   A proper view is more maintainable.

4. **Add to the tab strip instead** — Could show workspace indicator in
   the tab strip. Considered, but the omnibox is more central and
   follows the established Arc pattern.

## Risks & Rebase Concerns

- **Medium risk.** The location bar code is relatively stable but the
  decoration system has been refactored a few times (icon rows, chips,
  etc.). The patch hooks into the existing decoration pattern which
  has been conceptually stable.

- **Refactoring watch:** `LocationBarView::Init()`,
  `LocationBarView::RefreshContentViews()`, and the leading/trailing
  decoration layout system are areas that sometimes change between
  Chromium milestones.

- **Graceful degradation:** If the patch fails to apply, the Astra
  workspace indicator simply won't appear — no build break, no
  functional regression. The workspace service and sidebar continue
  to work normally.

- **Theme consistency:** If Chromium changes the location bar visual
  style, our decoration may look out of place. Mitigation: Use Chromium
  color IDs and layout constants; avoid hardcoded Astra-specific
  styling values in the view.

## Related

- ADR: (consider adding ADR for workspace UX / omnibox integration)
- Related patches: 0011 (omnibox Astra provider), 0002 (browser view install)
- Astra source: `astra/ui/views/omnibox/astra_location_bar_decoration.h`
- Astra service: `astra/browser/astra_workspace_service.h`
