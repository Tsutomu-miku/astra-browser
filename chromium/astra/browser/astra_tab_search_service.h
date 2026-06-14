// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_TAB_SEARCH_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_TAB_SEARCH_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace astra {

// =========================================================================
// AstraTabSearchCategory
// =========================================================================
//
// Categories of search results.  Each category maps to a different data
// source that the search service projects from Chromium or Astra.
//
// Chromium subsystems projected:
//   - kTab:          TabStripModel + AstraTabFeatures
//   - kBookmark:     BookmarkModel
//   - kHistory:      HistoryService
//   - kReadingList:  ReadingListModel
//
// Astra subsystems projected:
//   - kNote:         AstraNoteService
// =========================================================================

enum class AstraTabSearchCategory {
  kAll,          // Search across all categories.
  kTab,          // Open tabs (TabStripModel).
  kBookmark,     // Bookmarks (BookmarkModel).
  kHistory,      // Browsing history (HistoryService).
  kReadingList,  // Reading list items (ReadingListModel).
  kNote,         // Astra notes (AstraNoteService).
};

// =========================================================================
// AstraTabSearchResult
// =========================================================================
//
// A single search result from the unified tab search.
//
// Each result carries enough metadata for UI presentation (title, URL,
// category, relevance score) but does NOT own the underlying data — the
// source data lives in Chromium or Astra services and is projected here.
//
// The |source_data| pointer is a type-erased pointer to the original
// Chromium/Astra object.  Callers can cast it based on |category|:
//   - kTab:         content::WebContents* (or TabStripModel index proxy)
//   - kBookmark:    bookmarks::BookmarkNode*
//   - kHistory:     history::URLRow* (stubbed — see implementation)
//   - kReadingList: ReadingListEntry*
//   - kNote:        AstraNote*
//
// Truth model: this struct is a projection — all source data is owned by
// Chromium or Astra services, not by this search service.
// =========================================================================

struct AstraTabSearchResult {
  // Display title of the result.
  std::string title;

  // URL of the result (empty for notes without a URL).
  GURL url;

  // Which category / data source this result came from.
  AstraTabSearchCategory category = AstraTabSearchCategory::kTab;

  // Relevance score, higher = more relevant.
  // Scoring is based on match position, match type (title vs URL),
  // exact vs partial match, and recency.
  double score = 0.0;

  // Type-erased pointer to the source data.
  // Owned by the originating Chromium/Astra service — do not delete.
  // May be null for stubs or when source data is not available.
  void* source_data = nullptr;

  // Optional: workspace ID (for tab results with AstraTabFeatures).
  std::string workspace_id;

  // Optional: folder path (for bookmark results).
  std::string folder_path;

  // Optional: whether the reading list entry has been read.
  bool is_read = false;

  // Optional: note preview text (first N characters of note content).
  std::string note_preview;
};

// =========================================================================
// AstraTabSearchServiceObserver
// =========================================================================
//
// Observer interface for UI layers (tab search popup, omnibox, etc.) to
// react when underlying searchable data changes.
//
// UI must never be the source of truth — AstraTabSearchService projects
// from Chromium and Astra services, which are the truth sources.
// =========================================================================

class AstraTabSearchServiceObserver : public base::CheckedObserver {
 public:
  // Called when any underlying data source changes such that search
  // results may be different.  Observers should re-run their query
  // or at least invalidate cached results.
  virtual void OnSearchResultChanged() {}

 protected:
  ~AstraTabSearchServiceObserver() override = default;
};

