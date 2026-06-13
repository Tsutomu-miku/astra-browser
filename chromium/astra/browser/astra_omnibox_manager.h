#ifndef ASTRA_BROWSER_ASTRA_OMNIBOX_MANAGER_H_
#define ASTRA_BROWSER_ASTRA_OMNIBOX_MANAGER_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"

class Browser;
class Profile;

namespace astra {

class AstraOmniboxProvider;
struct AstraOmniboxAction;
struct AstraOmniboxSuggestion;
enum class AstraOmniboxActionType;
enum class AstraOmniboxActionCategory;

// =========================================================================
// AstraOmniboxManager::Observer
// =========================================================================
//
// Observer interface for Astra omnibox events.  All methods have default
// empty implementations so subclasses only need to override the ones they
// care about.
//
// Notifications:
//   - OnOmniboxActionExecuted: an Astra action was executed from the omnibox
//   - OnSuggestionsChanged: the set of available suggestions changed
//     (e.g. provider enabled/disabled, category toggled)
//   - OnProviderEnabledChanged: the Astra omnibox provider was toggled on/off
//   - OnOmniboxSettingsChanged: any omnibox presentation setting changed
//   - OnRecentActionsChanged: the recent actions list changed
//
// Observer pattern follows Chromium conventions:
//   - Derives from base::CheckedObserver for safe removal during iteration.
//   - Managed via base::ObserverList.
// =========================================================================

class AstraOmniboxManagerObserver : public base::CheckedObserver {
 public:
  // Called when an Astra omnibox action is executed.
  // |action_type| is the type of action executed.
  // |payload| is the action-specific payload string.
  // |success| is whether the action executed successfully.
  virtual void OnOmniboxActionExecuted(AstraOmniboxActionType action_type,
                                       const std::string& payload,
                                       bool success) {}

  // Called when the available suggestions change (e.g. provider toggled,
  // category filter changed, or max suggestions updated).
  virtual void OnSuggestionsChanged() {}

  // Called when the Astra omnibox provider is enabled or disabled.
  virtual void OnProviderEnabledChanged(bool enabled) {}

  // Called when any omnibox presentation setting changes.
  virtual void OnOmniboxSettingsChanged() {}

  // Called when the recent actions list changes.
  virtual void OnRecentActionsChanged() {}

 protected:
  ~AstraOmniboxManagerObserver() override = default;
};

// =========================================================================
// Astra omnibox manager
// =========================================================================
//
// AstraOmniboxManager is the coordinator for all Astra omnibox features.
// It owns the suggestion provider and handles action execution, acting as
// the bridge between the omnibox UI layer and Astra services.
//
// This class is the entry point that Chromium patch points call into.
// A patched AutocompleteController or OmniboxPedalProvider would call:
//   astra::AstraOmniboxManager::GetForProfile(profile)->GetSuggestions(text)
//
// And a patched OmniboxAction or suggestion handler would call:
//   astra::AstraOmniboxManager::GetForBrowser(browser)->ExecuteAction(...)
//
// Architecture:
//   Chromium patch          ->  AstraOmniboxManager  ->  Astra services
//   (omnibox controller)        (coordinator)            (workspace, etc.)
//
// The manager is per-profile (ProfileKeyedService-adjacent) because
// suggestions depend on profile-scoped data (workspaces, favorites, etc.).
// Action execution additionally requires a Browser* for tab strip access.
//
// Chromium owner: AutocompleteController (components/omnibox/browser/autocomplete_controller.h)
// and OmniboxPedalProvider (components/omnibox/browser/omnibox_pedal_provider.h).
//
// TODO(astra): Convert this to a proper ProfileKeyedService with a factory
// once the profile keyed service registration patch is in place.  Patch
// point: chrome/browser/profiles/profile_keyed_service_factory registrations.
// For now it is a plain class that callers can instantiate or obtain via
// a static accessor backed by BrowserUserData.
// =========================================================================

class AstraOmniboxManager {
 public:
  using Observer = AstraOmniboxManagerObserver;

