// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_sync_helper.h"

#include <algorithm>
#include <string>

#include "base/i18n/time_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout. In this overlay repo, the
// types are forward-declared in the header and the real definitions
// come from Chromium at build time.
//
// Chromium owner: SyncService
//   (components/sync/service/sync_service.h)
// Chromium owner: ProfileSyncServiceFactory
//   (chrome/browser/sync/profile_sync_service_factory.h)
// #include "chrome/browser/sync/profile_sync_service_factory.h"
// #include "components/sync/service/sync_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Number of data types supported by the helper.
constexpr size_t kDataTypeCount = 10;

// Valid sync frequency values.
constexpr char kSyncFrequencyAuto[] = "auto";
constexpr char kSyncFrequencyHourly[] = "hourly";
constexpr char kSyncFrequencyDaily[] = "daily";

// Clamps an item count to valid range (0 to max int).
int ClampItemCount(int count) {
  if (count < 0) return 0;
  return count;
}

// Validates a sync frequency string, returning "auto" if invalid.
std::string ValidateSyncFrequency(const std::string& frequency) {
  if (frequency == kSyncFrequencyAuto ||
      frequency == kSyncFrequencyHourly ||
      frequency == kSyncFrequencyDaily) {
    return frequency;
  }
  return kSyncFrequencyAuto;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSyncHelper::AstraSyncHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing SyncService for live updates.
  //   Requires implementing syncer::SyncServiceObserver.
  //
  // Chromium observer: SyncServiceObserver
  //   (components/sync/service/sync_service_observer.h)

  InitializeTypeStates();

  // Initialize presentation state from prefs.
  PrefService* prefs = GetPrefs();
  if (prefs) {
    sync_enabled_ = prefs->GetBoolean(prefs::kPrefSyncEnabled);
  }
}

AstraSyncHelper::~AstraSyncHelper() = default;

void AstraSyncHelper::Shutdown() {
  // TODO(astra): Remove observer from SyncService.
  //   syncer::SyncService* sync_service = GetSyncService();
  //   if (sync_service && is_observing_) {
  //     sync_service->RemoveObserver(this);
  //     is_observing_ = false;
  //   }
  profile_ = nullptr;
}

// =========================================================================
// Type state initialization
// =========================================================================

void AstraSyncHelper::InitializeTypeStates() const {
  type_states_.clear();
  type_states_.reserve(kDataTypeCount);

  AstraSyncDataType types[] = {
    AstraSyncDataType::kBookmarks,
    AstraSyncDataType::kPasswords,
    AstraSyncDataType::kHistory,
    AstraSyncDataType::kTabs,
    AstraSyncDataType::kSettings,
    AstraSyncDataType::kExtensions,
    AstraSyncDataType::kApps,
    AstraSyncDataType::kThemes,
    AstraSyncDataType::kAutofill,
    AstraSyncDataType::kReadingList,
  };

  for (auto type : types) {
    AstraSyncTypeState state;
    state.type = type;
    state.is_enabled = IsSyncDataTypeEnabledByDefault(type);
    state.item_count = 0;
    state.last_sync_time = base::Time();
    type_states_.push_back(state);
  }
}

// =========================================================================
// Sync status queries
// =========================================================================

AstraSyncStatus AstraSyncHelper::GetSyncStatus() const {
  // TODO(astra): Query real status from SyncService.
  //   Chromium API: SyncService::GetTransportState()
  //   (components/sync/service/sync_service.h)

  if (!is_signed_in_) {
    return AstraSyncStatus::kNotSignedIn;
  }
  if (!sync_enabled_) {
    return AstraSyncStatus::kDisabled;
  }
  if (has_sync_error_) {
    return AstraSyncStatus::kError;
  }
  if (is_sync_paused_) {
    return AstraSyncStatus::kPaused;
  }
  if (cached_status_ == AstraSyncStatus::kSyncing) {
    return AstraSyncStatus::kSyncing;
  }
  return AstraSyncStatus::kSynced;
}

bool AstraSyncHelper::IsSyncEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSyncEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefSyncEnabled);
}

void AstraSyncHelper::SetSyncEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefSyncEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSyncEnabled, enabled);
  sync_enabled_ = enabled;
  NotifySyncSettingsChanged();

  // If signing out or disabling sync, update status.
  if (!enabled) {
    cached_status_ = AstraSyncStatus::kDisabled;
  }
}

