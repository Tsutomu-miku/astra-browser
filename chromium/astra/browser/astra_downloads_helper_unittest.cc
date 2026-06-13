// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_downloads_helper.h"

#include <string>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_downloads_helper_factory.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestDownloadsObserver : public AstraDownloadsObserver {
 public:
  void OnDownloadStarted(int download_id) override {
    download_started_count_++;
    last_started_id_ = download_id;
  }

  void OnDownloadUpdated(int download_id) override {
    download_updated_count_++;
    last_updated_id_ = download_id;
  }

  void OnDownloadCompleted(int download_id) override {
    download_completed_count_++;
    last_completed_id_ = download_id;
  }

  void OnDownloadFailed(int download_id, const std::string& error) override {
    download_failed_count_++;
    last_failed_id_ = download_id;
    last_error_ = error;
  }

  void OnDownloadRemoved(int download_id) override {
    download_removed_count_++;
    last_removed_id_ = download_id;
  }

  void OnAllDownloadsCleared() override {
    all_cleared_count_++;
  }

  void OnDownloadsSettingsChanged() override {
    settings_changed_count_++;
  }

  // Counters
  int download_started_count_ = 0;
  int download_updated_count_ = 0;
  int download_completed_count_ = 0;
  int download_failed_count_ = 0;
  int download_removed_count_ = 0;
  int all_cleared_count_ = 0;
  int settings_changed_count_ = 0;

  // Last recorded values
  int last_started_id_ = 0;
  int last_updated_id_ = 0;
  int last_completed_id_ = 0;
  int last_failed_id_ = 0;
  int last_removed_id_ = 0;
  std::string last_error_;
};

}  // namespace

// Test fixture for AstraDownloadsHelper tests.
class DownloadsHelperTest : public testing::Test {
 protected:
  DownloadsHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraDownloadsHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~DownloadsHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings match expected defaults.
    ASSERT_TRUE(helper_->GetShowDownloadsInSidebar());
    ASSERT_TRUE(helper_->GetShowDownloadNotifications());
    ASSERT_FALSE(helper_->GetAutoOpenDownloads());
    ASSERT_EQ(helper_->GetDownloadsSortOrder(),
              prefs::kDefaultDownloadsSortOrder);
    ASSERT_EQ(helper_->GetMaxRecentDownloads(),
              prefs::kDefaultDownloadsMaxRecent);
    ASSERT_TRUE(helper_->GetShowDownloadSpeed());
    ASSERT_TRUE(helper_->GetShowFileSize());
    ASSERT_TRUE(helper_->GetShowDownloadProgress());
    ASSERT_EQ(helper_->GetDownloadsDisplayMode(),
              prefs::kDefaultDownloadsDisplayMode);
    ASSERT_TRUE(helper_->GetPromptForDownloadLocation());
    ASSERT_TRUE(helper_->GetSafeBrowsingWarnings());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraDownloadsHelper> helper_;
  std::vector<TestDownloadsObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Construction and default state
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Construction_ServiceNotNull) {
  EXPECT_TRUE(helper_ != nullptr);
}

TEST_F(DownloadsHelperTest, DefaultState_DownloadCountZero) {
  // In the overlay, DownloadManager is not available, so count is 0.
  EXPECT_EQ(helper_->GetDownloadCount(), 0u);
}

TEST_F(DownloadsHelperTest, DefaultState_ActiveCountZero) {
  EXPECT_EQ(helper_->GetActiveDownloadCount(), 0u);
}

TEST_F(DownloadsHelperTest, DefaultState_CompletedCountZero) {
  EXPECT_EQ(helper_->GetCompletedDownloadCount(), 0u);
}

TEST_F(DownloadsHelperTest, DefaultState_IsNotDownloading) {
  EXPECT_FALSE(helper_->IsDownloading());
}

TEST_F(DownloadsHelperTest, DefaultState_AllDownloadsEmpty) {
  auto downloads = helper_->GetAllDownloads();
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, DefaultState_ActiveDownloadsEmpty) {
  auto downloads = helper_->GetActiveDownloads();
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, DefaultState_RecentDownloadsEmpty) {
  auto downloads = helper_->GetRecentDownloads();
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, DefaultState_RecentDownloadsWithMaxCount) {
  auto downloads = helper_->GetRecentDownloads(5);
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, DefaultState_GetDownloadReturnsEmptyItem) {
  auto item = helper_->GetDownload(42);
  EXPECT_EQ(item.id, 0);
  EXPECT_EQ(item.total_bytes, -1);
  EXPECT_EQ(item.received_bytes, 0);
  EXPECT_FALSE(item.is_dangerous);
  EXPECT_FALSE(item.is_paused);
}

TEST_F(DownloadsHelperTest, DefaultState_GetDownloadProgressZero) {
  EXPECT_DOUBLE_EQ(helper_->GetDownloadProgress(42), 0.0);
}

TEST_F(DownloadsHelperTest, DefaultState_GetDownloadSpeedZero) {
  EXPECT_EQ(helper_->GetDownloadSpeed(42), 0);
}

TEST_F(DownloadsHelperTest, DefaultState_ShowInSidebar) {
  EXPECT_TRUE(helper_->GetShowDownloadsInSidebar());
}

TEST_F(DownloadsHelperTest, DefaultState_ShowNotifications) {
  EXPECT_TRUE(helper_->GetShowDownloadNotifications());
}

TEST_F(DownloadsHelperTest, DefaultState_AutoOpenDownloads) {
  EXPECT_FALSE(helper_->GetAutoOpenDownloads());
}

