#include "astra/browser/astra_omnibox_action.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

// Helper: converts a string payload to an integer command ID.
// Returns 0 if the payload is not a valid integer.
int PayloadToCommandId(base::StringPiece payload) {
  int command_id = 0;
  if (!base::StringToInt(payload, &command_id)) {
    return 0;
  }
  return command_id;
}

// Helper: gets the workspace service from a browser's profile.
AstraWorkspaceService* GetWorkspaceService(Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  return AstraWorkspaceServiceFactory::GetForProfile(browser->profile());
}

// =========================================================================
// Action catalog — static list of known Astra omnibox actions
// =========================================================================
//
// These are the Astra-specific actions that appear in the omnibox and
// command palette.  Each action has full metadata including title,
// description, icon, category, and keyboard shortcut.
//
// TODO(astra): Replace this static catalog with a dynamic registry so
// other Astra services (focus mode, screenshot, etc.) can register their
// own actions.  For now, all actions are listed here for simplicity.
//
// Chromium analog: OmniboxPedalProvider's built-in pedal list.
// =========================================================================

const AstraOmniboxAction kActionCatalog[] = {
    // -- Workspace category --
    {"switch_workspace", AstraOmniboxActionType::kSwitchWorkspace,
     u"Switch Workspace",
     u"Switch to a different workspace by name",
     "workspace",
     AstraOmniboxActionCategory::kWorkspace,
     u"Ctrl+`",
     ""},
    {"new_workspace", AstraOmniboxActionType::kRunCommand,
     u"New Workspace",
     u"Create a new workspace",
     "add_workspace",
     AstraOmniboxActionCategory::kWorkspace,
     u"",
     "60001"},  // kAstraCommandNewWorkspace

    // -- Tab category --
    {"search_tabs", AstraOmniboxActionType::kSearchTabs,
     u"Search Tabs",
     u"Find and switch to a tab by title or URL",
     "tab_search",
     AstraOmniboxActionCategory::kTab,
     u"Ctrl+Shift+A",
     ""},
    {"search_favorites", AstraOmniboxActionType::kSearchFavorites,
     u"Search Favorites",
     u"Find a favorite tab by title",
     "star",
     AstraOmniboxActionCategory::kTab,
     u"",
     ""},

    // -- Navigation category --
    {"open_command_palette", AstraOmniboxActionType::kOpenCommandPalette,
     u"Command Palette",
     u"Open the command palette to run any action",
     "command",
     AstraOmniboxActionCategory::kNavigation,
     u"Ctrl+Shift+P",
     ""},
    {"run_command", AstraOmniboxActionType::kRunCommand,
     u"Run Command",
     u"Execute an Astra command by ID",
     "play",
     AstraOmniboxActionCategory::kNavigation,
     u"",
     ""},

    // -- Tool category --
    {"toggle_split_view", AstraOmniboxActionType::kToggleSplitView,
     u"Toggle Split View",
     u"Enable or disable split view for the current tab",
     "split_view",
     AstraOmniboxActionCategory::kTool,
     u"Ctrl+\\",
     ""},
    {"toggle_focus_mode", AstraOmniboxActionType::kToggleFocusMode,
     u"Toggle Focus Mode",
     u"Start or stop a focus session",
     "focus",
     AstraOmniboxActionCategory::kTool,
     u"",
     ""},
    {"screenshot", AstraOmniboxActionType::kScreenshot,
     u"Screenshot",
     u"Take a screenshot of the visible area",
     "screenshot",
     AstraOmniboxActionCategory::kTool,
     u"Ctrl+Shift+S",
     "visible"},
    {"screenshot_region", AstraOmniboxActionType::kScreenshot,
     u"Screenshot Region",
     u"Take a screenshot of a selected region",
     "screenshot_region",
     AstraOmniboxActionCategory::kTool,
     u"",
     "region"},
    {"screenshot_full", AstraOmniboxActionType::kScreenshot,
     u"Screenshot Full Page",
     u"Take a screenshot of the entire page",
     "screenshot_full",
     AstraOmniboxActionCategory::kTool,
     u"",
     "full"},

    // -- Additional tool actions (commands) --
    {"toggle_sidebar", AstraOmniboxActionType::kRunCommand,
     u"Toggle Sidebar",
     u"Show or hide the Astra sidebar",
     "sidebar",
     AstraOmniboxActionCategory::kTool,
     u"Ctrl+B",
     "60000"},  // kAstraCommandToggleSidebar
    {"toggle_sidebar_pin", AstraOmniboxActionType::kRunCommand,
     u"Toggle Sidebar Pin",
     u"Pin the sidebar open or make it auto-hide",
     "pin",
     AstraOmniboxActionCategory::kTool,
     u"",
     "60001"},  // placeholder
};

