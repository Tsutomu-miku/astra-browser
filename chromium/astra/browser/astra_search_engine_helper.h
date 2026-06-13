#ifndef ASTRA_BROWSER_ASTRA_SEARCH_ENGINE_HELPER_H_
#define ASTRA_BROWSER_ASTRA_SEARCH_ENGINE_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"

class PrefService;
class Profile;
class TemplateURL;

namespace astra {

// =========================================================================
// AstraSearchEngineInfo — projected search engine data
// =========================================================================
//
// A lightweight struct representing a single search engine entry, projected
// from Chromium's TemplateURL.  This struct is a pure projection — it never
// mutates search engine state.  The truth source is always TemplateURLService.
//
// Chromium owner: TemplateURL (components/search_engines/template_url.h)
// Chromium service: TemplateURLService
//   (components/search_engines/template_url_service.h)
//
// TODO(astra): Use TemplateURL directly once the overlay is building against
// a full Chromium checkout and the views layer can depend on search_engines.
// This struct mirrors the fields we need for settings presentation.
struct AstraSearchEngineInfo {
  // Stable identifier for the search engine.
  // Corresponds to TemplateURL::id() or the GUID.
  std::string id;

  // Human-readable name (e.g. "Google", "Bing", "DuckDuckGo").
  // Corresponds to TemplateURL::short_name().
  std::u16string name;

  // Keyword / shortcut (e.g. "google.com", "!g").
  // Corresponds to TemplateURL::keyword().
  std::u16string keyword;

  // The search URL template (e.g. "https://www.google.com/search?q={searchTerms}").
  // Corresponds to TemplateURL::url().
  std::string url;

  // Whether this is the default search engine.
  bool is_default = false;

  // Whether this engine is a prepopulated / built-in engine.
  // Built-in engines cannot be fully deleted — only their shortcuts can be
  // edited, and they can be set as default.
  bool is_builtin = false;

  // Whether this engine can be edited (name, keyword, URL).
  // Built-in engines and policy-controlled engines may be read-only.
  bool is_editable = false;
};

// =========================================================================
// AstraSearchEngineHelper — search engine projection helper
// =========================================================================
//
// Helper that wraps Chromium's TemplateURLService for the Astra settings
// search engine section.  It provides a clean projection API for reading
// search engine state, performing search engine operations, and managing
// Astra-specific search presentation preferences.
//
// This is a thin projection layer — the truth source for search engine
// data is always TemplateURLService.  The helper never caches engine data;
// every call reads from the underlying service.
//
// Astra-specific presentation preferences (show default engine, recent
// queries, suggestions toggle) are persisted via the profile's PrefService.
// These are purely presentation concerns and never affect the underlying
// search engine data managed by Chromium.
//
// Why a helper instead of using TemplateURLService directly from the UI?
//   1. Encapsulates the service lookup (factory + profile).
//   2. Adapts the TemplateURLService API to Astra settings needs (e.g.
//      projecting to AstraSearchEngineInfo, separating default from others).
//   3. Gives the UI layer a stable Astra-owned interface that won't
//      churn as Chromium's API evolves.
//   4. Follows the same pattern as other Astra helpers — UI talks to
//      Astra-layer helpers, not raw Chromium APIs.
//
// Chromium owner: TemplateURLService
//   (components/search_engines/template_url_service.h)
// Chromium factory: TemplateURLServiceFactory
//   (chrome/browser/search_engines/template_url_service_factory.h)
//
// TODO(astra): Add TemplateURLServiceObserver support for real-time updates.
// The settings page currently reads state on demand; with an observer, it
// could reactively update whenever search engines change (e.g. from Chrome
// settings, extensions, or sync).
// Chromium observer: TemplateURLServiceObserver
//   (components/search_engines/template_url_service_observer.h)
class AstraSearchEngineHelper {
 public:
  // =======================================================================
  // Observer interface for search engine change notifications
  // =======================================================================
  //
  // The UI layer implements this observer to refresh the search settings
  // view when search engine state or presentation settings change.
  // All observer methods have empty default implementations so observers
  // only need to override the events they care about.
  //
  // The browser layer never depends on Views code.
  class Observer : public base::CheckedObserver {
   public:
    // Called when the default search engine changes.
    // The UI should re-read the default engine info and update the
    // prominent default engine display.
    virtual void OnDefaultSearchEngineChanged() {}

