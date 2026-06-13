// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_MODEL_H_
#define ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// A single password entry.
//
// Chromium owner: password_manager::PasswordForm
//   (components/password_manager/core/browser/password_form.h)
//
// TODO(astra): Replace scaffolding struct with projection from Chromium's
// password_manager::PasswordStore.  Patch point:
// components/password_manager/core/browser/password_store_interface.h
struct AstraPasswordEntry {
  std::string id;
  std::u16string site_name;
  std::u16string username;
  std::string password;
  std::string url;
  std::string favicon_url;
  gfx::ImageSkia favicon;
  base::Time date_created;
  base::Time date_last_used;
  base::Time date_password_changed;
  std::string category;
  std::string workspace;
  std::u16string notes;
  bool is_leaked = false;
  bool is_weak = false;
  bool is_reused = false;
  bool is_favorited = false;
  bool in_blacklisted_site = false;
};

// A group of password entries (e.g. grouped by first letter or category).
struct AstraPasswordGroup {
  std::u16string label;
  std::vector<AstraPasswordEntry> entries;
};

// Filter options for the password list.
enum class AstraPasswordFilter {
  kAll,
  kWeak,
  kReused,
  kLeaked,
  kFavorites,
};

// Password strength levels.
enum class AstraPasswordStrength {
  kVeryWeak,
  kWeak,
  kMedium,
  kStrong,
  kVeryStrong,
};

// Observer for AstraPasswordManagerModel.
class AstraPasswordManagerObserver : public base::CheckedObserver {
 public:
  virtual void OnPasswordsChanged() {}
  virtual void OnPasswordAdded(const std::string& id) {}
  virtual void OnPasswordRemoved(const std::string& id) {}
  virtual void OnPasswordUpdated(const std::string& id) {}
  virtual void OnSearchChanged(const std::u16string& query) {}
  virtual void OnFilterChanged() {}
  virtual void OnPasswordManagerModelShutdown() {}

 protected:
  ~AstraPasswordManagerObserver() override = default;
};

// Model for the password manager page.
//
// Owns password entry metadata and provides filtering/search logic.
// Password data ultimately comes from Chromium's PasswordStore — this
// model projects and augments it with Astra-specific categorization,
// workspace association, and favorite/notes metadata.
//
// Chromium owner: PasswordStore / PasswordManagerServiceImpl
//   (components/password_manager/core/browser/password_store/password_store_interface.h)
//   (chrome/browser/password_manager/password_manager_service_factory.cc)
//
// TODO(astra): Wire up to Chromium's PasswordStore via a KeyedService
// wrapper.  Patch point:
// chrome/browser/password_manager/password_manager_service_factory.cc
// or components/password_manager/core/browser/password_store/password_store.cc
class AstraPasswordManagerModel {
 public:
  AstraPasswordManagerModel();
  ~AstraPasswordManagerModel();

  AstraPasswordManagerModel(const AstraPasswordManagerModel&) = delete;
  AstraPasswordManagerModel& operator=(const AstraPasswordManagerModel&) = delete;

  // -- Observer management ----------------------------------------------------

  void AddObserver(AstraPasswordManagerObserver* observer);
  void RemoveObserver(AstraPasswordManagerObserver* observer);

  // -- Password data ---------------------------------------------------------

  // Get all passwords (unfiltered), sorted by site name.
  const std::vector<AstraPasswordEntry>& GetPasswords() const;

  // Get a specific password entry by ID.  Returns nullptr if not found.
  const AstraPasswordEntry* GetPassword(const std::string& id) const;

  // Get total number of password entries (unfiltered).
  size_t GetCount() const;

  // Get passwords grouped by first letter of site name (filtered).
  const std::vector<AstraPasswordGroup>& GetGroupedPasswords() const;

  // -- Search -----------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const;

  // -- Filtering --------------------------------------------------------------

  void SetFilter(AstraPasswordFilter filter);
  AstraPasswordFilter GetFilter() const;

