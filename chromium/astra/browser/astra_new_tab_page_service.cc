#include "astra/browser/astra_new_tab_page_service.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

// -- Legacy pref keys (kept for backward compatibility) ----------------------

// TODO(astra): Migrate legacy custom shortcuts to the new managed shortcut
//   system.  For now both systems coexist.

// Pref key for the list of custom shortcuts (legacy index-based).
constexpr char kCustomShortcutsPref[] = "astra.ntp.custom_shortcuts";

// Pref key for the NTP layout mode (legacy, integer, AstraNtpLayoutMode).
constexpr char kLayoutModePref[] = "astra.ntp.layout_mode";

// Pref key for whether recently visited section is visible (legacy, bool).
constexpr char kShowRecentlyVisitedPref[] = "astra.ntp.show_recently_visited";

// Pref key for whether favorites section is visible (legacy, bool).
constexpr char kShowFavoritesPref[] = "astra.ntp.show_favorites";

// Pref key for the background type (legacy, integer, AstraNtpBackgroundType).
constexpr char kBackgroundTypePref[] = "astra.ntp.background_type";

// -- Default values (legacy) -------------------------------------------------

constexpr AstraNtpLayoutMode kDefaultLayoutMode = AstraNtpLayoutMode::kStandard;
constexpr bool kDefaultShowRecentlyVisited = true;
constexpr bool kDefaultShowFavorites = true;
constexpr AstraNtpBackgroundType kDefaultBackgroundType =
    AstraNtpBackgroundType::kDefault;

// Default background color: white (light theme default).
// TODO(astra): Use Chromium's ColorProvider for proper default background
// color that respects the active theme (light/dark).
// Chromium owner: ColorProvider (ui/color/color_provider.h)
constexpr SkColor kDefaultBackgroundColor = SK_ColorWHITE;

// -- Default values (new API) -----------------------------------------------

// Default layout preset.
constexpr AstraNtpLayout kDefaultNtpLayout = AstraNtpLayout::kDefault;

// Default shortcut grid dimensions.
constexpr int kDefaultShortcutColumns = 4;
constexpr int kDefaultShortcutRows = 2;

// Default max workspace cards.
constexpr int kDefaultMaxWorkspaceCards = 6;

// Default max suggestions.
constexpr int kDefaultMaxSuggestions = 8;

// Default show/hide toggles.
constexpr bool kDefaultShowShortcuts = true;
constexpr bool kDefaultShowWorkspaceCards = true;
constexpr bool kDefaultShowSuggestions = true;
constexpr bool kDefaultShowGoogleLogo = true;
constexpr bool kDefaultShowSearchBox = true;
constexpr bool kDefaultShowMostVisited = true;
constexpr bool kDefaultShowRecentlyClosed = false;
constexpr bool kDefaultCustomBackgroundEnabled = false;

// Default dark mode.
constexpr AstraNtpDarkMode kDefaultDarkMode = AstraNtpDarkMode::kAuto;

// Default suggestions enabled.
constexpr bool kDefaultSuggestionsEnabled = true;

// -- Default shortcuts -------------------------------------------------------

// Default shortcut definitions.
// These populate astra-owned default shortcuts appear on a fresh profile.
const struct {
  const char* name;
  const char* url;
  const char* icon_url;  // may be nullptr for default icon
} kDefaultShortcutDefs[] = {
    {"Google", "https://www.google.com", nullptr},
    {"YouTube", "https://www.youtube.com", nullptr},
    {"Gmail", "https://mail.google.com", nullptr},
    {"Google Maps", "https://maps.google.com", nullptr},
    {"Google Drive", "https://drive.google.com", nullptr},
    {"Google Calendar", "https://calendar.google.com", nullptr},
    {"Google Photos", "https://photos.google.com", nullptr},
};

constexpr size_t kDefaultShortcutCount = std::size(kDefaultShortcutDefs);

// -- Placeholder data (legacy) ------------------------------------------

// Number of placeholder shortcuts to generate for UI development.
// TODO(astra): Remove once real TopSites data is wired up.
constexpr size_t kPlaceholderShortcutCount = 8;

// Placeholder shortcut data for UI development.
// TODO(astra): Replace with real data from Chromium's TopSites service.
const struct {
  const char* title;
  const char* url;
} kPlaceholderShortcuts[] = {
    {"Google", "https://www.google.com"},
    {"YouTube", "https://www.youtube.com"},
    {"Gmail", "https://mail.google.com"},
    {"GitHub", "https://github.com"},
    {"Twitter", "https://twitter.com"},
    {"Reddit", "https://www.reddit.com"},
    {"Amazon", "https://www.amazon.com"},
    {"Wikipedia", "https://www.wikipedia.org"},
};

// Number of placeholder recent visits to generate.
constexpr size_t kPlaceholderRecentVisitCount = 5;

// -- Placeholder suggested content -------------------------------------------

// Placeholder suggested content categories.
const char* kSuggestionCategories[] = {
    "technology", "news", "productivity", "entertainment", "science",
};

// Placeholder suggestion sources.
const char* kSuggestionSources[] = {
    "TechCrunch", "The Verge", "Wired", "Ars Technica", "Lifehacker",
};

// -- Color helpers -------------------------------------------------------

// Converts an SkColor to a "#RRGGBB" hex string.
std::string SkColorToHexString(SkColor color) {
  return base::StringPrintf("#%02X%02X%02X", SkColorGetR(color),
                            SkColorGetG(color), SkColorGetB(color));
}

// Parses a "#RRGGBB" or "RRGGBB" hex string to an SkColor.
// Returns the default color if parsing fails.
SkColor HexStringToSkColor(const std::string& hex, SkColor default_color) {
  std::string cleaned = hex;
  if (!cleaned.empty() && cleaned[0] == '#') {
    cleaned = cleaned.substr(1);
  }
  if (cleaned.size() != 6) {
    return default_color;
  }

  uint32_t rgb = 0;
  if (!base::HexStringToUInt(cleaned, &rgb)) {
    return default_color;
  }

  return SkColorSetRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// -- Suggestion generation (placeholder) ----------------------------------------------

// Generates placeholder suggested content items.
// TODO(astra): Replace with real data from Chromium history / most visited.
std::vector<AstraSuggestedContent> GeneratePlaceholderSuggestions() {
  std::vector<AstraSuggestedContent> result;
  result.reserve(12);

  for (int i = 0; i < 12; ++i) {
    AstraSuggestedContent item;
    item.title = "Suggested Article " + base::NumberToString(i + 1);
    item.url = GURL("https://example.com/suggestion/" + base::NumberToString(i + 1));
    item.source = kSuggestionSources[i % std::size(kSuggestionSources)];
    item.thumbnail_url =
        GURL("https://example.com/thumb/" + base::NumberToString(i + 1) + ".jpg");
    item.category = kSuggestionCategories[i % std::size(kSuggestionCategories)];
    // Scores from 0.9 down to ~0.2.
    item.score = 0.9 - (i * 0.06);
    item.is_read = false;
    item.published_time = base::Time::Now() - base::Hours(i * 2);
    result.push_back(std::move(item));
  }

  return result;
}

}  // namespace

