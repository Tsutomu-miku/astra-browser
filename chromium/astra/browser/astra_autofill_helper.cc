// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_autofill_helper.h"

#include <algorithm>
#include <string>

#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout. In this overlay repo, the
// types are forward-declared in the header and the real definitions
// come from Chromium at build time.
//
// Chromium owner: PersonalDataManager
//   (components/autofill/core/browser/personal_data_manager.h)
// Chromium owner: AutofillProfile
//   (components/autofill/core/browser/data_model/autofill_profile.h)
// Chromium owner: CreditCard
//   (components/autofill/core/browser/data_model/credit_card.h)
// #include "components/autofill/core/browser/personal_data_manager.h"
// #include "components/autofill/core/browser/data_model/autofill_profile.h"
// #include "components/autofill/core/browser/data_model/credit_card.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Clamp range for max suggestions.
constexpr int kMinMaxSuggestions = 1;
constexpr int kMaxMaxSuggestions = 20;

// Clamp range for truncation length.
constexpr int kMinTruncateLength = 1;
constexpr int kMaxTruncateLength = 1000;

// Valid popup position values.
constexpr char kPopupPositionBelowField[] = "below_field";
constexpr char kPopupPositionAboveField[] = "above_field";
constexpr char kPopupPositionAuto[] = "auto";

// Valid sort order values.
constexpr char kSortOrderMostRecent[] = "most_recent";
constexpr char kSortOrderMostUsed[] = "most_used";
constexpr char kSortOrderAlphabetical[] = "alphabetical";

// Card network labels.
constexpr char kCardNetworkVisa[] = "visa";
constexpr char kCardNetworkMastercard[] = "mastercard";
constexpr char kCardNetworkAmex[] = "amex";
constexpr char kCardNetworkDiscover[] = "discover";
constexpr char kCardNetworkJcb[] = "jcb";
constexpr char kCardNetworkDiners[] = "diners";
constexpr char kCardNetworkUnionPay[] = "unionpay";

// Clamp helper for max suggestions.
int ClampMaxSuggestions(int value) {
  if (value < kMinMaxSuggestions) return kMinMaxSuggestions;
  if (value > kMaxMaxSuggestions) return kMaxMaxSuggestions;
  return value;
}

// Validates a popup position string, returning a valid default if invalid.
std::string ValidatePopupPosition(const std::string& position) {
  if (position == kPopupPositionBelowField ||
      position == kPopupPositionAboveField ||
      position == kPopupPositionAuto) {
    return position;
  }
  return prefs::kDefaultAutofillPopupPosition;
}

// Validates a sort order string, returning a valid default if invalid.
std::string ValidateSortOrder(const std::string& order) {
  if (order == kSortOrderMostRecent ||
      order == kSortOrderMostUsed ||
      order == kSortOrderAlphabetical) {
    return order;
  }
  return prefs::kDefaultAutofillSuggestionsSortOrder;
}

// Comparator for sorting entries by last_used descending (most recent first).
bool CompareByMostRecent(const AstraAutofillEntry& a,
                         const AstraAutofillEntry& b) {
  return a.last_used > b.last_used;
}

// Comparator for sorting entries by use_count descending (most used first).
bool CompareByMostUsed(const AstraAutofillEntry& a,
                       const AstraAutofillEntry& b) {
  return a.use_count > b.use_count;
}

// Comparator for sorting entries by label alphabetically.
bool CompareByAlphabetical(const AstraAutofillEntry& a,
                           const AstraAutofillEntry& b) {
  return base::CompareCaseInsensitiveASCII(a.label, b.label) < 0;
}

// Applies the configured sort order to a list of entries.
void SortEntries(std::vector<AstraAutofillEntry>& entries,
                 const std::string& sort_order) {
  if (sort_order == kSortOrderMostRecent) {
    std::sort(entries.begin(), entries.end(), CompareByMostRecent);
  } else if (sort_order == kSortOrderMostUsed) {
    std::sort(entries.begin(), entries.end(), CompareByMostUsed);
  } else if (sort_order == kSortOrderAlphabetical) {
    std::sort(entries.begin(), entries.end(), CompareByAlphabetical);
  }
}

