#include "astra/browser/astra_search_engine_helper.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
// TODO(astra): Include TemplateURLService headers when building against
// a full Chromium checkout.  In the overlay repo, these headers may not
// be available, so we forward-declare TemplateURL and TemplateURLService
// and return placeholder values.
//
// Chromium headers:
//   #include "components/search_engines/template_url.h"
//   #include "components/search_engines/template_url_service.h"
//   #include "chrome/browser/search_engines/template_url_service_factory.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Truncate a list of values to |max_count| elements.
// Returns true if truncation actually occurred.
bool TruncateListToMax(base::Value::List& list, int max_count) {
  if (max_count < 0) {
    max_count = 0;
  }
  if (static_cast<int>(list.size()) <= max_count) {
    return false;
  }
  list.erase(list.begin() + max_count, list.end());
  return true;
}

// TODO(astra): These placeholder implementations return empty/stub data
// because the overlay repo does not have the full Chromium search engine
// headers and libraries available.  When building against a full Chromium
// checkout, replace these with real calls to TemplateURLService.
//
// The API surface is designed to match Chromium's TemplateURLService
// patterns so the migration is straightforward.

// Check if the TemplateURLService is available (loaded) for |profile|.
// In the overlay, this always returns false since we don't have the
// real service linked in.
bool IsServiceAvailable(Profile* profile) {
  // TODO(astra): Return TemplateURLServiceFactory::GetForProfile(profile)
  // && service->loaded() once we have the full Chromium build.
  // For the overlay, we simulate "not available" to be safe.
  return false;
}

}  // namespace

// =========================================================================
// Search engine queries
// =========================================================================

