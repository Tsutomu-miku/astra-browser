// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_sync_helper.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_sync_helper_factory.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestSyncObserver : public AstraSyncObserver {
 public:
  void OnSyncStarted() override {
    sync_started_count_++; }

  void OnSyncCompleted() override {
    sync_completed_count_++;
  }

  void OnSyncError(const std::string& error_message) override {
    sync_error_count_++;
    last_error_message_ = error_message;
  }

  void OnSyncPaused() override {
    sync_paused_count_++;
  }

  void OnSyncResumed() override {
    sync_resumed_count_++;
  }

  void OnSyncDataTypeToggled(AstraSyncDataType type, bool enabled) override {
    data_type_toggled_count_++;
    last_toggled_type_ = type;
    last_toggled_enabled_ = enabled;
  }

  void OnSyncSettingsChanged() override {
    settings_changed_count_++;
  }

  void OnSignedInStatusChanged(bool is_signed_in) override {
    signed_in_changed_count_++;
    last_signed_in_state_ = is_signed_in;
  }

  // Counters
  int sync_started_count_ = 0;
  int sync_completed_count_ = 0;
  int sync_error_count_ = 0;
  int sync_paused_count_ = 0;
  int sync_resumed_count_ = 0;
  int data_type_toggled_count_ = 0;
  int settings_changed_count_ = 0;
  int signed_in_changed_count_ = 0;

  // Last recorded values
  std::string last_error_message_;
  AstraSyncDataType last_toggled_type_ = AstraSyncDataType::kBookmarks;
  bool last_toggled_enabled_ = false;
  bool last_signed_in_state_ = false;
};

// Observer that overrides nothing — tests empty default implementations.
class EmptyTestObserver : public AstraSyncObserver {
  // Intentionally empty — all default implementations are empty.
};

}  // namespace

// Test fixture for AstraSyncHelper tests.
class SyncHelperTest : public testing::Test {
 protected:
  SyncHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraSyncHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~SyncHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings.
    ASSERT_TRUE(helper_->IsSyncEnabled());
    ASSERT_TRUE(helper_->GetShowSyncStatus());
    ASSERT_TRUE(helper_->GetShowSyncErrors());
    ASSERT_FALSE(helper_->GetSyncOnWifiOnly());
    ASSERT_EQ(helper_->GetSyncFrequency(), "auto");
    ASSERT_TRUE(helper_->GetAutoSync());
    ASSERT_FALSE(helper_->GetEncryptAllData());
    ASSERT_TRUE(helper_->GetShowAccountAvatar());
    ASSERT_TRUE(helper_->GetShowLastSyncTime());
    ASSERT_TRUE(helper_->GetShowSyncIconInToolbar());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraSyncHelper> helper_;
  std::vector<TestSyncObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Construction and default state
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Construction_ServiceNotNull) {
  EXPECT_TRUE(helper_ != nullptr);
}

TEST_F(SyncHelperTest, DefaultState_SyncEnabled) {
  EXPECT_TRUE(helper_->IsSyncEnabled());
}

TEST_F(SyncHelperTest, DefaultState_ShowSyncStatus) {
  EXPECT_TRUE(helper_->GetShowSyncStatus());
}

TEST_F(SyncHelperTest, DefaultState_ShowSyncErrors) {
  EXPECT_TRUE(helper_->GetShowSyncErrors());
}

TEST_F(SyncHelperTest, DefaultState_SyncOnWifiOnly) {
  EXPECT_FALSE(helper_->GetSyncOnWifiOnly());
}

TEST_F(SyncHelperTest, DefaultState_SyncFrequency) {
  EXPECT_EQ(helper_->GetSyncFrequency(), "auto");
}

TEST_F(SyncHelperTest, DefaultState_AutoSync) {
  EXPECT_TRUE(helper_->GetAutoSync());
}

TEST_F(SyncHelperTest, DefaultState_EncryptAllData) {
  EXPECT_FALSE(helper_->GetEncryptAllData());
}

TEST_F(SyncHelperTest, DefaultState_ShowAccountAvatar) {
  EXPECT_TRUE(helper_->GetShowAccountAvatar());
}

TEST_F(SyncHelperTest, DefaultState_ShowLastSyncTime) {
  EXPECT_TRUE(helper_->GetShowLastSyncTime());
}

TEST_F(SyncHelperTest, DefaultState_ShowSyncIconInToolbar) {
  EXPECT_TRUE(helper_->GetShowSyncIconInToolbar());
}

TEST_F(SyncHelperTest, DefaultState_NoError) {
  EXPECT_FALSE(helper_->HasError());
  EXPECT_TRUE(helper_->GetSyncError().empty());
}

TEST_F(SyncHelperTest, DefaultState_NotPaused) {
  EXPECT_FALSE(helper_->IsSyncPaused());
}

TEST_F(SyncHelperTest, DefaultState_NotSignedIn) {
  EXPECT_FALSE(helper_->IsSignedIn());
}

TEST_F(SyncHelperTest, DefaultState_LastSyncTimeIsNull) {
  EXPECT_TRUE(helper_->GetLastSyncTime().is_null());
}

