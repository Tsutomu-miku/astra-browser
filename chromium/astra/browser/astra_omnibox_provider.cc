#include "astra/browser/astra_omnibox_provider.h"

#include <algorithm>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_omnibox_action.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

// =========================================================================
// Prefix constants
// =========================================================================
//
// All string comparisons are case-insensitive.
// Prefixes may be followed by optional whitespace and a query string.
//
// TODO(astra): Consider making these configurable via prefs or feature
// flags.  Some users may prefer different trigger characters (e.g. "/"
// instead of ">" for commands).
// =========================================================================

const char16_t kCommandPrefix[] = u">";
const char16_t kWorkspacePrefix[] = u"@workspace";
const char16_t kTabPrefix[] = u"@tab";
const char16_t kFavoritesPrefix[] = u"@favorites";
const char16_t kSplitPrefix[] = u"@split";
const char16_t kCommandPalettePrefix[] = u"@command";
const char16_t kFocusPrefix[] = u"@focus";
const char16_t kScreenshotPrefix[] = u"@screenshot";

// Checks whether |text| starts with |prefix| (case-insensitive), and
// optionally followed by whitespace.
bool StartsWithPrefix(base::StringPiece16 text, base::StringPiece16 prefix) {
  if (text.size() < prefix.size()) {
    return false;
  }
  if (!base::StartsWith(text, prefix, base::CompareCase::INSENSITIVE_ASCII)) {
    return false;
  }
  // The character after the prefix should be end-of-string or whitespace
  // for a "clean" prefix match.  This prevents "@tab" from matching
  // "@table" or ">foo" matching ">foobar" as a bare prefix.
  if (text.size() == prefix.size()) {
    return true;
  }
  char16_t next = text[prefix.size()];
  return next == ' ' || next == '\t';
}

// Strips the prefix and any leading whitespace from |text|.
// Assumes |text| starts with |prefix|.
std::u16string StripPrefix(base::StringPiece16 text,
                           base::StringPiece16 prefix) {
  base::StringPiece16 rest = text.substr(prefix.size());
  // Trim leading whitespace.
  size_t start = 0;
  while (start < rest.size() && (rest[start] == ' ' || rest[start] == '\t')) {
    ++start;
  }
  return std::u16string(rest.substr(start));
}

