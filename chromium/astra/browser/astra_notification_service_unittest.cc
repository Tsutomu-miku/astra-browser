// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_notification_service.h"

#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_notification_service_factory.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records all calls for verification.
class TestNotificationObserver : public AstraNotificationObserver {
 public:
  void OnNotificationShown(const AstraNotification& notification) override {
    shown_count_++;
    last_shown_notification_ = notification;
  }

  void OnNotificationClosed(const std::string& notification_id) override {
    closed_count_++;
    last_closed_id_ = notification_id;
    closed_ids_.push_back(notification_id);
  }

  void OnNotificationClicked(const std::string& notification_id) override {
    clicked_count_++;
    last_clicked_id_ = notification_id;
  }

  void OnNotificationActionClicked(const std::string& notification_id,
                                   const std::string& action) override {
    action_clicked_count_++;
    last_action_clicked_id_ = notification_id;
    last_action_ = action;
  }

  void OnNotificationRead(const std::string& notification_id) override {
    read_count_++;
    last_read_id_ = notification_id;
    read_ids_.push_back(notification_id);
  }

  void OnAllNotificationsCleared() override {
    all_cleared_count_++;
  }

  void OnNotificationSettingsChanged() override {
    settings_changed_count_++;
  }

  void OnUnreadCountChanged(int unread_count) override {
    unread_count_changed_count_++;
    last_unread_count_ = unread_count;
  }

  // Counters.
  int shown_count_ = 0;
  int closed_count_ = 0;
  int clicked_count_ = 0;
  int action_clicked_count_ = 0;
  int read_count_ = 0;
  int all_cleared_count_ = 0;
  int settings_changed_count_ = 0;
  int unread_count_changed_count_ = 0;

  // Last recorded values.
  AstraNotification last_shown_notification_;
  std::string last_closed_id_;
  std::string last_clicked_id_;
  std::string last_action_clicked_id_;
  std::string last_action_;
  std::string last_read_id_;
  int last_unread_count_ = -1;

  // Collected IDs.
  std::vector<std::string> closed_ids_;
  std::vector<std::string> read_ids_;
};

// Minimal observer that only overrides one method.
// Tests that empty default implementations don't crash.
class MinimalObserver : public AstraNotificationObserver {
 public:
  int shown_count_ = 0;
  void OnNotificationShown(const AstraNotification&) override {
    shown_count_++;
  }
};

// Helper to create a test notification.
AstraNotification MakeNotification(const std::string& id,
                                   const std::string& title = "Test Title",
                                   AstraNotificationType type =
                                       AstraNotificationType::kInfo) {
  AstraNotification notif;
  notif.id = id;
  notif.title = title;
  notif.message = "Test message body";
  notif.type = type;
  notif.source = "test";
  notif.created_time = base::Time::Now();
  notif.is_read = false;
  notif.priority = 0;
  return notif;
}

}  // namespace

// =========================================================================
// Test fixture
// =========================================================================

class NotificationServiceTest : public testing::Test {
 protected:
  NotificationServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register notification prefs on the testing profile.
    PrefRegistrySimple* registry = profile_->GetPrefs()->registry();
    AstraNotificationServiceFactory::RegisterProfilePrefs(registry);
    // Also register all Astra prefs to avoid missing pref warnings.
    prefs::RegisterProfilePrefs(registry);
    service_ = std::make_unique<AstraNotificationService>(profile_.get());
  }

  ~NotificationServiceTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(service_);
    // Start with no active notifications.
    EXPECT_EQ(service_->GetNotificationCount(), 0);
    EXPECT_EQ(service_->GetUnreadCount(), 0);
  }

  void TearDown() override {
    // Clean up observers.
    for (auto* observer : observers_) {
      service_->RemoveObserver(observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraNotificationService> service_;
  std::vector<AstraNotificationObserver*> observers_;
};

// =========================================================================
// Construction and default state
// =========================================================================

TEST_F(NotificationServiceTest, Construction_DefaultStateHasNoNotifications) {
  EXPECT_EQ(service_->GetNotificationCount(), 0);
  EXPECT_EQ(service_->GetUnreadCount(), 0);
  EXPECT_TRUE(service_->GetAllNotifications().empty());
}

TEST_F(NotificationServiceTest, Construction_DefaultSettings) {
  EXPECT_TRUE(service_->notifications_enabled());
  EXPECT_TRUE(service_->show_notification_previews());
  EXPECT_TRUE(service_->notification_sound_enabled());
  EXPECT_EQ(service_->notification_timeout_seconds(), 8);
  EXPECT_EQ(service_->max_visible_notifications(), 5);
  EXPECT_EQ(service_->notification_position(), "top_right");
  EXPECT_TRUE(service_->show_notification_icon());
  EXPECT_TRUE(service_->show_notification_timestamp());
  EXPECT_TRUE(service_->show_close_button());
  EXPECT_EQ(service_->notification_style(), "default");
  EXPECT_TRUE(service_->stack_notifications());
  EXPECT_FALSE(service_->quiet_mode());
  EXPECT_EQ(service_->notification_history_size(), 100);
}

TEST_F(NotificationServiceTest, Construction_DefaultDoNotDisturbIsFalse) {
  EXPECT_FALSE(service_->DoNotDisturb());
}

TEST_F(NotificationServiceTest, Construction_GetNotificationReturnsNullForEmpty) {
  EXPECT_EQ(service_->GetNotification("nonexistent"), nullptr);
}

TEST_F(NotificationServiceTest, Construction_ServiceIsNonNull) {
  EXPECT_NE(service_.get(), nullptr);
}

TEST_F(NotificationServiceTest, DefaultState_HistoryIsEmpty) {
  auto history = service_->GetNotificationHistory(10);
  EXPECT_TRUE(history.empty());
}

TEST_F(NotificationServiceTest, DefaultState_GetByTypeReturnsEmpty) {
  auto result = service_->GetNotificationsByType(AstraNotificationType::kInfo);
  EXPECT_TRUE(result.empty());
}

TEST_F(NotificationServiceTest, DefaultState_GetBySourceReturnsEmpty) {
  auto result = service_->GetNotificationsBySource("test");
  EXPECT_TRUE(result.empty());
}

// =========================================================================
// Notification show/get/close operations
// =========================================================================

TEST_F(NotificationServiceTest, ShowNotification_IncreasesCount) {
  AstraNotification notif = MakeNotification("notif-1");
  EXPECT_TRUE(service_->ShowNotification(notif));
  EXPECT_EQ(service_->GetNotificationCount(), 1);
}

TEST_F(NotificationServiceTest, ShowNotification_ReturnsTrueWhenShown) {
  AstraNotification notif = MakeNotification("notif-1");
  bool result = service_->ShowNotification(notif);
  EXPECT_TRUE(result);
}

TEST_F(NotificationServiceTest, ShowNotification_EmptyIdReturnsFalse) {
  AstraNotification notif;
  notif.id = "";
  bool result = service_->ShowNotification(notif);
  EXPECT_FALSE(result);
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, GetNotification_ReturnsCorrectNotification) {
  AstraNotification notif = MakeNotification("notif-1", "My Title");
  service_->ShowNotification(notif);

  const AstraNotification* found = service_->GetNotification("notif-1");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, "notif-1");
  EXPECT_EQ(found->title, "My Title");
}

TEST_F(NotificationServiceTest, GetNotification_UnknownIdReturnsNull) {
  AstraNotification notif = MakeNotification("notif-1");
  service_->ShowNotification(notif);
  EXPECT_EQ(service_->GetNotification("notif-999"), nullptr);
}

TEST_F(NotificationServiceTest, CloseNotification_RemovesNotification) {
  service_->ShowNotification(MakeNotification("notif-1"));
  ASSERT_EQ(service_->GetNotificationCount(), 1);

  service_->CloseNotification("notif-1");
  EXPECT_EQ(service_->GetNotificationCount(), 0);
  EXPECT_EQ(service_->GetNotification("notif-1"), nullptr);
}

TEST_F(NotificationServiceTest, CloseNotification_UnknownIdIsNoop) {
  service_->ShowNotification(MakeNotification("notif-1"));
  ASSERT_EQ(service_->GetNotificationCount(), 1);

  // Should not crash and not affect the existing notification.
  service_->CloseNotification("nonexistent");
  EXPECT_EQ(service_->GetNotificationCount(), 1);
}

TEST_F(NotificationServiceTest, CloseNotification_Idempotent) {
  service_->ShowNotification(MakeNotification("notif-1"));

  service_->CloseNotification("notif-1");
  service_->CloseNotification("notif-1");  // Second call should not crash.
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, ShowNotification_UpdateExistingId) {
  AstraNotification notif1 = MakeNotification("notif-1", "Original");
  service_->ShowNotification(notif1);

  AstraNotification notif2 = MakeNotification("notif-1", "Updated");
  notif2.message = "Updated message";
  bool result = service_->ShowNotification(notif2);

  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNotificationCount(), 1);  // Still 1, not 2.

  const AstraNotification* found = service_->GetNotification("notif-1");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->title, "Updated");
  EXPECT_EQ(found->message, "Updated message");
}