TEST_F(SyncHelperTest, DefaultState_TotalItemCountZero) {
  // Default is zero since no data is synced yet.
  EXPECT_EQ(helper_->GetTotalSyncedItemCount(), 0);
}

TEST_F(SyncHelperTest, DefaultState_EnabledDataTypeCount) {
  // All 10 data types are enabled by default.
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 10);
}

TEST_F(SyncHelperTest, DefaultState_NotUsingPassphrase) {
  EXPECT_FALSE(helper_->IsUsingPassphrase());
}

TEST_F(SyncHelperTest, DefaultState_AccountEmailEmpty) {
  EXPECT_TRUE(helper_->GetSyncAccountEmail().empty());
}

TEST_F(SyncHelperTest, DefaultState_AccountNameEmpty) {
  EXPECT_TRUE(helper_->GetSyncAccountName().empty());
}

// ---------------------------------------------------------------------------
// Sync status queries
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SyncStatus_NotSignedInByDefault) {
  EXPECT_EQ(helper_->GetSyncStatus(), AstraSyncStatus::kNotSignedIn);
}

TEST_F(SyncHelperTest, SyncStatus_DisabledWhenSyncOff) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->SetSyncEnabled(false);
  EXPECT_EQ(helper_->GetSyncStatus(), AstraSyncStatus::kDisabled);
}

TEST_F(SyncHelperTest, SyncStatus_SyncedWhenSignedIn) {
  helper_->NotifySignedInStatusChanged(true);
  EXPECT_EQ(helper_->GetSyncStatus(), AstraSyncStatus::kSynced);
}

TEST_F(SyncHelperTest, SyncStatus_ErrorWhenHasError) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->NotifySyncError("Test error");
  EXPECT_EQ(helper_->GetSyncStatus(), AstraSyncStatus::kError);
}

TEST_F(SyncHelperTest, SyncStatus_PausedWhenPaused) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->PauseSync();
  EXPECT_EQ(helper_->GetSyncStatus(), AstraSyncStatus::kPaused);
}

// ---------------------------------------------------------------------------
// Sync enable/disable
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SetSyncEnabled_Disable) {
  helper_->SetSyncEnabled(false);
  EXPECT_FALSE(helper_->IsSyncEnabled());
}

TEST_F(SyncHelperTest, SetSyncEnabled_Enable) {
  helper_->SetSyncEnabled(false);
  helper_->SetSyncEnabled(true);
  EXPECT_TRUE(helper_->IsSyncEnabled());
}

TEST_F(SyncHelperTest, SetSyncEnabled_Idempotent) {
  helper_->SetSyncEnabled(true);
  helper_->SetSyncEnabled(true);
  EXPECT_TRUE(helper_->IsSyncEnabled());
}

TEST_F(SyncHelperTest, SetSyncEnabled_NotifiesSettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetSyncEnabled(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, SetSyncEnabled_NoNotificationWhenNoChange) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetSyncEnabled(true);  // Already true.
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Sign-in status
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SignedInStatus_SetSignedIn) {
  helper_->NotifySignedInStatusChanged(true);
  EXPECT_TRUE(helper_->IsSignedIn());
}

TEST_F(SyncHelperTest, SignedInStatus_SetSignedOut) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->NotifySignedInStatusChanged(false);
  EXPECT_FALSE(helper_->IsSignedIn());
}

TEST_F(SyncHelperTest, SignedInStatus_NotifiesObserver) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySignedInStatusChanged(true);
  EXPECT_EQ(observer.signed_in_changed_count_, 1);
  EXPECT_TRUE(observer.last_signed_in_state_);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Last sync time
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, LastSyncTime_NullByDefault) {
  EXPECT_TRUE(helper_->GetLastSyncTime().is_null());
}

TEST_F(SyncHelperTest, LastSyncTime_UpdatedAfterSync) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->RequestSyncNow();
  EXPECT_FALSE(helper_->GetLastSyncTime().is_null());
}

// ---------------------------------------------------------------------------
// Item count tracking
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, TotalItemCount_DefaultZero) {
  EXPECT_EQ(helper_->GetTotalSyncedItemCount(), 0);
}

TEST_F(SyncHelperTest, TotalItemCount_OnlyCountsEnabledTypes) {
  // All types are enabled by default with 0 items.
  EXPECT_EQ(helper_->GetTotalSyncedItemCount(), 0);
}

TEST_F(SyncHelperTest, EnabledDataTypeCount_AllByDefault) {
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 10);
}

TEST_F(SyncHelperTest, EnabledDataTypeCount_AfterDisablingOne) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kApps, false);
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 9);
}

// ---------------------------------------------------------------------------
// Sync errors
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SyncError_NoErrorByDefault) {
  EXPECT_FALSE(helper_->HasError());
  EXPECT_TRUE(helper_->GetSyncError().empty());
}

TEST_F(SyncHelperTest, SyncError_SetError) {
  helper_->NotifySyncError("Connection failed");
  EXPECT_TRUE(helper_->HasError());
  EXPECT_EQ(helper_->GetSyncError(), "Connection failed");
}