// Computes a relevance score for |query| against |candidate|.
// Higher = more relevant.  Returns -1 if no match.
int ComputeRelevance(base::StringPiece16 query,
                     base::StringPiece16 candidate) {
  if (query.empty()) {
    // Empty query: flat score so all candidates are shown in their
    // natural order.
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

// =========================================================================
// Command metadata for omnibox suggestions
// =========================================================================
//
// Subset of Astra commands that are useful as omnibox-triggerable actions.
// Not all commands are included — only those that make sense to run
// directly from the address bar (e.g. toggle actions, navigation,
// workspace operations).
//
// TODO(astra): Share this metadata with the command palette model instead
// of duplicating it.  The command palette has a more comprehensive list;
// the omnibox provider only includes the most relevant ones.
// =========================================================================

struct AstraOmniboxCommandEntry {
  int command_id;
  const char16_t* name;
  const char16_t* description;
};

const AstraOmniboxCommandEntry kOmniboxCommands[] = {
    // Sidebar
    {kAstraCommandToggleSidebar, u"Toggle Sidebar",
     u"Show or hide the Astra sidebar"},
    {kAstraCommandToggleSidebarPin, u"Toggle Sidebar Pin",
     u"Pin the sidebar open or make it auto-hide"},

    // Workspaces
    {kAstraCommandNewWorkspace, u"New Workspace", u"Create a new workspace"},
    {kAstraCommandNextWorkspace, u"Next Workspace",
     u"Switch to the next workspace"},
    {kAstraCommandPreviousWorkspace, u"Previous Workspace",
     u"Switch to the previous workspace"},
    {kAstraCommandShowAllWorkspaces, u"Show All Workspaces",
     u"Open the workspace overview"},

    // Favorites
    {kAstraCommandToggleTabFavorite, u"Toggle Favorite",
     u"Add or remove the current tab from favorites"},

    // Split view
    {kAstraCommandToggleSplitView, u"Toggle Split View",
     u"Enable or disable split view"},
    {kAstraCommandSplitViewVertical, u"Split View Vertical",
     u"Arrange split view vertically"},
    {kAstraCommandSplitViewHorizontal, u"Split View Horizontal",
     u"Arrange split view horizontally"},

    // Glance
    {kAstraCommandOpenGlance, u"Open Glance",
     u"Open a quick glance preview"},

    // Focus mode
    {kAstraCommandToggleFocusMode, u"Toggle Focus Mode",
     u"Start or stop a focus session"},

    // Screenshots
    {kAstraCommandScreenshotVisible, u"Screenshot Visible",
     u"Take a screenshot of the visible area"},
    {kAstraCommandScreenshotRegion, u"Screenshot Region",
     u"Take a screenshot of a selected region"},
    {kAstraCommandScreenshotFullPage, u"Screenshot Full Page",
     u"Take a screenshot of the entire page"},

    // Command palette
    {kAstraCommandOpenCommandPalette, u"Command Palette",
     u"Open the command palette"},
};

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraOmniboxProvider::AstraOmniboxProvider() = default;
AstraOmniboxProvider::~AstraOmniboxProvider() = default;

// =========================================================================
// Prefix detection
// =========================================================================

AstraOmniboxProvider::PrefixKind AstraOmniboxProvider::GetPrefixKind(
    base::StringPiece16 text) {
  // Check longer prefixes first to avoid shadowing by shorter ones.
  if (StartsWithPrefix(text, kWorkspacePrefix)) {
    return PrefixKind::kWorkspace;
  }
  if (StartsWithPrefix(text, kFavoritesPrefix)) {
    return PrefixKind::kFavorites;
  }
  if (StartsWithPrefix(text, kCommandPalettePrefix)) {
    return PrefixKind::kCommandPalette;
  }
  if (StartsWithPrefix(text, kScreenshotPrefix)) {
    return PrefixKind::kScreenshot;
  }
  if (StartsWithPrefix(text, kFocusPrefix)) {
    return PrefixKind::kFocus;
  }
  if (StartsWithPrefix(text, kTabPrefix)) {
    return PrefixKind::kTab;
  }
  if (StartsWithPrefix(text, kSplitPrefix)) {
    return PrefixKind::kSplit;
  }
  // ">" prefix must come last because it's the shortest and would
  // otherwise shadow longer prefixes if they also started with ">".
  if (StartsWithPrefix(text, kCommandPrefix)) {
    return PrefixKind::kCommand;
  }
  return PrefixKind::kNone;
}

bool AstraOmniboxProvider::MatchesQuery(base::StringPiece16 text) {
  return GetPrefixKind(text) != PrefixKind::kNone;
}

std::u16string AstraOmniboxProvider::ExtractQuery(base::StringPiece16 text) {
  switch (GetPrefixKind(text)) {
    case PrefixKind::kCommand:
      return StripPrefix(text, kCommandPrefix);
    case PrefixKind::kWorkspace:
      return StripPrefix(text, kWorkspacePrefix);
    case PrefixKind::kTab:
      return StripPrefix(text, kTabPrefix);
    case PrefixKind::kFavorites:
      return StripPrefix(text, kFavoritesPrefix);
    case PrefixKind::kSplit:
      return StripPrefix(text, kSplitPrefix);
    case PrefixKind::kCommandPalette:
      return StripPrefix(text, kCommandPalettePrefix);
    case PrefixKind::kFocus:
      return StripPrefix(text, kFocusPrefix);
    case PrefixKind::kScreenshot:
      return StripPrefix(text, kScreenshotPrefix);
    case PrefixKind::kNone:
      return std::u16string(text);
  }
  return std::u16string(text);
}

std::u16string AstraOmniboxProvider::GetPrefixLabel(PrefixKind kind) {
  switch (kind) {
    case PrefixKind::kNone:
      return u"";
    case PrefixKind::kCommand:
      return u">";
    case PrefixKind::kWorkspace:
      return u"@workspace";
    case PrefixKind::kTab:
      return u"@tab";
    case PrefixKind::kFavorites:
      return u"@favorites";
    case PrefixKind::kSplit:
      return u"@split";
    case PrefixKind::kCommandPalette:
      return u"@command";
    case PrefixKind::kFocus:
      return u"@focus";
    case PrefixKind::kScreenshot:
      return u"@screenshot";
  }
  return u"";
}

AstraOmniboxActionCategory AstraOmniboxProvider::GetPrefixCategory(
    PrefixKind kind) {
  switch (kind) {
    case PrefixKind::kWorkspace:
      return AstraOmniboxActionCategory::kWorkspace;
    case PrefixKind::kTab:
    case PrefixKind::kFavorites:
      return AstraOmniboxActionCategory::kTab;
    case PrefixKind::kCommandPalette:
    case PrefixKind::kCommand:
      return AstraOmniboxActionCategory::kNavigation;
    case PrefixKind::kSplit:
    case PrefixKind::kFocus:
    case PrefixKind::kScreenshot:
    case PrefixKind::kNone:
      return AstraOmniboxActionCategory::kTool;
  }
  return AstraOmniboxActionCategory::kTool;
}

// =========================================================================
// Suggestions entry point
// =========================================================================

std::vector<AstraOmniboxSuggestion> AstraOmniboxProvider::GetSuggestions(
    Browser* browser,
    base::StringPiece16 text) {
  if (!enabled_) {
    return {};
  }

  PrefixKind kind = GetPrefixKind(text);
  if (kind == PrefixKind::kNone) {
    return {};
  }

  std::u16string query = ExtractQuery(text);
  return GetSuggestionsForPrefix(browser, kind, query);
}

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetSuggestionsForPrefix(Browser* browser,
                                              PrefixKind kind,
                                              base::StringPiece16 query) {
  if (!enabled_) {
    return {};
  }

  std::vector<AstraOmniboxSuggestion> results;

  switch (kind) {
    case PrefixKind::kCommand:
      results = GetCommandSuggestions(query);
      break;
    case PrefixKind::kWorkspace:
      results = GetWorkspaceSuggestions(
          browser ? browser->profile() : nullptr, query);
      break;
    case PrefixKind::kTab:
      results = GetTabSuggestions(browser, query);
      break;
    case PrefixKind::kFavorites:
      results = GetFavoriteSuggestions(browser, query);
      break;
    case PrefixKind::kSplit:
      results = GetSplitViewSuggestion(query);
      break;
    case PrefixKind::kCommandPalette:
      results = GetCommandPaletteSuggestion(query);
      break;
    case PrefixKind::kFocus:
      results = GetFocusModeSuggestion(query);
      break;
    case PrefixKind::kScreenshot:
      results = GetScreenshotSuggestions(query);
      break;
    case PrefixKind::kNone:
      break;
  }

  // Sort by relevance (highest first).
  std::sort(results.begin(), results.end(),
            [](const AstraOmniboxSuggestion& a,
               const AstraOmniboxSuggestion& b) {
              return a.relevance > b.relevance;
            });

  // Cap at max suggestions.
  if (results.size() > max_suggestions_) {
    results.resize(max_suggestions_);
  }

  return results;
}

// =========================================================================
// Command suggestions ("> " prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetCommandSuggestions(base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  for (const auto& entry : kOmniboxCommands) {
    int score = ComputeRelevance(query, entry.name);
    if (score < 0) {
      // Also try matching against description as a fallback with lower score.
      int desc_score = ComputeRelevance(query, entry.description);
      if (desc_score >= 0) {
        score = desc_score - 200;  // Description matches are lower relevance.
      }
    }
    if (score < 0) {
      continue;
    }

    AstraOmniboxSuggestion suggestion;
    suggestion.display_text = entry.name;
    suggestion.description = entry.description;
    suggestion.action_type = AstraOmniboxActionType::kRunCommand;
    suggestion.category = AstraOmniboxActionCategory::kNavigation;
    suggestion.payload = base::NumberToString(entry.command_id);
    suggestion.relevance = score;
    results.push_back(std::move(suggestion));
  }

  return results;
}