// =========================================================================
// AstraTabSearchService
// =========================================================================
//
// Profile-scoped keyed service that provides unified fuzzy search across
// multiple Chromium and Astra data sources.
//
// This service PROJECTS state — it never owns tabs, bookmarks, history,
// reading list entries, or notes.  All source data lives in Chromium or
// Astra services, and this service aggregates and scores it for search.
//
// Data sources (all projected, never owned):
//   1. Open tabs       — TabStripModel + AstraTabFeatures
//   2. Bookmarks       — BookmarkModel (chromium)
//   3. History         — HistoryService (chromium)
//   4. Reading list    — ReadingListModel (chromium)
//   5. Notes           — AstraNoteService (astra)
//
// Search features:
//   - Fuzzy / substring matching on title and URL (case-insensitive)
//   - Category filtering (tabs, bookmarks, history, reading_list, notes, all)
//   - Result ranking by relevance score
//   - Recent search queries (persisted via PrefService, last 10)
//
// Persistence:
//   - Recent search queries persist via PrefService
//   - All other data is projected live from source services
//
// Chromium subsystems reused:
//   - Profile / ProfileKeyedServiceFactory
//   - PrefService (for recent queries)
//   - TabStripModel (for tab data, via projection)
//   - BookmarkModel (for bookmark data, via projection)
//   - HistoryService (for history data, via projection)
//   - ReadingListModel (for reading list data, via projection)
//
// Chromium patch points:
//   - Profile keyed service registration: chrome/browser/profiles/
//   - Tab observation: TabStripModelObserver
//   - Bookmark observation: BookmarkModelObserver
//   - History observation: HistoryServiceObserver
//   - Reading list observation: ReadingListModelObserver
//
// TODO(astra): Wire up observers for each data source so that
//   OnSearchResultChanged fires when underlying data changes.
//   Currently the service reads data on demand for each Search() call.
// =========================================================================

class AstraTabSearchService final : public KeyedService {
 public:
  // -- Pref keys (public for factory registration) -----------------------

  // List of recent search query strings (most recent first).
  static constexpr const char kPrefRecentQueries[] =
      "astra.tab_search.recent_queries";

  // Maximum number of recent queries to keep.
  static constexpr size_t kMaxRecentQueries = 10;

  // -----------------------------------------------------------------------
  // Construction / destruction
  // -----------------------------------------------------------------------