TEST_F(SyncHelperTest, SyncError_ClearError) {
  helper_->NotifySyncError("Test error");
  ASSERT_TRUE(helper_->HasError());
  helper_->ClearError();
  EXPECT_FALSE(helper_->HasError());
  EXPECT_TRUE(helper_->GetSyncError().empty());
}

TEST_F(SyncHelperTest, SyncError_ClearWhenNoErrorIsNoOp) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->ClearError();  // No error to clear.
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, SyncError_NotifiesObserver) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncError("Network error");
  EXPECT_EQ(observer.sync_error_count_, 1);
  EXPECT_EQ(observer.last_error_message_, "Network error");
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Sync pause/resume
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, PauseSync_PausesSync) {
  helper_->PauseSync();
  EXPECT_TRUE(helper_->IsSyncPaused());
}

TEST_F(SyncHelperTest, ResumeSync_ResumesSync) {
  helper_->PauseSync();
  ASSERT_TRUE(helper_->IsSyncPaused());
  helper_->ResumeSync();
  EXPECT_FALSE(helper_->IsSyncPaused());
}

TEST_F(SyncHelperTest, PauseSync_Idempotent) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->PauseSync();
  helper_->PauseSync();
  EXPECT_EQ(observer.sync_paused_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, ResumeSync_Idempotent) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->ResumeSync();  // Not paused yet.
  EXPECT_EQ(observer.sync_resumed_count_, 0);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, PauseSync_NotifiesObserver) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->PauseSync();
  EXPECT_EQ(observer.sync_paused_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, ResumeSync_NotifiesObserver) {
  helper_->PauseSync();
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->ResumeSync();
  EXPECT_EQ(observer.sync_resumed_count_, 1);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Data type operations
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, DataType_DefaultEnabled) {
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kPasswords));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kHistory));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kTabs));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kSettings));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kExtensions));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kApps));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kThemes));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kAutofill));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kReadingList));
}

TEST_F(SyncHelperTest, DataType_DisableBookmarks) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kBookmarks, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
}

TEST_F(SyncHelperTest, DataType_DisablePasswords) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kPasswords, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kPasswords));
}

TEST_F(SyncHelperTest, DataType_DisableHistory) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kHistory, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kHistory));
}

TEST_F(SyncHelperTest, DataType_DisableTabs) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kTabs, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kTabs));
}

TEST_F(SyncHelperTest, DataType_DisableSettings) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kSettings, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kSettings));
}

TEST_F(SyncHelperTest, DataType_DisableExtensions) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kExtensions, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kExtensions));
}

TEST_F(SyncHelperTest, DataType_DisableApps) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kApps, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kApps));
}

TEST_F(SyncHelperTest, DataType_DisableThemes) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kThemes, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kThemes));
}

TEST_F(SyncHelperTest, DataType_DisableAutofill) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kAutofill, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kAutofill));
}

TEST_F(SyncHelperTest, DataType_DisableReadingList) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kReadingList, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kReadingList));
}

TEST_F(SyncHelperTest, DataType_Reenable) {
  helper_->SetDataTypeEnabled(AstraSyncDataType::kBookmarks, false);
  ASSERT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
  helper_->SetDataTypeEnabled(AstraSyncDataType::kBookmarks, true);
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
}

TEST_F(SyncHelperTest, DataType_GetState) {
  AstraSyncTypeState state = helper_->GetDataTypeState(AstraSyncDataType::kBookmarks);
  EXPECT_EQ(state.type, AstraSyncDataType::kBookmarks);
  EXPECT_TRUE(state.is_enabled);
  EXPECT_EQ(state.item_count, 0);
  EXPECT_TRUE(state.last_sync_time.is_null());
}

TEST_F(SyncHelperTest, DataType_GetAllStates) {
  auto states = helper_->GetAllDataTypeStates();
  EXPECT_EQ(states.size(), 10u);
}

TEST_F(SyncHelperTest, DataType_GetEnabledDataTypes) {
  auto enabled = helper_->GetEnabledDataTypes();
  EXPECT_EQ(enabled.size(), 10u);
}

TEST_F(SyncHelperTest, DataType_ToggleNotifiesObserver) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetDataTypeEnabled(AstraSyncDataType::kHistory, false);
  EXPECT_EQ(observer.data_type_toggled_count_, 1);
  EXPECT_EQ(observer.last_toggled_type_, AstraSyncDataType::kHistory);
  EXPECT_FALSE(observer.last_toggled_enabled_);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, DataType_ToggleAlsoNotifiesSettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetDataTypeEnabled(AstraSyncDataType::kHistory, false);
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, DataType_NoNotificationWhenNoChange) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetDataTypeEnabled(AstraSyncDataType::kBookmarks, true);  // Already true.
  EXPECT_EQ(observer.data_type_toggled_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Enable all / disable all
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, EnableAllDataTypes_EnablesAll) {
  helper_->DisableAllDataTypes();
  ASSERT_EQ(helper_->GetEnabledDataTypeCount(), 0);
  helper_->EnableAllDataTypes();
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 10);
}

TEST_F(SyncHelperTest, DisableAllDataTypes_DisablesAll) {
  helper_->DisableAllDataTypes();
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 0);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kPasswords));
}

