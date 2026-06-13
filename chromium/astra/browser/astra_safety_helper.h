#ifndef ASTRA_BROWSER_ASTRA_SAFETY_HELPER_H_
#define ASTRA_BROWSER_ASTRA_SAFETY_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace astra {

// =========================================================================
// AstraSafetyLevel — overall safety rating
// =========================================================================
//
// Projected safety level for the current browsing context.
//
// Chromium owner: SafeBrowsingService
//   (components/safe_browsing/core/browser/safe_browsing_service.h)
// Chromium owner: SecurityStateTabHelper
//   (chrome/browser/ssl/security_state_tab_helper.h)
enum class AstraSafetyLevel {
  kSafe,       // No issues detected
  kWarning,    // Minor warnings (e.g., mixed content)
  kDangerous,  // Dangerous content blocked
  kUnknown,    // Safety status unknown (e.g., page not loaded)
};

// =========================================================================
// AstraThreatType — type of security threat
// =========================================================================
//
// Types of threats that safe browsing can detect.
//
// Chromium owner: safe_browsing::SBThreatType
//   (components/safe_browsing/core/common/proto/realtimeapi.pb.h)
enum class AstraThreatType {
  kMalware,        // Malware / phishing
  kDeceptive,      // Deceptive content (social engineering)
  kUnwanted,       // Unwanted software
  kHarmful,        // Harmful download
  kPasswordReuse,  // Password reuse detected
  kMixedContent,   // Mixed content (HTTP on HTTPS page)
};

// =========================================================================
// AstraThreatInfo — information about a detected threat
// =========================================================================
//
// Projection of a threat detection event for UI display.
//
// This is a presentation-only data structure — it mirrors a subset of
// Chromium's safe browsing threat state. The truth source is always
// Chromium's SafeBrowsingService.
struct AstraThreatInfo {
  // The threatening URL.
  GURL url;

  // Type of threat detected.
  AstraThreatType threat_type = AstraThreatType::kMalware;

  // Severity level (0-10).
  int severity = 0;

  // When the threat was discovered.
  base::Time discovered_time;

  // Whether the threat was blocked.
  bool is_blocked = false;

  // What action the user took ("proceed", "go_back", etc.).
  std::string user_action;
};

// =========================================================================
// AstraSecurityStatus — overall security status
// =========================================================================
//
// Aggregated security status for the current page/context.
//
// Truth source: Chromium's SecurityStateModel and SSLStatus.
//   - content/public/browser/ssl_status.h
//   - components/security_state/core/security_state.h
struct AstraSecurityStatus {
  // Whether the connection is secure (HTTPS with valid cert).
  bool is_secure = false;

  // Overall safety level.
  AstraSafetyLevel security_level = AstraSafetyLevel::kUnknown;

  // Whether mixed content is present on the page.
  bool has_mixed_content = false;

  // Whether the SSL certificate is valid.
  bool has_valid_cert = false;

  // Certificate status description.
  std::string certificate_status;

  // Number of cookies on the page.
  int cookie_count = 0;

  // Number of permissions granted to the site.
  int permission_count = 0;
};

// =========================================================================
// AstraSafetyObserver — observer interface
// =========================================================================
//
// Observer interface for AstraSafetyHelper. Notifies when safety state
// changes, threats are detected, or safety settings are modified.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh safety indicators
// when security state changes. The browser layer never depends on
// Views code.
class AstraSafetyObserver : public base::CheckedObserver {
 public:
  // Called when a new threat is detected.
  // |threat| contains details about the detected threat.
  virtual void OnThreatDetected(const AstraThreatInfo& threat) {}

  // Called when a threat is blocked by safe browsing.
  // |threat| contains details about the blocked threat.
  virtual void OnThreatBlocked(const AstraThreatInfo& threat) {}

  // Called when the user proceeds past a warning.
  // |url| is the URL the user chose to proceed to.
  virtual void OnUserProceeded(const GURL& url) {}

  // Called when the overall security status changes.
  // |status| contains the updated security status.
  virtual void OnSecurityStatusChanged(const AstraSecurityStatus& status) {}

  // Called when safe browsing is toggled on or off.
  // |enabled| is the new state.
  virtual void OnSafeBrowsingToggled(bool enabled) {}

  // Called when any safety setting changes.
  virtual void OnSafetySettingsChanged() {}

  // Called when a password breach is detected for a site.
  // |site| is the hostname of the affected site.
  virtual void OnPasswordBreachDetected(const std::string& site) {}

 protected:
  ~AstraSafetyObserver() override = default;
};