// Checks if an entry matches a field type for suggestions.
bool MatchesFieldType(const AstraAutofillEntry& entry,
                      const std::string& field_type) {
  std::string lower_type = base::ToLowerASCII(field_type);

  if (lower_type == "address" || lower_type == "street-address" ||
      lower_type == "address-line1" || lower_type == "address-line2" ||
      lower_type == "city" || lower_type == "state" ||
      lower_type == "zip" || lower_type == "postal-code" ||
      lower_type == "country") {
    return entry.type == AstraAutofillEntryType::kAddress;
  }

  if (lower_type == "email" || lower_type == "email-address") {
    return entry.type == AstraAutofillEntryType::kEmail;
  }

  if (lower_type == "tel" || lower_type == "phone" ||
      lower_type == "phone-number") {
    return entry.type == AstraAutofillEntryType::kPhone;
  }

  if (lower_type == "name" || lower_type == "given-name" ||
      lower_type == "family-name" || lower_type == "full-name") {
    return entry.type == AstraAutofillEntryType::kName;
  }

  if (lower_type == "cc" || lower_type == "credit-card" ||
      lower_type == "card-number" || lower_type == "cc-number" ||
      lower_type == "cc-exp" || lower_type == "cc-csc") {
    return entry.type == AstraAutofillEntryType::kCreditCard;
  }

  if (lower_type == "password" || lower_type == "current-password" ||
      lower_type == "new-password") {
    return entry.type == AstraAutofillEntryType::kPassword;
  }

  // Default: match all non-blocked entries.
  return !entry.is_blocked;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraAutofillHelper::AstraAutofillHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing PersonalDataManager for live updates.
  //   Requires implementing PersonalDataManagerObserver.
  //
  //   autofill::PersonalDataManager* pdm = GetPersonalDataManager();
  //   if (pdm) {
  //     pdm->AddObserver(this);
  //   }
  //
  // Chromium observer: PersonalDataManagerObserver
  //   (components/autofill/core/browser/personal_data_manager_observer.h)
}

AstraAutofillHelper::~AstraAutofillHelper() = default;

void AstraAutofillHelper::Shutdown() {
  // TODO(astra): Remove observer from PersonalDataManager.
  //   autofill::PersonalDataManager* pdm = GetPersonalDataManager();
  //   if (pdm) {
  //     pdm->RemoveObserver(this);
  //   }
  profile_ = nullptr;
}

// =========================================================================
// Entry count queries
// =========================================================================

size_t AstraAutofillHelper::GetEntryCount() const {
  return entries_.size();
}

size_t AstraAutofillHelper::GetAddressCount() const {
  return GetEntriesByType(AstraAutofillEntryType::kAddress).size();
}

size_t AstraAutofillHelper::GetCreditCardCount() const {
  return GetEntriesByType(AstraAutofillEntryType::kCreditCard).size();
}

// =========================================================================
// Entry list queries
// =========================================================================

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetAllEntries() const {
  // TODO(astra): In production, read from PersonalDataManager.
  //   For the overlay skeleton, return the in-memory entries.
  return entries_;
}

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetAddresses() const {
  return GetEntriesByType(AstraAutofillEntryType::kAddress);
}

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetCreditCards() const {
  return GetEntriesByType(AstraAutofillEntryType::kCreditCard);
}

AstraAutofillEntry AstraAutofillHelper::GetEntry(
    const std::string& id) const {
  for (const auto& entry : entries_) {
    if (entry.id == id) {
      return entry;
    }
  }
  return AstraAutofillEntry();
}

