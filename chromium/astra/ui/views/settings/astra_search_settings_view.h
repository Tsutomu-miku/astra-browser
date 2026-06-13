#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SEARCH_SETTINGS_VIEW_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SEARCH_SETTINGS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "url/gurl.h"
#include "ui/views/view.h"

class Browser;
class PrefService;

namespace views {
class Label;
class MdTextButton;
class ScrollView;
class ToggleButton;
}  // namespace views

namespace astra {

class AstraSearchEngineHelper;
class AstraSettingsModel;

// Search engine data structure.
struct AstraSearchEngine {
  std::string id;
  std::u16string name;
  std::u16string keyword;
  GURL url;
  bool is_default = false;
  bool is_editable = true;
};

// =========================================================================
// Astra search engine settings view
// =========================================================================
//
// AstraSearchSettingsView is a Views-based search engine settings section
// for the Astra settings page.  It shows the list of search engines,
// highlights the default one, and allows the user to change the default
// search engine or manage search engines.
//
// Truth source: All search engine data is read from and all modifications
// go through Chromium's TemplateURLService (via AstraSearchEngineHelper).
// No search engine state is stored in the view itself — the view is
// purely a projection of TemplateURLService state.
//
// Features:
//   - Shows the default search engine prominently.
//   - Lists other search engines with "Make default" button.
//   - Each engine shows name, keyword, and URL.
//   - Search suggestions toggle.
//   - Add/edit/remove search engines.
//
// Chromium subsystems reused:
//   - TemplateURLService (search engine management)
//   - TemplateURL (individual search engine definition)
//
// Chromium owner / pattern reference:
//   chrome/browser/resources/settings/search_engines_page/ — Chrome's
//     WebUI search engine settings page.
//   chrome/browser/ui/search_engines/search_engine_dialog.h — Chrome's
//     native search engine editor dialog.
//
// TODO(astra): Wire model changes to PrefService for persistence.
//   Currently the view delegates to AstraSearchEngineHelper which
//   projects TemplateURLService state.
//   Chromium owner: TemplateURLService (chrome/browser/search_engines/)
// =========================================================================

class AstraSearchSettingsView : public views::View {
 public:
  explicit AstraSearchSettingsView(Browser* browser);
  ~AstraSearchSettingsView() override;

  AstraSearchSettingsView(const AstraSearchSettingsView&) = delete;
  AstraSearchSettingsView& operator=(const AstraSearchSettingsView&) = delete;

  // -- Model integration ---------------------------------------------------

  // Set the settings model.  The view observes the model for changes.
  void SetModel(AstraSettingsModel* model);
  AstraSettingsModel* model() { return model_; }

  // -- Search engine management -------------------------------------------

  // Get the ID of the default search engine.
  std::string GetDefaultSearchEngine() const;

  // Set the default search engine by ID.  Returns true on success.
  bool SetDefaultSearchEngine(const std::string& engine_id);

  // Get the list of all available search engines.
  std::vector<AstraSearchEngine> GetSearchEngines() const;

  // Add a new search engine.  Returns the new engine ID, or empty string
  // on failure.
  std::string AddSearchEngine(const std::string& name, const GURL& url);

  // Remove a search engine by ID.  Returns true on success.
  bool RemoveSearchEngine(const std::string& engine_id);

  // Edit a search engine's name and URL.  Returns true on success.
  bool EditSearchEngine(const std::string& engine_id,
                        const std::string& name,
                        const GURL& url);

  // -- Search suggestions --------------------------------------------------

  // Enable/disable search suggestions.
  void SetSearchSuggestionsEnabled(bool enabled);
  bool IsSearchSuggestionsEnabled() const;

  // -- Refresh -------------------------------------------------------------

  // Refresh the search engine list from TemplateURLService.
  // Call this when external changes may have occurred.
  void RefreshFromService();

  // Returns true if this section matches the given search query.
  // Matches against section title, description, and engine names.
  bool MatchesSearch(const std::u16string& query) const;

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;

 private:
  // Build the entire search settings section.
  void BuildContents();

  // Build the "Default search engine" section.
  void BuildDefaultEngineSection();

  // Build the "Other search engines" list section.
  void BuildOtherEnginesSection();

  // Build the search suggestions toggle row.
  void BuildSuggestionsSection();

  // Build the "Add search engine" section.
  void BuildAddEngineSection();

  // -- Event handlers ----------------------------------------------------

  // Handler for "Make default" button on a search engine row.
  void OnMakeDefault(const std::string& engine_id);

  // Handler for "Add" button.
  void OnAddSearchEngine();

  // Handler for "Edit" button.
  void OnEditSearchEngine(const std::string& engine_id);

  // Handler for "Delete" button.
  void OnDeleteSearchEngine(const std::string& engine_id);

  // Handler for search suggestions toggle.
  void OnSearchSuggestionsToggled();

  // -- UI helpers --------------------------------------------------------

  // Creates a row view for a single search engine entry.
  // The row shows name, keyword, URL, and action buttons.
  std::unique_ptr<views::View> CreateEngineRow(
      const std::string& engine_id,
      const std::u16string& name,
      const std::u16string& keyword,
      const std::string& url,
      bool is_default,
      bool is_editable);

  // Creates a section header label.
  views::Label* AddSectionHeader(views::View* parent,
                                 const std::u16string& title);

  // Helpers ---------------------------------------------------------------

  PrefService* GetPrefs();

  raw_ptr<Browser> browser_;

  // Settings model (not owned, may be null).
  raw_ptr<AstraSettingsModel> model_ = nullptr;

  // Default engine display section.
  raw_ptr<views::View> default_engine_container_ = nullptr;

  // Other engines list section (scrollable).
  raw_ptr<views::ScrollView> other_engines_scroll_ = nullptr;
  raw_ptr<views::View> other_engines_container_ = nullptr;

  // Search suggestions toggle.
  raw_ptr<views::ToggleButton> suggestions_toggle_ = nullptr;

  // Add engine button.
  raw_ptr<views::MdTextButton> add_engine_button_ = nullptr;

  // Section title (for search matching).
  std::u16string section_title_;

  // Search keywords (for search matching).
  std::vector<std::u16string> search_keywords_;

  // Cached list of search engines.
  mutable std::vector<AstraSearchEngine> cached_engines_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SEARCH_SETTINGS_VIEW_H_
