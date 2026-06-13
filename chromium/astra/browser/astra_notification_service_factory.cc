// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_notification_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_notification_service.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNotificationServiceFactory::AstraNotificationServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraNotificationService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Incognito: own instance.
              // Notifications are per-browsing-context.  An incognito window
              // has its own notification state that doesn't affect the main
              // profile's active notifications.  Presentation prefs are still
              // shared via pref forwarding, but runtime state is per-instance.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed — system notifications
              // are handled directly by Chromium's notification system.
              .Build()) {}

AstraNotificationServiceFactory::~AstraNotificationServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraNotificationServiceFactory*
AstraNotificationServiceFactory::GetInstance() {
  static base::NoDestructor<AstraNotificationServiceFactory> instance;
  return instance.get();
}

// static
AstraNotificationService*
AstraNotificationServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraNotificationService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraNotificationServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Whether notifications are enabled globally.
  // Default: true — notifications are on by default.
  registry->RegisterBooleanPref(prefs::kPrefNotificationsEnabled,
                                prefs::kDefaultNotificationsEnabled);

  // Whether do-not-disturb mode is enabled.
  // Default: false — DND is off by default.
  registry->RegisterBooleanPref(prefs::kPrefNotificationDoNotDisturb,
                                prefs::kDefaultNotificationDoNotDisturb);

  // Whether message previews are shown in notifications.
  // Default: true — show full message content in notification popups.
  registry->RegisterBooleanPref(prefs::kPrefNotificationShowPreviews,
                                prefs::kDefaultNotificationShowPreviews);

  // Whether notification sounds are played.
  // Default: true — sound is on by default.
  registry->RegisterBooleanPref(prefs::kPrefNotificationSoundEnabled,
                                prefs::kDefaultNotificationSoundEnabled);

  // Auto-dismiss timeout in seconds.
  // Default: 8 seconds — standard notification timeout.
  registry->RegisterIntegerPref(prefs::kPrefNotificationTimeoutSeconds,
                                prefs::kDefaultNotificationTimeoutSeconds);

  // Maximum number of visible notifications at once.
  // Default: 5 — reasonable balance of visibility and clutter.
  registry->RegisterIntegerPref(prefs::kPrefNotificationMaxVisible,
                                prefs::kDefaultNotificationMaxVisible);

  // Notification position on screen.
  // Default: "top_right" — standard desktop notification position.
  registry->RegisterStringPref(prefs::kPrefNotificationPosition,
                               prefs::kDefaultNotificationPosition);

  // Whether to show the notification icon.
  // Default: true — icons help identify notification source.
  registry->RegisterBooleanPref(prefs::kPrefNotificationShowIcon,
                                prefs::kDefaultNotificationShowIcon);

  // Whether to show the notification timestamp.
  // Default: true — timestamps provide useful context.
  registry->RegisterBooleanPref(prefs::kPrefNotificationShowTimestamp,
                                prefs::kDefaultNotificationShowTimestamp);

  // Whether to show the close button on notifications.
  // Default: true — users expect to be able to dismiss notifications.
  registry->RegisterBooleanPref(prefs::kPrefNotificationShowCloseButton,
                                prefs::kDefaultNotificationShowCloseButton);

  // Notification visual style.
  // Default: "default" — standard notification style.
  registry->RegisterStringPref(prefs::kPrefNotificationStyle,
                               prefs::kDefaultNotificationStyle);

  // Whether similar notifications are stacked / grouped.
  // Default: true — stacking reduces visual clutter.
  registry->RegisterBooleanPref(prefs::kPrefNotificationStackNotifications,
                                prefs::kDefaultNotificationStackNotifications);

  // Whether quiet mode is enabled (no sound, no popups, just badge).
  // Default: false — quiet mode is off by default.
  registry->RegisterBooleanPref(prefs::kPrefNotificationQuietMode,
                                prefs::kDefaultNotificationQuietMode);

  // Maximum number of history items to remember.
  // Default: 100 — reasonable history depth without excessive memory.
  registry->RegisterIntegerPref(prefs::kPrefNotificationHistorySize,
                                prefs::kDefaultNotificationHistorySize);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraNotificationServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraNotificationService>(profile);
}

}  // namespace astra