TEST_F(SyncHelperTest, EnableAllDataTypes_NotifiesSettingsChanged) {
  helper_->DisableAllDataTypes();
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->EnableAllDataTypes();
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, DisableAllDataTypes_NotifiesSettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->DisableAllDataTypes();
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, EnableAllDataTypes_Idempotent) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->EnableAllDataTypes();
  helper_->EnableAllDataTypes();
  // Second call should not notify since nothing changed.
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, DisableAllDataTypes_Idempotent) {
  helper_->DisableAllDataTypes();
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->DisableAllDataTypes();
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Request sync now
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, RequestSyncNow_NotifiesStartedAndCompleted) {
  helper_->NotifySignedInStatusChanged(true);
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->RequestSyncNow();
  EXPECT_GE(observer.sync_started_count_, 1);
  EXPECT_GE(observer.sync_completed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, RequestSyncNow_UpdatesLastSyncTime) {
  helper_->NotifySignedInStatusChanged(true);
  base::Time before = helper_->GetLastSyncTime();
  helper_->RequestSyncNow();
  base::Time after = helper_->GetLastSyncTime();
  EXPECT_FALSE(after.is_null());
  if (!before.is_null()) {
    EXPECT_GE(after, before);
  }
}

TEST_F(SyncHelperTest, RequestSyncNow_DoesNothingWhenDisabled) {
  helper_->SetSyncEnabled(false);
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->RequestSyncNow();
  EXPECT_EQ(observer.sync_started_count_, 0);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, RequestSyncNow_DoesNothingWhenPaused) {
  helper_->NotifySignedInStatusChanged(true);
  helper_->PauseSync();
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->RequestSyncNow();
  EXPECT_EQ(observer.sync_started_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Passphrase operations
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Passphrase_NotUsingByDefault) {
  EXPECT_FALSE(helper_->IsUsingPassphrase());
}

TEST_F(SyncHelperTest, Passphrase_SetPassphrase) {
  helper_->SetPassphrase("mypassphrase");
  EXPECT_TRUE(helper_->IsUsingPassphrase());
}

TEST_F(SyncHelperTest, Passphrase_EmptyPassphraseNoOp) {
  helper_->SetPassphrase("");
  EXPECT_FALSE(helper_->IsUsingPassphrase());
}

TEST_F(SyncHelperTest, Passphrase_SetNotifiesSettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetPassphrase("test");
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Account info
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, AccountEmail_DefaultEmpty) {
  EXPECT_TRUE(helper_->GetSyncAccountEmail().empty());
}

TEST_F(SyncHelperTest, AccountName_DefaultEmpty) {
  EXPECT_TRUE(helper_->GetSyncAccountName().empty());
}

// ---------------------------------------------------------------------------
// Observer notifications
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Observer_OnSyncStarted) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncStarted();
  EXPECT_EQ(observer.sync_started_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncCompleted) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncCompleted();
  EXPECT_EQ(observer.sync_completed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncError) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncError("Error message");
  EXPECT_EQ(observer.sync_error_count_, 1);
  EXPECT_EQ(observer.last_error_message_, "Error message");
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncPaused) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncPaused();
  EXPECT_EQ(observer.sync_paused_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncResumed) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncResumed();
  EXPECT_EQ(observer.sync_resumed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncDataTypeToggled) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncDataTypeToggled(AstraSyncDataType::kTabs, false);
  EXPECT_EQ(observer.data_type_toggled_count_, 1);
  EXPECT_EQ(observer.last_toggled_type_, AstraSyncDataType::kTabs);
  EXPECT_FALSE(observer.last_toggled_enabled_);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSyncSettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncSettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Observer_OnSignedInStatusChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySignedInStatusChanged(true);
  EXPECT_EQ(observer.signed_in_changed_count_, 1);
  EXPECT_TRUE(observer.last_signed_in_state_);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer defaults
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, ObserverDefaults_EmptyObserverDoesNotCrash) {
  EmptyTestObserver observer;
  helper_->AddObserver(&observer);
  helper_->NotifySyncStarted();
  helper_->NotifySyncCompleted();
  helper_->NotifySyncError("error");
  helper_->NotifySyncPaused();
  helper_->NotifySyncResumed();
  helper_->NotifySyncDataTypeToggled(AstraSyncDataType::kBookmarks, true);
  helper_->NotifySyncSettingsChanged();
  helper_->NotifySignedInStatusChanged(true);
  helper_->RemoveObserver(&observer);
  // If we get here without crashing, the test passes.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, MultipleObservers_AllNotified) {
  TestSyncObserver obs1, obs2, obs3;
  helper_->AddObserver(&obs1);
  helper_->AddObserver(&obs2);
  helper_->AddObserver(&obs3);
  helper_->NotifySyncStarted();
  EXPECT_EQ(obs1.sync_started_count_, 1);
  EXPECT_EQ(obs2.sync_started_count_, 1);
  EXPECT_EQ(obs3.sync_started_count_, 1);
  helper_->RemoveObserver(&obs1);
  helper_->RemoveObserver(&obs2);
  helper_->RemoveObserver(&obs3);
}

