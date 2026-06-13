# 0001 — Register AstraBrowserMainExtraParts

**Patch ID:** 0001
**File:** `chrome/browser/chrome_browser_main.cc`
**Size estimate:** ~5 lines
**Status:** planned
**Astra component:** `astra/app/astra_browser_main_extra_parts.h`

## Context

Chromium's browser startup is orchestrated by `ChromeBrowserMainParts`, which
implements the `BrowserMainParts` interface. During startup, it creates a set
of `ChromeBrowserMainExtraParts` instances that hook into various phases of
the startup lifecycle (PreProfileInit, PostProfileInit, PostBrowserStart, etc.).

Astra needs a hook into the browser startup sequence to initialize
Astra-specific services (workspace metadata, etc.) at the correct phases of
Chrome browser startup. The `ChromeBrowserMainExtraParts` interface exists
exactly for this kind of extension — it lets embedders and product layers
plug into startup without modifying the core startup flow.

This cannot be done from `//astra` alone because the extra parts must be
registered during `ChromeBrowserMainParts` construction, which happens in
`chrome/browser/chrome_browser_main.cc`. A tiny patch adds the Astra extra
parts instance.

## Change

### Before

```cpp
// Inside ChromeBrowserMainParts::CreateExtraParts(),
// after other ChromeBrowserMainExtraParts subclasses are added.
void ChromeBrowserMainParts::CreateExtraParts() {
  // ... other extra parts registration ...
  AddParts(std::make_unique<ChromeBrowserMainExtraPartsProfiles>());
  // ... more extra parts ...
}
```

### After

Include at the top of the file, guarded by the build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/app/astra_browser_main_extra_parts.h"
#endif
```

Inside `ChromeBrowserMainParts::CreateExtraParts()`, after other extra parts
have been added:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  AddParts(std::make_unique<astra::AstraBrowserMainExtraParts>());
#endif
```

## Rationale

**Why ChromeBrowserMainExtraParts?**
- It is the standard extension point for browser startup in Chromium.
- It provides well-defined lifecycle hooks (PreProfileInit, PostProfileInit,
  PreBrowserStart, PostBrowserStart, PostMainMessageLoopRun).
- It is used by many Chrome features (profiles, metrics, extensions, etc.),
  so the pattern is well-established and stable.
- Astra initialization at startup fits naturally into this pattern.

**Why not AstraMainDelegate?**
- `AstraMainDelegate` is a higher-level concept that wraps multiple startup
  hooks. `AstraBrowserMainExtraParts` is the actual implementation that
  plugs into Chromium's startup. They work together — `AstraMainDelegate`
  is the conceptual entry point, and the extra parts is the concrete
  integration mechanism.

**What `//astra` code does this delegate to?**
- `astra/app/astra_browser_main_extra_parts.cc` — implements the extra parts
  with all Astra startup lifecycle hooks.
- `astra/browser/astra_workspace_service.*` — initialized via extra parts.
- Future Astra services will be added here as they are introduced.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `astra/build/astra_buildflags.h`
- **Default:** off (plain Chromium builds don't include Astra extra parts)
- All Astra includes and code are behind this flag.

## Alternatives Considered

1. **Patch ChromeMainDelegate directly** — Add Astra code to
   `chrome/app/chrome_main_delegate.cc` instead of using extra parts.
   Rejected: ChromeMainDelegate is more about process-level initialization,
   not browser-level. Extra parts are the right abstraction for browser
   startup features.

2. **Use BrowserListObserver** — Listen for the first Browser creation and
   initialize Astra services then. Rejected: Too late in the lifecycle —
   some services need to be ready before the first Browser is created
   (e.g., keyed service factories).

3. **PostTask at startup** — Schedule initialization via a delayed task.
   Rejected: Timing is unreliable, and initialization needs to happen at
   specific points in the startup sequence (before profile init, after
   profile init, etc.).

4. **Build a parallel startup system** — Create our own startup manager.
   Rejected: Massive duplication of Chromium's startup infrastructure.
   The whole point of direct Chromium is to reuse what exists.

## Risks & Rebase Concerns

- **Very low risk.** The `CreateExtraParts()` method is stable and changes
  rarely. The patch adds 3 lines in a well-defined location.
- **Graceful degradation:** If the patch fails to apply, Astra simply won't
  initialize its services. The browser will still start and function as
  plain Chromium. No crash, no data loss.
- **Ordering concerns:** Astra extra parts should be added late in the
  CreateExtraParts sequence so that Chrome's built-in extra parts are
  initialized first (since Astra depends on Chrome services like Profile
  and TabStripModel). The patch should place Astra after all built-in
  extra parts.

## Related

- ADR: `docs/adr/0009-direct-chromium-architecture.md`
- Related patches: 0005 (branding — build flag definition)
- Astra source: `astra/app/astra_browser_main_extra_parts.h`
- Astra source: `astra/app/astra_main_delegate.h`
- Astra source: `astra/browser/astra_workspace_service.h`
