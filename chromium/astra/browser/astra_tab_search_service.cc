// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_search_service.h"

#include <algorithm>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Scoring weights
// ---------------------------------------------------------------------------

// Score multiplier for title matches (vs URL matches).
constexpr double kTitleMatchMultiplier = 2.0;

// Score multiplier for exact matches (vs substring matches).
constexpr double kExactMatchMultiplier = 3.0;

// Score multiplier for prefix matches (query at start of string).
constexpr double kPrefixMatchMultiplier = 1.5;

// Base score for any match (ensures partial matches have positive score).
constexpr double kBaseMatchScore = 1.0;

// Bonus for each additional character matched beyond the query length.
// This helps longer matches (closer to full string) score higher.
constexpr double kMatchLengthBonus = 0.0;  // Currently disabled.

// Returns a case-insensitive substring match position (or npos).
size_t CaseInsensitiveFind(const std::string& text,
                           const std::string& query) {
  if (query.empty()) {
    return std::string::npos;
  }
  std::string text_lower = base::ToLowerASCII(text);
  std::string query_lower = base::ToLowerASCII(query);
  return text_lower.find(query_lower);
}

// Returns true if |text| contains |query| case-insensitively.
bool CaseInsensitiveContains(const std::string& text,
                             const std::string& query) {
  return CaseInsensitiveFind(text, query) != std::string::npos;
}

// Returns true if |text| equals |query| case-insensitively.
bool CaseInsensitiveEquals(const std::string& text,
                           const std::string& query) {
  return base::EqualsCaseInsensitiveASCII(text, query);
}

}  // namespace

// ===========================================================================
// AstraTabSearchService
// ===========================================================================

AstraTabSearchService::AstraTabSearchService(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  LoadRecentQueriesFromPrefs();
}

AstraTabSearchService::~AstraTabSearchService() = default;