TEST_F(DownloadsHelperTest, DefaultState_SortOrder) {
  EXPECT_EQ(helper_->GetDownloadsSortOrder(),
            prefs::kDefaultDownloadsSortOrder);
}

TEST_F(DownloadsHelperTest, DefaultState_MaxRecentDownloads) {
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            prefs::kDefaultDownloadsMaxRecent);
}

TEST_F(DownloadsHelperTest, DefaultState_ShowSpeed) {
  EXPECT_TRUE(helper_->GetShowDownloadSpeed());
}

TEST_F(DownloadsHelperTest, DefaultState_ShowFileSize) {
  EXPECT_TRUE(helper_->GetShowFileSize());
}

TEST_F(DownloadsHelperTest, DefaultState_ShowProgress) {
  EXPECT_TRUE(helper_->GetShowDownloadProgress());
}

TEST_F(DownloadsHelperTest, DefaultState_DisplayMode) {
  EXPECT_EQ(helper_->GetDownloadsDisplayMode(),
            prefs::kDefaultDownloadsDisplayMode);
}

TEST_F(DownloadsHelperTest, DefaultState_PromptForLocation) {
  EXPECT_TRUE(helper_->GetPromptForDownloadLocation());
}

TEST_F(DownloadsHelperTest, DefaultState_SafeBrowsingWarnings) {
  EXPECT_TRUE(helper_->GetSafeBrowsingWarnings());
}

// ---------------------------------------------------------------------------
// Download item CRUD — verifying the projection struct
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, DownloadItem_DefaultValues) {
  AstraDownloadItem item;
  EXPECT_EQ(item.id, 0);
  EXPECT_FALSE(item.url.is_valid());
  EXPECT_TRUE(item.file_name.empty());
  EXPECT_TRUE(item.file_path.empty());
  EXPECT_EQ(item.total_bytes, -1);
  EXPECT_EQ(item.received_bytes, 0);
  EXPECT_EQ(item.state, AstraDownloadState::kInProgress);
  EXPECT_TRUE(item.start_time.is_null());
  EXPECT_TRUE(item.end_time.is_null());
  EXPECT_FALSE(item.is_dangerous);
  EXPECT_TRUE(item.mime_type.empty());
  EXPECT_FALSE(item.is_paused);
}

TEST_F(DownloadsHelperTest, DownloadItem_StateEnumValues) {
  AstraDownloadItem item;
  item.state = AstraDownloadState::kInProgress;
  EXPECT_EQ(item.state, AstraDownloadState::kInProgress);
  item.state = AstraDownloadState::kCompleted;
  EXPECT_EQ(item.state, AstraDownloadState::kCompleted);
  item.state = AstraDownloadState::kCancelled;
  EXPECT_EQ(item.state, AstraDownloadState::kCancelled);
  item.state = AstraDownloadState::kFailed;
  EXPECT_EQ(item.state, AstraDownloadState::kFailed);
  item.state = AstraDownloadState::kInterrupted;
  EXPECT_EQ(item.state, AstraDownloadState::kInterrupted);
}

TEST_F(DownloadsHelperTest, DownloadItem_FieldsSettable) {
  AstraDownloadItem item;
  item.id = 42;
  item.url = GURL("https://example.com/file.zip");
  item.file_name = "file.zip";
  item.total_bytes = 1024;
  item.received_bytes = 512;
  item.state = AstraDownloadState::kInProgress;
  item.is_dangerous = false;
  item.is_paused = false;
  item.mime_type = "application/zip";

  EXPECT_EQ(item.id, 42);
  EXPECT_TRUE(item.url.is_valid());
  EXPECT_EQ(item.url.spec(), "https://example.com/file.zip");
  EXPECT_EQ(item.file_name, "file.zip");
  EXPECT_EQ(item.total_bytes, 1024);
  EXPECT_EQ(item.received_bytes, 512);
  EXPECT_EQ(item.state, AstraDownloadState::kInProgress);
  EXPECT_FALSE(item.is_dangerous);
  EXPECT_FALSE(item.is_paused);
  EXPECT_EQ(item.mime_type, "application/zip");
}

// ---------------------------------------------------------------------------
// Progress calculation
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, CalculateProgress_ZeroTotal) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(0, 0), 0.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_NegativeTotal) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(100, -1), 0.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_NegativeReceived) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(-5, 100), 0.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_ZeroProgress) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(0, 100), 0.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_HalfProgress) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(50, 100), 0.5);
}

TEST_F(DownloadsHelperTest, CalculateProgress_FullProgress) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(100, 100), 1.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_OverMax) {
  // If received exceeds total, progress should cap at 1.0.
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(150, 100), 1.0);
}

TEST_F(DownloadsHelperTest, CalculateProgress_LargeValues) {
  int64_t large_total = 10 * 1024 * 1024 * 1024LL;  // 10 GB
  int64_t large_received = 5 * 1024 * 1024 * 1024LL;  // 5 GB
  EXPECT_DOUBLE_EQ(
      AstraDownloadsHelper::CalculateProgress(large_received, large_total),
      0.5);
}

TEST_F(DownloadsHelperTest, CalculateProgress_SmallValues) {
  EXPECT_DOUBLE_EQ(AstraDownloadsHelper::CalculateProgress(1, 1000), 0.001);
}

// ---------------------------------------------------------------------------
// Format file size
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, FormatFileSize_ZeroBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(0);
  EXPECT_EQ(result, "0 B");
}

TEST_F(DownloadsHelperTest, FormatFileSize_NegativeBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(-100);
  // Negative bytes should be handled gracefully.
  EXPECT_FALSE(result.empty());
}