    // Called when a new search engine is added.
    // |engine| contains the projected info for the newly added engine.
    virtual void OnSearchEngineAdded(const AstraSearchEngineInfo& engine) {}

    // Called when a search engine is removed.
    // |engine_id| is the ID of the engine that was removed.
    virtual void OnSearchEngineRemoved(const std::string& engine_id) {}

    // Called when the set of search engines has changed in any way
    // (added, removed, renamed, default changed, etc.).
    // This is a catch-all notification — use OnDefaultSearchEngineChanged,
    // OnSearchEngineAdded, or OnSearchEngineRemoved for granular updates.
    // The UI should re-read the list via GetSearchEngines() and rebuild
    // its projection.
    virtual void OnSearchEnginesChanged() {}

    // Called when search presentation settings change (e.g. show default
    // engine, show other engines, suggestions enabled).
    // The UI should refresh its search section presentation.
    virtual void OnSearchPresentationChanged() {}

    // Called when the recent search queries list changes.
    // The UI should refresh the recent queries display.
    virtual void OnRecentQueriesChanged() {}

   protected:
    ~Observer() override = default;
  };

  AstraSearchEngineHelper() = delete;
  AstraSearchEngineHelper(const AstraSearchEngineHelper&) = delete;
  AstraSearchEngineHelper& operator=(const AstraSearchEngineHelper&) = delete;

  // -- Search engine queries ---------------------------------------------

  // Returns the default search engine for |profile|, or nullptr if no
  // default is set.
  //
  // Chromium method: TemplateURLService::GetDefaultSearchProvider()
  static const TemplateURL* GetDefaultSearchEngine(Profile* profile);

  // Returns the projected default search engine info for |profile|.
  // Returns an empty AstraSearchEngineInfo with is_default = true if no
  // default is found.
  //
  // TODO(astra): Return std::optional<AstraSearchEngineInfo> or a bool
  // success parameter once we have C++17/20 support available in our
  // overlay build config.  For now, check if id is empty to detect
  // "not found".
  static AstraSearchEngineInfo GetDefaultSearchEngineInfo(Profile* profile);

  // Returns a list of all search engines for |profile|.
  // Includes both built-in (prepopulated) engines and user-added engines.
  // Engines are sorted with the default first, then alphabetically by name.
  //
  // Returns an empty vector if TemplateURLService is not available.
  //
  // Chromium method: TemplateURLService::GetTemplateURLs()
  static std::vector<AstraSearchEngineInfo> GetSearchEngines(Profile* profile);

  // Returns a list of all search engines (alias for GetSearchEngines).
  // Provided for API consistency with other query methods.
  static std::vector<AstraSearchEngineInfo> GetSearchEnginesList(
      Profile* profile);

  // Returns a list of only the non-default search engines.
  // Useful for showing "other search engines" section separately.
  static std::vector<AstraSearchEngineInfo> GetNonDefaultSearchEngines(
      Profile* profile);

  // Returns the number of search engines.
  static size_t GetSearchEngineCount(Profile* profile);

  // Returns true if the engine with |engine_id| is the default search engine.
  // Returns false if the engine is not found or is not the default.
  static bool IsDefaultSearchEngine(Profile* profile,
                                    const std::string& engine_id);

  // Returns the search engine info for the engine with the given |name|.
  // Returns an empty AstraSearchEngineInfo (empty id) if not found.
  // Name comparison is case-insensitive.
  static AstraSearchEngineInfo GetSearchEngineByName(
      Profile* profile,
      const std::u16string& name);

  // Returns the search engine info for the engine with the given |keyword|.
  // Returns an empty AstraSearchEngineInfo (empty id) if not found.
  // Keyword comparison is case-insensitive.
  //
  // This is useful for quick keyword search: the user types a keyword
  // followed by a query, and we look up which engine to use.
  static AstraSearchEngineInfo GetSearchEngineByKeyword(
      Profile* profile,
      const std::u16string& keyword);

  // -- Search engine operations ------------------------------------------

  // Sets |template_url| as the default search engine for |profile|.
  // Returns true if successful.
  //
  // Chromium method: TemplateURLService::SetUserSelectedDefaultSearchProvider()
  // or TemplateURLService::SetDefaultSearchProvider().
  //
  // TODO(astra): Use the proper TemplateURLService::SetDefaultSearchProvider
  // method once we are building against a full Chromium checkout.  In the
  // overlay, this is a stub that returns false.
  static bool SetDefaultSearchEngine(Profile* profile,
                                     const TemplateURL* template_url);

