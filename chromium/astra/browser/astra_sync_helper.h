#ifndef ASTRA_BROWSER_ASTRA_SYNC_HELPER_H_
#define ASTRA_BROWSER_ASTRA_SYNC_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;
class Profile;

namespace astra {

// =========================================================================
// AstraSyncStatus — overall sync status
// =========================================================================
//
// Enumeration of possible sync states for UI projection.
//
// This is a projected value — the truth source is Chromium's SyncService.
// Astra only reflects the state, never owns it.
//
// Chromium owner: SyncService (components/sync/service/sync_service.h)
enum class AstraSyncStatus {
  kNotSignedIn,  // User not signed in
  kSyncing,      // Currently syncing
  kSynced,       // Everything synced
  kError,        // Sync error
  kPaused,       // Sync paused
  kDisabled,     // Sync disabled
};

// =========================================================================
// AstraSyncDataType — sync data type enumeration
// =========================================================================
//
// Data types that can be synced across devices.
//
// These are projected from Chromium's ModelType enum. The Astra helper
// only exposes a subset of types relevant to the Astra UI.
//
// Chromium owner: syncer::ModelType (components/sync/base/model_type.h)
enum class AstraSyncDataType {
  kBookmarks,    // Bookmarks
  kPasswords,    // Passwords
  kHistory,      // History
  kTabs,         // Open tabs
  kSettings,     // Settings
  kExtensions,   // Extensions
  kApps,         // Apps
  kThemes,       // Themes
  kAutofill,     // Autofill
  kReadingList,  // Reading list
};

// =========================================================================
// AstraSyncTypeState — per-type sync state
// =========================================================================
//
// Projection of the sync state for a single data type.
//
// This is a presentation-only struct — it mirrors a subset of
// Chromium's DataTypeState for UI display.
struct AstraSyncTypeState {
  // The data type.
  AstraSyncDataType type = AstraSyncDataType::kBookmarks;

  // Whether this data type is enabled for syncing.
  bool is_enabled = false;

  // Number of items of this type.
  int item_count = 0;

  // Last time this type was synced.
  base::Time last_sync_time;
};

// =========================================================================
// AstraSyncStatusInfo — overall sync status information
// =========================================================================
//
// Aggregated sync status information for UI display.
//
// This is a projected snapshot of sync state. The UI should re-query
// when notified of changes rather than caching this struct.
struct AstraSyncStatusInfo {
  // Overall sync status.
  AstraSyncStatus status = AstraSyncStatus::kNotSignedIn;

  // Whether the user is signed in.
  bool is_signed_in = false;

  // Last successful sync time.
  base::Time last_sync_time;

  // Whether there is a sync error.
  bool has_sync_error = false;

  // Error message description (empty if no error).
  std::string error_message;

  // Whether sync is currently paused.
  bool is_sync_paused = false;

  // Total number of synced items across all types.
  int total_item_count = 0;

  // Number of enabled data types.
  int data_types_enabled = 0;
};

// =========================================================================
// AstraSyncObserver — observer interface
// =========================================================================
//
// Observer interface for AstraSyncHelper. Notifies when sync state
// changes or when sync presentation settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh sync-related UI
// surfaces when sync state changes.
class AstraSyncObserver : public base::CheckedObserver {
 public:
  // Called when a sync cycle starts.
  virtual void OnSyncStarted() {}

  // Called when a sync cycle completes successfully.
  virtual void OnSyncCompleted() {}

  // Called when a sync error occurs.
  // |error_message| describes the error.
  virtual void OnSyncError(const std::string& error_message) {}

  // Called when sync is paused.
  virtual void OnSyncPaused() {}

  // Called when sync is resumed after being paused.
  virtual void OnSyncResumed() {}

  // Called when a data type is toggled on or off.
  // |type| is the data type that changed.
  // |enabled| is the new state.
  virtual void OnSyncDataTypeToggled(AstraSyncDataType type, bool enabled) {}

  // Called when sync presentation settings change.
  virtual void OnSyncSettingsChanged() {}

  // Called when the user's signed-in status changes.
  // |is_signed_in| is the new sign-in state.
  virtual void OnSignedInStatusChanged(bool is_signed_in) {}

