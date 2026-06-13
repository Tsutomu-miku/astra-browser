#ifndef ASTRA_BROWSER_ASTRA_OMNIBOX_PROVIDER_H_
#define ASTRA_BROWSER_ASTRA_OMNIBOX_PROVIDER_H_

#include <string>
#include <vector>

#include "base/strings/string_piece.h"

class Browser;
class Profile;

namespace astra {

enum class AstraOmniboxActionCategory;
struct AstraOmniboxSuggestion;

// =========================================================================
// Astra omnibox suggestion provider
// =========================================================================
//
// AstraOmniboxProvider generates Astra-specific omnibox suggestions for
// special prefixes in the address bar.  It is conceptually similar to an
// AutocompleteProvider but does NOT implement the full Chromium
// AutocompleteProvider interface — that requires patching Chromium to
// register a custom provider.
//
// Instead, this class contains the Astra-side suggestion logic.  A small
// patch to Chromium's AutocompleteController (or OmniboxPedalProvider)
// calls into this provider when the user types a recognized Astra prefix,
// and converts the returned AstraOmniboxSuggestion objects into real
// AutocompleteMatch entries.
//
// Supported prefix patterns:
//
//   >command        ->  Execute an Astra command (e.g. ">sidebar", ">split")
//   @workspace name ->  Switch to a workspace by name (e.g. "@workspace Work")
//   @tab query      ->  Search tabs by title/URL
//   @favorites q    ->  Search favorite tabs
//   @split          ->  Toggle split view
//   @command        ->  Open command palette
//   @focus          ->  Toggle focus mode
//   @screenshot     ->  Take a screenshot
//
// The provider has its own enabled state and max-suggestions cap.  The
// AstraOmniboxManager may add additional filtering on top (e.g. category
// filtering, presentation settings).
//
// Chromium owner: AutocompleteProvider (components/omnibox/browser/autocomplete_provider.h)
// and OmniboxPedalProvider (components/omnibox/browser/omnibox_pedal_provider.h).
//
// TODO(astra): Implement a full AutocompleteProvider subclass once the
// Chromium patch to register custom providers is in place.  Patch point:
// components/omnibox/browser/autocomplete_controller.cc — add an Astra
// provider to the provider list, guarded by BUILDFLAG(IS_ASTRA_BRANDED).
// =========================================================================

class AstraOmniboxProvider {
 public:
  // Prefix kind — which Astra action domain the user is querying.
  enum class PrefixKind {
    kNone,          // Not an Astra prefix.
    kCommand,       // ">" prefix — run a command.
    kWorkspace,     // "@workspace" prefix — switch workspace.
    kTab,           // "@tab" prefix — search tabs.
    kFavorites,     // "@favorites" prefix — search favorites.
    kSplit,         // "@split" prefix — toggle split view.
    kCommandPalette, // "@command" prefix — open command palette.
    kFocus,         // "@focus" prefix — toggle focus mode.
    kScreenshot,    // "@screenshot" prefix — take a screenshot.
  };

  AstraOmniboxProvider();
  ~AstraOmniboxProvider();

  AstraOmniboxProvider(const AstraOmniboxProvider&) = delete;
  AstraOmniboxProvider& operator=(const AstraOmniboxProvider&) = delete;

  // -- Enabled state ------------------------------------------------------

  // Whether the provider is enabled.  When disabled, GetSuggestions()
  // returns an empty list regardless of input.
  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }

  // -- Max suggestions ---------------------------------------------------

  // Maximum number of suggestions returned by GetSuggestions.
  size_t max_suggestions() const { return max_suggestions_; }
  void set_max_suggestions(size_t max) { max_suggestions_ = max; }

  // Default max suggestions constant.
  static constexpr size_t kDefaultMaxSuggestions = 8;

  // -- Prefix detection --------------------------------------------------

  // Returns the prefix kind of |text|, or kNone if it doesn't match any
  // known Astra prefix.
  //
  // Matches are case-insensitive.  The prefix may be followed by optional
  // whitespace and a query string.
  static PrefixKind GetPrefixKind(base::StringPiece16 text);

  // Returns true if |text| starts with a recognized Astra omnibox prefix.
  // Convenience wrapper around GetPrefixKind.
  static bool MatchesQuery(base::StringPiece16 text);

  // Extracts the query portion of |text| (everything after the prefix
  // and optional whitespace).  If |text| has no Astra prefix, returns
  // the entire text.
  static std::u16string ExtractQuery(base::StringPiece16 text);

  // Returns the human-readable label for a prefix kind (e.g. "@workspace").
  static std::u16string GetPrefixLabel(PrefixKind kind);

  // Returns the action category for a prefix kind.
  static AstraOmniboxActionCategory GetPrefixCategory(PrefixKind kind);

  // -- Suggestions -------------------------------------------------------

  // Generates Astra-specific suggestions for |text| in the context of
  // |browser|.
  //
  // If |text| does not match any Astra prefix, or if the provider is
  // disabled, returns an empty list.
  //
  // The suggestions are sorted by relevance (highest first).  The number
  // of suggestions is capped at max_suggestions().
  //
  // |browser| is used to access profile-scoped services (workspace list,
  // favorite service) and tab strip state (for tab search).
  std::vector<AstraOmniboxSuggestion> GetSuggestions(Browser* browser,
                                                     base::StringPiece16 text);

  // Generates suggestions for a specific prefix kind and query.
  // This is useful when the caller already knows the prefix kind.
  std::vector<AstraOmniboxSuggestion> GetSuggestionsForPrefix(
      Browser* browser,
      PrefixKind kind,
      base::StringPiece16 query);

 private:
  // Generates command suggestions (">" prefix) matching |query|.
  // Returns suggestions for Astra commands whose names/descriptions
  // contain the query string.
  std::vector<AstraOmniboxSuggestion> GetCommandSuggestions(
      base::StringPiece16 query);

  // Generates workspace switch suggestions ("@workspace" prefix) matching
  // |query|.  Lists all workspaces whose name matches the query.
  std::vector<AstraOmniboxSuggestion> GetWorkspaceSuggestions(
      Profile* profile,
      base::StringPiece16 query);

  // Generates tab search suggestions ("@tab" prefix) matching |query|.
  // Searches the browser's tab strip for tabs whose title or URL matches.
  std::vector<AstraOmniboxSuggestion> GetTabSuggestions(
      Browser* browser,
      base::StringPiece16 query);

  // Generates favorite tab search suggestions ("@favorites" prefix)
  // matching |query|.  Searches favorited tabs.
  std::vector<AstraOmniboxSuggestion> GetFavoriteSuggestions(
      Browser* browser,
      base::StringPiece16 query);

  // Generates the split view toggle suggestion ("@split" prefix).
  std::vector<AstraOmniboxSuggestion> GetSplitViewSuggestion(
      base::StringPiece16 query);

  // Generates the command palette suggestion ("@command" prefix).
  std::vector<AstraOmniboxSuggestion> GetCommandPaletteSuggestion(
      base::StringPiece16 query);

  // Generates the focus mode suggestion ("@focus" prefix).
  std::vector<AstraOmniboxSuggestion> GetFocusModeSuggestion(
      base::StringPiece16 query);

  // Generates screenshot suggestions ("@screenshot" prefix).
  std::vector<AstraOmniboxSuggestion> GetScreenshotSuggestions(
      base::StringPiece16 query);

  // Whether the provider is currently enabled.
  bool enabled_ = true;

  // Maximum number of suggestions to return.
  size_t max_suggestions_ = kDefaultMaxSuggestions;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_OMNIBOX_PROVIDER_H_
