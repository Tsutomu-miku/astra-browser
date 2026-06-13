#ifndef ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraMemorySaverService — tab suspend / memory saver policy
// =========================================================================
//
// Profile-scoped keyed service that manages the Astra memory saver feature.
//
// Memory saver automatically suspends (discards) inactive tabs to save memory.
// It builds on top of Chromium's existing tab discard / memory management
// infrastructure (resource_coordinator::TabManager) — Astra only owns the
// policy (when to suspend, which tabs to exclude) and UI projection.
//
// Truth model:
//   - Actual tab discard / suspension is performed by Chromium's TabManager.
//   - Astra owns the suspension policy: timeout duration, exceptions (audio,
//     pinned tabs, active workspace), and user settings.
//   - Per-tab suspension metadata (is_suspended, suspended_url,
//     last_active_time) lives on AstraTabFeatures (WebContentsUserData).
//   - Settings (enabled, timeout, active-workspace suspend) are persisted
//     via PrefService.
//
// Chromium subsystems reused:
//   - resource_coordinator::TabManager — actual tab discarding.
//   - TabStripModel — tab iteration and active tab tracking.
//   - content::WebContents — check IsDiscarded(), get URL.
//   - PrefService — settings persistence.
//   - ProfileKeyedServiceFactory pattern.
//
// Chromium patch points:
//   - TabManager / TabDiscards: to integrate with Chromium's native
//     tab discard mechanism instead of doing reload-based suspension.
//   - TabStripModelObserver: tab activation events to update last active time.
//   - resource_coordinator::TabManagerObserver: to sync suspension state.
//
// TODO(astra): Integrate with Chromium's TabManager / resource_coordinator
// for proper tab suspension instead of the current reload-based approach.
// The current implementation uses a timer and manual discard simulation
// as a placeholder; production code should delegate to TabManager.
// Chromium owner: resource_coordinator::TabManager
//   (chrome/browser/resource_coordinator/tab_manager.h)
// =========================================================================

class AstraMemorySaverServiceObserver : public base::CheckedObserver {
 public:
  // Called when a tab is suspended.
  // |web_contents| is the tab that was suspended.
  virtual void OnTabSuspended(content::WebContents* web_contents) {}

  // Called when a tab is restored (woken up from suspension).
  // |web_contents| is the tab that was restored.
  virtual void OnTabRestored(content::WebContents* web_contents) {}

  // Called when the memory saver enabled state changes.
  virtual void OnMemorySaverEnabledChanged(bool enabled) {}

  // Called when the auto-suspend timeout changes.
  virtual void OnMemorySaverTimeoutChanged(base::TimeDelta timeout) {}

  // Called when the whitelist (never-suspend sites) changes.
  virtual void OnWhitelistChanged() {}

 protected:
  ~AstraMemorySaverServiceObserver() override = default;
};

