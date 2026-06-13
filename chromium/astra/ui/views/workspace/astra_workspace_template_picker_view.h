// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_TEMPLATE_PICKER_VIEW_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_TEMPLATE_PICKER_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

#include "astra/browser/astra_workspace_template.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraWorkspaceTemplateCardView — single template card
// =========================================================================
//
// A card that displays a workspace template: icon, name, description,
// accent color bar, and default tab count.  Clicking the card selects
// the template.
//
// Layout:
//   +---------------------------+
//   |  accent color bar         |
//   +---------------------------+
//   |  [icon]  Template Name    |
//   |          Description...   |
//   |          N default tabs   |
//   +---------------------------+
//
// TODO(astra): Replace text icon with vector icon from
//   //ui/views/vector_icons or astra/ui/vector_icons.
// =========================================================================

class AstraWorkspaceTemplateCardView : public views::Button {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& template_id)>;

  explicit AstraWorkspaceTemplateCardView(
      const AstraWorkspaceTemplate& tmpl,
      SelectCallback select_callback);
  ~AstraWorkspaceTemplateCardView() override;

  AstraWorkspaceTemplateCardView(const AstraWorkspaceTemplateCardView&) = delete;
  AstraWorkspaceTemplateCardView& operator=(
      const AstraWorkspaceTemplateCardView&) = delete;

  // -- State ---------------------------------------------------------------

  void SetSelected(bool selected);
  bool IsSelected() const { return is_selected_; }

  const std::string& template_id() const { return template_id_; }
  const std::string& template_name() const { return template_name_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaintBorder(gfx::Canvas* canvas) override;

 private:
  void BuildLayout();
  void UpdateVisualState();

  std::string template_id_;
  std::string template_name_;
  std::string description_;
  std::string accent_color_;
  int tab_count_ = 0;
  AstraWorkspaceTemplateCategory category_;
  bool is_selected_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::View> accent_bar_ = nullptr;
  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> description_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
};

// =========================================================================
// AstraWorkspaceTemplatePickerView — template picker bubble
// =========================================================================
//
// A bubble dialog that lets users pick a workspace template when creating
// a new workspace.  Shows template categories and template cards in a
// grid layout.
//
// Layout:
//   +-------------------------------------------+
//   |  Create workspace from template    [X]    |
//   +-------------------------------------------+
//   |  All  |  Productivity  |  Development  ...  (category chips)
//   +-------------------------------------------+
//   |  +----------+  +----------+  +----------+  |
//   |  |  [icon]  |  |  [icon]  |  |  [icon]  |  |
//   |  |  Coding  |  |  Design  |  |  Research|  |
//   |  |  3 tabs  |  |  2 tabs  |  |  4 tabs  |  |
//   |  +----------+  +----------+  +----------+  |
//   |                                           |
//   |  +----------+  +----------+               |
//   |  |  [icon]  |  |  [icon]  |               |
//   |  |  Blank   |  |  Reading |               |
//   |  |  0 tabs  |  |  2 tabs  |               |
//   |  +----------+  +----------+               |
//   +-------------------------------------------+
//   |  [ Cancel ]              [ Create workspace ] |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Template data comes from
// AstraWorkspaceTemplate (browser layer).  Selection is reported via
// a callback to the controller.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
// =========================================================================

class AstraWorkspaceTemplatePickerView
    : public views::BubbleDialogDelegateView {
 public:
  // Callback when a template is selected and "Create" is clicked.
  using TemplateSelectedCallback =
      base::RepeatingCallback<void(const std::string& template_id)>;

  explicit AstraWorkspaceTemplatePickerView(
      views::View* anchor_view,
      TemplateSelectedCallback callback);
  ~AstraWorkspaceTemplatePickerView() override;

  AstraWorkspaceTemplatePickerView(
      const AstraWorkspaceTemplatePickerView&) = delete;
  AstraWorkspaceTemplatePickerView& operator=(
      const AstraWorkspaceTemplatePickerView&) = delete;

  // -- Selection -----------------------------------------------------------

  void SetSelectedTemplate(const std::string& template_id);
  const std::string& selected_template_id() const {
    return selected_template_id_;
  }

  // -- Category filter -----------------------------------------------------

  void SetCategoryFilter(
      std::optional<AstraWorkspaceTemplateCategory> category);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildHeader();
  void BuildCategoryChips();
  void BuildTemplateGrid();
  void BuildFooter();

  void RefreshTemplateGrid();

  void OnTemplateSelected(const std::string& template_id);
  void OnCategoryClicked(AstraWorkspaceTemplateCategory category);
  void OnCreateClicked();
  void OnCancelClicked();

  TemplateSelectedCallback selected_callback_;

  std::string selected_template_id_;
  std::optional<AstraWorkspaceTemplateCategory> active_category_;

  // Child views.
  raw_ptr<views::View> category_bar_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> template_grid_ = nullptr;
  raw_ptr<views::MdTextButton> create_button_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;

  // Template cards (owned by template_grid_).
  std::vector<raw_ptr<AstraWorkspaceTemplateCardView>> template_cards_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_TEMPLATE_PICKER_VIEW_H_
