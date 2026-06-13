#ifndef ASTRA_BROWSER_ASTRA_PASSWORD_HELPER_H_
#define ASTRA_BROWSER_ASTRA_PASSWORD_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace password_manager {
struct PasswordForm;
class PasswordStoreInterface;
}  // namespace password_manager

namespace astra {

// =========================================================================
// Password strength classification
// =========================================================================
//
// Strength rating for a password, used by the password generator and
// strength checker UI.  These are projected values — the actual strength
// computation is done by Chromium's password strength estimator.
//
// Chromium owner: PasswordStrengthEstimator
//   (components/password_manager/core/browser/password_strength/password_strength_estimator.h)
enum class AstraPasswordStrength {
  kVeryWeak,   // 0-20% strength — extremely guessable
  kWeak,       // 21-40% strength — easily guessable
  kMedium,     // 41-60% strength — moderate security
  kStrong,     // 61-80% strength — good security
  kVeryStrong, // 81-100% strength — excellent security
};

// =========================================================================
// Password sort order
// =========================================================================
//
// Sort order for the password list in the sidebar.
// These are projected from the user's presentation preference.
enum class AstraPasswordSortOrder {
  kAlphabetical,  // Sort by site name (A-Z)
  kLastUsed,      // Sort by last used time (most recent first)
  kDateCreated,   // Sort by creation date (newest first)
};

// =========================================================================
// Password filter
// =========================================================================
//
// Filter type for the password list.
// Controls which passwords are shown in the sidebar.
enum class AstraPasswordFilter {
  kAll,           // Show all saved passwords
  kCompromised,   // Show only compromised passwords
  kWeak,          // Show only weak passwords
  kReused,        // Show only reused passwords
};

// =========================================================================
// Password grouping mode
// =========================================================================
//
// How passwords are grouped in the sidebar view.
enum class AstraPasswordGroupBy {
  kNone,      // No grouping — flat list
  kSite,      // Group by site domain
  kAccount,   // Group by username/account
};

// =========================================================================
// AstraPasswordEntry — projected password entry data
// =========================================================================
//
// Projection of a saved password entry for UI display.
//
// This is a presentation-only data structure — it mirrors a subset of
// Chromium's PasswordForm state. The truth source is always Chromium's
// PasswordStore and PasswordManagerService.
//
// Security note: The password value itself is NOT stored in this struct
// for long-term projection. Passwords are only retrieved on-demand when
// the user explicitly requests a copy, and they pass directly through
// Chromium's clipboard system without being cached by Astra.
struct AstraPasswordEntry {
  // The signon realm (URL origin) of the password.
  GURL url;

  // Display name for the site (e.g. "example.com").
  std::u16string site_display_name;

  // The username / signon value.
  std::u16string username;

  // Whether this is a blocked (never-save) site.
  bool is_blocked = false;

  // Whether this password has been flagged as compromised.
  bool is_compromised = false;

  // Whether this password is considered "weak" by Chromium's password
  // health check.
  bool is_weak = false;

  // Whether this password is reused across multiple sites.
  bool is_reused = false;

  // Password strength rating (projected from Chromium's estimator).
  AstraPasswordStrength strength = AstraPasswordStrength::kMedium;

  // Password strength as a percentage (0-100).
  int strength_percent = 50;

  // Last time this password was used (base::Time encoded as microseconds
  // since epoch, or 0 if never used).
  // TODO(astra): Use base::Time directly when Chromium headers are available.
  int64_t last_used_time = 0;

  // Date this password was created (microseconds since epoch).
  int64_t date_created = 0;

  // Number of times this password has been used.
  int use_count = 0;

  // Whether this is a federated credential (e.g. Sign in with Google).
  bool is_federated = false;

  // Favicon URL for the site (for display in the item view).
  // TODO(astra): Populate from FaviconService.
  // Chromium owner: FaviconService (components/favicon/core/favicon_service.h)
  GURL favicon_url;

