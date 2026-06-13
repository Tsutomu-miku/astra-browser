// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_safety_helper.h"

#include <algorithm>
#include <string>

#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout. In this overlay repo, the
// types are forward-declared in the header and the real definitions
// come from Chromium at build time.
//
// Chromium owner: SafeBrowsingService
//   (components/safe_browsing/core/browser/safe_browsing_service.h)
// Chromium owner: SecurityStateModel
//   (components/security_state/core/security_state.h)
// Chromium owner: SafeBrowsingDatabaseManager
//   (components/safe_browsing/core/browser/db/database_manager.h)

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Clamp helper for safe browsing level (0-2).
int ClampLevel(int level) {
  if (level < 0) return 0;
  if (level > 2) return 2;
  return level;
}

// Maximum number of threats to keep in history.
constexpr size_t kMaxThreatHistory = 100;

// Maximum number of safety check results to store.
constexpr size_t kMaxSafetyCheckResults = 20;

// Maximum number of sites in allowlist/blocklist.
// TODO(astra): These are placeholders. Real implementation uses Chromium's
//   safe browsing allowlist.
constexpr size_t kMaxListSize = 100;

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSafetyHelper::AstraSafetyHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing SafeBrowsingService for live threat updates.
  //
  // Chromium observer: SafeBrowsingServiceObserver
  //   (components/safe_browsing/core/browser/safe_browsing_service.h)
  //
  // Typical pattern:
  //   auto* sb_service = safe_browsing::SafeBrowsingServiceFactory::
  //       GetSafeBrowsingService();
  //   if (sb_service) {
  //     sb_service->AddObserver(this);
  //     is_observing_safe_browsing_ = true;
  //   }
}

AstraSafetyHelper::~AstraSafetyHelper() {
  // Observers should already be cleaned up by Shutdown().
  DCHECK(!is_observing_safe_browsing_);
}

void AstraSafetyHelper::Shutdown() {
  // TODO(astra): Remove observer from SafeBrowsingService.
  //   auto* sb_service = safe_browsing::SafeBrowsingServiceFactory::
  //       GetSafeBrowsingService();
  //   if (sb_service && is_observing_safe_browsing_) {
  //     sb_service->RemoveObserver(this);
  //     is_observing_safe_browsing_ = false;
  //   }
  is_observing_safe_browsing_ = false;
  profile_ = nullptr;
}

// =========================================================================
// Security status queries
// =========================================================================

AstraSecurityStatus AstraSafetyHelper::GetSecurityStatus() const {
  AstraSecurityStatus status;

  // TODO(astra): Project real security state from Chromium's
  //   SecurityStateModel and SSLStatus.
  //
  // Chromium owner: SecurityStateModel
  //   (components/security_state/core/security_state.h)
  // Chromium owner: SSLStatus (content/public/browser/ssl_status.h)

  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return status;
  }

  // Derive basic status from safe browsing settings.
  bool sb_enabled = prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled);
  int sb_level = prefs->GetInteger(prefs::kPrefSafeBrowsingLevel);

  status.is_secure = sb_enabled;
  status.has_valid_cert = sb_enabled;
  status.certificate_status = sb_enabled ? "Valid" : "Unknown";
  status.has_mixed_content = !prefs->GetBoolean(prefs::kPrefMixedContentWarning);
  status.security_level = sb_enabled
      ? (sb_level == 2 ? AstraSafetyLevel::kSafe : AstraSafetyLevel::kSafe)
      : AstraSafetyLevel::kUnknown;

  if (!sb_enabled) {
    status.security_level = AstraSafetyLevel::kUnknown;
  } else if (threat_history_.empty()) {
    status.security_level = AstraSafetyLevel::kSafe;
  } else {
    // Check if any recent threats were severe.
    bool has_severe = false;
    bool has_blocked = false;
    for (const auto& threat : threat_history_) {
      if (IsSevereThreat(threat.threat_type)) {
        has_severe = true;
        if (threat.is_blocked) {
          has_blocked = true;
        }
      }
    }
    if (has_blocked) {
      status.security_level = AstraSafetyLevel::kDangerous;
    } else if (has_severe) {
      status.security_level = AstraSafetyLevel::kWarning;
    } else {
      status.security_level = AstraSafetyLevel::kSafe;
    }
  }

  return status;
}

AstraSafetyLevel AstraSafetyHelper::GetSafetyLevel() const {
  return GetSecurityStatus().security_level;
}

// =========================================================================
// Safe browsing enable/disable
// =========================================================================

