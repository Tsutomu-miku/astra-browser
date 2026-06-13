// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_H_
#define ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_H_

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
// AstraAutofillEntryType — type of autofill entry
// =========================================================================
//
// Enumeration of autofill entry types. These are projected from Chromium's
// autofill type system for use in Astra UI.
//
// Chromium owner: AutofillType / AutofillFieldType
//   (components/autofill/core/browser/autofill_type.h)
enum class AstraAutofillEntryType {
  kAddress,     // Address autofill entry
  kCreditCard,  // Credit card autofill entry
  kPassword,    // Password autofill entry (autofill perspective)
  kEmail,       // Email address entry
  kPhone,       // Phone number entry
  kName,        // Person name entry
  kCustom,      // Custom / autocomplete attribute entry
};

// =========================================================================
// AstraAutofillEntry — projected autofill entry data
// =========================================================================
//
// Projection of an autofill entry for UI display.
//
// This is a presentation-only data structure — it mirrors a subset of
// Chromium's autofill entry state. The truth source is always Chromium's
// AutofillProfile (for addresses) and CreditCard (for payment data).
//
// Chromium owner: AutofillProfile
//   (components/autofill/core/browser/data_model/autofill_profile.h)
// Chromium owner: CreditCard
//   (components/autofill/core/browser/data_model/credit_card.h)
struct AstraAutofillEntry {
  // Unique identifier for the entry.
  std::string id;

  // Type of the autofill entry.
  AstraAutofillEntryType type = AstraAutofillEntryType::kCustom;

  // Display label for the entry (e.g. "Home address", "Work Visa").
  std::string label;

  // The primary autofill value (e.g. the full address summary, email, phone).
  std::string value;

  // When the entry was last used.
  base::Time last_used;

  // Number of times the entry has been used.
  int use_count = 0;

  // Whether this entry is blocked / hidden from suggestions.
  bool is_blocked = false;

  // Associated profile ID (for addresses and credit cards that belong
  // to a named autofill profile).
  std::string profile_id;
};

// =========================================================================
// AstraAutofillAddressEntry — extended address entry
// =========================================================================
//
// Extended projection of an address autofill entry with all address fields.
// Used when the full address details are needed (e.g. in the address editor).
//
// Chromium owner: AutofillProfile
//   (components/autofill/core/browser/data_model/autofill_profile.h)
struct AstraAutofillAddressEntry {
  // Base entry data.
  AstraAutofillEntry base;

  // Full name of the person at this address.
  std::string full_name;

  // Company / organization name.
  std::string company;

  // Street address (line 1 + line 2 combined).
  std::string street_address;

  // City / locality.
  std::string city;

  // State / province / region.
  std::string state;

  // ZIP / postal code.
  std::string zip_code;

  // Country code or name.
  std::string country;

  // Phone number associated with this address.
  std::string phone_number;

  // Email address associated with this address.
  std::string email;
};

// =========================================================================
// AstraAutofillCreditCardEntry — extended credit card entry
// =========================================================================
//
// Extended projection of a credit card autofill entry with card details.
//
// Security note: The full card number is never stored in this struct.
// Only the last four digits and network are projected.
//
// Chromium owner: CreditCard
//   (components/autofill/core/browser/data_model/credit_card.h)
struct AstraAutofillCreditCardEntry {
  // Base entry data.
  AstraAutofillEntry base;

  // Last four digits of the card number.
  std::string card_number_last_four;

  // Cardholder name.
  std::string cardholder_name;

  // Expiration month (1-12).
  int expiration_month = 0;

  // Expiration year (4-digit).
  int expiration_year = 0;

  // Card network (e.g. "visa", "mastercard", "amex", "discover").
  std::string card_network;

  // Whether this is a virtual card number.
  bool is_virtual = false;

  // ID of the billing address profile.
  std::string billing_address_id;
};

// =========================================================================
// AstraAutofillObserver — observer interface
// =========================================================================
//
// Observer interface for AstraAutofillHelper. Notifies when autofill
// entries change, when suggestions are shown, or when presentation
// settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh autofill-related UI
// (sidebar section, popup suggestions, settings page) when state changes.
// The browser layer never depends on Views code.
class AstraAutofillObserver : public base::CheckedObserver {
 public:
  // Called when an autofill suggestion popup is shown.
  virtual void OnAutofillSuggestionShown() {}

  // Called when an autofill entry is selected from suggestions.
  // |entry_id| is the ID of the selected entry.
  virtual void OnAutofillEntrySelected(const std::string& entry_id) {}