  // Unique identifier for this password entry.
  // In Chromium, this would map to PasswordForm::unique_key or the form's
  // primary key in the password store.
  std::string id;

  // Notes attached to this password (if supported by the store).
  std::u16string notes;
};

// =========================================================================
// Password health stats
// =========================================================================
//
// Aggregated password health statistics projected from Chromium's
// PasswordHealthChecker.  Used by the password sidebar section to show
// a summary of password health (e.g. "3 compromised, 7 weak, 12 reused").
//
// Chromium owner: PasswordHealthChecker
//   (components/password_manager/core/browser/password_health_checker.h)
struct AstraPasswordHealthStats {
  // Total number of saved passwords.
  size_t total_passwords = 0;

  // Number of compromised (breached) passwords.
  size_t compromised_count = 0;

  // Number of weak passwords.
  size_t weak_count = 0;

  // Number of reused passwords.
  size_t reused_count = 0;

  // Number of blocked (never-save) sites.
  size_t blocked_count = 0;

  // Returns the total number of "problem" passwords (compromised + weak +
  // reused).  Note: a single password can be in multiple categories, so
  // this may exceed total_passwords.
  size_t problem_count() const {
    return compromised_count + weak_count + reused_count;
  }

  // Returns true if there are any compromised passwords.
  bool has_breaches() const { return compromised_count > 0; }
};

// =========================================================================
// AstraPasswordHelperObserver — observer interface
// =========================================================================
//
// Observer interface for AstraPasswordHelper. Notifies when the set of
// saved passwords changes (add, remove, update) or when password
// presentation settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh the passwords sidebar
// section when password store state changes. The browser layer never
// depends on Views code.
class AstraPasswordHelperObserver : public base::CheckedObserver {
 public:
  // Called when the full list of saved passwords has changed in any way
  // (added, removed, updated, etc.).
  // The UI should re-read the list via GetSavedPasswords() and rebuild
  // its projection.
  virtual void OnPasswordsChanged() {}

  // Called when a new password has been saved.
  // |entry| contains the projected info for the newly saved password.
  virtual void OnPasswordSaved(const AstraPasswordEntry& entry) {}

  // Called when an existing password has been updated.
  // |entry| contains the projected info for the updated password.
  virtual void OnPasswordUpdated(const AstraPasswordEntry& entry) {}

  // Called when a password has been deleted.
  // |entry| contains the projected info for the deleted password.
  virtual void OnPasswordDeleted(const AstraPasswordEntry& entry) {}

  // Called when password presentation settings have changed (e.g. show
  // suggestions, auto-fill enabled, password manager shortcut).
  // The UI should refresh its password section presentation.
  virtual void OnPasswordSettingsChanged() {}

  // Called when password health stats have been recomputed or changed.
  // The UI should refresh the health summary display.
  virtual void OnPasswordHealthChanged() {}

  // Called when a password breach is detected.
  // |compromised_count| is the number of newly detected compromised passwords.
  virtual void OnPasswordBreachDetected(size_t compromised_count) {}