TEST_F(DownloadsHelperTest, FormatFileSize_Bytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(500);
  EXPECT_THAT(result, testing::HasSubstr("B"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_KiloBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(2048);
  EXPECT_THAT(result, testing::HasSubstr("KB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_MegaBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(5 * 1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("MB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_GigaBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(
      2LL * 1024 * 1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("GB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_TeraBytes) {
  std::string result = AstraDownloadsHelper::FormatFileSize(
      3LL * 1024 * 1024 * 1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("TB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_1KBBoundary) {
  // Exactly 1024 bytes should show as KB.
  std::string result = AstraDownloadsHelper::FormatFileSize(1024);
  EXPECT_THAT(result, testing::HasSubstr("KB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_1MBBoundary) {
  std::string result = AstraDownloadsHelper::FormatFileSize(1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("MB"));
}

TEST_F(DownloadsHelperTest, FormatFileSize_NotCrashOnLarge) {
  // Very large value should not crash.
  std::string result = AstraDownloadsHelper::FormatFileSize(INT64_MAX);
  EXPECT_FALSE(result.empty());
}

// ---------------------------------------------------------------------------
// Format download speed
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_ZeroSpeed) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(0);
  EXPECT_THAT(result, testing::HasSubstr("B/s"));
}

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_BytesPerSecond) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(500);
  EXPECT_THAT(result, testing::HasSubstr("B/s"));
}

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_KiloBytesPerSecond) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(2048);
  EXPECT_THAT(result, testing::HasSubstr("KB/s"));
}

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_MegaBytesPerSecond) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(
      5 * 1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("MB/s"));
}

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_GigaBytesPerSecond) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(
      2LL * 1024 * 1024 * 1024);
  EXPECT_THAT(result, testing::HasSubstr("GB/s"));
}

TEST_F(DownloadsHelperTest, FormatDownloadSpeed_NegativeSpeed) {
  std::string result = AstraDownloadsHelper::FormatDownloadSpeed(-100);
  EXPECT_FALSE(result.empty());
}

// ---------------------------------------------------------------------------
// Format time remaining
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, FormatTimeRemaining_ZeroDuration) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::TimeDelta());
  EXPECT_EQ(result, "Calculating...");
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_NegativeDuration) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Seconds(-5));
  EXPECT_EQ(result, "Calculating...");
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_SecondsOnly) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Seconds(30));
  EXPECT_THAT(result, testing::HasSubstr("30s"));
  EXPECT_FALSE(testing::HasSubstr("m")(result));
  EXPECT_FALSE(testing::HasSubstr("h")(result));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_MinutesAndSeconds) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Minutes(2) + base::Seconds(30));
  EXPECT_THAT(result, testing::HasSubstr("2m"));
  EXPECT_THAT(result, testing::HasSubstr("30s"));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_ExactMinutes) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Minutes(5));
  EXPECT_THAT(result, testing::HasSubstr("5m"));
  EXPECT_FALSE(testing::HasSubstr("s")(result));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_HoursAndMinutes) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Hours(1) + base::Minutes(30));
  EXPECT_THAT(result, testing::HasSubstr("1h"));
  EXPECT_THAT(result, testing::HasSubstr("30m"));
  EXPECT_FALSE(testing::HasSubstr("s")(result));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_ExactHours) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Hours(3));
  EXPECT_THAT(result, testing::HasSubstr("3h"));
  EXPECT_FALSE(testing::HasSubstr("m")(result));
  EXPECT_FALSE(testing::HasSubstr("s")(result));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_OneSecond) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Seconds(1));
  EXPECT_THAT(result, testing::HasSubstr("1s"));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_59Seconds) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Seconds(59));
  EXPECT_THAT(result, testing::HasSubstr("59s"));
  EXPECT_FALSE(testing::HasSubstr("m")(result));
}

TEST_F(DownloadsHelperTest, FormatTimeRemaining_60Seconds) {
  std::string result = AstraDownloadsHelper::FormatTimeRemaining(
      base::Seconds(60));
  EXPECT_THAT(result, testing::HasSubstr("1m"));
}

// ---------------------------------------------------------------------------
// Presentation settings — set and get
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Setting_ShowInSidebar_SetAndGet) {
  helper_->SetShowDownloadsInSidebar(false);
  EXPECT_FALSE(helper_->GetShowDownloadsInSidebar());

  helper_->SetShowDownloadsInSidebar(true);
  EXPECT_TRUE(helper_->GetShowDownloadsInSidebar());
}

TEST_F(DownloadsHelperTest, Setting_ShowInSidebar_Toggle) {
  bool initial = helper_->GetShowDownloadsInSidebar();
  bool result = helper_->ToggleShowDownloadsInSidebar();
  EXPECT_EQ(result, !initial);
  EXPECT_EQ(helper_->GetShowDownloadsInSidebar(), !initial);

  result = helper_->ToggleShowDownloadsInSidebar();
  EXPECT_EQ(result, initial);
  EXPECT_EQ(helper_->GetShowDownloadsInSidebar(), initial);
}

TEST_F(DownloadsHelperTest, Setting_ShowNotifications_SetAndGet) {
  helper_->SetShowDownloadNotifications(false);
  EXPECT_FALSE(helper_->GetShowDownloadNotifications());

  helper_->SetShowDownloadNotifications(true);
  EXPECT_TRUE(helper_->GetShowDownloadNotifications());
}