// =========================================================================
// Suggestion queries
// =========================================================================

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetSuggestions(
    const std::string& field_type,
    int max_count) const {
  std::vector<AstraAutofillEntry> result;

  if (max_count <= 0) {
    max_count = GetMaxSuggestions();
  }
  max_count = ClampMaxSuggestions(max_count);

  for (const auto& entry : entries_) {
    if (entry.is_blocked) {
      continue;
    }
    if (MatchesFieldType(entry, field_type)) {
      result.push_back(entry);
    }
  }

  // Apply sort order.
  SortEntries(result, GetSuggestionsSortOrder());

  // Cap result size.
  if (result.size() > static_cast<size_t>(max_count)) {
    result.resize(max_count);
  }

  return result;
}

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetRecentlyUsed(
    int max_count) const {
  std::vector<AstraAutofillEntry> result = entries_;

  // Filter out blocked entries.
  std::vector<AstraAutofillEntry> filtered;
  for (const auto& entry : result) {
    if (!entry.is_blocked) {
      filtered.push_back(entry);
    }
  }

  std::sort(filtered.begin(), filtered.end(), CompareByMostRecent);

  if (max_count > 0 && filtered.size() > static_cast<size_t>(max_count)) {
    filtered.resize(max_count);
  }

  return filtered;
}

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetMostUsed(
    int max_count) const {
  std::vector<AstraAutofillEntry> filtered;
  for (const auto& entry : entries_) {
    if (!entry.is_blocked) {
      filtered.push_back(entry);
    }
  }

  std::sort(filtered.begin(), filtered.end(), CompareByMostUsed);

  if (max_count > 0 && filtered.size() > static_cast<size_t>(max_count)) {
    filtered.resize(max_count);
  }

  return filtered;
}

// =========================================================================
// Entry operations
// =========================================================================

void AstraAutofillHelper::AddEntry(const AstraAutofillEntry& entry) {
  if (entry.id.empty()) {
    return;
  }

  // Check if entry with this ID already exists.
  for (auto& existing : entries_) {
    if (existing.id == entry.id) {
      existing = entry;
      NotifyEntryUpdated(entry.id);
      return;
    }
  }

  entries_.push_back(entry);
  NotifyEntryAdded(entry.id);
}

void AstraAutofillHelper::UpdateEntry(const std::string& id,
                                      const AstraAutofillEntry& entry) {
  if (id.empty()) {
    return;
  }

  for (auto& existing : entries_) {
    if (existing.id == id) {
      // Preserve the ID in case the entry has a different ID.
      AstraAutofillEntry updated = entry;
      updated.id = id;
      existing = updated;
      NotifyEntryUpdated(id);
      return;
    }
  }
  // Not found — no-op.
}

void AstraAutofillHelper::RemoveEntry(const std::string& id) {
  if (id.empty()) {
    return;
  }

  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [&id](const AstraAutofillEntry& entry) {
                             return entry.id == id;
                           });
  if (it != entries_.end()) {
    entries_.erase(it, entries_.end());
    NotifyEntryRemoved(id);
  }
}

void AstraAutofillHelper::BlockEntry(const std::string& id) {
  if (id.empty()) {
    return;
  }

  for (auto& entry : entries_) {
    if (entry.id == id && !entry.is_blocked) {
      entry.is_blocked = true;
      NotifyEntryUpdated(id);
      return;
    }
  }
}

void AstraAutofillHelper::UnblockEntry(const std::string& id) {
  if (id.empty()) {
    return;
  }

  for (auto& entry : entries_) {
    if (entry.id == id && entry.is_blocked) {
      entry.is_blocked = false;
      NotifyEntryUpdated(id);
      return;
    }
  }
}

// =========================================================================
// Clear operations
// =========================================================================

void AstraAutofillHelper::ClearAllEntries() {
  if (entries_.empty()) {
    return;
  }
  entries_.clear();
  NotifyDataCleared();
}

void AstraAutofillHelper::ClearAddresses() {
  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [](const AstraAutofillEntry& entry) {
                             return entry.type == AstraAutofillEntryType::kAddress;
                           });
  if (it != entries_.end()) {
    entries_.erase(it, entries_.end());
    NotifyDataCleared();
  }
}

void AstraAutofillHelper::ClearCreditCards() {
  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [](const AstraAutofillEntry& entry) {
                             return entry.type == AstraAutofillEntryType::kCreditCard;
                           });
  if (it != entries_.end()) {
    entries_.erase(it, entries_.end());
    NotifyDataCleared();
  }
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraAutofillHelper::ImportEntries(
    const std::vector<AstraAutofillEntry>& entries) {
  for (const auto& entry : entries) {
    AddEntry(entry);
  }
}

void AstraAutofillHelper::RemoveEntries(
    const std::vector<std::string>& ids) {
  for (const auto& id : ids) {
    RemoveEntry(id);
  }
}