// =========================================================================
// AstraSafetyHelper — safe browsing projection helper
// =========================================================================
//
// Helper class for safe browsing and security-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that projects Chromium's
// SafeBrowsingService and security state. It provides a clean interface
// for the Astra UI layer to query safety state and manage safety
// presentation settings without directly depending on the full safe
// browsing subsystem.
//
// Truth source: Chromium's SafeBrowsingService
//   (components/safe_browsing/core/browser/safe_browsing_service.h)
// and SecurityStateModel
//   (components/security_state/core/security_state.h)
//
// Security principles:
//   - Safe browsing decisions are always made by Chromium.
//   - Astra only projects state and adds presentation preferences.
//   - Allowlist/blocklist operations delegate to Chromium's safe browsing.
//   - All Astra metadata persists via PrefService.
//
// Astra-specific presentation preferences (show security button,
// threat notifications, etc.) are persisted via the profile's PrefService.
// These are purely presentation concerns and never affect the underlying
// safe browsing behavior managed by Chromium.
//
// TODO(astra): Integrate with SafeBrowsingService for real threat data.
//   Currently this helper provides projection-only state. To get live
//   updates, we need to observe safe browsing events.
// Chromium observer: safe_browsing::SafeBrowsingServiceObserver
//   (components/safe_browsing/core/browser/safe_browsing_service.h)
// Chromium patch point: None needed — observer is a public interface.
class AstraSafetyHelper : public KeyedService {
 public:
  explicit AstraSafetyHelper(Profile* profile);
  AstraSafetyHelper(const AstraSafetyHelper&) = delete;
  AstraSafetyHelper& operator=(const AstraSafetyHelper&) = delete;
  ~AstraSafetyHelper() override;

  // -- Security status queries --------------------------------------------

  // Returns the current overall security status.
  //
  // TODO(astra): Project from Chromium's SecurityStateModel.
  //   For now, returns a default status based on safe browsing state.
  AstraSecurityStatus GetSecurityStatus() const;

  // Returns the current overall safety level.
  AstraSafetyLevel GetSafetyLevel() const;

  // -- Safe browsing enable/disable --------------------------------------

  // Returns whether safe browsing is enabled in Astra UI.
  //
  // This is a presentation preference — it does not control Chromium's
  // actual safe browsing state. Chromium's pref for safe browsing is
  // safebrowsing.safe_browsing_enabled.
  //
  // Persisted via PrefService. Default: true.
  bool IsSafeBrowsingEnabled() const;

  // Sets whether safe browsing is shown as enabled in Astra UI.
  // Fires OnSafeBrowsingToggled and OnSafetySettingsChanged observer
  // notifications.
  void SetSafeBrowsingEnabled(bool enabled);

  // -- Threat history -----------------------------------------------------

  // Returns the total number of threats detected.
  size_t GetThreatCount() const;

  // Returns recent threat detections, most recent first.
  // |max_count| limits the number of results. Use 0 for no limit.
  std::vector<AstraThreatInfo> GetRecentThreats(int max_count) const;

  // Returns the number of threats that were blocked.
  size_t GetBlockedThreatCount() const;

  // -- Password breach tracking ------------------------------------------

  // Returns the number of breached passwords.
  //
  // TODO(astra): Project from Chromium's PasswordHealthChecker.
  //   For now, returns 0 as placeholder.
  // Chromium owner: password_manager::PasswordHealthChecker
  size_t GetPasswordBreachCount() const;

  // -- Site safety checks ------------------------------------------------

  // Checks whether a site is considered safe.
  //
  // TODO(astra): Consult Chromium's SafeBrowsingDatabaseManager.
  //   For now, returns true unless the site is in the blocklist.
  //
  // Chromium owner: SafeBrowsingDatabaseManager
  //   (components/safe_browsing/core/browser/db/database_manager.h)
  bool IsSiteSafe(const GURL& url) const;

  // -- Site allowlist / blocklist ----------------------------------------

  // Adds a site to the safe browsing allowlist.
  // Idempotent: calling with an already-allowed site is a no-op.
  //
  // TODO(astra): Delegate to Chromium's safe browsing allowlist.
  //   For now, manages a local list via PrefService.
  void AllowSite(const GURL& url);

  // Adds a site to the blocklist.
  // Idempotent: calling with an already-blocked site is a no-op.
  void BlockSite(const GURL& url);

  // Returns whether a site is in the allowlist.
  bool IsSiteAllowed(const GURL& url) const;

  // Returns whether a site is in the blocklist.
  bool IsSiteBlocked(const GURL& url) const;

  // -- Threat history management -----------------------------------------

  // Clears all threat history.
  void ClearThreatHistory();

  // -- Protection level ---------------------------------------------------

  // Returns the safe browsing protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 (standard).
  int GetSafeBrowsingLevel() const;

  // Sets the safe browsing protection level.
  // Values are clamped to 0-2.
  // Fires OnSafetySettingsChanged observer notification.
  void SetSafeBrowsingLevel(int level);

  // Returns whether enhanced protection is enabled.
  // Convenience: equivalent to GetSafeBrowsingLevel() == 2.
  bool IsEnhancedProtectionEnabled() const;

  // Toggles enhanced protection.
  // When enabled, sets level to 2; when disabled, sets level to 1.
  void SetEnhancedProtection(bool enabled);

  // -- Password protection ------------------------------------------------

