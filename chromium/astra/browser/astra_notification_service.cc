// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_notification_service.h"

#include <algorithm>

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Clamp ranges for numeric settings.
constexpr int kMinTimeoutSeconds = 1;
constexpr int kMaxTimeoutSeconds = 60;
constexpr int kMinVisibleNotifications = 1;
constexpr int kMaxVisibleNotifications = 20;
constexpr int kMinHistorySize = 10;
constexpr int kMaxHistorySize = 1000;
constexpr int kMinPriority = 0;
constexpr int kMaxPriority = 3;

// Valid notification positions.
const char* const kValidPositions[] = {
    "top_right", "top_left", "bottom_right", "bottom_left",
};

// Valid notification styles.
const char* const kValidStyles[] = {
    "default", "compact", "minimal",
};

// Clamp helper.
int Clamp(int value, int min_val, int max_val) {
  return std::max(min_val, std::min(max_val, value));
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraNotificationService
// ---------------------------------------------------------------------------

AstraNotificationService::AstraNotificationService(Profile* profile)
    : profile_(profile) {
  LoadFromPrefs();
}

AstraNotificationService::~AstraNotificationService() = default;

void AstraNotificationService::Shutdown() {
  // KeyedService shutdown: clear all observer references before the profile
  // goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraNotificationService::AddObserver(
    AstraNotificationObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraNotificationService::RemoveObserver(
    AstraNotificationObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Notification operations -------------------------------------------------

bool AstraNotificationService::ShowNotification(
    const AstraNotification& notification) {
  if (!notifications_enabled_) {
    return false;
  }

  // Validate notification.
  if (notification.id.empty()) {
    return false;
  }

  // Check if should show based on type and settings.
  if (!ShouldShowNotification(notification.type)) {
    // Even if not shown (e.g. DND), still record in history and track unread.
    AstraNotification hist_notification = notification;
    if (hist_notification.created_time.is_null()) {
      hist_notification.created_time = base::Time::Now();
    }
    hist_notification.priority =
        Clamp(hist_notification.priority, kMinPriority, kMaxPriority);
    AddToHistory(hist_notification);
    if (!hist_notification.is_read) {
      UpdateUnreadCount();
    }
    return false;
  }

  AstraNotification notif = notification;
  if (notif.created_time.is_null()) {
    notif.created_time = base::Time::Now();
  }
  notif.priority = Clamp(notif.priority, kMinPriority, kMaxPriority);

  // Check if notification with same ID exists (update case).
  auto it = FindNotification(notif.id);
  if (it != active_notifications_.end()) {
    bool was_read = it->is_read;
    *it = notif;
    // If it was read and now is unread, or vice versa, update count.
    if (was_read != notif.is_read) {
      UpdateUnreadCount();
    }
  } else {
    active_notifications_.push_back(notif);
    if (!notif.is_read) {
      UpdateUnreadCount();
    }
  }

  AddToHistory(notif);

  for (auto& observer : observers_) {
    observer.OnNotificationShown(notif);
  }

  return true;
}

void AstraNotificationService::CloseNotification(const std::string& id) {
  auto it = FindNotification(id);
  if (it == active_notifications_.end()) {
    return;
  }

  bool was_unread = !it->is_read;
  active_notifications_.erase(it);

  if (was_unread) {
    UpdateUnreadCount();
  }

  for (auto& observer : observers_) {
    observer.OnNotificationClosed(id);
  }
}

const AstraNotification* AstraNotificationService::GetNotification(
    const std::string& id) const {
  auto it = FindNotification(id);
  if (it == active_notifications_.end()) {
    return nullptr;
  }
  return &(*it);
}

std::vector<AstraNotification>
AstraNotificationService::GetAllNotifications() const {
  return active_notifications_;
}

int AstraNotificationService::GetUnreadCount() const {
  return unread_count_;
}

int AstraNotificationService::GetNotificationCount() const {
  return static_cast<int>(active_notifications_.size());
}

std::vector<AstraNotification>
AstraNotificationService::GetNotificationsByType(
    AstraNotificationType type) const {
  std::vector<AstraNotification> result;
  for (const auto& notif : active_notifications_) {
    if (notif.type == type) {
      result.push_back(notif);
    }
  }
  return result;
}

std::vector<AstraNotification>
AstraNotificationService::GetNotificationsBySource(
    const std::string& source) const {
  std::vector<AstraNotification> result;
  for (const auto& notif : active_notifications_) {
    if (notif.source == source) {
      result.push_back(notif);
    }
  }
  return result;
}

void AstraNotificationService::MarkAsRead(const std::string& id) {
  auto it = FindNotification(id);
  if (it == active_notifications_.end() || it->is_read) {
    return;
  }

  it->is_read = true;
  UpdateUnreadCount();

  for (auto& observer : observers_) {
    observer.OnNotificationRead(id);
  }
}

void AstraNotificationService::MarkAllAsRead() {
  // Collect IDs of notifications that will be marked as read.
  std::vector<std::string> ids_to_read;
  for (auto& notif : active_notifications_) {
    if (!notif.is_read) {
      notif.is_read = true;
      ids_to_read.push_back(notif.id);
    }
  }

  if (ids_to_read.empty()) {
    return;
  }

  UpdateUnreadCount();

  // Notify observers for each newly-read notification.
  for (const auto& id : ids_to_read) {
    for (auto& observer : observers_) {
      observer.OnNotificationRead(id);
    }
  }
}

void AstraNotificationService::ClearAllNotifications() {
  active_notifications_.clear();
  UpdateUnreadCount();

  for (auto& observer : observers_) {
    observer.OnAllNotificationsCleared();
  }
}

void AstraNotificationService::ClearNotificationsByType(
    AstraNotificationType type) {
  auto new_end = std::remove_if(
      active_notifications_.begin(), active_notifications_.end(),
      [type](const AstraNotification& n) { return n.type == type; });

  if (new_end == active_notifications_.end()) {
    return;
  }

  // Collect IDs before erasing for observer notification.
  std::vector<std::string> cleared_ids;
  for (auto it = new_end; it != active_notifications_.end(); ++it) {
    cleared_ids.push_back(it->id);
  }

  active_notifications_.erase(new_end, active_notifications_.end());
  UpdateUnreadCount();

  for (const auto& id : cleared_ids) {
    for (auto& observer : observers_) {
      observer.OnNotificationClosed(id);
    }
  }
}

void AstraNotificationService::ClearNotificationsBySource(
    const std::string& source) {
  auto new_end = std::remove_if(
      active_notifications_.begin(), active_notifications_.end(),
      [&source](const AstraNotification& n) { return n.source == source; });

  if (new_end == active_notifications_.end()) {
    return;
  }

  std::vector<std::string> cleared_ids;
  for (auto it = new_end; it != active_notifications_.end(); ++it) {
    cleared_ids.push_back(it->id);
  }

  active_notifications_.erase(new_end, active_notifications_.end());
  UpdateUnreadCount();

  for (const auto& id : cleared_ids) {
    for (auto& observer : observers_) {
      observer.OnNotificationClosed(id);
    }
  }
}

std::vector<AstraNotification>
AstraNotificationService::GetNotificationHistory(int max_count) const {
  if (max_count <= 0) {
    return std::vector<AstraNotification>();
  }

  int count = std::min(max_count, static_cast<int>(history_.size()));
  return std::vector<AstraNotification>(history_.begin(),
                                         history_.begin() + count);
}

// -- Do not disturb ----------------------------------------------------------

bool AstraNotificationService::DoNotDisturb() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return false;
  }
  return profile_->GetPrefs()->GetBoolean(prefs::kPrefNotificationDoNotDisturb);
}

void AstraNotificationService::SetDoNotDisturb(bool enabled) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }
  if (DoNotDisturb() == enabled) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(prefs::kPrefNotificationDoNotDisturb,
                                   enabled);

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