const TemplateURL* AstraSearchEngineHelper::GetDefaultSearchEngine(
    Profile* profile) {
  if (!profile || !IsServiceAvailable(profile)) {
    return nullptr;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return nullptr;
  //   return service->GetDefaultSearchProvider();
  //
  // Chromium owner: TemplateURLService::GetDefaultSearchProvider
  //   (components/search_engines/template_url_service.h)
  return nullptr;
}

AstraSearchEngineInfo AstraSearchEngineHelper::GetDefaultSearchEngineInfo(
    Profile* profile) {
  const TemplateURL* default_engine = GetDefaultSearchEngine(profile);
  return ProjectEngine(default_engine, /*is_default=*/true);
}

std::vector<AstraSearchEngineInfo> AstraSearchEngineHelper::GetSearchEngines(
    Profile* profile) {
  std::vector<AstraSearchEngineInfo> result;

  if (!profile || !IsServiceAvailable(profile)) {
    return result;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return result;
  //
  //   const TemplateURL* default_engine = service->GetDefaultSearchProvider();
  //   std::vector<const TemplateURL*> engines = service->GetTemplateURLs();
  //
  //   for (const TemplateURL* turl : engines) {
  //     bool is_default = (turl == default_engine);
  //     result.push_back(ProjectEngine(turl, is_default));
  //   }
  //
  //   // Sort: default first, then alphabetically by name.
  //   std::sort(result.begin(), result.end(),
  //     [](const AstraSearchEngineInfo& a, const AstraSearchEngineInfo& b) {
  //       if (a.is_default != b.is_default) return a.is_default > b.is_default;
  //       return a.name < b.name;
  //     });
  //
  // Chromium method: TemplateURLService::GetTemplateURLs()
  //   (components/search_engines/template_url_service.h)

  return result;
}

std::vector<AstraSearchEngineInfo> AstraSearchEngineHelper::GetSearchEnginesList(
    Profile* profile) {
  return GetSearchEngines(profile);
}

std::vector<AstraSearchEngineInfo>
AstraSearchEngineHelper::GetNonDefaultSearchEngines(Profile* profile) {
  std::vector<AstraSearchEngineInfo> all = GetSearchEngines(profile);
  std::vector<AstraSearchEngineInfo> result;
  for (const auto& engine : all) {
    if (!engine.is_default) {
      result.push_back(engine);
    }
  }
  return result;
}

size_t AstraSearchEngineHelper::GetSearchEngineCount(Profile* profile) {
  return GetSearchEngines(profile).size();
}

bool AstraSearchEngineHelper::IsDefaultSearchEngine(
    Profile* profile,
    const std::string& engine_id) {
  if (engine_id.empty()) {
    return false;
  }

  AstraSearchEngineInfo default_engine = GetDefaultSearchEngineInfo(profile);
  if (default_engine.id.empty()) {
    return false;
  }

  return default_engine.id == engine_id;
}

AstraSearchEngineInfo AstraSearchEngineHelper::GetSearchEngineByName(
    Profile* profile,
    const std::u16string& name) {
  if (name.empty()) {
    return AstraSearchEngineInfo();
  }

  std::vector<AstraSearchEngineInfo> engines = GetSearchEngines(profile);
  for (const auto& engine : engines) {
    if (base::EqualsCaseInsensitiveASCII(engine.name, name)) {
      return engine;
    }
  }

  return AstraSearchEngineInfo();
}

AstraSearchEngineInfo AstraSearchEngineHelper::GetSearchEngineByKeyword(
    Profile* profile,
    const std::u16string& keyword) {
  if (keyword.empty()) {
    return AstraSearchEngineInfo();
  }

  std::vector<AstraSearchEngineInfo> engines = GetSearchEngines(profile);
  for (const auto& engine : engines) {
    if (base::EqualsCaseInsensitiveASCII(engine.keyword, keyword)) {
      return engine;
    }
  }

  return AstraSearchEngineInfo();
}

// =========================================================================
// Search engine operations
// =========================================================================

bool AstraSearchEngineHelper::SetDefaultSearchEngine(
    Profile* profile,
    const TemplateURL* template_url) {
  if (!profile || !template_url || !IsServiceAvailable(profile)) {
    return false;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return false;
  //   service->SetUserSelectedDefaultSearchProvider(template_url);
  //   NotifyDefaultSearchEngineChanged();
  //   NotifySearchEnginesChanged();
  //   return true;
  //
  // Chromium method:
  //   TemplateURLService::SetUserSelectedDefaultSearchProvider
  //   or TemplateURLService::SetDefaultSearchProvider
  //   (components/search_engines/template_url_service.h)
  return false;
}

bool AstraSearchEngineHelper::SetDefaultSearchEngineById(
    Profile* profile,
    const std::string& engine_id) {
  const TemplateURL* turl = FindTemplateURLById(profile, engine_id);
  if (!turl) {
    return false;
  }
  return SetDefaultSearchEngine(profile, turl);
}

const TemplateURL* AstraSearchEngineHelper::AddSearchEngine(
    Profile* profile,
    const std::u16string& name,
    const std::u16string& keyword,
    const std::string& url) {
  if (!profile || name.empty() || url.empty() || !IsServiceAvailable(profile)) {
    return nullptr;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return nullptr;
  //
  //   TemplateURLData data;
  //   data.SetShortName(name);
  //   data.SetKeyword(keyword);
  //   data.SetURL(url);
  //   // Set other fields: safe_for_autoreplace, date_created, etc.
  //
  //   TemplateURL* turl = service->Add(std::make_unique<TemplateURL>(data));
  //   if (turl) {
  //     AstraSearchEngineInfo info = ProjectEngine(turl, /*is_default=*/false);
  //     NotifySearchEngineAdded(info);
  //     NotifySearchEnginesChanged();
  //   }
  //   return turl;
  //
  // Chromium method: TemplateURLService::Add()
  //   (components/search_engines/template_url_service.h)
  // Chromium type: TemplateURLData
  //   (components/search_engines/template_url_data.h)
  return nullptr;
}

bool AstraSearchEngineHelper::RemoveSearchEngine(
    Profile* profile,
    const TemplateURL* template_url) {
  if (!profile || !template_url || !IsServiceAvailable(profile)) {
    return false;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return false;
  //
  //   // Check if engine is removable (not built-in, not policy-controlled).
  //   if (template_url->is_prepopulated() || template_url->IsManaged()) {
  //     return false;
  //   }
  //
  //   std::string engine_id = template_url->guid();
  //   bool was_default = (template_url == service->GetDefaultSearchProvider());
  //
  //   service->Remove(template_url);
  //
  //   NotifySearchEngineRemoved(engine_id);
  //   if (was_default) {
  //     NotifyDefaultSearchEngineChanged();
  //   }
  //   NotifySearchEnginesChanged();
  //   return true;
  //
  // Chromium method: TemplateURLService::Remove()
  //   (components/search_engines/template_url_service.h)
  return false;
}

bool AstraSearchEngineHelper::RemoveSearchEngineById(
    Profile* profile,
    const std::string& engine_id) {
  const TemplateURL* turl = FindTemplateURLById(profile, engine_id);
  if (!turl) {
    return false;
  }
  return RemoveSearchEngine(profile, turl);
}

// =========================================================================
// Chrome settings delegation
// =========================================================================

void AstraSearchEngineHelper::OpenChromeSearchSettings(Profile* profile) {
  if (!profile) {
    return;
  }

  // TODO(astra): Implement proper navigation to Chrome's search engine
  // settings page.  There are several approaches:
  //
  //   1. Use chrome::SettingsWindowManager::ShowSearchEngineSettings
  //      (chrome/browser/ui/settings_window_manager.h)
  //
  //   2. Use NavigateParams to open chrome://settings/searchEngines
  //      in a new tab or the active tab.
  //
  //   3. Open the SearchEngineDialog directly (modal dialog):
  //      SearchEngineDialog::ShowDialog()
  //      (chrome/browser/ui/search_engines/search_engine_dialog.h)
  //
  // For Astra's overlay approach, option 3 (SearchEngineDialog) may be
  // most appropriate since it is a self-contained modal dialog that
  // doesn't require navigating away from the current page.
  //
  // Chromium owner: SearchEngineDialog
  //   (chrome/browser/ui/search_engines/search_engine_dialog.h)
  // Chromium owner: SettingsWindowManager
  //   (chrome/browser/ui/settings_window_manager.h)
}

// =========================================================================
// Presentation settings
// =========================================================================

bool AstraSearchEngineHelper::GetShowDefaultEngine(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSearchShowDefaultEngine;
  }
  return prefs->GetBoolean(prefs::kPrefSearchShowDefaultEngine);
}

void AstraSearchEngineHelper::SetShowDefaultEngine(Profile* profile,
                                                   bool show) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefSearchShowDefaultEngine) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSearchShowDefaultEngine, show);
  NotifySearchPresentationChanged();
}

bool AstraSearchEngineHelper::GetShowOtherEngines(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSearchShowOtherEngines;
  }
  return prefs->GetBoolean(prefs::kPrefSearchShowOtherEngines);
}