bool AstraSafetyHelper::IsSafeBrowsingEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSafeBrowsingEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled);
}

void AstraSafetyHelper::SetSafeBrowsingEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSafeBrowsingEnabled, enabled);
  NotifySafeBrowsingToggled(enabled);
  NotifySafetySettingsChanged();
}

// =========================================================================
// Threat history
// =========================================================================

size_t AstraSafetyHelper::GetThreatCount() const {
  return threat_history_.size();
}

std::vector<AstraThreatInfo> AstraSafetyHelper::GetRecentThreats(
    int max_count) const {
  if (max_count <= 0 || static_cast<size_t>(max_count) >= threat_history_.size()) {
    return threat_history_;
  }
  return std::vector<AstraThreatInfo>(
      threat_history_.begin(),
      threat_history_.begin() + max_count);
}

size_t AstraSafetyHelper::GetBlockedThreatCount() const {
  size_t count = 0;
  for (const auto& threat : threat_history_) {
    if (threat.is_blocked) {
      count++;
    }
  }
  return count;
}

// =========================================================================
// Password breach tracking
// =========================================================================

size_t AstraSafetyHelper::GetPasswordBreachCount() const {
  // TODO(astra): Project from PasswordHealthChecker.
  //   For the overlay, return the in-memory count.
  return password_breach_count_;
}

// =========================================================================
// Site safety checks
// =========================================================================

bool AstraSafetyHelper::IsSiteSafe(const GURL& url) const {
  if (!url.is_valid() || url.is_empty()) {
    return true;  // Empty/invalid URLs are trivially "safe" (no threat).
  }

  // Check blocklist first.
  if (IsSiteBlocked(url)) {
    return false;
  }

  // Check allowlist.
  if (IsSiteAllowed(url)) {
    return true;
  }

  // TODO(astra): Consult Chromium's SafeBrowsingDatabaseManager.
  //   For the overlay, assume sites are safe unless explicitly blocked.
  //
  // Chromium owner: SafeBrowsingDatabaseManager
  //   (components/safe_browsing/core/browser/db/database_manager.h)

  return true;
}

// =========================================================================
// Site allowlist / blocklist
// =========================================================================

void AstraSafetyHelper::AllowSite(const GURL& url) {
  if (!url.is_valid() || url.is_empty()) {
    return;
  }

  // TODO(astra): Delegate to Chromium's safe browsing allowlist.
  //   For the overlay, we track this in memory.
  //
  // Chromium pref: safebrowsing.whitelist_domains (deprecated)
  //   or enterprise policy: SafeBrowsingAllowlistDomains

  // Idempotency: if already allowed, no-op.
  // (In-memory implementation uses a simple list; real impl uses pref.)
  // For now, this is a no-op in terms of state change since we don't
  // persist the list. The method exists for API completeness.
}

void AstraSafetyHelper::BlockSite(const GURL& url) {
  if (!url.is_valid() || url.is_empty()) {
    return;
  }

  // TODO(astra): Delegate to Chromium's safe browsing blocklist.
  //   For the overlay, we track this in memory.
}

bool AstraSafetyHelper::IsSiteAllowed(const GURL& url) const {
  if (!url.is_valid() || url.is_empty()) {
    return false;
  }

  // TODO(astra): Check Chromium's safe browsing allowlist.
  //   For the overlay, return false (no allowlist entries yet).
  return false;
}

bool AstraSafetyHelper::IsSiteBlocked(const GURL& url) const {
  if (!url.is_valid() || url.is_empty()) {
    return false;
  }

  // TODO(astra): Check Chromium's safe browsing blocklist.
  //   For the overlay, return false (no blocklist entries yet).
  return false;
}

// =========================================================================
// Threat history management
// =========================================================================

void AstraSafetyHelper::ClearThreatHistory() {
  threat_history_.clear();
  password_breach_count_ = 0;
}

// =========================================================================
// Protection level
// =========================================================================

int AstraSafetyHelper::GetSafeBrowsingLevel() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSafeBrowsingLevel;
  }
  return prefs->GetInteger(prefs::kPrefSafeBrowsingLevel);
}

void AstraSafetyHelper::SetSafeBrowsingLevel(int level) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampSafeBrowsingLevel(level);
  int current = prefs->GetInteger(prefs::kPrefSafeBrowsingLevel);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefSafeBrowsingLevel, clamped);

  // If level changed to 0, safe browsing is effectively off.
  if (clamped == 0 && prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled)) {
    prefs->SetBoolean(prefs::kPrefSafeBrowsingEnabled, false);
    NotifySafeBrowsingToggled(false);
  } else if (clamped > 0 && !prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled)) {
    prefs->SetBoolean(prefs::kPrefSafeBrowsingEnabled, true);
    NotifySafeBrowsingToggled(true);
  }

  NotifySafetySettingsChanged();
}