void AstraTabSearchService::Shutdown() {
  // KeyedService shutdown: clear all observer references and drop the
  // profile pointer before the profile goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraTabSearchService::AddObserver(
    AstraTabSearchServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabSearchService::RemoveObserver(
    AstraTabSearchServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Search API --------------------------------------------------------------

std::vector<AstraTabSearchResult> AstraTabSearchService::Search(
    const std::string& query,
    AstraTabSearchCategory category_filter) const {
  std::vector<AstraTabSearchResult> all_results;

  // Trim whitespace from the query.
  std::string trimmed_query = query;
  base::TrimWhitespaceASCII(trimmed_query, base::TRIM_ALL, &trimmed_query);

  // Collect items from each relevant source.
  if (category_filter == AstraTabSearchCategory::kAll ||
      category_filter == AstraTabSearchCategory::kTab) {
    auto tab_results = CollectTabs();
    all_results.insert(all_results.end(),
                       std::make_move_iterator(tab_results.begin()),
                       std::make_move_iterator(tab_results.end()));
  }

  if (category_filter == AstraTabSearchCategory::kAll ||
      category_filter == AstraTabSearchCategory::kBookmark) {
    auto bookmark_results = CollectBookmarks();
    all_results.insert(all_results.end(),
                       std::make_move_iterator(bookmark_results.begin()),
                       std::make_move_iterator(bookmark_results.end()));
  }

  if (category_filter == AstraTabSearchCategory::kAll ||
      category_filter == AstraTabSearchCategory::kHistory) {
    auto history_results = CollectHistory();
    all_results.insert(all_results.end(),
                       std::make_move_iterator(history_results.begin()),
                       std::make_move_iterator(history_results.end()));
  }

  if (category_filter == AstraTabSearchCategory::kAll ||
      category_filter == AstraTabSearchCategory::kReadingList) {
    auto reading_results = CollectReadingList();
    all_results.insert(all_results.end(),
                       std::make_move_iterator(reading_results.begin()),
                       std::make_move_iterator(reading_results.end()));
  }

  if (category_filter == AstraTabSearchCategory::kAll ||
      category_filter == AstraTabSearchCategory::kNote) {
    auto note_results = CollectNotes();
    all_results.insert(all_results.end(),
                       std::make_move_iterator(note_results.begin()),
                       std::make_move_iterator(note_results.end()));
  }

  // If query is empty, return all results (sorted by category/recency).
  if (trimmed_query.empty()) {
    // Sort by category order, then by title for stability.
    base::ranges::sort(all_results,
                       [](const AstraTabSearchResult& a,
                          const AstraTabSearchResult& b) {
                         if (a.category != b.category) {
                           return static_cast<int>(a.category) <
                                  static_cast<int>(b.category);
                         }
                         return a.title < b.title;
                       });
    return all_results;
  }

  // Score and filter results.
  std::vector<AstraTabSearchResult> matched_results;
  for (auto& result : all_results) {
    double score = ScoreItem(result.title, result.url.spec(), trimmed_query);
    if (score > 0.0) {
      result.score = score;
      matched_results.push_back(std::move(result));
    }
  }

  // Sort by score (descending).
  SortResultsByScore(&matched_results);

  return matched_results;
}

std::vector<AstraTabSearchResult> AstraTabSearchService::SearchTabs(
    const std::string& query) const {
  return Search(query, AstraTabSearchCategory::kTab);
}

std::vector<AstraTabSearchResult> AstraTabSearchService::SearchBookmarks(
    const std::string& query) const {
  return Search(query, AstraTabSearchCategory::kBookmark);
}

std::vector<AstraTabSearchResult> AstraTabSearchService::SearchHistory(
    const std::string& query) const {
  return Search(query, AstraTabSearchCategory::kHistory);
}

std::vector<AstraTabSearchResult> AstraTabSearchService::SearchReadingList(
    const std::string& query) const {
  return Search(query, AstraTabSearchCategory::kReadingList);
}

std::vector<AstraTabSearchResult> AstraTabSearchService::SearchNotes(
    const std::string& query) const {
  return Search(query, AstraTabSearchCategory::kNote);
}

// -- Recent queries ----------------------------------------------------------

std::vector<std::string> AstraTabSearchService::GetRecentQueries() const {
  return recent_queries_;
}

void AstraTabSearchService::AddToRecentQueries(const std::string& query) {
  // Trim whitespace.
  std::string trimmed_query = query;
  base::TrimWhitespaceASCII(trimmed_query, base::TRIM_ALL, &trimmed_query);

  // Ignore empty queries.
  if (trimmed_query.empty()) {
    return;
  }

  // Remove if already present (to move it to the front).
  auto it = std::find(recent_queries_.begin(), recent_queries_.end(),
                      trimmed_query);
  if (it != recent_queries_.end()) {
    recent_queries_.erase(it);
  }

  // Add to the front.
  recent_queries_.insert(recent_queries_.begin(), trimmed_query);

  // Enforce size limit.
  if (recent_queries_.size() > kMaxRecentQueries) {
    recent_queries_.pop_back();
  }

  SaveRecentQueriesToPrefs();
}

void AstraTabSearchService::ClearRecentQueries() {
  recent_queries_.clear();
  SaveRecentQueriesToPrefs();
}

// -- Incognito compatibility -------------------------------------------------

bool AstraTabSearchService::IsIncognito() const {
  if (!profile_) {
    return false;
  }
  // In a full Chromium build: profile_->IsOffTheRecord()
  // For the overlay repo stub, check via profile name pattern.
  // TODO(astra): Use profile_->IsOffTheRecord() in the full Chromium build.
  //   Chromium owner: Profile::IsOffTheRecord().
  return false;
}

// -- Test helpers ------------------------------------------------------------

void AstraTabSearchService::SetTabItemsForTesting(
    std::vector<AstraTabSearchResult> items) {
  test_tab_items_ = std::move(items);
}

void AstraTabSearchService::SetBookmarkItemsForTesting(
    std::vector<AstraTabSearchResult> items) {
  test_bookmark_items_ = std::move(items);
}

void AstraTabSearchService::SetHistoryItemsForTesting(
    std::vector<AstraTabSearchResult> items) {
  test_history_items_ = std::move(items);
}

void AstraTabSearchService::SetReadingListItemsForTesting(
    std::vector<AstraTabSearchResult> items) {
  test_reading_list_items_ = std::move(items);
}

void AstraTabSearchService::SetNoteItemsForTesting(
    std::vector<AstraTabSearchResult> items) {
  test_note_items_ = std::move(items);
}

void AstraTabSearchService::NotifySearchResultChangedForTesting() {
  for (auto& observer : observers_) {
    observer.OnSearchResultChanged();
  }
}

// -- Data source collection (internal) ---------------------------------------

std::vector<AstraTabSearchResult> AstraTabSearchService::CollectTabs() const {
  // Return test-injected items if available.
  // This allows unit tests to exercise search logic without real Chromium
  // services.  Production builds should not use SetTabItemsForTesting.
  if (!test_tab_items_.empty()) {
    return test_tab_items_;
  }

  // TODO(astra): Implement real tab collection from TabStripModel.
  //   Chromium owner: BrowserList + TabStripModel
  //   Patch point: chrome/browser/ui/browser_list.h + tab_strip_model.h
  //
  //   For each tab we'd collect:
  //     - title: WebContents::GetTitle()
  //     - url: WebContents::GetLastCommittedURL()
  //     - workspace_id: AstraTabFeatures::FromWebContents(wc)->workspace_id()
  //     - tab_color: AstraTabFeatures::FromWebContents(wc)->tab_color()
  //     - source_data: the WebContents* itself
  //
  //   Stub for now — returns empty in production.
  return {};
}

std::vector<AstraTabSearchResult> AstraTabSearchService::CollectBookmarks()
    const {
  // Return test-injected items if available.
  if (!test_bookmark_items_.empty()) {
    return test_bookmark_items_;
  }

  // TODO(astra): Implement real bookmark collection from BookmarkModel.
  //   Chromium owner: BookmarkModel (components/bookmarks/browser/bookmark_model.h)
  //   Patch point: BookmarkModel::GetBookmarksBarNode() + recursive traversal.
  //
  //   Stub for now — returns empty in production.
  return {};
}

std::vector<AstraTabSearchResult> AstraTabSearchService::CollectHistory()
    const {
  // Return test-injected items if available.
  if (!test_history_items_.empty()) {
    return test_history_items_;
  }

  // TODO(astra): Implement real history query via HistoryService.
  //   Chromium owner: HistoryService (components/history/core/browser/history_service.h)
  //   Patch point: HistoryService::QueryHistory() or similar.
  //
  //   Stub for now — returns empty in production.
  return {};
}

std::vector<AstraTabSearchResult>
AstraTabSearchService::CollectReadingList() const {
  // Return test-injected items if available.
  if (!test_reading_list_items_.empty()) {
    return test_reading_list_items_;
  }

  // TODO(astra): Implement real reading list collection from ReadingListModel.
  //   Chromium owner: ReadingListModel (components/reading_list/core/reading_list_model.h)
  //
  //   Stub for now — returns empty in production.
  return {};
}

std::vector<AstraTabSearchResult> AstraTabSearchService::CollectNotes() const {
  // Return test-injected items if available.
  if (!test_note_items_.empty()) {
    return test_note_items_;
  }

  // TODO(astra): Implement real note collection from AstraNoteService.
  //   Astra owner: AstraNoteService (astra/browser/astra_note_service.h)
  //
  //   Stub for now — returns empty in production.
  return {};
}

// -- Scoring helpers ---------------------------------------------------------

// static
double AstraTabSearchService::ScoreItem(const std::string& title,
                                        const std::string& url_spec,
                                        const std::string& query) {
  DCHECK(!query.empty());

  double total_score = 0.0;

  // -- Title matching --
  if (!title.empty()) {
    // Exact match (case-insensitive).
    if (CaseInsensitiveEquals(title, query)) {
      total_score += kBaseMatchScore * kTitleMatchMultiplier *
                     kExactMatchMultiplier;
    } else {
      size_t pos = CaseInsensitiveFind(title, query);
      if (pos != std::string::npos) {
        double title_score = kBaseMatchScore * kTitleMatchMultiplier;
        // Prefix bonus.
        if (pos == 0) {
          title_score *= kPrefixMatchMultiplier;
        }
        // Match length bonus (how much of the title is matched).
        if (kMatchLengthBonus > 0 && !title.empty()) {
          double coverage = static_cast<double>(query.length()) /
                            static_cast<double>(title.length());
          title_score += coverage * kMatchLengthBonus;
        }
        total_score += title_score;
      }
    }
  }

  // -- URL matching --
  if (!url_spec.empty()) {
    if (CaseInsensitiveEquals(url_spec, query)) {
      total_score += kBaseMatchScore * kExactMatchMultiplier;
    } else {
      size_t pos = CaseInsensitiveFind(url_spec, query);
      if (pos != std::string::npos) {
        double url_score = kBaseMatchScore;
        // Prefix bonus for URL.
        if (pos == 0) {
          url_score *= kPrefixMatchMultiplier;
        }
        total_score += url_score;
      }
    }
  }

  return total_score;
}

// static
void AstraTabSearchService::SortResultsByScore(
    std::vector<AstraTabSearchResult>* results) {
  DCHECK(results);
  base::ranges::sort(*results,
                     [](const AstraTabSearchResult& a,
                        const AstraTabSearchResult& b) {
                       if (a.score != b.score) {
                         return a.score > b.score;  // Higher score first.
                       }
                       // Tie-breaker: category order.
                       if (a.category != b.category) {
                         return static_cast<int>(a.category) <
                                static_cast<int>(b.category);
                       }
                       // Second tie-breaker: title alphabetical.
                       return a.title < b.title;
                     });
}

// -- Pref helpers ------------------------------------------------------------

void AstraTabSearchService::LoadRecentQueriesFromPrefs() {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Use PrefService::GetList in the full Chromium build.
  //   For the overlay repo stub, we start with an empty list.
  //   Chromium component: PrefService + PrefRegistry.
  //
  //   The pref format is a list of strings (most recent first).
  //
  //   Code pattern:
  //     const base::Value::List& list = prefs->GetList(kPrefRecentQueries);
  //     for (const auto& item : list) {
  //       if (item.is_string()) {
  //         recent_queries_.push_back(item.GetString());
  //       }
  //     }
}

void AstraTabSearchService::SaveRecentQueriesToPrefs() const {
  if (!profile_) {
    return;
  }
  // TODO(astra): Persist recent queries to PrefService in the full
  //   Chromium build.
  //   Chromium component: PrefService + PrefRegistry.
  //   Patch point: astra_prefs.h + PrefService.
  //
  //   Should be called after every change to recent_queries_.
  //   PrefService handles deferred disk writes internally.
  //
  //   Code pattern:
  //     base::Value::List list;
  //     for (const auto& query : recent_queries_) {
  //       list.Append(query);
  //     }
  //     prefs->Set(kPrefRecentQueries, base::Value(std::move(list)));
}

// ===========================================================================
// AstraTabSearchServiceFactory
// ===========================================================================

// static
AstraTabSearchService* AstraTabSearchServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraTabSearchService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraTabSearchServiceFactory*
AstraTabSearchServiceFactory::GetInstance() {
  static base::NoDestructor<AstraTabSearchServiceFactory> instance;
  return instance.get();
}

// static
void AstraTabSearchServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Register recent queries pref: list of query strings.
  registry->RegisterListPref(kPrefRecentQueries);
}

AstraTabSearchServiceFactory::AstraTabSearchServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraTabSearchService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Tab search state (recent queries, active filters) is
              // per-browsing-context and should not leak between regular
              // and incognito windows.  An incognito user should not see
              // their regular-profile recent queries, and vice versa.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to share with).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user-visible search, no instance.
              .Build()) {
  // TODO(astra): Declare dependencies on other ProfileKeyedServices.
  //   This service depends on (i.e., reads from):
  //     - AstraNoteService (for note search)
  //     - (indirectly) BookmarkModel (owned by Profile, not a keyed service)
  //     - (indirectly) HistoryService
  //     - (indirectly) ReadingListModel
  //
  //   Use DependsOn() to declare dependencies on other
  //   ProfileKeyedServiceFactory instances.
  //
  //   Chromium pattern: call DependsOn() in the factory constructor.
}

AstraTabSearchServiceFactory::~AstraTabSearchServiceFactory() = default;

std::unique_ptr<KeyedService>
AstraTabSearchServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);
  return std::make_unique<AstraTabSearchService>(profile);
}

}  // namespace astra
