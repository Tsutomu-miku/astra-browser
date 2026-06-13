# 0015 — DevTools dock state access

**Patch ID:** 0015
**File:** `chrome/browser/devtools/devtools_window.h`
(and `chrome/browser/devtools/devtools_window.cc`)
**Size estimate:** ~5-10 lines
**Status:** planned
**Astra component:** `astra/browser/astra_devtools_helper.h`

## Context

Chromium's `DevToolsWindow` class manages the DevTools window for each tab —
its creation, dock state (bottom, right, left, undocked), and lifecycle.
Astra wraps DevTools functionality through `AstraDevToolsHelper`, which provides
Astra-friendly APIs for toggling DevTools, setting dock state, and querying
DevTools state.

Some DevToolsWindow methods (like `ToggleDevToolsWindow`) are public and can
be called directly from `//astra` code without patches. However, other methods
(like querying the current dock state) may be private or protected, requiring
a small patch to expose them.

This patch exposes dock state querying and other needed DevToolsWindow APIs
so that AstraDevToolsHelper can access them without being a friend class.

## Change

### In the header (devtools_window.h):

#### Before

```cpp
class DevToolsWindow : public content::WebContentsObserver,
                       public views::WidgetDelegate,
                       ... {
  // ...
 private:
  DevToolsDockSide dock_side() const { return dock_side_; }
  // ...
};
```

#### After

Add public accessors for dock state and other needed methods:

```cpp
class DevToolsWindow : public content::WebContentsObserver,
                       public views::WidgetDelegate,
                       ... {
  // ...
 public:
  // ... existing public methods ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Returns the current dock side for this DevTools window.
  // Exposed for Astra product integration (AstraDevToolsHelper).
  // TODO(astra): Consider upstreaming this if generally useful.
  DevToolsDockSide dock_side() const { return dock_side_; }

  // Returns true if DevTools is currently open for the inspected WebContents.
  // Static helper for Astra to query DevTools state without needing
  // the DevToolsWindow instance.
  static bool IsDevToolsOpenFor(content::WebContents* web_contents);
#endif

  // ... rest of class ...
 private:
  DevToolsDockSide dock_side_ = DEVTOOLS_DOCK_SIDE_BOTTOM;
  // ...
};
```

### In the source (devtools_window.cc):

Add the implementation for the static helper:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
// static
bool DevToolsWindow::IsDevToolsOpenFor(content::WebContents* web_contents) {
  return FindForInspectedWebContents(web_contents) != nullptr;
}
#endif
```

## Rationale

**Why expose dock state?**
- Astra commands (kAstraCommandDevToolsDockBottom, etc.) need to know the
  current dock state to determine if a dock command should be enabled.
- AstraDevToolsHelper wraps DevToolsWindow but shouldn't depend on private APIs.
- Exposing a simple getter is a minimal, low-risk change.

**Why not use friend class?**
- Adding `friend class astra::AstraDevToolsHelper` would also work.
- But friend declarations are more invasive and couple the classes more tightly.
- Public accessors are cleaner and follow the principle of least privilege.

**What `//astra` code does this delegate to?**
- `astra/browser/astra_devtools_helper.h` — `AstraDevToolsHelper` static class.
- Used by `AstraCommandDelegate` for DevTools commands.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `build/config/chrome_build.gni` (patch 0004)
- All added methods are behind this flag.

## Alternatives Considered

1. **Use DevToolsAgentHost** — Query DevTools state through the agent host.
   Possible, but the agent host doesn't expose dock side (that's a UI concept,
   not a protocol concept). Rejected — not sufficient for our needs.

2. **Friend class declaration** — Add `friend class astra::AstraDevToolsHelper`
   to DevToolsWindow. More coupling, but no public API surface increase.
   Considered as an alternative if upstream is resistant to adding public APIs.
   Rejected in favor of public accessors for cleaner architecture.

3. **Track dock state in Astra** — Maintain our own dock state mirror.
   Rejected — would get out of sync with actual DevTools state (e.g., if the
   user changes dock side from the DevTools UI itself).

4. **Use extensions API** — Build DevTools integration as a DevTools extension.
   Rejected — extensions don't have access to dock state APIs, and it would
   be architecturally wrong for a product layer feature.

## Risks & Rebase Concerns

- **Very low risk.** Adding a public getter to a class is a minimal change.
  The patch is small and unlikely to conflict with upstream changes.

- **Method naming:** If Chromium adds its own `dock_side()` public method
  in the future, we'd have a conflict. Mitigation: the build flag gating
  means the patch is conditional, and we can rename or remove our version
  if Chromium adds its own.

- **Graceful degradation:** If the patch fails to apply, AstraDevToolsHelper
  won't be able to query dock state. DevTools toggle commands will still
  work (they use the public ToggleDevToolsWindow API), but dock state
  commands might not report correct enabled/disabled state. Non-critical.

## Related

- Related patches: 0003 (command forwarding — DevTools commands use this)
- Astra source:
  - `astra/browser/astra_devtools_helper.h`
  - `astra/browser/astra_devtools_helper.cc`
  - `astra/ui/views/devtools/`