 protected:
  ~AstraSyncObserver() override = default;
};

// =========================================================================
// AstraSyncHelper — sync projection helper
// =========================================================================
//
// Helper class for sync-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// SyncService APIs. It provides a clean interface for the Astra UI layer
// to query sync state and perform sync operations without directly
// depending on the full sync subsystem.
//
// Truth source: Chromium's SyncService
//   (components/sync/service/sync_service.h)
//
// Projection principles:
//   - Sync state is owned by Chromium's SyncService.
//   - Astra only projects the state for UI display.
//   - Astra-specific presentation preferences persist via PrefService.
//   - Operations like "toggle sync" delegate to Chromium's SyncService.
//
// TODO(astra): Proper SyncServiceObserver integration.
//   Currently this helper does not observe the sync service for live
//   updates. To get reactive updates, we need to implement
//   syncer::SyncServiceObserver.
// Chromium observer: SyncServiceObserver
//   (components/sync/service/sync_service_observer.h)
class AstraSyncHelper : public KeyedService {
 public:
  explicit AstraSyncHelper(Profile* profile);
  AstraSyncHelper(const AstraSyncHelper&) = delete;
  AstraSyncHelper& operator=(const AstraSyncHelper&) = delete;
  ~AstraSyncHelper() override;

  // -- Sync status queries ------------------------------------------------

  // Returns the overall sync status.
  AstraSyncStatus GetSyncStatus() const;

  // Returns whether sync is enabled.
  bool IsSyncEnabled() const;

  // Sets whether sync is enabled.
  // Fires OnSyncSettingsChanged observer notification if state changes.
  void SetSyncEnabled(bool enabled);

  // Returns whether the user is signed in.
  bool IsSignedIn() const;

  // Returns the last successful sync time.
  base::Time GetLastSyncTime() const;

  // Returns the total number of synced items across all types.
  int GetTotalSyncedItemCount() const;

  // Returns the number of enabled data types.
  int GetEnabledDataTypeCount() const;

  // -- Sync errors --------------------------------------------------------

  // Returns the sync error message (empty string if no error).
  std::string GetSyncError() const;

  // Returns whether there is a sync error.
  bool HasError() const;

  // Clears the sync error state.
  // Fires OnSyncSettingsChanged observer notification.
  void ClearError();

  // -- Sync pause / resume ------------------------------------------------

  // Returns whether sync is currently paused.
  bool IsSyncPaused() const;

  // Pauses sync.
  // Fires OnSyncPaused observer notification if state changes.
  void PauseSync();

  // Resumes sync.
  // Fires OnSyncResumed observer notification if state changes.
  void ResumeSync();

  // -- Data type operations ----------------------------------------------

  // Returns the state for a specific data type.
  AstraSyncTypeState GetDataTypeState(AstraSyncDataType type) const;

  // Returns whether a specific data type is enabled.
  bool IsDataTypeEnabled(AstraSyncDataType type) const;

  // Sets whether a specific data type is enabled.
  // Fires OnSyncDataTypeToggled observer notification if state changes.
  void SetDataTypeEnabled(AstraSyncDataType type, bool enabled);

  // Returns states for all data types.
  std::vector<AstraSyncTypeState> GetAllDataTypeStates() const;

  // Returns only the enabled data types.
  std::vector<AstraSyncDataType> GetEnabledDataTypes() const;

  // Enables all data types.
  // Fires OnSyncSettingsChanged observer notification.
  void EnableAllDataTypes();

  // Disables all data types.
  // Fires OnSyncSettingsChanged observer notification.
  void DisableAllDataTypes();

  // -- Sync operations ---------------------------------------------------

  // Requests an immediate sync.
  // Fires OnSyncStarted observer notification.
  void RequestSyncNow();

  // -- Account info ------------------------------------------------------

  // Returns the sync account email.
  std::string GetSyncAccountEmail() const;

  // Returns the sync account display name.
  std::string GetSyncAccountName() const;

  // -- Passphrase --------------------------------------------------------

  // Returns whether a custom passphrase is being used.
  bool IsUsingPassphrase() const;

  // Sets a custom passphrase (stub).
  // TODO(astra): Wire to real passphrase setting via SyncService.
  void SetPassphrase(const std::string& passphrase);

  // -- Presentation settings ---------------------------------------------

  // Returns whether sync status is shown in UI.
  bool GetShowSyncStatus() const;

  // Sets whether sync status is shown in UI.
  void SetShowSyncStatus(bool show);

  // Returns whether sync errors are shown.
  bool GetShowSyncErrors() const;

  // Sets whether sync errors are shown.
  void SetShowSyncErrors(bool show);