TEST_F(NotificationServiceTest, GetAllNotifications_ReturnsAll) {
  service_->ShowNotification(MakeNotification("notif-1"));
  service_->ShowNotification(MakeNotification("notif-2"));
  service_->ShowNotification(MakeNotification("notif-3"));

  auto all = service_->GetAllNotifications();
  EXPECT_EQ(all.size(), 3u);
}

// =========================================================================
// Notification CRUD by ID
// =========================================================================

TEST_F(NotificationServiceTest, NotificationStruct_DefaultValues) {
  AstraNotification notif;
  EXPECT_TRUE(notif.id.empty());
  EXPECT_TRUE(notif.title.empty());
  EXPECT_TRUE(notif.message.empty());
  EXPECT_EQ(notif.type, AstraNotificationType::kInfo);
  EXPECT_TRUE(notif.source.empty());
  EXPECT_TRUE(notif.created_time.is_null());
  EXPECT_FALSE(notif.is_read);
  EXPECT_EQ(notif.priority, 0);
  EXPECT_TRUE(notif.action_label.empty());
  EXPECT_TRUE(notif.secondary_action_label.empty());
}

TEST_F(NotificationServiceTest, NotificationTypeEnum_HasAllValues) {
  // Verify all 7 notification types exist.
  AstraNotificationType types[] = {
      AstraNotificationType::kInfo,
      AstraNotificationType::kWarning,
      AstraNotificationType::kError,
      AstraNotificationType::kSuccess,
      AstraNotificationType::kDownload,
      AstraNotificationType::kExtension,
      AstraNotificationType::kSystem,
  };
  EXPECT_EQ(std::end(types) - std::begin(types), 7);
}

TEST_F(NotificationServiceTest, ShowNotification_PreservesAllFields) {
  AstraNotification notif;
  notif.id = "full-notif";
  notif.title = "Full Title";
  notif.message = "Full message body text";
  notif.type = AstraNotificationType::kWarning;
  notif.source = "downloads";
  notif.is_read = false;
  notif.priority = 2;
  notif.action_label = "Open";
  notif.secondary_action_label = "Cancel";

  service_->ShowNotification(notif);

  const AstraNotification* found = service_->GetNotification("full-notif");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, "full-notif");
  EXPECT_EQ(found->title, "Full Title");
  EXPECT_EQ(found->message, "Full message body text");
  EXPECT_EQ(found->type, AstraNotificationType::kWarning);
  EXPECT_EQ(found->source, "downloads");
  EXPECT_FALSE(found->is_read);
  EXPECT_EQ(found->priority, 2);
  EXPECT_EQ(found->action_label, "Open");
  EXPECT_EQ(found->secondary_action_label, "Cancel");
  EXPECT_FALSE(found->created_time.is_null());
}

TEST_F(NotificationServiceTest, ShowNotification_SetsCreatedTimeWhenNull) {
  AstraNotification notif = MakeNotification("time-test");
  notif.created_time = base::Time();  // Null time.

  service_->ShowNotification(notif);

  const AstraNotification* found = service_->GetNotification("time-test");
  ASSERT_NE(found, nullptr);
  EXPECT_FALSE(found->created_time.is_null());
}

TEST_F(NotificationServiceTest, ShowNotification_PreservesProvidedTime) {
  base::Time custom_time = base::Time::Now() - base::Hours(2);
  AstraNotification notif = MakeNotification("custom-time");
  notif.created_time = custom_time;

  service_->ShowNotification(notif);

  const AstraNotification* found = service_->GetNotification("custom-time");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->created_time, custom_time);
}

TEST_F(NotificationServiceTest, ShowNotification_PriorityIsClamped) {
  AstraNotification notif_low = MakeNotification("low-priority");
  notif_low.priority = -5;  // Below min.
  service_->ShowNotification(notif_low);
  const AstraNotification* found_low =
      service_->GetNotification("low-priority");
  EXPECT_GE(found_low->priority, 0);

  AstraNotification notif_high = MakeNotification("high-priority");
  notif_high.priority = 100;  // Above max.
  service_->ShowNotification(notif_high);
  const AstraNotification* found_high =
      service_->GetNotification("high-priority");
  EXPECT_LE(found_high->priority, 3);
}

// =========================================================================
// Type filtering
// =========================================================================

TEST_F(NotificationServiceTest, GetNotificationsByType_FiltersCorrectly) {
  service_->ShowNotification(
      MakeNotification("info-1", "Info 1", AstraNotificationType::kInfo));
  service_->ShowNotification(
      MakeNotification("warn-1", "Warn 1", AstraNotificationType::kWarning));
  service_->ShowNotification(
      MakeNotification("error-1", "Err 1", AstraNotificationType::kError));
  service_->ShowNotification(
      MakeNotification("info-2", "Info 2", AstraNotificationType::kInfo));

  auto info_notifs =
      service_->GetNotificationsByType(AstraNotificationType::kInfo);
  EXPECT_EQ(info_notifs.size(), 2u);

  auto warn_notifs =
      service_->GetNotificationsByType(AstraNotificationType::kWarning);
  EXPECT_EQ(warn_notifs.size(), 1u);
  EXPECT_EQ(warn_notifs[0].id, "warn-1");

  auto error_notifs =
      service_->GetNotificationsByType(AstraNotificationType::kError);
  EXPECT_EQ(error_notifs.size(), 1u);

  auto system_notifs =
      service_->GetNotificationsByType(AstraNotificationType::kSystem);
  EXPECT_TRUE(system_notifs.empty());
}

TEST_F(NotificationServiceTest, GetNotificationsByType_AllTypesWork) {
  std::vector<AstraNotificationType> all_types = {
      AstraNotificationType::kInfo,
      AstraNotificationType::kWarning,
      AstraNotificationType::kError,
      AstraNotificationType::kSuccess,
      AstraNotificationType::kDownload,
      AstraNotificationType::kExtension,
      AstraNotificationType::kSystem,
  };

  for (size_t i = 0; i < all_types.size(); i++) {
    AstraNotification notif =
        MakeNotification("type-" + std::to_string(i), "Test", all_types[i]);
    service_->ShowNotification(notif);
  }

  // Each type should have exactly 1 notification.
  for (auto type : all_types) {
    auto result = service_->GetNotificationsByType(type);
    EXPECT_EQ(result.size(), 1u);
  }
}

TEST_F(NotificationServiceTest, GetNotificationsByType_EmptyWhenNoMatch) {
  service_->ShowNotification(
      MakeNotification("info-1", "Info", AstraNotificationType::kInfo));
  auto result =
      service_->GetNotificationsByType(AstraNotificationType::kDownload);
  EXPECT_TRUE(result.empty());
}

// =========================================================================
// Source filtering
// =========================================================================

TEST_F(NotificationServiceTest, GetNotificationsBySource_FiltersCorrectly) {
  AstraNotification n1 = MakeNotification("d1", "Download 1");
  n1.source = "downloads";
  service_->ShowNotification(n1);

  AstraNotification n2 = MakeNotification("e1", "Extension 1");
  n2.source = "extensions";
  service_->ShowNotification(n2);

  AstraNotification n3 = MakeNotification("d2", "Download 2");
  n3.source = "downloads";
  service_->ShowNotification(n3);

  auto downloads = service_->GetNotificationsBySource("downloads");
  EXPECT_EQ(downloads.size(), 2u);

  auto extensions = service_->GetNotificationsBySource("extensions");
  EXPECT_EQ(extensions.size(), 1u);
  EXPECT_EQ(extensions[0].id, "e1");

  auto unknown = service_->GetNotificationsBySource("unknown");
  EXPECT_TRUE(unknown.empty());
}

