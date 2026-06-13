// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_OVERVIEW_ASTRA_TAB_OVERVIEW_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_OVERVIEW_ASTRA_TAB_OVERVIEW_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabOverviewTileView — single tab tile in overview grid
// =========================================================================
//
// A rectangular tile representing one tab in the overview grid.
// Shows a colored favicon area, tab title, and domain.
//
// Layout:
//   +-----------------------+
//   |  🟦                    |
//   |                       |
//   |  Tab Title             |
//   |  domain.com            |
//   +-----------------------+
// =========================================================================

class AstraTabOverviewTileView : public views::View {
 public:
  using ClickCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CloseCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  struct TabInfo {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    std::string url;
    SkColor favicon_color = SkColorSetRGB(0x5A, 0x9B, 0xE5);
    bool is_active = false;
    bool is_pinned = false;
    bool is_audible = false;
    int tab_index = 0;
  };

  AstraTabOverviewTileView(const TabInfo& info,
                           ClickCallback click_callback,
                           CloseCallback close_callback);
  ~AstraTabOverviewTileView() override;

  AstraTabOverviewTileView(const AstraTabOverviewTileView&) = delete;
  AstraTabOverviewTileView& operator=(
      const AstraTabOverviewTileView&) = delete;

  const std::string& tab_id() const { return tab_id_; }
  bool is_active() const { return is_active_; }

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnClicked();
  void OnCloseClicked();

  void PaintFavicon(gfx::Canvas* canvas);
  void PaintCloseButton(gfx::Canvas* canvas);

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  SkColor favicon_color_ = SkColorSetRGB(0x5A, 0x9B, 0xE5);
  bool is_active_ = false;
  bool is_pinned_ = false;
  bool is_audible_ = false;
  int tab_index_ = 0;

  ClickCallback click_callback_;
  CloseCallback close_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<views::View> favicon_area_ = nullptr;
};

// =========================================================================
// AstraTabOverviewView — tab overview / tab grid panel
// =========================================================================
//
// A bubble showing all tabs in a grid overview (Safari-style), with
// search, grouping options, and quick close actions.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Overview                 [Close]    |
//   +-------------------------------------------+
//   |  🔍 Search tabs...                         |
//   +-------------------------------------------+
//   |  12 tabs · 3 windows                       |
//   +-------------------------------------------+
//   |  ┌──────┐ ┌──────┐ ┌──────┐             |
//   |  │ 🟦    │ │ 🟩    │ │ 🟨    │             |
//   |  │Title1 │ │Title2 │ │Title3 │             |
//   |  │d.com  │ │d.com  │ │d.com  │             |
//   |  └──────┘ └──────┘ └──────┘             |
//   |  ┌──────┐ ┌──────┐ ┌──────┐             |
//   |  │ 🟪    │ │ 🟥    │ │ 🟦    │             |
//   |  │Title4 │ │Title5 │ │Title6 │             |
//   |  │d.com  │ │d.com  │ │d.com  │             |
//   |  └──────┘ └──────┘ └──────┘             |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Tab data comes from TabStripModel.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - TabStripModel (source of tab data)
// =========================================================================

class AstraTabOverviewView : public views::BubbleDialogDelegateView {
 public:
  using TabClickCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using TabCloseCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using SearchCallback =
      base::RepeatingCallback<void(const std::u16string& query)>;

  enum class GroupMode { kNone, kByDomain, kByWindow, kByRecent };

  explicit AstraTabOverviewView(views::View* anchor_view);
  ~AstraTabOverviewView() override;

  AstraTabOverviewView(const AstraTabOverviewView&) = delete;
  AstraTabOverviewView& operator=(const AstraTabOverviewView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetTabs(
      const std::vector<AstraTabOverviewTileView::TabInfo>& tabs);
  void SetGroupMode(GroupMode mode);

  // -- Callbacks -----------------------------------------------------------

  void SetTabClickCallback(TabClickCallback callback);
  void SetTabCloseCallback(TabCloseCallback callback);
  void SetSearchCallback(SearchCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSearchBar();
  void BuildSummaryRow();
  void BuildTabsGrid();

  void RefreshTabs();
  void RefreshSummary();

  void OnTabClicked(const std::string& tab_id);
  void OnTabClosed(const std::string& tab_id);
  void OnSearchTextChanged();

  static std::u16 GroupModeLabel(GroupMode mode);

  std::vector<AstraTabOverviewTileView::TabInfo> tabs_;
  GroupMode group_mode_ = GroupMode::kNone;
  int window_count_ = 1;

  TabClickCallback click_callback_;
  TabCloseCallback close_callback_;
  SearchCallback search_callback_;

  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::Label> summary_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tabs_grid_ = nullptr;

  std::vector<raw_ptr<AstraTabOverviewTileView>> tile_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_OVERVIEW_ASTRA_TAB_OVERVIEW_VIEW_H_