// -----------------------------------------------------------------------
// AstraNewTabPageService
// -----------------------------------------------------------------------

AstraNewTabPageService::AstraNewTabPageService(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Subscribe to TopSites / HistoryService changes here
  // once real data integration is in place.
  //
  // Chromium subsystem: TopSites (components/history/core/browser/top_sites.h)
  // Patch point: none needed — use existing TopSites::AddObserver API.
  //
  // The service should cache the most recent data and notify observers
  // when it changes, so the NTP UI can update reactively.
}

AstraNewTabPageService::~AstraNewTabPageService() = default;

void AstraNewTabPageService::Shutdown() {
  // Notify new-style observers of shutdown first.
  NotifyShutdown();

  // KeyedService shutdown: drop profile pointer before the profile goes away.
  profile_ = nullptr;
}

// -- Observer management (new API) ---------------------------------------

void AstraNewTabPageService::AddObserver(AstraNewTabPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraNewTabPageService::RemoveObserver(AstraNewTabPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Observer management (legacy API) ------------------------------------

void AstraNewTabPageService::AddObserver(AstraNewTabPageServiceObserver* observer) {
  legacy_observers_.AddObserver(observer);
}

void AstraNewTabPageService::RemoveObserver(
    AstraNewTabPageServiceObserver* observer) {
  legacy_observers_.RemoveObserver(observer);
}

// -----------------------------------------------------------------------
// Managed shortcuts (new API)
// -----------------------------------------------------------------------

std::vector<AstraShortcut> AstraNewTabPageService::GetShortcuts() const {
  std::vector<AstraShortcut> result;

  if (!profile_) {
    return result;
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();
  result.reserve(shortcuts.size());

  for (const auto& entry : shortcuts) {
    auto shortcut = ShortcutFromDict(entry.GetDict());
    if (shortcut.has_value()) {
      result.push_back(std::move(*shortcut));
    }
  }

  // Sort by position.
  std::sort(result.begin(), result.end(),
            [](const AstraShortcut& a, const AstraShortcut& b) {
              return a.position < b.position;
            });

  return result;
}

size_t AstraNewTabPageService::GetShortcutCount() const {
  if (!profile_) {
    return 0;
  }
  const base::Value::List& shortcuts =
      profile_->GetPrefs()->GetList(kPrefShortcuts);
  return shortcuts.size();
}

std::string AstraNewTabPageService::AddShortcut(const std::string& name,
                                                const GURL& url,
                                                int position) {
  if (!profile_) {
    return std::string();
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();

  AstraShortcut shortcut;
  shortcut.shortcut_id = GenerateShortcutId();
  shortcut.name = name;
  shortcut.url = url;
  shortcut.is_default = false;
  shortcut.created_time = base::Time::Now();
  shortcut.last_used = base::Time();
  shortcut.use_count = 0;

  if (position < 0 || position >= static_cast<int>(shortcuts.size())) {
    // Append at the end.
    shortcut.position = static_cast<int>(shortcuts.size());
  } else {
    // Insert at the specified position and shift others.
    shortcut.position = position;
    // Shift positions of existing shortcuts at or after position.
    for (auto& entry : shortcuts) {
      base::Value::Dict& dict = entry.GetDict();
      int pos = dict.FindInt("position").value_or(0);
      if (pos >= position) {
        dict.Set("position", pos + 1);
      }
    }
  }

  shortcuts.Append(ShortcutToDict(shortcut));
  SaveShortcutsToPrefs(std::move(shortcuts));

  NotifyShortcutAdded(shortcut.shortcut_id);
  // Also notify legacy observers.
  NotifyCustomShortcutsChanged();

  return shortcut.shortcut_id;
}

bool AstraNewTabPageService::RemoveShortcut(const std::string& shortcut_id) {
  if (!profile_) {
    return false;
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();

  int index = FindShortcutIndexById(shortcuts, shortcut_id);
  if (index < 0) {
    return false;
  }

  int removed_position =
      shortcuts[static_cast<size_t>(index)].GetDict().FindInt("position")
          .value_or(0);

  shortcuts.erase(shortcuts.begin() + index);

  // Shift positions of remaining shortcuts that were after the removed one.
  for (auto& entry : shortcuts) {
    base::Value::Dict& dict = entry.GetDict();
    int pos = dict.FindInt("position").value_or(0);
    if (pos > removed_position) {
      dict.Set("position", pos - 1);
    }
  }

  SaveShortcutsToPrefs(std::move(shortcuts));

  NotifyShortcutRemoved(shortcut_id);
  // Also notify legacy observers.
  NotifyCustomShortcutsChanged();

  return true;
}

bool AstraNewTabPageService::UpdateShortcut(const std::string& shortcut_id,
                                            const std::string& name,
                                            const GURL& url) {
  if (!profile_) {
    return false;
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();

  int index = FindShortcutIndexById(shortcuts, shortcut_id);
  if (index < 0) {
    return false;
  }

  base::Value::Dict& dict =
      shortcuts[static_cast<size_t>(index)].GetDict();
  dict.Set("name", name);
  dict.Set("url", url.spec());

  SaveShortcutsToPrefs(std::move(shortcuts));

  NotifyShortcutChanged(shortcut_id);
  // Also notify legacy observers.
  NotifyCustomShortcutsChanged();

  return true;
}

void AstraNewTabPageService::ReorderShortcuts(
    const std::vector<std::string>& shortcut_ids_in_order) {
  if (!profile_) {
    return;
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();
  size_t count = shortcuts.size();

  // Validate: must contain all current IDs exactly once.
  if (shortcut_ids_in_order.size() != count) {
    return;
  }

  // Build a set of current IDs for validation.
  std::set<std::string> current_ids;
  for (const auto& entry : shortcuts) {
    const std::string* id = entry.GetDict().FindString("shortcut_id");
    if (id) {
      current_ids.insert(*id);
    }
  }

  // Check that every ID in the new order exists in the current set and there are no
  duplicates.
  std::set<std::string> seen_ids;
  for (const auto& id : shortcut_ids_in_order) {
    if (current_ids.find(id) == current_ids.end()) {
      return;  // Unknown ID — invalid.
    }
    if (seen_ids.count(id) > 0) {
      return;  // Duplicate — invalid.
    }
    seen_ids.insert(id);
  }

  // Build the reordered list by looking up each ID in the original list.
  base::Value::List reordered;
  reordered.reserve(count);

  for (size_t i = 0; i < shortcut_ids_in_order.size(); ++i) {
    int idx = FindShortcutIndexById(shortcuts, shortcut_ids_in_order[i]);
    if (idx < 0) {
      continue;  // Shouldn't happen since we validated.
    }
    base::Value::Dict new_dict =
        shortcuts[static_cast<size_t>(idx)].GetDict().Clone();
    new_dict.Set("position", static_cast<int>(i));
    reordered.Append(std::move(new_dict));
  }

  SaveShortcutsToPrefs(std::move(reordered));

  NotifyShortcutsReordered();
  // Also notify legacy observers.
  NotifyCustomShortcutsChanged();
}

std::optional<AstraShortcut> AstraNewTabPageService::GetShortcut(
    const std::string& shortcut_id) const {
  if (!profile_) {
    return std::nullopt;
  }

  const base::Value::List& shortcuts =
      profile_->GetPrefs()->GetList(kPrefShortcuts);

  int index = FindShortcutIndexById(shortcuts, shortcut_id);
  if (index < 0) {
    return std::nullopt;
  }

  return ShortcutFromDict(
      shortcuts[static_cast<size_t>(index)].GetDict());
}

bool AstraNewTabPageService::SetShortcutIcon(const std::string& shortcut_id,
                                        const GURL& icon_url) {
  if (!profile_) {
    return false;
  }

  base::Value::List shortcuts = LoadShortcutsFromPrefs();

  int index = FindShortcutIndexById(shortcuts, shortcut_id);
  if (index < 0) {
    return false;
  }

  base::Value::Dict& dict =
      shortcuts[static_cast<size_t>(index)].GetDict();
  dict.Set("icon_url", icon_url.spec());

  SaveShortcutsToPrefs(std::move(shortcuts));

  NotifyShortcutChanged(shortcut_id);

  return true;
}

void AstraNewTabPageService::ResetShortcutsToDefaults() {
  if (!profile_) {
    return;
  }

  std::vector<AstraShortcut> defaults = GetDefaultShortcuts();

  base::Value::List list;
  list.reserve(defaults.size());

  for (size_t i = 0; i < defaults.size(); ++i) {
    defaults[i].position = static_cast<int>(i);
    list.Append(ShortcutToDict(defaults[i]));
  }

  SaveShortcutsToPrefs(std::move(list));

  NotifyShortcutsReordered();
  // Also notify legacy observers.
  NotifyCustomShortcutsChanged();
}

// static
std::vector<AstraShortcut> AstraNewTabPageService::GetDefaultShortcuts() {
  std::vector<AstraShortcut> result;
  result.reserve(kDefaultShortcutCount);

  for (size_t i = 0; i < kDefaultShortcutCount; ++i) {
    AstraShortcut shortcut;
    // Default shortcut IDs are stable and predictable.
    shortcut.shortcut_id =
        std::string("default-") + base::NumberToString(i);
    shortcut.name = kDefaultShortcutDefs[i].name;
    shortcut.url = GURL(kDefaultShortcutDefs[i].url);
    if (kDefaultShortcutDefs[i].icon_url) {
      shortcut.icon_url = GURL(kDefaultShortcutDefs[i].icon_url);
    }
    shortcut.is_default = true;
    shortcut.position = static_cast<int>(i);
    shortcut.created_time = base::Time();  // Epoch for default shortcuts.
    shortcut.last_used = base::Time();
    shortcut.use_count = 0;
    result.push_back(std::move(shortcut));
  }

  return result;
}

bool AstraNewTabPageService::IsDefaultShortcut(
    const std::string& shortcut_id) const {
  auto shortcut = GetShortcut(shortcut_id);
  if (!shortcut.has_value()) {
    return false;
  }
  return shortcut->is_default;
}

// -- Shortcut helpers ----------------------------------------------------

base::Value::List AstraNewTabPageService::LoadShortcutsFromPrefs() const {
  if (!profile_) {
    return {};
  }
  const base::Value::List& list =
      profile_->GetPrefs()->GetList(kPrefShortcuts);
  return list.Clone();
}

void AstraNewTabPageService::SaveShortcutsToPrefs(
    base::Value::List shortcuts) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetList(kPrefShortcuts, std::move(shortcuts));
}

// static
std::optional<AstraShortcut> AstraNewTabPageService::ShortcutFromDict(
    const base::Value::Dict& dict) {
  AstraShortcut shortcut;

  const std::string* id = dict.FindString("shortcut_id");
  if (!id || id->empty()) {
    return std::nullopt;
  }
  shortcut.shortcut_id = *id;

  const std::string* name = dict.FindString("name");
  if (!name) {
    return std::nullopt;
  }
  shortcut.name = *name;

  const std::string* url_str = dict.FindString("url");
  if (!url_str) {
    return std::nullopt;
  }
  shortcut.url = GURL(*url_str);

  const std::string* icon_url_str = dict.FindString("icon_url");
  if (icon_url_str && !icon_url_str->empty()) {
    shortcut.icon_url = GURL(*icon_url_str);
  }

  shortcut.is_default = dict.FindBool("is_default").value_or(false);
  shortcut.position = dict.FindInt("position").value_or(0);

  const std::string* ws_id = dict.FindString("astra_workspace_id");
  if (ws_id) {
    shortcut.astra_workspace_id = *ws_id;
  }

  double created_time = dict.FindDouble("created_time").value_or(0);
  if (created_time != 0) {
    shortcut.created_time = base::Time::FromDoubleT(created_time);
  }

  double last_used = dict.FindDouble("last_used").value_or(0);
  if (last_used != 0) {
    shortcut.last_used = base::Time::FromDoubleT(last_used);
  }

  shortcut.use_count = dict.FindInt("use_count").value_or(0);

  return shortcut;
}

// static
base::Value::Dict AstraNewTabPageService::ShortcutToDict(
    const AstraShortcut& shortcut) {
  base::Value::Dict dict;
  dict.Set("shortcut_id", shortcut.shortcut_id);
  dict.Set("name", shortcut.name);
  dict.Set("url", shortcut.url.spec());
  if (shortcut.icon_url.is_valid()) {
    dict.Set("icon_url", shortcut.icon_url.spec());
  }
  dict.Set("is_default", shortcut.is_default);
  dict.Set("position", shortcut.position);
  if (!shortcut.astra_workspace_id.empty()) {
    dict.Set("astra_workspace_id", shortcut.astra_workspace_id);
  }
  dict.Set("created_time", shortcut.created_time.ToDoubleT());
  dict.Set("last_used", shortcut.last_used.ToDoubleT());
  dict.Set("use_count", shortcut.use_count);
  return dict;
}

// static
std::string AstraNewTabPageService::GenerateShortcutId() {
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

// static
int AstraNewTabPageService::FindShortcutIndexById(
    const base::Value::List& shortcuts,
    const std::string& shortcut_id) {
  for (size_t i = 0; i < shortcuts.size(); ++i) {
    const std::string* id = shortcuts[i].GetDict().FindString("shortcut_id");
    if (id && *id == shortcut_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// -----------------------------------------------------------------------
// Workspace cards on NTP
// -----------------------------------------------------------------------

std::vector<AstraNtpWorkspaceSummary>
AstraNewTabPageService::GetWorkspaceCards() const {
  if (!profile_) {
    return {};
  }

  // Get base workspace data from the workspace service.
  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!workspace_service) {
    // If workspace service is available, use real data.
    const auto& workspaces = workspace_service->workspaces();
    const std::string& active_id = workspace_service->active_workspace_id();

    std::vector<AstraNtpWorkspaceSummary> result;

    // Apply visibility filter and ordering.
    int max_cards = GetMaxWorkspaceCards();
    int count = 0;

    // First, add workspaces in the custom order that are visible.
    const base::Value::List& order_list =
        profile_->GetPrefs()->GetList(kPrefWorkspaceCardOrder);

    for (const auto& entry : order_list) {
      if (count >= max_cards) {
        break;
      }
      const std::string* ws_id = entry.GetIfString();
      if (!ws_id) {
        continue;
      }
      if (!IsWorkspaceVisible(*ws_id)) {
        continue;
      }
      // Find this workspace in the service.
      for (const auto& ws : workspaces) {
        if (ws.id == *ws_id) {
          AstraNtpWorkspaceSummary summary;
          summary.id = ws.id;
          summary.name = ws.name;
          summary.accent_color = ws.accent_color;
          summary.is_active = (ws.id == active_id);
          // TODO(astra): Compute real tab count.
          summary.tab_count = 0;
          result.push_back(std::move(summary));
          ++count;
          break;
        }
      }
    }

    // Then add any remaining visible workspaces not in the custom order.
    for (const auto& ws : workspaces) {
      if (count >= max_cards) {
        break;
      }
      // Skip if already added.
      bool already_added = false;
      for (const auto& added : result) {
        if (added.id == ws.id) {
          already_added = true;
          break;
        }
      }
      if (already_added) {
        continue;
      }
      if (!IsWorkspaceVisible(ws.id)) {
        continue;
      }
      AstraNtpWorkspaceSummary summary;
      summary.id = ws.id;
      summary.name = ws.name;
      summary.accent_color = ws.accent_color;
      summary.is_active = (ws.id == active_id);
      summary.tab_count = 0;
      result.push_back(std::move(summary));
      ++count;
    }

    return result;
  }

  // Fallback: return placeholder workspace summaries without workspace service.
  std::vector<AstraNtpWorkspaceSummary> result;
  int max_cards = GetMaxWorkspaceCards();

  // Generate placeholder workspaces for UI development.
  // TODO(astra): Remove placeholder once real workspace data is available.
  const struct {
    const char* id;
    const char* name;
    const char* color;
  } kPlaceholderWorkspaces[] = {
        {"ws-1", "Work", "#5B8FF9"},
        {"ws-2", "Personal", "#5AD8A6"},
        {"ws-3", "Research", "#F6BD16"},
        {"ws-4", "Entertainment", "#E86452"},
    };

  int count = 0;
  for (const auto& ws : kPlaceholderWorkspaces) {
    if (count >= max_cards) {
      break;
    }
    if (!IsWorkspaceVisible(ws.id)) {
      continue;
    }
    AstraNtpWorkspaceSummary summary;
    summary.id = ws.id;
    summary.name = ws.name;
    summary.accent_color = ws.color;
    summary.tab_count = 0;
    summary.is_active = (count == 0);
    result.push_back(std::move(summary));
    ++count;
  }

  return result;
}

size_t AstraNewTabPageService::GetVisibleWorkspacesCount() const {
  return GetWorkspaceCards().size();
}

void AstraNewTabPageService::ShowWorkspace(const std::string& workspace_id,
                                           bool show) {
  if (!profile_) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  base::Value::Dict visibility =
      prefs->GetDict(kPrefWorkspaceCardVisibility).Clone();

  visibility.Set(workspace_id, show);

  prefs->SetDict(kPrefWorkspaceCardVisibility, std::move(visibility));

  NotifyWorkspaceCardVisibilityChanged(workspace_id, show);
}

bool AstraNewTabPageService::IsWorkspaceVisible(
    const std::string& workspace_id) const {
  if (!profile_) {
    return true;  // Default to visible if no profile.
  }

  const base::Value::Dict& visibility =
      profile_->GetPrefs()->GetDict(kPrefWorkspaceCardVisibility);

  std::optional<bool> visible = visibility.FindBool(workspace_id);
  if (!visible.has_value()) {
    // Default: workspaces are visible.
    return true;
  }
  return *visible;
}

void AstraNewTabPageService::SetMaxWorkspaceCards(int max) {
  if (!profile_) {
    return;
  }

  // Clamp to non-negative.
  int clamped = std::max(0, max);
  profile_->GetPrefs()->SetInteger(kPrefMaxWorkspaceCards, clamped);

  NotifyNtpThemeChanged();
  // Also notify legacy layout observers.
  NotifyLayoutChanged();
}

int AstraNewTabPageService::GetMaxWorkspaceCards() const {
  if (!profile_) {
    return kDefaultMaxWorkspaceCards;
  }
  return profile_->GetPrefs()->GetInteger(kPrefMaxWorkspaceCards);
}

void AstraNewTabPageService::ReorderWorkspaceCards(
    const std::vector<std::string>& workspace_ids) {
  if (!profile_) {
    return;
  }

  base::Value::List order_list;
  for (const auto& id : workspace_ids) {
    order_list.Append(id);
  }

  profile_->GetPrefs()->SetList(kPrefWorkspaceCardOrder,
                                 std::move(order_list));

  NotifyNtpThemeChanged();
  // Also notify legacy layout observers.
  NotifyLayoutChanged();
}

// -----------------------------------------------------------------------
// Suggested content
// -----------------------------------------------------------------------

std::vector<AstraSuggestedContent> AstraNewTabPageService::GetSuggestedContent(
    int max_count) const {
  std::vector<AstraSuggestedContent> result;

  if (!profile_ || !IsSuggestionsEnabled() || max_count <= 0) {
    return result;
  }

  // Get the dismissed URLs from prefs.
  const base::Value::List& dismissed_list =
      profile_->GetPrefs()->GetList(kPrefDismissedSuggestions);

  std::set<std::string> dismissed_urls;
  for (const auto& entry : dismissed_list) {
    const std::string* url = entry.GetIfString();
    if (url) {
      dismissed_urls.insert(*url);
    }
  }

  // Generate placeholder suggestions.
  // TODO(astra): Replace with real data from Chromium history / most visited.
  std::vector<AstraSuggestedContent> all = GeneratePlaceholderSuggestions();

  // Filter out dismissed ones and limit to max_count.
  for (auto& item : all) {
    if (static_cast<int>(result.size()) >= max_count) {
      break;
    }
    if (dismissed_urls.count(item.url.spec()) > 0) {
      continue;
    }
    result.push_back(std::move(item));
  }

  return result;
}

void AstraNewTabPageService::DismissSuggestion(const GURL& url) {
  if (!profile_) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  base::Value::List dismissed =
      prefs->GetList(kPrefDismissedSuggestions).Clone();

  // Check if already dismissed.
  bool already_dismissed = false;
  for (const auto& entry : dismissed) {
    const std::string* s = entry.GetIfString();
    if (s && *s == url.spec()) {
      already_dismissed = true;
      break;
    }
  }

  if (!already_dismissed) {
    dismissed.Append(url.spec());
    prefs->SetList(kPrefDismissedSuggestions, std::move(dismissed));
  }

  NotifySuggestionsChanged();
}

void AstraNewTabPageService::RestoreDismissedSuggestions() {
  if (!profile_) {
    return;
  }

  profile_->GetPrefs()->SetList(kPrefDismissedSuggestions,
                                 base::Value::List());

  NotifySuggestionsChanged();
}

size_t AstraNewTabPageService::GetDismissedSuggestionCount() const {
  if (!profile_) {
    return 0;
  }
  const base::Value::List& dismissed =
      profile_->GetPrefs()->GetList(kPrefDismissedSuggestions);
  return dismissed.size();
}

void AstraNewTabPageService::RefreshSuggestions() {
  // In the stub implementation, just notify observers that suggestions have changed.
  // TODO(astra): Trigger real refresh from HistoryService / TopSites.
  NotifySuggestionsChanged();
}

bool AstraNewTabPageService::IsSuggestionsEnabled() const {
  if (!profile_) {
    return kDefaultSuggestionsEnabled;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefSuggestionsEnabled);
}

// -----------------------------------------------------------------------
// Layout & theme settings
// -----------------------------------------------------------------------

AstraNtpTheme AstraNewTabPageService::GetNtpTheme() const {
  AstraNtpTheme theme;

  theme.layout = layout();
  theme.background_color = background_color();
  theme.background_image_url = background_image_url();
  theme.show_shortcuts = show_shortcuts();
  theme.show_workspace_cards = show_workspace_cards();
  theme.show_suggestions = show_suggestions();
  theme.show_google_logo = show_google_logo();
  theme.show_search_box = show_search_box();
  theme.shortcut_columns = shortcut_columns();
  theme.shortcut_rows = shortcut_rows();

  return theme;
}

void AstraNewTabPageService::SetNtpTheme(const AstraNtpTheme& theme) {
  if (!profile_) {
    return;
  }

  // Set all theme fields individually so each pref is properly validated.
  set_layout(theme.layout);
  set_background_color(theme.background_color);
  set_background_image_url(theme.background_image_url);
  set_show_shortcuts(theme.show_shortcuts);
  set_show_workspace_cards(theme.show_workspace_cards);
  set_show_suggestions(theme.show_suggestions);
  set_show_google_logo(theme.show_google_logo);
  set_show_search_box(theme.show_search_box);
  set_shortcut_columns(theme.shortcut_columns);
  set_shortcut_rows(theme.shortcut_rows);

  // Send a single consolidated notification.
  NotifyNtpThemeChanged();
  // Also notify legacy observers.
  NotifyLayoutChanged();
  NotifyBackgroundChanged();
}

void AstraNewTabPageService::ResetNtpTheme() {
  if (!profile_) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetInteger(kPrefNtpLayout, static_cast<int>(kDefaultNtpLayout));
  prefs->SetString(kPrefBackgroundColor,
                   SkColorToHexString(kDefaultBackgroundColor));
  prefs->SetString(kPrefBackgroundImageUrl, std::string());
  prefs->SetBoolean(kPrefShowShortcuts, kDefaultShowShortcuts);
  prefs->SetBoolean(kPrefShowWorkspaceCards, kDefaultShowWorkspaceCards);
  prefs->SetBoolean(kPrefShowSuggestions, kDefaultShowSuggestions);
  prefs->SetBoolean(kPrefShowGoogleLogo, kDefaultShowGoogleLogo);
  prefs->SetBoolean(kPrefShowSearchBox, kDefaultShowSearchBox);
  prefs->SetInteger(kPrefShortcutColumns, kDefaultShortcutColumns);
  prefs->SetInteger(kPrefShortcutRows, kDefaultShortcutRows);

  NotifyNtpThemeChanged();
  // Also notify legacy observers.
  NotifyLayoutChanged();
  NotifyBackgroundChanged();
}

// -- Individual theme getters/setters -----------------------------------

AstraNtpLayout AstraNewTabPageService::layout() const {
  if (!profile_) {
    return kDefaultNtpLayout;
  }
  int value = profile_->GetPrefs()->GetInteger(kPrefNtpLayout);
  if (value < 0 || value > static_cast<int>(AstraNtpLayout::kCustom)) {
    return kDefaultNtpLayout;
  }
  return static_cast<AstraNtpLayout>(value);
}

void AstraNewTabPageService::set_layout(AstraNtpLayout layout) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetInteger(kPrefNtpLayout,
                                    static_cast<int>(layout));
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

SkColor AstraNewTabPageService::background_color() const {
  if (!profile_) {
    return kDefaultBackgroundColor;
  }
  const std::string& hex =
      profile_->GetPrefs()->GetString(kPrefBackgroundColor);
  if (hex.empty()) {
    return kDefaultBackgroundColor;
  }
  return HexStringToSkColor(hex, kDefaultBackgroundColor);
}

void AstraNewTabPageService::set_background_color(SkColor color) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetString(kPrefBackgroundColor,
                                  SkColorToHexString(color));
  NotifyNtpThemeChanged();
  NotifyBackgroundChanged();
}

std::string AstraNewTabPageService::background_image_url() const {
  if (!profile_) {
    return std::string();
  }
  return profile_->GetPrefs()->GetString(kPrefBackgroundImageUrl);
}

void AstraNewTabPageService::set_background_image_url(
    const std::string& url) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetString(kPrefBackgroundImageUrl, url);
  NotifyNtpThemeChanged();
  NotifyBackgroundChanged();
}

bool AstraNewTabPageService::show_shortcuts() const {
  if (!profile_) {
    return kDefaultShowShortcuts;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowShortcuts);
}

void AstraNewTabPageService::set_show_shortcuts(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowShortcuts, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_workspace_cards() const {
  if (!profile_) {
    return kDefaultShowWorkspaceCards;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowWorkspaceCards);
}

void AstraNewTabPageService::set_show_workspace_cards(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowWorkspaceCards, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_suggestions() const {
  if (!profile_) {
    return kDefaultShowSuggestions;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowSuggestions);
}

void AstraNewTabPageService::set_show_suggestions(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowSuggestions, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_google_logo() const {
  if (!profile_) {
    return kDefaultShowGoogleLogo;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowGoogleLogo);
}

void AstraNewTabPageService::set_show_google_logo(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowGoogleLogo, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_search_box() const {
  if (!profile_) {
    return kDefaultShowSearchBox;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowSearchBox);
}

void AstraNewTabPageService::set_show_search_box(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowSearchBox, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

int AstraNewTabPageService::shortcut_columns() const {
  if (!profile_) {
    return kDefaultShortcutColumns;
  }
  return profile_->GetPrefs()->GetInteger(kPrefShortcutColumns);
}

void AstraNewTabPageService::set_shortcut_columns(int columns) {
  if (!profile_) {
    return;
  }
  // Clamp to reasonable range: 1-10 columns.
  int clamped = std::clamp(columns, 1, 10);
  profile_->GetPrefs()->SetInteger(kPrefShortcutColumns, clamped);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

int AstraNewTabPageService::shortcut_rows() const {
  if (!profile_) {
    return kDefaultShortcutRows;
  }
  return profile_->GetPrefs()->GetInteger(kPrefShortcutRows);
}

void AstraNewTabPageService::set_shortcut_rows(int rows) {
  if (!profile_) {
    return;
  }
  // Clamp to reasonable range: 1-10 rows.
  int clamped = std::clamp(rows, 1, 10);
  profile_->GetPrefs()->SetInteger(kPrefShortcutRows, clamped);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

// -- Additional NTP settings ---------------------------------------------

int AstraNewTabPageService::max_suggestions() const {
  if (!profile_) {
    return kDefaultMaxSuggestions;
  }
  return profile_->GetPrefs()->GetInteger(kPrefMaxSuggestions);
}

void AstraNewTabPageService::set_max_suggestions(int max) {
  if (!profile_) {
    return;
  }
  int clamped = std::max(0, max);
  profile_->GetPrefs()->SetInteger(kPrefMaxSuggestions, clamped);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_most_visited() const {
  if (!profile_) {
    return kDefaultShowMostVisited;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowMostVisited);
}

void AstraNewTabPageService::set_show_most_visited(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowMostVisited, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_recently_closed() const {
  if (!profile_) {
    return kDefaultShowRecentlyClosed;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowRecentlyClosed);
}

void AstraNewTabPageService::set_show_recently_closed(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowRecentlyClosed, show);
  NotifyNtpThemeChanged();
  NotifyLayoutChanged();
}

AstraNtpDarkMode AstraNewTabPageService::dark_mode() const {
  if (!profile_) {
    return kDefaultDarkMode;
  }
  int value = profile_->GetPrefs()->GetInteger(kPrefDarkMode);
  if (value < 0 || value > static_cast<int>(AstraNtpDarkMode::kDark)) {
    return kDefaultDarkMode;
  }
  return static_cast<AstraNtpDarkMode>(value);
}

void AstraNewTabPageService::set_dark_mode(AstraNtpDarkMode mode) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetInteger(kPrefDarkMode,
                                    static_cast<int>(mode));
  NotifyNtpThemeChanged();
  NotifyBackgroundChanged();
}

bool AstraNewTabPageService::custom_background_enabled() const {
  if (!profile_) {
    return kDefaultCustomBackgroundEnabled;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefCustomBackgroundEnabled);
}

void AstraNewTabPageService::set_custom_background_enabled(bool enabled) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefCustomBackgroundEnabled, enabled);
  NotifyNtpThemeChanged();
  NotifyBackgroundChanged();
}

// -----------------------------------------------------------------------
// Legacy API implementations
// -----------------------------------------------------------------------

// -- Shortcut data (legacy) ----------------------------------------------

std::vector<AstraNtpShortcut> AstraNewTabPageService::GetTopSites(
    size_t count) const {
  // TODO(astra): Return real data from Chromium's TopSites service.
  // The TopSites service provides a list of most-visited URLs with titles
  // and thumbnail data.
  //
  // Chromium owner: TopSites (components/history/core/browser/top_sites.h)
  //
  // For now, return placeholder data.
  std::vector<AstraNtpShortcut> result;
  size_t actual_count =
      std::min(count, static_cast<size_t>(
                          std::size(kPlaceholderShortcuts)));
  actual_count = std::min(actual_count, kPlaceholderShortcutCount);

  for (size_t i = 0; i < actual_count; ++i) {
    AstraNtpShortcut shortcut;
    shortcut.title = base::UTF8ToUTF16(kPlaceholderShortcuts[i].title);
    shortcut.url = GURL(kPlaceholderShortcuts[i].url);
    shortcut.is_most_visited = true;
    result.push_back(std::move(shortcut));
  }

  return result;
}

std::vector<AstraNtpRecentVisit> AstraNewTabPageService::GetRecentlyVisited(
    size_t count) const {
  // TODO(astra): Return real data from Chromium's HistoryService.
  // For now, return placeholder data.
  std::vector<AstraNtpRecentVisit> result;
  size_t actual_count = std::min(count, kPlaceholderRecentVisitCount);

  for (size_t i = 0; i < actual_count; ++i) {
    AstraNtpRecentVisit visit;
    visit.title = base::UTF8ToUTF16(
        std::string("Recent Page ") + base::NumberToString(i + 1));
    visit.url = GURL("https://example.com/recent/" +
                     base::NumberToString(i + 1));
    visit.visit_time = base::Time::Now() - base::Minutes(i * 30);
    result.push_back(std::move(visit));
  }

  return result;
}

// -- Workspace summary (legacy) ----------------------------------------

std::vector<AstraNtpWorkspaceSummary>
AstraNewTabPageService::GetWorkspaceSummaries() const {
  if (!profile_) {
    return {};
  }

  AstraWorkspaceService* workspace_service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!workspace_service) {
    return {};
  }

  const auto& workspaces = workspace_service->workspaces();
  const std::string& active_id = workspace_service->active_workspace_id();

  std::vector<AstraNtpWorkspaceSummary> result;
  result.reserve(workspaces.size());

  for (const auto& ws : workspaces) {
    AstraNtpWorkspaceSummary summary;
    summary.id = ws.id;
    summary.name = ws.name;
    summary.accent_color = ws.accent_color;
    summary.is_active = (ws.id == active_id);
    // TODO(astra): Compute real tab count per workspace.
    summary.tab_count = 0;
    result.push_back(std::move(summary));
  }

  return result;
}

// -- Favorite shortcuts (legacy) -----------------------------------------

std::vector<AstraNtpShortcut> AstraNewTabPageService::GetFavoriteShortcuts(
    size_t /*count*/) const {
  // TODO(astra): Build favorite shortcuts from AstraFavoriteService.
  return {};
}

// -- Custom shortcuts (legacy index-based API) ---------------------------

size_t AstraNewTabPageService::AddCustomShortcut(const std::u16string& title,
                                                 const GURL& url) {
  if (!profile_) {
    return 0;
  }

  PrefService* prefs = profile_->GetPrefs();
  base::Value::List shortcuts = LoadCustomShortcutsFromPrefs();

  size_t new_index = shortcuts.size();

  base::Value::Dict shortcut_dict;
  shortcut_dict.Set("title", base::UTF16ToUTF8(title));
  shortcut_dict.Set("url", url.spec());
  shortcut_dict.Set("index", static_cast<int>(new_index));

  shortcuts.Append(std::move(shortcut_dict));

  SaveCustomShortcutsToPrefs(std::move(shortcuts));
  NotifyCustomShortcutsChanged();

  return new_index;
}

bool AstraNewTabPageService::RemoveCustomShortcut(size_t index) {
  if (!profile_) {
    return false;
  }

  base::Value::List shortcuts = LoadCustomShortcutsFromPrefs();

  if (index >= shortcuts.size()) {
    return false;
  }

  shortcuts.erase(shortcuts.begin() + static_cast<ptrdiff_t>(index));

  // Re-index remaining shortcuts.
  for (size_t i = 0; i < shortcuts.size(); ++i) {
    shortcuts[i].GetDict().Set("index", static_cast<int>(i));
  }

  SaveCustomShortcutsToPrefs(std::move(shortcuts));
  NotifyCustomShortcutsChanged();

  return true;
}

bool AstraNewTabPageService::UpdateCustomShortcut(size_t index,
                                                  const std::u16string& title,
                                                  const GURL& url) {
  if (!profile_) {
    return false;
  }

  base::Value::List shortcuts = LoadCustomShortcutsFromPrefs();

  if (index >= shortcuts.size()) {
    return false;
  }

  base::Value::Dict& shortcut_dict =
      shortcuts[static_cast<ptrdiff_t>(index)].GetDict();
  shortcut_dict.Set("title", base::UTF16ToUTF8(title));
  shortcut_dict.Set("url", url.spec());

  SaveCustomShortcutsToPrefs(std::move(shortcuts));
  NotifyCustomShortcutsChanged();

  return true;
}

void AstraNewTabPageService::ReorderCustomShortcuts(
    const std::vector<size_t>& ordered_indices) {
  if (!profile_) {
    return;
  }

  base::Value::List shortcuts = LoadCustomShortcutsFromPrefs();
  size_t count = shortcuts.size();

  // Validate: ordered_indices must be a permutation of [0, count).
  if (ordered_indices.size() != count) {
    return;
  }

  std::vector<bool> seen(count, false);
  for (size_t idx : ordered_indices) {
    if (idx >= count || seen[idx]) {
      return;  // Invalid index or duplicate — bail out.
    }
    seen[idx] = true;
  }

  // Build the reordered list.
  base::Value::List reordered;
  reordered.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    base::Value::Dict new_dict =
        shortcuts[static_cast<ptrdiff_t>(ordered_indices[i])].GetDict().Clone();
    new_dict.Set("index", static_cast<int>(i));
    reordered.Append(std::move(new_dict));
  }

  SaveCustomShortcutsToPrefs(std::move(reordered));
  NotifyCustomShortcutsChanged();
}

size_t AstraNewTabPageService::GetCustomShortcutCount() const {
  if (!profile_) {
    return 0;
  }

  const base::Value::List& shortcuts =
      profile_->GetPrefs()->GetList(kCustomShortcutsPref);
  return shortcuts.size();
}

std::vector<AstraNtpShortcut> AstraNewTabPageService::GetCustomShortcuts(
    size_t count) const {
  std::vector<AstraNtpShortcut> result;

  if (!profile_) {
    return result;
  }

  const base::Value::List& shortcuts =
      profile_->GetPrefs()->GetList(kCustomShortcutsPref);

  size_t actual_count = std::min(count, shortcuts.size());
  result.reserve(actual_count);

  for (size_t i = 0; i < actual_count; ++i) {
    const base::Value::Dict& dict =
        shortcuts[static_cast<ptrdiff_t>(i)].GetDict();
    AstraNtpShortcut shortcut;
    shortcut.title = base::UTF8ToUTF16(*dict.FindString("title"));
    shortcut.url = GURL(*dict.FindString("url"));
    shortcut.is_most_visited = false;
    result.push_back(std::move(shortcut));
  }

  return result;
}

std::vector<AstraNtpShortcut> AstraNewTabPageService::GetAllShortcuts(
    size_t count) const {
  // Get custom shortcuts first.
  std::vector<AstraNtpShortcut> custom = GetCustomShortcuts(count);

  if (custom.size() >= count) {
    return custom;
  }

  // Fill the rest with most-visited shortcuts.
  size_t remaining = count - custom.size();
  std::vector<AstraNtpShortcut> top_sites = GetTopSites(remaining);

  // Concatenate.
  std::vector<AstraNtpShortcut> result;
  result.reserve(custom.size() + top_sites.size());
  for (auto& s : custom) {
    result.push_back(std::move(s));
  }
  for (auto& s : top_sites) {
    result.push_back(std::move(s));
  }

  return result;
}

// -- Legacy custom shortcut pref helpers ----------------------------------

base::Value::List AstraNewTabPageService::LoadCustomShortcutsFromPrefs() const {
  if (!profile_) {
    return {};
  }
  const base::Value::List& list =
      profile_->GetPrefs()->GetList(kCustomShortcutsPref);
  return list.Clone();
}

void AstraNewTabPageService::SaveCustomShortcutsToPrefs(
    base::Value::List shortcuts) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetList(kCustomShortcutsPref, std::move(shortcuts));
}

// -- Legacy notification helpers ------------------------------------------

void AstraNewTabPageService::NotifyCustomShortcutsChanged() {
  for (auto& observer : legacy_observers_) {
    observer.OnCustomShortcutsChanged();
  }
}

void AstraNewTabPageService::NotifyLayoutChanged() {
  for (auto& observer : legacy_observers_) {
    observer.OnLayoutChanged();
  }
}

void AstraNewTabPageService::NotifyBackgroundChanged() {
  for (auto& observer : legacy_observers_) {
    observer.OnBackgroundChanged();
  }
}

// -- New notification helpers ---------------------------------------------------

void AstraNewTabPageService::NotifyNtpThemeChanged() {
  for (auto& observer : observers_) {
    observer.OnNtpThemeChanged(this);
  }
}

void AstraNewTabPageService::NotifyShortcutAdded(
    const std::string& shortcut_id) {
  for (auto& observer : observers_) {
    observer.OnShortcutAdded(this, shortcut_id);
  }
}

void AstraNewTabPageService::NotifyShortcutRemoved(
    const std::string& shortcut_id) {
  for (auto& observer : observers_) {
    observer.OnShortcutRemoved(this, shortcut_id);
  }
}

void AstraNewTabPageService::NotifyShortcutChanged(
    const std::string& shortcut_id) {
  for (auto& observer : observers_) {
    observer.OnShortcutChanged(this, shortcut_id);
  }
}

void AstraNewTabPageService::NotifyShortcutsReordered() {
  for (auto& observer : observers_) {
    observer.OnShortcutsReordered(this);
  }
}

void AstraNewTabPageService::NotifyWorkspaceCardVisibilityChanged(
    const std::string& workspace_id,
    bool visible) {
  for (auto& observer : observers_) {
    observer.OnWorkspaceCardVisibilityChanged(this, workspace_id, visible);
  }
}

void AstraNewTabPageService::NotifySuggestionsChanged() {
  for (auto& observer : observers_) {
    observer.OnSuggestionsChanged(this);
  }
}

void AstraNewTabPageService::NotifyShutdown() {
  for (auto& observer : observers_) {
    observer.OnNewTabPageServiceShutdown(this);
  }
}

// -- Layout options (legacy) -------------------------------------------

AstraNtpLayoutMode AstraNewTabPageService::layout_mode() const {
  if (!profile_) {
    return kDefaultLayoutMode;
  }
  int value = profile_->GetPrefs()->GetInteger(kLayoutModePref);
  if (value < 0 || value > static_cast<int>(AstraNtpLayoutMode::kFocused)) {
    return kDefaultLayoutMode;
  }
  return static_cast<AstraNtpLayoutMode>(value);
}

void AstraNewTabPageService::set_layout_mode(AstraNtpLayoutMode mode) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetInteger(kLayoutModePref, static_cast<int>(mode));
  NotifyLayoutChanged();
}

std::string AstraNewTabPageService::GetLayoutModeName() const {
  switch (layout_mode()) {
    case AstraNtpLayoutMode::kStandard:
      return "standard";
    case AstraNtpLayoutMode::kCompact:
      return "compact";
    case AstraNtpLayoutMode::kFocused:
      return "focused";
  }
  return "standard";
}

bool AstraNewTabPageService::show_recently_visited() const {
  if (!profile_) {
    return kDefaultShowRecentlyVisited;
  }
  return profile_->GetPrefs()->GetBoolean(kShowRecentlyVisitedPref);
}

void AstraNewTabPageService::set_show_recently_visited(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kShowRecentlyVisitedPref, show);
  NotifyLayoutChanged();
}

bool AstraNewTabPageService::show_favorites() const {
  if (!profile_) {
    return kDefaultShowFavorites;
  }
  return profile_->GetPrefs()->GetBoolean(kShowFavoritesPref);
}

void AstraNewTabPageService::set_show_favorites(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kShowFavoritesPref, show);
  NotifyLayoutChanged();
}

// -- Background / theme (legacy) ----------------------------------------

AstraNtpBackgroundType AstraNewTabPageService::background_type() const {
  if (!profile_) {
    return kDefaultBackgroundType;
  }
  int value = profile_->GetPrefs()->GetInteger(kBackgroundTypePref);
  if (value < 0 ||
      value > static_cast<int>(AstraNtpBackgroundType::kCustomImage)) {
    return kDefaultBackgroundType;
  }
  return static_cast<AstraNtpBackgroundType>(value);
}

void AstraNewTabPageService::set_background_type(
    AstraNtpBackgroundType type) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetInteger(kBackgroundTypePref,
                                   static_cast<int>(type));
  NotifyBackgroundChanged();
}

SkColor AstraNewTabPageService::GetBackgroundColor() const {
  if (!profile_) {
    return kDefaultBackgroundColor;
  }
  const std::string& hex =
      profile_->GetPrefs()->GetString(kPrefBackgroundColor);
  if (hex.empty()) {
    return kDefaultBackgroundColor;
  }
  return HexStringToSkColor(hex, kDefaultBackgroundColor);
}

void AstraNewTabPageService::SetBackgroundColor(SkColor color) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetString(kPrefBackgroundColor,
                                  SkColorToHexString(color));
  NotifyBackgroundChanged();
}

// -----------------------------------------------------------------------
// AstraNewTabPageServiceFactory
// -----------------------------------------------------------------------

// static
AstraNewTabPageService*
AstraNewTabPageServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<AstraNewTabPageService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraNewTabPageServiceFactory*
AstraNewTabPageServiceFactory::GetInstance() {
  static base::NoDestructor<AstraNewTabPageServiceFactory> instance;
  return instance.get();
}

// static
void AstraNewTabPageServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // -- Managed shortcuts --
  registry->RegisterListPref(AstraNewTabPageService::kPrefShortcuts);

  // -- Layout & theme settings --
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefNtpLayout,
      static_cast<int>(kDefaultNtpLayout));
  registry->RegisterStringPref(
      AstraNewTabPageService::kPrefBackgroundColor,
      SkColorToHexString(kDefaultBackgroundColor));
  registry->RegisterStringPref(
      AstraNewTabPageService::kPrefBackgroundImageUrl,
      std::string());
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowShortcuts,
      kDefaultShowShortcuts);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowWorkspaceCards,
      kDefaultShowWorkspaceCards);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowSuggestions,
      kDefaultShowSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowGoogleLogo,
      kDefaultShowGoogleLogo);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowSearchBox,
      kDefaultShowSearchBox);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefShortcutColumns,
      kDefaultShortcutColumns);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefShortcutRows,
      kDefaultShortcutRows);

  // -- Additional NTP settings --
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefMaxWorkspaceCards,
      kDefaultMaxWorkspaceCards);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefMaxSuggestions,
      kDefaultMaxSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowMostVisited,
      kDefaultShowMostVisited);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefShowRecentlyClosed,
      kDefaultShowRecentlyClosed);
  registry->RegisterIntegerPref(
      AstraNewTabPageService::kPrefDarkMode,
      static_cast<int>(kDefaultDarkMode));
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefCustomBackgroundEnabled,
      kDefaultCustomBackgroundEnabled);

  // -- Workspace card visibility & order --
  registry->RegisterDictionaryPref(
      AstraNewTabPageService::kPrefWorkspaceCardVisibility);
  registry->RegisterListPref(
      AstraNewTabPageService::kPrefWorkspaceCardOrder);

  // -- Suggested content --
  registry->RegisterListPref(
      AstraNewTabPageService::kPrefDismissedSuggestions);
  registry->RegisterBooleanPref(
      AstraNewTabPageService::kPrefSuggestionsEnabled,
      kDefaultSuggestionsEnabled);

  // -- Legacy prefs --
  registry->RegisterListPref(kCustomShortcutsPref);
  registry->RegisterIntegerPref(kLayoutModePref,
                                 static_cast<int>(kDefaultLayoutMode));
  registry->RegisterBooleanPref(kShowRecentlyVisitedPref,
                              kDefaultShowRecentlyVisited);
  registry->RegisterBooleanPref(kShowFavoritesPref, kDefaultShowFavorites);
  registry->RegisterIntegerPref(kBackgroundTypePref,
                                 static_cast<int>(kDefaultBackgroundType));
  // Note: kPrefBackgroundColor is registered above (shared by old and new API).
}

AstraNewTabPageServiceFactory::AstraNewTabPageServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraNewTabPageService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito uses kRedirectedToOriginal because NTP data
              // (shortcuts, workspaces, theme) is product-level state
              // that should reflect the user's main profile.  An incognito
              // new tab page still shows the same shortcuts and workspaces —
              // only the browsing session is isolated.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions have no original profile to redirect to,
              // so they get their own ephemeral NTP service instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user-visible NTP.
              .Build()) {}

AstraNewTabPageServiceFactory::~AstraNewTabPageServiceFactory() = default;

std::unique_ptr<KeyedService>
AstraNewTabPageServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<AstraNewTabPageService>(
      Profile::FromBrowserContext(context));
}

}  // namespace astra