// -- Presentation settings ---------------------------------------------------

void AstraNotificationService::set_notifications_enabled(bool enabled) {
  if (notifications_enabled_ == enabled) {
    return;
  }
  notifications_enabled_ = enabled;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_show_notification_previews(bool show) {
  if (show_notification_previews_ == show) {
    return;
  }
  show_notification_previews_ = show;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_notification_sound_enabled(bool enabled) {
  if (notification_sound_enabled_ == enabled) {
    return;
  }
  notification_sound_enabled_ = enabled;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_notification_timeout_seconds(int seconds) {
  int clamped = Clamp(seconds, kMinTimeoutSeconds, kMaxTimeoutSeconds);
  if (notification_timeout_seconds_ == clamped) {
    return;
  }
  notification_timeout_seconds_ = clamped;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_max_visible_notifications(int max) {
  int clamped = Clamp(max, kMinVisibleNotifications, kMaxVisibleNotifications);
  if (max_visible_notifications_ == clamped) {
    return;
  }
  max_visible_notifications_ = clamped;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_notification_position(
    const std::string& position) {
  std::string validated = ValidatePosition(position);
  if (notification_position_ == validated) {
    return;
  }
  notification_position_ = validated;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_show_notification_icon(bool show) {
  if (show_notification_icon_ == show) {
    return;
  }
  show_notification_icon_ = show;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_show_notification_timestamp(bool show) {
  if (show_notification_timestamp_ == show) {
    return;
  }
  show_notification_timestamp_ = show;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_show_close_button(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_notification_style(
    const std::string& style) {
  std::string validated = ValidateStyle(style);
  if (notification_style_ == validated) {
    return;
  }
  notification_style_ = validated;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_stack_notifications(bool stack) {
  if (stack_notifications_ == stack) {
    return;
  }
  stack_notifications_ = stack;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_quiet_mode(bool enabled) {
  if (quiet_mode_ == enabled) {
    return;
  }
  quiet_mode_ = enabled;
  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

void AstraNotificationService::set_notification_history_size(int size) {
  int clamped = Clamp(size, kMinHistorySize, kMaxHistorySize);
  if (notification_history_size_ == clamped) {
    return;
  }
  notification_history_size_ = clamped;

  // Trim history if it exceeds new size.
  if (static_cast<int>(history_.size()) > notification_history_size_) {
    history_.resize(notification_history_size_);
  }

  SaveSettingsToPrefs();

  for (auto& observer : observers_) {
    observer.OnNotificationSettingsChanged();
  }
}

// -- Utility methods ---------------------------------------------------------

// static
std::string AstraNotificationService::FormatNotificationTime(base::Time time) {
  if (time.is_null()) {
    return std::string();
  }

  base::TimeDelta delta = base::Time::Now() - time;

  if (delta < base::Seconds(60)) {
    return "Just now";
  }
  if (delta < base::Minutes(2)) {
    return "1 min ago";
  }
  if (delta < base::Minutes(60)) {
    return base::StringPrintf("%d min ago",
                              static_cast<int>(delta.InMinutes()));
  }
  if (delta < base::Hours(2)) {
    return "1 hour ago";
  }
  if (delta < base::Hours(24)) {
    return base::StringPrintf("%d hours ago",
                              static_cast<int>(delta.InHours()));
  }
  if (delta < base::Days(2)) {
    return "Yesterday";
  }
  if (delta < base::Days(7)) {
    return base::StringPrintf("%d days ago",
                              static_cast<int>(delta.InDays()));
  }

  // Older than a week: show date.
  // Use simple format: MMM d, YYYY
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  static const char* months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  int month = std::max(0, std::min(11, exploded.month - 1));
  return base::StringPrintf("%s %d, %d", months[month], exploded.day_of_month,
                            exploded.year);
}

// static
std::string AstraNotificationService::GetNotificationTypeLabel(
    AstraNotificationType type) {
  switch (type) {
    case AstraNotificationType::kInfo:
      return "Info";
    case AstraNotificationType::kWarning:
      return "Warning";
    case AstraNotificationType::kError:
      return "Error";
    case AstraNotificationType::kSuccess:
      return "Success";
    case AstraNotificationType::kDownload:
      return "Download";
    case AstraNotificationType::kExtension:
      return "Extension";
    case AstraNotificationType::kSystem:
      return "System";
  }
  return "Info";
}

// static
int AstraNotificationService::GetDefaultPriority(AstraNotificationType type) {
  switch (type) {
    case AstraNotificationType::kInfo:
      return 0;
    case AstraNotificationType::kWarning:
      return 2;
    case AstraNotificationType::kError:
      return 3;
    case AstraNotificationType::kSuccess:
      return 1;
    case AstraNotificationType::kDownload:
      return 1;
    case AstraNotificationType::kExtension:
      return 0;
    case AstraNotificationType::kSystem:
      return 2;
  }
  return 0;
}

bool AstraNotificationService::ShouldShowNotification(
    AstraNotificationType type) const {
  if (!notifications_enabled_) {
    return false;
  }

  // DND blocks popup notifications (but they're still recorded in history).
  if (DoNotDisturb()) {
    return false;
  }

  // Quiet mode suppresses popup notifications (no sound, no popups).
  if (quiet_mode_) {
    return false;
  }

  // All notification types are shown by default when enabled.
  // TODO(astra): Add per-type enablement settings if needed.
  return true;
}

// static
std::string AstraNotificationService::TruncateMessage(
    const std::string& message,
    int max_length) {
  if (max_length <= 0) {
    return std::string();
  }
  if (static_cast<int>(message.size()) <= max_length) {
    return message;
  }
  if (max_length <= 3) {
    return message.substr(0, max_length);
  }
  return message.substr(0, max_length - 3) + "...";
}

// -- Bulk operations ---------------------------------------------------------

int AstraNotificationService::ShowNotifications(
    const std::vector<AstraNotification>& notifications) {
  int shown = 0;
  for (const auto& notif : notifications) {
    if (ShowNotification(notif)) {
      shown++;
    }
  }
  return shown;
}

void AstraNotificationService::CloseNotifications(
    const std::vector<std::string>& ids) {
  for (const auto& id : ids) {
    CloseNotification(id);
  }
}

void AstraNotificationService::MarkAsRead(
    const std::vector<std::string>& ids) {
  for (const auto& id : ids) {
    MarkAsRead(id);
  }
}

// -- Private helpers ---------------------------------------------------------

void AstraNotificationService::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  notifications_enabled_ =
      prefs->GetBoolean(prefs::kPrefNotificationsEnabled);
  show_notification_previews_ =
      prefs->GetBoolean(prefs::kPrefNotificationShowPreviews);
  notification_sound_enabled_ =
      prefs->GetBoolean(prefs::kPrefNotificationSoundEnabled);
  notification_timeout_seconds_ =
      Clamp(prefs->GetInteger(prefs::kPrefNotificationTimeoutSeconds),
            kMinTimeoutSeconds, kMaxTimeoutSeconds);
  max_visible_notifications_ =
      Clamp(prefs->GetInteger(prefs::kPrefNotificationMaxVisible),
            kMinVisibleNotifications, kMaxVisibleNotifications);
  notification_position_ =
      ValidatePosition(prefs->GetString(prefs::kPrefNotificationPosition));
  show_notification_icon_ =
      prefs->GetBoolean(prefs::kPrefNotificationShowIcon);
  show_notification_timestamp_ =
      prefs->GetBoolean(prefs::kPrefNotificationShowTimestamp);
  show_close_button_ =
      prefs->GetBoolean(prefs::kPrefNotificationShowCloseButton);
  notification_style_ =
      ValidateStyle(prefs->GetString(prefs::kPrefNotificationStyle));
  stack_notifications_ =
      prefs->GetBoolean(prefs::kPrefNotificationStackNotifications);
  quiet_mode_ = prefs->GetBoolean(prefs::kPrefNotificationQuietMode);
  notification_history_size_ =
      Clamp(prefs->GetInteger(prefs::kPrefNotificationHistorySize),
            kMinHistorySize, kMaxHistorySize);
}

void AstraNotificationService::SaveSettingsToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetBoolean(prefs::kPrefNotificationsEnabled, notifications_enabled_);
  prefs->SetBoolean(prefs::kPrefNotificationShowPreviews,
                    show_notification_previews_);
  prefs->SetBoolean(prefs::kPrefNotificationSoundEnabled,
                    notification_sound_enabled_);
  prefs->SetInteger(prefs::kPrefNotificationTimeoutSeconds,
                    notification_timeout_seconds_);
  prefs->SetInteger(prefs::kPrefNotificationMaxVisible,
                    max_visible_notifications_);
  prefs->SetString(prefs::kPrefNotificationPosition, notification_position_);
  prefs->SetBoolean(prefs::kPrefNotificationShowIcon,
                    show_notification_icon_);
  prefs->SetBoolean(prefs::kPrefNotificationShowTimestamp,
                    show_notification_timestamp_);
  prefs->SetBoolean(prefs::kPrefNotificationShowCloseButton,
                    show_close_button_);
  prefs->SetString(prefs::kPrefNotificationStyle, notification_style_);
  prefs->SetBoolean(prefs::kPrefNotificationStackNotifications,
                    stack_notifications_);
  prefs->SetBoolean(prefs::kPrefNotificationQuietMode, quiet_mode_);
  prefs->SetInteger(prefs::kPrefNotificationHistorySize,
                    notification_history_size_);
}

void AstraNotificationService::AddToHistory(
    const AstraNotification& notification) {
  // Insert at beginning (most recent first).
  history_.insert(history_.begin(), notification);

  // Trim history to size limit.
  if (static_cast<int>(history_.size()) > notification_history_size_) {
    history_.resize(notification_history_size_);
  }
}

void AstraNotificationService::UpdateUnreadCount() {
  int count = 0;
  for (const auto& notif : active_notifications_) {
    if (!notif.is_read) {
      count++;
    }
  }
  // Also count unread items in history? No — unread count is for active only.
  // Actually, let's think about this. Unread count typically reflects all
  // unread notifications, including ones in the notification center.
  // For simplicity, active_notifications_ represents the notification center.

  if (unread_count_ != count) {
    unread_count_ = count;
    for (auto& observer : observers_) {
      observer.OnUnreadCountChanged(unread_count_);
    }
  }
}

// static
std::string AstraNotificationService::ValidatePosition(
    const std::string& position) {
  for (const char* valid : kValidPositions) {
    if (position == valid) {
      return position;
    }
  }
  return "top_right";  // Default.
}

// static
std::string AstraNotificationService::ValidateStyle(
    const std::string& style) {
  for (const char* valid : kValidStyles) {
    if (style == valid) {
      return style;
    }
  }
  return "default";  // Default.
}

std::vector<AstraNotification>::iterator
AstraNotificationService::FindNotification(const std::string& id) {
  return std::find_if(active_notifications_.begin(),
                      active_notifications_.end(),
                      [&id](const AstraNotification& n) { return n.id == id; });
}

std::vector<AstraNotification>::const_iterator
AstraNotificationService::FindNotification(const std::string& id) const {
  return std::find_if(active_notifications_.begin(),
                      active_notifications_.end(),
                      [&id](const AstraNotification& n) { return n.id == id; });
}

}  // namespace astra
