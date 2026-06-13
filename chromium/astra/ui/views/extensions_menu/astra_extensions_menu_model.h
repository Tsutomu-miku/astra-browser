// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_MODEL_H_
#define ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// State of an extension.
enum class AstraExtensionState {
  kEnabled,
  kDisabled,
  kBlocked,     // Blocked on this page
  kError,       // In error state
  kUninstalled, // Pending uninstall
};

// Categories for grouping extensions in the menu.
enum class AstraExtensionCategory {
  kPinned,       // Extensions pinned to the toolbar
  kActive,       // Extensions active on the current page
  kInactive,     // Extensions installed but not active
  kBlocked,      // Extensions blocked on the current page
};

// Information about a single extension for display in the menu.
struct AstraExtensionMenuEntry {
  std::string extension_id;
  std::u16string name;
  std::u16string description;
  gfx::ImageSkia icon;
  AstraExtensionState state = AstraExtensionState::kEnabled;
  AstraExtensionCategory category = AstraExtensionCategory::kActive;
  bool pinned_to_toolbar = false;
  bool can_show_in_toolbar = true;
  bool has_permission_for_current_page = true;
  std::u16string badge_text;
  SkColor badge_color = SK_ColorRED;
  bool has_badge = false;
  // Action text shown on the right side (e.g. "Pin", "Manage").
  std::u16string action_label;
};

// Observer for AstraExtensionsMenuModel.
class AstraExtensionsMenuObserver : public base::CheckedObserver {
 public:
  // Called when the list of extensions changes.
  virtual void OnExtensionsChanged(AstraExtensionsMenuModel* model) {}

  // Called when a specific extension's state changes.
  virtual void OnExtensionChanged(AstraExtensionsMenuModel* model,
                                  const std::string& extension_id) {}

  // Called when the model is about to be destroyed.
  virtual void OnExtensionsMenuModelShutdown(
      AstraExtensionsMenuModel* model) {}

 protected:
  ~AstraExtensionsMenuObserver() override = default;
};

// Model for the extensions menu.
//
// Owns the list of extensions and their display state.  The extensions
// themselves are owned by Chromium's extension system — this model
// projects Chromium extension state into a form suitable for the Astra
// extensions menu UI.
//
// Chromium owner: ExtensionsMenuModel / ExtensionsMenuBubbleController
//   (chrome/browser/ui/extensions/extensions_menu_model.h)
//
// TODO(astra): Wire up to real extension service via a KeyedService and
// observe ExtensionRegistry for changes.  Patch point:
// chrome/browser/ui/extensions/extension_action_test_helper.cc
// or chrome/browser/extensions/extension_action_manager.cc.
class AstraExtensionsMenuModel {
 public:
  AstraExtensionsMenuModel();
  ~AstraExtensionsMenuModel();

  AstraExtensionsMenuModel(const AstraExtensionsMenuModel&) = delete;
  AstraExtensionsMenuModel& operator=(const AstraExtensionsMenuModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraExtensionsMenuObserver* observer);
  void RemoveObserver(AstraExtensionsMenuObserver* observer);

  // -- Extension list -------------------------------------------------------

  // Get all extensions in the menu.
  const std::vector<AstraExtensionMenuEntry>& GetAllExtensions() const;

  // Get extensions in a specific category.
  std::vector<AstraExtensionMenuEntry> GetExtensionsByCategory(
      AstraExtensionCategory category) const;

  // Get the number of extensions.
  size_t GetExtensionCount() const;

  // Get a specific extension by ID. Returns nullptr if not found.
  const AstraExtensionMenuEntry* GetExtension(
      const std::string& extension_id) const;

  // -- Extension management -------------------------------------------------

  // Add or update an extension entry.
  void SetExtension(const AstraExtensionMenuEntry& entry);

  // Remove an extension.
  void RemoveExtension(const std::string& extension_id);

  // Update an extension's state.
  void SetExtensionState(const std::string& extension_id,
                         AstraExtensionState state);

  // Update whether an extension is pinned.
  void SetExtensionPinned(const std::string& extension_id, bool pinned);

  // Update the extension badge.
  void SetExtensionBadge(const std::string& extension_id,
                         const std::u16string& badge_text,
                         SkColor badge_color);

  // Update extension icon.
  void SetExtensionIcon(const std::string& extension_id,
                        const gfx::ImageSkia& icon);

  // -- Category counts ------------------------------------------------------

  size_t GetPinnedCount() const;
  size_t GetActiveCount() const;
  size_t GetInactiveCount() const;
  size_t GetBlockedCount() const;

  // -- Bulk operations ------------------------------------------------------

  // Populate with sample extensions for testing/development.
  void PopulateSampleExtensions();

  // Clear all extensions.
  void ClearAll();

  // -- Search ---------------------------------------------------------------

  // Set the search query filter. Empty string = no filter.
  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const;

  // Get filtered extensions (matching search query).
  std::vector<AstraExtensionMenuEntry> GetFilteredExtensions() const;

  // -- Compact mode ---------------------------------------------------------

  void SetCompactMode(bool compact);
  bool GetCompactMode() const;

 private:
  // Notify observers that the full list changed.
  void NotifyExtensionsChanged();

  // Notify observers that a specific extension changed.
  void NotifyExtensionChanged(const std::string& extension_id);

  // Find the index of an extension by ID. Returns -1 if not found.
  int FindExtensionIndex(const std::string& extension_id) const;

  // Check if an entry matches the search query.
  bool MatchesSearch(const AstraExtensionMenuEntry& entry) const;

  // The list of extension entries.
  std::vector<AstraExtensionMenuEntry> extensions_;

  // Current search filter.
  std::u16string search_query_;

  // Compact mode.
  bool compact_mode_ = false;

  // Observers.
  base::ObserverList<AstraExtensionsMenuObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_EXTENSIONS_MENU_ASTRA_EXTENSIONS_MENU_MODEL_H_