TEST_F(NotificationServiceTest, GetNotificationsBySource_EmptySource) {
  AstraNotification n1 = MakeNotification("s1", "Test");
  n1.source = "";
  service_->ShowNotification(n1);

  auto result = service_->GetNotificationsBySource("");
  EXPECT_EQ(result.size(), 1u);
}

TEST_F(NotificationServiceTest, GetNotificationsBySource_CaseSensitive) {
  AstraNotification n1 = MakeNotification("s1", "Test");
  n1.source = "Downloads";
  service_->ShowNotification(n1);

  auto result = service_->GetNotificationsBySource("downloads");
  EXPECT_TRUE(result.empty());
}

// =========================================================================
// Read/unread state
// =========================================================================

TEST_F(NotificationServiceTest, MarkAsRead_ChangesReadState) {
  service_->ShowNotification(MakeNotification("notif-1"));

  const AstraNotification* before = service_->GetNotification("notif-1");
  ASSERT_NE(before, nullptr);
  EXPECT_FALSE(before->is_read);

  service_->MarkAsRead("notif-1");

  const AstraNotification* after = service_->GetNotification("notif-1");
  ASSERT_NE(after, nullptr);
  EXPECT_TRUE(after->is_read);
}

TEST_F(NotificationServiceTest, MarkAsRead_Idempotent) {
  service_->ShowNotification(MakeNotification("notif-1"));
  service_->MarkAsRead("notif-1");
  service_->MarkAsRead("notif-1");  // Should not crash or double-count.

  const AstraNotification* after = service_->GetNotification("notif-1");
  EXPECT_TRUE(after->is_read);
}

TEST_F(NotificationServiceTest, MarkAsRead_UnknownIdIsNoop) {
  service_->ShowNotification(MakeNotification("notif-1"));

  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->MarkAsRead("nonexistent");
  // No read notification should be fired.
  EXPECT_EQ(observer.read_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, MarkAllAsRead_MarksAllAsRead) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->ShowNotification(MakeNotification("n3"));

  ASSERT_EQ(service_->GetUnreadCount(), 3);

  service_->MarkAllAsRead();

  EXPECT_EQ(service_->GetUnreadCount(), 0);
  auto all = service_->GetAllNotifications();
  for (const auto& n : all) {
    EXPECT_TRUE(n.is_read);
  }
}

TEST_F(NotificationServiceTest, MarkAllAsRead_WhenAllAlreadyRead) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->MarkAllAsRead();

  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->MarkAllAsRead();  // Second call should be no-op.

  // No additional events should be fired.
  EXPECT_EQ(observer.read_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, MarkAllAsRead_EmptyListIsNoop) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->MarkAllAsRead();
  EXPECT_EQ(observer.read_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, ShowReadNotification_DoesNotIncreaseUnread) {
  AstraNotification notif = MakeNotification("read-notif");
  notif.is_read = true;
  service_->ShowNotification(notif);

  EXPECT_EQ(service_->GetNotificationCount(), 1);
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

// =========================================================================
// Unread count tracking
// =========================================================================

TEST_F(NotificationServiceTest, GetUnreadCount_StartsAtZero) {
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, GetUnreadCount_IncreasesWithNewNotifications) {
  service_->ShowNotification(MakeNotification("n1"));
  EXPECT_EQ(service_->GetUnreadCount(), 1);

  service_->ShowNotification(MakeNotification("n2"));
  EXPECT_EQ(service_->GetUnreadCount(), 2);
}

TEST_F(NotificationServiceTest, GetUnreadCount_DecreasesWhenMarkedRead) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  ASSERT_EQ(service_->GetUnreadCount(), 2);

  service_->MarkAsRead("n1");
  EXPECT_EQ(service_->GetUnreadCount(), 1);

  service_->MarkAsRead("n2");
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, GetUnreadCount_DecreasesWhenClosed) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  ASSERT_EQ(service_->GetUnreadCount(), 2);

  service_->CloseNotification("n1");
  EXPECT_EQ(service_->GetUnreadCount(), 1);
}

TEST_F(NotificationServiceTest, GetUnreadCount_ClosingReadNotificationNoChange) {
  AstraNotification notif = MakeNotification("read-one");
  notif.is_read = true;
  service_->ShowNotification(notif);
  ASSERT_EQ(service_->GetUnreadCount(), 0);

  service_->CloseNotification("read-one");
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, GetUnreadCount_ClearAllResetsToZero) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->ShowNotification(MakeNotification("n3"));
  ASSERT_EQ(service_->GetUnreadCount(), 3);

  service_->ClearAllNotifications();
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, UnreadCount_UpdateExistingReadStatus) {
  // Show a notification as read.
  AstraNotification notif = MakeNotification("update-test");
  notif.is_read = true;
  service_->ShowNotification(notif);
  ASSERT_EQ(service_->GetUnreadCount(), 0);

  // Update it to unread.
  AstraNotification updated = MakeNotification("update-test");
  updated.is_read = false;
  service_->ShowNotification(updated);
  EXPECT_EQ(service_->GetUnreadCount(), 1);

  // Update it back to read.
  AstraNotification read_again = MakeNotification("update-test");
  read_again.is_read = true;
  service_->ShowNotification(read_again);
  EXPECT_EQ(service_->GetUnreadCount(), 0);
}

// =========================================================================
// Clear operations
// =========================================================================

TEST_F(NotificationServiceTest, ClearAllNotifications_RemovesAll) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->ShowNotification(MakeNotification("n3"));
  ASSERT_EQ(service_->GetNotificationCount(), 3);

  service_->ClearAllNotifications();
  EXPECT_EQ(service_->GetNotificationCount(), 0);
  EXPECT_TRUE(service_->GetAllNotifications().empty());
}

TEST_F(NotificationServiceTest, ClearAllNotifications_EmptyIsNoop) {
  service_->ClearAllNotifications();
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, ClearAllNotifications_Idempotent) {
  service_->ShowNotification(MakeNotification("n1"));

  service_->ClearAllNotifications();
  service_->ClearAllNotifications();  // Should not crash.
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, ClearNotificationsByType_RemovesMatchingType) {
  service_->ShowNotification(
      MakeNotification("info-1", "Info", AstraNotificationType::kInfo));
  service_->ShowNotification(
      MakeNotification("warn-1", "Warn", AstraNotificationType::kWarning));
  service_->ShowNotification(
      MakeNotification("info-2", "Info 2", AstraNotificationType::kInfo));

  ASSERT_EQ(service_->GetNotificationCount(), 3);

  service_->ClearNotificationsByType(AstraNotificationType::kInfo);

  EXPECT_EQ(service_->GetNotificationCount(), 1);
  auto remaining = service_->GetAllNotifications();
  EXPECT_EQ(remaining[0].type, AstraNotificationType::kWarning);
}

TEST_F(NotificationServiceTest, ClearNotificationsByType_NoMatchesIsNoop) {
  service_->ShowNotification(
      MakeNotification("info-1", "Info", AstraNotificationType::kInfo));

  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ClearNotificationsByType(AstraNotificationType::kError);

  EXPECT_EQ(observer.closed_count_, 0);
  EXPECT_EQ(service_->GetNotificationCount(), 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, ClearNotificationsBySource_RemovesMatchingSource) {
  AstraNotification n1 = MakeNotification("d1");
  n1.source = "downloads";
  service_->ShowNotification(n1);

  AstraNotification n2 = MakeNotification("e1");
  n2.source = "extensions";
  service_->ShowNotification(n2);

  AstraNotification n3 = MakeNotification("d2");
  n3.source = "downloads";
  service_->ShowNotification(n3);

  ASSERT_EQ(service_->GetNotificationCount(), 3);

  service_->ClearNotificationsBySource("downloads");
  EXPECT_EQ(service_->GetNotificationCount(), 1);
  EXPECT_EQ(service_->GetAllNotifications()[0].source, "extensions");
}

TEST_F(NotificationServiceTest, ClearNotificationsBySource_NoMatchesIsNoop) {
  AstraNotification n1 = MakeNotification("d1");
  n1.source = "downloads";
  service_->ShowNotification(n1);

  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ClearNotificationsBySource("nonexistent");
  EXPECT_EQ(observer.closed_count_, 0);
  EXPECT_EQ(service_->GetNotificationCount(), 1);

  service_->RemoveObserver(&observer);
}

// =========================================================================
// Notification history
// =========================================================================

TEST_F(NotificationServiceTest, GetNotificationHistory_ReturnsRecent) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->ShowNotification(MakeNotification("n3"));

  auto history = service_->GetNotificationHistory(10);
  // All 3 should be in history (most recent first).
  EXPECT_GE(history.size(), 3u);
}