// =========================================================================
// Workspace suggestions ("@workspace " prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetWorkspaceSuggestions(
    Profile* profile,
    base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  if (!profile) {
    return results;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return results;
  }

  const auto& workspaces = service->workspaces();
  for (const auto& workspace : workspaces) {
    std::u16string name = base::UTF8ToUTF16(workspace.name);
    int score = ComputeRelevance(query, name);
    if (score < 0 && !query.empty()) {
      continue;
    }

    AstraOmniboxSuggestion suggestion;
    suggestion.display_text = name;
    std::u16string desc = u"Switch to workspace " + name;
    if (workspace.is_default) {
      desc += u" (default)";
    }
    suggestion.description = desc;
    suggestion.action_type = AstraOmniboxActionType::kSwitchWorkspace;
    suggestion.category = AstraOmniboxActionCategory::kWorkspace;
    suggestion.payload = workspace.id;
    // Boost score for active workspace so it appears first.
    suggestion.relevance =
        score + (workspace.id == service->active_workspace_id() ? 50 : 0);
    results.push_back(std::move(suggestion));
  }

  return results;
}

// =========================================================================
// Tab search suggestions ("@tab " prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetTabSuggestions(Browser* browser,
                                        base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  if (!browser) {
    return results;
  }

  TabStripModel* tab_strip = browser->tab_strip_model();
  if (!tab_strip) {
    return results;
  }

  for (int i = 0; i < tab_strip->count(); ++i) {
    content::WebContents* contents = tab_strip->GetWebContentsAt(i);
    if (!contents) {
      continue;
    }

    std::u16string title = contents->GetTitle();
    int title_score = ComputeRelevance(query, title);

    // Also check URL as a lower-relevance match.
    std::u16string url =
        base::UTF8ToUTF16(contents->GetVisibleURL().spec());
    int url_score = ComputeRelevance(query, url);
    if (url_score >= 0) {
      url_score -= 100;  // URL matches are lower relevance than title matches.
    }

    int score = std::max(title_score, url_score);
    if (score < 0) {
      continue;
    }

    AstraOmniboxSuggestion suggestion;
    suggestion.display_text = title.empty() ? u"(Untitled)" : title;
    suggestion.description =
        base::UTF8ToUTF16(contents->GetVisibleURL().host());
    suggestion.action_type = AstraOmniboxActionType::kSearchTabs;
    suggestion.category = AstraOmniboxActionCategory::kTab;
    suggestion.payload = base::NumberToString(i);
    // Active tab gets a small boost.
    suggestion.relevance = score + (i == tab_strip->active_index() ? 30 : 0);
    results.push_back(std::move(suggestion));

    // Stop early if we have enough — no need to scan all tabs.
    if (results.size() >= max_suggestions_ * 2) {
      break;
    }
  }

  return results;
}

