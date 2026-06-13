#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_MODEL_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_MODEL_H_

#include <map>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"

class PrefService;

namespace astra {

// =========================================================================
// AstraSettingsModel — data model for Astra settings UI
// =========================================================================
//
// AstraSettingsModel is the model layer for the Astra settings UI.  It owns
// presentation state (section expansion, search query) and mediates between
// the view layer and preferences (actual data is owned by Chromium's
// PrefService).  The model adds an observer pattern so views observe to
// stay in sync with state changes.
//
// Setting types: boolean, integer, double, string, enum, list, action.
// Each setting has metadata (title, description, section, subpage, icon,
// search tags, managed/recommended flags, default/current values, options
// for enums, min/max for numerics).
//
// Sections organize settings into logical groups: Appearance, Workspaces,
// Sidebar, Tabs, Privacy & Security, Search, Accessibility, Performance,
// Notifications, Advanced.
//
// Truth source: All actual setting values are owned by Chromium's PrefService.
// The model stores only presentation state and provides typed accessors that
// read from and write to PrefService.
//
// Observer pattern: Views observe the model for changes to presentation state
// (section expansion, search query, settings changed notifications when prefs
// change).
//
// Chromium subsystems reused:
//   - PrefService (actual settings persistence)
//   - base::ObserverList (observer pattern)
//   - base::Value (setting value storage)
//
// Chromium owner / pattern reference:
//   chrome/browser/ui/webui/settings/ — Chrome's settings model layer
//   components/prefs/pref_service.h — PrefService
//
// TODO(astra): Wire model changes to PrefService for persistence.
//   Currently the model holds values in memory.  Production code should
//   delegate reads/writes to PrefService.
//   Chromium owner: PrefService (components/prefs/pref_service.h)
// =========================================================================

// Setting value types.
enum class AstraSettingType {
  kBoolean = 0,
  kInteger = 1,
  kDouble = 2,
  kString = 3,
  kEnum = 4,
  kList = 5,
  kAction = 6,
};

// Total number of setting types.
constexpr size_t kNumSettingTypes =
    static_cast<size_t>(AstraSettingType::kAction) + 1;

// =========================================================================
// AstraSettingItem — metadata and current value for a single setting
// =========================================================================
//
// Each setting has a unique key (pref key), display metadata (title,
// description, icon), organizational info (section, subpage), and
// searchability tags.  The value is stored as a base::Value for
// type flexibility.
//
// Managed settings are controlled by enterprise policy and cannot be
// changed by the user.  Recommended settings have a suggested value
// that differs from the default.
// =========================================================================
struct AstraSettingItem {
  // Unique key (typically a pref key).
  std::string key;

  // Display title.
  std::u16string title;

  // Detailed description.
  std::u16string description;

  // Value type.
  AstraSettingType type = AstraSettingType::kBoolean;

  // Section this setting belongs to.
  std::string section;

  // Subpage path (empty = main page).
  std::string subpage;

  // Icon name identifier.
  std::string icon_name;

  // Search tags for indexing.
  std::vector<std::u16string> search_tags;

  // Whether this setting is managed by policy (read-only).
  bool is_managed = false;

  // Whether this setting has a recommended value.
  bool is_recommended = false;

  // Default value.
  base::Value default_value;

  // Current value.
  base::Value current_value;

  // Options for enum settings (display strings).
  std::vector<std::u16string> options;

  // Minimum value for numeric settings.
  base::Value min_value;

  // Maximum value for numeric settings.
  base::Value max_value;

  AstraSettingItem();
  AstraSettingItem(const AstraSettingItem& other);
  AstraSettingItem& operator=(const AstraSettingItem& other);
  ~AstraSettingItem();
};

// =========================================================================
// AstraSettingsSection — a section/group of settings
// =========================================================================
//
// Each section has an ID, display name, icon, description, and a list
// of setting keys that belong to it.
// =========================================================================
struct AstraSettingsSectionInfo {
  // Section identifier.
  std::string id;

  // Display name.
  std::u16string name;

  // Section description.
  std::u16string description;

  // Icon name identifier.
  std::string icon_name;

  // List of setting keys in this section.
  std::vector<std::string> setting_keys;
};

// =========================================================================
// AstraSettingsObserver — observer for settings model changes
// =========================================================================
//
// All methods have empty default implementations so observers can override
// only the methods they care about.
//
// Extends base::CheckedObserver for safe observation patterns.
// =========================================================================
class AstraSettingsObserver : public base::CheckedObserver {
 public:
  // Called when a single setting value changes.
  virtual void OnSettingChanged(AstraSettingsModel* model,
                                const std::string& key) {}

