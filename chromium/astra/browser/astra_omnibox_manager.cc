#include "astra/browser/astra_omnibox_manager.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_omnibox_action.h"
#include "astra/browser/astra_omnibox_provider.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Static map from profile pointer to manager instance.
//
// TODO(astra): Replace this with a proper ProfileKeyedServiceFactory
// pattern once the patch to register Astra profile-keyed services is in
// place.  The static map is a temporary stand-in that works for single
// profile testing but doesn't handle profile lifecycle (destruction,
// incognito redirect, etc.) correctly.
//
// Chromium pattern: ProfileKeyedServiceFactory.
using ManagerMap = std::map<Profile*, std::unique_ptr<AstraOmniboxManager>>;

ManagerMap& GetManagerMap() {
  static base::NoDestructor<ManagerMap> managers;
  return *managers;
}

// Helper: convert category to pref key suffix.
const char* CategoryToPrefSuffix(AstraOmniboxActionCategory category) {
  switch (category) {
    case AstraOmniboxActionCategory::kWorkspace:
      return "workspace";
    case AstraOmniboxActionCategory::kTab:
      return "tab";
    case AstraOmniboxActionCategory::kNavigation:
      return "navigation";
    case AstraOmniboxActionCategory::kTool:
      return "tool";
  }
  return "unknown";
}

// Clamps max suggestions to a valid range.
int ClampMaxSuggestions(int max) {
  if (max < 1) return 1;
  if (max > 20) return 20;
  return max;
}

// Clamps max recent actions to a valid range.
int ClampMaxRecentActions(int max) {
  if (max < 1) return 1;
  if (max > 50) return 50;
  return max;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraOmniboxManager::AstraOmniboxManager(Profile* profile)
    : profile_(profile),
      provider_(std::make_unique<AstraOmniboxProvider>()) {
  LoadFromPrefs();
}

AstraOmniboxManager::~AstraOmniboxManager() {
  observers_.Clear();
}

// =========================================================================
// Static accessors
// =========================================================================

AstraOmniboxManager* AstraOmniboxManager::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }

  ManagerMap& managers = GetManagerMap();
  auto it = managers.find(profile);
  if (it != managers.end()) {
    return it->second.get();
  }

  // Create a new manager for this profile.
  auto manager = std::make_unique<AstraOmniboxManager>(profile);
  AstraOmniboxManager* raw = manager.get();
  managers[profile] = std::move(manager);
  return raw;
}

AstraOmniboxManager* AstraOmniboxManager::GetForBrowser(Browser* browser) {
  if (!browser || !browser->profile()) {
    return nullptr;
  }
  return GetForProfile(browser->profile());
}

// =========================================================================
// Observers
// =========================================================================

void AstraOmniboxManager::AddObserver(Observer* observer) {
  if (observer) {
    observers_.AddObserver(observer);
  }
}

void AstraOmniboxManager::RemoveObserver(Observer* observer) {
  if (observer) {
    observers_.RemoveObserver(observer);
  }
}

// =========================================================================
// Suggestions
// =========================================================================

std::vector<AstraOmniboxSuggestion> AstraOmniboxManager::GetSuggestions(
    Browser* browser,
    const std::u16string& text) {
  if (!provider_enabled_ || !show_astra_suggestions_ || !provider_) {
    return {};
  }

  std::vector<AstraOmniboxSuggestion> all =
      provider_->GetSuggestions(browser, text);

  // Filter by category enablement.
  if (!category_workspace_enabled_ || !category_tab_enabled_ ||
      !category_navigation_enabled_ || !category_tool_enabled_) {
    std::vector<AstraOmniboxSuggestion> filtered;
    for (auto& suggestion : all) {
      AstraOmniboxActionCategory cat =
          GetActionCategory(suggestion.action_type);
      if (IsCategoryEnabled(cat)) {
        filtered.push_back(std::move(suggestion));
      }
    }
    all = std::move(filtered);
  }

  // Cap at max_astra_suggestions_.
  if (static_cast<int>(all.size()) > max_astra_suggestions_) {
    all.resize(static_cast<size_t>(max_astra_suggestions_));
  }

  return all;
}

bool AstraOmniboxManager::MatchesAstraPrefix(
    const std::u16string& text) const {
  if (!provider_) {
    return false;
  }
  return provider_->MatchesQuery(text);
}

// =========================================================================
// Action execution
// =========================================================================

