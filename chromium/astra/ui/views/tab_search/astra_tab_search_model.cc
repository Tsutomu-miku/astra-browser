#include "astra/ui/views/tab_search/astra_tab_search_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Search scoring constants.
constexpr double kScoreExactTitleMatch = 1000.0;
constexpr double kScoreTitlePrefix = 500.0;
constexpr double kScoreTitleWordBoundary = 300.0;
constexpr double kScoreExactHostMatch = 300.0;
constexpr double kScoreHostPrefix = 150.0;
constexpr double kScoreTitleSubstring = 100.0;
constexpr double kScoreHostSubstring = 50.0;
constexpr double kScoreWorkspaceMatch = 80.0;
constexpr double kScoreGroupMatch = 60.0;
constexpr double kScoreHistoryVisitBonus = 0.5;  // Per visit.
constexpr double kScoreRecencyMaxBonus = 49.0;  // Less than lowest match tier.
constexpr double kScoreBookmarkBoost = 20.0;
constexpr double kScoreOpenTabBoost = 10.0;

// Fuzzy match: check if query characters appear in order in the text.
// Returns a score boost for how well the query matches fuzzily.
// 0 means no match at all.
double FuzzyMatchScoreInternal(const std::u16string& text,
                               const std::u16string& query_lower) {
  if (query_lower.empty()) {
    return 0.0;
  }

  std::u16string text_lower = base::i18n::ToLower(text);

  // Simple sequential character matching.
  size_t query_pos = 0;
  size_t text_pos = 0;
  int consecutive_bonus = 0;
  int max_consecutive = 0;
  int first_match_pos = -1;

  while (text_pos < text_lower.size() && query_pos < query_lower.size()) {
    if (text_lower[text_pos] == query_lower[query_pos]) {
      if (first_match_pos < 0) {
        first_match_pos = static_cast<int>(text_pos);
      }
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

  // Score based on:
  //   - Base score for matching at all
  //   - Bonus for consecutive matches
  //   - Penalty for match starting later in the string
  double base_score = 20.0;
  double consecutive_boost = static_cast<double>(max_consecutive) * 3.0;
  double position_penalty = static_cast<double>(first_match_pos) * 0.5;

  return std::max(0.0, base_score + consecutive_boost - position_penalty);
}

// Compute a recency bonus based on last_visited_time.
// More recent items get a higher bonus (up to kScoreRecencyMaxBonus).
double RecencyBonusInternal(const base::Time& last_visited,
                            const base::Time& now) {
  if (last_visited.is_null()) {
    return 0.0;
  }

  base::TimeDelta delta = now - last_visited;

  // 0 minutes ago = full bonus.
  // 60 minutes ago = zero bonus.
  double minutes_ago = delta.InSecondsF() / 60.0;
  double bonus = kScoreRecencyMaxBonus *
                 std::max(0.0, 1.0 - (minutes_ago / 60.0));

  return std::max(0.0, std::min(kScoreRecencyMaxBonus, bonus));
}

// Check if a position is a word boundary in the given text.
bool IsWordBoundary(const std::u16string& text, size_t pos) {
  if (pos == 0) {
    return true;
  }
  if (pos >= text.size()) {
    return false;
  }
  char16_t prev = text[pos - 1];
  return !base::IsAsciiAlpha(prev) && !base::IsAsciiDigit(prev);
}

// Find match ranges in text for a query.
std::vector<gfx::Range> FindMatchRanges(const std::u16string& text_lower,
                                        const std::u16string& query_lower) {
  std::vector<gfx::Range> ranges;
  if (query_lower.empty() || text_lower.empty()) {
    return ranges;
  }

  size_t pos = 0;
  while (pos < text_lower.size()) {
    size_t found = text_lower.find(query_lower, pos);
    if (found == std::u16string::npos) {
      break;
    }
    ranges.emplace_back(found, found + query_lower.size());
    pos = found + query_lower.size();
  }

  return ranges;
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

void AstraTabSearchModel::SetQuery(const std::u16string& query) {
  if (query_ == query) {
    return;
  }
  query_ = query;
  results_dirty_ = true;
  RunSearch();
  NotifyQueryChanged();
  NotifySearchResultsChanged();

  // Reset selection to first item when query changes.
  if (!results_.empty() && selected_index_ >= results_.size()) {
    size_t old_index = selected_index_;
    selected_index_ = 0;
    NotifySelectedIndexChanged(old_index, 0);
  }
}

void AstraTabSearchModel::RunSearch() {
  if (!results_dirty_) {
    return;
  }
  results_dirty_ = false;

  std::u16string lower_query = base::i18n::ToLower(query_);
  std::vector<AstraTabSearchItem> results;

  // Reserve space to avoid reallocations.
  results.reserve(tabs_.size() + recently_closed_tabs_.size() +
                  bookmarks_.size() + history_.size());

  // --- Open tabs ---
  if (FilterAllowsType(AstraTabSearchResultType::kOpenTab)) {
    for (const auto& item : tabs_) {
      if (!PassesModeFilter(item, search_mode_)) {
        continue;
      }
      if (!query_.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query_, lower_query);
      // Boost for being an open tab (slightly preferred).
      scored.relevance_score += kScoreOpenTabBoost;
      results.push_back(std::move(scored));
    }
  }

  // --- Recently closed tabs ---
  if (FilterAllowsType(AstraTabSearchResultType::kRecentlyClosed) &&
      show_recently_closed_section_) {
    for (const auto& item : recently_closed_tabs_) {
      if (!query_.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query_, lower_query);
      results.push_back(std::move(scored));
    }
  }

  // --- Bookmarks ---
  if (FilterAllowsType(AstraTabSearchResultType::kBookmark)) {
    for (const auto& item : bookmarks_) {
      if (!query_.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query_, lower_query);
      scored.relevance_score += kScoreBookmarkBoost;
      results.push_back(std::move(scored));
    }
  }

  // --- History ---
  if (FilterAllowsType(AstraTabSearchResultType::kHistory)) {
    for (const auto& item : history_) {
      if (!query_.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query_, lower_query);
      // Bonus based on visit count (up to a reasonable cap).
      scored.relevance_score +=
          kScoreHistoryVisitBonus * std::min(item.visit_count, 100);
      results.push_back(std::move(scored));
    }
  }

  // Sort results.
  SortResults(results);

  // Cap results.
  if (results.size() > max_search_results_) {
    results.resize(max_search_results_);
  }

  results_ = std::move(results);
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::SearchTabs(
    const std::u16string& query) const {
  return SearchTabsInMode(query, search_mode_);
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::SearchTabsInMode(
    const std::u16string& query,
    AstraTabSearchMode mode) const {
  std::u16string lower_query = base::i18n::ToLower(query);

  std::vector<AstraTabSearchItem> results;
  results.reserve(tabs_.size());

  // Collect tabs that pass the mode filter and match the query.
  if (FilterAllowsType(AstraTabSearchResultType::kOpenTab)) {
    for (const auto& tab : tabs_) {
      if (!PassesModeFilter(tab, mode)) {
        continue;
      }
      if (!query.empty() && !MatchesQuery(tab, lower_query)) {
        continue;
      }

      AstraTabSearchItem scored = tab;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(tab, query, lower_query);
      scored.relevance_score += kScoreOpenTabBoost;
      results.push_back(std::move(scored));
    }
  }

  // Include recently closed tabs if mode allows and filter permits.
  if (show_recently_closed_section_ &&
      FilterAllowsType(AstraTabSearchResultType::kRecentlyClosed) &&
      (mode == AstraTabSearchMode::kAllTabs ||
       mode == AstraTabSearchMode::kRecentlyClosed)) {
    for (const auto& tab : recently_closed_tabs_) {
      if (!query.empty() && !MatchesQuery(tab, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = tab;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(tab, query, lower_query);
      results.push_back(std::move(scored));
    }
  }

  // Include bookmarks if filter permits.
  if (FilterAllowsType(AstraTabSearchResultType::kBookmark)) {
    for (const auto& item : bookmarks_) {
      if (!query.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query, lower_query);
      scored.relevance_score += kScoreBookmarkBoost;
      results.push_back(std::move(scored));
    }
  }

  // Include history if filter permits.
  if (FilterAllowsType(AstraTabSearchResultType::kHistory)) {
    for (const auto& item : history_) {
      if (!query.empty() && !MatchesQuery(item, lower_query)) {
        continue;
      }
      AstraTabSearchItem scored = item;
      scored.relevance_score =
          ComputeRelevanceScoreInternal(item, query, lower_query);
      scored.relevance_score +=
          kScoreHistoryVisitBonus * std::min(item.visit_count, 100);
      results.push_back(std::move(scored));
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

AstraTabSearchGroupedResults AstraTabSearchModel::GetGroupedResults() const {
  AstraTabSearchGroupedResults grouped;

  for (const auto& item : results_) {
    switch (item.result_type) {
      case AstraTabSearchResultType::kOpenTab:
        grouped.open_tabs.push_back(item);
        break;
      case AstraTabSearchResultType::kRecentlyClosed:
        grouped.recently_closed.push_back(item);
        break;
      case AstraTabSearchResultType::kBookmark:
        grouped.bookmarks.push_back(item);
        break;
      case AstraTabSearchResultType::kHistory:
        grouped.history.push_back(item);
        break;
      case AstraTabSearchResultType::kSearchHistory:
      case AstraTabSearchResultType::kAction:
        // These are handled separately.
        break;
    }
  }

  // Add recent searches (only if query is empty).
  if (query_.empty() && show_recent_searches_) {
    grouped.recent_searches = recent_searches_;
  }

  return grouped;
}

std::vector<AstraTabSearchMatch> AstraTabSearchModel::ComputeMatches(
    const AstraTabSearchItem& item,
    const std::u16string& query) const {
  std::vector<AstraTabSearchMatch> matches;
  if (query.empty()) {
    return matches;
  }

  std::u16string lower_query = base::i18n::ToLower(query);
  std::u16string lower_title = base::i18n::ToLower(item.title);
  std::u16string lower_host = base::i18n::ToLower(item.hostname);

  // Title matches.
  auto title_ranges = FindMatchRanges(lower_title, lower_query);
  for (const auto& range : title_ranges) {
    matches.push_back({AstraTabSearchMatch::Type::kTitle, range});
  }

  // Hostname matches (if URL search is enabled).
  if (search_in_urls_ && !search_in_tab_titles_only_) {
    auto host_ranges = FindMatchRanges(lower_host, lower_query);
    for (const auto& range : host_ranges) {
      matches.push_back({AstraTabSearchMatch::Type::kHostname, range});
    }
  }

  // Workspace name matches.
  if (!search_in_tab_titles_only_ && !item.workspace_name.empty()) {
    std::u16string lower_ws = base::i18n::ToLower(item.workspace_name);
    auto ws_ranges = FindMatchRanges(lower_ws, lower_query);
    for (const auto& range : ws_ranges) {
      matches.push_back({AstraTabSearchMatch::Type::kWorkspace, range});
    }
  }

  // Group name matches.
  if (!search_in_tab_titles_only_ && item.is_in_group &&
      !item.group_name.empty()) {
    std::u16string lower_group = base::i18n::ToLower(item.group_name);
    auto group_ranges = FindMatchRanges(lower_group, lower_query);
    for (const auto& range : group_ranges) {
      matches.push_back({AstraTabSearchMatch::Type::kGroup, range});
    }
  }

  return matches;
}

// =========================================================================
// Search filter
// =========================================================================

void AstraTabSearchModel::SetFilter(AstraTabSearchFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  results_dirty_ = true;
  RunSearch();
  NotifyFilterChanged();
  NotifySearchResultsChanged();
}

bool AstraTabSearchModel::FilterAllowsType(
    AstraTabSearchResultType type) const {
  switch (type) {
    case AstraTabSearchResultType::kOpenTab:
      return (static_cast<int>(filter_) &
              static_cast<int>(AstraTabSearchFilter::kTabs)) != 0;
    case AstraTabSearchResultType::kRecentlyClosed:
      return (static_cast<int>(filter_) &
              static_cast<int>(AstraTabSearchFilter::kRecentlyClosed)) != 0;
    case AstraTabSearchResultType::kBookmark:
      return (static_cast<int>(filter_) &
              static_cast<int>(AstraTabSearchFilter::kBookmarks)) != 0;
    case AstraTabSearchResultType::kHistory:
      return (static_cast<int>(filter_) &
              static_cast<int>(AstraTabSearchFilter::kHistory)) != 0;
    case AstraTabSearchResultType::kSearchHistory:
    case AstraTabSearchResultType::kAction:
      return false;
  }
  return false;
}

// =========================================================================
// Search mode
// =========================================================================

void AstraTabSearchModel::SetSearchMode(AstraTabSearchMode mode) {
  if (search_mode_ == mode) {
    return;
  }
  search_mode_ = mode;
  results_dirty_ = true;
  RunSearch();
  NotifySearchModeChanged();
  NotifySearchResultsChanged();
}

// =========================================================================
// Selected index (keyboard navigation)
// =========================================================================

void AstraTabSearchModel::SetSelectedIndex(size_t index) {
  if (results_.empty()) {
    return;
  }
  size_t clamped = std::min(index, results_.size() - 1);
  if (selected_index_ == clamped) {
    return;
  }
  size_t old_index = selected_index_;
  selected_index_ = clamped;
  NotifySelectedIndexChanged(old_index, selected_index_);
}

void AstraTabSearchModel::SelectNext() {
  if (results_.empty()) {
    return;
  }
  size_t old_index = selected_index_;
  if (selected_index_ < results_.size() - 1) {
    selected_index_++;
  } else {
    selected_index_ = 0;  // Wrap around to first.
  }
  if (selected_index_ != old_index) {
    NotifySelectedIndexChanged(old_index, selected_index_);
  }
}

void AstraTabSearchModel::SelectPrevious() {
  if (results_.empty()) {
    return;
  }
  size_t old_index = selected_index_;
  if (selected_index_ > 0) {
    selected_index_--;
  } else {
    selected_index_ = results_.size() - 1;  // Wrap around to last.
  }
  if (selected_index_ != old_index) {
    NotifySelectedIndexChanged(old_index, selected_index_);
  }
}

void AstraTabSearchModel::SelectFirst() {
  if (results_.empty()) {
    return;
  }
  if (selected_index_ == 0) {
    return;
  }
  size_t old_index = selected_index_;
  selected_index_ = 0;
  NotifySelectedIndexChanged(old_index, 0);
}

void AstraTabSearchModel::SelectLast() {
  if (results_.empty()) {
    return;
  }
  size_t last = results_.size() - 1;
  if (selected_index_ == last) {
    return;
  }
  size_t old_index = selected_index_;
  selected_index_ = last;
  NotifySelectedIndexChanged(old_index, last);
}

const AstraTabSearchItem* AstraTabSearchModel::GetSelectedItem() const {
  if (results_.empty() || selected_index_ >= results_.size()) {
    return nullptr;
  }
  return &results_[selected_index_];
}

void AstraTabSearchModel::ActivateSelected() {
  const AstraTabSearchItem* selected = GetSelectedItem();
  if (!selected) {
    return;
  }
  // For open tabs, use SwitchToTab.
  if (selected->result_type == AstraTabSearchResultType::kOpenTab &&
      selected->tab_index >= 0) {
    SwitchToTab(selected->tab_index);
  }
  // TODO(astra): Handle activation for bookmarks, history, etc.
  //   Chromium components: BookmarkModel, HistoryService.
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

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetBookmarks(
    int max_count) const {
  std::vector<AstraTabSearchItem> result;
  int count = std::min(max_count,
                       static_cast<int>(bookmarks_.size()));
  for (int i = 0; i < count; ++i) {
    result.push_back(bookmarks_[static_cast<size_t>(i)]);
  }
  return result;
}

std::vector<AstraTabSearchItem> AstraTabSearchModel::GetHistory(
    int max_count) const {
  std::vector<AstraTabSearchItem> result;
  int count = std::min(max_count,
                       static_cast<int>(history_.size()));
  for (int i = 0; i < count; ++i) {
    result.push_back(history_[static_cast<size_t>(i)]);
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
// Recent searches
// =========================================================================

void AstraTabSearchModel::AddRecentSearch(const std::u16string& query) {
  if (query.empty()) {
    return;
  }

  // Check if query already exists; if so, move to top and increment count.
  for (auto it = recent_searches_.begin(); it != recent_searches_.end();
       ++it) {
    if (it->query == query) {
      it->visit_count++;
      it->timestamp = base::Time::Now();
      // Move to front.
      AstraTabSearchRecentSearch entry = *it;
      recent_searches_.erase(it);
      recent_searches_.insert(recent_searches_.begin(), std::move(entry));
      NotifyRecentSearchesChanged();
      return;
    }
  }

  // Add new entry at the beginning.
  AstraTabSearchRecentSearch entry;
  entry.query = query;
  entry.timestamp = base::Time::Now();
  entry.visit_count = 1;
  recent_searches_.insert(recent_searches_.begin(), std::move(entry));

  // Trim to max size.
  if (recent_searches_.size() > kMaxRecentSearches) {
    recent_searches_.resize(kMaxRecentSearches);
  }

  NotifyRecentSearchesChanged();
}

void AstraTabSearchModel::ClearRecentSearches() {
  if (recent_searches_.empty()) {
    return;
  }
  recent_searches_.clear();
  NotifyRecentSearchesChanged();
}

void AstraTabSearchModel::RemoveRecentSearch(const std::u16string& query) {
  for (auto it = recent_searches_.begin(); it != recent_searches_.end();
       ++it) {
    if (it->query == query) {
      recent_searches_.erase(it);
      NotifyRecentSearchesChanged();
      return;
    }
  }
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
    tabs_[static_cast<size_t>(tab_index)].last_visited_time =
        base::Time::Now();
  }

  // Add to recent searches (if there's an active query).
  if (!query_.empty()) {
    AddRecentSearch(query_);
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

  // Invalidate results.
  results_dirty_ = true;

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

  // Invalidate results.
  results_dirty_ = true;

  // Notify observers of tab list change.
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

// =========================================================================
// Data management
// =========================================================================

void AstraTabSearchModel::SetTabList(std::vector<AstraTabSearchItem> tabs) {
  tabs_ = std::move(tabs);
  results_dirty_ = true;
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::RefreshTabList() {
  // TODO(astra): Wire to TabStripModel observer for automatic refresh.
  //   Chromium component: TabStripModelObserver
  //   (chrome/browser/ui/tabs/tab_strip_model_observer.h)

  // For now, just notify observers that the list may have changed.
  results_dirty_ = true;
  NotifyTabListChanged();
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::SetRecentlyClosedTabs(
    std::vector<AstraTabSearchItem> tabs) {
  recently_closed_tabs_ = std::move(tabs);
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::SetBookmarks(
    std::vector<AstraTabSearchItem> bookmarks) {
  bookmarks_ = std::move(bookmarks);
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::SetHistory(
    std::vector<AstraTabSearchItem> history) {
  history_ = std::move(history);
  results_dirty_ = true;
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
    results_dirty_ = true;
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
  results_dirty_ = true;
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
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_search_in_tab_titles_only(bool enabled) {
  if (search_in_tab_titles_only_ == enabled) {
    return;
  }
  search_in_tab_titles_only_ = enabled;
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_fuzzy_search_enabled(bool enabled) {
  if (fuzzy_search_enabled_ == enabled) {
    return;
  }
  fuzzy_search_enabled_ = enabled;
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_group_headers(bool show) {
  if (show_group_headers_ == show) {
    return;
  }
  show_group_headers_ = show;
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
  results_dirty_ = true;
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
  results_dirty_ = true;
  NotifySearchResultsChanged();
}

void AstraTabSearchModel::set_show_recent_searches(bool show) {
  if (show_recent_searches_ == show) {
    return;
  }
  show_recent_searches_ = show;
  NotifySearchResultsChanged();
}

// =========================================================================
// Scoring / ranking
// =========================================================================

double AstraTabSearchModel::ComputeRelevanceScore(
    const AstraTabSearchItem& item,
    const std::u16string& query) const {
  std::u16string lower_query = base::i18n::ToLower(query);
  return ComputeRelevanceScoreInternal(item, query, lower_query);
}

// static
double AstraTabSearchModel::FuzzyMatchScore(const std::u16string& text,
                                            const std::u16string& query_lower) {
  return FuzzyMatchScoreInternal(text, query_lower);
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

  results_dirty_ = true;

  // Additional Astra-specific settings.
  // TODO(astra): Add pref entries for workspace name display, tab groups,
  //   search in URLs, recent searches, etc. in astra_prefs.h
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

double AstraTabSearchModel::ComputeRelevanceScoreInternal(
    const AstraTabSearchItem& item,
    const std::u16string& query,
    const std::u16string& lower_query) const {
  if (query.empty()) {
    // For empty queries, use recency bonus only.
    return RecencyBonusInternal(item.last_visited_time, base::Time::Now());
  }

  double score = 0.0;

  std::u16string lower_title = base::i18n::ToLower(item.title);
  std::u16string lower_host = base::i18n::ToLower(item.hostname);

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
  // Word boundary match (e.g. "my d" matches "My Document").
  else if (IsWordBoundary(lower_title, 0)) {
    // Check for word-boundary match.
    size_t pos = 0;
    bool word_boundary_match = true;
    size_t qpos = 0;
    while (qpos < lower_query.size() && pos < lower_title.size()) {
      if (lower_title[pos] == lower_query[qpos]) {
        ++qpos;
        ++pos;
      } else if (!base::IsAsciiAlpha(lower_title[pos]) &&
                 !base::IsAsciiDigit(lower_title[pos])) {
        // Skip non-word characters, next char is a word boundary.
        ++pos;
      } else {
        word_boundary_match = false;
        break;
      }
    }
    if (word_boundary_match && qpos == lower_query.size()) {
      score += kScoreTitleWordBoundary;
    }
  }

  // Title substring match (if not already matched higher).
  if (score < kScoreTitleSubstring) {
    if (lower_title.find(lower_query) != std::u16string::npos) {
      score = std::max(score, kScoreTitleSubstring);
    }
  }

  // --- Host / URL matching ---

  if (search_in_urls_ && !search_in_tab_titles_only_) {
    // Exact host match.
    if (lower_host == lower_query) {
      score += kScoreExactHostMatch;
    }
    // Host prefix match.
    else if (base::StartsWith(lower_host, lower_query,
                              base::CompareCase::SENSITIVE)) {
      score += kScoreHostPrefix;
    }
    // Host substring match.
    else if (lower_host.find(lower_query) != std::u16string::npos) {
      score += kScoreHostSubstring;
    }

    // Full URL search could be added here.
    // TODO(astra): Consider searching full URL spec, not just hostname.
    //   For performance, we currently only search hostname.
  }

  // --- Workspace matching ---

  if (!search_in_tab_titles_only_ && !item.workspace_name.empty()) {
    std::u16string ws_lower = base::i18n::ToLower(item.workspace_name);
    if (ws_lower.find(lower_query) != std::u16string::npos) {
      score += kScoreWorkspaceMatch;
    }
  }

  // --- Group matching ---

  if (!search_in_tab_titles_only_ && item.is_in_group &&
      !item.group_name.empty()) {
    std::u16string group_lower = base::i18n::ToLower(item.group_name);
    if (group_lower.find(lower_query) != std::u16string::npos) {
      score += kScoreGroupMatch;
    }
  }

  // --- Fuzzy match fallback ---

  if (score == 0.0 && fuzzy_search_enabled_) {
    double fuzzy_title = FuzzyMatchScoreInternal(item.title, lower_query);
    if (fuzzy_title > 0.0) {
      score += fuzzy_title;
    } else if (search_in_urls_ && !search_in_tab_titles_only_) {
      double fuzzy_host = FuzzyMatchScoreInternal(item.hostname, lower_query);
      if (fuzzy_host > 0.0) {
        score += fuzzy_host / 2.0;  // Host fuzzy counts less than title.
      }
    }
  }

  // --- Recency bonus (tiebreaker only) ---

  double recency_bonus =
      RecencyBonusInternal(item.last_visited_time, base::Time::Now());
  score += std::min(recency_bonus, kScoreRecencyMaxBonus);

  return score;
}

bool AstraTabSearchModel::MatchesQuery(
    const AstraTabSearchItem& item,
    const std::u16string& lower_query) const {
  if (lower_query.empty()) {
    return true;  // Empty query matches everything.
  }

  std::u16string lower_title = base::i18n::ToLower(item.title);

  // Title match.
  if (lower_title.find(lower_query) != std::u16string::npos) {
    return true;
  }

  // Host / URL match.
  if (search_in_urls_ && !search_in_tab_titles_only_) {
    std::u16string lower_host = base::i18n::ToLower(item.hostname);
    if (lower_host.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Workspace name match.
  if (!search_in_tab_titles_only_ && !item.workspace_name.empty()) {
    std::u16string ws_lower = base::i18n::ToLower(item.workspace_name);
    if (ws_lower.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Group name match.
  if (!search_in_tab_titles_only_ && item.is_in_group &&
      !item.group_name.empty()) {
    std::u16string group_lower = base::i18n::ToLower(item.group_name);
    if (group_lower.find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Fuzzy match as fallback.
  if (fuzzy_search_enabled_) {
    if (FuzzyMatchScoreInternal(item.title, lower_query) > 0.0) {
      return true;
    }
    if (search_in_urls_ && !search_in_tab_titles_only_) {
      if (FuzzyMatchScoreInternal(item.hostname, lower_query) > 0.0) {
        return true;
      }
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
                         if (a.last_visited_time != b.last_visited_time) {
                           return a.last_visited_time > b.last_visited_time;
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

    case AstraTabSearchSortOrder::kByRelevance:
      SortByRelevance(results);
      break;
  }
}

// static
void AstraTabSearchModel::SortByRelevance(
    std::vector<AstraTabSearchItem>& results) {
  std::stable_sort(results.begin(), results.end(),
                   [](const AstraTabSearchItem& a,
                      const AstraTabSearchItem& b) {
                     // Higher relevance first.
                     if (a.relevance_score != b.relevance_score) {
                       return a.relevance_score > b.relevance_score;
                     }
                     // Fallback: recency.
                     return a.last_visited_time > b.last_visited_time;
                   });
}

// static
double AstraTabSearchModel::RecencyBonus(const base::Time& last_visited,
                                         const base::Time& now) {
  return RecencyBonusInternal(last_visited, now);
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
  // Make sure results are up to date before notifying.
  if (results_dirty_) {
    RunSearch();
  }
  for (auto& observer : observers_) {
    observer.OnSearchResultsChanged(this);
  }
}

void AstraTabSearchModel::NotifySearchModeChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchModeChanged(this, search_mode_);
  }
}

void AstraTabSearchModel::NotifyFilterChanged() {
  for (auto& observer : observers_) {
    observer.OnFilterChanged(this, filter_);
  }
}

void AstraTabSearchModel::NotifyQueryChanged() {
  for (auto& observer : observers_) {
    observer.OnQueryChanged(this, query_);
  }
}

void AstraTabSearchModel::NotifySelectedIndexChanged(size_t old_index,
                                                     size_t new_index) {
  for (auto& observer : observers_) {
    observer.OnSelectedIndexChanged(this, old_index, new_index);
  }
}

void AstraTabSearchModel::NotifyRecentSearchesChanged() {
  for (auto& observer : observers_) {
    observer.OnRecentSearchesChanged(this);
  }
}

}  // namespace astra