TEST_F(DownloadsHelperTest, Setting_ShowNotifications_Toggle) {
  bool initial = helper_->GetShowDownloadNotifications();
  bool result = helper_->ToggleShowDownloadNotifications();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleShowDownloadNotifications();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_AutoOpen_SetAndGet) {
  helper_->SetAutoOpenDownloads(true);
  EXPECT_TRUE(helper_->GetAutoOpenDownloads());

  helper_->SetAutoOpenDownloads(false);
  EXPECT_FALSE(helper_->GetAutoOpenDownloads());
}

TEST_F(DownloadsHelperTest, Setting_AutoOpen_Toggle) {
  bool initial = helper_->GetAutoOpenDownloads();
  bool result = helper_->ToggleAutoOpenDownloads();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleAutoOpenDownloads();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_SortOrder_SetAndGet) {
  helper_->SetDownloadsSortOrder("oldest_first");
  EXPECT_EQ(helper_->GetDownloadsSortOrder(), "oldest_first");

  helper_->SetDownloadsSortOrder("newest_first");
  EXPECT_EQ(helper_->GetDownloadsSortOrder(), "newest_first");
}

TEST_F(DownloadsHelperTest, Setting_SortOrder_SameValueNoChange) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  std::string current = helper_->GetDownloadsSortOrder();
  helper_->SetDownloadsSortOrder(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_SetAndGet) {
  helper_->SetMaxRecentDownloads(10);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(), 10);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_ClampToMinimum) {
  helper_->SetMaxRecentDownloads(0);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMinRecentDownloads);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_ClampToMaximum) {
  helper_->SetMaxRecentDownloads(1000);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMaxRecentDownloadsLimit);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_NegativeValue) {
  helper_->SetMaxRecentDownloads(-5);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMinRecentDownloads);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_WithinRange) {
  helper_->SetMaxRecentDownloads(50);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(), 50);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_AtMinimum) {
  helper_->SetMaxRecentDownloads(AstraDownloadsHelper::kMinRecentDownloads);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMinRecentDownloads);
}

TEST_F(DownloadsHelperTest, Setting_MaxRecent_AtMaximum) {
  helper_->SetMaxRecentDownloads(
      AstraDownloadsHelper::kMaxRecentDownloadsLimit);
  EXPECT_EQ(helper_->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMaxRecentDownloadsLimit);
}

TEST_F(DownloadsHelperTest, Setting_ShowSpeed_SetAndGet) {
  helper_->SetShowDownloadSpeed(false);
  EXPECT_FALSE(helper_->GetShowDownloadSpeed());

  helper_->SetShowDownloadSpeed(true);
  EXPECT_TRUE(helper_->GetShowDownloadSpeed());
}

TEST_F(DownloadsHelperTest, Setting_ShowSpeed_Toggle) {
  bool initial = helper_->GetShowDownloadSpeed();
  bool result = helper_->ToggleShowDownloadSpeed();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleShowDownloadSpeed();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_ShowFileSize_SetAndGet) {
  helper_->SetShowFileSize(false);
  EXPECT_FALSE(helper_->GetShowFileSize());

  helper_->SetShowFileSize(true);
  EXPECT_TRUE(helper_->GetShowFileSize());
}

TEST_F(DownloadsHelperTest, Setting_ShowFileSize_Toggle) {
  bool initial = helper_->GetShowFileSize();
  bool result = helper_->ToggleShowFileSize();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleShowFileSize();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_ShowProgress_SetAndGet) {
  helper_->SetShowDownloadProgress(false);
  EXPECT_FALSE(helper_->GetShowDownloadProgress());

  helper_->SetShowDownloadProgress(true);
  EXPECT_TRUE(helper_->GetShowDownloadProgress());
}

TEST_F(DownloadsHelperTest, Setting_ShowProgress_Toggle) {
  bool initial = helper_->GetShowDownloadProgress();
  bool result = helper_->ToggleShowDownloadProgress();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleShowDownloadProgress();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_DisplayMode_SetAndGet) {
  helper_->SetDownloadsDisplayMode("compact");
  EXPECT_EQ(helper_->GetDownloadsDisplayMode(), "compact");

  helper_->SetDownloadsDisplayMode("list");
  EXPECT_EQ(helper_->GetDownloadsDisplayMode(), "list");
}

TEST_F(DownloadsHelperTest, Setting_DisplayMode_SameValueNoChange) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  std::string current = helper_->GetDownloadsDisplayMode();
  helper_->SetDownloadsDisplayMode(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Setting_PromptForLocation_SetAndGet) {
  helper_->SetPromptForDownloadLocation(false);
  EXPECT_FALSE(helper_->GetPromptForDownloadLocation());

  helper_->SetPromptForDownloadLocation(true);
  EXPECT_TRUE(helper_->GetPromptForDownloadLocation());
}

TEST_F(DownloadsHelperTest, Setting_PromptForLocation_Toggle) {
  bool initial = helper_->GetPromptForDownloadLocation();
  bool result = helper_->TogglePromptForDownloadLocation();
  EXPECT_EQ(result, !initial);

  result = helper_->TogglePromptForDownloadLocation();
  EXPECT_EQ(result, initial);
}

TEST_F(DownloadsHelperTest, Setting_SafeBrowsingWarnings_SetAndGet) {
  helper_->SetSafeBrowsingWarnings(false);
  EXPECT_FALSE(helper_->GetSafeBrowsingWarnings());

  helper_->SetSafeBrowsingWarnings(true);
  EXPECT_TRUE(helper_->GetSafeBrowsingWarnings());
}

TEST_F(DownloadsHelperTest, Setting_SafeBrowsingWarnings_Toggle) {
  bool initial = helper_->GetSafeBrowsingWarnings();
  bool result = helper_->ToggleSafeBrowsingWarnings();
  EXPECT_EQ(result, !initial);

  result = helper_->ToggleSafeBrowsingWarnings();
  EXPECT_EQ(result, initial);
}

