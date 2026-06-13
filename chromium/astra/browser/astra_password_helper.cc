#include "astra/browser/astra_password_helper.h"

#include <algorithm>
#include <random>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout. In this overlay repo, the
// types are forward-declared in the header and the real definitions
// come from Chromium at build time.
//
// Chromium owner: PasswordManagerService
//   (chrome/browser/password_manager/password_manager_service.h)
// Chromium owner: PasswordStore
//   (components/password_manager/core/browser/password_store.h)
// Chromium owner: PasswordForm
//   (components/password_manager/core/browser/password_form.h)
// Chromium owner: PasswordHealthChecker
//   (components/password_manager/core/browser/password_health_checker.h)
// Chromium owner: PasswordStrengthEstimator
//   (components/password_manager/core/browser/password_strength/)
// Chromium owner: PasswordGenerator
//   (components/password_manager/core/browser/password_generator.h)
// #include "chrome/browser/password_manager/password_store_factory.h"
// #include "components/password_manager/core/browser/password_store_interface.h"
// #include "components/password_manager/core/browser/password_form.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Maximum number of passwords to show in the sidebar by default.
constexpr size_t kDefaultMaxPasswords = 20;

// Character sets for password generation.
constexpr char kLowerCaseChars[] = "abcdefghijklmnopqrstuvwxyz";
constexpr char kUpperCaseChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr char kDigitChars[] = "0123456789";
constexpr char kSymbolChars[] = "!@#$%^&*()-_=+[]{};:,.<>?";

// Minimum password length for complexity requirements.
constexpr size_t kMinPasswordLength = 12;

// Clamp helper for max sidebar passwords.
int ClampMaxSidebarPasswords(int value) {
  if (value < 1) return 1;
  if (value > 100) return 100;
  return value;
}

}  // namespace

AstraPasswordHelper::AstraPasswordHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing PasswordStore for live updates.
  //   Requires implementing PasswordStoreConsumer or
  //   PasswordStoreObserver.
  //   password_manager::PasswordStoreInterface* store = GetPasswordStore();
  //   if (store) {
  //     store->AddObserver(this);
  //     is_observing_store_ = true;
  //   }
  //
  // Chromium observer: PasswordStoreObserver
  //   (components/password_manager/core/browser/password_store_consumer.h)
}

AstraPasswordHelper::~AstraPasswordHelper() {
  // Observers should already be cleaned up by Shutdown().
  DCHECK(!is_observing_store_);
}

void AstraPasswordHelper::Shutdown() {
  // TODO(astra): Remove observer from PasswordStore.
  //   password_manager::PasswordStoreInterface* store = GetPasswordStore();
  //   if (store && is_observing_store_) {
  //     store->RemoveObserver(this);
  //     is_observing_store_ = false;
  //   }
  is_observing_store_ = false;
  profile_ = nullptr;
}

// =========================================================================
// Password list queries
// =========================================================================

std::vector<AstraPasswordEntry> AstraPasswordHelper::GetSavedPasswords(
    size_t max_count) const {
  std::vector<AstraPasswordEntry> result;

  // TODO(astra): Query PasswordStore for saved passwords.
  //   The proper implementation would:
  //     1. Get the password store via PasswordStoreFactory.
  //     2. Call GetAutofillableLogins() with a consumer callback.
  //     3. On results received, convert PasswordForms to AstraPasswordEntry
  //        projections and notify observers.
  //
  //   Since the password store is async, GetSavedPasswords() would ideally
  //   also be async. For now, we return an empty list as a placeholder
  //   and document the async pattern.
  //
  //   Chromium API:
  //     password_manager::PasswordStoreInterface::GetAutofillableLogins
  //       (components/password_manager/core/browser/password_store_interface.h)
  //
  //   Typical pattern:
  //     class AstraPasswordHelper ... : public password_manager::PasswordStoreConsumer {
  //       ...
  //       void OnGetPasswordStoreResults(
  //         std::vector<std::unique_ptr<password_manager::PasswordForm>> results) override;
  //     };
  //
  //   On the caller side (sidebar view), the view would call
  //   password_helper->GetSavedPasswords() which triggers an async query,
  //   and the helper notifies the view via observer when results arrive.

  password_manager::PasswordStoreInterface* store = GetPasswordStore();
  if (!store) {
    // Password store not available — return empty projection.
    return result;
  }

  // TODO(astra): Replace placeholder with real password store query.
  // For now, the helper returns an empty list — the sidebar will show
  // an empty state with a "Manage passwords" link.

  // Cap result size.
  if (max_count == 0) {
    max_count = kDefaultMaxPasswords;
  }
  if (result.size() > max_count) {
    result.resize(max_count);
  }

  return result;
}