 protected:
  ~AstraPasswordHelperObserver() override = default;
};

// =========================================================================
// AstraPasswordHelper — password projection helper
// =========================================================================
//
// Helper class for password-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// PasswordManagerService and PasswordStore APIs. It provides a clean
// interface for the Astra UI layer to query password state and perform
// password operations without directly depending on the full password
// manager subsystem.
//
// Truth source: Chromium's PasswordManagerService
//   (chrome/browser/password_manager/password_manager_service.h)
// and PasswordStore
//   (components/password_manager/core/browser/password_store.h)
//
// Security principles:
//   - Passwords are never stored in Astra-owned memory long-term.
//   - All password access goes through Chromium's password manager.
//   - Copy to clipboard uses Chromium's clipboard system.
//   - Master password / biometric auth is handled by Chromium, not Astra.
//   - Astra only projects metadata (site, username, count, health stats).
//
// Astra-specific presentation preferences (show suggestions, auto-fill
// shortcut, etc.) are persisted via the profile's PrefService.  These
// are purely presentation concerns and never affect the underlying
// password data managed by Chromium.
//
// TODO(astra): Proper PasswordStoreObserver integration.
//   Currently this helper does not observe the password store for live
//   updates. To get reactive updates, we need to implement
//   password_manager::PasswordStoreConsumer or use the PasswordStore
//   observer interface.
// Chromium owner: PasswordStore
//   (components/password_manager/core/browser/password_store.h)
// Chromium observer: PasswordStoreConsumer
//   (components/password_manager/core/browser/password_store_consumer.h)
// Chromium patch point: None needed — PasswordStoreConsumer is a public
//   interface that any KeyedService can use.
class AstraPasswordHelper : public KeyedService {
 public:
  explicit AstraPasswordHelper(Profile* profile);
  AstraPasswordHelper(const AstraPasswordHelper&) = delete;
  AstraPasswordHelper& operator=(const AstraPasswordHelper&) = delete;
  ~AstraPasswordHelper() override;

  // -- Password list queries ---------------------------------------------

  // Returns a list of saved passwords (metadata only — no password values).
  //
  // The returned list is a projection — callers should not cache it
  // across event loops since password store state can change at any time.
  //
  // |max_count| limits the number of results. Use 0 for no limit.
  //
  // TODO(astra): This is currently a synchronous stub. Real password
  //   store queries are asynchronous. The proper implementation would:
  //     1. Call PasswordStore->GetAutofillableLogins(consumer)
  //     2. Receive results via PasswordStoreConsumer::OnGetPasswordStoreResults
  //     3. Notify observers when results arrive
  //   For the overlay, we return an empty vector as a placeholder.
  //
  // Chromium API: PasswordStoreInterface::GetAutofillableLogins
  //   (components/password_manager/core/browser/password_store_interface.h)
  std::vector<AstraPasswordEntry> GetSavedPasswords(size_t max_count) const;

  // Returns saved passwords filtered and sorted according to the current
  // presentation settings (sort_order, filter, group_by).
  //
  // This is the primary query method for UI views.
  std::vector<AstraPasswordEntry> GetDisplayPasswords(size_t max_count) const;

  // Searches saved passwords by site name, username, or domain.
  // Returns entries where |query| matches the site display name,
  // username, or URL hostname (case-insensitive substring match).
  //
  // |max_count| limits the number of results.
  //
  // TODO(astra): Implement real search using PasswordStore query APIs
  //   or by filtering results from GetAutofillableLogins.
  // Chromium owner: PasswordStoreInterface
  std::vector<AstraPasswordEntry> SearchPasswords(
      const std::u16string& query,
      size_t max_count) const;

  // Searches saved passwords by URL / domain.
  // Returns entries whose URL matches |url| or whose hostname matches.
  //
  // |max_count| limits the number of results.
  std::vector<AstraPasswordEntry> GetPasswordsForURL(
      const GURL& url,
      size_t max_count = 0) const;

  // Returns a single password entry by ID, if found.
  absl::optional<AstraPasswordEntry> GetPasswordById(
      const std::string& id) const;

  // Returns the total number of saved passwords.
  //
  // TODO(astra): Implement with proper async count query or by reading
  //   from a cached count. For now, returns 0 as a placeholder.
  // Chromium owner: PasswordStoreInterface
  size_t GetPasswordCount() const;

  // Returns the total number of saved passwords.
  // Alias for GetPasswordCount() — provided for API clarity.
  size_t GetSavedPasswordsCount() const;

  // Returns the number of compromised (breached) passwords.
  //
  // TODO(astra): Query PasswordHealthChecker for compromised count.
  // Chromium owner: PasswordHealthChecker
  size_t GetCompromisedPasswordsCount() const;