void AstraSearchEngineHelper::SetShowOtherEngines(Profile* profile,
                                                  bool show) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefSearchShowOtherEngines) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSearchShowOtherEngines, show);
  NotifySearchPresentationChanged();
}

bool AstraSearchEngineHelper::ToggleShowOtherEngines(Profile* profile) {
  bool new_state = !GetShowOtherEngines(profile);
  SetShowOtherEngines(profile, new_state);
  return GetShowOtherEngines(profile);
}

bool AstraSearchEngineHelper::GetSuggestionsEnabled(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSearchSuggestionsEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefSearchSuggestionsEnabled);
}

void AstraSearchEngineHelper::SetSuggestionsEnabled(Profile* profile,
                                                    bool enabled) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefSearchSuggestionsEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSearchSuggestionsEnabled, enabled);
  NotifySearchPresentationChanged();
}

bool AstraSearchEngineHelper::ToggleSuggestionsEnabled(Profile* profile) {
  bool new_state = !GetSuggestionsEnabled(profile);
  SetSuggestionsEnabled(profile, new_state);
  return GetSuggestionsEnabled(profile);
}

// =========================================================================
// Recent search queries
// =========================================================================

std::vector<std::string> AstraSearchEngineHelper::GetRecentQueries(
    Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return {};
  }

  const base::Value::List& list =
      prefs->GetList(prefs::kPrefSearchRecentQueries)->GetList();

  std::vector<std::string> result;
  result.reserve(list.size());
  for (const auto& item : list) {
    if (item.is_string()) {
      result.push_back(item.GetString());
    }
  }
  return result;
}

