// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_LABELS_ASTRA_TAB_LABELS_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_LABELS_ASTRA_TAB_LABELS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Checkbox;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabLabelItemView — single label tag chip
// =========================================================================
//
// A colored chip representing a tab label, showing label name and count
// of tabs with this label.
//
// Layout:
//   +-------------------+
//   | 🔴 Work (12)       |
//   +-------------------+
// =========================================================================

class AstraTabLabelItemView : public views::View {
 public:
  using ClickCallback = base::RepeatingCallback<void(const std::string& label_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& label_id)>;

  struct LabelInfo {
    std::string label_id;
    std::u16string name;
    SkColor color = SK_ColorRED;
    int tab_count = 0;
    bool is_selected = false;
  };

  AstraTabLabelItemView(const LabelInfo& info,
                        ClickCallback click_callback,
                        DeleteCallback delete_callback);
  ~AstraTabLabelItemView() override;

  AstraTabLabelItemView(const AstraTabLabelItemView&) = delete;
  AstraTabLabelItemView& operator=(const AstraTabLabelItemView&) = delete;

  const std::string& label_id() const { return label_id_; }
  bool is_selected() const { return is_selected_; }

  void SetSelected(bool selected);
  void SetTabCount(int count);

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnClicked();
  void OnDeleteClicked();

  void PaintColorDot(gfx::Canvas* canvas);

  std::string label_id_;
  std::u16string name_;
  SkColor color_ = SK_ColorRED;
  int tab_count_ = 0;
  bool is_selected_ = false;

  ClickCallback click_callback_;
  DeleteCallback delete_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::MdTextButton> delete_button_ = nullptr;
};

// =========================================================================
// AstraTabLabeledTabItemView — a tab showing its labels
// =========================================================================
//
// A tab row that displays labels assigned to it, used in the label
// assignment / tab listing view.
// =========================================================================

class AstraTabLabeledTabItemView : public views::View {
 public:
  struct TabInfo {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    std::vector<std::string> label_ids;
    bool is_selected = false;
  };

  AstraTabLabeledTabItemView(const TabInfo& info);
  ~AstraTabLabeledTabItemView() override;

  AstraTabLabeledTabItemView(const AstraTabLabeledTabItemView&) = delete;
  AstraTabLabeledTabItemView& operator=(
      const AstraTabLabeledTabItemView&) = delete;

  const std::string& tab_id() const { return tab_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  std::vector<std::string> label_ids_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<views::View> labels_row_ = nullptr;
};

// =========================================================================
// AstraTabLabelsView — tab labels panel
// =========================================================================
//
// A bubble showing tab labels (tags): create labels, assign to tabs,
// filter tabs by label.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Labels                   [Close]    |
//   +-------------------------------------------+
//   |  [ + New label ]                           |
//   +-------------------------------------------+
//   |  Labels                                     |
//   |  🔴 Work (12)  🟢 Personal (5)             |
//   |  🔵 Research (8) 🟡 To Read (3)          |
//   +-------------------------------------------+
//   |  Tabs with "Work" label (12)               |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ Project Alpha — work.com              │  |
//   |  │  🔴 Work                              │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Label metadata is persisted by
// Astra's tab labels service (as tab-level metadata).
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - TabStripModel (source of tab data)
//   - content::WebContentsUserData (for tab label storage)
// =========================================================================

class AstraTabLabelsView : public views::BubbleDialogDelegateView {
 public:
  using NewLabelCallback = base::RepeatingClosure;
  using DeleteLabelCallback =
      base::RepeatingCallback<void(const std::string& label_id)>;
  using LabelSelectedCallback =
      base::RepeatingCallback<void(const std::string& label_id)>;

  explicit AstraTabLabelsView(views::View* anchor_view);
  ~AstraTabLabelsView() override;

  AstraTabLabelsView(const AstraTabLabelsView&) = delete;
  AstraTabLabelsView& operator=(const AstraTabLabelsView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetLabels(
      const std::vector<AstraTabLabelItemView::LabelInfo>& labels);
  void SetTabs(
      const std::vector<AstraTabLabeledTabItemView::TabInfo>& tabs);
  void SetSelectedLabel(const std::string& label_id);

  // -- Callbacks -----------------------------------------------------------

  void SetNewLabelCallback(NewLabelCallback callback);
  void SetDeleteLabelCallback(DeleteLabelCallback callback);
  void SetLabelSelectedCallback(LabelSelectedCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildNewLabelSection();
  void BuildLabelsRow();
  void BuildTabsSection();

  void RefreshLabels();
  void RefreshTabs();

  void OnNewLabel();
  void OnLabelSelected(const std::string& label_id);
  void OnDeleteLabel(const std::string& label_id);

  std::vector<AstraTabLabelItemView::LabelInfo> labels_;
  std::vector<AstraTabLabeledTabItemView::TabInfo> tabs_;
  std::string selected_label_id_;

  NewLabelCallback new_label_callback_;
  DeleteLabelCallback delete_label_callback_;
  LabelSelectedCallback label_selected_callback_;

  raw_ptr<views::MdTextButton> new_label_button_ = nullptr;
  raw_ptr<views::View> labels_row_ = nullptr;
  raw_ptr<views::Label> tabs_header_label_ = nullptr;
  raw_ptr<views::View> tabs_list_ = nullptr;

  std::vector<raw_ptr<AstraTabLabelItemView>> label_views_;
  std::vector<raw_ptr<AstraTabLabeledTabItemView>> tab_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_LABELS_ASTRA_TAB_LABELS_VIEW_H_