  // Returns the number of weak passwords.
  //
  // TODO(astra): Query PasswordHealthChecker for weak count.
  size_t GetWeakPasswordsCount() const;

  // Returns the number of reused passwords.
  //
  // TODO(astra): Query PasswordHealthChecker for reused count.
  size_t GetReusedPasswordsCount() const;

  // Returns aggregated password health stats.
  //
  // TODO(astra): Compute from real PasswordHealthChecker data.
  AstraPasswordHealthStats GetPasswordHealthStats() const;

  // Returns the number of passwords that match the current filter setting.
  size_t GetFilteredPasswordCount() const;

  // -- Password strength -------------------------------------------------

  // Computes the strength rating for a given password string.
  //
  // This is a projection method — the actual strength estimation is
  // delegated to Chromium's PasswordStrengthEstimator.  For the overlay,
  // we provide a heuristic implementation based on length, character
  // variety, and common patterns.
  //
  // Chromium owner: PasswordStrengthEstimator
  //   (components/password_manager/core/browser/password_strength/)
  static AstraPasswordStrength GetPasswordStrength(
      const std::u16string& password);

  // Returns the strength of a password as a percentage (0-100).
  // 0 = weakest, 100 = strongest.
  static int GetPasswordStrengthPercent(const std::u16string& password);

  // Returns a human-readable strength label (e.g. "Weak", "Strong").
  static std::u16string GetPasswordStrengthLabel(
      AstraPasswordStrength strength);

  // Returns a color hint for the strength level (for UI display).
  // Returns an SkColor appropriate for the strength (red for weak,
  // green for strong).
  // TODO(astra): Use proper Chromium color IDs when available.
  static SkColor GetPasswordStrengthColor(AstraPasswordStrength strength);

  // -- Password generator -----------------------------------------------

  // Generates a random strong password of the given length.
  //
  // This delegates to Chromium's password generator when available.
  // For the overlay, we provide a basic implementation.
  //
  // |length| is the desired password length (default: 15).
  // |include_symbols| controls whether special characters are included.
  //
  // Chromium owner: PasswordGenerator
  //   (components/password_manager/core/browser/password_generator.h)
  static std::u16string GeneratePassword(size_t length = 15,
                                         bool include_symbols = true);

  // Checks whether a password meets basic complexity requirements.
  // A strong password should:
  //   - Be at least 12 characters long
  //   - Contain uppercase and lowercase letters
  //   - Contain at least one digit
  //   - Optionally contain symbols
  static bool MeetsComplexityRequirements(const std::u16string& password);

  // -- Password operations ----------------------------------------------

  // Copies the password for |entry| to the system clipboard.
  //
  // This delegates to Chromium's password manager and clipboard system.
  // The password value is never stored by Astra code — it flows directly
  // from PasswordStore to the clipboard.
  //
  // Returns true if the copy was successful.
  //
  // TODO(astra): Implement real copy using PasswordStore to retrieve
  //   the password value, then ui::Clipboard to write it.
  //   Requires proper biometric / master password reauth handled by
  //   Chromium's password manager.
  //
  // Chromium owner: PasswordManagerService
  //   (chrome/browser/password_manager/password_manager_service.h)
  // Chromium clipboard: ui::Clipboard (ui/base/clipboard/clipboard.h)
  bool CopyPasswordToClipboard(const AstraPasswordEntry& entry) const;

  // Copies the username for |entry| to the system clipboard.
  // Returns true if the copy was successful.
  bool CopyUsernameToClipboard(const AstraPasswordEntry& entry) const;

  // Opens the password settings page (chrome://settings/passwords).
  //
  // TODO(astra): Implement proper navigation to password settings.
  //   For now, this is a no-op stub.
  // Chromium WebUI: chrome://settings/passwords
  //   (chrome/browser/ui/webui/settings/password_manager_handler.h)
  void OpenPasswordSettings(Profile* profile) const;

