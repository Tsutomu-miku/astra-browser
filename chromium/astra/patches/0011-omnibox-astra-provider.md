# 0011 — Inject Astra suggestions into omnibox autocomplete

**Patch ID:** 0011
**File:** `components/omnibox/browser/autocomplete_controller.cc`
(alternative: `components/omnibox/browser/omnibox_pedal_provider.cc`)
**Size estimate:** ~40 lines
**Status:** planned
**Astra component:** `astra/browser/astra_omnibox_provider.h`
`astra/browser/astra_omnibox_manager.h`
`astra/browser/astra_omnibox_action.h`

## Context

The Chromium omnibox uses a set of `AutocompleteProvider` instances to
generate suggestions as the user types. Each provider specializes in one
kind of result (history, search, bookmarks, etc.). The
`AutocompleteController` orchestrates all providers and merges their results.

Astra needs to inject its own suggestions (workspace switch, tab search,
command execution) when the user types special prefixes like `>`,
`@workspace`, `@tab`, etc.

This cannot be done from `//astra` alone because the set of autocomplete
providers is fixed at build time and the `AutocompleteController` has no
plug-in mechanism for third-party providers. A small patch adds an
Astra-specific provider to the provider list.

**Two approaches are documented below:**

### Approach A: Custom AutocompleteProvider (recommended)

Create a full `AutocompleteProvider` subclass in `//astra` and register it
with the `AutocompleteController`. This gives full control over suggestion
lifecycle, relevance scoring, and asynchronous behavior.

### Approach B: Extend OmniboxPedalProvider

Add Astra "pedals" to the existing `OmniboxPedalProvider`. Pedals are a
lighter-weight mechanism for "action" suggestions that don't navigate to
a URL. This is a smaller patch but more constrained.

---

## Change — Approach A: Custom AutocompleteProvider (recommended)

### Before

```cpp
// In AutocompleteController::AutocompleteController() constructor,
// where providers are created.
AutocompleteController::AutocompleteController(...) {
  // ... existing providers (history, search, bookmark, etc.) ...
  providers_.push_back(base::MakeRefCounted<HistoryURLProvider>(...));
  providers_.push_back(base::MakeRefCounted<SearchProvider>(...));
  providers_.push_back(base::MakeRefCounted<BookmarkProvider>(...));
  // ... more providers ...
}
```

### After

Include at the top of the file, guarded by build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/browser/astra_omnibox_manager.h"
#endif
```

In the constructor, after other providers are created:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra-specific suggestions (workspace switch, tab search, commands).
  providers_.push_back(base::MakeRefCounted<AstraAutocompleteProvider>(
      profile_, astra::AstraOmniboxManager::GetForProfile(profile_)));
#endif
```

**The AstraAutocompleteProvider class** would live in `//astra/browser/`
(or `//astra/components/`) and implement the `AutocompleteProvider`
interface. It wraps `AstraOmniboxProvider` and converts
`AstraOmniboxSuggestion` objects into real `AutocompleteMatch` objects.

```cpp
// In astra/browser/astra_autocomplete_provider.h (not yet created):
//
// class AstraAutocompleteProvider : public AutocompleteProvider {
//  public:
//   explicit AstraAutocompleteProvider(Profile* profile,
//                                       astra::AstraOmniboxManager* manager);
//
//   // AutocompleteProvider:
//   void Start(const AutocompleteInput& input, bool minimal_changes) override;
//   void Stop(bool clear_cached_results, bool immediate) override;
//
//  private:
//   raw_ptr<astra::AstraOmniboxManager> manager_;
// };
```

The `Start()` method would:
1. Check if the input matches an Astra prefix via `manager_->MatchesAstraPrefix()`
2. If yes, call `manager_->GetSuggestions()` to get Astra suggestions
3. Convert each `AstraOmniboxSuggestion` to an `AutocompleteMatch`:
   - `fill_into_edit` = the full text (prefix + query)
   - `contents` = display_text
   - `description` = description
   - `relevance` = suggestion.relevance
   - `type` = `AutocompleteMatchType::EXTENSION_APP` or similar
   - `action` = an `OmniboxAction` that calls `ExecuteAstraOmniboxAction`

---

## Change — Approach B: Extend OmniboxPedalProvider

Smaller patch, reuses the existing pedal infrastructure.

### Before

```cpp
// In OmniboxPedalProvider::AddPedalMatches() or wherever pedals are collected.
void OmniboxPedalProvider::AddPedalMatches(
    const AutocompleteInput& input,
    ACMatches& matches) {
  // ... existing pedal matching ...
}
```

