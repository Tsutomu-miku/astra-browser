// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_ANCESTRY_ASTRA_TAB_ANCESTRY_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_ANCESTRY_ASTRA_TAB_ANCESTRY_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabAncestryNodeView — single node in the tab family tree
// =========================================================================
//
// A tree node representing one tab, with nested children for tabs opened
// from this tab (e.g. via "open in new tab").
//
// Layout (collapsed):
//   ▶ 📄 Tab Title
//
// Layout (expanded):
//   ▼ 📄 Tab Title
//     ├── 📄 Child Tab A
//     │     └── 📄 Grandchild Tab
//     └── 📄 Child Tab B
// =========================================================================

class AstraTabAncestryNodeView : public views::View {
 public:
  using TabClickCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CollapseCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  struct TabNode {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    std::string favicon_url;
    base::Time last_active;
    bool is_active = false;
    int depth = 0;
    std::vector<TabNode> children;
  };

  AstraTabAncestryNodeView(const TabNode& node,
                           TabClickCallback click_callback,
                           CollapseCallback collapse_callback);
  ~AstraTabAncestryNodeView() override;

  AstraTabAncestryNodeView(const AstraTabAncestryNodeView&) = delete;
  AstraTabAncestryNodeView& operator=(
      const AstraTabAncestryNodeView&) = delete;

  const std::string& tab_id() const { return tab_id_; }
  bool expanded() const { return expanded_; }

  void SetExpanded(bool expanded);
  void SetActive(bool active);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  void BuildLayout();
  void BuildChildren();
  void OnToggleClicked();
  void OnTabClicked();

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  int depth_ = 0;
  bool expanded_ = true;
  bool is_active_ = false;

  TabNode node_;
  TabClickCallback click_callback_;
  CollapseCallback collapse_callback_;

  raw_ptr<views::View> toggle_button_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::View> children_container_ = nullptr;

  std::vector<raw_ptr<AstraTabAncestryNodeView>> child_nodes_;
};

// =========================================================================
// AstraTabAncestryView — tab family tree browser panel
// =========================================================================
//
// A bubble / side panel showing the "family tree" of tabs — how tabs were
// opened from other tabs via link clicks.  Helps visualize browsing
// sessions and find related tabs.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Family Tree              [Close]   |
//   +-------------------------------------------+
//   |  [Collapse all] [Expand all]  [🌳 Tree]   |
//   +-------------------------------------------+
//   |  ▼ 📄 Google Search                         |
//   |    ├── ▶ 📄 Wikipedia Article              |
//   |    │     └── ▶ 📄 Reference Paper          |
//   |    └── ▼ 📄 News Site                      |
//   |          ├── 📄 Story A                    |
//   |          └── 📄 Story B                    |
//   |  ...                                       |
//   +-------------------------------------------+
//   |  12 tabs · 3 branches · depth 4            |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Ancestry data comes from Astra's
// tab ancestry service which observes TabStripModel changes.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - Custom Skia drawing for tree connectors
//   - TabStripModel (source of ancestry data via observer)
// =========================================================================

class AstraTabAncestryView : public views::BubbleDialogDelegateView {
 public:
  using TabActivatedCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CollapseAllCallback = base::RepeatingClosure;
  using ExpandAllCallback = base::RepeatingClosure;

  explicit AstraTabAncestryView(views::View* anchor_view);
  ~AstraTabAncestryView() override;

  AstraTabAncestryView(const AstraTabAncestryView&) = delete;
  AstraTabAncestryView& operator=(const AstraTabAncestryView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetRootNodes(
      const std::vector<AstraTabAncestryNodeView::TabNode>& roots);

  void SetActiveTabId(const std::string& tab_id);

  // -- Stats ---------------------------------------------------------------

  void SetTabCount(int count);
  void SetBranchCount(int count);
  void SetMaxDepth(int depth);

  // -- Callbacks -----------------------------------------------------------

  void SetTabActivatedCallback(TabActivatedCallback callback);
  void SetCollapseAllCallback(CollapseAllCallback callback);
  void SetExpandAllCallback(ExpandAllCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildToolbar();
  void BuildTreeView();
  void BuildStatsFooter();

  void RefreshTree();
  void RefreshStats();

  void OnTabClicked(const std::string& tab_id);
  void OnNodeCollapsed(const std::string& tab_id);
  void OnCollapseAllClicked();
  void OnExpandAllClicked();

  // Tree data.
  std::vector<AstraTabAncestryNodeView::TabNode> root_nodes_;
  std::string active_tab_id_;

  // Stats.
  int tab_count_ = 0;
  int branch_count_ = 0;
  int max_depth_ = 0;

  // Callbacks.
  TabActivatedCallback tab_activated_callback_;
  CollapseAllCallback collapse_all_callback_;
  ExpandAllCallback expand_all_callback_;

  // Child views.
  raw_ptr<views::MdTextButton> collapse_all_button_ = nullptr;
  raw_ptr<views::MdTextButton> expand_all_button_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tree_container_ = nullptr;
  raw_ptr<views::Label> stats_label_ = nullptr;

  // Root node views (owned by tree_container_).
  std::vector<raw_ptr<AstraTabAncestryNodeView>> root_node_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_ANCESTRY_ASTRA_TAB_ANCESTRY_VIEW_H_