  // Called when a new autofill entry has been added.
  // |entry_id| is the ID of the newly added entry.
  virtual void OnAutofillEntryAdded(const std::string& entry_id) {}

  // Called when an autofill entry has been removed.
  // |entry_id| is the ID of the removed entry.
  virtual void OnAutofillEntryRemoved(const std::string& entry_id) {}

  // Called when an autofill entry has been updated.
  // |entry_id| is the ID of the updated entry.
  virtual void OnAutofillEntryUpdated(const std::string& entry_id) {}

  // Called when autofill presentation settings have changed.
  // The UI should refresh its autofill-related presentation.
  virtual void OnAutofillSettingsChanged() {}

  // Called when the active autofill profile changes.
  virtual void OnAutofillProfileChanged() {}

  // Called when autofill data has been cleared (all or a category).
  virtual void OnAutofillDataCleared() {}

 protected:
  ~AstraAutofillObserver() override = default;
};

// =========================================================================
// AstraAutofillHelper — autofill projection helper
// =========================================================================
//
// Helper class for autofill-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// autofill subsystem. It provides a clean interface for the Astra UI
// layer to query autofill state and perform autofill operations without
// directly depending on the full autofill subsystem.
//
// Truth source: Chromium's PersonalDataManager
//   (components/autofill/core/browser/personal_data_manager.h)
//
// Projection pattern:
//   - Astra only projects autofill state — it never owns the data.
//   - All modifications delegate to Chromium's PersonalDataManager.
//   - Presentation preferences are Astra-owned and persist via PrefService.
//   - TODO(astra): In the overlay skeleton, entries are stored in-memory.
//     In production, they would be read from PersonalDataManager.
//
// TODO(astra): Integrate with Chromium's PersonalDataManager for live data.
//   Currently this helper uses an in-memory entry store as a placeholder.
//   The proper implementation would:
//     1. Observe PersonalDataManager for changes.
//     2. Convert AutofillProfile and CreditCard objects to projected entries.
//     3. Notify observers when the underlying data changes.
//
// Chromium subsystem: PersonalDataManager
//   (components/autofill/core/browser/personal_data_manager.h)
// Chromium observer: PersonalDataManagerObserver
//   (components/autofill/core/browser/personal_data_manager_observer.h)
class AstraAutofillHelper : public KeyedService {
 public:
  explicit AstraAutofillHelper(Profile* profile);
  AstraAutofillHelper(const AstraAutofillHelper&) = delete;
  AstraAutofillHelper& operator=(const AstraAutofillHelper&) = delete;
  ~AstraAutofillHelper() override;

  // -- Entry count queries ------------------------------------------------

  // Returns the total number of autofill entries (all types).
  size_t GetEntryCount() const;

  // Returns the number of address entries.
  size_t GetAddressCount() const;

  // Returns the number of credit card entries.
  size_t GetCreditCardCount() const;

  // -- Entry list queries -------------------------------------------------

  // Returns all autofill entries (all types).
  //
  // The returned list is a projection — callers should not cache it
  // across event loops since autofill state can change at any time.
  std::vector<AstraAutofillEntry> GetAllEntries() const;

  // Returns all address entries.
  std::vector<AstraAutofillEntry> GetAddresses() const;

  // Returns all credit card entries.
  std::vector<AstraAutofillEntry> GetCreditCards() const;

  // Returns a single entry by its ID.
  // Returns a default-constructed entry if not found.
  AstraAutofillEntry GetEntry(const std::string& id) const;

  // -- Suggestion queries -------------------------------------------------

  // Returns suggestions for a given field type.
  // |field_type| is an autofill field type hint (e.g. "email", "address",
  // "name", "phone", "credit-card").
  // |max_count| limits the number of results.
  //
  // TODO(astra): Map field types to Chromium's AutofillType system.
  //   Chromium owner: AutofillType
  //     (components/autofill/core/browser/autofill_type.h)
  std::vector<AstraAutofillEntry> GetSuggestions(
      const std::string& field_type,
      int max_count) const;

  // Returns the most recently used entries.
  // |max_count| limits the number of results.
  std::vector<AstraAutofillEntry> GetRecentlyUsed(int max_count) const;

  // Returns the most frequently used entries.
  // |max_count| limits the number of results.
  std::vector<AstraAutofillEntry> GetMostUsed(int max_count) const;

  // -- Entry operations ---------------------------------------------------