// =========================================================================
// Favorite suggestions ("@favorites " prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetFavoriteSuggestions(Browser* browser,
                                             base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  if (!browser) {
    return results;
  }

  TabStripModel* tab_strip = browser->tab_strip_model();
  if (!tab_strip) {
    return results;
  }

  for (int i = 0; i < tab_strip->count(); ++i) {
    content::WebContents* contents = tab_strip->GetWebContentsAt(i);
    if (!contents) {
      continue;
    }

    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(contents);
    if (!features || !features->is_favorite()) {
      continue;
    }

    std::u16string title = contents->GetTitle();
    int score = ComputeRelevance(query, title);
    if (score < 0 && !query.empty()) {
      continue;
    }

    AstraOmniboxSuggestion suggestion;
    suggestion.display_text = title.empty() ? u"(Untitled)" : title;
    suggestion.description =
        u"Favorite — " +
        base::UTF8ToUTF16(contents->GetVisibleURL().host());
    suggestion.action_type = AstraOmniboxActionType::kSearchFavorites;
    suggestion.category = AstraOmniboxActionCategory::kTab;
    suggestion.payload = base::NumberToString(i);
    suggestion.relevance = score;
    results.push_back(std::move(suggestion));

    if (results.size() >= max_suggestions_ * 2) {
      break;
    }
  }

  return results;
}

// =========================================================================
// Split view suggestion ("@split" prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetSplitViewSuggestion(base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  // Always show the toggle suggestion when "@split" is typed, regardless
  // of trailing query text.
  AstraOmniboxSuggestion suggestion;
  suggestion.display_text = u"Toggle Split View";
  suggestion.description = u"Enable or disable split view for the current tab";
  suggestion.action_type = AstraOmniboxActionType::kToggleSplitView;
  suggestion.category = AstraOmniboxActionCategory::kTool;
  suggestion.payload = "";
  suggestion.relevance = 1200;  // High relevance for direct action.
  results.push_back(std::move(suggestion));

  // If the user typed more, also show orientation options.
  if (!query.empty()) {
    AstraOmniboxSuggestion vertical;
    vertical.display_text = u"Split View Vertical";
    vertical.description = u"Arrange split view top-to-bottom";
    vertical.action_type = AstraOmniboxActionType::kRunCommand;
    vertical.category = AstraOmniboxActionCategory::kTool;
    vertical.payload = base::NumberToString(kAstraCommandSplitViewVertical);
    vertical.relevance = 1000;
    results.push_back(std::move(vertical));

    AstraOmniboxSuggestion horizontal;
    horizontal.display_text = u"Split View Horizontal";
    horizontal.description = u"Arrange split view side-by-side";
    horizontal.action_type = AstraOmniboxActionType::kRunCommand;
    horizontal.category = AstraOmniboxActionCategory::kTool;
    horizontal.payload = base::NumberToString(kAstraCommandSplitViewHorizontal);
    horizontal.relevance = 1000;
    results.push_back(std::move(horizontal));
  }

  return results;
}