bool AstraSyncHelper::IsSignedIn() const {
  // TODO(astra): Query real sign-in state from IdentityManager.
  //   Chromium API: IdentityManager::HasPrimaryAccount()
  //   (services/identity/public/cpp/identity_manager.h)
  return is_signed_in_;
}

base::Time AstraSyncHelper::GetLastSyncTime() const {
  // TODO(astra): Query real last sync time from SyncService.
  //   Chromium API: SyncService::GetLastSyncedTime()
  return last_sync_time_;
}

int AstraSyncHelper::GetTotalSyncedItemCount() const {
  // TODO(astra): Calculate from real SyncService data.
  int total = 0;
  for (const auto& state : type_states_) {
    if (state.is_enabled) {
      total += state.item_count;
    }
  }
  return ClampItemCount(total);
}

int AstraSyncHelper::GetEnabledDataTypeCount() const {
  int count = 0;
  for (const auto& state : type_states_) {
    if (state.is_enabled) {
      count++;
    }
  }
  return count;
}

// =========================================================================
// Sync errors
// =========================================================================

std::string AstraSyncHelper::GetSyncError() const {
  return error_message_;
}

bool AstraSyncHelper::HasError() const {
  return has_sync_error_;
}

void AstraSyncHelper::ClearError() {
  if (!has_sync_error_) {
    return;
  }

  has_sync_error_ = false;
  error_message_.clear();
  NotifySyncSettingsChanged();
}

// =========================================================================
// Sync pause / resume
// =========================================================================

bool AstraSyncHelper::IsSyncPaused() const {
  return is_sync_paused_;
}

void AstraSyncHelper::PauseSync() {
  if (is_sync_paused_) {
    return;
  }

  is_sync_paused_ = true;
  NotifySyncPaused();
}

void AstraSyncHelper::ResumeSync() {
  if (!is_sync_paused_) {
    return;
  }

  is_sync_paused_ = false;
  NotifySyncResumed();
}

// =========================================================================
// Data type operations
// =========================================================================

AstraSyncTypeState AstraSyncHelper::GetDataTypeState(
    AstraSyncDataType type) const {
  for (const auto& state : type_states_) {
    if (state.type == type) {
      return state;
    }
  }
  // Return a default state for unknown types.
  AstraSyncTypeState default_state;
  default_state.type = type;
  default_state.is_enabled = false;
  default_state.item_count = 0;
  return default_state;
}

bool AstraSyncHelper::IsDataTypeEnabled(AstraSyncDataType type) const {
  return GetDataTypeState(type).is_enabled;
}

void AstraSyncHelper::SetDataTypeEnabled(AstraSyncDataType type,
                                         bool enabled) {
  for (auto& state : type_states_) {
    if (state.type == type) {
      if (state.is_enabled == enabled) {
        return;
      }
      state.is_enabled = enabled;
      NotifySyncDataTypeToggled(type, enabled);
      NotifySyncSettingsChanged();
      return;
    }
  }
}

std::vector<AstraSyncTypeState> AstraSyncHelper::GetAllDataTypeStates() const {
  return type_states_;
}

std::vector<AstraSyncDataType> AstraSyncHelper::GetEnabledDataTypes() const {
  std::vector<AstraSyncDataType> result;
  for (const auto& state : type_states_) {
    if (state.is_enabled) {
      result.push_back(state.type);
    }
  }
  return result;
}

void AstraSyncHelper::EnableAllDataTypes() {
  bool any_changed = false;
  for (auto& state : type_states_) {
    if (!state.is_enabled) {
      state.is_enabled = true;
      any_changed = true;
    }
  }
  if (any_changed) {
    NotifySyncSettingsChanged();
  }
}

void AstraSyncHelper::DisableAllDataTypes() {
  bool any_changed = false;
  for (auto& state : type_states_) {
    if (state.is_enabled) {
      state.is_enabled = false;
      any_changed = true;
    }
  }
  if (any_changed) {
    NotifySyncSettingsChanged();
  }
}

// =========================================================================
// Sync operations
// =========================================================================

void AstraSyncHelper::RequestSyncNow() {
  // TODO(astra): Trigger real sync via SyncService.
  //   Chromium API: SyncService::TriggerRefresh()
  if (!sync_enabled_ || is_sync_paused_) {
    return;
  }

  cached_status_ = AstraSyncStatus::kSyncing;
  NotifySyncStarted();

  // In the overlay, simulate sync completion.
  // TODO(astra): Remove when real SyncService integration is done.
  cached_status_ = AstraSyncStatus::kSynced;
  last_sync_time_ = base::Time::Now();
  NotifySyncCompleted();
}