void AstraAutofillHelper::BlockEntries(
    const std::vector<std::string>& ids) {
  for (const auto& id : ids) {
    BlockEntry(id);
  }
}

int AstraAutofillHelper::MergeDuplicateAddresses() {
  // TODO(astra): Use Chromium's AutofillProfileComparator for proper
  //   duplicate detection. For the overlay, we use a simple heuristic:
  //   addresses with the same value (full address string) are duplicates.
  //
  // Chromium: AutofillProfileComparator
  //   (components/autofill/core/browser/autofill_profile_comparator.h)

  int merge_count = 0;
  std::vector<AstraAutofillEntry> addresses = GetAddresses();

  // Compare each pair of addresses.
  for (size_t i = 0; i < addresses.size(); ++i) {
    if (addresses[i].id.empty()) {
      continue;
    }
    for (size_t j = i + 1; j < addresses.size(); ++j) {
      if (addresses[j].id.empty()) {
        continue;
      }
      // Simple heuristic: same value (normalized) means duplicate.
      std::string value_i = base::ToLowerASCII(
          base::TrimWhitespaceASCII(addresses[i].value, base::TRIM_ALL));
      std::string value_j = base::ToLowerASCII(
          base::TrimWhitespaceASCII(addresses[j].value, base::TRIM_ALL));

      if (value_i == value_j && !value_i.empty()) {
        // Merge: keep the one with higher use_count or more recent last_used.
        AstraAutofillEntry& target = addresses[i];
        AstraAutofillEntry& source = addresses[j];

        // Merge use counts.
        target.use_count += source.use_count;

        // Use the more recent last_used.
        if (source.last_used > target.last_used) {
          target.last_used = source.last_used;
        }

        // Update the entry.
        UpdateEntry(target.id, target);

        // Remove the duplicate.
        RemoveEntry(source.id);
        source.id.clear();  // Mark as merged.

        merge_count++;
      }
    }
  }

  return merge_count;
}

// =========================================================================
// Autofill enable settings
// =========================================================================

bool AstraAutofillHelper::IsAutofillEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefAutofillEnabled);
}

void AstraAutofillHelper::SetAutofillEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefAutofillEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefAutofillEnabled, enabled);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::IsAddressAutofillEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAddressAutofillEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefAddressAutofillEnabled);
}

void AstraAutofillHelper::SetAddressAutofillEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefAddressAutofillEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefAddressAutofillEnabled, enabled);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::IsCreditCardAutofillEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultCreditCardAutofillEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefCreditCardAutofillEnabled);
}

void AstraAutofillHelper::SetCreditCardAutofillEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefCreditCardAutofillEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefCreditCardAutofillEnabled, enabled);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::IsAutosignInEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutosignInEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefAutosignInEnabled);
}

void AstraAutofillHelper::SetAutosignInEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefAutosignInEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefAutosignInEnabled, enabled);
  NotifySettingsChanged();
}

// =========================================================================
// Presentation settings
// =========================================================================

bool AstraAutofillHelper::GetShowAutofillPopup() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowAutofillPopup;
  }
  return prefs->GetBoolean(prefs::kPrefShowAutofillPopup);
}

void AstraAutofillHelper::SetShowAutofillPopup(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefShowAutofillPopup) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefShowAutofillPopup, show);
  NotifySettingsChanged();
}

std::string AstraAutofillHelper::GetAutofillPopupPosition() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillPopupPosition;
  }
  return prefs->GetString(prefs::kPrefAutofillPopupPosition);
}

void AstraAutofillHelper::SetAutofillPopupPosition(
    const std::string& position) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  std::string validated = ValidatePopupPosition(position);
  if (prefs->GetString(prefs::kPrefAutofillPopupPosition) == validated) {
    return;
  }

  prefs->SetString(prefs::kPrefAutofillPopupPosition, validated);
  NotifySettingsChanged();
}

int AstraAutofillHelper::GetMaxSuggestions() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillMaxSuggestions;
  }
  return prefs->GetInteger(prefs::kPrefAutofillMaxSuggestions);
}