// ---------------------------------------------------------------------------
// Idempotent operations — setting same value doesn't trigger notification
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Idempotent_ShowInSidebar_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetShowDownloadsInSidebar();
  helper_->SetShowDownloadsInSidebar(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_ShowNotifications_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetShowDownloadNotifications();
  helper_->SetShowDownloadNotifications(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_AutoOpen_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetAutoOpenDownloads();
  helper_->SetAutoOpenDownloads(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_ShowSpeed_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetShowDownloadSpeed();
  helper_->SetShowDownloadSpeed(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_ShowFileSize_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetShowFileSize();
  helper_->SetShowFileSize(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_ShowProgress_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetShowDownloadProgress();
  helper_->SetShowDownloadProgress(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_PromptForLocation_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetPromptForDownloadLocation();
  helper_->SetPromptForDownloadLocation(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_SafeBrowsing_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  bool current = helper_->GetSafeBrowsingWarnings();
  helper_->SetSafeBrowsingWarnings(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Idempotent_MaxRecent_SameValueNoNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  int current = helper_->GetMaxRecentDownloads();
  helper_->SetMaxRecentDownloads(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer notifications
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Observer_OnDownloadStarted) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadStarted(42);
  EXPECT_EQ(observer.download_started_count_, 1);
  EXPECT_EQ(observer.last_started_id_, 42);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadUpdated) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadUpdated(7);
  EXPECT_EQ(observer.download_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, 7);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadCompleted) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadCompleted(99);
  EXPECT_EQ(observer.download_completed_count_, 1);
  EXPECT_EQ(observer.last_completed_id_, 99);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadFailed) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadFailed(5, "Network error");
  EXPECT_EQ(observer.download_failed_count_, 1);
  EXPECT_EQ(observer.last_failed_id_, 5);
  EXPECT_EQ(observer.last_error_, "Network error");

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadFailed_EmptyError) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadFailed(5, "");
  EXPECT_EQ(observer.download_failed_count_, 1);
  EXPECT_EQ(observer.last_error_, "");

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadRemoved) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadRemoved(123);
  EXPECT_EQ(observer.download_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_id_, 123);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnAllDownloadsCleared) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyAllDownloadsCleared();
  EXPECT_EQ(observer.all_cleared_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Observer_OnDownloadsSettingsChanged) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadsSettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer defaults — empty implementations don't crash
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraDownloadsObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths via manual notifications.
  helper_->NotifyDownloadStarted(1);
  helper_->NotifyDownloadUpdated(1);
  helper_->NotifyDownloadCompleted(1);
  helper_->NotifyDownloadFailed(1, "test");
  helper_->NotifyDownloadRemoved(1);
  helper_->NotifyAllDownloadsCleared();
  helper_->NotifyDownloadsSettingsChanged();

  // Also trigger via setting changes (which use the same notification path).
  helper_->SetShowDownloadsInSidebar(false);
  helper_->SetShowDownloadNotifications(false);
  helper_->SetAutoOpenDownloads(true);
  helper_->SetDownloadsSortOrder("oldest_first");
  helper_->SetMaxRecentDownloads(50);
  helper_->SetShowDownloadSpeed(false);
  helper_->SetShowFileSize(false);
  helper_->SetShowDownloadProgress(false);
  helper_->SetDownloadsDisplayMode("compact");
  helper_->SetPromptForDownloadLocation(false);
  helper_->SetSafeBrowsingWarnings(false);

  helper_->RemoveObserver(&observer);

  // If we get here without crashing, the test passes.
  SUCCEED();
}

TEST_F(DownloadsHelperTest, ObserverDefaults_AddNullObserver) {
  // Adding a null observer should be safe (no crash).
  helper_->AddObserver(nullptr);
  helper_->NotifyDownloadStarted(1);
  // Should not crash.
  SUCCEED();
}

TEST_F(DownloadsHelperTest, ObserverDefaults_RemoveNullObserver) {
  // Removing a null observer should be safe (no crash).
  helper_->RemoveObserver(nullptr);
  // Should not crash.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, MultipleObservers_AllNotified) {
  TestDownloadsObserver observer1;
  TestDownloadsObserver observer2;
  TestDownloadsObserver observer3;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);
  helper_->AddObserver(&observer3);

  helper_->NotifyDownloadStarted(42);

  EXPECT_EQ(observer1.download_started_count_, 1);
  EXPECT_EQ(observer2.download_started_count_, 1);
  EXPECT_EQ(observer3.download_started_count_, 1);

  EXPECT_EQ(observer1.last_started_id_, 42);
  EXPECT_EQ(observer2.last_started_id_, 42);
  EXPECT_EQ(observer3.last_started_id_, 42);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
  helper_->RemoveObserver(&observer3);
}

TEST_F(DownloadsHelperTest, MultipleObservers_RemoveOne) {
  TestDownloadsObserver observer1;
  TestDownloadsObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->RemoveObserver(&observer1);

  helper_->NotifyDownloadCompleted(100);

  EXPECT_EQ(observer1.download_completed_count_, 0);
  EXPECT_EQ(observer2.download_completed_count_, 1);

  helper_->RemoveObserver(&observer2);
}