std::vector<AstraPasswordEntry> AstraPasswordHelper::SearchPasswords(
    const std::u16string& query,
    size_t max_count) const {
  std::vector<AstraPasswordEntry> result;

  if (query.empty()) {
    return GetSavedPasswords(max_count);
  }

  // TODO(astra): Implement real search.
  //   Options:
  //     1. Filter results from GetAutofillableLogins (simple but O(n)).
  //     2. Use PasswordStore's search APIs if available.
  //
  //   For the sidebar search box (which is interactive), option 1 is
  //   acceptable since we only show ~20 items max.
  //
  //   The search should match:
  //     - Site display name (substring, case-insensitive)
  //     - Username (substring, case-insensitive)
  //     - Signon realm / URL (substring, case-insensitive)
  //
  //   Chromium reference: PasswordManagerUI::HandleSearch
  //     (chrome/browser/ui/webui/password_manager/password_manager_ui.cc)

  // Placeholder: return empty results.
  // Real implementation would filter GetSavedPasswords() by the query.

  return result;
}

size_t AstraPasswordHelper::GetPasswordCount() const {
  // TODO(astra): Get actual count from PasswordStore.
  //   Options:
  //     1. Query all passwords and count (simple but O(n) per call).
  //     2. Maintain a cached count updated via store observers.
  //   Option 2 is better for performance since the sidebar header shows
  //   the count and we don't want to query everything just for a number.
  //
  //   Chromium owner: PasswordStoreInterface
  //
  // For now, return 0 as a placeholder.
  return 0;
}

size_t AstraPasswordHelper::GetSavedPasswordsCount() const {
  return GetPasswordCount();
}

size_t AstraPasswordHelper::GetCompromisedPasswordsCount() const {
  // TODO(astra): Query PasswordHealthChecker for compromised passwords.
  //
  // Chromium owner: PasswordHealthChecker
  //   (components/password_manager/core/browser/password_health_checker.h)
  //
  //   PasswordHealthChecker provides:
  //     - GetCompromisedCredentialsCount()
  //     - GetWeakCredentialsCount()
  //     - GetReusedCredentialsCount()
  //
  // In the overlay, return 0 as a placeholder.
  return 0;
}

size_t AstraPasswordHelper::GetWeakPasswordsCount() const {
  // TODO(astra): Query PasswordHealthChecker for weak passwords.
  // Chromium owner: PasswordHealthChecker
  return 0;
}

size_t AstraPasswordHelper::GetReusedPasswordsCount() const {
  // TODO(astra): Query PasswordHealthChecker for reused passwords.
  // Chromium owner: PasswordHealthChecker
  return 0;
}

AstraPasswordHealthStats AstraPasswordHelper::GetPasswordHealthStats() const {
  AstraPasswordHealthStats stats;

  stats.total_passwords = GetPasswordCount();
  stats.compromised_count = GetCompromisedPasswordsCount();
  stats.weak_count = GetWeakPasswordsCount();
  stats.reused_count = GetReusedPasswordsCount();
  // TODO(astra): Get blocked site count from PasswordStore.
  stats.blocked_count = 0;

  return stats;
}

std::vector<AstraPasswordEntry> AstraPasswordHelper::GetDisplayPasswords(
    size_t max_count) const {
  // Get all saved passwords and apply filter + sort based on current settings.
  // TODO(astra): In a real implementation, we'd query the store and apply
  //   sorting/filtering. For the overlay, we return what GetSavedPasswords
  //   returns (which is empty) since there's no real store.
  auto entries = GetSavedPasswords(max_count);

  // Apply filter.
  AstraPasswordFilter filter = GetFilter();
  if (filter != AstraPasswordFilter::kAll) {
    std::vector<AstraPasswordEntry> filtered;
    for (const auto& entry : entries) {
      switch (filter) {
        case AstraPasswordFilter::kCompromised:
          if (entry.is_compromised) filtered.push_back(entry);
          break;
        case AstraPasswordFilter::kWeak:
          if (entry.is_weak) filtered.push_back(entry);
          break;
        case AstraPasswordFilter::kReused:
          if (entry.is_reused) filtered.push_back(entry);
          break;
        default:
          filtered.push_back(entry);
          break;
      }
    }
    entries = std::move(filtered);
  }

  // Apply sort.
  AstraPasswordSortOrder sort_order = GetSortOrder();
  switch (sort_order) {
    case AstraPasswordSortOrder::kAlphabetical:
      std::sort(entries.begin(), entries.end(),
                [](const AstraPasswordEntry& a, const AstraPasswordEntry& b) {
                  return a.site_display_name < b.site_display_name;
                });
      break;
    case AstraPasswordSortOrder::kLastUsed:
      std::sort(entries.begin(), entries.end(),
                [](const AstraPasswordEntry& a, const AstraPasswordEntry& b) {
                  return a.last_used_time > b.last_used_time;
                });
      break;
    case AstraPasswordSortOrder::kDateCreated:
      std::sort(entries.begin(), entries.end(),
                [](const AstraPasswordEntry& a, const AstraPasswordEntry& b) {
                  return a.date_created > b.date_created;
                });
      break;
  }

  // Cap result size.
  if (max_count > 0 && entries.size() > max_count) {
    entries.resize(max_count);
  }

  return entries;
}