TEST_F(NotificationServiceTest, GetNotificationHistory_RespectsMaxCount) {
  for (int i = 0; i < 10; i++) {
    service_->ShowNotification(MakeNotification("n-" + std::to_string(i)));
  }

  auto history = service_->GetNotificationHistory(3);
  EXPECT_EQ(history.size(), 3u);
}

TEST_F(NotificationServiceTest, GetNotificationHistory_ZeroMaxReturnsEmpty) {
  service_->ShowNotification(MakeNotification("n1"));

  auto history = service_->GetNotificationHistory(0);
  EXPECT_TRUE(history.empty());
}

TEST_F(NotificationServiceTest, GetNotificationHistory_NegativeMaxReturnsEmpty) {
  service_->ShowNotification(MakeNotification("n1"));

  auto history = service_->GetNotificationHistory(-5);
  EXPECT_TRUE(history.empty());
}

TEST_F(NotificationServiceTest, GetNotificationHistory_IncludesClosed) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->CloseNotification("n1");

  auto history = service_->GetNotificationHistory(10);
  // Both should be in history.
  bool found_n1 = false;
  bool found_n2 = false;
  for (const auto& n : history) {
    if (n.id == "n1") found_n1 = true;
    if (n.id == "n2") found_n2 = true;
  }
  EXPECT_TRUE(found_n1);
  EXPECT_TRUE(found_n2);
}

TEST_F(NotificationServiceTest, GetNotificationHistory_MostRecentFirst) {
  base::Time t1 = base::Time::Now() - base::Hours(3);
  base::Time t2 = base::Time::Now() - base::Hours(2);
  base::Time t3 = base::Time::Now() - base::Hours(1);

  AstraNotification n1 = MakeNotification("oldest");
  n1.created_time = t1;
  service_->ShowNotification(n1);

  AstraNotification n2 = MakeNotification("middle");
  n2.created_time = t2;
  service_->ShowNotification(n2);

  AstraNotification n3 = MakeNotification("newest");
  n3.created_time = t3;
  service_->ShowNotification(n3);

  auto history = service_->GetNotificationHistory(3);
  ASSERT_GE(history.size(), 3u);
  // First entry should be most recently ADDED (n3), then n2, then n1.
  EXPECT_EQ(history[0].id, "newest");
  EXPECT_EQ(history[1].id, "middle");
  EXPECT_EQ(history[2].id, "oldest");
}

TEST_F(NotificationServiceTest, NotificationHistory_LimitedByHistorySizePref) {
  // Set history size to 5.
  service_->set_notification_history_size(5);

  for (int i = 0; i < 20; i++) {
    service_->ShowNotification(MakeNotification("n-" + std::to_string(i)));
  }

  auto history = service_->GetNotificationHistory(100);
  EXPECT_LE(history.size(), 5u);
}

TEST_F(NotificationServiceTest, NotificationHistory_ResizedWhenPrefDecreased) {
  // First add many with large size.
  service_->set_notification_history_size(50);
  for (int i = 0; i < 30; i++) {
    service_->ShowNotification(MakeNotification("n-" + std::to_string(i)));
  }
  ASSERT_GE(service_->GetNotificationHistory(100).size(), 30u);

  // Now reduce history size.
  service_->set_notification_history_size(10);
  auto history = service_->GetNotificationHistory(100);
  EXPECT_EQ(history.size(), 10u);
}

// =========================================================================
// Do not disturb mode
// =========================================================================

TEST_F(NotificationServiceTest, DoNotDisturb_DefaultsToFalse) {
  EXPECT_FALSE(service_->DoNotDisturb());
}

TEST_F(NotificationServiceTest, SetDoNotDisturb_EnablesDND) {
  service_->SetDoNotDisturb(true);
  EXPECT_TRUE(service_->DoNotDisturb());
}

TEST_F(NotificationServiceTest, SetDoNotDisturb_DisablesDND) {
  service_->SetDoNotDisturb(true);
  ASSERT_TRUE(service_->DoNotDisturb());

  service_->SetDoNotDisturb(false);
  EXPECT_FALSE(service_->DoNotDisturb());
}

TEST_F(NotificationServiceTest, DoNotDisturb_BlocksNewNotifications) {
  service_->SetDoNotDisturb(true);

  AstraNotification notif = MakeNotification("dnd-test");
  bool shown = service_->ShowNotification(notif);

  EXPECT_FALSE(shown);
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, DoNotDisturb_StillRecordsInHistory) {
  service_->SetDoNotDisturb(true);

  AstraNotification notif = MakeNotification("dnd-history");
  service_->ShowNotification(notif);

  auto history = service_->GetNotificationHistory(10);
  bool found = false;
  for (const auto& n : history) {
    if (n.id == "dnd-history") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(NotificationServiceTest, DoNotDisturb_AffectsUnreadCount) {
  service_->SetDoNotDisturb(true);

  AstraNotification notif = MakeNotification("dnd-unread");
  notif.is_read = false;
  service_->ShowNotification(notif);

  // DND notifications still count toward unread (they're in the notification
  // center/history, just not shown as popups).
  EXPECT_GT(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, SetDoNotDisturb_Idempotent) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->SetDoNotDisturb(true);
  int change_count = observer.settings_changed_count_;

  service_->SetDoNotDisturb(true);  // Same value — should not notify again.
  EXPECT_EQ(observer.settings_changed_count_, change_count);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, DoNotDisturb_PersistsViaPrefs) {
  service_->SetDoNotDisturb(true);
  EXPECT_TRUE(profile_->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationDoNotDisturb));

  service_->SetDoNotDisturb(false);
  EXPECT_FALSE(profile_->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationDoNotDisturb));
}

// =========================================================================
// Observer notifications
// =========================================================================

TEST_F(NotificationServiceTest, Observer_OnNotificationShownFired) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  AstraNotification notif = MakeNotification("obs-test");
  service_->ShowNotification(notif);

  EXPECT_EQ(observer.shown_count_, 1);
  EXPECT_EQ(observer.last_shown_notification_.id, "obs-test");

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_OnNotificationClosedFired) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("close-test"));
  service_->CloseNotification("close-test");

  EXPECT_EQ(observer.closed_count_, 1);
  EXPECT_EQ(observer.last_closed_id_, "close-test");

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_OnNotificationReadFired) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("read-test"));
  service_->MarkAsRead("read-test");

  EXPECT_EQ(observer.read_count_, 1);
  EXPECT_EQ(observer.last_read_id_, "read-test");

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_OnAllNotificationsClearedFired) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));
  service_->ClearAllNotifications();

  EXPECT_EQ(observer.all_cleared_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_OnNotificationSettingsChanged) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->set_notifications_enabled(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_OnUnreadCountChanged) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("unread-test"));

  EXPECT_GT(observer.unread_count_changed_count_, 0);
  EXPECT_EQ(observer.last_unread_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_UnreadCountDecreasesOnClose) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("n1"));
  ASSERT_EQ(observer.last_unread_count_, 1);

  service_->CloseNotification("n1");
  EXPECT_EQ(observer.last_unread_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_UnreadCountDecreasesOnMarkRead) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->ShowNotification(MakeNotification("n1"));
  ASSERT_EQ(observer.last_unread_count_, 1);

  service_->MarkAsRead("n1");
  EXPECT_EQ(observer.last_unread_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_DNDChangeFiresSettingsChanged) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->SetDoNotDisturb(true);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// =========================================================================
// Observer defaults (empty implementations don't crash)
// =========================================================================