void AstraAutofillHelper::SetMaxSuggestions(int max_count) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampMaxSuggestions(max_count);
  int current = prefs->GetInteger(prefs::kPrefAutofillMaxSuggestions);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefAutofillMaxSuggestions, clamped);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetShowSuggestionIcons() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSuggestionIcons;
  }
  return prefs->GetBoolean(prefs::kPrefShowSuggestionIcons);
}

void AstraAutofillHelper::SetShowSuggestionIcons(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefShowSuggestionIcons) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefShowSuggestionIcons, show);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetShowSuggestionLabels() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSuggestionLabels;
  }
  return prefs->GetBoolean(prefs::kPrefShowSuggestionLabels);
}

void AstraAutofillHelper::SetShowSuggestionLabels(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefShowSuggestionLabels) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefShowSuggestionLabels, show);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetShowSuggestionSubtext() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowSuggestionSubtext;
  }
  return prefs->GetBoolean(prefs::kPrefShowSuggestionSubtext);
}

void AstraAutofillHelper::SetShowSuggestionSubtext(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefShowSuggestionSubtext) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefShowSuggestionSubtext, show);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetAutofillOnTap() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillOnTap;
  }
  return prefs->GetBoolean(prefs::kPrefAutofillOnTap);
}

void AstraAutofillHelper::SetAutofillOnTap(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefAutofillOnTap) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefAutofillOnTap, enabled);
  NotifySettingsChanged();
}

std::string AstraAutofillHelper::GetSuggestionsSortOrder() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillSuggestionsSortOrder;
  }
  return prefs->GetString(prefs::kPrefAutofillSuggestionsSortOrder);
}

void AstraAutofillHelper::SetSuggestionsSortOrder(const std::string& order) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  std::string validated = ValidateSortOrder(order);
  if (prefs->GetString(prefs::kPrefAutofillSuggestionsSortOrder) == validated) {
    return;
  }

  prefs->SetString(prefs::kPrefAutofillSuggestionsSortOrder, validated);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetShowCreditCardIcons() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultShowCreditCardIcons;
  }
  return prefs->GetBoolean(prefs::kPrefShowCreditCardIcons);
}

void AstraAutofillHelper::SetShowCreditCardIcons(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefShowCreditCardIcons) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefShowCreditCardIcons, show);
  NotifySettingsChanged();
}

bool AstraAutofillHelper::GetAutofillQuickCheckout() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultAutofillQuickCheckout;
  }
  return prefs->GetBoolean(prefs::kPrefAutofillQuickCheckout);
}

void AstraAutofillHelper::SetAutofillQuickCheckout(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefAutofillQuickCheckout) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefAutofillQuickCheckout, enabled);
  NotifySettingsChanged();
}

// =========================================================================
// Utility methods
// =========================================================================

// static
std::string AstraAutofillHelper::FormatCardNumberLastFour(
    const std::string& last_four) {
  if (last_four.empty()) {
    return std::string();
  }
  return "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2 " + last_four;
}

// static
std::string AstraAutofillHelper::GetCardNetworkLabel(
    const std::string& network) {
  std::string lower = base::ToLowerASCII(network);

  if (lower == kCardNetworkVisa) return "Visa";
  if (lower == kCardNetworkMastercard) return "Mastercard";
  if (lower == kCardNetworkAmex) return "American Express";
  if (lower == kCardNetworkDiscover) return "Discover";
  if (lower == kCardNetworkJcb) return "JCB";
  if (lower == kCardNetworkDiners) return "Diners Club";
  if (lower == kCardNetworkUnionPay) return "UnionPay";

  return network;
}

// static
std::string AstraAutofillHelper::GetEntryTypeLabel(
    AstraAutofillEntryType type) {
  switch (type) {
    case AstraAutofillEntryType::kAddress:
      return "Address";
    case AstraAutofillEntryType::kCreditCard:
      return "Credit Card";
    case AstraAutofillEntryType::kPassword:
      return "Password";
    case AstraAutofillEntryType::kEmail:
      return "Email";
    case AstraAutofillEntryType::kPhone:
      return "Phone";
    case AstraAutofillEntryType::kName:
      return "Name";
    case AstraAutofillEntryType::kCustom:
      return "Custom";
  }
  return "Unknown";
}