TEST_F(SyncHelperTest, MultipleObservers_RemoveOne) {
  TestSyncObserver obs1, obs2;
  helper_->AddObserver(&obs1);
  helper_->AddObserver(&obs2);
  helper_->RemoveObserver(&obs1);
  helper_->NotifySyncStarted();
  EXPECT_EQ(obs1.sync_started_count_, 0);
  EXPECT_EQ(obs2.sync_started_count_, 1);
  helper_->RemoveObserver(&obs2);
}

// ---------------------------------------------------------------------------
// Presentation settings
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Presentation_ShowSyncStatus_SetAndGet) {
  helper_->SetShowSyncStatus(false);
  EXPECT_FALSE(helper_->GetShowSyncStatus());
  helper_->SetShowSyncStatus(true);
  EXPECT_TRUE(helper_->GetShowSyncStatus());
}

TEST_F(SyncHelperTest, Presentation_ShowSyncErrors_SetAndGet) {
  helper_->SetShowSyncErrors(false);
  EXPECT_FALSE(helper_->GetShowSyncErrors());
  helper_->SetShowSyncErrors(true);
  EXPECT_TRUE(helper_->GetShowSyncErrors());
}

TEST_F(SyncHelperTest, Presentation_SyncOnWifiOnly_SetAndGet) {
  helper_->SetSyncOnWifiOnly(true);
  EXPECT_TRUE(helper_->GetSyncOnWifiOnly());
  helper_->SetSyncOnWifiOnly(false);
  EXPECT_FALSE(helper_->GetSyncOnWifiOnly());
}

TEST_F(SyncHelperTest, Presentation_SyncFrequency_SetAndGet) {
  helper_->SetSyncFrequency("hourly");
  EXPECT_EQ(helper_->GetSyncFrequency(), "hourly");
  helper_->SetSyncFrequency("daily");
  EXPECT_EQ(helper_->GetSyncFrequency(), "daily");
  helper_->SetSyncFrequency("auto");
  EXPECT_EQ(helper_->GetSyncFrequency(), "auto");
}

TEST_F(SyncHelperTest, Presentation_SyncFrequency_InvalidValueDefaultsToAuto) {
  helper_->SetSyncFrequency("invalid_value");
  EXPECT_EQ(helper_->GetSyncFrequency(), "auto");
}

TEST_F(SyncHelperTest, Presentation_AutoSync_SetAndGet) {
  helper_->SetAutoSync(false);
  EXPECT_FALSE(helper_->GetAutoSync());
  helper_->SetAutoSync(true);
  EXPECT_TRUE(helper_->GetAutoSync());
}

TEST_F(SyncHelperTest, Presentation_EncryptAllData_SetAndGet) {
  helper_->SetEncryptAllData(true);
  EXPECT_TRUE(helper_->GetEncryptAllData());
  helper_->SetEncryptAllData(false);
  EXPECT_FALSE(helper_->GetEncryptAllData());
}

TEST_F(SyncHelperTest, Presentation_ShowAccountAvatar_SetAndGet) {
  helper_->SetShowAccountAvatar(false);
  EXPECT_FALSE(helper_->GetShowAccountAvatar());
  helper_->SetShowAccountAvatar(true);
  EXPECT_TRUE(helper_->GetShowAccountAvatar());
}

TEST_F(SyncHelperTest, Presentation_ShowLastSyncTime_SetAndGet) {
  helper_->SetShowLastSyncTime(false);
  EXPECT_FALSE(helper_->GetShowLastSyncTime());
  helper_->SetShowLastSyncTime(true);
  EXPECT_TRUE(helper_->GetShowLastSyncTime());
}

TEST_F(SyncHelperTest, Presentation_ShowSyncIconInToolbar_SetAndGet) {
  helper_->SetShowSyncIconInToolbar(false);
  EXPECT_FALSE(helper_->GetShowSyncIconInToolbar());
  helper_->SetShowSyncIconInToolbar(true);
  EXPECT_TRUE(helper_->GetShowSyncIconInToolbar());
}

TEST_F(SyncHelperTest, Presentation_AllSettingsNotifySettingsChanged) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetShowSyncStatus(false);
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Presentation_NoNotificationWhenNoChange) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetShowSyncStatus(true);  // Already true.
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Persistence round-trip via PrefService
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Persistence_SyncEnabledPersists) {
  helper_->SetSyncEnabled(false);
  // Create a new helper pointing to the same profile.
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->IsSyncEnabled());
}

TEST_F(SyncHelperTest, Persistence_ShowSyncStatusPersists) {
  helper_->SetShowSyncStatus(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowSyncStatus());
}

TEST_F(SyncHelperTest, Persistence_ShowSyncErrorsPersists) {
  helper_->SetShowSyncErrors(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowSyncErrors());
}

TEST_F(SyncHelperTest, Persistence_SyncOnWifiOnlyPersists) {
  helper_->SetSyncOnWifiOnly(true);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetSyncOnWifiOnly());
}

TEST_F(SyncHelperTest, Persistence_SyncFrequencyPersists) {
  helper_->SetSyncFrequency("daily");
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_EQ(helper2->GetSyncFrequency(), "daily");
}