// =========================================================================
// Account info
// =========================================================================

std::string AstraSyncHelper::GetSyncAccountEmail() const {
  // TODO(astra): Query real account email from IdentityManager.
  return account_email_;
}

std::string AstraSyncHelper::GetSyncAccountName() const {
  // TODO(astra): Query real account name from IdentityManager.
  return account_name_;
}

// =========================================================================
// Passphrase
// =========================================================================

bool AstraSyncHelper::IsUsingPassphrase() const {
  // TODO(astra): Query real passphrase state from SyncService.
  //   Chromium API: SyncService::IsUsingExplicitPassphrase()
  return using_passphrase_;
}

void AstraSyncHelper::SetPassphrase(const std::string& passphrase) {
  // TODO(astra): Set real passphrase via SyncService.
  //   Chromium API: SyncService::SetEncryptionPassphrase()
  if (passphrase.empty()) {
    return;
  }
  using_passphrase_ = true;
  NotifySyncSettingsChanged();
}

// =========================================================================
// Presentation settings
// =========================================================================

bool AstraSyncHelper::GetShowSyncStatus() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSyncStatus;
  }
  return prefs->GetBoolean(prefs::kPrefShowSyncStatus);
}

void AstraSyncHelper::SetShowSyncStatus(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowSyncStatus) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowSyncStatus, show);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetShowSyncErrors() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSyncErrors;
  }
  return prefs->GetBoolean(prefs::kPrefShowSyncErrors);
}

void AstraSyncHelper::SetShowSyncErrors(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowSyncErrors) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowSyncErrors, show);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetSyncOnWifiOnly() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSyncOnWifiOnly;
  }
  return prefs->GetBoolean(prefs::kPrefSyncOnWifiOnly);
}

void AstraSyncHelper::SetSyncOnWifiOnly(bool wifi_only) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefSyncOnWifiOnly) == wifi_only) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefSyncOnWifiOnly, wifi_only);
  NotifySyncSettingsChanged();
}

std::string AstraSyncHelper::GetSyncFrequency() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultSyncFrequency;
  }
  return prefs->GetString(prefs::kPrefSyncFrequency);
}

void AstraSyncHelper::SetSyncFrequency(const std::string& frequency) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  std::string validated = ValidateSyncFrequency(frequency);
  if (prefs->GetString(prefs::kPrefSyncFrequency) == validated) {
    return;
  }
  prefs->SetString(prefs::kPrefSyncFrequency, validated);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetAutoSync() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutoSync;
  }
  return prefs->GetBoolean(prefs::kPrefAutoSync);
}

void AstraSyncHelper::SetAutoSync(bool auto_sync) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefAutoSync) == auto_sync) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefAutoSync, auto_sync);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetEncryptAllData() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultEncryptAllData;
  }
  return prefs->GetBoolean(prefs::kPrefEncryptAllData);
}

void AstraSyncHelper::SetEncryptAllData(bool encrypt) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefEncryptAllData) == encrypt) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefEncryptAllData, encrypt);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetShowAccountAvatar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowAccountAvatar;
  }
  return prefs->GetBoolean(prefs::kPrefShowAccountAvatar);
}

void AstraSyncHelper::SetShowAccountAvatar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowAccountAvatar) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowAccountAvatar, show);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetShowLastSyncTime() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowLastSyncTime;
  }
  return prefs->GetBoolean(prefs::kPrefShowLastSyncTime);
}

void AstraSyncHelper::SetShowLastSyncTime(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowLastSyncTime) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowLastSyncTime, show);
  NotifySyncSettingsChanged();
}

bool AstraSyncHelper::GetShowSyncIconInToolbar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSyncIconInToolbar;
  }
  return prefs->GetBoolean(prefs::kPrefShowSyncIconInToolbar);
}

void AstraSyncHelper::SetShowSyncIconInToolbar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  if (prefs->GetBoolean(prefs::kPrefShowSyncIconInToolbar) == show) {
    return;
  }
  prefs->SetBoolean(prefs::kPrefShowSyncIconInToolbar, show);
  NotifySyncSettingsChanged();
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraSyncHelper::ToggleDataTypes(
    const std::vector<AstraSyncDataType>& types,
    bool enabled) {
  bool any_changed = false;
  for (auto type : types) {
    for (auto& state : type_states_) {
      if (state.type == type && state.is_enabled != enabled) {
        state.is_enabled = enabled;
        any_changed = true;
        break;
      }
    }
  }
  if (any_changed) {
    NotifySyncSettingsChanged();
  }
}