// static
std::string AstraAutofillHelper::MaskCardNumber(
    const std::string& full_number) {
  // Remove any non-digit characters.
  std::string digits;
  for (char c : full_number) {
    if (base::IsAsciiDigit(c)) {
      digits += c;
    }
  }

  if (digits.empty()) {
    return std::string();
  }

  // Get last 4 digits (or fewer if shorter).
  size_t last_four_count = std::min(digits.size(), size_t{4});
  std::string last_four = digits.substr(digits.size() - last_four_count);

  // Build masked number with bullet characters.
  std::string result;
  size_t mask_count = digits.size() - last_four_count;

  // Group masked digits in groups of 4 for readability.
  for (size_t i = 0; i < mask_count; ++i) {
    if (i > 0 && i % 4 == 0) {
      result += ' ';
    }
    result += "\xE2\x80\xA2";  // bullet character
  }

  if (!last_four.empty()) {
    if (!result.empty()) {
      result += ' ';
    }
    result += last_four;
  }

  return result;
}

// static
std::string AstraAutofillHelper::FormatExpiration(int month, int year) {
  if (month < 1 || month > 12 || year <= 0) {
    return std::string();
  }

  // Format as "MM/YY".
  std::string result;
  if (month < 10) {
    result += '0';
  }
  result += std::to_string(month);
  result += '/';

  // Get last 2 digits of year.
  int yy = year % 100;
  if (yy < 10) {
    result += '0';
  }
  result += std::to_string(yy);

  return result;
}

// static
std::string AstraAutofillHelper::TruncateSuggestion(const std::string& text,
                                                    int max_length) {
  if (max_length <= 0) {
    return text;
  }
  if (max_length < kMinTruncateLength) {
    max_length = kMinTruncateLength;
  }
  if (static_cast<int>(text.size()) <= max_length) {
    return text;
  }

  // Truncate and add ellipsis.
  std::string result = text.substr(0, max_length);
  result += "\xE2\x80\xA6";  // ellipsis
  return result;
}

// =========================================================================
// Observers
// =========================================================================

void AstraAutofillHelper::AddObserver(AstraAutofillObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraAutofillHelper::RemoveObserver(AstraAutofillObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraAutofillHelper::NotifySuggestionShown() {
  for (auto& observer : observers_) {
    observer.OnAutofillSuggestionShown();
  }
}

void AstraAutofillHelper::NotifyEntrySelected(const std::string& entry_id) {
  for (auto& observer : observers_) {
    observer.OnAutofillEntrySelected(entry_id);
  }
}

void AstraAutofillHelper::NotifyEntryAdded(const std::string& entry_id) {
  for (auto& observer : observers_) {
    observer.OnAutofillEntryAdded(entry_id);
  }
}

void AstraAutofillHelper::NotifyEntryRemoved(const std::string& entry_id) {
  for (auto& observer : observers_) {
    observer.OnAutofillEntryRemoved(entry_id);
  }
}

void AstraAutofillHelper::NotifyEntryUpdated(const std::string& entry_id) {
  for (auto& observer : observers_) {
    observer.OnAutofillEntryUpdated(entry_id);
  }
}

void AstraAutofillHelper::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnAutofillSettingsChanged();
  }
}

void AstraAutofillHelper::NotifyProfileChanged() {
  for (auto& observer : observers_) {
    observer.OnAutofillProfileChanged();
  }
}

void AstraAutofillHelper::NotifyDataCleared() {
  for (auto& observer : observers_) {
    observer.OnAutofillDataCleared();
  }
}

// =========================================================================
// Private helpers
// =========================================================================

PrefService* AstraAutofillHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

std::vector<AstraAutofillEntry> AstraAutofillHelper::GetEntriesByType(
    AstraAutofillEntryType type) const {
  std::vector<AstraAutofillEntry> result;
  for (const auto& entry : entries_) {
    if (entry.type == type) {
      result.push_back(entry);
    }
  }
  return result;
}

bool AstraAutofillHelper::HasEntry(const std::string& id) const {
  for (const auto& entry : entries_) {
    if (entry.id == id) {
      return true;
    }
  }
  return false;
}

}  // namespace astra