// =========================================================================
// Command palette suggestion ("@command" prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetCommandPaletteSuggestion(base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  AstraOmniboxSuggestion suggestion;
  suggestion.display_text = u"Open Command Palette";
  suggestion.description =
      query.empty() ? u"Search all browser and Astra commands"
                    : u"Search commands for: " + std::u16string(query);
  suggestion.action_type = AstraOmniboxActionType::kOpenCommandPalette;
  suggestion.category = AstraOmniboxActionCategory::kNavigation;
  suggestion.payload = base::UTF16ToUTF8(query);
  suggestion.relevance = 1150;
  results.push_back(std::move(suggestion));

  return results;
}

// =========================================================================
// Focus mode suggestion ("@focus" prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetFocusModeSuggestion(base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  AstraOmniboxSuggestion suggestion;
  suggestion.display_text = u"Toggle Focus Mode";
  suggestion.description = u"Start or stop a focus session";
  suggestion.action_type = AstraOmniboxActionType::kToggleFocusMode;
  suggestion.category = AstraOmniboxActionCategory::kTool;
  suggestion.payload = "";
  suggestion.relevance = 1200;
  results.push_back(std::move(suggestion));

  // Also show related commands if the user typed a query.
  if (!query.empty()) {
    // Add focus mode start command option.
    AstraOmniboxSuggestion start;
    start.display_text = u"Start Focus Session";
    start.description = u"Start a 25-minute focus session";
    start.action_type = AstraOmniboxActionType::kToggleFocusMode;
    start.category = AstraOmniboxActionCategory::kTool;
    start.payload = "on";
    start.relevance = 1050;
    results.push_back(std::move(start));

    AstraOmniboxSuggestion stop;
    stop.display_text = u"Stop Focus Session";
    stop.description = u"End the current focus session";
    stop.action_type = AstraOmniboxActionType::kToggleFocusMode;
    stop.category = AstraOmniboxActionCategory::kTool;
    stop.payload = "off";
    stop.relevance = 1000;
    results.push_back(std::move(stop));
  }

  return results;
}

// =========================================================================
// Screenshot suggestions ("@screenshot" prefix)
// =========================================================================

std::vector<AstraOmniboxSuggestion>
AstraOmniboxProvider::GetScreenshotSuggestions(base::StringPiece16 query) {
  std::vector<AstraOmniboxSuggestion> results;

  // Primary suggestion: visible area screenshot.
  AstraOmniboxSuggestion visible;
  visible.display_text = u"Screenshot Visible";
  visible.description = u"Take a screenshot of the visible area";
  visible.action_type = AstraOmniboxActionType::kScreenshot;
  visible.category = AstraOmniboxActionCategory::kTool;
  visible.payload = "visible";
  visible.relevance = 1200;
  results.push_back(std::move(visible));

  // Additional screenshot modes.
  AstraOmniboxSuggestion region;
  region.display_text = u"Screenshot Region";
  region.description = u"Take a screenshot of a selected region";
  region.action_type = AstraOmniboxActionType::kScreenshot;
  region.category = AstraOmniboxActionCategory::kTool;
  region.payload = "region";
  region.relevance = 1100;
  results.push_back(std::move(region));

  AstraOmniboxSuggestion full;
  full.display_text = u"Screenshot Full Page";
  full.description = u"Take a screenshot of the entire page";
  full.action_type = AstraOmniboxActionType::kScreenshot;
  full.category = AstraOmniboxActionCategory::kTool;
  full.payload = "full";
  full.relevance = 1050;
  results.push_back(std::move(full));

  // Filter by query if one is provided.
  if (!query.empty()) {
    std::vector<AstraOmniboxSuggestion> filtered;
    for (auto& s : results) {
      int score = ComputeRelevance(query, s.display_text);
      if (score >= 0) {
        s.relevance = score + 500;  // Keep high base relevance.
        filtered.push_back(std::move(s));
      }
    }
    results = std::move(filtered);
  }

  return results;
}

}  // namespace astra