std::vector<AstraPasswordEntry> AstraPasswordHelper::GetPasswordsForURL(
    const GURL& url,
    size_t max_count) const {
  // TODO(astra): Query PasswordStore for passwords matching this URL.
  //   In Chromium, this would use PasswordStore::GetLoginsForUrl() or
  //   similar. For the overlay, we filter from the full list.
  auto all = GetSavedPasswords(0);
  if (!url.is_valid()) {
    return all;
  }

  std::vector<AstraPasswordEntry> matches;
  std::u16string host = base::UTF8ToUTF16(url.host());
  for (const auto& entry : all) {
    if (entry.url.host() == url.host()) {
      matches.push_back(entry);
    }
    if (max_count > 0 && matches.size() >= max_count) {
      break;
    }
  }
  return matches;
}

absl::optional<AstraPasswordEntry> AstraPasswordHelper::GetPasswordById(
    const std::string& id) const {
  // TODO(astra): Look up by ID in PasswordStore.
  //   For the overlay, iterate the saved passwords.
  auto all = GetSavedPasswords(0);
  for (const auto& entry : all) {
    if (entry.id == id) {
      return entry;
    }
  }
  return absl::nullopt;
}

size_t AstraPasswordHelper::GetFilteredPasswordCount() const {
  // Returns the count after applying the current filter.
  AstraPasswordFilter filter = GetFilter();
  switch (filter) {
    case AstraPasswordFilter::kAll:
      return GetPasswordCount();
    case AstraPasswordFilter::kCompromised:
      return GetCompromisedPasswordsCount();
    case AstraPasswordFilter::kWeak:
      return GetWeakPasswordsCount();
    case AstraPasswordFilter::kReused:
      return GetReusedPasswordsCount();
  }
  return GetPasswordCount();
}

// =========================================================================
// Password strength
// =========================================================================

AstraPasswordStrength AstraPasswordHelper::GetPasswordStrength(
    const std::u16string& password) {
  int percent = GetPasswordStrengthPercent(password);

  if (percent <= 20) return AstraPasswordStrength::kVeryWeak;
  if (percent <= 40) return AstraPasswordStrength::kWeak;
  if (percent <= 60) return AstraPasswordStrength::kMedium;
  if (percent <= 80) return AstraPasswordStrength::kStrong;
  return AstraPasswordStrength::kVeryStrong;
}

