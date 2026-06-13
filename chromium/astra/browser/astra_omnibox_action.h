#ifndef ASTRA_BROWSER_ASTRA_OMNIBOX_ACTION_H_
#define ASTRA_BROWSER_ASTRA_OMNIBOX_ACTION_H_

#include <string>
#include <vector>

#include "base/strings/string_piece.h"

class Browser;

namespace astra {

// =========================================================================
// Astra omnibox action categories
// =========================================================================
//
// Broad categories for Astra omnibox actions, used for filtering and UI
// grouping.  These are analogous to Chromium's omnibox suggestion types
// (search, history, bookmark, etc.) but for Astra-specific actions.
//
// Categories:
//   - kWorkspace: workspace switching and management
//   - kTab: tab search, tab switching, favorite tabs
//   - kNavigation: command palette, navigation shortcuts
//   - kTool: utility actions like split view, focus mode, screenshots
// =========================================================================

enum class AstraOmniboxActionCategory {
  kWorkspace = 0,
  kTab = 1,
  kNavigation = 2,
  kTool = 3,
};

// Returns a human-readable label for an action category.
const char* GetActionCategoryLabel(AstraOmniboxActionCategory category);

// =========================================================================
// Astra omnibox action types
// =========================================================================
//
// Enumerates the kinds of actions an Astra omnibox suggestion can trigger.
// These are Astra-specific action types that complement Chromium's built-in
// OmniboxAction / OmniboxPedal kinds (navigation, search, history, etc.).
//
// Each action type corresponds to an Astra product feature:
//   - kSwitchWorkspace: switch to a named workspace
//   - kSearchTabs: open a tab search / switch to a matching tab
//   - kRunCommand: execute an Astra command (sidebar toggle, split view, etc.)
//   - kSearchFavorites: search among favorite tabs
//   - kToggleSplitView: directly toggle split view for the active tab
//   - kOpenCommandPalette: open the full command palette
//   - kFocusMode: start/stop focus mode
//   - kScreenshot: take a screenshot
//
// Chromium owner: OmniboxAction (components/omnibox/browser/actions/omnibox_action.h)
// and OmniboxPedal (components/omnibox/browser/omnibox_pedal.h).
//
// TODO(astra): Consider deriving from OmniboxAction directly once the
// omnibox provider patch is in place.  Currently these are standalone action
// descriptors that a patched AutocompleteProvider maps into real
// AutocompleteMatch objects with OmniboxAction set.
// =========================================================================

enum class AstraOmniboxActionType {
  // Sentinel: no action (should not occur in valid suggestions).
  kNone = 0,

  // Switch to a workspace by name.
  // Payload: workspace id string.
  // Category: kWorkspace.
  kSwitchWorkspace,

  // Search for and switch to a tab by title/URL.
  // Payload: tab index or tab identifier string.
  // Category: kTab.
  kSearchTabs,

  // Execute an Astra command by command ID.
  // Payload: integer command id (AstraCommandId).
  // Category: kNavigation / kTool (depends on the specific command).
  kRunCommand,

  // Search among favorite tabs by title.
  // Payload: favorite tab identifier.
  // Category: kTab.
  kSearchFavorites,

  // Toggle split view for the active tab.
  // Payload: empty (uses active tab context).
  // Category: kTool.
  kToggleSplitView,

  // Open the full command palette UI.
  // Payload: optional pre-filled query string.
  // Category: kNavigation.
  kOpenCommandPalette,

  // Toggle focus mode.
  // Payload: "on" or "off" or empty (toggle).
  // Category: kTool.
  kToggleFocusMode,

  // Take a screenshot.
  // Payload: "visible", "region", "full", or empty (default).
  // Category: kTool.
  kScreenshot,
};

// Returns the category that an action type belongs to.
AstraOmniboxActionCategory GetActionCategory(AstraOmniboxActionType action_type);

// =========================================================================
// AstraOmniboxAction
// =========================================================================
//
// Full metadata for an Astra omnibox action.  Represents a known action
// with all the information needed to display it in UI and execute it.
//
// This is distinct from AstraOmniboxSuggestion — an Action is a known,
// reusable definition (like "Toggle Sidebar"), while a Suggestion is a
// concrete match produced by the provider for a specific query.
//
// Fields:
//   - id: unique string identifier for the action (e.g. "toggle_sidebar")
//   - type: the AstraOmniboxActionType enum value
//   - title: user-visible short name (e.g. "Toggle Sidebar")
//   - description: longer description shown in secondary text
//   - icon: icon identifier string (e.g. "sidebar", "workspace")
//   - category: which category the action belongs to
//   - keyboard_shortcut: optional keyboard shortcut string (e.g. "Ctrl+B")
//   - default_payload: default payload for execution (may be empty)
//
// Chromium analog: OmniboxAction + AutocompleteMatch metadata.
// =========================================================================

struct AstraOmniboxAction {
  // Unique string identifier for the action.
  std::string id;

  // The action type enum value.
  AstraOmniboxActionType type = AstraOmniboxActionType::kNone;

  // Short user-visible title (primary display text).
  std::u16string title;

  // Longer description (secondary display text).
  std::u16string description;

  // Icon identifier (used by UI to pick the correct icon).
  std::string icon;

  // Which category this action belongs to.
  AstraOmniboxActionCategory category = AstraOmniboxActionCategory::kTool;

  // Optional keyboard shortcut string (e.g. "Ctrl+Shift+P").
  // Empty if no shortcut is assigned.
  std::u16string keyboard_shortcut;