  explicit AstraTabSearchService(Profile* profile);
  AstraTabSearchService(const AstraTabSearchService&) = delete;
  AstraTabSearchService& operator=(const AstraTabSearchService&) = delete;
  ~AstraTabSearchService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraTabSearchServiceObserver* observer);
  void RemoveObserver(AstraTabSearchServiceObserver* observer);

  // -- Search API --------------------------------------------------------

  // Search across all categories (or a filtered category).
  // Returns results sorted by relevance score (highest first).
  // An empty query returns all items in the selected category(s).
  std::vector<AstraTabSearchResult> Search(
      const std::string& query,
      AstraTabSearchCategory category_filter =
          AstraTabSearchCategory::kAll) const;

  // Convenience: search only tabs.
  std::vector<AstraTabSearchResult> SearchTabs(
      const std::string& query) const;

  // Convenience: search only bookmarks.
  std::vector<AstraTabSearchResult> SearchBookmarks(
      const std::string& query) const;

  // Convenience: search only history.
  std::vector<AstraTabSearchResult> SearchHistory(
      const std::string& query) const;

  // Convenience: search only reading list.
  std::vector<AstraTabSearchResult> SearchReadingList(
      const std::string& query) const;

  // Convenience: search only notes.
  std::vector<AstraTabSearchResult> SearchNotes(
      const std::string& query) const;

  // -- Recent queries ----------------------------------------------------

  // Returns recent search queries (most recent first), up to
  // kMaxRecentQueries entries.
  std::vector<std::string> GetRecentQueries() const;

  // Adds a query to the recent queries list (at the front).
  // If the query already exists, it moves to the front (no duplicate).
  // Empty strings are ignored.
  void AddToRecentQueries(const std::string& query);

  // Clears all recent search queries.
  void ClearRecentQueries();

  // -- Incognito compatibility -------------------------------------------

  // Returns true if this service instance is associated with an incognito
  // (off-the-record) profile.
  bool IsIncognito() const;

  // -- Test helpers -------------------------------------------------------
  //
  // These methods allow tests to inject mock data without requiring
  // real Chromium services.  They are for test use only.

  // Sets test tab items.  Replaces any existing test items.
  void SetTabItemsForTesting(std::vector<AstraTabSearchResult> items);

  // Sets test bookmark items.
  void SetBookmarkItemsForTesting(std::vector<AstraTabSearchResult> items);

  // Sets test history items.
  void SetHistoryItemsForTesting(std::vector<AstraTabSearchResult> items);

  // Sets test reading list items.
  void SetReadingListItemsForTesting(std::vector<AstraTabSearchResult> items);

  // Sets test note items.
  void SetNoteItemsForTesting(std::vector<AstraTabSearchResult> items);

  // Notifies observers that search results have changed.
  // For test use only — production code notifies via data source observers.
  void NotifySearchResultChangedForTesting();

 private:
  // -- Data source collection (internal) ---------------------------------

  // Collects all open tabs from TabStripModel + AstraTabFeatures.
  // TODO(astra): Implement with real TabStripModel iteration.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  //   Patch point: iterate BrowserList for all windows belonging to this
  //   profile, then iterate each Browser's TabStripModel.
  std::vector<AstraTabSearchResult> CollectTabs() const;

  // Collects all bookmarks from BookmarkModel.
  // TODO(astra): Implement with real BookmarkModel traversal.
  //   Chromium owner: BookmarkModel (components/bookmarks/browser/bookmark_model.h)
  //   Patch point: BookmarkModelObserver for change notifications.
  std::vector<AstraTabSearchResult> CollectBookmarks() const;

  // Collects history entries from HistoryService.
  // TODO(astra): Implement with real HistoryService query.
  //   Chromium owner: HistoryService (components/history/core/browser/history_service.h)
  //   Patch point: HistoryServiceObserver for change notifications.
  std::vector<AstraTabSearchResult> CollectHistory() const;

  // Collects reading list entries from ReadingListModel.
  // TODO(astra): Implement with real ReadingListModel.
  //   Chromium owner: ReadingListModel (components/reading_list/core/reading_list_model.h)
  //   Patch point: ReadingListModelObserver for change notifications.
  std::vector<AstraTabSearchResult> CollectReadingList() const;

  // Collects notes from AstraNoteService.
  // TODO(astra): Implement with real AstraNoteService query.
  //   Astra owner: AstraNoteService (astra/browser/astra_note_service.h)
  std::vector<AstraTabSearchResult> CollectNotes() const;

  // -- Scoring helpers ---------------------------------------------------

  // Scores a single item against a query.
  // Returns a relevance score (higher = more relevant).
  //
  // Scoring rules:
  //   - Title matches score higher than URL matches
  //   - Exact (full-string) matches score higher than partial
  //   - Case-insensitive matching
  //   - Matches at the start of the string score higher
  //   - Empty query returns 0 (but all items are included)
  static double ScoreItem(const std::string& title,
                          const std::string& url_spec,
                          const std::string& query);

  // Sorts results by score descending (highest score first).
  static void SortResultsByScore(std::vector<AstraTabSearchResult>* results);

  // -- Pref helpers ------------------------------------------------------

  // Loads recent queries from PrefService.
  void LoadRecentQueriesFromPrefs();

  // Saves recent queries to PrefService.
  void SaveRecentQueriesToPrefs() const;

  // -- Member variables --------------------------------------------------

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraTabSearchServiceObserver> observers_;

  // Cached recent queries (most recent first).
  // Source of truth is PrefService; this is the in-memory copy.
  std::vector<std::string> recent_queries_;

  // -- Test data (only used when SetXxxItemsForTesting is called) --------
  //
  // Test-injected items.  When non-empty, the corresponding CollectXxx()
  // method returns these instead of querying real Chromium/Astra services.
  // Only for unit test use.

  std::vector<AstraTabSearchResult> test_tab_items_;
  std::vector<AstraTabSearchResult> test_bookmark_items_;
  std::vector<AstraTabSearchResult> test_history_items_;
  std::vector<AstraTabSearchResult> test_reading_list_items_;
  std::vector<AstraTabSearchResult> test_note_items_;
};

// =========================================================================
// AstraTabSearchServiceFactory
// =========================================================================
//
// Factory for AstraTabSearchService.
//
// Incognito behavior: kOwnInstance — each incognito profile gets its own
// tab search service instance.  Search state (recent queries, active
// filters) should be per-browsing-context and not leak between regular
// and incognito windows.
//
// Guest: kOwnInstance — guest sessions get their own ephemeral instance.
//
// System: kNone — system profile has no user-visible search.
// =========================================================================

class AstraTabSearchServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraTabSearchService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraTabSearchService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraTabSearchServiceFactory* GetInstance();

  // Registers tab-search-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into Chromium's profile pref registration
  //   pipeline so prefs are registered at profile creation time.
  //   Chromium patch point: chrome/browser/prefs/browser_prefs.cc or
  //   chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraTabSearchServiceFactory>;

  AstraTabSearchServiceFactory();
  ~AstraTabSearchServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_SEARCH_SERVICE_H_
