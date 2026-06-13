# ADR-0023: Omnibox Astra Actions

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra integrates with the Chrome omnibox (address bar) to provide Astra-specific actions and search results. Users can type special prefixes in the address bar to trigger Astra features, such as:
- `> command` -- Run an Astra command.
- `@workspace Name` -- Switch to a workspace.
- `@tab query` -- Search open tabs.
- `@favorites query` -- Search favorite tabs.
- `@split` -- Toggle split view.

The architectural question is how Astra hooks into the omnibox suggestion system. Chromium's omnibox is built on the `AutocompleteProvider` pattern, with multiple providers contributing suggestions to `AutocompleteController`.

## Decision

Astra uses the **AutocompleteProvider pattern** with a small patch to register an `AstraOmniboxProvider` in `AutocompleteController`. The Astra provider generates suggestions for recognized prefix patterns, and the suggestions appear alongside standard omnibox results.

**AstraOmniboxProvider:**
- Implements (or wraps) the `AutocompleteProvider` interface.
- Generates suggestions when the user types a recognized Astra prefix.
- Each suggestion has a display string, description, relevance score, and an action type.
- Suggestions are sorted by relevance and mixed with other providers' results.
- The provider is stateless and queries Astra services at suggestion time.

**Supported prefixes:**
- `>` -- Command search (fuzzy search over Chrome + Astra commands, similar to command palette).
- `@workspace` or `@ws` -- Switch to workspace by name.
- `@tab` -- Search open tabs by title or URL.
- `@favorites` or `@fav` -- Search favorite tabs.
- `@split` -- Toggle split view.
- `@command` or `@cmd` -- Open command palette.

**Suggestion execution:**
- When the user selects an Astra omnibox suggestion, it triggers the corresponding action.
- Actions are dispatched through the standard command system (`BrowserCommandController` / `AstraCommandDelegate`) or directly to Astra services.
- Each suggestion has an `AstraOmniboxAction` that encapsulates what to do when executed.
- For tab/workspace switch actions, the navigation is handled in the browser process, not as a URL navigation.

**Integration with AutocompleteController:**
- A small patch adds `AstraOmniboxProvider` to the list of autocomplete providers.
- The patch is build-flag gated (`BUILDFLAG(IS_ASTRA_BRANDED)`).
- The provider is registered at the same point as other providers (in `AutocompleteController` construction or via a factory).
- The Astra provider has an appropriate relevance range so its results interleave correctly with other providers.

**AstraOmniboxManager:**
- Browser-level coordinator that owns the Astra omnibox provider and manages its lifecycle.
- Connects the provider to the browser context (profile, tab strip model, workspace service).
- Handles execution of Astra actions in the browser context.

**Omnibox decoration:**
- A small location bar decoration (icon or badge) indicates when an Astra action is available or active.
- Implemented as a `views::View` added to the location bar via a patch point.
- See `AstraLocationBarDecoration` in `astra/ui/views/omnibox/`.

## Consequences

Positive:

- Native feel: Astra suggestions look and behave like standard omnibox suggestions.
- Reuses Chromium's omnibox infrastructure: suggestion rendering, keyboard navigation, accessibility, IME support.
- Provider pattern is the standard Chromium extension point for omnibox functionality.
- Suggestions are mixed with other results in a unified list, consistent with user expectations.
- The Astra provider is self-contained -- most logic lives in `//astra`, with only a small registration patch in Chromium.

Negative:

- Requires a patch to `AutocompleteController` to register the custom provider. The exact patch point depends on how providers are created in the current Chromium version.
- Omnibox APIs can change between Chromium versions, requiring rebase work.
- The provider must be careful about performance -- suggestion generation must be fast (synchronous in the current model) or use async patterns properly.
- Maintaining prefix-based commands in two places (omnibox + command palette) could lead to drift. The command palette model should be the source of truth for command search, with the omnibox provider reusing it.

Neutral:

- Astra suggestions appear only for recognized prefixes. For normal URL/search queries, the omnibox behaves exactly like Chrome.
- The `>` prefix for commands overlaps with the command palette. This is intentional -- both access the same commands but in different UI contexts.

## Alternatives Considered

### Omnibox pedals (OmniboxPedalProvider)
Use Chromium's omnibox pedal system for Astra actions.

- Considered: Omnibox pedals are a simpler system for single-action suggestions (e.g., "clear browsing data"). However, they are designed for a small set of fixed actions, not for dynamic lists like tab search or workspace switching. The full `AutocompleteProvider` pattern is more flexible.

### Side search / side panel
Put Astra actions in a side panel instead of the omnibox.

- Rejected: The omnibox is the primary command surface in a browser. Users expect to type commands and searches there. A side panel would be less discoverable and less efficient.

### Command palette only
Skip omnibox integration entirely and rely on the command palette (`Cmd/Ctrl+K`).

- Rejected: The omnibox is more discoverable and always available. Users already use the address bar for navigation and search; extending it with Astra actions provides a natural transition. The command palette complements the omnibox but does not replace it.

### Chrome extensions omnibox API
Use the `chrome.omnibox` extension API with a built-in extension.

- Rejected: Extension IPC overhead, limited functionality compared to native providers, and bundling a built-in extension adds complexity. A native provider integrated directly is faster and more capable.

## References

- **Chromium subsystems reused:** `AutocompleteProvider`, `AutocompleteController`, `AutocompleteMatch`, `OmniboxEditModel`, `OmniboxPedalProvider` (reference)
- **Astra components:** `AstraOmniboxProvider`, `AstraOmniboxManager`, `AstraOmniboxAction`, `AstraLocationBarDecoration`
- **Patch points:** `AutocompleteController` provider registration (`components/omnibox/browser/autocomplete_controller.cc`), location bar decoration install (`chrome/browser/ui/views/location_bar/location_bar_view.cc`)
- **Related patches:** 0010-location-bar-decoration.md, 0011-omnibox-astra-provider.md