  // Sets the search engine with |engine_id| as the default.
  // Returns true if successful.
  //
  // This is a convenience wrapper that finds the TemplateURL by id and
  // then calls SetDefaultSearchEngine.
  static bool SetDefaultSearchEngineById(Profile* profile,
                                         const std::string& engine_id);

  // Adds a new search engine with the given name, keyword, and URL.
  // Returns the new TemplateURL* on success, or nullptr on failure.
  //
  // |url| should be a valid search URL template with {searchTerms}
  // placeholder, e.g. "https://example.com/search?q={searchTerms}".
  //
  // Chromium method: TemplateURLService::Add() with a TemplateURLData.
  //
  // TODO(astra): Implement with TemplateURLService::Add() once we have
  // full Chromium headers.  In the overlay, returns nullptr as a stub.
  static const TemplateURL* AddSearchEngine(Profile* profile,
                                            const std::u16string& name,
                                            const std::u16string& keyword,
                                            const std::string& url);

  // Removes (deletes) the given search engine.
  // Returns true if successful.
  //
  // Built-in (prepopulated) engines may not be fully removable — they may
  // just be hidden or reset.  Policy-controlled engines cannot be removed.
  //
  // Chromium method: TemplateURLService::Remove()
  //
  // TODO(astra): Implement with TemplateURLService::Remove() once we have
  // full Chromium headers.  In the overlay, returns false as a stub.
  static bool RemoveSearchEngine(Profile* profile,
                                 const TemplateURL* template_url);

  // Removes the search engine with |engine_id|.
  // Returns true if successful.
  static bool RemoveSearchEngineById(Profile* profile,
                                     const std::string& engine_id);

  // -- Chrome settings delegation ----------------------------------------

  // Opens Chrome's full search engine settings page (chrome://settings/searchEngines).
  //
  // Since Astra is an overlay and cannot fully reimplement search engine
  // management, we delegate advanced operations (add/edit/delete with full
  // validation) to Chrome's settings page.
  //
  // TODO(astra): Implement using NavigateParams or chrome::SettingsWindowManager
  // to open the search engine settings subpage.
  // Chromium owner: chrome/browser/ui/settings_window_manager.h
  // Chromium WebUI: chrome://settings/searchEngines
  //   (chrome/browser/resources/settings/search_engines_page/)
  static void OpenChromeSearchSettings(Profile* profile);

  // -- Presentation settings ---------------------------------------------

  // Returns whether the default search engine is shown as a prominent
  // shortcut in the Astra new tab page or sidebar search section.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This is a presentation preference — it never affects which search
  // engines exist or which is the default.
  static bool GetShowDefaultEngine(Profile* profile);

  // Sets whether the default search engine is shown prominently.
  // Fires OnSearchPresentationChanged observer notification.
  static void SetShowDefaultEngine(Profile* profile, bool show);

  // Returns whether the "Other search engines" list is shown in the Astra
  // sidebar search section.
  //
  // Persisted via PrefService.  Default: false.
  //
  // This is a presentation preference — it never affects which search
  // engines exist or which is the default.
  static bool GetShowOtherEngines(Profile* profile);

  // Sets whether the "Other search engines" list is shown.
  // Fires OnSearchPresentationChanged observer notification.
  static void SetShowOtherEngines(Profile* profile, bool show);

  // Toggles the "Other search engines" visibility.
  // Returns the new state.
  static bool ToggleShowOtherEngines(Profile* profile);

  // Returns whether search suggestions are enabled in the Astra search UI.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This controls whether Astra surfaces show search suggestions (e.g.
  // in the sidebar search box or command palette).  The actual suggestion
  // data comes from Chromium's search suggest service.
  //
  // Chromium owner: SearchSuggestService / Omnibox suggest providers
  //   (components/omnibox/browser/)
  static bool GetSuggestionsEnabled(Profile* profile);

  // Sets whether search suggestions are enabled.
  // Fires OnSearchPresentationChanged observer notification.
  static void SetSuggestionsEnabled(Profile* profile, bool enabled);

  // Toggles search suggestions.  Returns the new state.
  static bool ToggleSuggestionsEnabled(Profile* profile);

  // -- Recent search queries ---------------------------------------------