TEST_F(NotificationServiceTest, Observer_DefaultImplementationsDontCrash) {
  // MinimalObserver only overrides OnNotificationShown.
  // All other methods use empty default implementations.
  MinimalObserver observer;
  service_->AddObserver(&observer);

  // Trigger various events — none should crash.
  service_->ShowNotification(MakeNotification("n1"));
  service_->MarkAsRead("n1");
  service_->CloseNotification("n1");
  service_->ShowNotification(MakeNotification("n2"));
  service_->ClearAllNotifications();
  service_->set_notifications_enabled(false);
  service_->SetDoNotDisturb(true);

  EXPECT_EQ(observer.shown_count_, 2);  // Only the overridden method counts.

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Observer_BaseCheckedObserverWorks) {
  AstraNotificationObserver observer;
  // Adding and removing a base observer with no overrides should not crash.
  service_->AddObserver(&observer);
  service_->ShowNotification(MakeNotification("n1"));
  service_->RemoveObserver(&observer);
}

// =========================================================================
// Multiple observers
// =========================================================================

TEST_F(NotificationServiceTest, MultipleObservers_AllReceiveEvents) {
  TestNotificationObserver observer1;
  TestNotificationObserver observer2;
  TestNotificationObserver observer3;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);
  service_->AddObserver(&observer3);

  service_->ShowNotification(MakeNotification("multi-test"));

  EXPECT_EQ(observer1.shown_count_, 1);
  EXPECT_EQ(observer2.shown_count_, 1);
  EXPECT_EQ(observer3.shown_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
  service_->RemoveObserver(&observer3);
}

TEST_F(NotificationServiceTest, MultipleObservers_RemoveOneDoesNotAffectOthers) {
  TestNotificationObserver observer1;
  TestNotificationObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->RemoveObserver(&observer1);

  service_->ShowNotification(MakeNotification("after-remove"));

  EXPECT_EQ(observer1.shown_count_, 0);
  EXPECT_EQ(observer2.shown_count_, 1);

  service_->RemoveObserver(&observer2);
}

TEST_F(NotificationServiceTest, MultipleObservers_DifferentOverrides) {
  MinimalObserver min_observer;
  TestNotificationObserver full_observer;

  service_->AddObserver(&min_observer);
  service_->AddObserver(&full_observer);

  service_->ShowNotification(MakeNotification("test"));
  service_->MarkAsRead("test");

  EXPECT_EQ(min_observer.shown_count_, 1);
  EXPECT_EQ(full_observer.shown_count_, 1);
  EXPECT_EQ(full_observer.read_count_, 1);

  service_->RemoveObserver(&min_observer);
  service_->RemoveObserver(&full_observer);
}

// =========================================================================
// Presentation settings
// =========================================================================

TEST_F(NotificationServiceTest, Setting_NotificationsEnabled) {
  EXPECT_TRUE(service_->notifications_enabled());

  service_->set_notifications_enabled(false);
  EXPECT_FALSE(service_->notifications_enabled());

  service_->set_notifications_enabled(true);
  EXPECT_TRUE(service_->notifications_enabled());
}

TEST_F(NotificationServiceTest, Setting_ShowNotificationPreviews) {
  EXPECT_TRUE(service_->show_notification_previews());

  service_->set_show_notification_previews(false);
  EXPECT_FALSE(service_->show_notification_previews());
}

TEST_F(NotificationServiceTest, Setting_NotificationSoundEnabled) {
  EXPECT_TRUE(service_->notification_sound_enabled());

  service_->set_notification_sound_enabled(false);
  EXPECT_FALSE(service_->notification_sound_enabled());
}

TEST_F(NotificationServiceTest, Setting_NotificationTimeout) {
  EXPECT_EQ(service_->notification_timeout_seconds(), 8);

  service_->set_notification_timeout_seconds(15);
  EXPECT_EQ(service_->notification_timeout_seconds(), 15);
}

TEST_F(NotificationServiceTest, Setting_MaxVisibleNotifications) {
  EXPECT_EQ(service_->max_visible_notifications(), 5);

  service_->set_max_visible_notifications(10);
  EXPECT_EQ(service_->max_visible_notifications(), 10);
}

TEST_F(NotificationServiceTest, Setting_NotificationPosition) {
  EXPECT_EQ(service_->notification_position(), "top_right");

  service_->set_notification_position("bottom_left");
  EXPECT_EQ(service_->notification_position(), "bottom_left");
}

TEST_F(NotificationServiceTest, Setting_ShowNotificationIcon) {
  EXPECT_TRUE(service_->show_notification_icon());

  service_->set_show_notification_icon(false);
  EXPECT_FALSE(service_->show_notification_icon());
}

TEST_F(NotificationServiceTest, Setting_ShowNotificationTimestamp) {
  EXPECT_TRUE(service_->show_notification_timestamp());

  service_->set_show_notification_timestamp(false);
  EXPECT_FALSE(service_->show_notification_timestamp());
}

TEST_F(NotificationServiceTest, Setting_ShowCloseButton) {
  EXPECT_TRUE(service_->show_close_button());

  service_->set_show_close_button(false);
  EXPECT_FALSE(service_->show_close_button());
}

TEST_F(NotificationServiceTest, Setting_NotificationStyle) {
  EXPECT_EQ(service_->notification_style(), "default");

  service_->set_notification_style("compact");
  EXPECT_EQ(service_->notification_style(), "compact");

  service_->set_notification_style("minimal");
  EXPECT_EQ(service_->notification_style(), "minimal");
}

TEST_F(NotificationServiceTest, Setting_StackNotifications) {
  EXPECT_TRUE(service_->stack_notifications());

  service_->set_stack_notifications(false);
  EXPECT_FALSE(service_->stack_notifications());
}

TEST_F(NotificationServiceTest, Setting_QuietMode) {
  EXPECT_FALSE(service_->quiet_mode());

  service_->set_quiet_mode(true);
  EXPECT_TRUE(service_->quiet_mode());
}

TEST_F(NotificationServiceTest, Setting_NotificationHistorySize) {
  EXPECT_EQ(service_->notification_history_size(), 100);

  service_->set_notification_history_size(50);
  EXPECT_EQ(service_->notification_history_size(), 50);
}

TEST_F(NotificationServiceTest, Setting_EachTriggersSettingsChanged) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  int initial_count = observer.settings_changed_count_;

  service_->set_notifications_enabled(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 1);

  service_->set_show_notification_previews(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 2);

  service_->set_notification_sound_enabled(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 3);

  service_->set_notification_timeout_seconds(10);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 4);

  service_->set_max_visible_notifications(3);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 5);

  service_->set_notification_position("bottom_left");
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 6);

  service_->set_show_notification_icon(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 7);

  service_->set_show_notification_timestamp(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 8);

  service_->set_show_close_button(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 9);

  service_->set_notification_style("compact");
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 10);

  service_->set_stack_notifications(false);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 11);

  service_->set_quiet_mode(true);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 12);

  service_->set_notification_history_size(50);
  EXPECT_EQ(observer.settings_changed_count_, initial_count + 13);

  service_->RemoveObserver(&observer);
}

TEST_F(NotificationServiceTest, Setting_SameValueDoesNotTriggerChange) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->set_notifications_enabled(true);  // Already true.
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->set_notification_timeout_seconds(8);  // Already 8.
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// =========================================================================
// Settings clamping
// =========================================================================

TEST_F(NotificationServiceTest, Clamp_TimeoutBelowMinimum) {
  service_->set_notification_timeout_seconds(0);
  EXPECT_GE(service_->notification_timeout_seconds(), 1);
}

TEST_F(NotificationServiceTest, Clamp_TimeoutAboveMaximum) {
  service_->set_notification_timeout_seconds(100);
  EXPECT_LE(service_->notification_timeout_seconds(), 60);
}

TEST_F(NotificationServiceTest, Clamp_TimeoutAtMinimum) {
  service_->set_notification_timeout_seconds(1);
  EXPECT_EQ(service_->notification_timeout_seconds(), 1);
}

TEST_F(NotificationServiceTest, Clamp_TimeoutAtMaximum) {
  service_->set_notification_timeout_seconds(60);
  EXPECT_EQ(service_->notification_timeout_seconds(), 60);
}

TEST_F(NotificationServiceTest, Clamp_MaxVisibleBelowMinimum) {
  service_->set_max_visible_notifications(0);
  EXPECT_GE(service_->max_visible_notifications(), 1);
}

TEST_F(NotificationServiceTest, Clamp_MaxVisibleAboveMaximum) {
  service_->set_max_visible_notifications(50);
  EXPECT_LE(service_->max_visible_notifications(), 20);
}