std::vector<AstraSyncTypeState> AstraSyncHelper::GetDataTypeStates(
    const std::vector<AstraSyncDataType>& types) const {
  std::vector<AstraSyncTypeState> result;
  result.reserve(types.size());
  for (auto type : types) {
    result.push_back(GetDataTypeState(type));
  }
  return result;
}

void AstraSyncHelper::SyncDataTypesNow(
    const std::vector<AstraSyncDataType>& types) {
  // TODO(astra): Trigger sync for specific types via SyncService.
  if (!sync_enabled_ || is_sync_paused_ || types.empty()) {
    return;
  }

  cached_status_ = AstraSyncStatus::kSyncing;
  NotifySyncStarted();

  // In the overlay, simulate sync completion.
  cached_status_ = AstraSyncStatus::kSynced;
  last_sync_time_ = base::Time::Now();

  // Update last sync time for specified types.
  for (auto type : types) {
    for (auto& state : type_states_) {
      if (state.type == type) {
        state.last_sync_time = base::Time::Now();
        break;
      }
    }
  }

  NotifySyncCompleted();
}

void AstraSyncHelper::ResetSyncSettingsToDefaults() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefSyncEnabled, prefs::kDefaultSyncEnabled);
  prefs->SetBoolean(prefs::kPrefShowSyncStatus, prefs::kDefaultShowSyncStatus);
  prefs->SetBoolean(prefs::kPrefShowSyncErrors, prefs::kDefaultShowSyncErrors);
  prefs->SetBoolean(prefs::kPrefSyncOnWifiOnly, prefs::kDefaultSyncOnWifiOnly);
  prefs->SetString(prefs::kPrefSyncFrequency, prefs::kDefaultSyncFrequency);
  prefs->SetBoolean(prefs::kPrefAutoSync, prefs::kDefaultAutoSync);
  prefs->SetBoolean(prefs::kPrefEncryptAllData, prefs::kDefaultEncryptAllData);
  prefs->SetBoolean(prefs::kPrefShowAccountAvatar,
                    prefs::kDefaultShowAccountAvatar);
  prefs->SetBoolean(prefs::kPrefShowLastSyncTime,
                    prefs::kDefaultShowLastSyncTime);
  prefs->SetBoolean(prefs::kPrefShowSyncIconInToolbar,
                    prefs::kDefaultShowSyncIconInToolbar);

  sync_enabled_ = prefs::kDefaultSyncEnabled;

  NotifySyncSettingsChanged();
}

// =========================================================================
// Utility methods
// =========================================================================

// static
std::string AstraSyncHelper::GetSyncStatusLabel(AstraSyncStatus status) {
  switch (status) {
    case AstraSyncStatus::kNotSignedIn:
      return "Not signed in";
    case AstraSyncStatus::kSyncing:
      return "Syncing";
    case AstraSyncStatus::kSynced:
      return "Synced";
    case AstraSyncStatus::kError:
      return "Sync error";
    case AstraSyncStatus::kPaused:
      return "Sync paused";
    case AstraSyncStatus::kDisabled:
      return "Sync disabled";
  }
  return "";
}

// static
std::string AstraSyncHelper::GetDataTypeLabel(AstraSyncDataType type) {
  switch (type) {
    case AstraSyncDataType::kBookmarks:
      return "Bookmarks";
    case AstraSyncDataType::kPasswords:
      return "Passwords";
    case AstraSyncDataType::kHistory:
      return "History";
    case AstraSyncDataType::kTabs:
      return "Open Tabs";
    case AstraSyncDataType::kSettings:
      return "Settings";
    case AstraSyncDataType::kExtensions:
      return "Extensions";
    case AstraSyncDataType::kApps:
      return "Apps";
    case AstraSyncDataType::kThemes:
      return "Themes";
    case AstraSyncDataType::kAutofill:
      return "Autofill";
    case AstraSyncDataType::kReadingList:
      return "Reading List";
  }
  return "";
}