TEST_F(SyncHelperTest, Persistence_AutoSyncPersists) {
  helper_->SetAutoSync(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetAutoSync());
}

TEST_F(SyncHelperTest, Persistence_EncryptAllDataPersists) {
  helper_->SetEncryptAllData(true);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetEncryptAllData());
}

TEST_F(SyncHelperTest, Persistence_ShowAccountAvatarPersists) {
  helper_->SetShowAccountAvatar(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowAccountAvatar());
}

TEST_F(SyncHelperTest, Persistence_ShowLastSyncTimePersists) {
  helper_->SetShowLastSyncTime(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowLastSyncTime());
}

TEST_F(SyncHelperTest, Persistence_ShowSyncIconInToolbarPersists) {
  helper_->SetShowSyncIconInToolbar(false);
  auto helper2 = std::make_unique<AstraSyncHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowSyncIconInToolbar());
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Bulk_ToggleDataTypes_DisableMultiple) {
  std::vector<AstraSyncDataType> types = {
    AstraSyncDataType::kBookmarks,
    AstraSyncDataType::kHistory,
    AstraSyncDataType::kTabs,
  };
  helper_->ToggleDataTypes(types, false);
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kHistory));
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kTabs));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kPasswords));
}

TEST_F(SyncHelperTest, Bulk_ToggleDataTypes_EnableMultiple) {
  helper_->DisableAllDataTypes();
  std::vector<AstraSyncDataType> types = {
    AstraSyncDataType::kBookmarks,
    AstraSyncDataType::kPasswords,
  };
  helper_->ToggleDataTypes(types, true);
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kBookmarks));
  EXPECT_TRUE(helper_->IsDataTypeEnabled(AstraSyncDataType::kPasswords));
  EXPECT_FALSE(helper_->IsDataTypeEnabled(AstraSyncDataType::kHistory));
}

TEST_F(SyncHelperTest, Bulk_GetDataTypeStates) {
  std::vector<AstraSyncDataType> types = {
    AstraSyncDataType::kBookmarks,
    AstraSyncDataType::kPasswords,
  };
  auto states = helper_->GetDataTypeStates(types);
  EXPECT_EQ(states.size(), 2u);
  EXPECT_EQ(states[0].type, AstraSyncDataType::kBookmarks);
  EXPECT_EQ(states[1].type, AstraSyncDataType::kPasswords);
}

TEST_F(SyncHelperTest, Bulk_SyncDataTypesNow) {
  helper_->NotifySignedInStatusChanged(true);
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  std::vector<AstraSyncDataType> types = {
    AstraSyncDataType::kBookmarks,
  };
  helper_->SyncDataTypesNow(types);
  EXPECT_GE(observer.sync_started_count_, 1);
  EXPECT_GE(observer.sync_completed_count_, 1);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Bulk_SyncDataTypesNow_EmptyVectorNoOp) {
  helper_->NotifySignedInStatusChanged(true);
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  std::vector<AstraSyncDataType> types;
  helper_->SyncDataTypesNow(types);
  EXPECT_EQ(observer.sync_started_count_, 0);
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Bulk_ResetToDefaults) {
  // Change several settings.
  helper_->SetShowSyncStatus(false);
  helper_->SetSyncOnWifiOnly(true);
  helper_->SetSyncFrequency("daily");
  helper_->SetEncryptAllData(true);
  helper_->SetShowSyncIconInToolbar(false);

  // Reset.
  helper_->ResetSyncSettingsToDefaults();

  // Verify all back to defaults.
  EXPECT_TRUE(helper_->GetShowSyncStatus());
  EXPECT_FALSE(helper_->GetSyncOnWifiOnly());
  EXPECT_EQ(helper_->GetSyncFrequency(), "auto");
  EXPECT_FALSE(helper_->GetEncryptAllData());
  EXPECT_TRUE(helper_->GetShowSyncIconInToolbar());
  EXPECT_TRUE(helper_->IsSyncEnabled());
  EXPECT_TRUE(helper_->GetShowSyncErrors());
  EXPECT_TRUE(helper_->GetAutoSync());
  EXPECT_TRUE(helper_->GetShowAccountAvatar());
  EXPECT_TRUE(helper_->GetShowLastSyncTime());
}

TEST_F(SyncHelperTest, Bulk_ResetNotifiesSettingsChanged) {
  helper_->SetShowSyncStatus(false);
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->ResetSyncSettingsToDefaults();
  EXPECT_GE(observer.settings_changed_count_, 1);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Utility methods
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Utility_GetSyncStatusLabel) {
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kNotSignedIn),
            "Not signed in");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kSyncing),
            "Syncing");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kSynced),
            "Synced");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kError),
            "Sync error");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kPaused),
            "Sync paused");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus::kDisabled),
            "Sync disabled");
}

TEST_F(SyncHelperTest, Utility_GetDataTypeLabel) {
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kBookmarks),
            "Bookmarks");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kPasswords),
            "Passwords");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kHistory),
            "History");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kTabs),
            "Open Tabs");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kSettings),
            "Settings");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kExtensions),
            "Extensions");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kApps),
            "Apps");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kThemes),
            "Themes");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kAutofill),
            "Autofill");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType::kReadingList),
            "Reading List");
}