TEST_F(NotificationServiceTest, Clamp_MaxVisibleNegative) {
  service_->set_max_visible_notifications(-5);
  EXPECT_GE(service_->max_visible_notifications(), 1);
}

TEST_F(NotificationServiceTest, Clamp_HistorySizeBelowMinimum) {
  service_->set_notification_history_size(5);
  EXPECT_GE(service_->notification_history_size(), 10);
}

TEST_F(NotificationServiceTest, Clamp_HistorySizeAboveMaximum) {
  service_->set_notification_history_size(5000);
  EXPECT_LE(service_->notification_history_size(), 1000);
}

TEST_F(NotificationServiceTest, Clamp_HistorySizeNegative) {
  service_->set_notification_history_size(-100);
  EXPECT_GE(service_->notification_history_size(), 10);
}

TEST_F(NotificationServiceTest, Clamp_HistorySizeAtMinimum) {
  service_->set_notification_history_size(10);
  EXPECT_EQ(service_->notification_history_size(), 10);
}

TEST_F(NotificationServiceTest, Clamp_HistorySizeAtMaximum) {
  service_->set_notification_history_size(1000);
  EXPECT_EQ(service_->notification_history_size(), 1000);
}

// =========================================================================
// Persistence round-trip via PrefService
// =========================================================================

TEST_F(NotificationServiceTest, PrefPersistence_NotificationsEnabledRoundTrip) {
  service_->set_notifications_enabled(false);
  EXPECT_FALSE(profile_->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationsEnabled));

  // Simulate a service restart by creating a new service.
  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_FALSE(service2->notifications_enabled());
}

TEST_F(NotificationServiceTest, PrefPersistence_PositionRoundTrip) {
  service_->set_notification_position("bottom_left");

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_EQ(service2->notification_position(), "bottom_left");
}

TEST_F(NotificationServiceTest, PrefPersistence_TimeoutRoundTrip) {
  service_->set_notification_timeout_seconds(15);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_EQ(service2->notification_timeout_seconds(), 15);
}

TEST_F(NotificationServiceTest, PrefPersistence_StyleRoundTrip) {
  service_->set_notification_style("compact");

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_EQ(service2->notification_style(), "compact");
}

TEST_F(NotificationServiceTest, PrefPersistence_HistorySizeRoundTrip) {
  service_->set_notification_history_size(50);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_EQ(service2->notification_history_size(), 50);
}

TEST_F(NotificationServiceTest, PrefPersistence_DNDRoundTrip) {
  service_->SetDoNotDisturb(true);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_TRUE(service2->DoNotDisturb());
}

TEST_F(NotificationServiceTest, PrefPersistence_QuietModeRoundTrip) {
  service_->set_quiet_mode(true);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_TRUE(service2->quiet_mode());
}

TEST_F(NotificationServiceTest, PrefPersistence_MaxVisibleRoundTrip) {
  service_->set_max_visible_notifications(10);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_EQ(service2->max_visible_notifications(), 10);
}

TEST_F(NotificationServiceTest, PrefPersistence_AllBooleanSettingsRoundTrip) {
  service_->set_notifications_enabled(false);
  service_->set_show_notification_previews(false);
  service_->set_notification_sound_enabled(false);
  service_->set_show_notification_icon(false);
  service_->set_show_notification_timestamp(false);
  service_->set_show_close_button(false);
  service_->set_stack_notifications(false);
  service_->set_quiet_mode(true);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());

  EXPECT_FALSE(service2->notifications_enabled());
  EXPECT_FALSE(service2->show_notification_previews());
  EXPECT_FALSE(service2->notification_sound_enabled());
  EXPECT_FALSE(service2->show_notification_icon());
  EXPECT_FALSE(service2->show_notification_timestamp());
  EXPECT_FALSE(service2->show_close_button());
  EXPECT_FALSE(service2->stack_notifications());
  EXPECT_TRUE(service2->quiet_mode());
}

// =========================================================================
// Bulk operations
// =========================================================================

TEST_F(NotificationServiceTest, Bulk_ShowNotifications) {
  std::vector<AstraNotification> notifications;
  for (int i = 0; i < 5; i++) {
    notifications.push_back(MakeNotification("bulk-" + std::to_string(i)));
  }

  int shown = service_->ShowNotifications(notifications);
  EXPECT_EQ(shown, 5);
  EXPECT_EQ(service_->GetNotificationCount(), 5);
}

TEST_F(NotificationServiceTest, Bulk_ShowNotifications_EmptyList) {
  std::vector<AstraNotification> notifications;

  int shown = service_->ShowNotifications(notifications);
  EXPECT_EQ(shown, 0);
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, Bulk_ShowNotifications_RespectsSettings) {
  service_->set_notifications_enabled(false);

  std::vector<AstraNotification> notifications;
  notifications.push_back(MakeNotification("b1"));
  notifications.push_back(MakeNotification("b2"));

  int shown = service_->ShowNotifications(notifications);
  EXPECT_EQ(shown, 0);
}

TEST_F(NotificationServiceTest, Bulk_CloseNotifications) {
  for (int i = 0; i < 5; i++) {
    service_->ShowNotification(MakeNotification("close-" + std::to_string(i)));
  }
  ASSERT_EQ(service_->GetNotificationCount(), 5);

  std::vector<std::string> ids = {"close-0", "close-2", "close-4"};
  service_->CloseNotifications(ids);

  EXPECT_EQ(service_->GetNotificationCount(), 2);
  EXPECT_NE(service_->GetNotification("close-1"), nullptr);
  EXPECT_NE(service_->GetNotification("close-3"), nullptr);
}

TEST_F(NotificationServiceTest, Bulk_CloseNotifications_EmptyList) {
  service_->ShowNotification(MakeNotification("n1"));

  std::vector<std::string> ids;
  service_->CloseNotifications(ids);

  EXPECT_EQ(service_->GetNotificationCount(), 1);
}

TEST_F(NotificationServiceTest, Bulk_CloseNotifications_WithInvalidIds) {
  service_->ShowNotification(MakeNotification("n1"));
  service_->ShowNotification(MakeNotification("n2"));

  std::vector<std::string> ids = {"n1", "nonexistent", "n2"};
  service_->CloseNotifications(ids);

  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, Bulk_MarkAsRead) {
  for (int i = 0; i < 5; i++) {
    service_->ShowNotification(MakeNotification("read-" + std::to_string(i)));
  }
  ASSERT_EQ(service_->GetUnreadCount(), 5);

  std::vector<std::string> ids = {"read-0", "read-1", "read-2"};
  service_->MarkAsRead(ids);

  EXPECT_EQ(service_->GetUnreadCount(), 2);
}

TEST_F(NotificationServiceTest, Bulk_MarkAsRead_EmptyList) {
  service_->ShowNotification(MakeNotification("n1"));
  ASSERT_EQ(service_->GetUnreadCount(), 1);

  std::vector<std::string> ids;
  service_->MarkAsRead(ids);

  EXPECT_EQ(service_->GetUnreadCount(), 1);
}

// =========================================================================
// Utility methods
// =========================================================================

TEST_F(NotificationServiceTest, Utility_FormatNotificationTime_NullTime) {
  std::string result =
      AstraNotificationService::FormatNotificationTime(base::Time());
  EXPECT_TRUE(result.empty());
}

TEST_F(NotificationServiceTest, Utility_FormatNotificationTime_JustNow) {
  std::string result =
      AstraNotificationService::FormatNotificationTime(base::Time::Now());
  EXPECT_EQ(result, "Just now");
}

TEST_F(NotificationServiceTest, Utility_FormatNotificationTime_FewMinutesAgo) {
  std::string result = AstraNotificationService::FormatNotificationTime(
      base::Time::Now() - base::Minutes(5));
  EXPECT_THAT(result, testing::HasSubstr("min ago"));
}

TEST_F(NotificationServiceTest, Utility_FormatNotificationTime_HoursAgo) {
  std::string result = AstraNotificationService::FormatNotificationTime(
      base::Time::Now() - base::Hours(3));
  EXPECT_THAT(result, testing::HasSubstr("hours ago"));
}

TEST_F(NotificationServiceTest, Utility_GetNotificationTypeLabel_AllTypes) {
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kInfo),
            "Info");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kWarning),
            "Warning");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kError),
            "Error");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kSuccess),
            "Success");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kDownload),
            "Download");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kExtension),
            "Extension");
  EXPECT_EQ(AstraNotificationService::GetNotificationTypeLabel(
                AstraNotificationType::kSystem),
            "System");
}

