// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUTS_MODEL_H_
#define ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUTS_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "astra/ui/views/split_view/astra_split_view_model.h"

namespace astra {

// =========================================================================
// Astra saved split view layouts
// =========================================================================
//
// Model for saved split view layout presets.  Users can save their current
// split view configuration (layout mode, pane sizes, optionally which tabs
// are in each pane) and quickly restore it later.
//
// This is an Astra-unique feature: quick layout switching for different
// work modes (e.g. "Research" = main + notes, "Coding" = editor + docs
// + terminal, "Design review" = Figma + notes + feedback).
//
// Layouts contain:
//   - id, name, description, icon
//   - layout_mode (2-pane horizontal, 3-pane vertical, grid, etc.)
//   - pane_ratios (size ratios for each pane)
//   - is_builtin (whether this is a built-in preset or user-created)
//   - last_used_time
//   - use_count (how many times this layout has been applied)
//
// Chromium subsystems reused:
//   - PrefService (persistence)
//
// TODO(astra): Persist saved layouts to PrefService.
//   Chromium owner: components/prefs/pref_service.h
//   Patch point: astra/browser/astra_prefs.h
// =========================================================================

// A saved split view layout preset.
struct AstraSplitLayout {
  std::string id;
  std::u16string name;
  std::u16string description;
  std::string icon_name;  // Name for icon drawing

  // Layout configuration.
  AstraSplitLayoutMode layout_mode = AstraSplitLayoutMode::kTwoPaneHorizontal;
  std::vector<float> pane_ratios;  // Size ratios, must sum to 1.0

  // Optional: tab / URL assignments per pane (for layouts that include
  // specific sites).  Empty means "keep current tabs, just reflow layout".
  std::vector<std::string> pane_urls;  // URL or tab ID per pane

  // Metadata.
  bool is_builtin = false;
  base::Time created_time;
  base::Time last_used_time;
  int use_count = 0;

  // Display color accent for this layout.
  std::string accent_color;
};

// Observer for split layouts model.
class AstraSplitLayoutsObserver : public base::CheckedObserver {
 public:
  // Called when a layout is saved / created.
  virtual void OnLayoutSaved(const AstraSplitLayout& layout) {}

  // Called when a layout is deleted.
  virtual void OnLayoutDeleted(const std::string& layout_id) {}

  // Called when a layout is applied / activated.
  virtual void OnLayoutApplied(const std::string& layout_id) {}

  // Called when layouts are reordered.
  virtual void OnLayoutsReordered() {}

  // Called when the model is about to be destroyed.
  virtual void OnSplitLayoutsModelShutdown() {}

 protected:
  ~AstraSplitLayoutsObserver() override = default;
};

// Model for saved split view layouts.
class AstraSplitLayoutsModel {
 public:
  AstraSplitLayoutsModel();
  ~AstraSplitLayoutsModel();

  AstraSplitLayoutsModel(const AstraSplitLayoutsModel&) = delete;
  AstraSplitLayoutsModel& operator=(const AstraSplitLayoutsModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraSplitLayoutsObserver* observer);
  void RemoveObserver(AstraSplitLayoutsObserver* observer);

  // -- Layout management ----------------------------------------------------

  // Save the current split view configuration as a named layout.
  std::string SaveLayout(const std::u16string& name,
                         const std::u16string& description,
                         AstraSplitLayoutMode mode,
                         const std::vector<float>& ratios,
                         const std::string& icon_name = std::string());

  // Delete a saved layout.
  void DeleteLayout(const std::string& layout_id);

  // Apply/activate a layout.
  void ApplyLayout(const std::string& layout_id);

  // Get a layout by ID.
  const AstraSplitLayout* GetLayout(const std::string& layout_id) const;

  // Get all saved layouts (excluding built-ins if requested).
  std::vector<AstraSplitLayout> GetAllLayouts() const;
  std::vector<AstraSplitLayout> GetUserLayouts() const;
  std::vector<AstraSplitLayout> GetBuiltInLayouts() const;

  // Get frequently used layouts.
  std::vector<AstraSplitLayout> GetRecentLayouts(size_t max_count = 5) const;
  std::vector<AstraSplitLayout> GetFavoriteLayouts(size_t max_count = 5) const;

  // Get layout count.
  size_t GetLayoutCount() const { return layouts_.size(); }
  size_t GetUserLayoutCount() const;
  size_t GetBuiltInLayoutCount() const;

  // Rename a layout.
  void RenameLayout(const std::string& layout_id,
                    const std::u16string& new_name);

  // Reorder layouts.
  void ReorderLayout(const std::string& layout_id, int new_index);

  // -- Built-in presets -----------------------------------------------------

  // Populate with built-in layout presets.
  void PopulateBuiltInLayouts();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

 private:
  // Notify helpers.
  void NotifyLayoutSaved(const AstraSplitLayout& layout);
  void NotifyLayoutDeleted(const std::string& layout_id);
  void NotifyLayoutApplied(const std::string& layout_id);
  void NotifyLayoutsReordered();

  // Find layout by ID.
  AstraSplitLayout* FindLayout(const std::string& layout_id);

  // All layouts.
  std::vector<AstraSplitLayout> layouts_;

  bool loading_ = false;
  int next_layout_id_ = 1;

  base::ObserverList<AstraSplitLayoutsObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUTS_MODEL_H_