  // Opens the password check page (chrome://settings/passwords/check).
  //
  // TODO(astra): Implement navigation to password check page.
  void OpenPasswordCheck(Profile* profile) const;

  // -- Bulk operations --------------------------------------------------

  // Returns true if bulk password export is available.
  //
  // TODO(astra): Check if password export is enabled by policy.
  // Chromium owner: PasswordManagerSettingsService
  bool IsBulkExportAvailable() const;

  // Exports all saved passwords to a CSV file at |file_path|.
  //
  // TODO(astra): Implement using PasswordManager's export functionality.
  // Returns true if export was initiated successfully.
  //
  // Chromium owner: PasswordExporter
  //   (components/password_manager/core/browser/export/password_exporter.h)
  bool ExportPasswords(const std::string& file_path) const;

  // -- Sort order settings ----------------------------------------------

  // Gets the current sort order for the password list.
  // Persisted via PrefService. Default: kAlphabetical.
  AstraPasswordSortOrder GetSortOrder() const;

  // Sets the sort order. Fires OnPasswordSettingsChanged.
  void SetSortOrder(AstraPasswordSortOrder order);

  // Toggles through available sort orders.
  // Returns the new sort order.
  AstraPasswordSortOrder CycleSortOrder();

  // -- Filter settings --------------------------------------------------

  // Gets the current filter for the password list.
  // Persisted via PrefService. Default: kAll.
  AstraPasswordFilter GetFilter() const;

  // Sets the filter. Fires OnPasswordSettingsChanged.
  void SetFilter(AstraPasswordFilter filter);

  // Toggles through available filters.
  // Returns the new filter.
  AstraPasswordFilter CycleFilter();

  // -- Grouping settings ------------------------------------------------

  // Gets how passwords are grouped in the sidebar.
  // Persisted via PrefService. Default: kNone.
  AstraPasswordGroupBy GetGroupBy() const;

  // Sets the grouping mode. Fires OnPasswordSettingsChanged.
  void SetGroupBy(AstraPasswordGroupBy group_by);

  // Toggles through available grouping modes.
  // Returns the new grouping mode.
  AstraPasswordGroupBy CycleGroupBy();

  // -- Password visibility settings -------------------------------------

  // Whether passwords are hidden (shown as dots) by default.
  // Persisted via PrefService. Default: true.
  bool GetHidePasswordsByDefault() const;

  // Sets whether passwords are hidden by default.
  // Fires OnPasswordSettingsChanged.
  void SetHidePasswordsByDefault(bool hide);

  // Toggles the hide-passwords-by-default setting.
  // Returns the new state.
  bool ToggleHidePasswordsByDefault();

  // -- Presentation settings --------------------------------------------

  // Returns whether password suggestions are shown in Astra UI surfaces
  // (e.g. in the login form sidebar panel, or when typing in forms).
  //
  // Persisted via PrefService.  Default: true.
  //
  // This is a presentation preference — it never affects whether
  // Chromium's password manager auto-fills or saves passwords.
  bool GetShowPasswordSuggestions() const;

  // Sets whether password suggestions are shown in Astra UI.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetShowPasswordSuggestions(bool show);

  // Toggles the show password suggestions setting.
  // Returns the new state.
  bool ToggleShowPasswordSuggestions();

  // Returns whether auto-fill is shown as enabled in Astra UI.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This is a presentation preference for the Astra UI.  The actual
  // auto-fill behavior is controlled by Chromium's password manager
  // settings (prefs::kPasswordManagerAutoSignin etc.).
  bool GetAutoFillEnabled() const;

  // Sets whether auto-fill is shown as enabled in Astra UI.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetAutoFillEnabled(bool enabled);

  // Toggles the auto-fill enabled setting.
  // Returns the new state.
  bool ToggleAutoFillEnabled();

  // Returns whether the password manager shortcut is shown in the
  // Astra sidebar or toolbar.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This controls whether the password manager entry point appears in
  // the Astra UI.  The actual password manager is always available
  // through Chrome settings.
  bool GetPasswordManagerShortcut() const;