bool AstraOmniboxManager::ExecuteAction(Browser* browser,
                                        AstraOmniboxActionType action_type,
                                        const std::string& payload) {
  bool success = ExecuteAstraOmniboxAction(browser, action_type, payload);

  // Record in recent actions if execution was attempted (even on failure,
  // to track user intent).  We only track actions that are valid types.
  if (action_type != AstraOmniboxActionType::kNone) {
    AddRecentAction(action_type, payload);
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnOmniboxActionExecuted(action_type, payload, success);
  }

  return success;
}

bool AstraOmniboxManager::ExecuteSuggestion(
    Browser* browser,
    const AstraOmniboxSuggestion& suggestion) {
  return ExecuteAction(browser, suggestion.action_type, suggestion.payload);
}

// =========================================================================
// Provider enabled state
// =========================================================================

void AstraOmniboxManager::SetProviderEnabled(bool enabled) {
  if (provider_enabled_ == enabled) {
    return;
  }

  provider_enabled_ = enabled;
  SaveProviderEnabledToPrefs();

  for (auto& observer : observers_) {
    observer.OnProviderEnabledChanged(enabled);
  }

  NotifySuggestionsChanged();
}

// =========================================================================
// Presentation settings
// =========================================================================

void AstraOmniboxManager::SetShowAstraSuggestions(bool show) {
  if (show_astra_suggestions_ == show) {
    return;
  }

  show_astra_suggestions_ = show;
  SaveShowAstraSuggestionsToPrefs();

  NotifySettingsChanged();
  NotifySuggestionsChanged();
}

void AstraOmniboxManager::SetMaxAstraSuggestions(int max) {
  int clamped = ClampMaxSuggestions(max);
  if (max_astra_suggestions_ == clamped) {
    return;
  }

  max_astra_suggestions_ = clamped;
  SaveMaxAstraSuggestionsToPrefs();

  NotifySettingsChanged();
  NotifySuggestionsChanged();
}

void AstraOmniboxManager::SetSuggestionPosition(const std::string& position) {
  if (position != "top" && position != "bottom") {
    return;
  }
  if (suggestion_position_ == position) {
    return;
  }

  suggestion_position_ = position;
  SaveSuggestionPositionToPrefs();

  NotifySettingsChanged();
}

// =========================================================================
// Category filtering
// =========================================================================

bool AstraOmniboxManager::IsCategoryEnabled(
    AstraOmniboxActionCategory category) const {
  switch (category) {
    case AstraOmniboxActionCategory::kWorkspace:
      return category_workspace_enabled_;
    case AstraOmniboxActionCategory::kTab:
      return category_tab_enabled_;
    case AstraOmniboxActionCategory::kNavigation:
      return category_navigation_enabled_;
    case AstraOmniboxActionCategory::kTool:
      return category_tool_enabled_;
  }
  return true;
}

void AstraOmniboxManager::SetCategoryEnabled(AstraOmniboxActionCategory category,
                                             bool enabled) {
  bool* flag = nullptr;
  switch (category) {
    case AstraOmniboxActionCategory::kWorkspace:
      flag = &category_workspace_enabled_;
      break;
    case AstraOmniboxActionCategory::kTab:
      flag = &category_tab_enabled_;
      break;
    case AstraOmniboxActionCategory::kNavigation:
      flag = &category_navigation_enabled_;
      break;
    case AstraOmniboxActionCategory::kTool:
      flag = &category_tool_enabled_;
      break;
  }
  if (!flag || *flag == enabled) {
    return;
  }

  *flag = enabled;
  SaveCategoriesToPrefs();

  NotifySuggestionsChanged();
}

void AstraOmniboxManager::EnableAllCategories() {
  if (category_workspace_enabled_ && category_tab_enabled_ &&
      category_navigation_enabled_ && category_tool_enabled_) {
    return;
  }

  category_workspace_enabled_ = true;
  category_tab_enabled_ = true;
  category_navigation_enabled_ = true;
  category_tool_enabled_ = true;
  SaveCategoriesToPrefs();

  NotifySuggestionsChanged();
}

void AstraOmniboxManager::DisableAllCategories() {
  if (!category_workspace_enabled_ && !category_tab_enabled_ &&
      !category_navigation_enabled_ && !category_tool_enabled_) {
    return;
  }

  category_workspace_enabled_ = false;
  category_tab_enabled_ = false;
  category_navigation_enabled_ = false;
  category_tool_enabled_ = false;
  SaveCategoriesToPrefs();

  NotifySuggestionsChanged();
}