int AstraPasswordHelper::GetPasswordStrengthPercent(
    const std::u16string& password) {
  // TODO(astra): Delegate to Chromium's PasswordStrengthEstimator.
  //   For the overlay, we use a heuristic based on:
  //     1. Length (0-40 points)
  //     2. Character variety (uppercase, lowercase, digits, symbols) (0-40 points)
  //     3. Pattern penalties (all same chars, sequential, common patterns)
  //
  // Chromium owner: PasswordStrengthEstimator
  //   (components/password_manager/core/browser/password_strength/)

  if (password.empty()) {
    return 0;
  }

  int score = 0;

  // Length score: 0-40 points.
  //   < 4 chars: 0 points
  //   8 chars: ~20 points
  //   12 chars: ~30 points
  //   16+ chars: 40 points
  size_t length = password.size();
  if (length >= 16) {
    score += 40;
  } else if (length >= 12) {
    score += 30 + (length - 12) * 2.5;  // 30-40
  } else if (length >= 8) {
    score += 20 + (length - 8) * 2.5;   // 20-30
  } else if (length >= 4) {
    score += 5 + (length - 4) * 3.75;   // 5-20
  } else {
    score += 0;
  }

  // Character variety: up to 40 points (10 per category).
  bool has_lower = false, has_upper = false, has_digit = false, has_symbol = false;
  for (char16_t c : password) {
    if (base::IsAsciiLower(c)) has_lower = true;
    else if (base::IsAsciiUpper(c)) has_upper = true;
    else if (base::IsAsciiDigit(c)) has_digit = true;
    else has_symbol = true;
  }

  int variety_score = 0;
  if (has_lower) variety_score += 10;
  if (has_upper) variety_score += 10;
  if (has_digit) variety_score += 10;
  if (has_symbol) variety_score += 10;
  score += variety_score;

  // Bonus for mixed character types + length (diversity bonus).
  int categories = (has_lower ? 1 : 0) + (has_upper ? 1 : 0) +
                   (has_digit ? 1 : 0) + (has_symbol ? 1 : 0);
  if (categories >= 3 && length >= 10) {
    score += 10;  // Diversity bonus
  }
  if (categories >= 4 && length >= 14) {
    score += 10;  // Strong diversity bonus
  }

  // Pattern penalty: all same characters.
  bool all_same = true;
  char16_t first = password[0];
  for (char16_t c : password) {
    if (c != first) {
      all_same = false;
      break;
    }
  }
  if (all_same) {
    score -= 30;
  }

  // Penalty for sequential characters (e.g., "abcdef", "12345").
  bool is_sequential = true;
  for (size_t i = 1; i < password.size(); ++i) {
    if (password[i] != password[i - 1] + 1) {
      is_sequential = false;
      break;
    }
  }
  if (is_sequential && password.size() >= 4) {
    score -= 20;
  }

  // Clamp to 0-100.
  if (score < 0) score = 0;
  if (score > 100) score = 100;

  return score;
}

std::u16string AstraPasswordHelper::GetPasswordStrengthLabel(
    AstraPasswordStrength strength) {
  switch (strength) {
    case AstraPasswordStrength::kVeryWeak:
      return u"Very Weak";
    case AstraPasswordStrength::kWeak:
      return u"Weak";
    case AstraPasswordStrength::kMedium:
      return u"Medium";
    case AstraPasswordStrength::kStrong:
      return u"Strong";
    case AstraPasswordStrength::kVeryStrong:
      return u"Very Strong";
  }
  return u"";
}

SkColor AstraPasswordHelper::GetPasswordStrengthColor(
    AstraPasswordStrength strength) {
  // Color hints for password strength display.
  // These are placeholder colors — in a real Chromium build, use
  // color IDs from the color provider system.
  // TODO(astra): Use proper color IDs from ui/color/color_id.h
  //   or astra/ui/color/astra_color_ids.h.
  switch (strength) {
    case AstraPasswordStrength::kVeryWeak:
      return SK_ColorRED;
    case AstraPasswordStrength::kWeak:
      return SkColorSetRGB(0xFF, 0x6B, 0x6B);  // Light red
    case AstraPasswordStrength::kMedium:
      return SkColorSetRGB(0xFF, 0xC1, 0x07);  // Amber/yellow
    case AstraPasswordStrength::kStrong:
      return SkColorSetRGB(0x34, 0xD3, 0x99);  // Green-ish
    case AstraPasswordStrength::kVeryStrong:
      return SkColorSetRGB(0x1E, 0x8E, 0x3E);  // Dark green
  }
  return SK_ColorGRAY;
}

// =========================================================================
// Password generator
// =========================================================================

std::u16string AstraPasswordHelper::GeneratePassword(size_t length,
                                                     bool include_symbols) {
  // TODO(astra): Use Chromium's PasswordGenerator when building against
  //   a full Chromium checkout.  For the overlay, we provide a basic
  //   implementation using standard random number generation.
  //
  // Chromium owner: PasswordGenerator
  //   (components/password_manager/core/browser/password_generator.h)

  if (length == 0) {
    return std::u16string();
  }

  // Build the character pool.
  std::string pool = std::string(kLowerCaseChars) + kUpperCaseChars + kDigitChars;
  if (include_symbols) {
    pool += kSymbolChars;
  }

  // Use a random device for entropy.
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);

  std::string result;
  result.reserve(length);

  // Ensure at least one character from each required category.
  if (length >= 3) {
    // At least one lowercase.
    std::uniform_int_distribution<size_t> lower_dist(
        0, std::string_view(kLowerCaseChars).size() - 1);
    result += kLowerCaseChars[lower_dist(gen)];

    // At least one uppercase.
    std::uniform_int_distribution<size_t> upper_dist(
        0, std::string_view(kUpperCaseChars).size() - 1);
    result += kUpperCaseChars[upper_dist(gen)];

    // At least one digit.
    std::uniform_int_distribution<size_t> digit_dist(
        0, std::string_view(kDigitChars).size() - 1);
    result += kDigitChars[digit_dist(gen)];
  }

  // Fill the rest from the full pool.
  while (result.size() < length) {
    result += pool[dist(gen)];
  }

  // Shuffle to avoid the guaranteed categories being at the start.
  std::shuffle(result.begin(), result.end(), gen);

  return base::UTF8ToUTF16(result);
}

