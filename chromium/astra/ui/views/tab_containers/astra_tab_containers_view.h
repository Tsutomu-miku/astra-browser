// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_CONTAINERS_ASTRA_TAB_CONTAINERS_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_CONTAINERS_ASTRA_TAB_CONTAINERS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Button;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraContainerItemView — single tab container row
// =========================================================================
//
// A row showing one tab container: color dot, name, tab count, and actions.
//
// Layout:
//   +-------------------------------------------+
//   |  ●  Personal          12 tabs   [Open]   |
//   +-------------------------------------------+
// =========================================================================

class AstraContainerItemView : public views::View {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;
  using EditCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;

  struct ContainerInfo {
    std::string container_id;
    std::u16string name;
    SkColor color = SK_ColorBLUE;
    int tab_count = 0;
    bool is_default = false;
    bool is_active = false;
  };

  AstraContainerItemView(const ContainerInfo& info,
                         SelectCallback select_callback,
                         EditCallback edit_callback,
                         DeleteCallback delete_callback);
  ~AstraContainerItemView() override;

  AstraContainerItemView(const AstraContainerItemView&) = delete;
  AstraContainerItemView& operator=(const AstraContainerItemView&) = delete;

  const std::string& container_id() const { return container_id_; }
  bool is_active() const { return is_active_; }

  void SetActive(bool active);

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnSelect();
  void OnEdit();
  void OnDelete();
  void PaintColorDot(gfx::Canvas* canvas);

  std::string container_id_;
  std::u16string name_;
  SkColor color_ = SK_ColorBLUE;
  int tab_count_ = 0;
  bool is_default_ = false;
  bool is_active_ = false;

  SelectCallback select_callback_;
  EditCallback edit_callback_;
  DeleteCallback delete_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::MdTextButton> open_button_ = nullptr;
};

// =========================================================================
// AstraTabContainersView — tab containers management panel
// =========================================================================
//
// A bubble showing color-coded tab containers (like Firefox Multi-Account
// Containers). Each container isolates tabs with separate cookie jars and
// identities.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Containers               [Close]    |
//   +-------------------------------------------+
//   |  [+ New Container]                        |
//   +-------------------------------------------+
//   |  ●  Personal          12 tabs   [Open]   |
//   |  ●  Work               8 tabs   [Open]   |
//   |  ●  Banking            2 tabs   [Open]   |
//   |  ●  Social             5 tabs   [Open]   |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Container state and tab isolation
// are managed by Astra's tab containers service, which projects onto
// Chromium's profile/cookie isolation infrastructure.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - Profile / CookieStore / StoragePartition (identity isolation)
// =========================================================================

class AstraTabContainersView : public views::BubbleDialogDelegateView {
 public:
  using ContainerSelectedCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;
  using ContainerCreateCallback = base::RepeatingClosure;
  using ContainerEditCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;
  using ContainerDeleteCallback =
      base::RepeatingCallback<void(const std::string& container_id)>;

  explicit AstraTabContainersView(views::View* anchor_view);
  ~AstraTabContainersView() override;

  AstraTabContainersView(const AstraTabContainersView&) = delete;
  AstraTabContainersView& operator=(const AstraTabContainersView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetContainers(
      const std::vector<AstraContainerItemView::ContainerInfo>& containers);
  void SetActiveContainer(const std::string& container_id);

  // -- Callbacks -----------------------------------------------------------

  void SetContainerSelectedCallback(ContainerSelectedCallback callback);
  void SetContainerCreateCallback(ContainerCreateCallback callback);
  void SetContainerEditCallback(ContainerEditCallback callback);
  void SetContainerDeleteCallback(ContainerDeleteCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildHeader();
  void BuildContainerList();

  void RefreshContainers();

  void OnNewContainer();

  std::vector<AstraContainerItemView::ContainerInfo> containers_;
  std::string active_container_id_;

  ContainerSelectedCallback select_callback_;
  ContainerCreateCallback create_callback_;
  ContainerEditCallback edit_callback_;
  ContainerDeleteCallback delete_callback_;

  raw_ptr<views::View> containers_list_ = nullptr;
  raw_ptr<views::MdTextButton> new_button_ = nullptr;

  std::vector<raw_ptr<AstraContainerItemView>> container_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_CONTAINERS_ASTRA_TAB_CONTAINERS_VIEW_H_