  explicit AstraOmniboxManager(Profile* profile);
  ~AstraOmniboxManager();

  AstraOmniboxManager(const AstraOmniboxManager&) = delete;
  AstraOmniboxManager& operator=(const AstraOmniboxManager&) = delete;

  // Returns the AstraOmniboxManager for |profile|.
  //
  // TODO(astra): Use ProfileKeyedServiceFactory pattern instead of a
  // static accessor.  For now this is a stand-in that creates a new
  // instance on first call.  This is not thread-safe and is intended for
  // single-browser testing.
  //
  // Chromium pattern to follow: see AstraWorkspaceServiceFactory.
  static AstraOmniboxManager* GetForProfile(Profile* profile);

  // Returns the AstraOmniboxManager for |browser|'s profile.
  static AstraOmniboxManager* GetForBrowser(Browser* browser);

  // -- Observers ----------------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Suggestions --------------------------------------------------------

  // Returns Astra-specific omnibox suggestions for |text|.
  // If |text| doesn't match an Astra prefix, returns an empty list.
  // If the provider is disabled, also returns an empty list.
  //
  // |browser| is used for tab-strip-dependent suggestions (@tab, @favorites).
  // It may be null; in that case tab-dependent suggestions are skipped.
  std::vector<AstraOmniboxSuggestion> GetSuggestions(Browser* browser,
                                                     const std::u16string& text);

  // Returns true if |text| starts with a recognized Astra omnibox prefix.
  bool MatchesAstraPrefix(const std::u16string& text) const;

  // -- Action execution ---------------------------------------------------

  // Executes an Astra omnibox action in the context of |browser|.
  // Returns true if the action was recognized and executed.
  //
  // This delegates to ExecuteAstraOmniboxAction in astra_omnibox_action.h,
  // but is also callable through the manager for convenience.
  //
  // Also records the action in recent actions and notifies observers.
  bool ExecuteAction(Browser* browser,
                     AstraOmniboxActionType action_type,
                     const std::string& payload);

  // Convenience: execute from a suggestion struct.
  bool ExecuteSuggestion(Browser* browser,
                         const AstraOmniboxSuggestion& suggestion);

  // -- Provider access ----------------------------------------------------

  // Direct access to the underlying provider.  Used by tests and by
  // callers that need provider-specific methods.
  AstraOmniboxProvider* provider() { return provider_.get(); }
  const AstraOmniboxProvider* provider() const { return provider_.get(); }

  // -- Provider enabled state ---------------------------------------------

  // Whether the Astra omnibox suggestion provider is enabled.
  // When disabled, GetSuggestions() returns an empty list.
  bool provider_enabled() const { return provider_enabled_; }
  void SetProviderEnabled(bool enabled);

  // -- Presentation settings ----------------------------------------------

  // Whether Astra suggestions are shown in the omnibox dropdown.
  // This is a presentation preference — it controls whether the
  // Chromium patch point includes Astra suggestions in the result set.
  bool show_astra_suggestions() const { return show_astra_suggestions_; }
  void SetShowAstraSuggestions(bool show);

  // Maximum number of Astra suggestions to show.
  // This caps the number of Astra results in the omnibox dropdown.
  int max_astra_suggestions() const { return max_astra_suggestions_; }
  void SetMaxAstraSuggestions(int max);

  // Position of Astra suggestions relative to native ones.
  // Values: "top" or "bottom".
  const std::string& suggestion_position() const { return suggestion_position_; }
  void SetSuggestionPosition(const std::string& position);

  // -- Category filtering -------------------------------------------------

  // Whether a specific action category is enabled for suggestions.
  // When a category is disabled, actions of that type are not included
  // in suggestion results.
  bool IsCategoryEnabled(AstraOmniboxActionCategory category) const;
  void SetCategoryEnabled(AstraOmniboxActionCategory category, bool enabled);

