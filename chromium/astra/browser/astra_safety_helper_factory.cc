// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_safety_helper_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_safety_helper.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSafetyHelperFactory::AstraSafetyHelperFactory()
    : ProfileKeyedServiceFactory(
          "AstraSafetyHelper",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Regular profile: own instance — each profile has its own
              // safety settings and threat history.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance — guest sessions get their own
              // ephemeral safety state.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user-facing safety features.
              .Build()) {}

AstraSafetyHelperFactory::~AstraSafetyHelperFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraSafetyHelperFactory* AstraSafetyHelperFactory::GetInstance() {
  static base::NoDestructor<AstraSafetyHelperFactory> instance;
  return instance.get();
}

// static
AstraSafetyHelper* AstraSafetyHelperFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraSafetyHelper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraSafetyHelperFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Safety / safe browsing presentation preferences.
  //
  // Safe browsing state is fully owned by Chromium's SafeBrowsingService.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: SafeBrowsingService
  //   (components/safe_browsing/core/browser/safe_browsing_service.h)
  // Chromium factory: SafeBrowsingServiceFactory
  //
  // These Astra-specific prefs control only presentation — they never
  // store safe browsing data or change Chromium's safe browsing behavior.
  // See AstraSafetyHelper for the projection layer.

  // Whether safe browsing is enabled in Astra UI.
  // Default: true — safe browsing is on by default for security.
  registry->RegisterBooleanPref(prefs::kPrefSafeBrowsingEnabled,
                                prefs::kDefaultSafeBrowsingEnabled);

  // Safe browsing protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 = standard protection.
  registry->RegisterIntegerPref(prefs::kPrefSafeBrowsingLevel,
                                prefs::kDefaultSafeBrowsingLevel);

  // Whether enhanced protection is shown as a separate toggle.
  // Default: false — shown as part of the level setting.
  registry->RegisterBooleanPref(prefs::kPrefEnhancedProtection,
                                prefs::kDefaultEnhancedProtection);

  // Whether password reuse warnings are shown.
  // Default: true — warnings are on by default for security.
  registry->RegisterBooleanPref(prefs::kPrefWarnOnPasswordReuse,
                                prefs::kDefaultWarnOnPasswordReuse);

  // Password protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 = standard protection.
  registry->RegisterIntegerPref(prefs::kPrefPasswordProtectionLevel,
                                prefs::kDefaultPasswordProtectionLevel);

  // Whether the security button is shown in the toolbar.
  // Default: true — security info is easily accessible.
  registry->RegisterBooleanPref(prefs::kPrefShowSecurityButton,
                                prefs::kDefaultShowSecurityButton);

  // Whether threat notifications are shown.
  // Default: true — users should be aware of threats.
  registry->RegisterBooleanPref(prefs::kPrefShowThreatNotifications,
                                prefs::kDefaultShowThreatNotifications);

  // Whether dangerous downloads are blocked.
  // Default: true — block dangerous downloads for safety.
  registry->RegisterBooleanPref(prefs::kPrefBlockDangerousDownloads,
                                prefs::kDefaultBlockDangerousDownloads);

  // Whether to warn on dangerous downloads.
  // Default: true — warn users about potentially harmful downloads.
  registry->RegisterBooleanPref(prefs::kPrefWarnOnDangerousDownloads,
                                prefs::kDefaultWarnOnDangerousDownloads);

  // Whether to auto-report safety issues.
  // Default: false — opt-in for privacy.
  registry->RegisterBooleanPref(prefs::kPrefAutoReportSafetyIssues,
                                prefs::kDefaultAutoReportSafetyIssues);

  // Whether to show mixed content warnings.
  // Default: true — mixed content is a security concern.
  registry->RegisterBooleanPref(prefs::kPrefMixedContentWarning,
                                prefs::kDefaultMixedContentWarning);

  // Whether the site info button is shown.
  // Default: true — site info is easily accessible.
  registry->RegisterBooleanPref(prefs::kPrefShowSiteInfoButton,
                                prefs::kDefaultShowSiteInfoButton);

  // Whether safety check reminders are shown.
  // Default: true — encourage regular safety checks.
  registry->RegisterBooleanPref(prefs::kPrefSafetyCheckReminders,
                                prefs::kDefaultSafetyCheckReminders);

  // Whether third-party cookie protection is enabled.
  // Default: true — cookie protection is a privacy feature.
  registry->RegisterBooleanPref(prefs::kPrefCookieProtection,
                                prefs::kDefaultCookieProtection);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraSafetyHelperFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraSafetyHelper>(profile);
}

}  // namespace astra
