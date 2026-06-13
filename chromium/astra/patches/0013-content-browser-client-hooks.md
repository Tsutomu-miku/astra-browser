# 0013 — ChromeContentBrowserClient hooks

**Patch ID:** 0013
**File:** `chrome/browser/chrome_content_browser_client.cc`
(and optionally `chrome/browser/chrome_content_browser_client.h`)
**Size estimate:** ~15 lines total
**Status:** planned
**Astra component:** `astra/app/astra_content_browser_client.h`

## Context

`ChromeContentBrowserClient` is Chromium's main implementation of
`content::ContentBrowserClient`, the interface that embedders use to customize
browser process behavior: web preferences, permissions, URL policy, renderer
process management, WebUI registration, and more.

Astra needs to add small product-specific overrides to the browser client, such
as:
- Custom web preferences for Astra-specific surfaces (sidebar web views).
- Product-specific incognito URL policy.
- External URL handling overrides.

This cannot be done from `//astra` alone because `ChromeContentBrowserClient`
is a singleton constructed by Chromium. A small patch adds Astra hooks to
the existing client.

**Important:** Astra does NOT replace `ChromeContentBrowserClient`. It only
adds thin hooks for product-specific policy. All standard browser behavior
remains in Chromium's hands.

## Change

### In the header (optional, if forward declarations are needed):

#### Before

```cpp
class ChromeContentBrowserClient : public content::ContentBrowserClient {
  // ...
};
```

#### After

No header changes needed if all hooks are in the .cc file and use
forward-declared types. The Astra helper class is all-static, so it doesn't
need to be a member.

### In the source (chrome_content_browser_client.cc):

#### Before

```cpp
// Include at the top of the file.
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
// ... other includes ...
```

#### After

Include the Astra header, guarded by build flag:

```cpp
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
// ... other includes ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/app/astra_content_browser_client.h"
#endif
```

### In the constructor:

#### Before

```cpp
ChromeContentBrowserClient::ChromeContentBrowserClient() {
  // ... initialization ...
}
```

#### After

```cpp
ChromeContentBrowserClient::ChromeContentBrowserClient() {
  // ... initialization ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Install Astra-specific ChromeContentBrowserClient hooks.
  // This registers Astra policy overrides (web prefs, URL policy, etc.).
  // Kept minimal — Astra only adds product-specific tweaks.
  astra::AstraContentBrowserClient::InstallChromeContentBrowserClientHooks();
#endif
}
```

### In OverrideWebPreferences (if it exists):

#### Before

```cpp
void ChromeContentBrowserClient::OverrideWebPreferences(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* prefs) {
  // ... Chrome's web preference overrides ...
}
```

#### After

```cpp
void ChromeContentBrowserClient::OverrideWebPreferences(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* prefs) {
  // ... Chrome's web preference overrides ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra-specific web preference overrides for Astra surfaces.
  // Most web prefs are controlled by Chrome's existing pref service.
  // Only product-specific surfaces (e.g., sidebar web views) get
  // Astra-specific tweaks here.
  astra::AstraContentBrowserClient::OverrideWebPreferences(web_contents, prefs);
#endif
}
```

### In IsURLAllowedInIncognito (if it exists):

```cpp
bool ChromeContentBrowserClient::IsURLAllowedInIncognito(
    const GURL& url,
    content::BrowserContext* browser_context) {
  // ... Chrome's logic ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra-specific incognito URL policy.
  // Only use this for product-specific restrictions that cannot be expressed
  // through Safe Browsing or enterprise policy.
  if (!astra::AstraContentBrowserClient::IsURLAllowedInIncognito(url)) {
    return false;
  }
#endif

  return true;
}
```

## Rationale

**Why patch ChromeContentBrowserClient?**
- It is the standard extension point for embedder-level browser policy.
- It provides hooks for web preferences, URL policy, permissions, and
  renderer process management.
- Astra's product-level policy decisions (e.g., sidebar web view prefs)
  need a hook at this layer.

**Why not do it all from //astra?**
- The content browser client is a singleton constructed by Chromium.
- There's no plugin mechanism for adding extra policy hooks.
- A small patch is the only way to inject Astra-specific behavior.

**Why keep it minimal?**
- Most browser behavior should remain Chromium's responsibility.
- We only override things that are genuinely product-specific.
- Fewer overrides = fewer rebase conflicts = lower maintenance cost.

**What `//astra` code does this delegate to?**
- `astra/app/astra_content_browser_client.h` — static helper class.
- `astra/app/astra_content_browser_client.cc` — implementation.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `build/config/chrome_build.gni` (patch 0004)
- All Astra includes and calls are behind this flag.

## Alternatives Considered

1. **Subclass ChromeContentBrowserClient** — Create `AstraContentBrowserClient`
   as a subclass and replace the instance. Rejected: too invasive, would require
   patching the construction site, and high rebase cost.

2. **Use BrowserContext / Profile policy** — Handle policy at the profile
   level via PrefService. Rejected: some policy decisions (web prefs, URL
   filtering) need to be in the browser client, not per-profile.

3. **Use extensions** — Build Astra policy as an extension. Rejected:
   extensions don't have access to all the policy hooks we need, and it
   would be architecturally wrong.

4. **Do nothing** — Skip the browser client patch for now. Possible if
   Astra doesn't need custom web prefs or URL policy in the initial
   version. Rejected in principle — the hook point is valuable and the
   patch is tiny.

## Risks & Rebase Concerns

- **Low risk.** ChromeContentBrowserClient is a large class, but the patch
  adds only a few lines at well-defined hook points. The exact method
  signatures may shift slightly between Chromium versions, but the
  overall structure is stable.

- **Hook availability:** Not all methods exist in all Chromium revisions.
  Some hooks (like OverrideWebPreferences) may have different names or
  signatures. The patch should be adjusted to match the actual API.

- **Graceful degradation:** If the patch fails to apply, Astra-specific
  browser client customizations won't be active. The browser still works
  normally — only Astra-specific web pref tweaks and URL policies are
  missing.

## Related

- ADR: `docs/adr/0009-direct-chromium-architecture.md`
- Related patches: 0001 (browser main extra parts — also a startup hook)
- Astra source:
  - `astra/app/astra_content_browser_client.h`
  - `astra/app/astra_content_browser_client.cc`