bool AstraSafetyHelper::IsEnhancedProtectionEnabled() const {
  return GetSafeBrowsingLevel() == 2;
}

void AstraSafetyHelper::SetEnhancedProtection(bool enabled) {
  if (enabled) {
    SetSafeBrowsingLevel(2);
  } else {
    // If disabling enhanced and currently at level 2, drop to standard (1).
    if (GetSafeBrowsingLevel() == 2) {
      SetSafeBrowsingLevel(1);
    }
  }
}

// =========================================================================
// Password protection
// =========================================================================

int AstraSafetyHelper::GetPasswordProtectionLevel() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordProtectionLevel;
  }
  return prefs->GetInteger(prefs::kPrefPasswordProtectionLevel);
}

void AstraSafetyHelper::SetPasswordProtectionLevel(int level) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampPasswordProtectionLevel(level);
  int current = prefs->GetInteger(prefs::kPrefPasswordProtectionLevel);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefPasswordProtectionLevel, clamped);
  NotifySafetySettingsChanged();
}

bool AstraSafetyHelper::WarnOnPasswordReuse() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultWarnOnPasswordReuse;
  }
  return prefs->GetBoolean(prefs::kPrefWarnOnPasswordReuse);
}

void AstraSafetyHelper::SetWarnOnPasswordReuse(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefWarnOnPasswordReuse) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefWarnOnPasswordReuse, enabled);
  NotifySafetySettingsChanged();
}

// =========================================================================
// Observers
// =========================================================================

void AstraSafetyHelper::AddObserver(AstraSafetyObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSafetyHelper::RemoveObserver(AstraSafetyObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraSafetyHelper::NotifyThreatDetected(const AstraThreatInfo& threat) {
  // Add to history.
  threat_history_.insert(threat_history_.begin(), threat);
  if (threat_history_.size() > kMaxThreatHistory) {
    threat_history_.pop_back();
  }

  for (auto& observer : observers_) {
    observer.OnThreatDetected(threat);
  }
}

void AstraSafetyHelper::NotifyThreatBlocked(const AstraThreatInfo& threat) {
  // Add to history as blocked.
  AstraThreatInfo blocked = threat;
  blocked.is_blocked = true;
  threat_history_.insert(threat_history_.begin(), blocked);
  if (threat_history_.size() > kMaxThreatHistory) {
    threat_history_.pop_back();
  }

  for (auto& observer : observers_) {
    observer.OnThreatBlocked(threat);
  }
}

void AstraSafetyHelper::NotifyUserProceeded(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnUserProceeded(url);
  }
}

void AstraSafetyHelper::NotifySecurityStatusChanged(
    const AstraSecurityStatus& status) {
  for (auto& observer : observers_) {
    observer.OnSecurityStatusChanged(status);
  }
}

void AstraSafetyHelper::NotifySafeBrowsingToggled(bool enabled) {
  for (auto& observer : observers_) {
    observer.OnSafeBrowsingToggled(enabled);
  }
}

void AstraSafetyHelper::NotifySafetySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnSafetySettingsChanged();
  }
}

void AstraSafetyHelper::NotifyPasswordBreachDetected(const std::string& site) {
  password_breach_count_++;

  for (auto& observer : observers_) {
    observer.OnPasswordBreachDetected(site);
  }

  // Also fire safety settings changed since breach count affects safety level.
  NotifySafetySettingsChanged();
}

// =========================================================================
// Utility methods
// =========================================================================

// static
std::string AstraSafetyHelper::GetSafetyLevelLabel(AstraSafetyLevel level) {
  switch (level) {
    case AstraSafetyLevel::kSafe:
      return "Safe";
    case AstraSafetyLevel::kWarning:
      return "Warning";
    case AstraSafetyLevel::kDangerous:
      return "Dangerous";
    case AstraSafetyLevel::kUnknown:
      return "Unknown";
  }
  return "Unknown";
}