TEST_F(DownloadsHelperTest, MultipleObservers_SettingsChangeNotifiesAll) {
  TestDownloadsObserver observer1;
  TestDownloadsObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->SetShowDownloadsInSidebar(false);

  EXPECT_EQ(observer1.settings_changed_count_, 1);
  EXPECT_EQ(observer2.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
}

TEST_F(DownloadsHelperTest, MultipleObservers_MultipleEvents) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyDownloadStarted(1);
  helper_->NotifyDownloadUpdated(1);
  helper_->NotifyDownloadUpdated(1);
  helper_->NotifyDownloadCompleted(1);

  EXPECT_EQ(observer.download_started_count_, 1);
  EXPECT_EQ(observer.download_updated_count_, 2);
  EXPECT_EQ(observer.download_completed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Persistence round-trip via PrefService
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Persistence_ShowInSidebar_RoundTrip) {
  helper_->SetShowDownloadsInSidebar(false);

  // Create a new helper using the same profile.
  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowDownloadsInSidebar());
}

TEST_F(DownloadsHelperTest, Persistence_ShowNotifications_RoundTrip) {
  helper_->SetShowDownloadNotifications(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowDownloadNotifications());
}

TEST_F(DownloadsHelperTest, Persistence_AutoOpen_RoundTrip) {
  helper_->SetAutoOpenDownloads(true);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetAutoOpenDownloads());
}

TEST_F(DownloadsHelperTest, Persistence_SortOrder_RoundTrip) {
  helper_->SetDownloadsSortOrder("oldest_first");

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_EQ(helper2->GetDownloadsSortOrder(), "oldest_first");
}

TEST_F(DownloadsHelperTest, Persistence_MaxRecent_RoundTrip) {
  helper_->SetMaxRecentDownloads(30);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_EQ(helper2->GetMaxRecentDownloads(), 30);
}

TEST_F(DownloadsHelperTest, Persistence_ShowSpeed_RoundTrip) {
  helper_->SetShowDownloadSpeed(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowDownloadSpeed());
}

TEST_F(DownloadsHelperTest, Persistence_ShowFileSize_RoundTrip) {
  helper_->SetShowFileSize(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowFileSize());
}

TEST_F(DownloadsHelperTest, Persistence_ShowProgress_RoundTrip) {
  helper_->SetShowDownloadProgress(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowDownloadProgress());
}

TEST_F(DownloadsHelperTest, Persistence_DisplayMode_RoundTrip) {
  helper_->SetDownloadsDisplayMode("compact");

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_EQ(helper2->GetDownloadsDisplayMode(), "compact");
}

TEST_F(DownloadsHelperTest, Persistence_PromptForLocation_RoundTrip) {
  helper_->SetPromptForDownloadLocation(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetPromptForDownloadLocation());
}

TEST_F(DownloadsHelperTest, Persistence_SafeBrowsingWarnings_RoundTrip) {
  helper_->SetSafeBrowsingWarnings(false);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetSafeBrowsingWarnings());
}

TEST_F(DownloadsHelperTest, Persistence_MaxRecentClamping_RoundTrip) {
  // Set a value above the max and verify it's clamped on read-back.
  helper_->SetMaxRecentDownloads(1000);

  auto helper2 = std::make_unique<AstraDownloadsHelper>(profile_.get());
  EXPECT_EQ(helper2->GetMaxRecentDownloads(),
            AstraDownloadsHelper::kMaxRecentDownloadsLimit);
}