bool AstraPasswordHelper::MeetsComplexityRequirements(
    const std::u16string& password) {
  if (password.size() < kMinPasswordLength) {
    return false;
  }

  bool has_lower = false, has_upper = false, has_digit = false;
  for (char16_t c : password) {
    if (base::IsAsciiLower(c)) has_lower = true;
    else if (base::IsAsciiUpper(c)) has_upper = true;
    else if (base::IsAsciiDigit(c)) has_digit = true;
  }

  // Require at least 3 of: lowercase, uppercase, digit, symbol.
  int categories = (has_lower ? 1 : 0) + (has_upper ? 1 : 0) +
                   (has_digit ? 1 : 0);
  // Count symbols (anything not alphanumeric).
  bool has_symbol = false;
  for (char16_t c : password) {
    if (!base::IsAsciiAlphaNumeric(c)) {
      has_symbol = true;
      break;
    }
  }
  if (has_symbol) categories++;

  return categories >= 3;
}

// =========================================================================
// Password operations
// =========================================================================

bool AstraPasswordHelper::CopyPasswordToClipboard(
    const AstraPasswordEntry& entry) const {
  // TODO(astra): Implement real password copy using Chromium's password
  //   manager and clipboard system.
  //
  //   The proper flow:
  //     1. Retrieve the password value from PasswordStore (may trigger
  //        biometric / master password reauth).
  //     2. Write the password to the system clipboard using
  //        ui::Clipboard::WriteText().
  //     3. Optionally set a clipboard clear timeout for security.
  //
  //   Security considerations:
  //     - Never log the password value.
  //     - Never store it in Astra-owned memory longer than the copy op.
  //     - Delegate reauth to Chromium's password manager.
  //     - Consider using a secure clipboard if available on the platform.
  //
  //   Chromium APIs:
  //     - password_manager::PasswordStoreInterface::GetAutofillableLogins
  //     - ui::Clipboard::GetForCurrentThread()
  //     - ui::Clipboard::WriteText()
  //
  //   Chromium owner: PasswordManagerService (chrome/browser/password_manager/)
  //   Chromium clipboard: ui/base/clipboard/clipboard.h
  //
  //   Reference implementation: PasswordManagerUI::HandleCopyPassword
  //     (chrome/browser/ui/webui/password_manager/password_manager_ui.cc)

  // Placeholder: return false to indicate not yet implemented.
  // The sidebar UI will show a "Copied!" or error state accordingly.
  return false;
}

bool AstraPasswordHelper::CopyUsernameToClipboard(
    const AstraPasswordEntry& entry) const {
  // Copy the username to the system clipboard.
  //
  // TODO(astra): Implement real username copy using Chromium's clipboard
  //   system. The username is not sensitive in the same way as the password,
  //   but we still use the standard clipboard API for consistency.
  //
  //   In the full Chromium build:
  //     ui::Clipboard::GetForCurrentThread()->WriteText(
  //         ui::ClipboardBuffer::kCopyPaste,
  //         base::UTF16ToUTF8(entry.username));
  //
  // Placeholder: return false for now.
  // The UI will show appropriate feedback when implemented.
  if (entry.username.empty()) {
    return false;
  }
  return false;
}

void AstraPasswordHelper::OpenPasswordSettings(Profile* profile) const {
  if (!profile) {
    return;
  }

  // TODO(astra): Navigate to chrome://settings/passwords.
  //   This should use Browser::OpenURL with the settings URL.
  //   The helper doesn't have direct access to Browser, so this method
  //   is available for the UI layer to call — the UI layer is
  //   responsible for actually navigating.
  //
  //   Alternatively, we could have the UI layer open the URL directly
  //   since it has Browser access. The helper provides the URL as a
  //   canonical value.
  //
  //   Chromium WebUI: chrome://settings/passwords
  //     (chrome/browser/resources/settings/passwords_page/)
  //
  //   Patch point: None needed — standard WebUI navigation.
}

void AstraPasswordHelper::OpenPasswordCheck(Profile* profile) const {
  if (!profile) {
    return;
  }

  // TODO(astra): Navigate to chrome://settings/passwords/check.
  //   This opens the Password Checkup page which runs a health check
  //   on all saved passwords.
  //
  //   Chromium WebUI: chrome://settings/passwords/check
  //     (chrome/browser/resources/settings/passwords_page/)
}