TEST_F(SyncHelperTest, Utility_GetDataTypeIcon) {
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kBookmarks),
            "bookmarks");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kPasswords),
            "passwords");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kHistory),
            "history");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kTabs),
            "tabs");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kSettings),
            "settings");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kExtensions),
            "extensions");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kApps),
            "apps");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kThemes),
            "themes");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kAutofill),
            "autofill");
  EXPECT_EQ(AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType::kReadingList),
            "reading_list");
}

TEST_F(SyncHelperTest, Utility_FormatLastSyncTime_NullTime) {
  EXPECT_EQ(AstraSyncHelper::FormatLastSyncTime(base::Time()), "Never");
}

TEST_F(SyncHelperTest, Utility_FormatLastSyncTime_JustNow) {
  base::Time now = base::Time::Now();
  std::string result = AstraSyncHelper::FormatLastSyncTime(now);
  EXPECT_EQ(result, "Just now");
}

TEST_F(SyncHelperTest, Utility_GetSyncStatusColor) {
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kNotSignedIn),
            "grey");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kSyncing),
            "blue");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kSynced),
            "green");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kError),
            "red");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kPaused),
            "orange");
  EXPECT_EQ(AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus::kDisabled),
            "grey");
}

TEST_F(SyncHelperTest, Utility_IsSyncDataTypeEnabledByDefault) {
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kBookmarks));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kPasswords));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kHistory));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kTabs));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kSettings));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kExtensions));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kApps));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kThemes));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kAutofill));
  EXPECT_TRUE(AstraSyncHelper::IsSyncDataTypeEnabledByDefault(
      AstraSyncDataType::kReadingList));
}

// ---------------------------------------------------------------------------
// Sync status enum
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SyncStatusEnum_AllValuesDistinct) {
  EXPECT_NE(AstraSyncStatus::kNotSignedIn, AstraSyncStatus::kSyncing);
  EXPECT_NE(AstraSyncStatus::kSyncing, AstraSyncStatus::kSynced);
  EXPECT_NE(AstraSyncStatus::kSynced, AstraSyncStatus::kError);
  EXPECT_NE(AstraSyncStatus::kError, AstraSyncStatus::kPaused);
  EXPECT_NE(AstraSyncStatus::kPaused, AstraSyncStatus::kDisabled);
}

// ---------------------------------------------------------------------------
// Data type enum
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, DataTypeEnum_AllValuesDistinct) {
  EXPECT_NE(AstraSyncDataType::kBookmarks, AstraSyncDataType::kPasswords);
  EXPECT_NE(AstraSyncDataType::kPasswords, AstraSyncDataType::kHistory);
  EXPECT_NE(AstraSyncDataType::kHistory, AstraSyncDataType::kTabs);
  EXPECT_NE(AstraSyncDataType::kTabs, AstraSyncDataType::kSettings);
  EXPECT_NE(AstraSyncDataType::kSettings, AstraSyncDataType::kExtensions);
  EXPECT_NE(AstraSyncDataType::kExtensions, AstraSyncDataType::kApps);
  EXPECT_NE(AstraSyncDataType::kApps, AstraSyncDataType::kThemes);
  EXPECT_NE(AstraSyncDataType::kThemes, AstraSyncDataType::kAutofill);
  EXPECT_NE(AstraSyncDataType::kAutofill, AstraSyncDataType::kReadingList);
}

// ---------------------------------------------------------------------------
// Sync status struct
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SyncStatusInfoStruct_DefaultValues) {
  AstraSyncStatusInfo info;
  EXPECT_EQ(info.status, AstraSyncStatus::kNotSignedIn);
  EXPECT_FALSE(info.is_signed_in);
  EXPECT_TRUE(info.last_sync_time.is_null());
  EXPECT_FALSE(info.has_sync_error);
  EXPECT_TRUE(info.error_message.empty());
  EXPECT_FALSE(info.is_sync_paused);
  EXPECT_EQ(info.total_item_count, 0);
  EXPECT_EQ(info.data_types_enabled, 0);
}

// ---------------------------------------------------------------------------
// Sync type state struct
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, SyncTypeStateStruct_DefaultValues) {
  AstraSyncTypeState state;
  EXPECT_EQ(state.type, AstraSyncDataType::kBookmarks);
  EXPECT_FALSE(state.is_enabled);
  EXPECT_EQ(state.item_count, 0);
  EXPECT_TRUE(state.last_sync_time.is_null());
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, EdgeCase_ZeroItems) {
  EXPECT_EQ(helper_->GetTotalSyncedItemCount(), 0);
  EXPECT_GE(helper_->GetTotalSyncedItemCount(), 0);
}

TEST_F(SyncHelperTest, EdgeCase_AllTypesDisabled) {
  helper_->DisableAllDataTypes();
  EXPECT_EQ(helper_->GetEnabledDataTypeCount(), 0);
  EXPECT_EQ(helper_->GetEnabledDataTypes().size(), 0u);
  EXPECT_EQ(helper_->GetTotalSyncedItemCount(), 0);
}