  // -- Category filter --------------------------------------------------------

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const;

  // Get all unique categories across password entries.
  std::vector<std::string> GetCategories() const;

  // -- Entry manipulation -----------------------------------------------------

  // Add a new password entry.  Returns the generated ID.
  std::string AddPassword(const std::u16string& site_name,
                          const std::u16string& username,
                          const std::string& password,
                          const std::string& url,
                          const std::string& category);

  // Remove a password entry by ID.
  void RemovePassword(const std::string& id);

  // Update a password entry.
  void UpdatePassword(const std::string& id,
                      const std::u16string& username,
                      const std::string& password,
                      const std::u16string& notes);

  // Copy the password to the clipboard.  Stub for now.
  void CopyPassword(const std::string& id) const;

  // Toggle the favorite status of a password entry.
  void ToggleFavorite(const std::string& id);

  // -- Import / Export --------------------------------------------------------

  // Import passwords from a file.  Stub for now.
  void ImportPasswords(const std::string& from_file);

  // Export passwords.  Stub for now.
  void ExportPasswords() const;

  // -- Sample data ------------------------------------------------------------

  // Populate with sample passwords for testing/development.
  void PopulateSamplePasswords();

  // -- Password strength ------------------------------------------------------

  // Check the strength of a password.
  static AstraPasswordStrength CheckPasswordStrength(const std::string& password);

  // -- State ------------------------------------------------------------------

  bool IsLoading() const;
  void SetLoading(bool loading);

  // Get the total number of password issues (weak + leaked + reused).
  size_t GetPasswordIssuesCount() const;

  // Get filtered passwords (after search + filter + category).
  const std::vector<AstraPasswordEntry>& GetFilteredPasswords() const;

 private:
  // Notify helpers.
  void NotifyPasswordsChanged();
  void NotifyPasswordAdded(const std::string& id);
  void NotifyPasswordRemoved(const std::string& id);
  void NotifyPasswordUpdated(const std::string& id);
  void NotifySearchChanged();
  void NotifyFilterChanged();

  // Apply current filters and regenerate grouped view.
  void ApplyFilters();

  // Generate groups from flat entry list (grouped by first letter).
  static std::vector<AstraPasswordGroup> GroupByFirstLetter(
      const std::vector<AstraPasswordEntry>& entries);

  // Match predicates.
  bool MatchesSearch(const AstraPasswordEntry& entry) const;
  bool MatchesFilter(const AstraPasswordEntry& entry) const;
  bool MatchesCategory(const AstraPasswordEntry& entry) const;

  // Find an entry by ID (non-const, returns pointer).
  AstraPasswordEntry* FindPassword(const std::string& id);

  // Find an entry by ID (returns iterator).
  std::vector<AstraPasswordEntry>::iterator FindEntry(const std::string& id);
  std::vector<AstraPasswordEntry>::const_iterator FindEntry(
      const std::string& id) const;

  // Recompute is_weak and is_reused flags for all entries.
  void RecomputePasswordFlags();

  // Generate a unique ID for a new password entry.
  std::string GenerateId();

  // All password entries (unfiltered), sorted by site name.
  std::vector<AstraPasswordEntry> all_passwords_;

  // Filtered password entries (what the UI displays).
  std::vector<AstraPasswordEntry> filtered_passwords_;

  // Filtered and grouped passwords.
  std::vector<AstraPasswordGroup> grouped_passwords_;

  // Current search query.
  std::u16string search_query_;

  // Current filter.
  AstraPasswordFilter filter_ = AstraPasswordFilter::kAll;

  // Current category filter (empty = all).
  std::string category_filter_;

  // Loading state.
  bool loading_ = false;

  // Next ID counter for generated IDs.
  int next_id_ = 1;

  base::ObserverList<AstraPasswordManagerObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PASSWORD_MANAGER_ASTRA_PASSWORD_MANAGER_MODEL_H_