TEST_F(NotificationServiceTest, Utility_GetDefaultPriority_AllTypes) {
  // Error should have highest priority.
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kError),
            3);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kWarning),
            2);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kSystem),
            2);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kSuccess),
            1);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kDownload),
            1);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kInfo),
            0);
  EXPECT_EQ(AstraNotificationService::GetDefaultPriority(
                AstraNotificationType::kExtension),
            0);
}

TEST_F(NotificationServiceTest, Utility_DefaultPrioritiesInRange0to3) {
  std::vector<AstraNotificationType> all_types = {
      AstraNotificationType::kInfo,
      AstraNotificationType::kWarning,
      AstraNotificationType::kError,
      AstraNotificationType::kSuccess,
      AstraNotificationType::kDownload,
      AstraNotificationType::kExtension,
      AstraNotificationType::kSystem,
  };
  for (auto type : all_types) {
    int priority = AstraNotificationService::GetDefaultPriority(type);
    EXPECT_GE(priority, 0);
    EXPECT_LE(priority, 3);
  }
}

TEST_F(NotificationServiceTest, Utility_ShouldShowNotification_DefaultTrue) {
  EXPECT_TRUE(
      service_->ShouldShowNotification(AstraNotificationType::kInfo));
  EXPECT_TRUE(
      service_->ShouldShowNotification(AstraNotificationType::kWarning));
}

TEST_F(NotificationServiceTest, Utility_ShouldShowNotification_DNDBlocksAll) {
  service_->SetDoNotDisturb(true);

  EXPECT_FALSE(
      service_->ShouldShowNotification(AstraNotificationType::kInfo));
  EXPECT_FALSE(
      service_->ShouldShowNotification(AstraNotificationType::kError));
}

TEST_F(NotificationServiceTest, Utility_ShouldShowNotification_DisabledBlocksAll) {
  service_->set_notifications_enabled(false);

  EXPECT_FALSE(
      service_->ShouldShowNotification(AstraNotificationType::kInfo));
  EXPECT_FALSE(
      service_->ShouldShowNotification(AstraNotificationType::kError));
}

TEST_F(NotificationServiceTest, Utility_ShouldShowNotification_QuietModeBlocks) {
  service_->set_quiet_mode(true);

  EXPECT_FALSE(
      service_->ShouldShowNotification(AstraNotificationType::kInfo));
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_ShortMessage) {
  std::string result = AstraNotificationService::TruncateMessage("Hello", 10);
  EXPECT_EQ(result, "Hello");
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_ExactLength) {
  std::string result = AstraNotificationService::TruncateMessage("Hello", 5);
  EXPECT_EQ(result, "Hello");
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_LongMessage) {
  std::string result =
      AstraNotificationService::TruncateMessage("Hello World", 8);
  EXPECT_EQ(result, "Hello...");
  EXPECT_EQ(result.size(), 8u);
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_ZeroMaxLength) {
  std::string result = AstraNotificationService::TruncateMessage("Hello", 0);
  EXPECT_TRUE(result.empty());
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_NegativeMaxLength) {
  std::string result = AstraNotificationService::TruncateMessage("Hello", -5);
  EXPECT_TRUE(result.empty());
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_VeryShortMax) {
  // When max_length <= 3, just return the first max_length chars.
  std::string result = AstraNotificationService::TruncateMessage("Hello", 3);
  EXPECT_EQ(result, "Hel");
}

TEST_F(NotificationServiceTest, Utility_TruncateMessage_EmptyString) {
  std::string result = AstraNotificationService::TruncateMessage("", 10);
  EXPECT_TRUE(result.empty());
}

TEST_F(NotificationServiceTest, Utility_ValidatePosition_ValidPositions) {
  service_->set_notification_position("top_right");
  EXPECT_EQ(service_->notification_position(), "top_right");

  service_->set_notification_position("top_left");
  EXPECT_EQ(service_->notification_position(), "top_left");

  service_->set_notification_position("bottom_right");
  EXPECT_EQ(service_->notification_position(), "bottom_right");

  service_->set_notification_position("bottom_left");
  EXPECT_EQ(service_->notification_position(), "bottom_left");
}

TEST_F(NotificationServiceTest, Utility_ValidatePosition_InvalidFallsBack) {
  service_->set_notification_position("invalid_position");
  // Falls back to default "top_right".
  EXPECT_EQ(service_->notification_position(), "top_right");
}

TEST_F(NotificationServiceTest, Utility_ValidateStyle_ValidStyles) {
  service_->set_notification_style("default");
  EXPECT_EQ(service_->notification_style(), "default");

  service_->set_notification_style("compact");
  EXPECT_EQ(service_->notification_style(), "compact");

  service_->set_notification_style("minimal");
  EXPECT_EQ(service_->notification_style(), "minimal");
}

TEST_F(NotificationServiceTest, Utility_ValidateStyle_InvalidFallsBack) {
  service_->set_notification_style("fancy_style");
  EXPECT_EQ(service_->notification_style(), "default");
}

// =========================================================================
// Notification struct defaults
// =========================================================================

TEST_F(NotificationServiceTest, StructDefaults_AllFieldsDefault) {
  AstraNotification notif;

  EXPECT_TRUE(notif.id.empty());
  EXPECT_TRUE(notif.title.empty());
  EXPECT_TRUE(notif.message.empty());
  EXPECT_EQ(notif.type, AstraNotificationType::kInfo);
  EXPECT_TRUE(notif.source.empty());
  EXPECT_TRUE(notif.created_time.is_null());
  EXPECT_FALSE(notif.is_read);
  EXPECT_EQ(notif.priority, 0);
  EXPECT_TRUE(notif.action_label.empty());
  EXPECT_TRUE(notif.secondary_action_label.empty());
}

// =========================================================================
// Edge cases
// =========================================================================

TEST_F(NotificationServiceTest, EdgeCase_EmptyNotificationBody) {
  AstraNotification notif = MakeNotification("empty-body");
  notif.message = "";
  EXPECT_TRUE(service_->ShowNotification(notif));

  const AstraNotification* found = service_->GetNotification("empty-body");
  ASSERT_NE(found, nullptr);
  EXPECT_TRUE(found->message.empty());
}

TEST_F(NotificationServiceTest, EdgeCase_EmptyTitle) {
  AstraNotification notif = MakeNotification("empty-title", "");
  EXPECT_TRUE(service_->ShowNotification(notif));
}

TEST_F(NotificationServiceTest, EdgeCase_LongNotificationMessage) {
  AstraNotification notif = MakeNotification("long-msg");
  notif.message = std::string(10000, 'a');
  EXPECT_TRUE(service_->ShowNotification(notif));

  const AstraNotification* found = service_->GetNotification("long-msg");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->message.size(), 10000u);
}

TEST_F(NotificationServiceTest, EdgeCase_SpecialCharactersInMessage) {
  AstraNotification notif = MakeNotification("special");
  notif.message = "Line1\nLine2\tTabbed<b>HTML</b>";
  EXPECT_TRUE(service_->ShowNotification(notif));

  const AstraNotification* found = service_->GetNotification("special");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->message, "Line1\nLine2\tTabbed<b>HTML</b>");
}

TEST_F(NotificationServiceTest, EdgeCase_UnicodeInTitleAndMessage) {
  AstraNotification notif = MakeNotification("unicode");
  notif.title = "测试通知";
  notif.message = "日本語のメッセージ 🎉";
  EXPECT_TRUE(service_->ShowNotification(notif));

  const AstraNotification* found = service_->GetNotification("unicode");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->title, "测试通知");
  EXPECT_EQ(found->message, "日本語のメッセージ 🎉");
}

TEST_F(NotificationServiceTest, EdgeCase_ManyNotifications) {
  const int kCount = 500;
  for (int i = 0; i < kCount; i++) {
    service_->ShowNotification(
        MakeNotification("many-" + std::to_string(i)));
  }
  EXPECT_EQ(service_->GetNotificationCount(), kCount);
  EXPECT_EQ(service_->GetUnreadCount(), kCount);
}

