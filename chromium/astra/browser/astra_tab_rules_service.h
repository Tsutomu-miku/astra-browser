// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_TAB_RULES_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_TAB_RULES_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

// =========================================================================
// AstraTabRule — auto-action rule definition
// =========================================================================
//
// Tab rules let users define auto-actions that trigger when a tab matches
// conditions.  Each rule has a trigger (when it fires) and an action
// (what happens when it fires).
//
// Truth model:
//   - Rule definitions live in this service (profile-scoped, persisted
//     via PrefService).
//   - Rule matching is evaluated against tab state (URL, title) owned by
//     Chromium's WebContents / TabStripModel.
//   - Actions are dispatched to Chromium subsystems (TabStripModel,
//     AstraWorkspaceService, AstraFavoriteService, etc.) — this service
//     never owns tab state directly.
//
// Chromium subsystems reused:
//   - PrefService (for rule persistence).
//   - WebContents / TabStripModel (tab state truth source).
//   - ProfileKeyedServiceFactory pattern.
//
// Chromium patch points:
//   - Tab creation / navigation: to evaluate rules against new tab state.
//     Patch point: chrome/browser/ui/tabs/tab_strip_model.cc or
//     content::WebContentsObserver for DidFinishNavigation.
//   - Profile keyed service registration: to wire up the factory.
//     Patch point: chrome/browser/profiles/profile_keyed_service_factory*.
// =========================================================================

// Trigger type determines what condition causes a rule to fire.
enum class AstraTabRuleTriggerType {
  // Rule matches when the tab URL matches a wildcard pattern.
  // trigger_value is a URL pattern with * wildcards, e.g. "*://*.youtube.com/*".
  kUrlPattern,

  // Rule matches when the tab's domain matches exactly.
  // trigger_value is a domain, e.g. "github.com".
  kDomain,

  // Rule matches when the tab title contains a keyword.
  // trigger_value is the keyword string (case-insensitive).
  kTitleContains,

  // Rule triggers when a new tab is opened.
  // trigger_value is unused / empty for this trigger type.
  kTabOpen,

  // Rule triggers at a specific time of day.
  // trigger_value is "HH:MM" in 24-hour format, e.g. "09:00".
  kTimeOfDay,

  // Rule triggers when the user has been idle for N minutes.
  // trigger_value is the idle threshold in minutes, e.g. "30".
  kIdle,
};

// Action type determines what happens when a rule triggers.
enum class AstraTabRuleActionType {
  // Move the matching tab to a workspace.
  // action_value is the target workspace_id.
  kMoveToWorkspace,

  // Add the matching tab to favorites.
  // action_value is optional: favorite_folder_id, or empty for root.
  kAddToFavorites,

  // Mute the matching tab.
  // action_value is unused.
  kMute,

  // Pin the matching tab.
  // action_value is unused.
  kPin,

  // Snooze the tab (close and schedule reopen).
  // action_value is the snooze duration in minutes, e.g. "60".
  kSnooze,

  // Close the matching tab.
  // action_value is unused.
  kClose,

  // Add a label to the tab.
  // action_value is the label_id.
  kAddLabel,

  // Hibernate the tab (unload from memory, keep metadata).
  // action_value is unused.
  kHibernate,
};

struct AstraTabRule {
  // Unique identifier for this rule.
  std::string id;

  // Human-readable name for the rule (shown in settings UI).
  std::string name;

  // Whether the rule is currently active.  Disabled rules are never matched.
  bool enabled = true;

  // What triggers the rule.
  AstraTabRuleTriggerType trigger_type = AstraTabRuleTriggerType::kUrlPattern;

  // Parameter for the trigger.  Meaning depends on trigger_type.
  std::string trigger_value;

  // What action to take when the rule triggers.
  AstraTabRuleActionType action_type = AstraTabRuleActionType::kMoveToWorkspace;

  // Parameter for the action.  Meaning depends on action_type.
  std::string action_value;

  // Priority of the rule.  Lower numbers = higher priority.
  // When multiple rules match a tab, they are applied in priority order.
  int priority = 100;

  // When the rule was created.
  base::Time created_time;

  // When the rule last triggered (fired and executed its action).
  // Null if the rule has never triggered.
  base::Time last_triggered_time;

  // How many times the rule has triggered since creation.
  int trigger_count = 0;
};

// =========================================================================
// AstraTabRulesServiceObserver
// =========================================================================
//
// Observer interface for UI layers (settings, sidebar, notifications) to
// react to rule changes and triggers.  UI must never be the source of truth
// — AstraTabRulesService is.
// =========================================================================

class AstraTabRulesServiceObserver : public base::CheckedObserver {
 public:
  // Called after a new rule is added to the service.
  virtual void OnRuleAdded(const std::string& rule_id) {}

  // Called after a rule is removed.
  virtual void OnRuleRemoved(const std::string& rule_id) {}

  // Called after a rule's properties (name, trigger, action, priority,
  // enabled state) are updated.
  virtual void OnRuleUpdated(const std::string& rule_id) {}

