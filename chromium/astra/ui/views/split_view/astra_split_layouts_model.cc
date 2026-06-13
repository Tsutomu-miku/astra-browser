// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/split_view/astra_split_layouts_model.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Built-in layout presets.
struct BuiltInLayoutDef {
  const char* name;
  const char* description;
  const char* icon_name;
  AstraSplitLayoutMode mode;
  std::vector<float> ratios;
  const char* accent_color;
};

const BuiltInLayoutDef kBuiltInLayouts[] = {
    // Two-pane horizontal.
    {"50/50 Split", "Equal side-by-side panes", "split_h",
     AstraSplitLayoutMode::kTwoPaneHorizontal, {0.5f, 0.5f}, "#5B8FF9"},

    {"70/30 Focus", "Main focus with side pane", "focus",
     AstraSplitLayoutMode::kTwoPaneHorizontal, {0.7f, 0.3f}, "#61DDAA"},

    {"30/70 Side", "Side pane with main content", "side",
     AstraSplitLayoutMode::kTwoPaneHorizontal, {0.3f, 0.7f}, "#F6BD16"},

    // Two-pane vertical.
    {"Top/Bottom", "Stacked vertical split", "split_v",
     AstraSplitLayoutMode::kTwoPaneVertical, {0.5f, 0.5f}, "#7262FD"},

    // Three-pane horizontal.
    {"Three Columns", "Three equal side-by-side panes", "three_col",
     AstraSplitLayoutMode::kThreePaneHorizontal, {0.333f, 0.333f, 0.334f},
     "#E8684A"},

    {"Code + Docs + Terminal", "Coding layout with docs and terminal",
     "code",
     AstraSplitLayoutMode::kThreePaneHorizontal, {0.5f, 0.3f, 0.2f},
     "#5AD8A6"},

    {"Research Mode", "Main + notes + references", "research",
     AstraSplitLayoutMode::kThreePaneHorizontal, {0.45f, 0.35f, 0.2f},
     "#F6BD16"},

    // Grid layouts.
    {"2x2 Grid", "Four equal panes in a grid", "grid_2x2",
     AstraSplitLayoutMode::kGridTwoByTwo, {0.5f, 0.5f, 0.5f, 0.5f},
     "#7262FD"},

    {"Dashboard", "Dashboard-style 2x2 layout", "dashboard",
     AstraSplitLayoutMode::kGridTwoByTwo, {0.6f, 0.4f, 0.5f, 0.5f},
     "#5B8FF9"},

    // Picture in picture.
    {"Picture in Picture", "Floating small pane over main content", "pip",
     AstraSplitLayoutMode::kPictureInPicture, {0.85f, 0.15f}, "#E8684A"},

    // Tab shift.
    {"Tab Shift", "Narrow tab list with main content", "tab_shift",
     AstraSplitLayoutMode::kTabShift, {0.2f, 0.8f}, "#61DDAA"},

    // Design review.
    {"Design Review", "Figma + notes + feedback", "design",
     AstraSplitLayoutMode::kThreePaneHorizontal, {0.5f, 0.3f, 0.2f},
     "#F6BD16"},

    // Meeting layout.
    {"Meeting Notes", "Video call + notes + chat", "meeting",
     AstraSplitLayoutMode::kThreePaneVertical, {0.5f, 0.3f, 0.2f},
     "#5B8FF9"},

    // Reading.
    {"Reading Mode", "Article + table of contents", "reading",
     AstraSplitLayoutMode::kTwoPaneHorizontal, {0.25f, 0.75f},
     "#61DDAA"},

    // Social media.
    {"Social + Feed", "Social media + browser", "social",
     AstraSplitLayoutMode::kTwoPaneHorizontal, {0.35f, 0.65f},
     "#E8684A"},
};

}  // namespace

// ===========================================================================
// AstraSplitLayoutsModel
// ===========================================================================

AstraSplitLayoutsModel::AstraSplitLayoutsModel() {
  PopulateBuiltInLayouts();
}

AstraSplitLayoutsModel::~AstraSplitLayoutsModel() {
  for (auto& observer : observers_) {
    observer.OnSplitLayoutsModelShutdown();
  }
}

