// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_STRIP_VIEW_H_
#define ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_STRIP_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class ImageButton;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

class AstraVerticalTabView;

// =========================================================================
// AstraVerticalTabStripView — Arc-style vertical tab strip
// =========================================================================
//
// A vertical tab strip that replaces or augments the traditional horizontal
// tab strip.  Inspired by Arc Browser, Vivaldi, and Sidebery.
//
// Layout:
//
//   +---------------------------+
//   |  [🔍] Search tabs          |
//   +---------------------------+
//   |  Pinned                    |
//   |  [📌][📌][📌]              |
//   +---------------------------+
//   |  Tabs                      |
//   |  📄 Tab 1              [x] |
//   |  📄 Tab 2 is longe... [x]  |
//   |  📄 Tab 3              [x] |
//   |  📄 Tab 4              [x] |
//   |  ...                      |
//   +---------------------------+
//   |  [+] New tab   [≡] Menu   |
//   +---------------------------+
//
// Features:
//   - Pinned tabs section (icon-only grid)
//   - Scrollable regular tabs list
//   - Tab search (quick find)
//   - Tab groups/stacks
//   - Collapsible to icon-only mode
//   - New tab button
//
// Chromium subsystems reused:
//   - TabStripModel (truth source for tabs)
//   - TabStripModelObserver (for tab change notifications)
//   - views::ScrollView (for scrollable tab list)
//
// TODO(astra): Wire to TabStripModel via TabStripModelObserver.
//   Chromium owner: chrome/browser/ui/tabs/tab_strip_model.h
//   Patch point: chrome/browser/ui/views/tabs/tab_strip_controller.h
// TODO(astra): Integrate with AstraTabFeatures for tab metadata.
//   Chromium owner: astra/browser/astra_tab_features.h
// =========================================================================

// Observer for vertical tab strip.
class AstraVerticalTabStripObserver : public base::CheckedObserver {
 public:
  // Called when a tab is activated/selected.
  virtual void OnTabActivated(const std::string& tab_id) {}

  // Called when a tab is closed.
  virtual void OnTabClosed(const std::string& tab_id) {}

  // Called when a new tab is requested.
  virtual void OnNewTabRequested() {}

  // Called when the tab strip is collapsed/expanded.
  virtual void OnTabStripCollapsed(bool collapsed) {}

  // Called when tab search is activated.
  virtual void OnTabSearchActivated() {}

 protected:
  ~AstraVerticalTabStripObserver() override = default;
};

// Data for a tab in the vertical strip.
struct AstraVerticalTabData {
  std::string id;
  std::u16string title;
  std::u16string url;
  // gfx::ImageSkia favicon;  // Would use favicon in real implementation
  bool is_active = false;
  bool is_pinned = false;
  bool is_audible = false;
  bool is_loading = false;
  bool is_crashed = false;
  std::string group_id;     // Tab group / stack ID
  std::string workspace_id;  // Workspace this tab belongs to
  int index = -1;           // Position in TabStripModel
};

// Main vertical tab strip view.
class AstraVerticalTabStripView : public views::View {
 public:
  AstraVerticalTabStripView();
  ~AstraVerticalTabStripView() override;

  AstraVerticalTabStripView(const AstraVerticalTabStripView&) = delete;
  AstraVerticalTabStripView& operator=(const AstraVerticalTabStripView&) =
      delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraVerticalTabStripObserver* observer);
  void RemoveObserver(AstraVerticalTabStripObserver* observer);

  // -- Tab management -------------------------------------------------------

  // Add a tab to the strip.
  void AddTab(const AstraVerticalTabData& tab_data);

  // Remove a tab by ID.
  void RemoveTab(const std::string& tab_id);

  // Activate a tab by ID.
  void ActivateTab(const std::string& tab_id);

  // Update a tab's data.
  void UpdateTab(const AstraVerticalTabData& tab_data);

  // Get tab count.
  size_t GetTabCount() const { return tabs_.size(); }
  size_t GetPinnedTabCount() const;

  // Get all tabs.
  const std::vector<AstraVerticalTabData>& GetTabs() const { return tabs_; }

  // Get active tab ID.
  std::string GetActiveTabId() const;

  // -- Strip state ----------------------------------------------------------

  // Collapse/expand the tab strip.
  void SetCollapsed(bool collapsed);
  bool IsCollapsed() const { return is_collapsed_; }
  void ToggleCollapsed();

  // Set the strip width.
  void SetStripWidth(int width);
  int GetStripWidth() const;

  // -- Populate -------------------------------------------------------------

  // Populate with sample tabs for testing/demo.
  void PopulateSampleTabs();

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize() const override;

  // -- Testing accessors ---------------------------------------------------

  views::Textfield* search_field() { return search_field_; }
  views::ImageButton* new_tab_button() { return new_tab_button_; }
  views::ImageButton* collapse_button() { return collapse_button_; }
  views::View* pinned_tab_container() { return pinned_container_; }
  views::ScrollView* scroll_view() { return scroll_view_; }
  views::View* tab_list() { return tab_list_; }

 private:
  void BuildUI();
  void BuildHeader();
  void BuildPinnedSection();
  void BuildTabList();
  void BuildFooter();

  // Rebuild tab views from model.
  void RebuildTabViews();
  void RebuildPinnedViews();

  // Find tab data by ID.
  AstraVerticalTabData* FindTab(const std::string& tab_id);

  // Event handlers.
  void OnTabClicked(const std::string& tab_id);
  void OnTabClosed(const std::string& tab_id);
  void OnNewTabClicked();
  void OnCollapseClicked();
  void OnSearchChanged();

  // Notify helpers.
  void NotifyTabActivated(const std::string& tab_id);
  void NotifyTabClosed(const std::string& tab_id);
  void NotifyNewTabRequested();
  void NotifyTabStripCollapsed(bool collapsed);

  // Draw icons.
  static void DrawNewTabIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawCollapseIcon(gfx::Canvas* canvas,
                               const gfx::Rect& bounds,
                               SkColor color);
  static void DrawSearchIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawMenuIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);

  // Tab data model.
  std::vector<AstraVerticalTabData> tabs_;

  // State.
  bool is_collapsed_ = false;
  int strip_width_ = 240;

  // Views.
  raw_ptr<views::View> header_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> collapse_button_ = nullptr;

  raw_ptr<views::View> pinned_section_ = nullptr;
  raw_ptr<views::Label> pinned_label_ = nullptr;
  raw_ptr<views::View> pinned_container_ = nullptr;

  raw_ptr<views::View> tabs_section_ = nullptr;
  raw_ptr<views::Label> tabs_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tab_list_ = nullptr;

  raw_ptr<views::View> footer_ = nullptr;
  raw_ptr<views::ImageButton> new_tab_button_ = nullptr;
  raw_ptr<views::ImageButton> menu_button_ = nullptr;

  // Observers.
  base::ObserverList<AstraVerticalTabStripObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_STRIP_VIEW_H_
