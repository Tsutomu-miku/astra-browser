#ifndef ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

// Notification type enum.
//
// These categorize notifications for filtering, styling, and priority
// determination.  The actual notification delivery is owned by Chromium's
// NotificationPlatformBridge and message_center.  Astra adds presentation
// metadata and projection on top of Chromium's notification system.
//
// Chromium subsystem: MessageCenter / NotificationPlatformBridge
//   (ui/message_center/message_center.h)
// Astra projection: AstraNotificationService
enum class AstraNotificationType {
  kInfo,       // Informational notification.
  kWarning,    // Warning notification.
  kError,      // Error notification.
  kSuccess,    // Success / completion notification.
  kDownload,   // Download-related notification.
  kExtension,  // Extension-related notification.
  kSystem,     // System-level notification.
};

// Notification item struct.
//
// Represents a single notification with all its metadata.
// This struct is used both for active notifications and for history entries.
//
// Chromium analog: message_center::Notification
//   (ui/message_center/public/cpp/notification.h)
// Astra projection: wraps notification metadata for Astra UI surfaces.
struct AstraNotification {
  std::string id;                  // Unique notification ID.
  std::string title;               // Notification title.
  std::string message;             // Notification body message.
  AstraNotificationType type = AstraNotificationType::kInfo;  // Notification type.
  std::string source;              // Source of notification (e.g. "downloads").
  base::Time created_time;         // When notification was created.
  bool is_read = false;            // Whether user has read it.
  int priority = 0;                // Priority level (0-3, higher = more important).
  std::string action_label;        // Primary action button label.
  std::string secondary_action_label;  // Secondary action label.
};

// =========================================================================
// AstraNotificationObserver
// =========================================================================
//
// Observer interface for AstraNotificationService.
// All methods have empty default implementations so observers only need to
// override the methods they care about.
//
// Chromium pattern: base::CheckedObserver + ObserverList
// =========================================================================

class AstraNotificationObserver : public base::CheckedObserver {
 public:
  // Called when a notification is shown.
  virtual void OnNotificationShown(const AstraNotification& notification) {}

  // Called when a notification is closed (dismissed by user or timeout).
  virtual void OnNotificationClosed(const std::string& notification_id) {}

  // Called when a notification body is clicked.
  virtual void OnNotificationClicked(const std::string& notification_id) {}

  // Called when an action button on a notification is clicked.
  virtual void OnNotificationActionClicked(
      const std::string& notification_id,
      const std::string& action) {}

  // Called when a notification is marked as read.
  virtual void OnNotificationRead(const std::string& notification_id) {}

  // Called when all notifications are cleared.
  virtual void OnAllNotificationsCleared() {}

  // Called when notification presentation settings change.
  virtual void OnNotificationSettingsChanged() {}

  // Called when the unread notification count changes.
  virtual void OnUnreadCountChanged(int unread_count) {}

 protected:
  ~AstraNotificationObserver() override = default;
};

// =========================================================================
// AstraNotificationService
// =========================================================================
//
// Profile-keyed service that manages Astra-specific notification state and
// presentation settings.
//
// This service PROJECTS Chromium's notification system — it does not replace
// it.  Chromium's MessageCenter and NotificationPlatformBridge handle actual
// notification display, scheduling, and platform integration.  Astra adds:
//   - Notification type metadata and categorization
//   - Per-source filtering and grouping
//   - Read/unread tracking
//   - History / recent notifications
//   - Presentation settings (position, style, timeout, etc.)
//   - Do-not-disturb mode
//   - Astra UI integration (sidebar badge, notification center)
//
// Chromium owner: MessageCenter / NotificationService
//   (chrome/browser/notifications/notification_service.h)
// Astra projection: AstraNotificationService
//
// TODO(astra): Wire into Chromium's notification system as an observer.
//   Patch point: message_center::MessageCenter::AddObserver() or
//   NotificationUIManager.  The service would listen to Chromium's
//   notification events and project them into Astra's metadata model.
// =========================================================================