void AstraSearchEngineHelper::AddRecentQuery(Profile* profile,
                                             const std::string& query) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs || query.empty()) {
    return;
  }

  base::Value::List list =
      prefs->GetList(prefs::kPrefSearchRecentQueries)->GetList().Clone();

  // Remove existing entry if present (we'll move it to the front).
  auto it = std::find_if(list.begin(), list.end(),
                         [&query](const base::Value& item) {
                           return item.is_string() && item.GetString() == query;
                         });
  if (it != list.end()) {
    list.erase(it);
  }

  // Insert at the front (most recent first).
  list.Insert(list.begin(), base::Value(query));

  // Truncate to max size.
  int max_count = GetMaxRecentQueries(profile);
  TruncateListToMax(list, max_count);

  // Update the pref.
  prefs->SetList(prefs::kPrefSearchRecentQueries, std::move(list));
  NotifyRecentQueriesChanged();
}

void AstraSearchEngineHelper::ClearRecentQueries(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  const base::Value::List* current =
      prefs->GetList(prefs::kPrefSearchRecentQueries);
  if (current->GetList().empty()) {
    return;  // Nothing to clear.
  }

  prefs->SetList(prefs::kPrefSearchRecentQueries, base::Value::List());
  NotifyRecentQueriesChanged();
}

int AstraSearchEngineHelper::GetMaxRecentQueries(Profile* profile) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return prefs::kDefaultSearchMaxRecentQueries;
  }
  return prefs->GetInteger(prefs::kPrefSearchMaxRecentQueries);
}

void AstraSearchEngineHelper::SetMaxRecentQueries(Profile* profile,
                                                  int max_count) {
  PrefService* prefs = GetPrefs(profile);
  if (!prefs) {
    return;
  }

  // Clamp to reasonable range.
  if (max_count < 0) {
    max_count = 0;
  }
  if (max_count > prefs::kMaxSearchRecentQueriesLimit) {
    max_count = prefs::kMaxSearchRecentQueriesLimit;
  }

  int current_max = prefs->GetInteger(prefs::kPrefSearchMaxRecentQueries);
  if (current_max == max_count) {
    return;
  }

  prefs->SetInteger(prefs::kPrefSearchMaxRecentQueries, max_count);

  // If the new max is smaller, truncate the list.
  if (max_count < current_max) {
    base::Value::List list =
        prefs->GetList(prefs::kPrefSearchRecentQueries)->GetList().Clone();
    if (static_cast<int>(list.size()) > max_count) {
      list.erase(list.begin() + max_count, list.end());
      prefs->SetList(prefs::kPrefSearchRecentQueries, std::move(list));
      NotifyRecentQueriesChanged();
    }
  }
}

// =========================================================================
// Search shortcut helpers
// =========================================================================

std::string AstraSearchEngineHelper::BuildSearchUrl(
    Profile* profile,
    const std::u16string& keyword,
    const std::u16string& query) {
  if (!profile || keyword.empty() || query.empty()) {
    return std::string();
  }

  if (!IsServiceAvailable(profile)) {
    return std::string();
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return std::string();
  //
  //   const TemplateURL* turl =
  //       service->GetTemplateURLForKeyword(keyword);
  //   if (!turl) return std::string();
  //
  //   TemplateURLRef::SearchTermsArgs search_terms_args(query);
  //   return turl->url_ref().ReplaceSearchTerms(
  //       search_terms_args,
  //       service->search_terms_data(),
  //       /*post_content=*/nullptr);
  //
  // Chromium method: TemplateURL::ReplaceSearchTerms / TemplateURLRef
  //   (components/search_engines/template_url.h)
  //   (components/search_engines/template_url_ref.h)

  return std::string();
}

// =========================================================================
// Observers
// =========================================================================

void AstraSearchEngineHelper::AddObserver(Observer* observer) {
  GetObservers().AddObserver(observer);
}

void AstraSearchEngineHelper::RemoveObserver(Observer* observer) {
  GetObservers().RemoveObserver(observer);
}

void AstraSearchEngineHelper::NotifySearchEnginesChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnSearchEnginesChanged();
  }
}