  // Returns whether sync only happens on WiFi.
  bool GetSyncOnWifiOnly() const;

  // Sets whether sync only happens on WiFi.
  void SetSyncOnWifiOnly(bool wifi_only);

  // Returns the sync frequency setting.
  // Values: "auto", "hourly", "daily".
  std::string GetSyncFrequency() const;

  // Sets the sync frequency.
  void SetSyncFrequency(const std::string& frequency);

  // Returns whether automatic sync is enabled.
  bool GetAutoSync() const;

  // Sets whether automatic sync is enabled.
  void SetAutoSync(bool auto_sync);

  // Returns whether all sync data is encrypted.
  bool GetEncryptAllData() const;

  // Sets whether all sync data is encrypted.
  void SetEncryptAllData(bool encrypt);

  // Returns whether the account avatar is shown.
  bool GetShowAccountAvatar() const;

  // Sets whether the account avatar is shown.
  void SetShowAccountAvatar(bool show);

  // Returns whether last sync time is shown.
  bool GetShowLastSyncTime() const;

  // Sets whether last sync time is shown.
  void SetShowLastSyncTime(bool show);

  // Returns whether the sync icon is shown in the toolbar.
  bool GetShowSyncIconInToolbar() const;

  // Sets whether the sync icon is shown in the toolbar.
  void SetShowSyncIconInToolbar(bool show);

  // -- Bulk operations ---------------------------------------------------

  // Toggles multiple data types at once.
  // Fires OnSyncSettingsChanged observer notification.
  void ToggleDataTypes(const std::vector<AstraSyncDataType>& types,
                       bool enabled);

  // Gets states for multiple data types at once.
  std::vector<AstraSyncTypeState> GetDataTypeStates(
      const std::vector<AstraSyncDataType>& types) const;

  // Requests sync for specific data types now.
  void SyncDataTypesNow(const std::vector<AstraSyncDataType>& types);

  // Resets all sync presentation settings to their defaults.
  // Fires OnSyncSettingsChanged observer notification.
  void ResetSyncSettingsToDefaults();

  // -- Utility methods ---------------------------------------------------

  // Returns a human-readable label for a sync status.
  static std::string GetSyncStatusLabel(AstraSyncStatus status);

  // Returns a human-readable label for a data type.
  static std::string GetDataTypeLabel(AstraSyncDataType type);

  // Returns the icon name for a data type.
  static std::string GetDataTypeIcon(AstraSyncDataType type);

  // Formats a last sync time for display.
  static std::string FormatLastSyncTime(base::Time time);

  // Returns a color identifier for a sync status.
  static std::string GetSyncStatusColor(AstraSyncStatus status);

  // Returns whether a data type is enabled by default.
  static bool IsSyncDataTypeEnabledByDefault(AstraSyncDataType type);

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraSyncObserver* observer);
  void RemoveObserver(AstraSyncObserver* observer);

  // -- Notification helpers (public for testing) -------------------------

  void NotifySyncStarted();
  void NotifySyncCompleted();
  void NotifySyncError(const std::string& error_message);
  void NotifySyncPaused();
  void NotifySyncResumed();
  void NotifySyncDataTypeToggled(AstraSyncDataType type, bool enabled);
  void NotifySyncSettingsChanged();
  void NotifySignedInStatusChanged(bool is_signed_in);

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the PrefService for the associated profile.
  PrefService* GetPrefs() const;

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for sync state changes.
  base::ObserverList<AstraSyncObserver> observers_;

  // Cached sync state (projection from Chromium SyncService).
  // TODO(astra): Replace cached state with live SyncService queries.
  mutable AstraSyncStatus cached_status_ = AstraSyncStatus::kNotSignedIn;
  mutable bool is_signed_in_ = false;
  mutable base::Time last_sync_time_;
  mutable bool has_sync_error_ = false;
  mutable std::string error_message_;
  mutable bool is_sync_paused_ = false;
  mutable int total_item_count_ = 0;
  mutable bool sync_enabled_ = true;

  // Per-type state storage.
  // TODO(astra): Replace with live queries from SyncService.
  mutable std::vector<AstraSyncTypeState> type_states_;

  // Account info (projection).
  mutable std::string account_email_;
  mutable std::string account_name_;
  mutable bool using_passphrase_ = false;

  // Initializes default type states.
  void InitializeTypeStates() const;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SYNC_HELPER_H_