### After

Include at the top of the file, guarded by build flag:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/browser/astra_omnibox_provider.h"
#endif
```

In the method where pedals are collected:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Add Astra-specific pedals (workspace switch, tab search, commands).
  AddAstraPedalMatches(input, matches);
#endif
```

Helper function (could be in the same file or in `//astra`):

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
void OmniboxPedalProvider::AddAstraPedalMatches(
    const AutocompleteInput& input,
    ACMatches& matches) {
  astra::AstraOmniboxProvider provider;
  if (!provider.MatchesQuery(input.text())) {
    return;
  }
  auto suggestions = provider.GetSuggestions(
      /*browser=*/nullptr, input.text());
  for (const auto& suggestion : suggestions) {
    AutocompleteMatch match;
    match.provider = this;
    match.type = AutocompleteMatchType::PEDAL;
    match.contents = suggestion.display_text;
    match.description = suggestion.description;
    match.relevance = suggestion.relevance;
    // ... set up OmniboxAction to execute the Astra action ...
    matches.push_back(std::move(match));
  }
}
#endif
```

---

## Rationale

**Why approach A (custom provider)?**
- Full control over suggestion behavior (async, caching, relevance).
- Clean separation: Astra provider is self-contained.
- Follows the same pattern as all other omnibox providers.
- Can support both prefix-triggered and contextual suggestions.

**Why approach B (pedal provider)?**
- Smaller patch surface area.
- Reuses existing pedal UI rendering.
- Pedals are designed exactly for "action" suggestions (not navigation).

**Recommendation:** Start with approach A (custom provider) for maximum
flexibility and clean architecture. If the patch proves too fragile during
rebases, fall back to approach B (pedal provider) for a smaller surface.

**What `//astra` code does this delegate to?**
- `astra::AstraOmniboxProvider` — generates suggestions based on prefixes.
- `astra::AstraOmniboxManager` — coordinator and profile-scoped access.
- `astra::ExecuteAstraOmniboxAction` — executes selected actions.

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)**
- **Build flag defined in:** `astra/build/astra_buildflags.h`
- **Default:** off (no Astra suggestions in plain Chromium builds)
- The Astra provider target (`//astra/browser`) must be a dependency
  of the omnibox target.

## Alternatives Considered

1. **`AutocompleteProvider` from `//components` level** — Could be done
   but requires more extensive build system changes and `//components`
   doesn't naturally depend on `//chrome/browser` (which Astra does).

2. **WebUI-based fake omnibox** — Would miss all the native omnibox UX
   (keyboard navigation, suggestion rendering, accessibility, IME support).
   Rejected — we want the real omnibox, not a fake one.

3. **Overriding from the embedder layer** — `//chrome` is already an
   embedder of `//components/omnibox`. Adding Astra at the `//chrome`
   level would make more sense than at the components level, but the
   provider list is defined in `components/omnibox/browser/`, not
   in `//chrome`.

4. **Using the extension omnibox API** — Chrome extensions have an omnibox
   API that lets extensions provide suggestions for a keyword prefix.
   This would be architecturally cleaner but slower and less integrated.
   Astra is not an extension. Rejected.

## Risks & Rebase Concerns

- **Medium-to-high risk.** The autocomplete provider system changes
  periodically as the omnibox team refactors suggestion pipelines.
  Approach A (custom provider) is more likely to break with upstream
  changes because it depends on the `AutocompleteProvider` interface.

- **Interface stability:** Watch for changes to:
  - `AutocompleteProvider::Start()` signature
  - `AutocompleteMatch` field additions/removals
  - Provider type enum changes
  - `OmniboxPedal` API changes (if using approach B)

- **Performance:** Astra suggestions are computed synchronously and should
  be fast (string matching against workspace/tab lists). For large tab
  counts, we may need to make it asynchronous — the `AutocompleteProvider`
  interface supports this via `Done()`.

- **Graceful degradation:** If the patch fails to apply, the omnibox
  simply won't show Astra suggestions. All other omnibox functionality
  (history, search, etc.) continues to work.

## Related

- Related patches: 0010 (location bar decoration), 0003 (command forwarding)
- Astra source: `astra/browser/astra_omnibox_provider.h`
- Astra source: `astra/browser/astra_omnibox_manager.h`
- Astra source: `astra/browser/astra_omnibox_action.h`
