// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_tab_rules_service.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Sample rule constants
// ---------------------------------------------------------------------------

constexpr char kSampleMuteVideoSitesId[] = "sample-mute-video-sites";
constexpr char kSampleMuteVideoSitesName[] = "Mute video sites";
constexpr char kSampleWorkDomainsId[] = "sample-work-domains";
constexpr char kSampleWorkDomainsName[] = "Move work domains to Work workspace";
constexpr char kSampleNewsSitesId[] = "sample-news-focus";
constexpr char kSampleNewsSitesName[] = "Pin frequently visited sites";
constexpr char kSampleSocialHibernateId[] = "sample-social-hibernate";
constexpr char kSampleSocialHibernateName[] = "Hibernate social media tabs";

// Default workspace id for the "Work" workspace referenced by sample rules.
// TODO(astra): Reference the actual default workspace IDs from
// AstraWorkspaceService.  For sample rules we use a well-known string.
constexpr char kWorkWorkspaceId[] = "work";

// ---------------------------------------------------------------------------
// URL pattern matching helpers
// ---------------------------------------------------------------------------

// Matches a pattern with * wildcards against a string.
// Case-insensitive.
bool WildcardMatch(const std::string& pattern, const std::string& text) {
  // Simple DP-based wildcard matching.
  // * matches any sequence of characters (including empty).
  std::string pattern_lower = base::ToLowerASCII(pattern);
  std::string text_lower = base::ToLowerASCII(text);

  size_t p = 0;   // pattern position
  size_t t = 0;   // text position
  size_t star_p = std::string::npos;  // last * position in pattern
  size_t star_t = 0;  // corresponding text position

  while (t < text_lower.size()) {
    if (p < pattern_lower.size() &&
        (pattern_lower[p] == text_lower[t] || pattern_lower[p] == '?')) {
      p++;
      t++;
    } else if (p < pattern_lower.size() && pattern_lower[p] == '*') {
      star_p = p;
      star_t = t;
      p++;
    } else if (star_p != std::string::npos) {
      p = star_p + 1;
      star_t++;
      t = star_t;
    } else {
      return false;
    }
  }

  // Consume remaining * in pattern
  while (p < pattern_lower.size() && pattern_lower[p] == '*') {
    p++;
  }

  return p == pattern_lower.size();
}

// Checks if |host| matches |domain|.
// Matches if:
//   - host == domain (exact match)
//   - host ends with "." + domain (subdomain match)
bool DomainMatch(const std::string& domain, const std::string& host) {
  std::string domain_lower = base::ToLowerASCII(domain);
  std::string host_lower = base::ToLowerASCII(host);

  if (host_lower == domain_lower) {
    return true;
  }

  // Subdomain match: host must end with "." + domain
  std::string suffix = "." + domain_lower;
  if (host_lower.size() > suffix.size() &&
      host_lower.substr(host_lower.size() - suffix.size()) == suffix) {
    return true;
  }

  return false;
}

// Case-insensitive substring match for title.
bool TitleContains(const std::string& keyword, const std::u16string& title) {
  if (keyword.empty()) {
    return false;
  }
  std::u16string keyword_16 = base::UTF8ToUTF16(keyword);
  return base::ToLowerASCII(title).find(base::ToLowerASCII(keyword_16)) !=
         std::u16string::npos;
}

}  // namespace

// ===========================================================================
// AstraTabRulesService
// ===========================================================================

AstraTabRulesService::AstraTabRulesService(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Load persisted rules from PrefService.
  // For now, start with an empty rule list.
  LoadFromPrefs();
}

AstraTabRulesService::~AstraTabRulesService() = default;