void AstraSplitLayoutsModel::AddObserver(
    AstraSplitLayoutsObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSplitLayoutsModel::RemoveObserver(
    AstraSplitLayoutsObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::string AstraSplitLayoutsModel::SaveLayout(
    const std::u16string& name,
    const std::u16string& description,
    AstraSplitLayoutMode mode,
    const std::vector<float>& ratios,
    const std::string& icon_name) {
  AstraSplitLayout layout;
  layout.id = "layout_" + base::NumberToString(next_layout_id_++);
  layout.name = name;
  layout.description = description;
  layout.icon_name = icon_name.empty() ? "split_h" : icon_name;
  layout.layout_mode = mode;
  layout.pane_ratios = ratios;
  layout.is_builtin = false;
  layout.created_time = base::Time::Now();
  layout.last_used_time = base::Time::Now();
  layout.use_count = 0;

  layouts_.push_back(std::move(layout));

  const auto& saved = layouts_.back();
  NotifyLayoutSaved(saved);
  return saved.id;
}

void AstraSplitLayoutsModel::DeleteLayout(const std::string& layout_id) {
  auto it = std::find_if(layouts_.begin(), layouts_.end(),
                         [&layout_id](const AstraSplitLayout& l) {
                           return l.id == layout_id;
                         });
  if (it == layouts_.end()) return;

  // Don't delete built-in layouts.
  if (it->is_builtin) return;

  layouts_.erase(it);
  NotifyLayoutDeleted(layout_id);
}

void AstraSplitLayoutsModel::ApplyLayout(const std::string& layout_id) {
  auto* layout = FindLayout(layout_id);
  if (!layout) return;

  layout->use_count++;
  layout->last_used_time = base::Time::Now();

  NotifyLayoutApplied(layout_id);
}

const AstraSplitLayout* AstraSplitLayoutsModel::GetLayout(
    const std::string& layout_id) const {
  for (const auto& layout : layouts_) {
    if (layout.id == layout_id) {
      return &layout;
    }
  }
  return nullptr;
}

std::vector<AstraSplitLayout> AstraSplitLayoutsModel::GetAllLayouts() const {
  return layouts_;
}

std::vector<AstraSplitLayout> AstraSplitLayoutsModel::GetUserLayouts() const {
  std::vector<AstraSplitLayout> result;
  for (const auto& l : layouts_) {
    if (!l.is_builtin) {
      result.push_back(l);
    }
  }
  return result;
}

std::vector<AstraSplitLayout> AstraSplitLayoutsModel::GetBuiltInLayouts()
    const {
  std::vector<AstraSplitLayout> result;
  for (const auto& l : layouts_) {
    if (l.is_builtin) {
      result.push_back(l);
    }
  }
  return result;
}

std::vector<AstraSplitLayout> AstraSplitLayoutsModel::GetRecentLayouts(
    size_t max_count) const {
  std::vector<AstraSplitLayout> result = layouts_;
  std::sort(result.begin(), result.end(),
            [](const AstraSplitLayout& a, const AstraSplitLayout& b) {
              return a.last_used_time > b.last_used_time;
            });
  if (result.size() > max_count) {
    result.resize(max_count);
  }
  return result;
}

std::vector<AstraSplitLayout> AstraSplitLayoutsModel::GetFavoriteLayouts(
    size_t max_count) const {
  std::vector<AstraSplitLayout> result = layouts_;
  std::sort(result.begin(), result.end(),
            [](const AstraSplitLayout& a, const AstraSplitLayout& b) {
              return a.use_count > b.use_count;
            });
  if (result.size() > max_count) {
    result.resize(max_count);
  }
  return result;
}

size_t AstraSplitLayoutsModel::GetUserLayoutCount() const {
  size_t count = 0;
  for (const auto& l : layouts_) {
    if (!l.is_builtin) count++;
  }
  return count;
}

size_t AstraSplitLayoutsModel::GetBuiltInLayoutCount() const {
  size_t count = 0;
  for (const auto& l : layouts_) {
    if (l.is_builtin) count++;
  }
  return count;
}

void AstraSplitLayoutsModel::RenameLayout(const std::string& layout_id,
                                          const std::u16string& new_name) {
  auto* layout = FindLayout(layout_id);
  if (!layout || layout->is_builtin) return;

  if (layout->name == new_name) return;
  layout->name = new_name;
  NotifyLayoutSaved(*layout);
}

void AstraSplitLayoutsModel::ReorderLayout(const std::string& layout_id,
                                           int new_index) {
  auto it = std::find_if(layouts_.begin(), layouts_.end(),
                         [&layout_id](const AstraSplitLayout& l) {
                           return l.id == layout_id;
                         });
  if (it == layouts_.end()) return;

  // Don't reorder built-in layouts (they stay at the end or beginning).
  if (it->is_builtin) return;

  // Move to new index.
  // TODO(astra): Implement proper reordering with user layouts only.
  NotifyLayoutsReordered();
}

void AstraSplitLayoutsModel::PopulateBuiltInLayouts() {
  base::Time now = base::Time::Now();

  for (const auto& def : kBuiltInLayouts) {
    AstraSplitLayout layout;
    layout.id = std::string("builtin_") + def.name;
    // Replace spaces with underscores for ID.
    std::string sanitized_name = def.name;
    std::replace(sanitized_name.begin(), sanitized_name.end(), ' ', '_');
    layout.id = "builtin_" + sanitized_name;

    layout.name = base::UTF8ToUTF16(def.name);
    layout.description = base::UTF8ToUTF16(def.description);
    layout.icon_name = def.icon_name;
    layout.layout_mode = def.mode;
    layout.pane_ratios = def.ratios;
    layout.is_builtin = true;
    layout.created_time = now;
    layout.last_used_time = now;
    layout.use_count = 0;
    layout.accent_color = def.accent_color;

    layouts_.push_back(std::move(layout));
  }
}

void AstraSplitLayoutsModel::SetLoading(bool loading) {
  loading_ = loading;
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraSplitLayoutsModel::NotifyLayoutSaved(
    const AstraSplitLayout& layout) {
  for (auto& observer : observers_) {
    observer.OnLayoutSaved(layout);
  }
}

void AstraSplitLayoutsModel::NotifyLayoutDeleted(
    const std::string& layout_id) {
  for (auto& observer : observers_) {
    observer.OnLayoutDeleted(layout_id);
  }
}

void AstraSplitLayoutsModel::NotifyLayoutApplied(
    const std::string& layout_id) {
  for (auto& observer : observers_) {
    observer.OnLayoutApplied(layout_id);
  }
}

void AstraSplitLayoutsModel::NotifyLayoutsReordered() {
  for (auto& observer : observers_) {
    observer.OnLayoutsReordered();
  }
}

AstraSplitLayout* AstraSplitLayoutsModel::FindLayout(
    const std::string& layout_id) {
  for (auto& layout : layouts_) {
    if (layout.id == layout_id) {
      return &layout;
    }
  }
  return nullptr;
}

}  // namespace astra