  // Called after a rule has triggered and executed its action on a tab.
  // |tab_id| identifies the tab that matched.
  // TODO(astra): Define the tab_id format.  Options:
  //   1. SessionID (from sessions::SessionTabHelper).
  //   2. WebContents* pointer (not stable across navigations).
  //   3. AstraTabFeatures unique ID.
  // Chromium owner: TabStripModel / sessions::SessionTabHelper.
  virtual void OnRuleTriggered(const std::string& rule_id,
                               const std::string& tab_id) {}

  // Called after all rules are cleared.
  virtual void OnRulesCleared() {}

 protected:
  ~AstraTabRulesServiceObserver() override = default;
};

// =========================================================================
// AstraTabRulesService
// =========================================================================
//
// Profile-scoped keyed service that owns all Astra tab rule definitions
// and provides rule matching and execution.
//
// Truth source for:
//   - Rule definitions (trigger, action, priority, enabled state).
//   - Rule trigger statistics (last_triggered_time, trigger_count).
//
// Not owned here:
//   - Tabs / WebContents (Chromium TabStripModel owns them).
//   - Workspaces (AstraWorkspaceService owns them).
//   - Favorites (AstraFavoriteService owns folder metadata).
//   - Mute, pin, close tab actions (Chromium TabStripModel / WebContents).
//
// Persistence:
//   TODO(astra): Persist rules via Chromium PrefService (PrefRegistry).
//   Do NOT add a custom file-based storage layer — always go through the
//   profile's PrefService so rule state participates in profile
//   lifecycle, sync, and policy correctly.
// =========================================================================