  // Sets whether the password manager shortcut is shown.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetPasswordManagerShortcut(bool show);

  // Toggles the password manager shortcut visibility.
  // Returns the new state.
  bool TogglePasswordManagerShortcut();

  // Returns whether the password health summary is shown in the
  // password sidebar section.
  //
  // Persisted via PrefService.  Default: true.
  bool GetShowPasswordHealth() const;

  // Sets whether the password health summary is shown.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetShowPasswordHealth(bool show);

  // Toggles the show password health setting.
  // Returns the new state.
  bool ToggleShowPasswordHealth();

  // Returns whether breach alerts are shown in Astra UI.
  //
  // Persisted via PrefService.  Default: true.
  //
  // When enabled, the Astra UI shows a warning indicator when compromised
  // passwords are detected.  The actual breach detection is done by
  // Chromium's password manager.
  bool GetBreachAlertsEnabled() const;

  // Sets whether breach alerts are shown.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetBreachAlertsEnabled(bool enabled);

  // Toggles breach alerts.
  // Returns the new state.
  bool ToggleBreachAlerts();

  // Returns the maximum number of passwords to show in the sidebar
  // password section.
  //
  // Persisted via PrefService.  Default: 20.
  int GetMaxSidebarPasswords() const;

  // Sets the maximum number of passwords to show in the sidebar.
  // Values are clamped between 1 and 100.
  // Fires OnPasswordSettingsChanged observer notification.
  void SetMaxSidebarPasswords(int max_count);

  // -- Breach detection state -------------------------------------------

  // Returns whether there are any compromised (breached) passwords.
  //
  // This is a convenience method — equivalent to
  // GetCompromisedPasswordsCount() > 0.
  bool HasCompromisedPasswords() const;

  // Returns whether a breach has been detected since the user last
  // acknowledged it.
  //
  // Persisted via PrefService.  Default: false.
  //
  // TODO(astra): Wire up to real breach detection events.
  bool IsBreachDetected() const;

  // Marks the breach state as acknowledged (resets the unread indicator).
  // Fires OnPasswordSettingsChanged observer notification if state changed.
  void AcknowledgeBreach();

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraPasswordHelperObserver* observer);
  void RemoveObserver(AstraPasswordHelperObserver* observer);

  // -- Notification helpers (public for testing) ------------------------

  // Notify all observers that the password list has changed.
  void NotifyPasswordsChanged();

  // Notify all observers that a password has been saved.
  void NotifyPasswordSaved(const AstraPasswordEntry& entry);

  // Notify all observers that a password has been updated.
  void NotifyPasswordUpdated(const AstraPasswordEntry& entry);

  // Notify all observers that a password has been deleted.
  void NotifyPasswordDeleted(const AstraPasswordEntry& entry);

  // Notify all observers that password settings have changed.
  void NotifyPasswordSettingsChanged();

  // Notify all observers that password health stats have changed.
  void NotifyPasswordHealthChanged();

  // Notify all observers that a password breach has been detected.
  void NotifyPasswordBreachDetected(size_t compromised_count);

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the PasswordStore for the associated profile.
  // Returns nullptr if the store is not available.
  //
  // TODO(astra): Use PasswordStoreFactory::GetForProfile() when building
  //   against the full Chromium source tree. In the overlay, we return
  //   nullptr as a placeholder.
  //
  // Chromium factory: PasswordStoreFactory
  //   (chrome/browser/password_manager/password_store_factory.h)
  password_manager::PasswordStoreInterface* GetPasswordStore() const;

  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for password store changes.
  base::ObserverList<AstraPasswordHelperObserver> observers_;

  // Tracks whether we're currently observing the password store.
  // TODO(astra): Flip this to true once PasswordStoreConsumer
  // integration is implemented.
  bool is_observing_store_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_PASSWORD_HELPER_H_