  // Called when settings are reset (all or partial).
  virtual void OnSettingsReset(AstraSettingsModel* model) {}

  // Called when search results change (new query, etc.).
  virtual void OnSettingsSearchResultsChanged(AstraSettingsModel* model) {}

  // Called when the model is being shut down.
  virtual void OnSettingsModelShutdown(AstraSettingsModel* model) {}

 protected:
  ~AstraSettingsObserver() override = default;
};

// =========================================================================
// AstraSettingsModel — the main model class
// =========================================================================

class AstraSettingsModel {
 public:
  explicit AstraSettingsModel(PrefService* pref_service);
  ~AstraSettingsModel();

  AstraSettingsModel(const AstraSettingsModel&) = delete;
  AstraSettingsModel& operator=(const AstraSettingsModel&) = delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraSettingsObserver* observer);
  void RemoveObserver(AstraSettingsObserver* observer);

  // -- Setting accessors ---------------------------------------------------

  // Returns the setting item for the given key, or nullptr if not found.
  const AstraSettingItem* GetSetting(const std::string& key) const;

  // Returns all settings.
  std::vector<const AstraSettingItem*> GetAllSettings() const;

  // Returns settings belonging to the given section.
  std::vector<const AstraSettingItem*> GetSettingsBySection(
      const std::string& section_id) const;

  // Returns the total number of settings.
  size_t GetSettingCount() const;

  // -- Section accessors ---------------------------------------------------

  // Returns section info for the given section ID, or nullptr if not found.
  const AstraSettingsSectionInfo* GetSection(
      const std::string& section_id) const;

  // Returns all sections.
  std::vector<const AstraSettingsSectionInfo*> GetAllSections() const;

  // Returns the number of sections.
  size_t GetSectionCount() const;

  // -- Search --------------------------------------------------------------

  // Search settings by query (matches title, description, tags).
  // Returns matching setting keys.
  std::vector<std::string> SearchSettings(const std::u16string& query) const;

  // -- Setting value mutation ----------------------------------------------

  // Sets the value of a setting.  Returns true if the setting exists and
  // the value was changed.  Does nothing and returns false if the setting
  // is managed or doesn't exist.
  bool SetSettingValue(const std::string& key, const base::Value& value);

  // Resets a setting to its default value.  Returns true if the setting
  // exists and was reset.  Returns false for managed or non-existent
  // settings.
  bool ResetSetting(const std::string& key);

  // Resets all settings to their default values.
  void ResetAllSettings();

  // -- Managed / default helpers -------------------------------------------

  // Returns true if the setting is managed by policy (read-only).
  bool IsSettingManaged(const std::string& key) const;

  // Returns the default value for a setting, or a null value if the
  // setting doesn't exist.
  base::Value GetDefaultValue(const std::string& key) const;

  // -- Presentation state (section expansion, search query) -------------

  // Get/set whether a section is expanded (by section ID string).
  bool IsSectionExpanded(const std::string& section_id) const;
  void SetSectionExpanded(const std::string& section_id, bool expanded);
  void ToggleSectionExpanded(const std::string& section_id);
  void ExpandAllSections();
  void CollapseAllSections();

  // Current search query.
  const std::u16string& search_query() const { return search_query_; }
  void SetSearchQuery(const std::u16string& query);

  // -- PrefService access --------------------------------------------------

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

  // -- Static helpers ------------------------------------------------------

  // Get the display title for a section by ID.
  static std::u16string GetSectionTitle(const std::string& section_id);

  // Get the description for a section by ID.
  static std::u16string GetSectionDescription(const std::string& section_id);

  // Get search keywords for a section by ID.
  static std::vector<std::u16string> GetSectionKeywords(
      const std::string& section_id);

 private:
  // Initialize all default settings and sections.
  void InitializeDefaults();

  // Notify observers that a setting changed.
  void NotifySettingChanged(const std::string& key);

  // Notify observers that settings were reset.
  void NotifySettingsReset();

  // Notify observers that search results changed.
  void NotifySearchResultsChanged();

  // Notify observers that the model is shutting down.
  void NotifyShutdown();

  // Find a setting by key (non-const internal helper).
  AstraSettingItem* FindSetting(const std::string& key);

  raw_ptr<PrefService> pref_service_ = nullptr;

  // All settings, keyed by setting key.
  std::map<std::string, AstraSettingItem> settings_;

  // All sections, keyed by section ID.
  std::map<std::string, AstraSettingsSectionInfo> sections_;

  // Section expansion state (presentation state, not persisted).
  std::map<std::string, bool> section_expanded_;

  // Current search query (presentation state, not persisted).
  std::u16string search_query_;

  // Observers.
  base::ObserverList<AstraSettingsObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_MODEL_H_
