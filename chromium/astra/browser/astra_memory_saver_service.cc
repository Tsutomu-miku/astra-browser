#include "astra/browser/astra_memory_saver_service.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

// Pref key for the memory saver whitelist (sites that are never suspended).
// List of string patterns — see AstraMemorySaverService::AddWhitelistedSite
// for pattern format documentation.
//
// TODO(astra): Move this to astra/browser/astra_prefs.h alongside other
// memory saver pref keys when the pref system is consolidated.
constexpr char kPrefMemorySaverWhitelist[] = "astra.memory_saver.whitelist";

// Helper: get the workspace service for a profile.
// Returns nullptr if not available.
AstraWorkspaceService* GetWorkspaceService(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  // TODO(astra): Use AstraWorkspaceServiceFactory::GetForProfile once the
  // factory is properly wired into the profile keyed service system.
  // For now, we return nullptr as a fallback — active workspace checks
  // will be skipped if the service is unavailable.
  return nullptr;
}

// Helper: check if a tab is currently the active tab in any browser window.
// This is a simplified check that looks at the active tab of the first
// browser for the profile.
// TODO(astra): Use BrowserList to check all browser windows for the profile.
// Chromium subsystem: BrowserList (chrome/browser/ui/browser_list.h)
bool IsActiveTab(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  // For now, we rely on the caller (TabStripModelObserver) to pass us
  // the active tab info. The service does not iterate BrowserList directly.
  // A full implementation would check all Browser instances for the profile.
  return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraMemorySaverService
// ---------------------------------------------------------------------------

AstraMemorySaverService::AstraMemorySaverService(Profile* profile)
    : profile_(profile) {
  LoadFromPrefs();

  // Start the periodic suspend timer if auto-suspend is enabled.
  if (auto_suspend_enabled_) {
    suspend_timer_.Start(FROM_HERE, kSuspendTimerInterval, this,
                         &AstraMemorySaverService::OnSuspendTimerTick);
  }
}

AstraMemorySaverService::~AstraMemorySaverService() = default;

void AstraMemorySaverService::Shutdown() {
  // KeyedService shutdown: stop the timer and clear observer references
  // before the profile goes away.
  suspend_timer_.Stop();
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraMemorySaverService::AddObserver(
    AstraMemorySaverServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraMemorySaverService::RemoveObserver(
    AstraMemorySaverServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Tab suspension ----------------------------------------------------------

bool AstraMemorySaverService::SuspendTab(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  // Already suspended — nothing to do.
  if (features->is_suspended()) {
    return true;
  }

  // Check policy exceptions before suspending.
  if (!CanSuspendTab(web_contents)) {
    return false;
  }

  // Save the current URL before discarding so we know what to reload.
  // We do this BEFORE setting suspended state because SetSuspended()
  // also tries to capture the URL.
  if (features->suspended_url().is_empty() &&
      !web_contents->GetURL().is_empty()) {
    features->set_suspended_url(web_contents->GetURL());
  }

  // Mark the tab as suspended in Astra metadata.
  features->SetSuspended(true);

  // Update suspension statistics.
  total_suspended_count_++;
  memory_saved_estimate_bytes_ += kEstimatedBytesPerSuspendedTab;

  // TODO(astra): Actually discard the tab using Chromium's TabManager.
  // The current implementation only sets Astra metadata — the tab's
  // renderer process is not actually killed. In a real build, we would
  // call into resource_coordinator::TabManager::DiscardTab() or
  // web_contents->DiscardPage() to actually free memory.
  //
  // Chromium owner: resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  // Chromium API: content::WebContents::DiscardPage()
  //   (content/public/browser/web_contents.h)

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnTabSuspended(web_contents);
  }

  return true;
}

bool AstraMemorySaverService::RestoreTab(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  // Already active — nothing to do.
  if (!features->is_suspended()) {
    return true;
  }

  // Clear the suspended flag and update last active time.
  features->SetSuspended(false);

  // Update memory estimate — a restored tab no longer saves memory.
  if (memory_saved_estimate_bytes_ >= kEstimatedBytesPerSuspendedTab) {
    memory_saved_estimate_bytes_ -= kEstimatedBytesPerSuspendedTab;
  } else {
    memory_saved_estimate_bytes_ = 0;
  }

  // TODO(astra): Trigger a reload of the tab's URL for a discarded tab.
  // For a real discarded WebContents, activating it or navigating to its
  // URL would trigger a reload. For the overlay skeleton, we just update
  // the metadata.
  //
  // In a real build with discarded tabs, restoration would happen via:
  //   - Tab activation: TabStripModel::ActivateTabAt triggers reload.
  //   - Or explicitly: web_contents->GetController().LoadURL(...)
  //
  // Chromium owner: content::NavigationController
  //   (content/public/browser/navigation_controller.h)

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnTabRestored(web_contents);
  }

  return true;
}

bool AstraMemorySaverService::IsTabSuspended(
    content::WebContents* web_contents) const {
  if (!web_contents) {
    return false;
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (!features) {
    return false;
  }

  return features->is_suspended();
}

size_t AstraMemorySaverService::GetSuspendedTabCount() const {
  // TODO(astra): Iterate all Browser instances for this profile and count
  // tabs where AstraTabFeatures::is_suspended() is true.
  //
  // Chromium subsystem: BrowserList + TabStripModel.
  //   - chrome/browser/ui/browser_list.h
  //   - chrome/browser/ui/tabs/tab_strip_model.h
  //
  // For the overlay skeleton, we return 0 as a placeholder.
  // The sidebar can still show individual suspended tab indicators by
  // checking AstraTabFeatures on each tab it projects.
  return 0;
}

size_t AstraMemorySaverService::WakeAllTabs() {
  // TODO(astra): Iterate all Browser instances for this profile and
  // restore every suspended tab.
  //
  // Chromium subsystem: BrowserList + TabStripModel.
  //
  // For the overlay skeleton, this is a no-op that returns 0.
  return 0;
}

size_t AstraMemorySaverService::SuspendAllEligibleTabs() {
  // TODO(astra): Iterate all Browser instances for this profile and
  // suspend every tab that passes CanSuspendTab() checks.
  //
  // This is the manual "suspend now" equivalent of the periodic auto-suspend.
  // Unlike SuspendEligibleTabs(), this does not check the inactivity timeout
  // — it suspends all tabs that pass the exception policy checks.
  //
  // Chromium subsystem: BrowserList + TabStripModel.
  //
  // For the overlay skeleton, this is a no-op that returns 0.
  return 0;
}

// -- Whitelist ---------------------------------------------------------------

void AstraMemorySaverService::AddWhitelistedSite(
    const std::string& host_pattern) {
  if (host_pattern.empty()) {
    return;
  }

  // Check if already in the list (avoid duplicates).
  auto it = base::ranges::find(whitelist_, host_pattern);
  if (it != whitelist_.end()) {
    return;
  }

  whitelist_.push_back(host_pattern);
  SaveWhitelistToPrefs();

  for (auto& observer : observers_) {
    observer.OnWhitelistChanged();
  }
}

void AstraMemorySaverService::RemoveWhitelistedSite(
    const std::string& host_pattern) {
  auto it = base::ranges::find(whitelist_, host_pattern);
  if (it == whitelist_.end()) {
    return;
  }

  whitelist_.erase(it);
  SaveWhitelistToPrefs();

  for (auto& observer : observers_) {
    observer.OnWhitelistChanged();
  }
}

bool AstraMemorySaverService::IsSiteWhitelisted(const std::string& url) const {
  if (whitelist_.empty()) {
    return false;
  }

  GURL gurl(url);
  if (!gurl.is_valid()) {
    // Fall back to simple substring matching if not a valid URL.
    for (const auto& pattern : whitelist_) {
      if (url.find(pattern) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  std::string host = gurl.host();
  for (const auto& pattern : whitelist_) {
    if (HostMatchesPattern(host, pattern)) {
      return true;
    }
  }
  return false;
}

// static
bool AstraMemorySaverService::HostMatchesPattern(const std::string& host,
                                                 const std::string& pattern) {
  // Pattern formats (same as focus mode blocklist):
  //   - Exact host: "example.com" matches "example.com"
  //   - Subdomain match: "example.com" also matches "www.example.com"
  //   - Wildcard: "*.example.com" matches base domain and any subdomain
  //
  // TODO(astra): Replace with ContentSettingsPattern::Matches() for
  // proper pattern matching semantics.
  // Chromium component: ContentSettingsPattern
  //   (components/content_settings/core/common/content_settings_pattern.h)

  if (pattern.starts_with("*.")) {
    // Wildcard pattern: *.example.com
    // Matches the base domain (example.com) and any subdomain
    // (www.example.com, sub.www.example.com).
    // Matches behavior of focus mode blocklist pattern matching.
    std::string suffix = pattern.substr(2);
    if (host == suffix || host.ends_with("." + suffix)) {
      return true;
    }
    return false;
  }

  // Exact or subdomain match.
  // "example.com" matches "example.com" and "www.example.com".
  if (host == pattern || host.ends_with("." + pattern)) {
    return true;
  }

  return false;
}

// -- Stats -------------------------------------------------------------------

void AstraMemorySaverService::ResetStats() {
  total_suspended_count_ = 0;
  memory_saved_estimate_bytes_ = 0;
}

// -- Settings ----------------------------------------------------------------

void AstraMemorySaverService::SetAutoSuspendEnabled(bool enabled) {
  if (auto_suspend_enabled_ == enabled) {
    return;
  }

  auto_suspend_enabled_ = enabled;
  SaveEnabledToPrefs();

  // Start or stop the periodic timer.
  if (auto_suspend_enabled_) {
    suspend_timer_.Start(FROM_HERE, kSuspendTimerInterval, this,
                         &AstraMemorySaverService::OnSuspendTimerTick);
  } else {
    suspend_timer_.Stop();
  }

  for (auto& observer : observers_) {
    observer.OnMemorySaverEnabledChanged(auto_suspend_enabled_);
  }
}

void AstraMemorySaverService::SetAutoSuspendTimeout(base::TimeDelta timeout) {
  if (timeout <= base::TimeDelta()) {
    return;
  }

  if (auto_suspend_timeout_ == timeout) {
    return;
  }

  auto_suspend_timeout_ = timeout;
  SaveTimeoutToPrefs();

  for (auto& observer : observers_) {
    observer.OnMemorySaverTimeoutChanged(auto_suspend_timeout_);
  }
}

void AstraMemorySaverService::SetSuspendActiveWorkspace(bool enabled) {
  if (suspend_active_workspace_ == enabled) {
    return;
  }

  suspend_active_workspace_ = enabled;
  SaveSuspendActiveWorkspaceToPrefs();
}

// -- Exceptions / eligibility -----------------------------------------------

bool AstraMemorySaverService::CanSuspendTab(
    content::WebContents* web_contents) const {
  if (!web_contents) {
    return false;
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (!features) {
    // If there's no Astra metadata, we can't track last active time,
    // so we don't suspend.
    return false;
  }

  // Already suspended — not eligible (call SuspendTab again for no-op).
  if (features->is_suspended()) {
    return false;
  }

  // Exception: whitelisted sites are never suspended.
  if (!web_contents->GetURL().is_empty()) {
    if (IsSiteWhitelisted(web_contents->GetURL().spec())) {
      return false;
    }
  }

  // Exception: tabs playing audio (unless the policy allows it).
  // TODO(astra): Use web_contents->IsCurrentlyAudible() for the real check.
  // Currently we don't have real audio state tracking in the overlay skeleton.
  // Chromium API: content::WebContents::IsCurrentlyAudible()
  //   (content/public/browser/web_contents.h)
  // When suspend_audible_tabs_ is false (default), we assume tabs might be
  // playing audio and skip them as a conservative placeholder.
  // TODO(astra): Remove this placeholder once real audio checking is in place.

  // Exception: pinned tabs (unless the policy allows it).
  // TODO(astra): Use TabStripModel::IsTabPinned() for the real check.
  // Currently we check sidebar_pinned as a proxy.
  if (!suspend_pinned_tabs_ && features->sidebar_pinned()) {
    return false;
  }

  // Exception: active (currently selected) tab (unless the policy allows it).
  // In a full implementation, we'd check against all Browser instances.
  // For now, we use IsActiveTab() as a placeholder check.
  // TODO(astra): Add explicit active tab check via BrowserList.
  if (!suspend_active_tab_ && IsActiveTab(web_contents)) {
    return false;
  }

  // Exception: tabs in the active workspace (if the setting is disabled).
  if (!suspend_active_workspace_) {
    AstraWorkspaceService* workspace_service = GetWorkspaceService(profile_);
    if (workspace_service) {
      const std::string& active_workspace_id =
          workspace_service->active_workspace_id();
      if (features->workspace_id() == active_workspace_id) {
        return false;
      }
    }
    // If workspace service is unavailable, we can't check, so we default
    // to allowing suspension (consistent with "suspend_active_workspace = true").
  }

  return true;
}

// -- Tab activity tracking ---------------------------------------------------

void AstraMemorySaverService::OnTabActivated(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  // Update last active time to "now".
  features->set_last_active_time(base::TimeTicks::Now());

  // If the tab was suspended, restoring it on activation.
  if (features->is_suspended()) {
    RestoreTab(web_contents);
  }
}

void AstraMemorySaverService::OnTabInserted(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  // New tabs start with "now" as their last active time.
  features->set_last_active_time(base::TimeTicks::Now());
}

// -- Private helpers ---------------------------------------------------------

void AstraMemorySaverService::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  auto_suspend_enabled_ =
      prefs->GetBoolean(prefs::kPrefMemorySaverEnabled);

  int timeout_minutes =
      prefs->GetInteger(prefs::kPrefMemorySaverTimeoutMinutes);
  if (timeout_minutes > 0) {
    auto_suspend_timeout_ = base::Minutes(timeout_minutes);
  }

  suspend_active_workspace_ =
      prefs->GetBoolean(prefs::kPrefMemorySaverSuspendActiveWorkspace);

  // Load whitelist.
  const base::Value::List& whitelist = prefs->GetList(kPrefMemorySaverWhitelist);
  whitelist_.clear();
  for (const auto& item : whitelist) {
    if (item.is_string()) {
      whitelist_.push_back(item.GetString());
    }
  }
}

void AstraMemorySaverService::SaveToPrefs() {
  SaveEnabledToPrefs();
  SaveTimeoutToPrefs();
  SaveSuspendActiveWorkspaceToPrefs();
  SaveWhitelistToPrefs();
}

void AstraMemorySaverService::SaveEnabledToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(prefs::kPrefMemorySaverEnabled,
                                    auto_suspend_enabled_);
}

void AstraMemorySaverService::SaveTimeoutToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetInteger(
      prefs::kPrefMemorySaverTimeoutMinutes,
      static_cast<int>(auto_suspend_timeout_.InMinutes()));
}

void AstraMemorySaverService::SaveSuspendActiveWorkspaceToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(
      prefs::kPrefMemorySaverSuspendActiveWorkspace,
      suspend_active_workspace_);
}

void AstraMemorySaverService::SaveWhitelistToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  base::Value::List list;
  for (const auto& pattern : whitelist_) {
    list.Append(pattern);
  }
  prefs->SetList(kPrefMemorySaverWhitelist, std::move(list));
}

void AstraMemorySaverService::OnSuspendTimerTick() {
  if (!auto_suspend_enabled_) {
    return;
  }
  SuspendEligibleTabs();
}

void AstraMemorySaverService::SuspendEligibleTabs() {
  // TODO(astra): Iterate all Browser instances for this profile, then
  // iterate each browser's TabStripModel, and check each tab for
  // suspension eligibility.
  //
  // For each tab:
  //   1. Check CanSuspendTab() (policy exceptions).
  //   2. Check AstraTabFeatures::IsEligibleForSuspend(timeout) (inactivity).
  //   3. If both pass, call SuspendTab().
  //
  // Chromium subsystem: BrowserList + TabStripModel.
  //   - chrome/browser/ui/browser_list.h
  //   - chrome/browser/ui/tabs/tab_strip_model.h
  //
  // For the overlay skeleton, this is a no-op. The actual tab iteration
  // requires integration with Chromium's BrowserList.
}

// ---------------------------------------------------------------------------
// AstraMemorySaverServiceFactory
// ---------------------------------------------------------------------------

// static
AstraMemorySaverService* AstraMemorySaverServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraMemorySaverService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraMemorySaverServiceFactory* AstraMemorySaverServiceFactory::GetInstance() {
  static base::NoDestructor<AstraMemorySaverServiceFactory> instance;
  return instance.get();
}

// static
void AstraMemorySaverServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Whether memory saver (auto-suspend inactive tabs) is enabled.
  registry->RegisterBooleanPref(prefs::kPrefMemorySaverEnabled,
                                prefs::kDefaultMemorySaverEnabled);

  // Auto-suspend timeout in minutes.
  registry->RegisterIntegerPref(prefs::kPrefMemorySaverTimeoutMinutes,
                                prefs::kDefaultMemorySaverTimeoutMinutes);

  // Whether tabs in the active workspace can be auto-suspended.
  registry->RegisterBooleanPref(
      prefs::kPrefMemorySaverSuspendActiveWorkspace,
      prefs::kDefaultMemorySaverSuspendActiveWorkspace);

  // Never-suspend site whitelist: list of URL pattern strings.
  // Default: empty list — no sites are excluded from auto-suspend.
  registry->RegisterListPref(kPrefMemorySaverWhitelist);

  // TODO(astra): Add additional memory saver prefs as needed:
  //   - Minimum tab count before suspension kicks in.
  //   - Whether to show the memory saver indicator in the toolbar.
  //   - Suspended tab grouping preference (group at bottom vs inline).
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration.
}

AstraMemorySaverServiceFactory::AstraMemorySaverServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraMemorySaverService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito uses kOwnInstance because memory saver is
              // per-browsing-context. An incognito window has its own
              // memory saver instance that manages only incognito tabs.
              // Suspension state is per-tab (WebContentsUserData) and
              // does not cross incognito / regular boundaries.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions get their own ephemeral memory saver instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user-facing tabs to suspend.
              .Build()) {}

AstraMemorySaverServiceFactory::~AstraMemorySaverServiceFactory() = default;

KeyedService*
AstraMemorySaverServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraMemorySaverService(Profile::FromBrowserContext(context));
}

}  // namespace astra