void AstraTabRulesService::Shutdown() {
  // KeyedService shutdown: clear all observer references and drop profile
  // pointer before the profile goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraTabRulesService::AddObserver(
    AstraTabRulesServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabRulesService::RemoveObserver(
    AstraTabRulesServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Rule CRUD ---------------------------------------------------------------

const AstraTabRule* AstraTabRulesService::GetRule(const std::string& id) const {
  auto it = base::ranges::find(rules_, id, &AstraTabRule::id);
  return it == rules_.end() ? nullptr : &(*it);
}

std::vector<AstraTabRule> AstraTabRulesService::GetEnabledRules() const {
  std::vector<AstraTabRule> enabled;
  for (const auto& rule : rules_) {
    if (rule.enabled) {
      enabled.push_back(rule);
    }
  }
  return enabled;
}

std::string AstraTabRulesService::AddRule(AstraTabRule rule) {
  // Disallow duplicate ids.
  if (!rule.id.empty() && GetRule(rule.id)) {
    return std::string();
  }

  // Auto-generate id if not provided.
  if (rule.id.empty()) {
    rule.id = GenerateRuleId();
  }

  // Set default created_time if not set.
  if (rule.created_time.is_null()) {
    rule.created_time = base::Time::Now();
  }

  rules_.push_back(std::move(rule));
  SortRules();

  // Find the newly added rule (after sorting it may have moved).
  const std::string& added_id = rules_.back().id;

  // TODO(astra): In a sorted vector we need to find by id, not assume
  // it's at the end.  The SortRules() call reorders, so we need to find
  // the element we just added.
  const AstraTabRule* added = GetRule(added_id);
  DCHECK(added);

  for (auto& observer : observers_) {
    observer.OnRuleAdded(added_id);
  }

  SaveToPrefs();

  return added_id;
}

bool AstraTabRulesService::RemoveRule(const std::string& id) {
  auto it = base::ranges::find(rules_, id, &AstraTabRule::id);
  if (it == rules_.end()) {
    return false;
  }

  rules_.erase(it);

  for (auto& observer : observers_) {
    observer.OnRuleRemoved(id);
  }

  SaveToPrefs();

  return true;
}

bool AstraTabRulesService::UpdateRule(const std::string& id,
                                      const AstraTabRule& updated_rule) {
  AstraTabRule* rule = FindRule(id);
  if (!rule) {
    return false;
  }

  // Immutable fields: id and created_time.
  // Update everything else.
  rule->name = updated_rule.name;
  rule->enabled = updated_rule.enabled;
  rule->trigger_type = updated_rule.trigger_type;
  rule->trigger_value = updated_rule.trigger_value;
  rule->action_type = updated_rule.action_type;
  rule->action_value = updated_rule.action_value;
  rule->priority = updated_rule.priority;
  // last_triggered_time and trigger_count are not overwritten by UpdateRule.

  SortRules();

  for (auto& observer : observers_) {
    observer.OnRuleUpdated(id);
  }

  SaveToPrefs();

  return true;
}

// -- Rule management ---------------------------------------------------------

bool AstraTabRulesService::EnableRule(const std::string& id) {
  AstraTabRule* rule = FindRule(id);
  if (!rule) {
    return false;
  }

  if (rule->enabled) {
    return false;
  }

  rule->enabled = true;

  for (auto& observer : observers_) {
    observer.OnRuleUpdated(id);
  }

  SaveToPrefs();

  return true;
}

bool AstraTabRulesService::DisableRule(const std::string& id) {
  AstraTabRule* rule = FindRule(id);
  if (!rule) {
    return false;
  }

  if (!rule->enabled) {
    return false;
  }

  rule->enabled = false;

  for (auto& observer : observers_) {
    observer.OnRuleUpdated(id);
  }

  SaveToPrefs();

  return true;
}

std::optional<bool> AstraTabRulesService::ToggleRule(const std::string& id) {
  AstraTabRule* rule = FindRule(id);
  if (!rule) {
    return std::nullopt;
  }

  bool new_state = !rule->enabled;
  rule->enabled = new_state;

  for (auto& observer : observers_) {
    observer.OnRuleUpdated(id);
  }

  SaveToPrefs();

  return new_state;
}

bool AstraTabRulesService::SetRulePriority(const std::string& id, int priority) {
  AstraTabRule* rule = FindRule(id);
  if (!rule) {
    return false;
  }

  if (rule->priority == priority) {
    return false;
  }

  rule->priority = priority;
  SortRules();

  for (auto& observer : observers_) {
    observer.OnRuleUpdated(id);
  }

  SaveToPrefs();

  return true;
}

// -- Rule matching -----------------------------------------------------------

// static
bool AstraTabRulesService::DoesTabMatchRule(const GURL& tab_url,
                                            const std::u16string& tab_title,
                                            const AstraTabRule& rule) {
  if (!rule.enabled) {
    return false;
  }

  switch (rule.trigger_type) {
    case AstraTabRuleTriggerType::kUrlPattern:
      if (!tab_url.is_valid()) {
        return false;
      }
      return WildcardMatch(rule.trigger_value, tab_url.spec());

    case AstraTabRuleTriggerType::kDomain:
      if (!tab_url.is_valid() || !tab_url.has_host()) {
        return false;
      }
      return DomainMatch(rule.trigger_value, tab_url.host());

    case AstraTabRuleTriggerType::kTitleContains:
      return TitleContains(rule.trigger_value, tab_title);

    case AstraTabRuleTriggerType::kTabOpen:
      // kTabOpen is an event-based trigger, not a content match.
      // It should be evaluated by the tab creation observer, not by
      // DoesTabMatchRule.  Return false here for static matching.
      return false;

    case AstraTabRuleTriggerType::kTimeOfDay:
      // Time-based triggers are evaluated by a timer, not by content
      // matching.  Return false here.
      return false;

    case AstraTabRuleTriggerType::kIdle:
      // Idle triggers are evaluated by the idle detector, not by content
      // matching.  Return false here.
      return false;
  }

  return false;
}

std::vector<AstraTabRule> AstraTabRulesService::GetMatchingRules(
    const GURL& tab_url,
    const std::u16string& tab_title) const {
  std::vector<AstraTabRule> matching;

  // rules_ is already sorted by priority (ascending = highest priority first).
  for (const auto& rule : rules_) {
    if (!rule.enabled) {
      continue;
    }
    // Only check content-based triggers here.
    if (rule.trigger_type == AstraTabRuleTriggerType::kUrlPattern ||
        rule.trigger_type == AstraTabRuleTriggerType::kDomain ||
        rule.trigger_type == AstraTabRuleTriggerType::kTitleContains) {
      if (DoesTabMatchRule(tab_url, tab_title, rule)) {
        matching.push_back(rule);
      }
    }
  }

  return matching;
}

// -- Trigger execution -------------------------------------------------------

void AstraTabRulesService::TriggerRuleForTab(const std::string& rule_id,
                                             const std::string& tab_id) {
  AstraTabRule* rule = FindRule(rule_id);
  if (!rule) {
    return;
  }

  // Update trigger stats.
  rule->last_triggered_time = base::Time::Now();
  rule->trigger_count++;

  // Dispatch action.
  // TODO(astra): Implement real action dispatch to Chromium subsystems.
  // Each action type delegates to the appropriate Chromium or Astra service:
  //
  //   kMoveToWorkspace -> AstraWorkspaceService + TabStripModel
  //       Patch point: chrome/browser/ui/tabs/tab_strip_model.cc
  //       Use AstraTabFeatures::set_workspace_id() on the WebContents.
  //
  //   kAddToFavorites -> AstraFavoriteService
  //       Set AstraTabFeatures::set_is_favorite(true) and
  //       AstraTabFeatures::set_favorite_folder_id(action_value).
  //
  //   kMute -> content::WebContents::SetAudioMuted(true)
  //       Chromium owner: WebContents audio system.
  //
  //   kPin -> TabStripModel::ToggleTabPinnedState() or equivalent.
  //       Chromium owner: TabStripModel.
  //
  //   kSnooze -> Future AstraSnoozeService.
  //       Would close the tab and schedule a reopen timer.
  //
  //   kClose -> TabStripModel::CloseWebContentsAt().
  //       Chromium owner: TabStripModel.
  //
  //   kAddLabel -> Future AstraLabelService.
  //       Would add a label/tag to the tab's metadata.
  //
  //   kHibernate -> AstraMemorySaverService.
  //       Would discard the tab's WebContents to save memory.
  //
  // For now, all actions are stubs — we just update the trigger stats
  // and notify observers.

  switch (rule->action_type) {
    case AstraTabRuleActionType::kMoveToWorkspace:
      // TODO(astra): Call AstraWorkspaceService to move the tab.
      // Chromium patch point: TabStripModel + AstraTabFeatures.
      break;
    case AstraTabRuleActionType::kAddToFavorites:
      // TODO(astra): Call AstraFavoriteService to favorite the tab.
      // Chromium patch point: AstraFavoriteService + AstraTabFeatures.
      break;
    case AstraTabRuleActionType::kMute:
      // TODO(astra): Call content::WebContents::SetAudioMuted(true).
      // Chromium owner: content::WebContents audio system.
      break;
    case AstraTabRuleActionType::kPin:
      // TODO(astra): Call TabStripModel::SetTabPinned().
      // Chromium owner: TabStripModel.
      break;
    case AstraTabRuleActionType::kSnooze:
      // TODO(astra): Implement snooze via future AstraSnoozeService.
      break;
    case AstraTabRuleActionType::kClose:
      // TODO(astra): Call TabStripModel::CloseWebContentsAt().
      // Chromium owner: TabStripModel.
      break;
    case AstraTabRuleActionType::kAddLabel:
      // TODO(astra): Implement label via future AstraLabelService.
      break;
    case AstraTabRuleActionType::kHibernate:
      // TODO(astra): Call AstraMemorySaverService to hibernate the tab.
      // Chromium owner: AstraMemorySaverService + WebContents discard.
      break;
  }

  for (auto& observer : observers_) {
    observer.OnRuleTriggered(rule_id, tab_id);
  }

  SaveToPrefs();
}

// -- Bulk operations ---------------------------------------------------------

void AstraTabRulesService::ClearRules() {
  if (rules_.empty()) {
    return;
  }

  rules_.clear();

  for (auto& observer : observers_) {
    observer.OnRulesCleared();
  }

  SaveToPrefs();
}

size_t AstraTabRulesService::EnableAllRules() {
  size_t changed = 0;
  for (auto& rule : rules_) {
    if (!rule.enabled) {
      rule.enabled = true;
      changed++;
    }
  }

  if (changed > 0) {
    for (auto& observer : observers_) {
      for (const auto& rule : rules_) {
        observer.OnRuleUpdated(rule.id);
      }
    }
    SaveToPrefs();
  }

  return changed;
}

size_t AstraTabRulesService::DisableAllRules() {
  size_t changed = 0;
  for (auto& rule : rules_) {
    if (rule.enabled) {
      rule.enabled = false;
      changed++;
    }
  }

  if (changed > 0) {
    for (auto& observer : observers_) {
      for (const auto& rule : rules_) {
        observer.OnRuleUpdated(rule.id);
      }
    }
    SaveToPrefs();
  }

  return changed;
}

// -- Import / Export ---------------------------------------------------------

std::string AstraTabRulesService::ExportRulesJson() const {
  // TODO(astra): Use base::JSONWriter for proper JSON serialization.
  // Chromium component: base/json/json_writer.h.
  // For now, produce a simple JSON-like string as a placeholder.

  std::string result = "[\n";
  bool first = true;
  for (const auto& rule : rules_) {
    if (!first) {
      result += ",\n";
    }
    first = false;
    result += "  {\n";
    result += "    \"id\": \"" + rule.id + "\",\n";
    result += "    \"name\": \"" + rule.name + "\",\n";
    result += "    \"enabled\": " + std::string(rule.enabled ? "true" : "false") +
              ",\n";
    result += "    \"trigger_type\": " +
              std::to_string(static_cast<int>(rule.trigger_type)) + ",\n";
    result += "    \"trigger_value\": \"" + rule.trigger_value + "\",\n";
    result += "    \"action_type\": " +
              std::to_string(static_cast<int>(rule.action_type)) + ",\n";
    result += "    \"action_value\": \"" + rule.action_value + "\",\n";
    result += "    \"priority\": " + base::NumberToString(rule.priority) + ",\n";
    result += "    \"trigger_count\": " +
              base::NumberToString(rule.trigger_count) + "\n";
    result += "  }";
  }
  result += "\n]";

  return result;
}

size_t AstraTabRulesService::ImportRulesJson(const std::string& json,
                                             bool merge) {
  // TODO(astra): Use base::JSONReader for proper JSON parsing.
  // Chromium component: base/json/json_reader.h.
  // For now, this is a stub that returns 0.
  //
  // Expected JSON format (array of rule objects):
  // [
  //   {
  //     "id": "rule-id",
  //     "name": "Rule Name",
  //     "enabled": true,
  //     "trigger_type": 0,
  //     "trigger_value": "pattern",
  //     "action_type": 0,
  //     "action_value": "value",
  //     "priority": 100
  //   },
  //   ...
  // ]
  //
  // If merge is false, all existing rules are cleared first.
  // If merge is true, imported rules are added with new unique IDs
  // to avoid conflicts.

  if (!merge) {
    ClearRules();
  }

  // TODO(astra): Real parsing here.
  return 0;
}

// -- Sample rules ------------------------------------------------------------

// static
std::vector<AstraTabRule> AstraTabRulesService::GetSampleRules() {
  std::vector<AstraTabRule> samples;

  // Sample 1: Mute video sites
  {
    AstraTabRule rule;
    rule.id = kSampleMuteVideoSitesId;
    rule.name = kSampleMuteVideoSitesName;
    rule.enabled = false;
    rule.trigger_type = AstraTabRuleTriggerType::kDomain;
    rule.trigger_value = "youtube.com";
    rule.action_type = AstraTabRuleActionType::kMute;
    rule.priority = 50;
    rule.created_time = base::Time::Now();
    samples.push_back(std::move(rule));
  }

  // Sample 2: Move work domains to Work workspace
  {
    AstraTabRule rule;
    rule.id = kSampleWorkDomainsId;
    rule.name = kSampleWorkDomainsName;
    rule.enabled = false;
    rule.trigger_type = AstraTabRuleTriggerType::kDomain;
    rule.trigger_value = "github.com";
    rule.action_type = AstraTabRuleActionType::kMoveToWorkspace;
    rule.action_value = kWorkWorkspaceId;
    rule.priority = 30;
    rule.created_time = base::Time::Now();
    samples.push_back(std::move(rule));
  }

  // Sample 3: Pin frequently visited sites
  {
    AstraTabRule rule;
    rule.id = kSampleNewsSitesId;
    rule.name = kSampleNewsSitesName;
    rule.enabled = false;
    rule.trigger_type = AstraTabRuleTriggerType::kUrlPattern;
    rule.trigger_value = "*://mail.google.com/*";
    rule.action_type = AstraTabRuleActionType::kPin;
    rule.priority = 80;
    rule.created_time = base::Time::Now();
    samples.push_back(std::move(rule));
  }

  // Sample 4: Hibernate social media tabs
  {
    AstraTabRule rule;
    rule.id = kSampleSocialHibernateId;
    rule.name = kSampleSocialHibernateName;
    rule.enabled = false;
    rule.trigger_type = AstraTabRuleTriggerType::kDomain;
    rule.trigger_value = "twitter.com";
    rule.action_type = AstraTabRuleActionType::kHibernate;
    rule.priority = 200;
    rule.created_time = base::Time::Now();
    samples.push_back(std::move(rule));
  }

  return samples;
}

size_t AstraTabRulesService::AddSampleRules() {
  size_t added = 0;
  std::vector<AstraTabRule> samples = GetSampleRules();
  for (auto& sample : samples) {
    // Skip if a rule with this ID already exists.
    if (GetRule(sample.id)) {
      continue;
    }
    // Sample rules are added as disabled by default.
    AstraTabRule rule_copy = sample;
    rule_copy.created_time = base::Time::Now();
    rule_copy.last_triggered_time = base::Time();
    rule_copy.trigger_count = 0;

    rules_.push_back(std::move(rule_copy));
    added++;
  }

  if (added > 0) {
    SortRules();

    for (auto& observer : observers_) {
      for (const auto& sample : samples) {
        if (GetRule(sample.id)) {
          observer.OnRuleAdded(sample.id);
        }
      }
    }

    SaveToPrefs();
  }

  return added;
}

// -- Incognito compatibility -------------------------------------------------

bool AstraTabRulesService::IsIncognito() const {
  // TODO(astra): Use profile_->IsIncognitoProfile() once we're in a real
  // Chromium build.  For the overlay repo stub, return false.
  // Chromium owner: Profile::IsIncognitoProfile().
  return false;
}

// -- Private helpers ---------------------------------------------------------

AstraTabRule* AstraTabRulesService::FindRule(const std::string& id) {
  auto it = base::ranges::find(rules_, id, &AstraTabRule::id);
  return it == rules_.end() ? nullptr : &(*it);
}

void AstraTabRulesService::SortRules() {
  base::ranges::sort(rules_, [](const AstraTabRule& a, const AstraTabRule& b) {
    // Lower priority number = higher priority = comes first.
    if (a.priority != b.priority) {
      return a.priority < b.priority;
    }
    // Secondary sort by name for stability.
    return a.name < b.name;
  });
}

// static
std::string AstraTabRulesService::GenerateRuleId() {
  return base::UnguessableToken::Create().ToString();
}

void AstraTabRulesService::LoadFromPrefs() {
  // TODO(astra): Load rules from the profile's PrefService.
  // Chromium component: PrefService.
  // Patch point: astra_prefs.h + PrefService.
  //
  // Pref format: a list of dictionaries under "astra.tab_rules.rules",
  // each containing the serialized AstraTabRule fields.
  //
  // For now, no persisted state — start with empty list.
}

void AstraTabRulesService::SaveToPrefs() {
  // TODO(astra): Persist rules to the profile's PrefService.
  // Chromium component: PrefService + PrefRegistry.
  // Patch point: astra_prefs.h + PrefService.
  //
  // Should be called after every mutation so that rule changes survive
  // browser restarts.  PrefService handles deferred disk writes.
  //
  // For now, this is a no-op.
}

// ===========================================================================
// AstraTabRulesServiceFactory
// ===========================================================================

// static
AstraTabRulesService* AstraTabRulesServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraTabRulesService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraTabRulesServiceFactory* AstraTabRulesServiceFactory::GetInstance() {
  static base::NoDestructor<AstraTabRulesServiceFactory> instance;
  return instance.get();
}

// static
void AstraTabRulesServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // TODO(astra): Register tab rules prefs on the profile's PrefRegistry.
  //
  // Pref keys to register:
  //   - astra.tab_rules.rules       (list of dict) — all rule definitions
  //   - astra.tab_rules.version     (int) — schema version for migration
  //
  // Chromium component: PrefRegistry + PrefService.
  // Patch point: astra_prefs.h + browser_prefs.cc.
  //
  // For now, no prefs are registered.
}

AstraTabRulesServiceFactory::AstraTabRulesServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraTabRulesService",
          ProfileSelections::BuildForRegularAndIncognito()) {
  // TODO(astra): Declare dependencies on other ProfileKeyedServices.
  // For example, this service might depend on AstraWorkspaceService if
  // rule actions reference workspaces.
}

AstraTabRulesServiceFactory::~AstraTabRulesServiceFactory() = default;

KeyedService* AstraTabRulesServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return new AstraTabRulesService(profile);
}

}  // namespace astra
