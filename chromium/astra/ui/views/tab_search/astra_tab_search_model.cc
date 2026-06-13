#include "astra/ui/views/tab_search/astra_tab_search_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Search scoring constants.
constexpr double kScoreExactTitleMatch = 1000.0;
constexpr double kScoreTitlePrefix = 500.0;
constexpr double kScoreExactHostMatch = 300.0;
constexpr double kScoreTitleSubstring = 100.0;
constexpr double kScoreHostSubstring = 50.0;
constexpr double kScoreWorkspaceMatch = 80.0;
constexpr double kScoreGroupMatch = 60.0;
constexpr double kScoreRecencyMaxBonus = 49.0;  // Less than lowest match tier.

// Fuzzy match: check if query characters appear in order in the text.
// Returns a score boost for how well the query matches fuzzily.
// 0 means no match at all.
double FuzzyMatchScore(const std::u16string& text,
                       const std::u16string& query_lower) {
  if (query_lower.empty()) {
    return 0.0;
  }

  std::u16string text_lower = base::ToLowerASCII(text);

  // Simple sequential character matching.
  size_t query_pos = 0;
  size_t text_pos = 0;
  int consecutive_bonus = 0;
  int max_consecutive = 0;

  while (text_pos < text_lower.size() && query_pos < query_lower.size()) {
    if (text_lower[text_pos] == query_lower[query_pos]) {
      ++query_pos;
      ++consecutive_bonus;
      max_consecutive = std::max(max_consecutive, consecutive_bonus);
    } else {
      consecutive_bonus = 0;
    }
    ++text_pos;
  }

  if (query_pos < query_lower.size()) {
    return 0.0;  // Not all characters found in order.
  }

  // Score based on how much of the text is matched consecutively.
  return 20.0 + static_cast<double>(max_consecutive) * 2.0;
}