  // Returns the list of recent search queries, most recent first.
  //
  // Persisted via PrefService.  Default: empty list.
  //
  // Recent queries are an Astra-level presentation feature — they let
  // the user quickly re-run a recent search.  They are not the same as
  // Chromium's search history (which is managed by the HistoryService).
  //
  // Chromium owner: HistoryService (components/history/)
  static std::vector<std::string> GetRecentQueries(Profile* profile);

  // Adds a query to the recent queries list.
  // If the query already exists, it is moved to the front (most recent).
  // The list is capped at GetMaxRecentQueries() entries.
  // Fires OnRecentQueriesChanged observer notification.
  static void AddRecentQuery(Profile* profile, const std::string& query);

  // Clears all recent search queries.
  // Fires OnRecentQueriesChanged observer notification.
  static void ClearRecentQueries(Profile* profile);

  // Returns the maximum number of recent queries to keep.
  //
  // Persisted via PrefService.  Default: 10.
  static int GetMaxRecentQueries(Profile* profile);

  // Sets the maximum number of recent queries to keep.
  // If the current list exceeds the new maximum, it is truncated.
  // Fires OnRecentQueriesChanged observer notification if truncation occurs.
  static void SetMaxRecentQueries(Profile* profile, int max_count);

  // -- Search shortcut helpers -------------------------------------------

  // Builds a search URL for the given |keyword| and |query|.
  // Looks up the engine by keyword and substitutes the query into its
  // URL template.
  //
  // Returns an empty string if no engine matches the keyword or if the
  // TemplateURLService is not available.
  //
  // This is useful for quick keyword search in the command palette or
  // omnibox-style search input.
  //
  // TODO(astra): Implement with TemplateURL::ReplaceSearchTerms once we
  // have full Chromium headers.  In the overlay, returns empty string.
  static std::string BuildSearchUrl(Profile* profile,
                                    const std::u16string& keyword,
                                    const std::u16string& query);

  // -- Observers ---------------------------------------------------------

  // Registers an observer for search engine change notifications.
  //
  // TODO(astra): Wire up to TemplateURLServiceObserver once we have
  // live observation support.  Currently observers are registered but
  // never notified automatically because the observation bridge is not
  // implemented.  Presentation setting changes do notify observers.
  static void AddObserver(Observer* observer);

  // Unregisters an observer.
  static void RemoveObserver(Observer* observer);

  // Notify all observers that search engine state has changed.
  //
  // TODO(astra): This should be called from the TemplateURLServiceObserver
  // implementation.  Currently it is a manual trigger for testing.
  static void NotifySearchEnginesChanged();

  // Notify all observers that the default search engine has changed.
  static void NotifyDefaultSearchEngineChanged();

  // Notify all observers that a search engine has been added.
  static void NotifySearchEngineAdded(const AstraSearchEngineInfo& engine);

  // Notify all observers that a search engine has been removed.
  static void NotifySearchEngineRemoved(const std::string& engine_id);

  // Notify all observers that search presentation settings have changed.
  static void NotifySearchPresentationChanged();

  // Notify all observers that the recent queries list has changed.
  static void NotifyRecentQueriesChanged();

 private:
  // Get the TemplateURLService for |profile| via the factory.
  // Returns nullptr if the service is not available.
  //
  // TODO(astra): Use TemplateURLServiceFactory::GetForProfile() when
  // building against a full Chromium checkout.  In the overlay, the
  // service may not be linked, so we return nullptr as a placeholder.
  // Chromium factory: TemplateURLServiceFactory
  //   (chrome/browser/search_engines/template_url_service_factory.h)
  static class TemplateURLService* GetTemplateURLService(Profile* profile);

  // Find a TemplateURL by its string id in the service.
  // Returns nullptr if not found.
  static const TemplateURL* FindTemplateURLById(Profile* profile,
                                                const std::string& engine_id);

  // Project a TemplateURL into an AstraSearchEngineInfo struct.
  // Returns default-constructed info (empty id) if template_url is nullptr.
  static AstraSearchEngineInfo ProjectEngine(const TemplateURL* template_url,
                                             bool is_default);

  // Returns the static observer list.  Wrapped in a function to avoid
  // static initialization order issues.
  static base::ObserverList<Observer>& GetObservers();

  // Helper to get the PrefService from a profile, with null checks.
  static PrefService* GetPrefs(Profile* profile);

  // Internal helper to truncate the recent queries list to max size.
  // Returns true if truncation actually occurred.
  static bool TruncateRecentQueries(PrefService* prefs, int max_count);
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SEARCH_ENGINE_HELPER_H_