  // Adds a new autofill entry.
  // If an entry with the same ID already exists, it is updated.
  // Fires OnAutofillEntryAdded (or OnAutofillEntryUpdated) notification.
  //
  // TODO(astra): Delegate to PersonalDataManager in production.
  //   Chromium: PersonalDataManager::AddProfile / AddCreditCard
  void AddEntry(const AstraAutofillEntry& entry);

  // Updates an existing autofill entry.
  // If no entry with |id| exists, this is a no-op.
  // Fires OnAutofillEntryUpdated notification.
  //
  // TODO(astra): Delegate to PersonalDataManager in production.
  //   Chromium: PersonalDataManager::UpdateProfile / UpdateCreditCard
  void UpdateEntry(const std::string& id, const AstraAutofillEntry& entry);

  // Removes an autofill entry by ID.
  // If no entry with |id| exists, this is a no-op.
  // Fires OnAutofillEntryRemoved notification.
  //
  // TODO(astra): Delegate to PersonalDataManager in production.
  //   Chromium: PersonalDataManager::RemoveByGUID
  void RemoveEntry(const std::string& id);

  // Blocks / hides an autofill entry from suggestions.
  // If no entry with |id| exists, this is a no-op.
  // Fires OnAutofillEntryUpdated notification.
  void BlockEntry(const std::string& id);

  // Unblocks / shows an autofill entry in suggestions.
  // If no entry with |id| exists, this is a no-op.
  // Fires OnAutofillEntryUpdated notification.
  void UnblockEntry(const std::string& id);

  // -- Clear operations ---------------------------------------------------

  // Clears all autofill entries.
  // Fires OnAutofillDataCleared notification.
  void ClearAllEntries();

  // Clears all address entries.
  // Fires OnAutofillDataCleared notification.
  void ClearAddresses();

  // Clears all credit card entries.
  // Fires OnAutofillDataCleared notification.
  void ClearCreditCards();

  // -- Bulk operations ----------------------------------------------------

  // Bulk imports autofill entries.
  // Entries with duplicate IDs are treated as updates.
  // Fires added/updated notifications for each entry.
  void ImportEntries(const std::vector<AstraAutofillEntry>& entries);

  // Bulk removes entries by ID.
  // Fires removed notifications for each found entry.
  void RemoveEntries(const std::vector<std::string>& ids);

  // Bulk blocks entries by ID.
  void BlockEntries(const std::vector<std::string>& ids);

  // Finds and merges duplicate address entries.
  // Duplicates are detected by comparing normalized address data.
  // Returns the number of merges performed.
  //
  // TODO(astra): Use Chromium's duplicate detection logic.
  //   Chromium: AutofillProfileComparator
  //     (components/autofill/core/browser/autofill_profile_comparator.h)
  int MergeDuplicateAddresses();

  // -- Autofill enable settings ------------------------------------------

  // Returns whether autofill is globally enabled for Astra UI.
  //
  // Persisted via PrefService. Default: true.
  //
  // This is a presentation preference — it controls whether Astra UI
  // surfaces autofill functionality. The actual autofill behavior is
  // controlled by Chromium's autofill settings.
  bool IsAutofillEnabled() const;

  // Sets whether autofill is globally enabled in Astra UI.
  // Fires OnAutofillSettingsChanged notification.
  void SetAutofillEnabled(bool enabled);

  // Returns whether address autofill is enabled in Astra UI.
  bool IsAddressAutofillEnabled() const;

  // Sets whether address autofill is enabled.
  // Fires OnAutofillSettingsChanged notification.
  void SetAddressAutofillEnabled(bool enabled);

  // Returns whether credit card autofill is enabled in Astra UI.
  bool IsCreditCardAutofillEnabled() const;

  // Sets whether credit card autofill is enabled.
  // Fires OnAutofillSettingsChanged notification.
  void SetCreditCardAutofillEnabled(bool enabled);

  // Returns whether auto sign-in is enabled in Astra UI.
  // Auto sign-in automatically fills and submits login forms.
  bool IsAutosignInEnabled() const;

  // Sets whether auto sign-in is enabled.
  // Fires OnAutofillSettingsChanged notification.
  void SetAutosignInEnabled(bool enabled);

  // -- Presentation settings ---------------------------------------------

  // Returns whether the autofill popup is shown.
  bool GetShowAutofillPopup() const;

  // Sets whether the autofill popup is shown.
  // Fires OnAutofillSettingsChanged notification.
  void SetShowAutofillPopup(bool show);

  // Returns the autofill popup position.
  // Values: "below_field", "above_field", "auto".
  std::string GetAutofillPopupPosition() const;