// =========================================================================
// Recent actions
// =========================================================================

std::vector<AstraOmniboxActionType> AstraOmniboxManager::GetRecentActions()
    const {
  std::vector<AstraOmniboxActionType> result;
  for (const auto& entry : recent_actions_) {
    result.push_back(entry.first);
  }
  return result;
}

std::vector<AstraOmniboxAction> AstraOmniboxManager::GetRecentActionDetails()
    const {
  std::vector<AstraOmniboxAction> result;
  for (const auto& entry : recent_actions_) {
    AstraOmniboxAction action;
    if (GetActionMetadata(entry.first, &action)) {
      // Use the stored payload as default if it's not empty.
      if (!entry.second.empty()) {
        action.default_payload = entry.second;
      }
      result.push_back(action);
    }
  }
  return result;
}

void AstraOmniboxManager::SetMaxRecentActions(int max) {
  int clamped = ClampMaxRecentActions(max);
  if (max_recent_actions_ == clamped) {
    return;
  }

  max_recent_actions_ = clamped;
  SaveMaxRecentActionsToPrefs();

  // Truncate list if necessary.
  if (recent_actions_.size() > static_cast<size_t>(max_recent_actions_)) {
    recent_actions_.resize(static_cast<size_t>(max_recent_actions_));
    SaveRecentActionsToPrefs();
  }

  for (auto& observer : observers_) {
    observer.OnRecentActionsChanged();
  }
}

void AstraOmniboxManager::AddRecentAction(AstraOmniboxActionType action_type,
                                          const std::string& payload) {
  if (action_type == AstraOmniboxActionType::kNone) {
    return;
  }

  // Remove existing entry if present (move to front).
  auto it = std::find_if(recent_actions_.begin(), recent_actions_.end(),
                         [action_type](const auto& entry) {
                           return entry.first == action_type;
                         });
  if (it != recent_actions_.end()) {
    recent_actions_.erase(it);
  }

  // Insert at front.
  recent_actions_.insert(recent_actions_.begin(),
                         std::make_pair(action_type, payload));

  // Truncate if over max.
  if (recent_actions_.size() > static_cast<size_t>(max_recent_actions_)) {
    recent_actions_.resize(static_cast<size_t>(max_recent_actions_));
  }

  SaveRecentActionsToPrefs();

  for (auto& observer : observers_) {
    observer.OnRecentActionsChanged();
  }
}

void AstraOmniboxManager::ClearRecentActions() {
  if (recent_actions_.empty()) {
    return;
  }

  recent_actions_.clear();
  SaveRecentActionsToPrefs();

  for (auto& observer : observers_) {
    observer.OnRecentActionsChanged();
  }
}

// =========================================================================
// Action catalog access
// =========================================================================

std::vector<AstraOmniboxAction> AstraOmniboxManager::GetAllActions() const {
  return GetAllOmniboxActions();
}

std::vector<AstraOmniboxAction> AstraOmniboxManager::GetActionsByCategory(
    AstraOmniboxActionCategory category) const {
  return astra::GetActionsByCategory(category);
}

std::vector<AstraOmniboxAction> AstraOmniboxManager::SearchActions(
    const std::u16string& query) const {
  return astra::SearchActions(query);
}

// =========================================================================
// Prefs loading / saving
// =========================================================================

void AstraOmniboxManager::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  // Provider enabled.
  provider_enabled_ = prefs->GetBoolean(prefs::kPrefOmniboxProviderEnabled);

  // Presentation settings.
  show_astra_suggestions_ =
      prefs->GetBoolean(prefs::kPrefOmniboxShowAstraSuggestions);
  max_astra_suggestions_ = ClampMaxSuggestions(
      prefs->GetInteger(prefs::kPrefOmniboxMaxAstraSuggestions));
  suggestion_position_ =
      prefs->GetString(prefs::kPrefOmniboxSuggestionPosition);

  // Category enablement.
  LoadCategoriesFromPrefs();

  // Recent actions.
  max_recent_actions_ = ClampMaxRecentActions(
      prefs->GetInteger(prefs::kPrefOmniboxMaxRecentActions));
  LoadRecentActionsFromPrefs();
}

void AstraOmniboxManager::SaveSettingsToPrefs() const {
  SaveProviderEnabledToPrefs();
  SaveShowAstraSuggestionsToPrefs();
  SaveMaxAstraSuggestionsToPrefs();
  SaveSuggestionPositionToPrefs();
  SaveCategoriesToPrefs();
  SaveRecentActionsToPrefs();
  SaveMaxRecentActionsToPrefs();
}

