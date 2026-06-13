// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/password_manager/astra_password_manager_model.h"

#include <algorithm>
#include <map>
#include <set>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace astra {

namespace {

// Case-insensitive substring match of |needle| in |haystack|.
bool CaseInsensitiveContains(const std::u16string& haystack,
                             const std::u16string& needle) {
  if (needle.empty()) {
    return true;
  }
  return base::StrContains(base::ToLowerASCII(haystack),
                           base::ToLowerASCII(needle));
}

// Case-insensitive substring match for UTF8 string with UTF16 needle.
bool CaseInsensitiveContainsUtf8(const std::string& haystack_utf8,
                                 const std::u16string& needle) {
  if (needle.empty()) {
    return true;
  }
  std::u16string haystack = base::UTF8ToUTF16(haystack_utf8);
  return CaseInsensitiveContains(haystack, needle);
}

// Get the first letter of the site name, uppercase.
std::u16string GetFirstLetter(const std::u16string& site_name) {
  if (site_name.empty()) {
    return u"#";
  }
  char16_t first = base::ToUpperASCII(site_name[0]);
  if ((first >= u'A' && first <= u'Z') || (first >= u'0' && first <= u'9')) {
    return std::u16string(1, first);
  }
  return u"#";
}

}  // namespace

// ===========================================================================
// AstraPasswordManagerModel
// ===========================================================================

AstraPasswordManagerModel::AstraPasswordManagerModel() = default;

AstraPasswordManagerModel::~AstraPasswordManagerModel() {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnPasswordManagerModelShutdown();
  }
}

// -- Observer management ----------------------------------------------------

void AstraPasswordManagerModel::AddObserver(
    AstraPasswordManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPasswordManagerModel::RemoveObserver(
    AstraPasswordManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Password data ---------------------------------------------------------

const std::vector<AstraPasswordEntry>&
AstraPasswordManagerModel::GetPasswords() const {
  return all_passwords_;
}

const AstraPasswordEntry* AstraPasswordManagerModel::GetPassword(
    const std::string& id) const {
  auto it = FindEntry(id);
  if (it != all_passwords_.end()) {
    return &(*it);
  }
  return nullptr;
}

size_t AstraPasswordManagerModel::GetCount() const {
  return all_passwords_.size();
}

const std::vector<AstraPasswordGroup>&
AstraPasswordManagerModel::GetGroupedPasswords() const {
  return grouped_passwords_;
}

// -- Search -----------------------------------------------------------------

void AstraPasswordManagerModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  ApplyFilters();
  NotifySearchChanged();
  NotifyPasswordsChanged();
}

const std::u16string& AstraPasswordManagerModel::GetSearchQuery() const {
  return search_query_;
}

// -- Filtering --------------------------------------------------------------

void AstraPasswordManagerModel::SetFilter(AstraPasswordFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyPasswordsChanged();
}

AstraPasswordFilter AstraPasswordManagerModel::GetFilter() const {
  return filter_;
}

// -- Category filter --------------------------------------------------------

void AstraPasswordManagerModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyPasswordsChanged();
}

const std::string& AstraPasswordManagerModel::GetCategoryFilter() const {
  return category_filter_;
}

std::vector<std::string> AstraPasswordManagerModel::GetCategories() const {
  std::set<std::string> unique_categories;
  for (const auto& entry : all_passwords_) {
    if (!entry.category.empty()) {
      unique_categories.insert(entry.category);
    }
  }
  return std::vector<std::string>(unique_categories.begin(),
                                  unique_categories.end());
}

// -- Entry manipulation -----------------------------------------------------