// static
std::string AstraSyncHelper::GetDataTypeIcon(AstraSyncDataType type) {
  switch (type) {
    case AstraSyncDataType::kBookmarks:
      return "bookmarks";
    case AstraSyncDataType::kPasswords:
      return "passwords";
    case AstraSyncDataType::kHistory:
      return "history";
    case AstraSyncDataType::kTabs:
      return "tabs";
    case AstraSyncDataType::kSettings:
      return "settings";
    case AstraSyncDataType::kExtensions:
      return "extensions";
    case AstraSyncDataType::kApps:
      return "apps";
    case AstraSyncDataType::kThemes:
      return "themes";
    case AstraSyncDataType::kAutofill:
      return "autofill";
    case AstraSyncDataType::kReadingList:
      return "reading_list";
  }
  return "";
}

// static
std::string AstraSyncHelper::FormatLastSyncTime(base::Time time) {
  if (time.is_null()) {
    return "Never";
  }

  base::Time now = base::Time::Now();
  base::TimeDelta delta = now - time;

  if (delta < base::Minutes(1)) {
    return "Just now";
  }
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    return base::StringPrintf("%d minute%s ago",
                              minutes,
                              minutes == 1 ? "" : "s");
  }
  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    return base::StringPrintf("%d hour%s ago",
                              hours,
                              hours == 1 ? "" : "s");
  }
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    return base::StringPrintf("%d day%s ago",
                              days,
                              days == 1 ? "" : "s");
  }

  // For older times, return formatted date.
  return base::UnlocalizedTimeFormatWithPattern(time, "MMM d, yyyy");
}

// static
std::string AstraSyncHelper::GetSyncStatusColor(AstraSyncStatus status) {
  switch (status) {
    case AstraSyncStatus::kNotSignedIn:
      return "grey";
    case AstraSyncStatus::kSyncing:
      return "blue";
    case AstraSyncStatus::kSynced:
      return "green";
    case AstraSyncStatus::kError:
      return "red";
    case AstraSyncStatus::kPaused:
      return "orange";
    case AstraSyncStatus::kDisabled:
      return "grey";
  }
  return "grey";
}

// static
bool AstraSyncHelper::IsSyncDataTypeEnabledByDefault(AstraSyncDataType type) {
  switch (type) {
    case AstraSyncDataType::kBookmarks:
      return true;
    case AstraSyncDataType::kPasswords:
      return true;
    case AstraSyncDataType::kHistory:
      return true;
    case AstraSyncDataType::kTabs:
      return true;
    case AstraSyncDataType::kSettings:
      return true;
    case AstraSyncDataType::kExtensions:
      return true;
    case AstraSyncDataType::kApps:
      return true;
    case AstraSyncDataType::kThemes:
      return true;
    case AstraSyncDataType::kAutofill:
      return true;
    case AstraSyncDataType::kReadingList:
      return true;
  }
  return false;
}

// =========================================================================
// Observers
// =========================================================================

void AstraSyncHelper::AddObserver(AstraSyncObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSyncHelper::RemoveObserver(AstraSyncObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraSyncHelper::NotifySyncStarted() {
  for (auto& observer : observers_) {
    observer.OnSyncStarted();
  }
}

void AstraSyncHelper::NotifySyncCompleted() {
  for (auto& observer : observers_) {
    observer.OnSyncCompleted();
  }
}

void AstraSyncHelper::NotifySyncError(const std::string& error_message) {
  has_sync_error_ = true;
  error_message_ = error_message;
  for (auto& observer : observers_) {
    observer.OnSyncError(error_message);
  }
}

void AstraSyncHelper::NotifySyncPaused() {
  for (auto& observer : observers_) {
    observer.OnSyncPaused();
  }
}

void AstraSyncHelper::NotifySyncResumed() {
  for (auto& observer : observers_) {
    observer.OnSyncResumed();
  }
}

void AstraSyncHelper::NotifySyncDataTypeToggled(AstraSyncDataType type,
                                                bool enabled) {
  for (auto& observer : observers_) {
    observer.OnSyncDataTypeToggled(type, enabled);
  }
}

void AstraSyncHelper::NotifySyncSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnSyncSettingsChanged();
  }
}

void AstraSyncHelper::NotifySignedInStatusChanged(bool is_signed_in) {
  is_signed_in_ = is_signed_in;
  for (auto& observer : observers_) {
    observer.OnSignedInStatusChanged(is_signed_in);
  }
}

// =========================================================================
// Pref access
// =========================================================================

PrefService* AstraSyncHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

}  // namespace astra