  // Sets the autofill popup position.
  // Fires OnAutofillSettingsChanged notification.
  void SetAutofillPopupPosition(const std::string& position);

  // Returns the maximum number of suggestions shown.
  int GetMaxSuggestions() const;

  // Sets the maximum number of suggestions shown.
  // Values are clamped between 1 and 20.
  // Fires OnAutofillSettingsChanged notification.
  void SetMaxSuggestions(int max_count);

  // Returns whether suggestion icons are shown.
  bool GetShowSuggestionIcons() const;

  // Sets whether suggestion icons are shown.
  // Fires OnAutofillSettingsChanged notification.
  void SetShowSuggestionIcons(bool show);

  // Returns whether suggestion labels are shown.
  bool GetShowSuggestionLabels() const;

  // Sets whether suggestion labels are shown.
  // Fires OnAutofillSettingsChanged notification.
  void SetShowSuggestionLabels(bool show);

  // Returns whether suggestion subtext is shown.
  bool GetShowSuggestionSubtext() const;

  // Sets whether suggestion subtext is shown.
  // Fires OnAutofillSettingsChanged notification.
  void SetShowSuggestionSubtext(bool show);

  // Returns whether autofill triggers on tap/click.
  bool GetAutofillOnTap() const;

  // Sets whether autofill triggers on tap/click.
  // Fires OnAutofillSettingsChanged notification.
  void SetAutofillOnTap(bool enabled);

  // Returns the suggestions sort order.
  // Values: "most_recent", "most_used", "alphabetical".
  std::string GetSuggestionsSortOrder() const;

  // Sets the suggestions sort order.
  // Fires OnAutofillSettingsChanged notification.
  void SetSuggestionsSortOrder(const std::string& order);

  // Returns whether credit card network icons are shown.
  bool GetShowCreditCardIcons() const;

  // Sets whether credit card network icons are shown.
  // Fires OnAutofillSettingsChanged notification.
  void SetShowCreditCardIcons(bool show);

  // Returns whether quick checkout flow is enabled.
  bool GetAutofillQuickCheckout() const;

  // Sets whether quick checkout flow is enabled.
  // Fires OnAutofillSettingsChanged notification.
  void SetAutofillQuickCheckout(bool enabled);

  // -- Utility methods ----------------------------------------------------

  // Formats the last four digits of a card number for display.
  // e.g. "1234" -> "•••• 1234"
  static std::string FormatCardNumberLastFour(const std::string& last_four);

  // Returns a human-readable label for a card network.
  // e.g. "visa" -> "Visa", "mastercard" -> "Mastercard"
  static std::string GetCardNetworkLabel(const std::string& network);

  // Returns a human-readable label for an entry type.
  static std::string GetEntryTypeLabel(AstraAutofillEntryType type);

  // Masks a full card number, showing only the last four digits.
  // e.g. "4111111111111234" -> "•••• •••• •••• 1234"
  static std::string MaskCardNumber(const std::string& full_number);

  // Formats an expiration date as a string.
  // e.g. month=12, year=2025 -> "12/25"
  static std::string FormatExpiration(int month, int year);

  // Truncates suggestion text to a maximum length.
  // Adds ellipsis if truncated.
  static std::string TruncateSuggestion(const std::string& text, int max_length);

  // -- Observers ----------------------------------------------------------

  void AddObserver(AstraAutofillObserver* observer);
  void RemoveObserver(AstraAutofillObserver* observer);

  // -- Notification helpers (public for testing) -------------------------

  void NotifySuggestionShown();
  void NotifyEntrySelected(const std::string& entry_id);
  void NotifyEntryAdded(const std::string& entry_id);
  void NotifyEntryRemoved(const std::string& entry_id);
  void NotifyEntryUpdated(const std::string& entry_id);
  void NotifySettingsChanged();
  void NotifyProfileChanged();
  void NotifyDataCleared();

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // Returns entries of a specific type.
  std::vector<AstraAutofillEntry> GetEntriesByType(
      AstraAutofillEntryType type) const;

  // Returns true if an entry with |id| exists.
  bool HasEntry(const std::string& id) const;

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for autofill state changes.
  base::ObserverList<AstraAutofillObserver> observers_;

  // In-memory entry store.
  //
  // TODO(astra): This is a placeholder for the overlay skeleton.
  //   In production, entries are read from Chromium's PersonalDataManager.
  //   The in-memory store is used for testing and as a projection cache.
  //
  // Chromium owner: PersonalDataManager
  //   (components/autofill/core/browser/personal_data_manager.h)
  std::vector<AstraAutofillEntry> entries_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_H_