  // Returns the password protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 (standard).
  int GetPasswordProtectionLevel() const;

  // Sets the password protection level.
  // Values are clamped to 0-2.
  // Fires OnSafetySettingsChanged observer notification.
  void SetPasswordProtectionLevel(int level);

  // Returns whether password reuse warnings are enabled.
  bool WarnOnPasswordReuse() const;

  // Sets whether password reuse warnings are enabled.
  // Fires OnSafetySettingsChanged observer notification.
  void SetWarnOnPasswordReuse(bool enabled);

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraSafetyObserver* observer);
  void RemoveObserver(AstraSafetyObserver* observer);

  // -- Notification helpers (public for testing) -------------------------

  // Notify all observers that a threat has been detected.
  void NotifyThreatDetected(const AstraThreatInfo& threat);

  // Notify all observers that a threat has been blocked.
  void NotifyThreatBlocked(const AstraThreatInfo& threat);

  // Notify all observers that the user proceeded past a warning.
  void NotifyUserProceeded(const GURL& url);

  // Notify all observers that security status has changed.
  void NotifySecurityStatusChanged(const AstraSecurityStatus& status);

  // Notify all observers that safe browsing has been toggled.
  void NotifySafeBrowsingToggled(bool enabled);

  // Notify all observers that safety settings have changed.
  void NotifySafetySettingsChanged();

  // Notify all observers that a password breach has been detected.
  void NotifyPasswordBreachDetected(const std::string& site);

  // -- Utility methods ----------------------------------------------------

  // Returns a human-readable label for a safety level.
  static std::string GetSafetyLevelLabel(AstraSafetyLevel level);

  // Returns a human-readable label for a threat type.
  static std::string GetThreatTypeLabel(AstraThreatType type);

  // Returns the icon identifier for a threat type.
  static std::string GetThreatTypeIcon(AstraThreatType type);

  // Returns whether a threat type is considered severe.
  // Severe threats: malware, deceptive, unwanted, harmful.
  static bool IsSevereThreat(AstraThreatType type);

  // Returns a formatted description string for a threat.
  static std::string FormatThreatDescription(const AstraThreatInfo& threat);

  // Returns a human-readable label for a protection level (0-2).
  static std::string GetProtectionLevelLabel(int level);

  // -- Bulk operations ----------------------------------------------------

  // Adds multiple sites to the allowlist.
  void AllowSites(const std::vector<GURL>& urls);

  // Adds multiple sites to the blocklist.
  void BlockSites(const std::vector<GURL>& urls);

  // Removes multiple sites from the allowlist.
  void RemoveSitesFromAllowlist(const std::vector<GURL>& urls);

  // -- Safety check -------------------------------------------------------

  // Runs a full safety check.
  // Updates the internal safety state and notifies observers.
  //
  // TODO(astra): Delegate to Chromium's safety check feature.
  //   Chromium owner: SafetyCheck (chrome/browser/safety_check/)
  void RunSafetyCheck();

  // Returns the results of the last safety check.
  // Returns an empty vector if no check has been run.
  std::vector<std::string> GetSafetyCheckResults() const;

  // -- Presentation settings (getters) -----------------------------------

  bool GetShowSecurityButton() const;
  void SetShowSecurityButton(bool show);

  bool GetShowThreatNotifications() const;
  void SetShowThreatNotifications(bool show);

  bool GetBlockDangerousDownloads() const;
  void SetBlockDangerousDownloads(bool block);

  bool GetWarnOnDangerousDownloads() const;
  void SetWarnOnDangerousDownloads(bool warn);

  bool GetAutoReportSafetyIssues() const;
  void SetAutoReportSafetyIssues(bool report);

  bool GetMixedContentWarning() const;
  void SetMixedContentWarning(bool warn);

  bool GetShowSiteInfoButton() const;
  void SetShowSiteInfoButton(bool show);

  bool GetSafetyCheckReminders() const;
  void SetSafetyCheckReminders(bool remind);

  bool GetCookieProtection() const;
  void SetCookieProtection(bool enable);

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // Clamp safe browsing level to valid range (0-2).
  static int ClampSafeBrowsingLevel(int level);

  // Clamp password protection level to valid range (0-2).
  static int ClampPasswordProtectionLevel(int level);

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for safety state changes.
  base::ObserverList<AstraSafetyObserver> observers_;

  // In-memory threat history (most recent first).
  // TODO(astra): This should be projected from Chromium's safe browsing
  //   history, not stored locally. For the overlay, we keep a small
  //   in-memory list.
  std::vector<AstraThreatInfo> threat_history_;

  // In-memory password breach count.
  // TODO(astra): Project from PasswordHealthChecker.
  size_t password_breach_count_ = 0;

  // In-memory safety check results.
  std::vector<std::string> safety_check_results_;

  // Tracks whether we're currently observing safe browsing service.
  // TODO(astra): Flip this to true once observer integration is implemented.
  bool is_observing_safe_browsing_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SAFETY_HELPER_H_