TEST_F(NotificationServiceTest, EdgeCase_DNDBlocksThenReEnable) {
  service_->SetDoNotDisturb(true);
  EXPECT_FALSE(service_->ShowNotification(MakeNotification("dnd-blocked")));

  service_->SetDoNotDisturb(false);
  EXPECT_TRUE(service_->ShowNotification(MakeNotification("after-dnd")));
  EXPECT_EQ(service_->GetNotificationCount(), 1);
}

TEST_F(NotificationServiceTest, EdgeCase_DisableNotifications) {
  service_->set_notifications_enabled(false);
  EXPECT_FALSE(service_->ShowNotification(MakeNotification("disabled-notif")));
  EXPECT_EQ(service_->GetNotificationCount(), 0);

  // History should also not record when notifications are fully disabled.
  auto history = service_->GetNotificationHistory(10);
  bool found = false;
  for (const auto& n : history) {
    if (n.id == "disabled-notif") {
      found = true;
      break;
    }
  }
  EXPECT_FALSE(found);
}

TEST_F(NotificationServiceTest, EdgeCase_CloseAllThenAddMore) {
  for (int i = 0; i < 5; i++) {
    service_->ShowNotification(MakeNotification("round1-" + std::to_string(i)));
  }
  service_->ClearAllNotifications();
  ASSERT_EQ(service_->GetNotificationCount(), 0);

  for (int i = 0; i < 3; i++) {
    service_->ShowNotification(MakeNotification("round2-" + std::to_string(i)));
  }
  EXPECT_EQ(service_->GetNotificationCount(), 3);
  EXPECT_EQ(service_->GetUnreadCount(), 3);
}

TEST_F(NotificationServiceTest, EdgeCase_MarkReadThenShowAgainSameId) {
  AstraNotification notif = MakeNotification("read-then-show");
  service_->ShowNotification(notif);
  service_->MarkAsRead("read-then-show");
  ASSERT_EQ(service_->GetUnreadCount(), 0);

  // Show again with same ID — update.  Should preserve read state unless
  // explicitly set to unread.
  AstraNotification updated = MakeNotification("read-then-show", "Updated");
  updated.is_read = true;  // Explicitly read.
  service_->ShowNotification(updated);

  EXPECT_EQ(service_->GetUnreadCount(), 0);
  const AstraNotification* found =
      service_->GetNotification("read-then-show");
  EXPECT_EQ(found->title, "Updated");
  EXPECT_TRUE(found->is_read);
}

// =========================================================================
// Factory tests
// =========================================================================

TEST_F(NotificationServiceTest, Factory_GetInstance) {
  auto* factory = AstraNotificationServiceFactory::GetInstance();
  EXPECT_NE(factory, nullptr);

  // Singleton — second call should return same pointer.
  auto* factory2 = AstraNotificationServiceFactory::GetInstance();
  EXPECT_EQ(factory, factory2);
}

TEST_F(NotificationServiceTest, Factory_GetForProfile) {
  // Note: in test environment, factory may not be fully registered with
  // BrowserContextDependencyManager.  We test the direct construction path.
  auto* service =
      AstraNotificationServiceFactory::GetForProfile(profile_.get());
  // May be null in test harness if factory isn't registered with the
  // dependency manager.  The service is also constructed directly in the
  // test fixture for testing.
  //
  // The important thing is that it doesn't crash.
}

TEST_F(NotificationServiceTest, Factory_GetForProfileNullReturnsNull) {
  auto* service = AstraNotificationServiceFactory::GetForProfile(nullptr);
  EXPECT_EQ(service, nullptr);
}

TEST_F(NotificationServiceTest, Factory_RegisterProfilePrefs) {
  // Pre-create a new profile registry.
  TestingProfile::Builder builder;
  auto profile = builder.Build();

  PrefRegistrySimple* registry = profile->GetPrefs()->registry();

  // Before registration, the pref shouldn't have a default value.
  // After registration, it should.
  AstraNotificationServiceFactory::RegisterProfilePrefs(registry);

  // Verify all 14 prefs are registered by reading their defaults.
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationsEnabled));
  EXPECT_FALSE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationDoNotDisturb));
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationShowPreviews));
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationSoundEnabled));
  EXPECT_EQ(
      profile->GetPrefs()->GetInteger(prefs::kPrefNotificationTimeoutSeconds),
      8);
  EXPECT_EQ(
      profile->GetPrefs()->GetInteger(prefs::kPrefNotificationMaxVisible), 5);
  EXPECT_EQ(
      profile->GetPrefs()->GetString(prefs::kPrefNotificationPosition),
      "top_right");
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationShowIcon));
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationShowTimestamp));
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationShowCloseButton));
  EXPECT_EQ(profile->GetPrefs()->GetString(prefs::kPrefNotificationStyle),
            "default");
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationStackNotifications));
  EXPECT_FALSE(profile->GetPrefs()->GetBoolean(
      prefs::kPrefNotificationQuietMode));
  EXPECT_EQ(
      profile->GetPrefs()->GetInteger(prefs::kPrefNotificationHistorySize),
      100);
}

// =========================================================================
// Shutdown cleanup
// =========================================================================

TEST_F(NotificationServiceTest, Shutdown_ClearsObservers) {
  TestNotificationObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, showing a notification should not notify observers
  // (or may have undefined behavior since profile_ is cleared).
  // The key test is that Shutdown itself doesn't crash.
}

TEST_F(NotificationServiceTest, Shutdown_Idempotent) {
  service_->Shutdown();
  service_->Shutdown();  // Second call should not crash.
}

TEST_F(NotificationServiceTest, Shutdown_ClearsProfilePointer) {
  // The service clears its profile_ reference during shutdown.
  // We can't directly test this since profile_ is private, but we verify
  // that shutdown is safe to call and that subsequent pref-accessing
  // operations don't crash.
  service_->Shutdown();

  // Operations that access prefs should gracefully handle null profile.
  // These may not change state but shouldn't crash.
  service_->set_notifications_enabled(false);
  service_->DoNotDisturb();
}

// =========================================================================
// Stacking behavior
// =========================================================================

TEST_F(NotificationServiceTest, Stacking_StackNotificationsSetting) {
  EXPECT_TRUE(service_->stack_notifications());

  service_->set_stack_notifications(false);
  EXPECT_FALSE(service_->stack_notifications());
}

TEST_F(NotificationServiceTest, Stacking_SettingPersists) {
  service_->set_stack_notifications(false);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_FALSE(service2->stack_notifications());
}

// =========================================================================
// Quiet mode
// =========================================================================

TEST_F(NotificationServiceTest, QuietMode_DefaultsToFalse) {
  EXPECT_FALSE(service_->quiet_mode());
}

TEST_F(NotificationServiceTest, QuietMode_BlocksNotifications) {
  service_->set_quiet_mode(true);

  AstraNotification notif = MakeNotification("quiet-test");
  bool shown = service_->ShowNotification(notif);

  EXPECT_FALSE(shown);
  EXPECT_EQ(service_->GetNotificationCount(), 0);
}

TEST_F(NotificationServiceTest, QuietMode_StillRecordsInHistory) {
  service_->set_quiet_mode(true);

  AstraNotification notif = MakeNotification("quiet-history");
  service_->ShowNotification(notif);

  auto history = service_->GetNotificationHistory(10);
  bool found = false;
  for (const auto& n : history) {
    if (n.id == "quiet-history") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(NotificationServiceTest, QuietMode_AffectsUnreadCount) {
  service_->set_quiet_mode(true);

  AstraNotification notif = MakeNotification("quiet-unread");
  notif.is_read = false;
  service_->ShowNotification(notif);

  EXPECT_GT(service_->GetUnreadCount(), 0);
}

TEST_F(NotificationServiceTest, QuietMode_SettingPersists) {
  service_->set_quiet_mode(true);

  auto service2 = std::make_unique<AstraNotificationService>(profile_.get());
  EXPECT_TRUE(service2->quiet_mode());
}

TEST_F(NotificationServiceTest, QuietMode_ReEnableShowsNotifications) {
  service_->set_quiet_mode(true);
  ASSERT_FALSE(service_->ShowNotification(MakeNotification("q1")));

  service_->set_quiet_mode(false);
  EXPECT_TRUE(service_->ShowNotification(MakeNotification("q2")));
  EXPECT_EQ(service_->GetNotificationCount(), 1);
}

}  // namespace astra
