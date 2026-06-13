#ifndef ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraNotificationService;

// =========================================================================
// Factory for AstraNotificationService
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraNotificationService.
//
// Incognito behavior: kOwnInstance — notifications are per-browsing-context.
// An incognito window has its own notification state that doesn't affect the
// main profile's active notifications.  Presentation setting prefs are still
// shared with the original profile (via pref forwarding), but the runtime
// notification state is per-instance.
//
// Guest: kOwnInstance.
// System: kNone.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// Chromium subsystem reused: NotificationService / MessageCenter
//   (chrome/browser/notifications/notification_service.h)
// Astra projection: AstraNotificationService
// TODO(astra): Wire into Chromium's notification system as an observer.
//   Patch point: message_center::MessageCenter::AddObserver() or
//   chrome/browser/notifications/notification_display_service_impl.cc.
// =========================================================================

class AstraNotificationServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraNotificationService instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraNotificationService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraNotificationServiceFactory* GetInstance();

  // Registers notification-related profile prefs on the given registry.
  //
  // These prefs control Astra-specific notification presentation settings
  // and do not duplicate Chromium's native notification prefs.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraNotificationServiceFactory>;

  AstraNotificationServiceFactory();
  ~AstraNotificationServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_FACTORY_H_