// Computes a relevance score for |query| against |candidate|.
// Higher = more relevant.  Returns -1 if no match.
// Used by SearchActions for ranking.
int ComputeActionRelevance(const std::u16string& query,
                           const std::u16string& candidate) {
  if (query.empty()) {
    return 500;
  }

  std::u16string query_lower = base::ToLowerASCII(query);
  std::u16string candidate_lower = base::ToLowerASCII(candidate);

  if (base::StartsWith(candidate_lower, query_lower)) {
    // Prefix match — highest relevance.
    return 1100;
  }
  size_t pos = candidate_lower.find(query_lower);
  if (pos != std::u16string::npos) {
    // Substring match — medium relevance, bonus for earlier match.
    return 900 - static_cast<int>(pos) * 2;
  }
  return -1;
}

}  // namespace

// =========================================================================
// Action category helpers
// =========================================================================

const char* GetActionCategoryLabel(AstraOmniboxActionCategory category) {
  switch (category) {
    case AstraOmniboxActionCategory::kWorkspace:
      return "Workspace";
    case AstraOmniboxActionCategory::kTab:
      return "Tab";
    case AstraOmniboxActionCategory::kNavigation:
      return "Navigation";
    case AstraOmniboxActionCategory::kTool:
      return "Tool";
  }
  return "Unknown";
}

AstraOmniboxActionCategory GetActionCategory(AstraOmniboxActionType action_type) {
  switch (action_type) {
    case AstraOmniboxActionType::kNone:
      return AstraOmniboxActionCategory::kTool;
    case AstraOmniboxActionType::kSwitchWorkspace:
      return AstraOmniboxActionCategory::kWorkspace;
    case AstraOmniboxActionType::kSearchTabs:
    case AstraOmniboxActionType::kSearchFavorites:
      return AstraOmniboxActionCategory::kTab;
    case AstraOmniboxActionType::kRunCommand:
      // Commands span multiple categories; default to Navigation since
      // running a command is primarily a navigation action.
      return AstraOmniboxActionCategory::kNavigation;
    case AstraOmniboxActionType::kToggleSplitView:
    case AstraOmniboxActionType::kToggleFocusMode:
    case AstraOmniboxActionType::kScreenshot:
      return AstraOmniboxActionCategory::kTool;
    case AstraOmniboxActionType::kOpenCommandPalette:
      return AstraOmniboxActionCategory::kNavigation;
  }
  return AstraOmniboxActionCategory::kTool;
}

// =========================================================================
// Action catalog
// =========================================================================

std::vector<AstraOmniboxAction> GetAllOmniboxActions() {
  std::vector<AstraOmniboxAction> actions;
  for (const auto& action : kActionCatalog) {
    actions.push_back(action);
  }
  return actions;
}

std::vector<AstraOmniboxAction> GetActionsByCategory(
    AstraOmniboxActionCategory category) {
  std::vector<AstraOmniboxAction> actions;
  for (const auto& action : kActionCatalog) {
    if (action.category == category) {
      actions.push_back(action);
    }
  }
  return actions;
}

std::vector<AstraOmniboxAction> SearchActions(const std::u16string& query) {
  std::vector<AstraOmniboxAction> results;

  for (const auto& action : kActionCatalog) {
    // Try matching against title first.
    int title_score = ComputeActionRelevance(query, action.title);

    // Also try matching against description with lower score.
    int desc_score = ComputeActionRelevance(query, action.description);
    if (desc_score >= 0) {
      desc_score -= 200;
    }

    // Also try matching against the ID.
    int id_score =
        ComputeActionRelevance(query, base::UTF8ToUTF16(action.id));
    if (id_score >= 0) {
      id_score -= 100;
    }

    int score = std::max({title_score, desc_score, id_score});
    if (score < 0) {
      continue;
    }

    // Include the action.  We sort later by a temporary "score" — but
    // since we don't have a score field on the struct, we sort by
    // relevance manually below.
    results.push_back(action);
  }

  // Sort by relevance (approximate: title prefix matches first,
  // then title substring, then description, then id).
  // We re-score during sort to determine order.
  std::sort(results.begin(), results.end(),
            [&query](const AstraOmniboxAction& a, const AstraOmniboxAction& b) {
              int a_score = std::max(
                  {ComputeActionRelevance(query, a.title),
                   ComputeActionRelevance(query, a.description) - 200,
                   ComputeActionRelevance(query, base::UTF8ToUTF16(a.id)) - 100});
              int b_score = std::max(
                  {ComputeActionRelevance(query, b.title),
                   ComputeActionRelevance(query, b.description) - 200,
                   ComputeActionRelevance(query, base::UTF8ToUTF16(b.id)) - 100});
              return a_score > b_score;
            });

  return results;
}

std::vector<AstraOmniboxAction> SearchActionsInCategory(
    AstraOmniboxActionCategory category,
    const std::u16string& query) {
  std::vector<AstraOmniboxAction> all = SearchActions(query);
  std::vector<AstraOmniboxAction> filtered;
  for (const auto& action : all) {
    if (action.category == category) {
      filtered.push_back(action);
    }
  }
  return filtered;
}

bool GetActionMetadata(AstraOmniboxActionType action_type,
                       AstraOmniboxAction* out_action) {
  if (!out_action) {
    return false;
  }

  for (const auto& action : kActionCatalog) {
    if (action.type == action_type) {
      *out_action = action;
      return true;
    }
  }
  return false;
}

// =========================================================================
// Action execution
// =========================================================================