class AstraMemorySaverService final : public KeyedService {
 public:
  explicit AstraMemorySaverService(Profile* profile);
  AstraMemorySaverService(const AstraMemorySaverService&) = delete;
  AstraMemorySaverService& operator=(const AstraMemorySaverService&) = delete;
  ~AstraMemorySaverService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraMemorySaverServiceObserver* observer);
  void RemoveObserver(AstraMemorySaverServiceObserver* observer);

  // -- Tab suspension ----------------------------------------------------

  // Manually suspend |web_contents|. Returns true if the tab was
  // successfully suspended (or was already suspended).
  //
  // The actual discard is delegated to Chromium's tab discard mechanism.
  // Astra manages the policy and metadata; Chromium does the work.
  //
  // TODO(astra): Use resource_coordinator::TabManager::DiscardTab or
  // content::WebContents::DiscardPage() for actual tab discarding.
  // Currently sets Astra metadata only as a placeholder.
  // Chromium owner: resource_coordinator::TabManager
  //   (chrome/browser/resource_coordinator/tab_manager.h)
  bool SuspendTab(content::WebContents* web_contents);

  // Restore (wake up) a suspended tab. Returns true if the tab was
  // successfully restored (or was already active).
  //
  // For a discarded tab, this triggers a reload of the suspended URL.
  //
  // TODO(astra): Integrate with Chromium's tab restore / reload flow.
  // Chromium owner: content::NavigationController / WebContents reload.
  bool RestoreTab(content::WebContents* web_contents);

  // Returns true if |web_contents| is currently suspended.
  //
  // Reads from AstraTabFeatures metadata. In the future, this should
  // also check Chromium's WebContents::IsDiscarded() as the source of truth.
  //
  // TODO(astra): Use content::WebContents::IsDiscarded() as the primary
  // source of truth for suspension state, with Astra metadata as a cache.
  // Chromium owner: content::WebContents (content/public/browser/web_contents.h)
  bool IsTabSuspended(content::WebContents* web_contents) const;

  // Returns the number of currently suspended tabs in the profile.
  //
  // Iterates all Browser windows for this profile and counts tabs whose
  // AstraTabFeatures mark them as suspended.
  //
  // TODO(astra): Use BrowserList + TabStripModel iteration to count
  // suspended tabs across all browser windows for this profile.
  // Currently returns 0 as a placeholder pending full BrowserList integration.
  // Chromium subsystem: BrowserList (chrome/browser/ui/browser_list.h)
  size_t GetSuspendedTabCount() const;

  // Restore all suspended tabs.
  // Returns the number of tabs that were restored.
  size_t WakeAllTabs();

  // Immediately suspends all currently eligible tabs.
  // Returns the number of tabs that were suspended.
  //
  // Eligibility is determined by CanSuspendTab() — tabs must pass all
  // exception checks (pinned, audible, active, whitelist, workspace).
  // This is the same as the periodic auto-suspend but triggered manually.
  //
  // TODO(astra): Use BrowserList + TabStripModel to iterate all tabs
  // across all browser windows for this profile.
  // Chromium subsystem: BrowserList + TabStripModel.
  size_t SuspendAllEligibleTabs();

  // -- Whitelist (never-suspend sites) -----------------------------------

  // Adds a site pattern to the never-suspend whitelist.
  // Tabs whose URL matches any whitelist pattern will not be suspended.
  //
  // Pattern formats (same as focus mode blocklist):
  //   - Exact host: "example.com" matches example.com
  //   - Subdomain match: "example.com" also matches www.example.com
  //   - Wildcard: "*.example.com" matches any subdomain of example.com
  //
  // TODO(astra): Use Chromium's ContentSettingsPattern for proper
  // pattern matching semantics.
  // Chromium component: ContentSettingsPattern
  //   (components/content_settings/core/common/content_settings_pattern.h)
  void AddWhitelistedSite(const std::string& host_pattern);

  // Removes a site pattern from the never-suspend whitelist.
  void RemoveWhitelistedSite(const std::string& host_pattern);

  // Returns true if |url| matches any pattern in the whitelist.
  // If |url| is not a valid URL, falls back to substring matching.
  bool IsSiteWhitelisted(const std::string& url) const;

  // Returns the full list of whitelist patterns.
  const std::vector<std::string>& whitelist() const { return whitelist_; }

  // -- Settings ----------------------------------------------------------

  // Whether auto-suspend (memory saver) is enabled.
  bool auto_suspend_enabled() const { return auto_suspend_enabled_; }
  void SetAutoSuspendEnabled(bool enabled);

  // The inactivity timeout after which tabs are auto-suspended.
  base::TimeDelta auto_suspend_timeout() const {
    return auto_suspend_timeout_;
  }
  void SetAutoSuspendTimeout(base::TimeDelta timeout);

  // Whether tabs in the active workspace can be auto-suspended.
  // When false, only tabs in non-active workspaces are eligible.
  bool suspend_active_workspace() const {
    return suspend_active_workspace_;
  }
  void SetSuspendActiveWorkspace(bool enabled);

  // -- Suspension stats ---------------------------------------------------

  // Returns the total number of tabs suspended since the service was
  // created (or since the last ResetStats()).
  //
  // This is a cumulative counter — it counts every successful SuspendTab()
  // call, even if the tab was later restored and suspended again.
  size_t GetTotalSuspendedCount() const { return total_suspended_count_; }

  // Returns an estimate of memory saved by tab suspension, in bytes.
  //
  // This is a placeholder calculation based on the number of suspended
  // tabs and an average per-tab memory footprint.  In production, this
  // should use Chromium's memory metrics (e.g., MemoryUsageMonitor or
  // per-renderer-process memory stats).
  //
  // TODO(astra): Use real memory metrics from Chromium's memory system.
  // Chromium component: resource_coordinator / memory_instrumentation.
  int64_t GetMemorySavedEstimateBytes() const {
    return memory_saved_estimate_bytes_;
  }

  // Resets all suspension counters (total suspended, memory estimate).
  // Does not affect currently suspended tabs.
  void ResetStats();

  // -- Exception policy controls -----------------------------------------
  //
  // These in-memory settings control which tabs are eligible for
  // suspension.  They are not persisted — they are runtime policy
  // toggles used by UI surfaces or feature experiments.

  // Whether pinned tabs can be suspended.
  // Default: false — pinned tabs are preserved and never auto-suspended.
  bool suspend_pinned_tabs() const { return suspend_pinned_tabs_; }
  void set_suspend_pinned_tabs(bool value) { suspend_pinned_tabs_ = value; }

  // Whether tabs playing audio can be suspended.
  // Default: false — tabs playing audio are never auto-suspended.
  bool suspend_audible_tabs() const { return suspend_audible_tabs_; }
  void set_suspend_audible_tabs(bool value) { suspend_audible_tabs_ = value; }

  // Whether the currently active (foreground) tab can be suspended.
  // Default: false — the active tab is never auto-suspended.
  bool suspend_active_tab() const { return suspend_active_tab_; }
  void set_suspend_active_tab(bool value) { suspend_active_tab_ = value; }

  // Returns true if |web_contents| can be suspended based on current
  // policy (exceptions: audio, pinned, active tab, active workspace).
  // Does NOT check the timeout — use IsEligibleForSuspend on
  // AstraTabFeatures for that.
  //
  // TODO(astra): Check actual audio state via WebContents::IsCurrentlyAudible().
  // TODO(astra): Check pinned state via TabStripModel::IsTabPinned().
  // Currently reads from AstraTabFeatures metadata only.
  bool CanSuspendTab(content::WebContents* web_contents) const;

  // Notifies the service that a tab has become active (foreground).
  // Updates the tab's last active time and potentially restores it
  // if it was suspended.
  //
  // Called from TabStripModelObserver::ActiveTabChanged via a patch point.
  //
  // Chromium owner: TabStripModelObserver
  //   (chrome/browser/ui/tabs/tab_strip_model_observer.h)
  // Patch point: add Astra hook in TabStripModelObserver::ActiveTabChanged.
  void OnTabActivated(content::WebContents* web_contents);

  // Notifies the service that a tab was inserted.
  // Initializes last_active_time_ on the tab's AstraTabFeatures.
  void OnTabInserted(content::WebContents* web_contents);

 private:
  // Loads persisted settings from the profile's PrefService.
  void LoadFromPrefs();

  // Saves current settings to the profile's PrefService.
  void SaveToPrefs();

  // Saves enabled state to prefs.
  void SaveEnabledToPrefs();

  // Saves timeout to prefs.
  void SaveTimeoutToPrefs();

  // Saves suspend-active-workspace setting to prefs.
  void SaveSuspendActiveWorkspaceToPrefs();

  // Saves the whitelist to prefs.
  void SaveWhitelistToPrefs();

  // Helper: checks if a host matches a whitelist pattern.
  // Supports exact match, subdomain match, and *.wildcard patterns.
  static bool HostMatchesPattern(const std::string& host,
                                 const std::string& pattern);

  // Periodic timer callback — checks for tabs that have exceeded the
  // inactivity timeout and suspends eligible ones.
  void OnSuspendTimerTick();

  // Iterates all tabs in the profile and suspends any that are past
  // the inactivity timeout and pass the exception checks.
  //
  // TODO(astra): Use BrowserList + TabStripModel to iterate all tabs
  // across all browser windows for this profile.
  // Chromium subsystem: BrowserList (chrome/browser/ui/browser_list.h)
  void SuspendEligibleTabs();

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraMemorySaverServiceObserver> observers_;

  // Settings (persisted via PrefService).
  bool auto_suspend_enabled_ = true;
  base::TimeDelta auto_suspend_timeout_ = base::Minutes(5);
  bool suspend_active_workspace_ = false;

  // Whitelist of sites that are never suspended (persisted via PrefService).
  std::vector<std::string> whitelist_;

  // Suspension statistics (in-memory, reset on service creation).
  size_t total_suspended_count_ = 0;
  int64_t memory_saved_estimate_bytes_ = 0;

  // Exception policy toggles (in-memory, not persisted).
  bool suspend_pinned_tabs_ = false;
  bool suspend_audible_tabs_ = false;
  bool suspend_active_tab_ = false;

  // Estimated memory savings per suspended tab, in bytes.
  // Used for the placeholder memory estimate calculation.
  // Average of ~50 MB per tab is a rough order-of-magnitude estimate.
  //
  // TODO(astra): Replace with real per-tab memory metrics.
  static constexpr int64_t kEstimatedBytesPerSuspendedTab = 50 * 1024 * 1024;

  // Periodic timer that checks for eligible tabs to suspend.
  // Fires every minute when auto-suspend is enabled.
  //
  // TODO(astra): Replace with TabManager integration — Chromium's
  // resource_coordinator has its own scheduling for tab discarding.
  // This timer is a stand-in for the overlay skeleton.
  base::RepeatingTimer suspend_timer_;

  // Interval at which the suspend timer ticks.
  // 1 minute provides a reasonable balance between responsiveness and
  // performance overhead.
  static constexpr base::TimeDelta kSuspendTimerInterval = base::Minutes(1);
};

// =========================================================================
// AstraMemorySaverServiceFactory
// =========================================================================
//
// Factory for AstraMemorySaverService.
//
// Incognito behavior: kOwnInstance — memory saver is per-profile instance.
// An incognito window has its own memory saver that manages only the
// incognito tabs. Suspension state is per-tab (WebContentsUserData) and
// does not carry across incognito / regular boundaries.
//
// Guest: kOwnInstance.
// System: kNone.
// =========================================================================

class AstraMemorySaverServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraMemorySaverService* GetForProfile(Profile* profile);
  static AstraMemorySaverServiceFactory* GetInstance();

  // Registers memory-saver-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation. Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraMemorySaverServiceFactory();
  ~AstraMemorySaverServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_MEMORY_SAVER_SERVICE_H_