// =========================================================================
// Bulk operations
// =========================================================================

bool AstraPasswordHelper::IsBulkExportAvailable() const {
  // TODO(astra): Check if password export is available (not blocked by
  //   policy).
  //
  // Chromium owner: PasswordManagerSettingsService
  //   (chrome/browser/password_manager/password_manager_settings_service.h)
  //
  // Policy: PasswordManagerExportEnabled
  //   (components/password_manager/core/common/password_manager_pref_names.h)
  //
  // In the overlay, return true as a placeholder (assumes available).
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return true;  // Default: available
  }
  return true;
}

bool AstraPasswordHelper::ExportPasswords(
    const std::string& file_path) const {
  if (!GetPasswordStore()) {
    return false;
  }

  // TODO(astra): Implement using PasswordManager's export functionality.
  //
  // Chromium owner: PasswordExporter
  //   (components/password_manager/core/browser/export/password_exporter.h)
  //
  //   Typical flow:
  //     1. Call PasswordExporter::Export() with destination path.
  //     2. Listen for completion via ExportProgressObserver.
  //     3. Notify observers when export completes.
  //
  // In the overlay, return false as a stub.
  return false;
}

// =========================================================================
// Sort order settings
// =========================================================================

AstraPasswordSortOrder AstraPasswordHelper::GetSortOrder() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return AstraPasswordSortOrder::kAlphabetical;
  }
  std::string order = prefs->GetString(prefs::kPrefPasswordSortOrder);
  if (order == "last_used") {
    return AstraPasswordSortOrder::kLastUsed;
  }
  if (order == "date_created") {
    return AstraPasswordSortOrder::kDateCreated;
  }
  return AstraPasswordSortOrder::kAlphabetical;
}

void AstraPasswordHelper::SetSortOrder(AstraPasswordSortOrder order) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  std::string order_str;
  switch (order) {
    case AstraPasswordSortOrder::kAlphabetical:
      order_str = "alphabetical";
      break;
    case AstraPasswordSortOrder::kLastUsed:
      order_str = "last_used";
      break;
    case AstraPasswordSortOrder::kDateCreated:
      order_str = "date_created";
      break;
  }

  if (prefs->GetString(prefs::kPrefPasswordSortOrder) == order_str) {
    return;
  }

  prefs->SetString(prefs::kPrefPasswordSortOrder, order_str);
  NotifyPasswordSettingsChanged();
}

AstraPasswordSortOrder AstraPasswordHelper::CycleSortOrder() {
  AstraPasswordSortOrder current = GetSortOrder();
  AstraPasswordSortOrder next;
  switch (current) {
    case AstraPasswordSortOrder::kAlphabetical:
      next = AstraPasswordSortOrder::kLastUsed;
      break;
    case AstraPasswordSortOrder::kLastUsed:
      next = AstraPasswordSortOrder::kDateCreated;
      break;
    case AstraPasswordSortOrder::kDateCreated:
      next = AstraPasswordSortOrder::kAlphabetical;
      break;
  }
  SetSortOrder(next);
  return GetSortOrder();
}

// =========================================================================
// Filter settings
// =========================================================================

AstraPasswordFilter AstraPasswordHelper::GetFilter() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return AstraPasswordFilter::kAll;
  }
  std::string filter = prefs->GetString(prefs::kPrefPasswordFilter);
  if (filter == "compromised") {
    return AstraPasswordFilter::kCompromised;
  }
  if (filter == "weak") {
    return AstraPasswordFilter::kWeak;
  }
  if (filter == "reused") {
    return AstraPasswordFilter::kReused;
  }
  return AstraPasswordFilter::kAll;
}

void AstraPasswordHelper::SetFilter(AstraPasswordFilter filter) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  std::string filter_str;
  switch (filter) {
    case AstraPasswordFilter::kAll:
      filter_str = "all";
      break;
    case AstraPasswordFilter::kCompromised:
      filter_str = "compromised";
      break;
    case AstraPasswordFilter::kWeak:
      filter_str = "weak";
      break;
    case AstraPasswordFilter::kReused:
      filter_str = "reused";
      break;
  }

  if (prefs->GetString(prefs::kPrefPasswordFilter) == filter_str) {
    return;
  }

  prefs->SetString(prefs::kPrefPasswordFilter, filter_str);
  NotifyPasswordSettingsChanged();
}