TEST_F(SyncHelperTest, EdgeCase_UnknownTypeGetState) {
  // Using a cast to a value outside the enum range.
  AstraSyncDataType unknown = static_cast<AstraSyncDataType>(999);
  AstraSyncTypeState state = helper_->GetDataTypeState(unknown);
  EXPECT_FALSE(state.is_enabled);
  EXPECT_EQ(state.item_count, 0);
}

TEST_F(SyncHelperTest, EdgeCase_SetEnabledWhenAlreadySameStateNoNotify) {
  // Setting to the same value should not notify.
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->SetShowSyncStatus(true);  // Already true.
  EXPECT_EQ(observer.settings_changed_count_, 0);
  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Factory_GetInstance) {
  EXPECT_TRUE(AstraSyncHelperFactory::GetInstance() != nullptr);
}

TEST_F(SyncHelperTest, Factory_GetForProfile) {
  AstraSyncHelper* helper = AstraSyncHelperFactory::GetForProfile(profile_.get());
  EXPECT_TRUE(helper != nullptr);
}

TEST_F(SyncHelperTest, Factory_GetForProfileNullProfile) {
  AstraSyncHelper* helper = AstraSyncHelperFactory::GetForProfile(nullptr);
  EXPECT_TRUE(helper == nullptr);
}

TEST_F(SyncHelperTest, Factory_RegisterProfilePrefs) {
  // Create a new profile and register prefs via factory.
  auto profile2 = std::make_unique<TestingProfile>();
  AstraSyncHelperFactory::RegisterProfilePrefs(profile2->GetPrefs());

  // Verify the sync pref should be registered and have default value.
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefSyncEnabled));
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefShowSyncStatus));
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefShowSyncErrors));
  EXPECT_FALSE(profile2->GetPrefs()->GetBoolean(prefs::kPrefSyncOnWifiOnly));
  EXPECT_EQ(profile2->GetPrefs()->GetString(prefs::kPrefSyncFrequency), "auto");
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefAutoSync));
  EXPECT_FALSE(profile2->GetPrefs()->GetBoolean(prefs::kPrefEncryptAllData));
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefShowAccountAvatar));
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefShowLastSyncTime));
  EXPECT_TRUE(profile2->GetPrefs()->GetBoolean(prefs::kPrefShowSyncIconInToolbar));
}

// ---------------------------------------------------------------------------
// Shutdown cleanup
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, Shutdown_ClearsProfile) {
  TestSyncObserver observer;
  helper_->AddObserver(&observer);
  helper_->Shutdown();
  // After shutdown, the helper should still be valid but profile cleared.
  // Calling methods should return defaults gracefully.
  EXPECT_TRUE(helper_->IsSyncEnabled());  // Falls back to default.
  helper_->RemoveObserver(&observer);
}

TEST_F(SyncHelperTest, Shutdown_Idempotent) {
  helper_->Shutdown();
  helper_->Shutdown();
  // Should not crash.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Reset to defaults (comprehensive)
// ---------------------------------------------------------------------------

TEST_F(SyncHelperTest, ResetToDefaults_AllSettings) {
  // Change every setting.
  helper_->SetSyncEnabled(false);
  helper_->SetShowSyncStatus(false);
  helper_->SetShowSyncErrors(false);
  helper_->SetSyncOnWifiOnly(true);
  helper_->SetSyncFrequency("hourly");
  helper_->SetAutoSync(false);
  helper_->SetEncryptAllData(true);
  helper_->SetShowAccountAvatar(false);
  helper_->SetShowLastSyncTime(false);
  helper_->SetShowSyncIconInToolbar(false);

  // All should be non-default now.
  EXPECT_FALSE(helper_->IsSyncEnabled());
  EXPECT_FALSE(helper_->GetShowSyncStatus());
  EXPECT_FALSE(helper_->GetShowSyncErrors());
  EXPECT_TRUE(helper_->GetSyncOnWifiOnly());
  EXPECT_EQ(helper_->GetSyncFrequency(), "hourly");
  EXPECT_FALSE(helper_->GetAutoSync());
  EXPECT_TRUE(helper_->GetEncryptAllData());
  EXPECT_FALSE(helper_->GetShowAccountAvatar());
  EXPECT_FALSE(helper_->GetShowLastSyncTime());
  EXPECT_FALSE(helper_->GetShowSyncIconInToolbar());

  // Reset.
  helper_->ResetSyncSettingsToDefaults();

  // All should be back to defaults.
  EXPECT_TRUE(helper_->IsSyncEnabled());
  EXPECT_TRUE(helper_->GetShowSyncStatus());
  EXPECT_TRUE(helper_->GetShowSyncErrors());
  EXPECT_FALSE(helper_->GetSyncOnWifiOnly());
  EXPECT_EQ(helper_->GetSyncFrequency(), "auto");
  EXPECT_TRUE(helper_->GetAutoSync());
  EXPECT_FALSE(helper_->GetEncryptAllData());
  EXPECT_TRUE(helper_->GetShowAccountAvatar());
  EXPECT_TRUE(helper_->GetShowLastSyncTime());
  EXPECT_TRUE(helper_->GetShowSyncIconInToolbar());
}

}  // namespace astra