std::string AstraPasswordManagerModel::AddPassword(
    const std::u16string& site_name,
    const std::u16string& username,
    const std::string& password,
    const std::string& url,
    const std::string& category) {
  AstraPasswordEntry entry;
  entry.id = GenerateId();
  entry.site_name = site_name;
  entry.username = username;
  entry.password = password;
  entry.url = url;
  entry.category = category;
  entry.date_created = base::Time::Now();
  entry.date_last_used = base::Time::Now();
  entry.date_password_changed = base::Time::Now();
  entry.favicon_url = url.empty() ? "" : url + "/favicon.ico";

  // Determine if password is weak.
  AstraPasswordStrength strength = CheckPasswordStrength(password);
  entry.is_weak = (strength <= AstraPasswordStrength::kWeak);

  all_passwords_.push_back(std::move(entry));

  // Sort by site name.
  std::sort(all_passwords_.begin(), all_passwords_.end(),
            [](const AstraPasswordEntry& a, const AstraPasswordEntry& b) {
              return base::ToLowerASCII(a.site_name) <
                     base::ToLowerASCII(b.site_name);
            });

  RecomputePasswordFlags();
  ApplyFilters();
  NotifyPasswordAdded(entry.id);
  NotifyPasswordsChanged();

  return entry.id;
}

void AstraPasswordManagerModel::RemovePassword(const std::string& id) {
  auto it = FindEntry(id);
  if (it == all_passwords_.end()) {
    return;
  }
  all_passwords_.erase(it);
  RecomputePasswordFlags();
  ApplyFilters();
  NotifyPasswordRemoved(id);
  NotifyPasswordsChanged();
}

void AstraPasswordManagerModel::UpdatePassword(const std::string& id,
                                               const std::u16string& username,
                                               const std::string& password,
                                               const std::u16string& notes) {
  AstraPasswordEntry* entry = FindPassword(id);
  if (!entry) {
    return;
  }
  entry->username = username;
  if (entry->password != password) {
    entry->password = password;
    entry->date_password_changed = base::Time::Now();

    // Re-evaluate weakness.
    AstraPasswordStrength strength = CheckPasswordStrength(password);
    entry->is_weak = (strength <= AstraPasswordStrength::kWeak);

    RecomputePasswordFlags();
  }
  entry->notes = notes;

  ApplyFilters();
  NotifyPasswordUpdated(id);
  NotifyPasswordsChanged();
}

void AstraPasswordManagerModel::CopyPassword(const std::string& id) const {
  // TODO(astra): Implement clipboard copy using
  // ui/base/clipboard/clipboard.h.
  // This is a scaffolding stub.
  const AstraPasswordEntry* entry = GetPassword(id);
  if (!entry) {
    return;
  }
  // Stub: would write entry->password to clipboard.
}

void AstraPasswordManagerModel::ToggleFavorite(const std::string& id) {
  AstraPasswordEntry* entry = FindPassword(id);
  if (!entry) {
    return;
  }
  entry->is_favorited = !entry->is_favorited;
  ApplyFilters();
  NotifyPasswordUpdated(id);
  NotifyPasswordsChanged();
}

// -- Import / Export --------------------------------------------------------

void AstraPasswordManagerModel::ImportPasswords(const std::string& from_file) {
  // TODO(astra): Implement CSV/JSON password import.
  // Chromium owner: password_manager::PasswordImporter
  //   (components/password_manager/core/browser/import/password_importer.h)
}

void AstraPasswordManagerModel::ExportPasswords() const {
  // TODO(astra): Implement password export.
  // Chromium owner: password_manager::PasswordExporter
  //   (components/password_manager/core/browser/export/password_exporter.h)
}

// -- Sample data ------------------------------------------------------------