// static
std::string AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType type) {
  switch (type) {
    case AstraThreatType::kMalware:
      return "Malware";
    case AstraThreatType::kDeceptive:
      return "Deceptive content";
    case AstraThreatType::kUnwanted:
      return "Unwanted software";
    case AstraThreatType::kHarmful:
      return "Harmful download";
    case AstraThreatType::kPasswordReuse:
      return "Password reuse";
    case AstraThreatType::kMixedContent:
      return "Mixed content";
  }
  return "Unknown threat";
}

// static
std::string AstraSafetyHelper::GetThreatTypeIcon(AstraThreatType type) {
  switch (type) {
    case AstraThreatType::kMalware:
      return "shield_error";
    case AstraThreatType::kDeceptive:
      return "phishing";
    case AstraThreatType::kUnwanted:
      return "extension_error";
    case AstraThreatType::kHarmful:
      return "download_off";
    case AstraThreatType::kPasswordReuse:
      return "password";
    case AstraThreatType::kMixedContent:
      return "http";
  }
  return "warning";
}

// static
bool AstraSafetyHelper::IsSevereThreat(AstraThreatType type) {
  switch (type) {
    case AstraThreatType::kMalware:
    case AstraThreatType::kDeceptive:
    case AstraThreatType::kUnwanted:
    case AstraThreatType::kHarmful:
      return true;
    case AstraThreatType::kPasswordReuse:
    case AstraThreatType::kMixedContent:
      return false;
  }
  return false;
}

// static
std::string AstraSafetyHelper::FormatThreatDescription(
    const AstraThreatInfo& threat) {
  std::string desc = GetThreatTypeLabel(threat.threat_type);
  if (!threat.url.is_empty()) {
    desc += " on " + threat.url.host();
  }
  if (threat.is_blocked) {
    desc += " (blocked)";
  }
  if (!threat.user_action.empty()) {
    desc += " — user: " + threat.user_action;
  }
  return desc;
}

// static
std::string AstraSafetyHelper::GetProtectionLevelLabel(int level) {
  switch (level) {
    case 0:
      return "Off";
    case 1:
      return "Standard protection";
    case 2:
      return "Enhanced protection";
    default:
      return "Unknown";
  }
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraSafetyHelper::AllowSites(const std::vector<GURL>& urls) {
  for (const auto& url : urls) {
    AllowSite(url);
  }
}

void AstraSafetyHelper::BlockSites(const std::vector<GURL>& urls) {
  for (const auto& url : urls) {
    BlockSite(url);
  }
}

void AstraSafetyHelper::RemoveSitesFromAllowlist(
    const std::vector<GURL>& urls) {
  // TODO(astra): Implement removal from allowlist.
  //   For now, this is a no-op since we don't persist the allowlist.
  //   Real implementation would modify the pref or Chromium's list.
  if (urls.empty()) {
    return;
  }
}

// =========================================================================
// Safety check
// =========================================================================

void AstraSafetyHelper::RunSafetyCheck() {
  safety_check_results_.clear();

  // TODO(astra): Delegate to Chromium's safety check feature.
  //   Chromium owner: SafetyCheck (chrome/browser/safety_check/)
  //
  // For the overlay, run a basic simulated check:

  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // Check 1: Safe browsing status
  if (prefs->GetBoolean(prefs::kPrefSafeBrowsingEnabled)) {
    int level = prefs->GetInteger(prefs::kPrefSafeBrowsingLevel);
    if (level == 2) {
      safety_check_results_.push_back("Safe browsing: Enhanced protection");
    } else {
      safety_check_results_.push_back("Safe browsing: Standard protection");
    }
  } else {
    safety_check_results_.push_back("Safe browsing: Disabled");
  }

  // Check 2: Passwords
  if (password_breach_count_ > 0) {
    safety_check_results_.push_back(
        "Passwords: " + std::to_string(password_breach_count_) +
        " breached passwords detected");
  } else {
    safety_check_results_.push_back("Passwords: No breached passwords");
  }

  // Check 3: Extensions
  // TODO(astra): Check extension safety. Placeholder.
  safety_check_results_.push_back("Extensions: No harmful extensions");

  // Check 4: Updates
  // TODO(astra): Check update status. Placeholder.
  safety_check_results_.push_back("Updates: Chrome is up to date");

  // Check 5: Threats
  if (!threat_history_.empty()) {
    size_t blocked = GetBlockedThreatCount();
    safety_check_results_.push_back(
        "Threats: " + std::to_string(threat_history_.size()) +
        " detected, " + std::to_string(blocked) + " blocked");
  } else {
    safety_check_results_.push_back("Threats: No recent threats");
  }

  // Trim to max.
  if (safety_check_results_.size() > kMaxSafetyCheckResults) {
    safety_check_results_.resize(kMaxSafetyCheckResults);
  }

  // Notify observers.
  AstraSecurityStatus status = GetSecurityStatus();
  NotifySecurityStatusChanged(status);
  NotifySafetySettingsChanged();
}

std::vector<std::string> AstraSafetyHelper::GetSafetyCheckResults() const {
  return safety_check_results_;
}

// =========================================================================
// Presentation settings
// =========================================================================

// -- Show security button --

bool AstraSafetyHelper::GetShowSecurityButton() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSecurityButton;
  }
  return prefs->GetBoolean(prefs::kPrefShowSecurityButton);
}