class AstraTabRulesService final : public KeyedService {
 public:
  explicit AstraTabRulesService(Profile* profile);
  AstraTabRulesService(const AstraTabRulesService&) = delete;
  AstraTabRulesService& operator=(const AstraTabRulesService&) = delete;
  ~AstraTabRulesService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraTabRulesServiceObserver* observer);
  void RemoveObserver(AstraTabRulesServiceObserver* observer);

  // -- Rule CRUD ---------------------------------------------------------

  // Returns all rules, sorted by priority (lowest first = highest priority).
  const std::vector<AstraTabRule>& rules() const { return rules_; }

  // Returns the number of rules.
  size_t rule_count() const { return rules_.size(); }

  // Returns the rule with the given id, or nullptr if not found.
  const AstraTabRule* GetRule(const std::string& id) const;

  // Returns all enabled rules, sorted by priority.
  std::vector<AstraTabRule> GetEnabledRules() const;

  // Adds a new rule.  The rule gets an auto-generated id if empty,
  // and default values for created_time and priority if not set.
  // Returns the id of the added rule, or empty string on failure.
  // Fires OnRuleAdded.
  std::string AddRule(AstraTabRule rule);

  // Removes the rule with the given id.
  // Returns true if the rule existed and was removed.
  // Fires OnRuleRemoved.
  bool RemoveRule(const std::string& id);

  // Updates a rule's properties (name, trigger_type/value, action_type/value,
  // priority, enabled).  The rule's id and created_time are immutable.
  // Returns true if the rule existed and was updated.
  // Fires OnRuleUpdated.
  bool UpdateRule(const std::string& id, const AstraTabRule& updated_rule);

  // -- Rule management ---------------------------------------------------

  // Enables a rule.  Returns true if the rule existed and was disabled
  // before this call.
  // Fires OnRuleUpdated.
  bool EnableRule(const std::string& id);

  // Disables a rule.  Returns true if the rule existed and was enabled
  // before this call.
  // Fires OnRuleUpdated.
  bool DisableRule(const std::string& id);

  // Toggles a rule's enabled state.  Returns the new state, or nullopt
  // if the rule was not found.
  // Fires OnRuleUpdated if the state changed.
  std::optional<bool> ToggleRule(const std::string& id);

  // Sets a rule's priority.  Lower number = higher priority.
  // Returns true if the rule existed and the priority changed.
  // Fires OnRuleUpdated if changed.
  bool SetRulePriority(const std::string& id, int priority);

  // -- Rule matching -----------------------------------------------------

  // Returns true if |tab_url| and |tab_title| satisfy |rule|'s trigger.
  // Only URL/domain/title trigger types are evaluated by this method —
  // time-based and idle triggers are checked by separate timers.
  //
  // URL pattern matching supports * wildcards:
  //   - * matches any sequence of characters
  //   - Patterns are case-insensitive
  //
  // Domain matching checks the tab's host against the rule's domain:
  //   - Exact match (github.com == github.com)
  //   - Subdomain match (www.github.com matches github.com)
  //
  // Title matching is case-insensitive substring matching.
  static bool DoesTabMatchRule(const GURL& tab_url,
                               const std::u16string& tab_title,
                               const AstraTabRule& rule);

  // Returns all enabled rules that match the given tab state, sorted by
  // priority (highest priority first).
  std::vector<AstraTabRule> GetMatchingRules(
      const GURL& tab_url,
      const std::u16string& tab_title) const;

  // -- Trigger execution -------------------------------------------------

  // Executes the rule's action on the specified tab.
  // Updates last_triggered_time and trigger_count on the rule.
  // Fires OnRuleTriggered.
  //
  // TODO(astra): This is a stub that dispatches to the appropriate
  // Chromium / Astra service.  Real implementation needs:
  //   - kMoveToWorkspace: AstraWorkspaceService + TabStripModel
  //   - kAddToFavorites: AstraFavoriteService
  //   - kMute: content::WebContents::SetAudioMuted
  //   - kPin: TabStripModel::ToggleTabPinnedState
  //   - kSnooze: AstraSnoozeService (future)
  //   - kClose: TabStripModel::CloseWebContentsAt
  //   - kAddLabel: AstraLabelService (future)
  //   - kHibernate: AstraMemorySaverService
  //
  // Chromium patch point: tab trigger evaluation should be wired into
  //   WebContentsObserver::DidFinishNavigation so rules are re-evaluated
  //   whenever a tab navigates.
  void TriggerRuleForTab(const std::string& rule_id, const std::string& tab_id);

  // -- Bulk operations ---------------------------------------------------

  // Removes all rules.
  // Fires OnRulesCleared.
  void ClearRules();

  // Enables all rules.  Returns the number of rules whose state changed.
  // Fires OnRuleUpdated for each rule that was enabled.
  size_t EnableAllRules();

  // Disables all rules.  Returns the number of rules whose state changed.
  // Fires OnRuleUpdated for each rule that was disabled.
  size_t DisableAllRules();

  // -- Import / Export ---------------------------------------------------

  // Exports all rules as a JSON string.
  // Returns a JSON string with all rule data.
  // TODO(astra): Use base::JSONWriter for proper JSON serialization.
  // Chromium component: base/json/json_writer.h
  std::string ExportRulesJson() const;

  // Imports rules from a JSON string.
  // If |merge| is true, imported rules are added to existing rules
  // (with unique IDs generated to avoid conflicts).
  // If |merge| is false, existing rules are replaced.
  // Returns the number of rules imported.
  // Fires OnRuleAdded (and OnRulesCleared if merge=false) as appropriate.
  // TODO(astra): Use base::JSONReader for proper JSON parsing.
  // Chromium component: base/json/json_reader.h
  size_t ImportRulesJson(const std::string& json, bool merge = false);

  // -- Sample rules ------------------------------------------------------

  // Returns a list of built-in sample rules that users can enable or use
  // as templates.  These are not automatically added — the UI should
  // present them as "suggested rules" and let the user add them.
  static std::vector<AstraTabRule> GetSampleRules();

  // Adds all sample rules to the service.  Useful for onboarding.
  // Returns the number of rules added.
  // Fires OnRuleAdded for each sample rule added.
  size_t AddSampleRules();

  // -- Incognito compatibility -------------------------------------------

  // Returns true if this service instance is associated with an incognito
  // (off-the-record) profile.
  //
  // Note: because the factory uses kRedirectedToOriginal for incognito,
  // this will return false even when called from an incognito context —
  // the service instance belongs to the original profile.
  bool IsIncognito() const;

 private:
  // Non-const lookup helper for internal use.
  AstraTabRule* FindRule(const std::string& id);

  // Sorts rules_ by priority in ascending order (lower = higher priority).
  void SortRules();

  // Generates a unique rule ID.
  // Uses base::UnguessableToken for uniqueness.
  static std::string GenerateRuleId();

  // Loads rule state from the profile's PrefService.  Called from the
  // constructor.  If no persisted state exists (fresh profile), no rules
  // are loaded.
  //
  // Chromium component: PrefService.
  // All persistence goes through PrefService — no custom file I/O.
  // TODO(astra): Implement once pref keys are registered.
  // Patch point: astra_prefs.h + PrefService.
  void LoadFromPrefs();

  // Persists current rule state to the profile's PrefService.
  // Called from every mutation method.  PrefService handles deferred
  // disk writes internally.
  //
  // Chromium component: PrefService + PrefRegistry.
  // TODO(astra): Implement once pref keys are registered.
  // Patch point: astra_prefs.h + PrefService.
  void SaveToPrefs();

  raw_ptr<Profile> profile_;
  std::vector<AstraTabRule> rules_;
  base::ObserverList<AstraTabRulesServiceObserver> observers_;
};

// =========================================================================
// AstraTabRulesServiceFactory
// =========================================================================
//
// Factory for AstraTabRulesService.
//
// Incognito behavior: the factory uses kRedirectedToOriginal for regular
// incognito profiles because tab rules are a product-level concern that
// should reflect the user's main profile state.  A separate incognito
// window still uses the same rule set — only the browsing context is
// isolated.  Guest sessions get their own instance (kOwnInstance) because
// they have no backing profile to redirect to.
// =========================================================================

class AstraTabRulesServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraTabRulesService* GetForProfile(Profile* profile);
  static AstraTabRulesServiceFactory* GetInstance();

  // Registers tab rules-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation.  Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraTabRulesServiceFactory();
  ~AstraTabRulesServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_RULES_SERVICE_H_
