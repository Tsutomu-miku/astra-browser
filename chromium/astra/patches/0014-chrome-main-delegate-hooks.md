# 0014 — ChromeMainDelegate hooks

**Patch ID:** 0014
**File:** `chrome/app/chrome_main_delegate.cc`
(and optionally `chrome/app/chrome_main_delegate.h`)
**Size estimate:** ~15 lines
**Status:** planned
**Astra component:** `astra/app/astra_main_delegate.h`

## Context

`ChromeMainDelegate` is Chromium's entry-point delegate for the browser process.
It handles very early startup before the browser is fully initialized:
- Pre-sandbox setup (before the sandbox is engaged)
- Basic startup completion (after feature list and locale initialization)
- Browser main parts creation

Astra needs hooks at these early startup phases for:
- Pre-sandbox resource initialization (rare — kept minimal by design)
- Astra feature list registration
- Astra browser main extra parts registration

This cannot be done from `//astra` alone because `ChromeMainDelegate` is the
process-level entry point constructed by Chromium's main() function. A small
patch adds Astra delegate calls.

**Important:** Astra does NOT replace `ChromeMainDelegate`. It only adds thin
hooks for Astra-specific early startup. All core Chromium startup is untouched.

## Change

### In the source (chrome_main_delegate.cc):

#### Before

```cpp
// Include at the top of the file.
#include "chrome/app/chrome_main_delegate.h"
#include "chrome/browser/chrome_browser_main.h"
// ... other includes ...
```

#### After

Include the Astra header, guarded by build flag:

```cpp
#include "chrome/app/chrome_main_delegate.h"
#include "chrome/browser/chrome_browser_main.h"
// ... other includes ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/app/astra_main_delegate.h"
#endif
```

### In PreSandboxStartup:

#### Before

```cpp
void ChromeMainDelegate::PreSandboxStartup() {
  // ... Chromium pre-sandbox setup ...
}
```

#### After

```cpp
void ChromeMainDelegate::PreSandboxStartup() {
  // ... Chromium pre-sandbox setup ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra pre-sandbox setup.
  //
  // Keep this empty by default — most initialization should happen after
  // the sandbox is engaged to maintain Chromium's security model.
  // Only add pre-sandbox work here if it absolutely cannot happen later.
  //
  // Chromium subsystem: sandbox policy, resource loading
  // Astra owner: astra/app/astra_main_delegate.h
  astra::AstraMainDelegate::PreSandboxStartup();
#endif
}
```

### In BasicStartupComplete:

#### Before

```cpp
void ChromeMainDelegate::BasicStartupComplete() {
  // ... Chromium basic startup (feature list, locale, etc.) ...
}
```

#### After

```cpp
void ChromeMainDelegate::BasicStartupComplete() {
  // ... Chromium basic startup (feature list, locale, etc.) ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra basic startup complete hook.
  //
  // Called after Chromium's basic startup (feature list, locale, crash keys)
  // is done.  Astra uses this to log feature state and register any
  // post-startup hooks that depend on basic initialization being complete.
  astra::AstraMainDelegate::BasicStartupComplete();
#endif
}
```

### In CreateBrowserMainParts (optional — can also be done in chrome_browser_main.cc):

#### Before

```cpp
int ChromeMainDelegate::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters,
    std::unique_ptr<content::BrowserMainParts>* browser_main_parts) {
  // ... create ChromeBrowserMainParts ...
}
```

#### After

```cpp
int ChromeMainDelegate::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters,
    std::unique_ptr<content::BrowserMainParts>* browser_main_parts) {
  // ... create ChromeBrowserMainParts ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Register Astra's browser main extra parts.
  //
  // This is an alternative to patching chrome_browser_main.cc directly.
  // We can call RegisterBrowserMainExtraParts here instead of patching
  // ChromeBrowserMainParts::CreateExtraParts().
  //
  // Note: Currently handled by patch 0001 (chrome_browser_main.cc).
  // This is listed here as an alternative insertion point.
  if (chrome_browser_main_parts) {
    astra::AstraMainDelegate::RegisterBrowserMainExtraParts(
        chrome_browser_main_parts);
  }
#endif

  // ... return result code ...
}
```

## Rationale

**Why patch ChromeMainDelegate?**
- It is the process-level entry point — the first place product-specific
  code can run.
- Pre-sandbox setup must happen here (before the sandbox is engaged).
- Basic startup complete is the right place for post-feature-list initialization.

**Why keep it minimal?**
- More code in pre-sandbox = larger attack surface.
- Most Astra initialization should happen in BrowserMainExtraParts (patch 0001),
  which runs after the sandbox is engaged.
- The main delegate should only handle things that CAN'T be done later.

**What about the browser main extra parts registration?**
- Patch 0001 handles this in `chrome_browser_main.cc` via `CreateExtraParts()`.
- This patch adds an alternative entry point through `ChromeMainDelegate`.
- In practice, only one of these is needed. Patch 0001 is preferred because
  it uses the standard `ChromeBrowserMainExtraParts` pattern.

**What `//astra` code does this delegate to?**
- `astra/app/astra_main_delegate.h` — `AstraMainDelegate` static class.
- `astra/app/astra_browser_main_extra_parts.h` — actual initialization.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `build/config/chrome_build.gni` (patch 0004)
- All Astra includes and calls are behind this flag.

## Alternatives Considered

1. **Only use ChromeBrowserMainExtraParts** — No main delegate patch at all.
   This is the minimal approach and covers most needs. Consider this the
   default — only add a main delegate patch if Astra truly needs pre-sandbox
   or very early startup hooks.

2. **Patch chrome_main.cc directly** — Add Astra code at the very top of main().
   Rejected: too early, before anything is set up, and very fragile.

3. **Use a separate main() function** — Replace Chromium's main() with our own.
   Rejected: massive duplication of Chromium's main() logic, high rebase cost.

4. **Use the content embedder API** — Build as a proper content embedder
   instead of patching Chrome. Rejected: would lose all of chrome/ and
   require reimplementing huge amounts of browser functionality.

## Risks & Rebase Concerns

- **Low risk.** ChromeMainDelegate is relatively stable. The exact method
  signatures may change between major versions, but the overall structure
  (PreSandboxStartup, BasicStartupComplete) has been stable for years.

- **Pre-sandbox complexity:** Adding code before the sandbox is engaged
  is security-sensitive. Astra should keep this path empty by default
  and only add code here when absolutely necessary.

- **Graceful degradation:** If the patch fails to apply, Astra's early
  startup hooks won't run. Most Astra features should still work because
  they're initialized in BrowserMainExtraParts (patch 0001), which runs
  later in the startup sequence.

## Related

- ADR: `docs/adr/0009-direct-chromium-architecture.md`
- Related patches: 0001 (browser main extra parts — main initialization)
- Astra source:
  - `astra/app/astra_main_delegate.h`
  - `astra/app/astra_main_delegate.cc`
  - `astra/app/astra_browser_main_extra_parts.h`