void AstraSafetyHelper::SetShowSecurityButton(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowSecurityButton) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowSecurityButton, show);
  NotifySafetySettingsChanged();
}

// -- Show threat notifications --

bool AstraSafetyHelper::GetShowThreatNotifications() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowThreatNotifications;
  }
  return prefs->GetBoolean(prefs::kPrefShowThreatNotifications);
}

void AstraSafetyHelper::SetShowThreatNotifications(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowThreatNotifications) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowThreatNotifications, show);
  NotifySafetySettingsChanged();
}

// -- Block dangerous downloads --

bool AstraSafetyHelper::GetBlockDangerousDownloads() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultBlockDangerousDownloads;
  }
  return prefs->GetBoolean(prefs::kPrefBlockDangerousDownloads);
}

void AstraSafetyHelper::SetBlockDangerousDownloads(bool block) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefBlockDangerousDownloads) == block) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefBlockDangerousDownloads, block);
  NotifySafetySettingsChanged();
}

// -- Warn on dangerous downloads --

bool AstraSafetyHelper::GetWarnOnDangerousDownloads() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultWarnOnDangerousDownloads;
  }
  return prefs->GetBoolean(prefs::kPrefWarnOnDangerousDownloads);
}

void AstraSafetyHelper::SetWarnOnDangerousDownloads(bool warn) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefWarnOnDangerousDownloads) == warn) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefWarnOnDangerousDownloads, warn);
  NotifySafetySettingsChanged();
}

// -- Auto report safety issues --

bool AstraSafetyHelper::GetAutoReportSafetyIssues() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutoReportSafetyIssues;
  }
  return prefs->GetBoolean(prefs::kPrefAutoReportSafetyIssues);
}

void AstraSafetyHelper::SetAutoReportSafetyIssues(bool report) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefAutoReportSafetyIssues) == report) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefAutoReportSafetyIssues, report);
  NotifySafetySettingsChanged();
}

// -- Mixed content warning --

bool AstraSafetyHelper::GetMixedContentWarning() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultMixedContentWarning;
  }
  return prefs->GetBoolean(prefs::kPrefMixedContentWarning);
}

void AstraSafetyHelper::SetMixedContentWarning(bool warn) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefMixedContentWarning) == warn) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefMixedContentWarning, warn);
  NotifySafetySettingsChanged();
}

// -- Show site info button --

bool AstraSafetyHelper::GetShowSiteInfoButton() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSiteInfoButton;
  }
  return prefs->GetBoolean(prefs::kPrefShowSiteInfoButton);
}

void AstraSafetyHelper::SetShowSiteInfoButton(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowSiteInfoButton) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowSiteInfoButton, show);
  NotifySafetySettingsChanged();
}

// -- Safety check reminders --

bool AstraSafetyHelper::GetSafetyCheckReminders() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSafetyCheckReminders;
  }
  return prefs->GetBoolean(prefs::kPrefSafetyCheckReminders);
}

void AstraSafetyHelper::SetSafetyCheckReminders(bool remind) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefSafetyCheckReminders) == remind) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSafetyCheckReminders, remind);
  NotifySafetySettingsChanged();
}

// -- Cookie protection --

bool AstraSafetyHelper::GetCookieProtection() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultCookieProtection;
  }
  return prefs->GetBoolean(prefs::kPrefCookieProtection);
}

void AstraSafetyHelper::SetCookieProtection(bool enable) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefCookieProtection) == enable) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefCookieProtection, enable);
  NotifySafetySettingsChanged();
}

// =========================================================================
// Prefs access
// =========================================================================

PrefService* AstraSafetyHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

// static
int AstraSafetyHelper::ClampSafeBrowsingLevel(int level) {
  return ClampLevel(level);
}

// static
int AstraSafetyHelper::ClampPasswordProtectionLevel(int level) {
  return ClampLevel(level);
}

}  // namespace astra