class AstraNotificationService final : public KeyedService {
 public:
  explicit AstraNotificationService(Profile* profile);
  AstraNotificationService(const AstraNotificationService&) = delete;
  AstraNotificationService& operator=(const AstraNotificationService&) = delete;
  ~AstraNotificationService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraNotificationObserver* observer);
  void RemoveObserver(AstraNotificationObserver* observer);

  // -- Notification operations --------------------------------------------

  // Shows a notification.  If a notification with the same ID already exists,
  // it is updated (replaced).  Respects do-not-disturb and notifications
  // enabled settings.
  // Returns true if the notification was actually shown.
  bool ShowNotification(const AstraNotification& notification);

  // Closes (dismisses) a notification by ID.
  // No-op if the notification does not exist.
  void CloseNotification(const std::string& id);

  // Returns the notification with the given ID, or nullptr if not found.
  const AstraNotification* GetNotification(const std::string& id) const;

  // Returns all active (not closed) notifications.
  std::vector<AstraNotification> GetAllNotifications() const;

  // Returns the count of unread notifications.
  int GetUnreadCount() const;

  // Returns the total number of active notifications.
  int GetNotificationCount() const;

  // Returns active notifications filtered by type.
  std::vector<AstraNotification> GetNotificationsByType(
      AstraNotificationType type) const;

  // Returns active notifications filtered by source.
  std::vector<AstraNotification> GetNotificationsBySource(
      const std::string& source) const;

  // Marks a notification as read.  No-op if not found or already read.
  void MarkAsRead(const std::string& id);

  // Marks all active notifications as read.
  void MarkAllAsRead();

  // Clears (removes) all active notifications.
  void ClearAllNotifications();

  // Clears all active notifications of a given type.
  void ClearNotificationsByType(AstraNotificationType type);

  // Clears all active notifications from a given source.
  void ClearNotificationsBySource(const std::string& source);

  // Returns recent notification history (including closed ones).
  // |max_count| limits the number of entries returned (most recent first).
  std::vector<AstraNotification> GetNotificationHistory(int max_count) const;

  // -- Do not disturb ------------------------------------------------------

  // Returns true if do-not-disturb mode is enabled.
  bool DoNotDisturb() const;

  // Enables or disables do-not-disturb mode.
  // When DND is enabled, new notifications do not pop up but are still
  // recorded in the history and unread count.
  void SetDoNotDisturb(bool enabled);

  // -- Presentation settings ----------------------------------------------
  //
  // These settings control how notifications are presented in the UI.
  // They are persisted via PrefService.
  // Truth source: PrefService (read by service, written by service).
  // Views read these through the service, never directly from prefs.

  // Whether notifications are enabled globally.
  bool notifications_enabled() const { return notifications_enabled_; }
  void set_notifications_enabled(bool enabled);

  // Whether message previews are shown in notifications.
  bool show_notification_previews() const { return show_notification_previews_; }
  void set_show_notification_previews(bool show);

  // Whether notification sounds are played.
  bool notification_sound_enabled() const {
    return notification_sound_enabled_;
  }
  void set_notification_sound_enabled(bool enabled);

  // Auto-dismiss timeout in seconds.  Clamped to [1, 60].
  int notification_timeout_seconds() const {
    return notification_timeout_seconds_;
  }
  void set_notification_timeout_seconds(int seconds);

  // Maximum number of visible notifications at once.  Clamped to [1, 20].
  int max_visible_notifications() const { return max_visible_notifications_; }
  void set_max_visible_notifications(int max);

  // Notification position on screen.
  // Values: "top_right", "top_left", "bottom_right", "bottom_left".
  const std::string& notification_position() const {
    return notification_position_;
  }
  void set_notification_position(const std::string& position);

  // Whether to show the notification icon.
  bool show_notification_icon() const { return show_notification_icon_; }
  void set_show_notification_icon(bool show);

  // Whether to show the notification timestamp.
  bool show_notification_timestamp() const {
    return show_notification_timestamp_;
  }
  void set_show_notification_timestamp(bool show);

  // Whether to show the close button on notifications.
  bool show_close_button() const { return show_close_button_; }
  void set_show_close_button(bool show);

  // Notification visual style.
  // Values: "default", "compact", "minimal".
  const std::string& notification_style() const { return notification_style_; }
  void set_notification_style(const std::string& style);

  // Whether similar notifications are stacked / grouped.
  bool stack_notifications() const { return stack_notifications_; }
  void set_stack_notifications(bool stack);

  // Whether quiet mode is enabled (no sound, no popups, just badge).
  bool quiet_mode() const { return quiet_mode_; }
  void set_quiet_mode(bool enabled);

  // Maximum number of history items to remember.  Clamped to [10, 1000].
  int notification_history_size() const { return notification_history_size_; }
  void set_notification_history_size(int size);

  // -- Utility methods -----------------------------------------------------

  // Formats a notification timestamp for display.
  // Returns a human-readable string (e.g. "2m ago", "1h ago", or date).
  static std::string FormatNotificationTime(base::Time time);

  // Returns a human-readable label for a notification type.
  static std::string GetNotificationTypeLabel(AstraNotificationType type);

  // Returns the default priority for a given notification type.
  // Higher values = higher priority (range 0-3).
  static int GetDefaultPriority(AstraNotificationType type);

  // Returns true if a notification of the given type should be shown
  // based on current settings (notifications enabled, DND, quiet mode, etc.).
  bool ShouldShowNotification(AstraNotificationType type) const;

  // Truncates a message string to |max_length| characters, appending "..."
  // if truncation occurred.
  static std::string TruncateMessage(const std::string& message,
                                     int max_length);

  // -- Bulk operations ----------------------------------------------------

  // Shows multiple notifications at once.  Each notification is processed
  // individually (respects settings per-notification).
  // Returns the number of notifications that were actually shown.
  int ShowNotifications(const std::vector<AstraNotification>& notifications);

  // Closes multiple notifications by ID.
  void CloseNotifications(const std::vector<std::string>& ids);

  // Marks multiple notifications as read.
  void MarkAsRead(const std::vector<std::string>& ids);

 private:
  // Loads persisted preferences from the profile's PrefService.
  void LoadFromPrefs();

  // Saves current presentation settings to the profile's PrefService.
  void SaveSettingsToPrefs();

  // Adds a notification to the history list.
  // Maintains the history size limit.
  void AddToHistory(const AstraNotification& notification);

  // Recalculates and notifies observers of unread count changes.
  void UpdateUnreadCount();

  // Clamps and validates notification position string.
  static std::string ValidatePosition(const std::string& position);

  // Clamps and validates notification style string.
  static std::string ValidateStyle(const std::string& style);

  // Finds an active notification by ID.  Returns end() if not found.
  std::vector<AstraNotification>::iterator FindNotification(const std::string& id);
  std::vector<AstraNotification>::const_iterator FindNotification(
      const std::string& id) const;

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraNotificationObserver> observers_;

  // Active notifications (currently shown, not closed).
  std::vector<AstraNotification> active_notifications_;

  // Notification history (includes closed notifications).
  // Most recent first.  Limited by notification_history_size_.
  std::vector<AstraNotification> history_;

  // Cached unread count.
  int unread_count_ = 0;

  // -- Persisted presentation settings -------------------------------------

  bool notifications_enabled_ = true;
  bool show_notification_previews_ = true;
  bool notification_sound_enabled_ = true;
  int notification_timeout_seconds_ = 8;      // Clamped [1, 60]
  int max_visible_notifications_ = 5;        // Clamped [1, 20]
  std::string notification_position_ = "top_right";
  bool show_notification_icon_ = true;
  bool show_notification_timestamp_ = true;
  bool show_close_button_ = true;
  std::string notification_style_ = "default";
  bool stack_notifications_ = true;
  bool quiet_mode_ = false;
  int notification_history_size_ = 100;       // Clamped [10, 1000]
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NOTIFICATION_SERVICE_H_