  // Default payload string for execution.
  std::string default_payload;
};

// =========================================================================
// Action catalog
// =========================================================================
//
// The action catalog lists all known Astra omnibox actions with their full
// metadata.  It is used for searching, filtering, and displaying action
// lists (e.g. in the command palette or when the user types "> " in the
// omnibox).
//
// TODO(astra): Replace the static catalog with a dynamic registry that
// other Astra services can register actions into.  For now, all known
// actions are listed here.
// =========================================================================

// Returns the full list of known Astra omnibox actions.
std::vector<AstraOmniboxAction> GetAllOmniboxActions();

// Returns all actions belonging to |category|.
std::vector<AstraOmniboxAction> GetActionsByCategory(
    AstraOmniboxActionCategory category);

// Searches actions by title and description, returning all that match
// |query| (case-insensitive substring match).
// Results are sorted by relevance (prefix matches first, then title
// matches before description matches).
std::vector<AstraOmniboxAction> SearchActions(const std::u16string& query);

// Searches actions limited to |category|.
std::vector<AstraOmniboxAction> SearchActionsInCategory(
    AstraOmniboxActionCategory category,
    const std::u16string& query);

// Returns the action metadata for |action_type|, or nullopt if not found.
// Note: some action types (like kRunCommand and kSearchTabs) map to
// multiple concrete actions — this returns the "canonical" one.
bool GetActionMetadata(AstraOmniboxActionType action_type,
                       AstraOmniboxAction* out_action);

// =========================================================================
// AstraOmniboxSuggestion
// =========================================================================
//
// A single Astra-specific omnibox suggestion.  Analogous to a subset of
// AutocompleteMatch fields that the Astra provider generates and a patched
// Chromium AutocompleteProvider converts into a full AutocompleteMatch.
//
// Fields:
//   - display_text: the primary text shown in the suggestion (e.g.
//     "Switch to Work", "Toggle Sidebar").
//   - description: secondary text shown below or alongside the main text
//     (e.g. "@workspace Work", "Astra command").
//   - action_type: which kind of Astra action this suggestion triggers.
//   - payload: type-specific data used during execution (workspace id,
//     command id, tab index, etc.).
//   - relevance: integer relevance score for sorting among all suggestions.
//     Higher = more relevant.  Used by the patched AutocompleteProvider to
//     position Astra suggestions relative to native ones.
//   - category: the action's category (copied here for convenience so the
//     suggestion can be filtered without looking up metadata).
//
// This struct intentionally does not duplicate full AutocompleteMatch
// semantics — it carries only the fields the Astra layer needs to produce.
// The Chromium patch point converts it into a real AutocompleteMatch with
// icon, classification, etc.
//
// Chromium owner: AutocompleteMatch (components/omnibox/browser/autocomplete_match.h)
// =========================================================================

struct AstraOmniboxSuggestion {
  // Display text for the suggestion (primary line).
  std::u16string display_text;

  // Description / secondary text for the suggestion.
  std::u16string description;

  // Type of action this suggestion will trigger.
  AstraOmniboxActionType action_type = AstraOmniboxActionType::kNone;

  // Category of the action (convenience copy for filtering).
  AstraOmniboxActionCategory category = AstraOmniboxActionCategory::kTool;

  // Action-specific payload.  Interpretation depends on action_type:
  //   kSwitchWorkspace   -> workspace id (string)
  //   kRunCommand        -> command id as string, e.g. "60001"
  //   kSearchTabs        -> tab index as string, e.g. "3"
  //   kSearchFavorites   -> tab / favorite identifier
  //   kToggleSplitView   -> empty
  //   kOpenCommandPalette -> optional prefill text
  //   kToggleFocusMode   -> "on", "off", or empty (toggle)
  //   kScreenshot        -> "visible", "region", "full", or empty
  std::string payload;

  // Relevance score for ordering.  Higher = more relevant.
  // Astra suggestions typically score in the 500-1200 range to compete
  // with search and history suggestions.
  int relevance = 700;
};

// =========================================================================
// Action execution
// =========================================================================
//
// Executes an Astra omnibox action in the context of |browser|.
//
// Routes the action to the appropriate Astra service or command delegate:
//   - kSwitchWorkspace   -> AstraWorkspaceService::ActivateWorkspace
//   - kRunCommand        -> AstraCommandDelegate::ExecuteCommand
//   - kSearchTabs        -> TabStripModel::ActivateTabAt
//   - kSearchFavorites   -> (tab switch among favorited tabs)
//   - kToggleSplitView   -> AstraCommandDelegate (kAstraCommandToggleSplitView)
//   - kOpenCommandPalette -> AstraCommandDelegate (kAstraCommandOpenCommandPalette)
//   - kToggleFocusMode   -> AstraCommandDelegate (kAstraCommandToggleFocusMode)
//   - kScreenshot        -> AstraCommandDelegate (screenshot commands)
//
// Returns true if the action was recognized and executed successfully.
//
// This function is the execution counterpart to AstraOmniboxSuggestion.
// A patched OmniboxAction or AutocompleteController would call this to
// carry out the Astra action when the user selects a suggestion.
//
// Chromium owner: OmniboxAction::Execute (components/omnibox/browser/actions/)
//
// TODO(astra): Integrate with OmniboxAction via a patch to
// components/omnibox/browser/actions/omnibox_action.h so that Astra
// suggestions have a proper OmniboxAction object instead of relying on
// this standalone execution function.
// =========================================================================
bool ExecuteAstraOmniboxAction(Browser* browser,
                               AstraOmniboxActionType action_type,
                               base::StringPiece payload);

// Convenience overload: executes the action from a suggestion struct.
bool ExecuteAstraOmniboxSuggestion(Browser* browser,
                                   const AstraOmniboxSuggestion& suggestion);

// Returns a human-readable label for an action type, used in descriptions
// and UI annotations (e.g. "@workspace", "@command").
std::u16string GetActionTypeLabel(AstraOmniboxActionType action_type);

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_OMNIBOX_ACTION_H_