// Compute a recency bonus based on last_active_time.
// More recent tabs get a higher bonus (up to kScoreRecencyMaxBonus).
double RecencyBonus(const base::Time& last_active,
                    const base::Time& now) {
  if (last_active.is_null()) {
    return 0.0;
  }

  base::TimeDelta delta = now - last_active;

  // 0 minutes ago = full bonus.
  // 60 minutes ago = zero bonus.
  double minutes_ago = delta.InSecondsF() / 60.0;
  double bonus = kScoreRecencyMaxBonus *
                 std::max(0.0, 1.0 - (minutes_ago / 60.0));

  return std::max(0.0, std::min(kScoreRecencyMaxBonus, bonus));
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabSearchModel::AstraTabSearchModel() = default;

AstraTabSearchModel::~AstraTabSearchModel() {
  // Notify observers that the model is shutting down.
  for (auto& observer : observers_) {
    observer.OnTabSearchModelShutdown(this);
  }
}

// =========================================================================
// Observer management
// =========================================================================

void AstraTabSearchModel::AddObserver(AstraTabSearchObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabSearchModel::RemoveObserver(AstraTabSearchObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Tab list access
// =========================================================================

size_t AstraTabSearchModel::GetTabCount() const {
  return tabs_.size();
}

const AstraTabSearchItem* AstraTabSearchModel::GetTabAt(int index) const {
  if (index < 0 || static_cast<size_t>(index) >= tabs_.size()) {
    return nullptr;
  }
  return &tabs_[static_cast<size_t>(index)];
}

// =========================================================================
// Search
// =========================================================================

std::vector<AstraTabSearchItem> AstraTabSearchModel::SearchTabs(
    const std::u16string& query) const {
  return SearchTabsInMode(query, search_mode_);
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::SearchTabsInMode(
    const std::u16string& query,
    AstraTabSearchMode mode) const {
  std::u16string lower_query = base::ToLowerASCII(query);

  std::vector<AstraTabSearchItem> results;
  results.reserve(tabs_.size());

  // Collect tabs that pass the mode filter and match the query.
  for (const auto& tab : tabs_) {
    if (!PassesModeFilter(tab, mode)) {
      continue;
    }
    if (!query.empty() && !MatchesQuery(tab, lower_query)) {
      continue;
    }

    AstraTabSearchItem scored = tab;
    scored.relevance_score = ComputeRelevanceScore(tab, query, lower_query);
    results.push_back(std::move(scored));
  }

  // Include recently closed tabs if mode allows.
  if (show_recently_closed_section_ &&
      (mode == AstraTabSearchMode::kAllTabs ||
       mode == AstraTabSearchMode::kRecentlyClosed)) {
    for (const auto& tab : recently_closed_tabs_) {
      if (mode == AstraTabSearchMode::kRecentlyClosed ||
          PassesModeFilter(tab, mode)) {
        if (!query.empty() && !MatchesQuery(tab, lower_query)) {
          continue;
        }
        AstraTabSearchItem scored = tab;
        scored.relevance_score =
            ComputeRelevanceScore(tab, query, lower_query);
        results.push_back(std::move(scored));
      }
    }
  }

  // Sort results.
  SortResults(results);

  // Cap results.
  if (results.size() > max_search_results_) {
    results.resize(max_search_results_);
  }

  return results;
}

// =========================================================================
// Search mode
// =========================================================================

void AstraTabSearchModel::SetSearchMode(AstraTabSearchMode mode) {
  if (search_mode_ == mode) {
    return;
  }
  search_mode_ = mode;
  NotifySearchModeChanged();
  NotifySearchResultsChanged();
}

// =========================================================================
// Workspace filtering
// =========================================================================

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetTabsByWorkspace(
    const std::string& workspace_id) const {
  std::vector<AstraTabSearchItem> result;
  for (const auto& tab : tabs_) {
    if (tab.workspace_id == workspace_id) {
      result.push_back(tab);
    }
  }
  return result;
}

const AstraTabSearchItem* AstraTabSearchModel::GetActiveTab() const {
  for (const auto& tab : tabs_) {
    if (tab.is_active) {
      return &tab;
    }
  }
  if (!tabs_.empty()) {
    return &tabs_[0];  // Fallback to first tab.
  }
  return nullptr;
}

// =========================================================================
// Special collections
// =========================================================================

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetRecentlyClosedTabs(
    int max_count) const {
  std::vector<AstraTabSearchItem> result;
  int count = std::min(max_count,
                       static_cast<int>(recently_closed_tabs_.size()));
  for (int i = 0; i < count; ++i) {
    result.push_back(recently_closed_tabs_[static_cast<size_t>(i)]);
  }
  return result;
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetTabsWithAudio()
    const {
  std::vector<AstraTabSearchItem> result;
  for (const auto& tab : tabs_) {
    if (tab.is_audible) {
      result.push_back(tab);
    }
  }
  return result;
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetPinnedTabs() const {
  std::vector<AstraTabSearchItem> result;
  for (const auto& tab : tabs_) {
    if (tab.is_pinned) {
      result.push_back(tab);
    }
  }
  return result;
}

// =========================================================================
// Tab groups
// =========================================================================

std::vector<AstraTabSearchGroupInfo> AstraTabSearchModel::GetTabGroups()
    const {
  return tab_groups_;
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetTabsInGroup(
    const std::string& group_id) const {
  std::vector<AstraTabSearchItem> result;
  for (const auto& tab : tabs_) {
    if (tab.is_in_group && tab.group_id == group_id) {
      result.push_back(tab);
    }
  }
  return result;
}

// =========================================================================
// Windows
// =========================================================================

int AstraTabSearchModel::GetWindowCount() const {
  std::set<int> windows;
  for (const auto& tab : tabs_) {
    windows.insert(tab.window_id);
  }
  return static_cast<int>(windows.size());
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetTabsInWindow(
    int window_id) const {
  std::vector<AstraTabSearchItem> result;
  for (const auto& tab : tabs_) {
    if (tab.window_id == window_id) {
      result.push_back(tab);
    }
  }
  return result;
}

// =========================================================================
// Tab actions
// =========================================================================

void AstraTabSearchModel::SwitchToTab(int tab_index) {
  if (tab_index < 0 || static_cast<size_t>(tab_index) >= tabs_.size()) {
    return;
  }

  // TODO(astra): Delegate to TabStripModel::ActivateTabAt.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  //   Patch point: Browser::tab_strip_model()->ActivateTabAt(index)

  // Update our projected state to reflect the activation.
  for (auto& tab : tabs_) {
    if (tab.window_id == tabs_[static_cast<size_t>(tab_index)].window_id) {
      tab.is_active = false;
    }
  }
  if (static_cast<size_t>(tab_index) < tabs_.size()) {
    tabs_[static_cast<size_t>(tab_index)].is_active = true;
    tabs_[static_cast<size_t>(tab_index)].last_active_time =
        base::Time::Now();
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnTabActivated(this, tab_index);
  }
}

void AstraTabSearchModel::CloseTab(int tab_index) {
  if (tab_index < 0 || static_cast<size_t>(tab_index) >= tabs_.size()) {
    return;
  }

  // TODO(astra): Delegate to TabStripModel::CloseWebContentsAt.
  //   Chromium owner: TabStripModel::CloseWebContentsAt

  // Remove from our projected list.
  int removed_index = tab_index;
  tabs_.erase(tabs_.begin() + tab_index);

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnTabClosed(this, removed_index);
    observer.OnTabListChanged(this);
    observer.OnSearchResultsChanged(this);
  }
}

void AstraTabSearchModel::MoveTabToWorkspace(
    int tab_index,
    const std::string& workspace_id) {
  if (tab_index < 0 || static_cast<size_t>(tab_index) >= tabs_.size()) {
    return;
  }

  // TODO(astra): Integrate with Astra workspace service.
  //   Astra owner: AstraWorkspaceService (astra/browser/)

  tabs_[static_cast<size_t>(tab_index)].workspace_id = workspace_id;

  // Notify observers of tab list change.
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

// =========================================================================
// Data management
// =========================================================================

void AstraTabSearchModel::SetTabList(std::vector<AstraTabSearchItem> tabs) {
  tabs_ = std::move(tabs);
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::RefreshTabList() {
  // TODO(astra): Wire to TabStripModel observer for automatic refresh.
  //   Chromium component: TabStripModelObserver
  //   (chrome/browser/ui/tabs/tab_strip_model_observer.h)

  // For now, just notify observers that the list may have changed.
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::SetRecentlyClosedTabs(
    std::vector<AstraTabSearchItem> tabs) {
  recently_closed_tabs_ = std::move(tabs);
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::SetTabGroups(
    std::vector<AstraTabSearchGroupInfo> groups) {
  tab_groups_ = std::move(groups);
}

void AstraTabSearchModel::SetCurrentWorkspaceId(
    const std::string& workspace_id) {
  if (current_workspace_id_ == workspace_id) {
    return;
  }
  current_workspace_id_ = workspace_id;
  // If current mode depends on workspace, refresh results.
  if (search_mode_ == AstraTabSearchMode::kCurrentWorkspace ||
      search_mode_ == AstraTabSearchMode::kOtherWorkspaces) {
    NotifySearchResultsChanged();
  }
}

// =========================================================================
// Settings
// =========================================================================

void AstraTabSearchModel::set_max_search_results(size_t max) {
  size_t clamped = std::min(max, kMaxSearchResultsMax);
  if (max_search_results_ == clamped) {
    return;
  }
  max_search_results_ = clamped;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_tab_urls(bool show) {
  if (show_tab_urls_ == show) {
    return;
  }
  show_tab_urls_ = show;
  // Pure presentation — notify results changed so UI updates.
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_workspace_name(bool show) {
  if (show_workspace_name_ == show) {
    return;
  }
  show_workspace_name_ = show;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_tab_groups(bool show) {
  if (show_tab_groups_ == show) {
    return;
  }
  show_tab_groups_ = show;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_favicons(bool show) {
  if (show_favicons_ == show) {
    return;
  }
  show_favicons_ = show;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_search_in_urls(bool enabled) {
  if (search_in_urls_ == enabled) {
    return;
  }
  search_in_urls_ = enabled;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_search_in_tab_titles_only(bool enabled) {
  if (search_in_tab_titles_only_ == enabled) {
    return;
  }
  search_in_tab_titles_only_ = enabled;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_default_search_mode(
    AstraTabSearchMode mode) {
  default_search_mode_ = mode;
}

void AstraTabSearchModel::SetSortOrder(AstraTabSearchSortOrder order) {
  if (sort_order_ == order) {
    return;
  }
  sort_order_ = order;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_close_tab_on_activate(bool enabled) {
  close_tab_on_activate_ = enabled;
}

void AstraTabSearchModel::set_show_recently_closed_section(bool show) {
  if (show_recently_closed_section_ == show) {
    return;
  }
  show_recently_closed_section_ = show;
  NotifySearchResultsChanged();
}

// =========================================================================
// Persistence
// =========================================================================

void AstraTabSearchModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  int max_results = prefs->GetInteger(prefs::kPrefTabSearchMaxVisible);
  if (max_results > 0) {
    max_search_results_ = std::min(
        static_cast<size_t>(max_results), kMaxSearchResultsMax);
  }

  show_tab_urls_ = prefs->GetBoolean(prefs::kPrefTabSearchShowUrls);
  show_favicons_ = prefs->GetBoolean(prefs::kPrefTabSearchShowThumbnails);
  show_recently_closed_section_ =
      prefs->GetBoolean(prefs::kPrefTabSearchShowRecentSection);

  int sort_order_int = prefs->GetInteger(prefs::kPrefTabSearchSortOrder);
  if (sort_order_int >= 0 &&
      sort_order_int <=
          static_cast<int>(AstraTabSearchSortOrder::kByPosition)) {
    sort_order_ = static_cast<AstraTabSearchSortOrder>(sort_order_int);
  }

  // Additional Astra-specific settings.
  // TODO(astra): Add pref entries for workspace name display, tab groups,
  //   search in URLs, etc. in astra_prefs.h
  //   Astra owner: AstraPrefs (astra/browser/astra_prefs.h)
}

void AstraTabSearchModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  prefs->SetInteger(prefs::kPrefTabSearchMaxVisible,
                   static_cast<int>(max_search_results_));
  prefs->SetBoolean(prefs::kPrefTabSearchShowUrls, show_tab_urls_);
  prefs->SetBoolean(prefs::kPrefTabSearchShowThumbnails, show_favicons_);
  prefs->SetBoolean(prefs::kPrefTabSearchShowRecentSection,
                    show_recently_closed_section_);
  prefs->SetInteger(prefs::kPrefTabSearchSortOrder,
                    static_cast<int>(sort_order_));
}

// =========================================================================
// Internal helpers — filtering
// =========================================================================

bool AstraTabSearchModel::PassesModeFilter(const AstraTabSearchItem& tab,
                                           AstraTabSearchMode mode) const {
  switch (mode) {
    case AstraTabSearchMode::kAllTabs:
      return true;

    case AstraTabSearchMode::kCurrentWorkspace:
      return tab.workspace_id == current_workspace_id_;

    case AstraTabSearchMode::kOtherWorkspaces:
      return !tab.workspace_id.empty() &&
             tab.workspace_id != current_workspace_id_;

    case AstraTabSearchMode::kRecentlyClosed:
      // Recently closed tabs are handled separately in SearchTabsInMode.
      return false;

    case AstraTabSearchMode::kFavorites:
      // TODO(astra): Implement favorites / bookmark filtering.
      //   Chromium component: BookmarkModel
      //   For now, no tabs match favorites mode (favorites come from
      //   bookmark data, not from open tabs).
      return false;

    case AstraTabSearchMode::kAudioPlaying:
      return tab.is_audible;
  }

  return false;
}

// =========================================================================
// Internal helpers — scoring
// =========================================================================

double AstraTabSearchModel::ComputeRelevanceScore(
    const AstraTabSearchItem& tab,
    const std::u16string& query,
    const std::u16string& lower_query) const {
  if (query.empty()) {
    // For empty queries, use recency bonus only.
    return RecencyBonus(tab.last_active_time, base::Time::Now());
  }

  double score = 0.0;

  std::u16string lower_title = base::ToLowerASCII(tab.title);
  std::u16string lower_host = base::ToLowerASCII(tab.hostname);

  // --- Title matching ---

  // Exact title match.
  if (lower_title == lower_query) {
    score += kScoreExactTitleMatch;
  }
  // Title prefix match.
  else if (base::StartsWith(lower_title, lower_query,
                            base::CompareCase::SENSITIVE)) {
    score += kScoreTitlePrefix;
  }
  // Title substring match.
  else if (lower_title.find(lower_query) != std::u16string::npos) {
    score += kScoreTitleSubstring;
  }

  // --- Host / URL matching ---

  if (search_in_urls_ && !search_in_tab_titles_only_) {
    // Exact host match.
    if (lower_host == lower_query) {
      score += kScoreExactHostMatch;
    }
    // Host substring match.
    else if (lower_host.find(lower_query) != std::u16string::npos) {
      score += kScoreHostSubstring;
    }

    // Full URL search.
    // TODO(astra): Consider searching full URL spec, not just hostname.
    //   For performance, we currently only search hostname.
  }

  // --- Workspace matching ---

  if (!search_in_tab_titles_only_ && !tab.workspace_name.empty()) {
    std::u16string ws_lower = base::ToLowerASCII(tab.workspace_name);
    if (ws_lower.find(lower_query) != std::u16string::npos) {
      score += kScoreWorkspaceMatch;
    }
  }

  // --- Group matching ---

  if (!search_in_tab_titles_only_ && tab.is_in_group &&
      !tab.group_name.empty()) {
    std::u16string group_lower = base::ToLowerASCII(tab.group_name);
    if (group_lower.find(lower_query) != std::u16string::npos) {
      score += kScoreGroupMatch;
    }
  }

  // --- Fuzzy match fallback ---

  if (score == 0.0) {
    double fuzzy_title = FuzzyMatchScore(tab.title, lower_query);
    if (fuzzy_title > 0.0) {
      score += fuzzy_title;
    } else if (search_in_urls_ && !search_in_tab_titles_only_) {
      double fuzzy_host = FuzzyMatchScore(tab.hostname, lower_query);
      if (fuzzy_host > 0.0) {
        score += fuzzy_host / 2.0;  // Host fuzzy counts less than title.
      }
    }
  }

  // --- Recency bonus (tiebreaker only) ---

  double recency_bonus = RecencyBonus(tab.last_active_time, base::Time::Now());
  score += std::min(recency_bonus, kScoreRecencyMaxBonus);

  return score;
}

bool AstraTabSearchModel::MatchesQuery(
    const AstraTabSearchItem& tab,
    const std::u16string& lower_query) const {
  if (lower_query.empty()) {
    return true;  // Empty query matches everything.
  }

  std::u16string lower_title = base::ToLowerASCII(tab.title);

  // Title match.
  if (lower_title.find(lower_query) != std::u16string::npos) {
    return true;
  }

  // Host / URL match.
  if (search_in_urls_ && !search_in_tab_titles_only_) {
    std::u16string lower_host = base::ToLowerASCII(tab.hostname);
    if (lower_host.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Workspace name match.
  if (!search_in_tab_titles_only_ && !tab.workspace_name.empty()) {
    std::u16string ws_lower = base::ToLowerASCII(tab.workspace_name);
    if (ws_lower.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Group name match.
  if (!search_in_tab_titles_only_ && tab.is_in_group &&
      !tab.group_name.empty()) {
    std::u16string group_lower = base::ToLowerASCII(tab.group_name);
    if (group_lower.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Fuzzy match as fallback.
  if (FuzzyMatchScore(tab.title, lower_query) > 0.0) {
    return true;
  }
  if (search_in_urls_ && !search_in_tab_titles_only_) {
    if (FuzzyMatchScore(tab.hostname, lower_query) > 0.0) {
      return true;
    }
  }

  return false;
}

// =========================================================================
// Internal helpers — sorting
// =========================================================================

void AstraTabSearchModel::SortResults(
    std::vector<AstraTabSearchItem>& results) const {
  switch (sort_order_) {
    case AstraTabSearchSortOrder::kByRecency:
      std::stable_sort(results.begin(), results.end(),
                       [](const AstraTabSearchItem& a,
                          const AstraTabSearchItem& b) {
                         // More recent first.
                         if (a.last_active_time != b.last_active_time) {
                           return a.last_active_time > b.last_active_time;
                         }
                         // Fallback: by tab index.
                         if (a.window_id != b.window_id) {
                           return a.window_id < b.window_id;
                         }
                         return a.tab_index < b.tab_index;
                       });
      break;

    case AstraTabSearchSortOrder::kByTitle:
      std::stable_sort(results.begin(), results.end(),
                       [](const AstraTabSearchItem& a,
                          const AstraTabSearchItem& b) {
                         return base::CompareCaseInsensitive(a.title,
                                                              b.title) < 0;
                       });
      break;

    case AstraTabSearchSortOrder::kByPosition:
      std::stable_sort(results.begin(), results.end(),
                       [](const AstraTabSearchItem& a,
                          const AstraTabSearchItem& b) {
                         // By window, then by tab index.
                         if (a.window_id != b.window_id) {
                           return a.window_id < b.window_id;
                         }
                         return a.tab_index < b.tab_index;
                       });
      break;
  }
}

// =========================================================================
// Internal helpers — notifications
// =========================================================================

void AstraTabSearchModel::NotifyTabListChanged() {
  for (auto& observer : observers_) {
    observer.OnTabListChanged(this);
  }
}

void AstraTabSearchModel::NotifySearchResultsChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }
}

void AstraTabSearchModel::NotifySearchModeChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchModeChanged(this, search_mode_);
  }
}

}  // namespace astra