void AstraPasswordManagerModel::PopulateSamplePasswords() {
  all_passwords_.clear();
  next_id_ = 1;

  base::Time now = base::Time::Now();

  struct SampleEntry {
    const char* site_name;
    const char* username;
    const char* password;
    const char* url;
    const char* category;
    const char* workspace;
    const char* notes;
    bool is_leaked;
    bool is_reused;
    bool is_favorited;
    int days_since_created;
    int days_since_used;
    int days_since_password_changed;
  };

  // 20+ sample entries across categories.
  SampleEntry samples[] = {
      // Social
      {"GitHub", "astra_dev", "Gh1_#Secure2024", "https://github.com", "Work",
       "Engineering", "Main dev account", false, false, true, 365, 1, 90},
      {"Twitter / X", "astra_user", "password123", "https://twitter.com",
       "Social", "Personal", "Personal account", true, true, false, 500, 3, 200},
      {"LinkedIn", "john.doe@astra.com", "L1nkedIn!Strong", "https://linkedin.com",
       "Social", "Professional", "Work profile", false, false, false, 400, 7, 150},
      {"Reddit", "astra_redditor", "redditpass", "https://reddit.com",
       "Social", "Personal", "", false, true, false, 600, 2, 300},
      {"Facebook", "john.doe@example.com", "F@ceb00k2023", "https://facebook.com",
       "Social", "Personal", "Old account, rarely used", true, false, false, 800,
       30, 400},

      // Work / Productivity
      {"Google Workspace", "john@astra.com", "G00gl3#Work!", "https://google.com",
       "Work", "Productivity", "Primary work account", false, false, true, 730,
       0, 60},
      {"Slack", "john@astra.com", "Sl@ckW0rk2024", "https://slack.com",
       "Work", "Communication", "Astra team workspace", false, false, false,
       300, 0, 100},
      {"Notion", "john@astra.com", "N0t1onP@ss", "https://notion.so",
       "Work", "Productivity", "", false, false, false, 200, 1, 180},
      {"Jira", "jdoe", "Jira#2024Secure", "https://atlassian.net",
       "Work", "Engineering", "", false, false, false, 180, 5, 120},
      {"Figma", "john.design", "F1gm@D3sign!", "https://figma.com",
       "Work", "Design", "Design team account", false, false, true, 150, 10, 90},
      {"Zoom", "john@astra.com", "Z00mMeeting!", "https://zoom.us",
       "Work", "Communication", "", false, false, false, 250, 14, 200},

      // Finance
      {"Chase Bank", "john.doe", "Ch@seB@nk2024!$", "https://chase.com",
       "Finance", "Personal", "Checking & savings", false, false, true, 1000,
       30, 180},
      {"PayPal", "john@astra.com", "P@yPal$ecure1", "https://paypal.com",
       "Finance", "Personal", "", false, false, false, 600, 20, 300},
      {"Coinbase", "crypto_astra", "123456", "https://coinbase.com",
       "Finance", "Personal", "Crypto account - weak password!", true, false,
       false, 400, 60, 400},

      // Shopping
      {"Amazon", "john.shop", "Amaz0n#Shop!", "https://amazon.com",
       "Shopping", "Personal", "Prime account", false, false, false, 500, 2,
       200},
      {"eBay", "john_ebay", "ebaypass123", "https://ebay.com",
       "Shopping", "Personal", "", false, true, false, 700, 15, 350},
      {"Etsy", "crafty_john", "EtsyCr@ft2024", "https://etsy.com",
       "Shopping", "Personal", "", false, false, false, 200, 30, 180},

      // Entertainment
      {"Netflix", "john.doe@astra.com", "N3tflixWatch!", "https://netflix.com",
       "Entertainment", "Personal", "Family plan", false, false, false, 365,
       5, 250},
      {"Spotify", "john.music", "password", "https://spotify.com",
       "Entertainment", "Personal", "Very weak password", false, true, false,
       450, 1, 400},
      {"YouTube", "john@gmail.com", "Y0uTub3#2024", "https://youtube.com",
       "Entertainment", "Personal", "Google-linked", false, false, false, 365,
       0, 60},
      {"Twitch", "astra_gamer", "Tw1tchG@mer!", "https://twitch.tv",
       "Entertainment", "Personal", "Gaming streams", false, false, false,
       180, 7, 150},

      // Other
      {"Wikipedia", "astra_editor", "W1kiPedi@!", "https://wikipedia.org",
       "Work", "Research", "", false, false, false, 300, 60, 250},
      {"Stack Overflow", "dev_astra", "St@ck0verflow!", "https://stackoverflow.com",
       "Work", "Engineering", "", false, false, true, 400, 3, 180},
  };

  for (const auto& sample : samples) {
    AstraPasswordEntry entry;
    entry.id = GenerateId();
    entry.site_name = base::UTF8ToUTF16(sample.site_name);
    entry.username = base::UTF8ToUTF16(sample.username);
    entry.password = sample.password;
    entry.url = sample.url;
    entry.favicon_url = sample.url + std::string("/favicon.ico");
    entry.category = sample.category;
    entry.workspace = sample.workspace;
    entry.notes = base::UTF8ToUTF16(sample.notes);
    entry.is_leaked = sample.is_leaked;
    entry.is_reused = sample.is_reused;
    entry.is_favorited = sample.is_favorited;
    entry.date_created = now - base::Days(sample.days_since_created);
    entry.date_last_used = now - base::Days(sample.days_since_used);
    entry.date_password_changed =
        now - base::Days(sample.days_since_password_changed);

    // Determine weakness.
    AstraPasswordStrength strength = CheckPasswordStrength(entry.password);
    entry.is_weak = (strength <= AstraPasswordStrength::kWeak);

    all_passwords_.push_back(std::move(entry));
  }

  // Sort by site name.
  std::sort(all_passwords_.begin(), all_passwords_.end(),
            [](const AstraPasswordEntry& a, const AstraPasswordEntry& b) {
              return base::ToLowerASCII(a.site_name) <
                     base::ToLowerASCII(b.site_name);
            });

  RecomputePasswordFlags();
  ApplyFilters();
  NotifyPasswordsChanged();
}