bool ExecuteAstraOmniboxAction(Browser* browser,
                               AstraOmniboxActionType action_type,
                               base::StringPiece payload) {
  if (!browser) {
    return false;
  }

  switch (action_type) {
    case AstraOmniboxActionType::kNone:
      return false;

    case AstraOmniboxActionType::kSwitchWorkspace: {
      AstraWorkspaceService* service = GetWorkspaceService(browser);
      if (!service) {
        return false;
      }
      std::string workspace_id(payload);
      if (!service->GetWorkspace(workspace_id)) {
        return false;
      }
      service->ActivateWorkspace(workspace_id);
      return true;
    }

    case AstraOmniboxActionType::kSearchTabs: {
      // TODO(astra): Implement tab switching by tab index or identifier.
      // The payload currently carries a tab index as a string (e.g. "3").
      // We should validate the index and activate the corresponding tab.
      //
      // Chromium subsystem reused: TabStripModel.
      // Patch point: this function is called from a patched AutocompleteMatch
      // action handler; the actual tab activation uses TabStripModel.
      int tab_index = 0;
      if (!base::StringToInt(payload, &tab_index)) {
        return false;
      }
      TabStripModel* tab_strip = browser->tab_strip_model();
      if (tab_index < 0 ||
          tab_index >= static_cast<int>(tab_strip->count())) {
        return false;
      }
      tab_strip->ActivateTabAt(tab_index);
      return true;
    }

    case AstraOmniboxActionType::kRunCommand: {
      int command_id = PayloadToCommandId(payload);
      if (command_id == 0 || !AstraCommandDelegate::IsAstraCommand(command_id)) {
        return false;
      }
      return AstraCommandDelegate::ExecuteCommand(browser, command_id);
    }

    case AstraOmniboxActionType::kSearchFavorites: {
      // TODO(astra): Implement favorite tab search and activation.
      // The payload could be a tab index or a favorite identifier.
      // For now we activate the tab at the given index as a stand-in,
      // but real implementation would search among favorited tabs only.
      //
      // Chromium subsystem reused: TabStripModel + AstraTabFeatures.
      // We filter TabStripModel by AstraTabFeatures::is_favorite().
      int tab_index = 0;
      if (!base::StringToInt(payload, &tab_index)) {
        return false;
      }
      TabStripModel* tab_strip = browser->tab_strip_model();
      if (tab_index < 0 ||
          tab_index >= static_cast<int>(tab_strip->count())) {
        return false;
      }
      tab_strip->ActivateTabAt(tab_index);
      return true;
    }

    case AstraOmniboxActionType::kToggleSplitView:
      return AstraCommandDelegate::ExecuteCommand(
          browser, kAstraCommandToggleSplitView);

    case AstraOmniboxActionType::kOpenCommandPalette: {
      // TODO(astra): Pass the prefill text to the command palette so it
      // opens with the user's current query already filled in.
      // This requires extending AstraCommandDelegate::Observer with an
      // OnOpenCommandPaletteWithQuery(const std::u16string& query) method.
      //
      // For now, just open the palette without prefill.
      return AstraCommandDelegate::ExecuteCommand(
          browser, kAstraCommandOpenCommandPalette);
    }

    case AstraOmniboxActionType::kToggleFocusMode:
      return AstraCommandDelegate::ExecuteCommand(
          browser, kAstraCommandToggleFocusMode);

    case AstraOmniboxActionType::kScreenshot: {
      // Dispatch to the appropriate screenshot command based on payload.
      std::string mode(payload);
      if (mode == "region") {
        return AstraCommandDelegate::ExecuteCommand(
            browser, kAstraCommandScreenshotRegion);
      } else if (mode == "full") {
        return AstraCommandDelegate::ExecuteCommand(
            browser, kAstraCommandScreenshotFullPage);
      } else {
        // Default: visible area screenshot.
        return AstraCommandDelegate::ExecuteCommand(
            browser, kAstraCommandScreenshotVisible);
      }
    }
  }

  return false;
}

bool ExecuteAstraOmniboxSuggestion(Browser* browser,
                                   const AstraOmniboxSuggestion& suggestion) {
  return ExecuteAstraOmniboxAction(browser, suggestion.action_type,
                                   suggestion.payload);
}

// =========================================================================
// Action type labels
// =========================================================================

std::u16string GetActionTypeLabel(AstraOmniboxActionType action_type) {
  switch (action_type) {
    case AstraOmniboxActionType::kNone:
      return u"";
    case AstraOmniboxActionType::kSwitchWorkspace:
      return u"@workspace";
    case AstraOmniboxActionType::kSearchTabs:
      return u"@tab";
    case AstraOmniboxActionType::kRunCommand:
      return u">";
    case AstraOmniboxActionType::kSearchFavorites:
      return u"@favorites";
    case AstraOmniboxActionType::kToggleSplitView:
      return u"@split";
    case AstraOmniboxActionType::kOpenCommandPalette:
      return u"@command";
    case AstraOmniboxActionType::kToggleFocusMode:
      return u"@focus";
    case AstraOmniboxActionType::kScreenshot:
      return u"@screenshot";
  }
  return u"";
}

}  // namespace astra