void AstraSearchEngineHelper::NotifyDefaultSearchEngineChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnDefaultSearchEngineChanged();
  }
}

void AstraSearchEngineHelper::NotifySearchEngineAdded(
    const AstraSearchEngineInfo& engine) {
  for (auto& observer : GetObservers()) {
    observer.OnSearchEngineAdded(engine);
  }
}

void AstraSearchEngineHelper::NotifySearchEngineRemoved(
    const std::string& engine_id) {
  for (auto& observer : GetObservers()) {
    observer.OnSearchEngineRemoved(engine_id);
  }
}

void AstraSearchEngineHelper::NotifySearchPresentationChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnSearchPresentationChanged();
  }
}

void AstraSearchEngineHelper::NotifyRecentQueriesChanged() {
  for (auto& observer : GetObservers()) {
    observer.OnRecentQueriesChanged();
  }
}

// =========================================================================
// Helpers
// =========================================================================

TemplateURLService* AstraSearchEngineHelper::GetTemplateURLService(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }

  // TODO(astra): Use TemplateURLServiceFactory::GetForProfile() when
  // building against a full Chromium checkout.
  //
  // Chromium factory: TemplateURLServiceFactory
  //   (chrome/browser/search_engines/template_url_service_factory.h)
  return nullptr;
}

const TemplateURL* AstraSearchEngineHelper::FindTemplateURLById(
    Profile* profile,
    const std::string& engine_id) {
  if (!profile || engine_id.empty() || !IsServiceAvailable(profile)) {
    return nullptr;
  }

  // TODO(astra): Replace with real implementation:
  //   TemplateURLService* service = GetTemplateURLService(profile);
  //   if (!service) return nullptr;
  //
  //   // Look up by GUID or by id.
  //   // TemplateURLService::GetTemplateURLForGUID() or iterate.
  //   for (const TemplateURL* turl : service->GetTemplateURLs()) {
  //     if (turl->guid() == engine_id ||
  //         base::NumberToString(turl->id()) == engine_id) {
  //       return turl;
  //     }
  //   }
  //   return nullptr;
  //
  // Chromium method: TemplateURLService::GetTemplateURLForGUID
  //   (components/search_engines/template_url_service.h)
  return nullptr;
}

AstraSearchEngineInfo AstraSearchEngineHelper::ProjectEngine(
    const TemplateURL* template_url,
    bool is_default) {
  AstraSearchEngineInfo info;
  info.is_default = is_default;

  if (!template_url) {
    return info;
  }

  // TODO(astra): Populate from real TemplateURL fields once we have
  // the full Chromium headers:
  //
  //   info.id = template_url->guid();
  //   info.name = template_url->short_name();
  //   info.keyword = template_url->keyword();
  //   info.url = template_url->url();
  //   info.is_builtin = template_url->is_prepopulated();
  //   info.is_editable = !template_url->IsManaged() &&
  //                      !template_url->is_prepopulated() &&
  //                      !template_url->is_extension_keyword();
  //
  // Chromium type: TemplateURL
  //   (components/search_engines/template_url.h)
  return info;
}

base::ObserverList<AstraSearchEngineHelper::Observer>&
AstraSearchEngineHelper::GetObservers() {
  static base::NoDestructor<base::ObserverList<Observer>> observers;
  return *observers;
}

PrefService* AstraSearchEngineHelper::GetPrefs(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

bool AstraSearchEngineHelper::TruncateRecentQueries(PrefService* prefs,
                                                    int max_count) {
  if (!prefs || max_count <= 0) {
    return false;
  }

  base::Value::List list =
      prefs->GetList(prefs::kPrefSearchRecentQueries)->GetList().Clone();

  if (!TruncateListToMax(list, max_count)) {
    return false;
  }

  prefs->SetList(prefs::kPrefSearchRecentQueries, std::move(list));
  return true;
}

}  // namespace astra