// -- Password strength ------------------------------------------------------

AstraPasswordStrength AstraPasswordManagerModel::CheckPasswordStrength(
    const std::string& password) {
  if (password.empty()) {
    return AstraPasswordStrength::kVeryWeak;
  }

  int score = 0;

  // Length.
  size_t length = password.length();
  if (length < 6) {
    score += 1;
  } else if (length < 8) {
    score += 2;
  } else if (length < 12) {
    score += 3;
  } else if (length < 16) {
    score += 4;
  } else {
    score += 5;
  }

  // Character variety.
  bool has_lower = false;
  bool has_upper = false;
  bool has_digit = false;
  bool has_special = false;

  for (char c : password) {
    if (c >= 'a' && c <= 'z') {
      has_lower = true;
    } else if (c >= 'A' && c <= 'Z') {
      has_upper = true;
    } else if (c >= '0' && c <= '9') {
      has_digit = true;
    } else {
      has_special = true;
    }
  }

  int variety = 0;
  if (has_lower) ++variety;
  if (has_upper) ++variety;
  if (has_digit) ++variety;
  if (has_special) ++variety;

  score += variety;

  // Very common weak passwords.
  static const char* weak_passwords[] = {
      "password", "123456", "12345678", "qwerty", "abc123",
      "password1", "admin", "letmein", "welcome", "monkey"};
  std::string lower_password = base::ToLowerASCII(password);
  for (const char* weak : weak_passwords) {
    if (lower_password == weak) {
      return AstraPasswordStrength::kVeryWeak;
    }
  }

  // Map score to strength level (max 9).
  if (score <= 2) {
    return AstraPasswordStrength::kVeryWeak;
  } else if (score <= 4) {
    return AstraPasswordStrength::kWeak;
  } else if (score <= 6) {
    return AstraPasswordStrength::kMedium;
  } else if (score <= 8) {
    return AstraPasswordStrength::kStrong;
  } else {
    return AstraPasswordStrength::kVeryStrong;
  }
}

// -- State ------------------------------------------------------------------

bool AstraPasswordManagerModel::IsLoading() const {
  return loading_;
}

void AstraPasswordManagerModel::SetLoading(bool loading) {
  loading_ = loading;
  // TODO(astra): Notify observers of loading state change when a
  // dedicated notification method is added to AstraPasswordManagerObserver.
}

size_t AstraPasswordManagerModel::GetPasswordIssuesCount() const {
  size_t count = 0;
  std::set<std::string> seen_passwords;
  for (const auto& entry : all_passwords_) {
    if (entry.is_weak) ++count;
    if (entry.is_leaked) ++count;
    if (entry.is_reused) ++count;
  }
  return count;
}

const std::vector<AstraPasswordEntry>&
AstraPasswordManagerModel::GetFilteredPasswords() const {
  return filtered_passwords_;
}

// -- Private methods --------------------------------------------------------

void AstraPasswordManagerModel::NotifyPasswordsChanged() {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnPasswordsChanged();
  }
}

void AstraPasswordManagerModel::NotifyPasswordAdded(const std::string& id) {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnPasswordAdded(id);
  }
}

void AstraPasswordManagerModel::NotifyPasswordRemoved(const std::string& id) {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnPasswordRemoved(id);
  }
}