void AstraOmniboxManager::SaveProviderEnabledToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(prefs::kPrefOmniboxProviderEnabled,
                                    provider_enabled_);
}

void AstraOmniboxManager::SaveShowAstraSuggestionsToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(prefs::kPrefOmniboxShowAstraSuggestions,
                                    show_astra_suggestions_);
}

void AstraOmniboxManager::SaveMaxAstraSuggestionsToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetInteger(prefs::kPrefOmniboxMaxAstraSuggestions,
                                    max_astra_suggestions_);
}

void AstraOmniboxManager::SaveSuggestionPositionToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetString(prefs::kPrefOmniboxSuggestionPosition,
                                   suggestion_position_);
}

void AstraOmniboxManager::SaveCategoriesToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  // Use a dict to store category enablement states.
  base::Value::Dict dict;
  dict.Set(CategoryToPrefSuffix(AstraOmniboxActionCategory::kWorkspace),
           category_workspace_enabled_);
  dict.Set(CategoryToPrefSuffix(AstraOmniboxActionCategory::kTab),
           category_tab_enabled_);
  dict.Set(CategoryToPrefSuffix(AstraOmniboxActionCategory::kNavigation),
           category_navigation_enabled_);
  dict.Set(CategoryToPrefSuffix(AstraOmniboxActionCategory::kTool),
           category_tool_enabled_);
  profile_->GetPrefs()->SetDict(prefs::kPrefOmniboxCategoryEnabled,
                                 std::move(dict));
}

void AstraOmniboxManager::SaveRecentActionsToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  base::Value::List list;
  for (const auto& entry : recent_actions_) {
    base::Value::Dict dict;
    dict.Set("action_type",
             static_cast<int>(entry.first));
    dict.Set("payload", entry.second);
    list.Append(std::move(dict));
  }
  profile_->GetPrefs()->SetList(prefs::kPrefOmniboxRecentActions,
                                 std::move(list));
}

void AstraOmniboxManager::SaveMaxRecentActionsToPrefs() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetInteger(prefs::kPrefOmniboxMaxRecentActions,
                                    max_recent_actions_);
}

void AstraOmniboxManager::LoadCategoriesFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  const base::Value::Dict& dict =
      profile_->GetPrefs()->GetDict(prefs::kPrefOmniboxCategoryEnabled);

  absl::optional<bool> workspace =
      dict.FindBool(CategoryToPrefSuffix(AstraOmniboxActionCategory::kWorkspace));
  if (workspace.has_value()) {
    category_workspace_enabled_ = *workspace;
  }

  absl::optional<bool> tab =
      dict.FindBool(CategoryToPrefSuffix(AstraOmniboxActionCategory::kTab));
  if (tab.has_value()) {
    category_tab_enabled_ = *tab;
  }

  absl::optional<bool> navigation = dict.FindBool(
      CategoryToPrefSuffix(AstraOmniboxActionCategory::kNavigation));
  if (navigation.has_value()) {
    category_navigation_enabled_ = *navigation;
  }

  absl::optional<bool> tool =
      dict.FindBool(CategoryToPrefSuffix(AstraOmniboxActionCategory::kTool));
  if (tool.has_value()) {
    category_tool_enabled_ = *tool;
  }
}

void AstraOmniboxManager::LoadRecentActionsFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  const base::Value::List& list =
      profile_->GetPrefs()->GetList(prefs::kPrefOmniboxRecentActions);
  recent_actions_.clear();

  for (const auto& item : list) {
    if (!item.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = item.GetDict();
    absl::optional<int> type_val = dict.FindInt("action_type");
    if (!type_val.has_value()) {
      continue;
    }
    const std::string* payload = dict.FindString("payload");
    AstraOmniboxActionType type =
        static_cast<AstraOmniboxActionType>(*type_val);
    recent_actions_.push_back(
        std::make_pair(type, payload ? *payload : std::string()));

    if (recent_actions_.size() >=
        static_cast<size_t>(max_recent_actions_)) {
      break;
    }
  }
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraOmniboxManager::NotifySuggestionsChanged() {
  for (auto& observer : observers_) {
    observer.OnSuggestionsChanged();
  }
}

void AstraOmniboxManager::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnOmniboxSettingsChanged();
  }
}

}  // namespace astra