AstraPasswordFilter AstraPasswordHelper::CycleFilter() {
  AstraPasswordFilter current = GetFilter();
  AstraPasswordFilter next;
  switch (current) {
    case AstraPasswordFilter::kAll:
      next = AstraPasswordFilter::kCompromised;
      break;
    case AstraPasswordFilter::kCompromised:
      next = AstraPasswordFilter::kWeak;
      break;
    case AstraPasswordFilter::kWeak:
      next = AstraPasswordFilter::kReused;
      break;
    case AstraPasswordFilter::kReused:
      next = AstraPasswordFilter::kAll;
      break;
  }
  SetFilter(next);
  return GetFilter();
}

// =========================================================================
// Grouping settings
// =========================================================================

AstraPasswordGroupBy AstraPasswordHelper::GetGroupBy() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return AstraPasswordGroupBy::kNone;
  }
  std::string group_by = prefs->GetString(prefs::kPrefPasswordGroupBy);
  if (group_by == "site") {
    return AstraPasswordGroupBy::kSite;
  }
  if (group_by == "account") {
    return AstraPasswordGroupBy::kAccount;
  }
  return AstraPasswordGroupBy::kNone;
}

void AstraPasswordHelper::SetGroupBy(AstraPasswordGroupBy group_by) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  std::string group_str;
  switch (group_by) {
    case AstraPasswordGroupBy::kNone:
      group_str = "none";
      break;
    case AstraPasswordGroupBy::kSite:
      group_str = "site";
      break;
    case AstraPasswordGroupBy::kAccount:
      group_str = "account";
      break;
  }

  if (prefs->GetString(prefs::kPrefPasswordGroupBy) == group_str) {
    return;
  }

  prefs->SetString(prefs::kPrefPasswordGroupBy, group_str);
  NotifyPasswordSettingsChanged();
}

AstraPasswordGroupBy AstraPasswordHelper::CycleGroupBy() {
  AstraPasswordGroupBy current = GetGroupBy();
  AstraPasswordGroupBy next;
  switch (current) {
    case AstraPasswordGroupBy::kNone:
      next = AstraPasswordGroupBy::kSite;
      break;
    case AstraPasswordGroupBy::kSite:
      next = AstraPasswordGroupBy::kAccount;
      break;
    case AstraPasswordGroupBy::kAccount:
      next = AstraPasswordGroupBy::kNone;
      break;
  }
  SetGroupBy(next);
  return GetGroupBy();
}

// =========================================================================
// Password visibility settings
// =========================================================================

bool AstraPasswordHelper::GetHidePasswordsByDefault() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordHideByDefault;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordHideByDefault);
}

void AstraPasswordHelper::SetHidePasswordsByDefault(bool hide) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordHideByDefault) == hide) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordHideByDefault, hide);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::ToggleHidePasswordsByDefault() {
  bool new_state = !GetHidePasswordsByDefault();
  SetHidePasswordsByDefault(new_state);
  return GetHidePasswordsByDefault();
}

// =========================================================================
// Presentation settings
// =========================================================================

bool AstraPasswordHelper::GetShowPasswordSuggestions() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordShowSuggestions;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordShowSuggestions);
}

void AstraPasswordHelper::SetShowPasswordSuggestions(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordShowSuggestions) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordShowSuggestions, show);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::ToggleShowPasswordSuggestions() {
  bool new_state = !GetShowPasswordSuggestions();
  SetShowPasswordSuggestions(new_state);
  return GetShowPasswordSuggestions();
}

bool AstraPasswordHelper::GetAutoFillEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordAutoFillEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordAutoFillEnabled);
}

void AstraPasswordHelper::SetAutoFillEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordAutoFillEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordAutoFillEnabled, enabled);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::ToggleAutoFillEnabled() {
  bool new_state = !GetAutoFillEnabled();
  SetAutoFillEnabled(new_state);
  return GetAutoFillEnabled();
}

bool AstraPasswordHelper::GetPasswordManagerShortcut() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordManagerShortcut;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordManagerShortcut);
}

void AstraPasswordHelper::SetPasswordManagerShortcut(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordManagerShortcut) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordManagerShortcut, show);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::TogglePasswordManagerShortcut() {
  bool new_state = !GetPasswordManagerShortcut();
  SetPasswordManagerShortcut(new_state);
  return GetPasswordManagerShortcut();
}

bool AstraPasswordHelper::GetShowPasswordHealth() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordShowHealth;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordShowHealth);
}

void AstraPasswordHelper::SetShowPasswordHealth(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordShowHealth) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordShowHealth, show);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::ToggleShowPasswordHealth() {
  bool new_state = !GetShowPasswordHealth();
  SetShowPasswordHealth(new_state);
  return GetShowPasswordHealth();
}