  // Bulk: enable or disable all categories.
  void EnableAllCategories();
  void DisableAllCategories();

  // -- Recent actions -----------------------------------------------------

  // Returns the list of recently executed action types (most recent first).
  // The list is capped at max_recent_actions().
  std::vector<AstraOmniboxActionType> GetRecentActions() const;

  // Returns recent actions with full metadata (most recent first).
  // Each entry includes title, description, icon, etc.
  std::vector<AstraOmniboxAction> GetRecentActionDetails() const;

  // Maximum number of recent actions to remember.
  int max_recent_actions() const { return max_recent_actions_; }
  void SetMaxRecentActions(int max);

  // Adds an action to the recent actions list.
  // If the action is already in the list, it is moved to the front.
  // If the list exceeds max_recent_actions(), the oldest is removed.
  void AddRecentAction(AstraOmniboxActionType action_type,
                       const std::string& payload = std::string());

  // Clears the recent actions list.
  void ClearRecentActions();

  // -- Action catalog access ----------------------------------------------

  // Returns all known Astra omnibox actions.
  std::vector<AstraOmniboxAction> GetAllActions() const;

  // Returns actions filtered by category.
  std::vector<AstraOmniboxAction> GetActionsByCategory(
      AstraOmniboxActionCategory category) const;

  // Searches actions by query string (case-insensitive).
  std::vector<AstraOmniboxAction> SearchActions(const std::u16string& query) const;

 private:
  // Loads all persisted settings from the profile's PrefService.
  void LoadFromPrefs();

  // Saves the current settings to PrefService.
  void SaveSettingsToPrefs() const;

  // Saves provider enabled state to prefs.
  void SaveProviderEnabledToPrefs() const;

  // Saves show_astra_suggestions to prefs.
  void SaveShowAstraSuggestionsToPrefs() const;

  // Saves max_astra_suggestions to prefs.
  void SaveMaxAstraSuggestionsToPrefs() const;

  // Saves suggestion_position to prefs.
  void SaveSuggestionPositionToPrefs() const;

  // Saves category enabled states to prefs.
  void SaveCategoriesToPrefs() const;

  // Saves recent actions to prefs.
  void SaveRecentActionsToPrefs() const;

  // Saves max_recent_actions to prefs.
  void SaveMaxRecentActionsToPrefs() const;

  // Loads category enabled states from prefs.
  void LoadCategoriesFromPrefs();

  // Loads recent actions from prefs.
  void LoadRecentActionsFromPrefs();

  // Notifies all observers that suggestions changed.
  void NotifySuggestionsChanged();

  // Notifies all observers that settings changed.
  void NotifySettingsChanged();

  raw_ptr<Profile> profile_;
  std::unique_ptr<AstraOmniboxProvider> provider_;
  base::ObserverList<Observer> observers_;

  // -- Provider state --
  bool provider_enabled_ = true;

  // -- Presentation settings (persisted via PrefService) --
  bool show_astra_suggestions_ = true;
  int max_astra_suggestions_ = 5;
  std::string suggestion_position_ = "bottom";

  // -- Category enablement (persisted) --
  bool category_workspace_enabled_ = true;
  bool category_tab_enabled_ = true;
  bool category_navigation_enabled_ = true;
  bool category_tool_enabled_ = true;

  // -- Recent actions (persisted) --
  // Each entry is a pair of (action_type, payload).
  // Most recent first.
  std::vector<std::pair<AstraOmniboxActionType, std::string>> recent_actions_;
  int max_recent_actions_ = 10;

  // Weak pointer factory for asynchronous suggestion requests.
  // TODO(astra): Use this for async tab search / workspace search when
  // those operations become asynchronous (e.g. with many tabs).
  base::WeakPtrFactory<AstraOmniboxManager> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_OMNIBOX_MANAGER_H_