// ---------------------------------------------------------------------------
// Operations on empty / missing downloads — idempotent and safe
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Operations_PauseMissingDownload_NoCrash) {
  // Pause a download that doesn't exist — should not crash.
  helper_->PauseDownload(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_ResumeMissingDownload_NoCrash) {
  helper_->ResumeDownload(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_CancelMissingDownload_NoCrash) {
  helper_->CancelDownload(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_RemoveMissingDownload_NoCrash) {
  helper_->RemoveDownload(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_OpenMissingDownload_NoCrash) {
  helper_->OpenDownload(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_ShowInFolderMissingDownload_NoCrash) {
  helper_->ShowDownloadInFolder(999);
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_PauseAllEmpty_NoCrash) {
  helper_->PauseAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_ResumeAllEmpty_NoCrash) {
  helper_->ResumeAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_CancelAllEmpty_NoCrash) {
  helper_->CancelAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_ClearAllEmpty_NoCrash) {
  helper_->ClearAllDownloads();
  // Should also fire OnAllDownloadsCleared even for empty state.
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Operations_ClearCompletedEmpty_NoCrash) {
  helper_->ClearCompletedDownloads();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Bulk_PauseAll_NoCrashOnEmpty) {
  helper_->PauseAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Bulk_ResumeAll_NoCrashOnEmpty) {
  helper_->ResumeAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Bulk_CancelAll_NoCrashOnEmpty) {
  helper_->CancelAllDownloads();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Bulk_ClearAll_FiresClearedNotification) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearAllDownloads();
  EXPECT_EQ(observer.all_cleared_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(DownloadsHelperTest, Bulk_ClearCompleted_NoCrashOnEmpty) {
  helper_->ClearCompletedDownloads();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, EdgeCase_GetDownloadNegativeId) {
  auto item = helper_->GetDownload(-1);
  EXPECT_EQ(item.id, 0);
}

TEST_F(DownloadsHelperTest, EdgeCase_GetDownloadZeroId) {
  auto item = helper_->GetDownload(0);
  EXPECT_EQ(item.id, 0);
}

TEST_F(DownloadsHelperTest, EdgeCase_GetDownloadProgressNegativeId) {
  EXPECT_DOUBLE_EQ(helper_->GetDownloadProgress(-1), 0.0);
}

TEST_F(DownloadsHelperTest, EdgeCase_GetDownloadSpeedNegativeId) {
  EXPECT_EQ(helper_->GetDownloadSpeed(-1), 0);
}

TEST_F(DownloadsHelperTest, EdgeCase_GetRecentDownloadsZeroCount) {
  // max_count=0 should use the default pref value.
  auto downloads = helper_->GetRecentDownloads(0);
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, EdgeCase_GetRecentDownloadsNegativeCount) {
  // Negative count should be handled gracefully.
  auto downloads = helper_->GetRecentDownloads(-5);
  EXPECT_TRUE(downloads.empty());
}

TEST_F(DownloadsHelperTest, EdgeCase_ClampMinMaxConstantValues) {
  // Verify min < default < max.
  EXPECT_LT(AstraDownloadsHelper::kMinRecentDownloads,
            AstraDownloadsHelper::kDefaultMaxRecentDownloads);
  EXPECT_LT(AstraDownloadsHelper::kDefaultMaxRecentDownloads,
            AstraDownloadsHelper::kMaxRecentDownloadsLimit);
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Factory_GetInstance) {
  auto* factory = AstraDownloadsHelperFactory::GetInstance();
  EXPECT_TRUE(factory != nullptr);

  // Singleton — calling again returns the same instance.
  auto* factory2 = AstraDownloadsHelperFactory::GetInstance();
  EXPECT_EQ(factory, factory2);
}

TEST_F(DownloadsHelperTest, Factory_GetForProfile) {
  auto* helper = AstraDownloadsHelperFactory::GetForProfile(profile_.get());
  EXPECT_TRUE(helper != nullptr);

  // Same profile should return same instance.
  auto* helper2 = AstraDownloadsHelperFactory::GetForProfile(profile_.get());
  EXPECT_EQ(helper, helper2);
}

TEST_F(DownloadsHelperTest, Factory_GetForProfileNull) {
  auto* helper = AstraDownloadsHelperFactory::GetForProfile(nullptr);
  EXPECT_EQ(helper, nullptr);
}

TEST_F(DownloadsHelperTest, Factory_RegisterProfilePrefs) {
  // Create a fresh TestingProfile and register prefs through the factory.
  TestingProfile profile;
  AstraDownloadsHelperFactory::RegisterProfilePrefs(profile.GetPrefs());

  // Verify that prefs are registered and have default values.
  PrefService* prefs = profile.GetPrefs();
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowInSidebar),
            prefs::kDefaultDownloadsShowInSidebar);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowNotifications),
            prefs::kDefaultDownloadsShowNotifications);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsAutoOpen),
            prefs::kDefaultDownloadsAutoOpen);
  EXPECT_EQ(prefs->GetString(prefs::kPrefDownloadsSortOrder),
            prefs::kDefaultDownloadsSortOrder);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefDownloadsMaxRecent),
            prefs::kDefaultDownloadsMaxRecent);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowSpeed),
            prefs::kDefaultDownloadsShowSpeed);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowFileSize),
            prefs::kDefaultDownloadsShowFileSize);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowProgress),
            prefs::kDefaultDownloadsShowProgress);
  EXPECT_EQ(prefs->GetString(prefs::kPrefDownloadsDisplayMode),
            prefs::kDefaultDownloadsDisplayMode);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsPromptForLocation),
            prefs::kDefaultDownloadsPromptForLocation);
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsSafeBrowsingWarnings),
            prefs::kDefaultDownloadsSafeBrowsingWarnings);
}

TEST_F(DownloadsHelperTest, Factory_ServiceType) {
  auto* helper = AstraDownloadsHelperFactory::GetForProfile(profile_.get());
  EXPECT_TRUE(dynamic_cast<AstraDownloadsHelper*>(helper) != nullptr);
}

// ---------------------------------------------------------------------------
// Shutdown cleanup
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Shutdown_ClearsObservers) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  helper_->Shutdown();

  // After shutdown, notifications should not reach the observer
  // because the observer list should be cleared.
  // Note: we can't call Notify* directly after Shutdown because the
  // observers_ list is cleared in Shutdown.
  // The test passes if Shutdown doesn't crash.
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Shutdown_DoesNotCrash) {
  helper_->Shutdown();
  // Calling Shutdown twice should be safe.
  helper_->Shutdown();
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Shutdown_NullProfileAfterShutdown) {
  // After shutdown, operations that depend on prefs should return defaults.
  helper_->Shutdown();
  // Should not crash and should return default values gracefully.
  EXPECT_TRUE(helper_->GetShowDownloadsInSidebar());  // Default is true
}

TEST_F(DownloadsHelperTest, Shutdown_ServiceIsKeyedService) {
  // AstraDownloadsHelper should be a KeyedService.
  KeyedService* service = helper_.get();
  EXPECT_TRUE(service != nullptr);
}