void AstraPasswordManagerModel::NotifyPasswordUpdated(const std::string& id) {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnPasswordUpdated(id);
  }
}

void AstraPasswordManagerModel::NotifySearchChanged() {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnSearchChanged(search_query_);
  }
}

void AstraPasswordManagerModel::NotifyFilterChanged() {
  for (AstraPasswordManagerObserver& observer : observers_) {
    observer.OnFilterChanged();
  }
}

void AstraPasswordManagerModel::ApplyFilters() {
  std::vector<AstraPasswordEntry> filtered;

  for (const auto& entry : all_passwords_) {
    if (!MatchesFilter(entry)) {
      continue;
    }
    if (!MatchesSearch(entry)) {
      continue;
    }
    if (!MatchesCategory(entry)) {
      continue;
    }
    filtered.push_back(entry);
  }

  filtered_passwords_ = std::move(filtered);
  grouped_passwords_ = GroupByFirstLetter(filtered_passwords_);
}

std::vector<AstraPasswordGroup>
AstraPasswordManagerModel::GroupByFirstLetter(
    const std::vector<AstraPasswordEntry>& entries) {
  std::vector<AstraPasswordGroup> groups;
  std::u16string current_letter;
  AstraPasswordGroup* current_group = nullptr;

  for (const auto& entry : entries) {
    std::u16string letter = GetFirstLetter(entry.site_name);
    if (letter != current_letter) {
      groups.emplace_back();
      current_group = &groups.back();
      current_group->label = letter;
      current_letter = letter;
    }
    current_group->entries.push_back(entry);
  }

  return groups;
}

bool AstraPasswordManagerModel::MatchesSearch(
    const AstraPasswordEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }

  if (CaseInsensitiveContains(entry.site_name, search_query_)) {
    return true;
  }

  if (CaseInsensitiveContains(entry.username, search_query_)) {
    return true;
  }

  if (CaseInsensitiveContainsUtf8(entry.url, search_query_)) {
    return true;
  }

  if (CaseInsensitiveContains(entry.notes, search_query_)) {
    return true;
  }

  return false;
}

bool AstraPasswordManagerModel::MatchesFilter(
    const AstraPasswordEntry& entry) const {
  switch (filter_) {
    case AstraPasswordFilter::kAll:
      return true;
    case AstraPasswordFilter::kWeak:
      return entry.is_weak;
    case AstraPasswordFilter::kReused:
      return entry.is_reused;
    case AstraPasswordFilter::kLeaked:
      return entry.is_leaked;
    case AstraPasswordFilter::kFavorites:
      return entry.is_favorited;
  }
  return true;
}

bool AstraPasswordManagerModel::MatchesCategory(
    const AstraPasswordEntry& entry) const {
  if (category_filter_.empty()) {
    return true;
  }
  return entry.category == category_filter_;
}

AstraPasswordEntry* AstraPasswordManagerModel::FindPassword(
    const std::string& id) {
  auto it = FindEntry(id);
  if (it != all_passwords_.end()) {
    return &(*it);
  }
  return nullptr;
}

void AstraPasswordManagerModel::RecomputePasswordFlags() {
  // Count password reuse.
  std::map<std::string, int> password_counts;
  for (const auto& entry : all_passwords_) {
    if (!entry.password.empty()) {
      password_counts[entry.password]++;
    }
  }

  for (auto& entry : all_passwords_) {
    if (!entry.password.empty() && password_counts[entry.password] > 1) {
      entry.is_reused = true;
    }
  }
}

std::vector<AstraPasswordEntry>::iterator
AstraPasswordManagerModel::FindEntry(const std::string& id) {
  return std::find_if(all_passwords_.begin(), all_passwords_.end(),
                      [&id](const AstraPasswordEntry& e) {
                        return e.id == id;
                      });
}

std::vector<AstraPasswordEntry>::const_iterator
AstraPasswordManagerModel::FindEntry(const std::string& id) const {
  return std::find_if(all_passwords_.begin(), all_passwords_.end(),
                      [&id](const AstraPasswordEntry& e) {
                        return e.id == id;
                      });
}

std::string AstraPasswordManagerModel::GenerateId() {
  return "pw_" + base::NumberToString(next_id_++);
}

}  // namespace astra
