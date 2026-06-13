// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUT_PICKER_VIEW_H_
#define ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUT_PICKER_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "astra/ui/views/split_view/astra_split_layouts_model.h"

namespace views {
class BoxLayout;
class Button;
class ImageButton;
class ImageView;
class Label;
class MdTextButton;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSplitLayoutPickerView — split view layout selector
// =========================================================================
//
// A popup/bubble that shows saved split view layouts as visual previews.
// Users can click a layout to apply it, or save the current layout.
//
// Layout:
//
//   +------------------------------------------+
//   |  Layouts                         [+ Save]|
//   |  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   |
//   |  | 50/50| |70/30 | |3 panes| | 2x2  |   |
//   |  |      | |      | |      | | grid |   |
//   |  └──────┘ └──────┘ └──────┘ └──────┘   |
//   |  Equal  Focus  Three    Grid           |
//   |                                          |
//   |  Your layouts:                           |
//   |  ┌──────┐ ┌──────┐                      |
//   |  | My   | | Dev  |                      |
//   |  |layout| |mode |                      |
//   |  └──────┘ └──────┘                      |
//   +------------------------------------------+
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView (bubble pattern)
//
// TODO(astra): Integrate with split view controller to apply layouts.
//   Chromium owner: astra/ui/views/split_view/astra_split_view_controller.h
// =========================================================================

// Single layout preview card.
class AstraSplitLayoutPreviewView : public views::Button {
 public:
  AstraSplitLayoutPreviewView(const AstraSplitLayout& layout,
                              base::RepeatingCallback<void()> callback);
  ~AstraSplitLayoutPreviewView() override;

  AstraSplitLayoutPreviewView(const AstraSplitLayoutPreviewView&) = delete;
  AstraSplitLayoutPreviewView& operator=(
      const AstraSplitLayoutPreviewView&) = delete;

  const std::string& layout_id() const { return layout_id_; }

  // views::View:
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void DrawLayoutPreview(gfx::Canvas* canvas,
                         const gfx::Rect& bounds,
                         const AstraSplitLayout& layout);

  std::string layout_id_;
  std::u16string layout_name_;

  raw_ptr<views::View> preview_area_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
};

// Main layout picker bubble.
class AstraSplitLayoutPickerView : public views::BubbleDialogDelegateView,
                                   public AstraSplitLayoutsObserver {
 public:
  explicit AstraSplitLayoutPickerView(views::View* anchor_view,
                                      AstraSplitLayoutsModel* model);
  ~AstraSplitLayoutPickerView() override;

  AstraSplitLayoutPickerView(const AstraSplitLayoutPickerView&) = delete;
  AstraSplitLayoutPickerView& operator=(
      const AstraSplitLayoutPickerView&) = delete;

  // Set the model.
  void SetModel(AstraSplitLayoutsModel* model);
  AstraSplitLayoutsModel* model() { return model_; }

  // Refresh from model.
  void RefreshFromModel();

  // -- AstraSplitLayoutsObserver -------------------------------------------

  void OnLayoutSaved(const AstraSplitLayout& layout) override;
  void OnLayoutDeleted(const std::string& layout_id) override;
  void OnLayoutApplied(const std::string& layout_id) override;
  void OnLayoutsReordered() override;
  void OnSplitLayoutsModelShutdown() override;

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

  // Accessors for testing.
  views::MdTextButton* save_button() { return save_button_; }
  views::View* builtin_section() { return builtin_section_; }
  views::View* user_section() { return user_section_; }

 private:
  void BuildUI();
  void BuildHeader();
  void BuildBuiltInSection();
  void BuildUserSection();

  // Rebuild layout preview grids.
  void RebuildLayouts();

  // Event handlers.
  void OnSaveClicked();
  void OnLayoutClicked(const std::string& layout_id);

  // Draw layout icons.
  static void DrawLayoutIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             AstraSplitLayoutMode mode,
                             SkColor color);

  // Model (not owned).
  raw_ptr<AstraSplitLayoutsModel> model_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraSplitLayoutsModel,
                          AstraSplitLayoutsObserver>
      scoped_observation_{this};

  // Header.
  raw_ptr<views::MdTextButton> save_button_ = nullptr;

  // Built-in layouts section.
  raw_ptr<views::View> builtin_section_ = nullptr;
  raw_ptr<views::View> builtin_grid_ = nullptr;

  // User layouts section.
  raw_ptr<views::View> user_section_ = nullptr;
  raw_ptr<views::View> user_grid_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_LAYOUT_PICKER_VIEW_H_