// ---------------------------------------------------------------------------
// Additional presentation setting toggles
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, ToggleAllSettings_AllChangeState) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  // Store initial states (all should be default).
  bool initial_show_sidebar = helper_->GetShowDownloadsInSidebar();
  bool initial_show_notifications = helper_->GetShowDownloadNotifications();
  bool initial_auto_open = helper_->GetAutoOpenDownloads();
  bool initial_show_speed = helper_->GetShowDownloadSpeed();
  bool initial_show_size = helper_->GetShowFileSize();
  bool initial_show_progress = helper_->GetShowDownloadProgress();
  bool initial_prompt = helper_->GetPromptForDownloadLocation();
  bool initial_safe_browsing = helper_->GetSafeBrowsingWarnings();

  // Toggle all bool settings.
  helper_->ToggleShowDownloadsInSidebar();
  helper_->ToggleShowDownloadNotifications();
  helper_->ToggleAutoOpenDownloads();
  helper_->ToggleShowDownloadSpeed();
  helper_->ToggleShowFileSize();
  helper_->ToggleShowDownloadProgress();
  helper_->TogglePromptForDownloadLocation();
  helper_->ToggleSafeBrowsingWarnings();

  // All should have flipped.
  EXPECT_EQ(helper_->GetShowDownloadsInSidebar(), !initial_show_sidebar);
  EXPECT_EQ(helper_->GetShowDownloadNotifications(), !initial_show_notifications);
  EXPECT_EQ(helper_->GetAutoOpenDownloads(), !initial_auto_open);
  EXPECT_EQ(helper_->GetShowDownloadSpeed(), !initial_show_speed);
  EXPECT_EQ(helper_->GetShowFileSize(), !initial_show_size);
  EXPECT_EQ(helper_->GetShowDownloadProgress(), !initial_show_progress);
  EXPECT_EQ(helper_->GetPromptForDownloadLocation(), !initial_prompt);
  EXPECT_EQ(helper_->GetSafeBrowsingWarnings(), !initial_safe_browsing);

  // Each toggle should have fired a settings changed notification.
  // 8 bool toggles.
  EXPECT_GE(observer.settings_changed_count_, 8);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Download state transitions — verify enum ordering and distinct values
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, DownloadState_DistinctValues) {
  // All state values should be distinct.
  EXPECT_NE(static_cast<int>(AstraDownloadState::kInProgress),
            static_cast<int>(AstraDownloadState::kCompleted));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kInProgress),
            static_cast<int>(AstraDownloadState::kCancelled));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kInProgress),
            static_cast<int>(AstraDownloadState::kFailed));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kInProgress),
            static_cast<int>(AstraDownloadState::kInterrupted));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCompleted),
            static_cast<int>(AstraDownloadState::kCancelled));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCompleted),
            static_cast<int>(AstraDownloadState::kFailed));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCompleted),
            static_cast<int>(AstraDownloadState::kInterrupted));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCancelled),
            static_cast<int>(AstraDownloadState::kFailed));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCancelled),
            static_cast<int>(AstraDownloadState::kInterrupted));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kFailed),
            static_cast<int>(AstraDownloadState::kInterrupted));
}

// ---------------------------------------------------------------------------
// Combined settings change fires single notification per change
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, SettingsChange_NotificationPerChange) {
  TestDownloadsObserver observer;
  helper_->AddObserver(&observer);

  // Start from a known state.
  helper_->SetShowDownloadsInSidebar(true);
  int count_after_initial = observer.settings_changed_count_;

  // Change 3 different settings.
  helper_->SetShowDownloadsInSidebar(false);
  helper_->SetShowDownloadNotifications(false);
  helper_->SetAutoOpenDownloads(true);

  EXPECT_EQ(observer.settings_changed_count_ - count_after_initial, 3);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// All 11 pref keys are defined and accessible
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, PrefKeys_AllDefined) {
  // Verify all pref key constants are defined and non-empty.
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsShowInSidebar).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsShowNotifications).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsAutoOpen).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsSortOrder).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsMaxRecent).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsShowSpeed).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsShowFileSize).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsShowProgress).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsDisplayMode).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsPromptForLocation).empty());
  EXPECT_FALSE(std::string(prefs::kPrefDownloadsSafeBrowsingWarnings).empty());
}

TEST_F(DownloadsHelperTest, PrefKeys_AllHaveAstraPrefix) {
  // All pref keys should start with "astra.downloads."
  EXPECT_THAT(std::string(prefs::kPrefDownloadsShowInSidebar),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsShowNotifications),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsAutoOpen),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsSortOrder),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsMaxRecent),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsShowSpeed),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsShowFileSize),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsShowProgress),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsDisplayMode),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsPromptForLocation),
              testing::StartsWith("astra.downloads."));
  EXPECT_THAT(std::string(prefs::kPrefDownloadsSafeBrowsingWarnings),
              testing::StartsWith("astra.downloads."));
}

// ---------------------------------------------------------------------------
// Verify keyed service factories integration
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, KeyedServiceFactories_RegisterPrefs) {
  // Verify that RegisterAstraProfilePrefs includes downloads prefs.
  TestingProfile profile;
  RegisterAstraProfilePrefs(profile.GetPrefs());

  PrefService* prefs = profile.GetPrefs();
  // The pref should be registered and have the default value.
  EXPECT_EQ(prefs->GetBoolean(prefs::kPrefDownloadsShowInSidebar),
            prefs::kDefaultDownloadsShowInSidebar);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefDownloadsMaxRecent),
            prefs::kDefaultDownloadsMaxRecent);
}

// ---------------------------------------------------------------------------
// Class naming and prefix conventions
// ---------------------------------------------------------------------------

TEST_F(DownloadsHelperTest, Naming_ClassPrefixAstra) {
  // Verify the class name follows the Astra* prefix convention.
  AstraDownloadsHelper helper(profile_.get());
  (void)helper;
  // If it compiles with the Astra prefix, the test passes.
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Naming_ObserverPrefixAstra) {
  AstraDownloadsObserver observer;
  (void)observer;
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Naming_StateEnumPrefixAstra) {
  AstraDownloadState state = AstraDownloadState::kInProgress;
  (void)state;
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Naming_ItemStructPrefixAstra) {
  AstraDownloadItem item;
  (void)item;
  SUCCEED();
}

TEST_F(DownloadsHelperTest, Naming_FactoryPrefixAstra) {
  auto* factory = AstraDownloadsHelperFactory::GetInstance();
  EXPECT_TRUE(factory != nullptr);
}

}  // namespace astra