bool AstraPasswordHelper::GetBreachAlertsEnabled() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordBreachAlertsEnabled;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordBreachAlertsEnabled);
}

void AstraPasswordHelper::SetBreachAlertsEnabled(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefPasswordBreachAlertsEnabled) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefPasswordBreachAlertsEnabled, enabled);
  NotifyPasswordSettingsChanged();
}

bool AstraPasswordHelper::ToggleBreachAlerts() {
  bool new_state = !GetBreachAlertsEnabled();
  SetBreachAlertsEnabled(new_state);
  return GetBreachAlertsEnabled();
}

int AstraPasswordHelper::GetMaxSidebarPasswords() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultPasswordMaxSidebarPasswords;
  }
  return prefs->GetInteger(prefs::kPrefPasswordMaxSidebarPasswords);
}

void AstraPasswordHelper::SetMaxSidebarPasswords(int max_count) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampMaxSidebarPasswords(max_count);
  int current = prefs->GetInteger(prefs::kPrefPasswordMaxSidebarPasswords);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefPasswordMaxSidebarPasswords, clamped);
  NotifyPasswordSettingsChanged();
}

// =========================================================================
// Breach detection state
// =========================================================================

bool AstraPasswordHelper::HasCompromisedPasswords() const {
  return GetCompromisedPasswordsCount() > 0;
}

bool AstraPasswordHelper::IsBreachDetected() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return false;
  }
  return prefs->GetBoolean(prefs::kPrefPasswordBreachDetected);
}

void AstraPasswordHelper::AcknowledgeBreach() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (!prefs->GetBoolean(prefs::kPrefPasswordBreachDetected)) {
    return;  // Nothing to acknowledge.
  }

  prefs->SetBoolean(prefs::kPrefPasswordBreachDetected, false);
  NotifyPasswordSettingsChanged();
}

// =========================================================================
// Observers
// =========================================================================

void AstraPasswordHelper::AddObserver(
    AstraPasswordHelperObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPasswordHelper::RemoveObserver(
    AstraPasswordHelperObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraPasswordHelper::NotifyPasswordsChanged() {
  for (auto& observer : observers_) {
    observer.OnPasswordsChanged();
  }
}

void AstraPasswordHelper::NotifyPasswordSaved(
    const AstraPasswordEntry& entry) {
  for (auto& observer : observers_) {
    observer.OnPasswordSaved(entry);
  }
  // Also fire the general change notification.
  NotifyPasswordsChanged();
}

void AstraPasswordHelper::NotifyPasswordUpdated(
    const AstraPasswordEntry& entry) {
  for (auto& observer : observers_) {
    observer.OnPasswordUpdated(entry);
  }
  NotifyPasswordsChanged();
}

void AstraPasswordHelper::NotifyPasswordDeleted(
    const AstraPasswordEntry& entry) {
  for (auto& observer : observers_) {
    observer.OnPasswordDeleted(entry);
  }
  NotifyPasswordsChanged();
}

void AstraPasswordHelper::NotifyPasswordSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnPasswordSettingsChanged();
  }
}

void AstraPasswordHelper::NotifyPasswordHealthChanged() {
  for (auto& observer : observers_) {
    observer.OnPasswordHealthChanged();
  }
}

void AstraPasswordHelper::NotifyPasswordBreachDetected(
    size_t compromised_count) {
  // Set the breach detected flag.
  PrefService* prefs = GetPrefs();
  if (prefs) {
    prefs->SetBoolean(prefs::kPrefPasswordBreachDetected, true);
  }

  for (auto& observer : observers_) {
    observer.OnPasswordBreachDetected(compromised_count);
  }
  // Also fire health change since breach affects health stats.
  NotifyPasswordHealthChanged();
}

// =========================================================================
// Password store access
// =========================================================================

password_manager::PasswordStoreInterface*
AstraPasswordHelper::GetPasswordStore() const {
  if (!profile_) {
    return nullptr;
  }

  // TODO(astra): Use PasswordStoreFactory::GetForProfile() when building
  //   against the full Chromium source tree. In the overlay, we return
  //   nullptr as a placeholder since the real service isn't linked.
  //
  // Chromium factory: PasswordStoreFactory
  //   (chrome/browser/password_manager/password_store_factory.h)
  // The password store is a BrowserContextKeyedService, one per profile.
  //
  // Patch point: None needed — we just call the existing factory.

  return nullptr;
}

PrefService* AstraPasswordHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

}  // namespace astra
