// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Individual workspace item row in the profile menu workspace list.
//
// Shows:
//   - Accent color dot (left)
//   - Workspace name (middle, bold if active)
//   - Tab count (right, secondary text)
//   - Checkmark indicator (right, shown only for active workspace)
//
// This is a pure presentation view — it does not own workspace state.
// Data is projected from AstraWorkspaceService by the parent menu view.
//
// Chromium pattern: LabelButton / MenuItemView style rows.
//   Owner: MenuItemView (ui/views/controls/menu/menu_item_view.h)
//   Patch point: This replaces the generic View-based workspace rows
//     in AstraProfileMenuWorkspaces with a proper interactive row.
//
// TODO(astra): Consider using views::MenuRunner or MenuItemView directly
//   for full menu semantics (accelerators, mnemonics, submenus).
//   For now we implement a custom row to keep the workspace list self-contained
//   and to match the visual style of Chromium's profile menu items.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_MENU_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_MENU_ITEM_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class Label;
class ImageView;
}  // namespace views

namespace astra {

// Display mode for workspace items — controls how much detail is shown.
enum class AstraWorkspaceItemDisplayMode {
  kIconsOnly,       // Only color dot / icon.
  kNamesOnly,       // Only name and tab count.
  kIconsAndNames,   // Color dot + name + tab count (default).
};

// Button size variant for workspace items.
enum class AstraWorkspaceItemSize {
  kSmall,    // Compact height, smaller icon.
  kMedium,   // Standard size (default).
  kLarge,    // Taller, larger icon and text.
};

// A single workspace row in the profile menu workspace list.
//
// Visual layout (horizontal):
//   [drag handle] [color dot]  Workspace Name          tab count  [checkmark]
//
// Deepened features:
//   - Reorder drag handle (optional)
//   - Multiple display modes (icons only, names only, icons+names)
//   - Multiple size variants (small, medium, large)
//   - Enhanced hover and active states
//   - Full keyboard navigation support
//   - Comprehensive accessibility
//   - Accent color theming
//
// Behavior:
//   - Hover: highlights background
//   - Click: triggers the callback
//   - Focus: shows focus ring
//   - Enter/Space: activates (same as click)
//   - Drag handle: initiates reordering
//
// The row is a Button subclass for proper keyboard handling, ink drop
// support, and accessibility semantics.
class AstraWorkspaceMenuItemView : public views::Button {
 public:
  // Callback invoked when the workspace item is activated (clicked or
  // keyboard-activated).
  using ActivatedCallback = base::RepeatingClosure;

  // Callback invoked when reorder is requested (drag handle clicked).
  using ReorderCallback = base::RepeatingCallback<void(int direction)>;

  AstraWorkspaceMenuItemView(const std::u16string& workspace_name,
                             SkColor accent_color,
                             int tab_count,
                             bool is_active,
                             ActivatedCallback callback);
  ~AstraWorkspaceMenuItemView() override;

  AstraWorkspaceMenuItemView(const AstraWorkspaceMenuItemView&) = delete;
  AstraWorkspaceMenuItemView& operator=(const AstraWorkspaceMenuItemView&) =
      delete;

  // -- Property updates (called when workspace state changes) -------------

  void SetWorkspaceName(const std::u16string& name);
  void SetAccentColor(SkColor color);
  void SetTabCount(int count);
  void SetIsActive(bool active);
  bool is_active() const { return is_active_; }

  // -- Display mode -------------------------------------------------------

  void SetDisplayMode(AstraWorkspaceItemDisplayMode mode);
  AstraWorkspaceItemDisplayMode display_mode() const { return display_mode_; }

  // -- Size variant -------------------------------------------------------

  void SetSizeVariant(AstraWorkspaceItemSize size);
  AstraWorkspaceItemSize size_variant() const { return size_variant_; }

  // -- Reorder handle ------------------------------------------------------

  void SetReorderHandleVisible(bool visible);
  bool reorder_handle_visible() const { return reorder_handle_visible_; }

  void set_reorder_callback(ReorderCallback callback) {
    reorder_callback_ = std::move(callback);
  }

  // -- views::Button ------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void Layout() override;

 private:
  // Updates the background and text styling for active/inactive state.
  void UpdateActiveVisuals();

  // Updates the color dot's background color.
  void UpdateColorDot();

  // Updates the tab count label text.
  void UpdateTabCountLabel();

  // Updates visibility of child views based on display mode.
  void UpdateDisplayModeVisibility();

  // Updates layout constants based on size variant.
  void UpdateSizeVariant();

  // Called by Button when the button is pressed (click, keyboard Enter/Space).
  void ButtonPressed();

  // Handles reorder up button click.
  void OnReorderUpClicked();

  // Handles reorder down button click.
  void OnReorderDownClicked();

  // Returns the current row height based on size variant.
  int GetRowHeight() const;

  // Returns the current color dot size based on size variant.
  int GetColorDotSize() const;

  // Child views (owned by view hierarchy).
  raw_ptr<views::View> reorder_container_ = nullptr;
  raw_ptr<views::View> reorder_up_ = nullptr;
  raw_ptr<views::View> reorder_down_ = nullptr;
  raw_ptr<views::View> color_dot_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::View> checkmark_indicator_ = nullptr;

  // Display state.
  std::u16string workspace_name_;
  SkColor accent_color_ = SK_ColorBLUE;
  int tab_count_ = 0;
  bool is_active_ = false;

  // Display mode and size.
  AstraWorkspaceItemDisplayMode display_mode_ =
      AstraWorkspaceItemDisplayMode::kIconsAndNames;
  AstraWorkspaceItemSize size_variant_ = AstraWorkspaceItemSize::kMedium;

  // Reorder handle visibility.
  bool reorder_handle_visible_ = false;

  // Hover state.
  bool is_hovered_ = false;

  // Callbacks.
  ActivatedCallback activated_callback_;
  ReorderCallback reorder_callback_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_WORKSPACE_MENU_ITEM_VIEW_H_
