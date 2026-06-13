// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Button;
class Combobox;
class FlexLayout;
class ImageButton;
class ImageView;
class Label;
class MdTextButton;
class ScrollView;
class Separator;
class Textfield;
class ToggleButton;
}  // namespace views

namespace astra {

class AstraTabGroupsPageModel;
class AstraTabGroup;
enum class AstraTabGroupColor;

// Delegate interface for tab groups page interactions.
class AstraTabGroupsPageDelegate {
 public:
  virtual ~AstraTabGroupsPageDelegate() = default;

  // Called when a tab group should be activated/switched to.
  virtual void OnActivateGroup(const std::string& group_id) {}

  // Called when a tab within a group should be activated.
  virtual void OnActivateTab(const std::string& group_id,
                             const std::string& tab_id) {}

  // Called when a new group should be created.
  virtual void OnCreateNewGroup() {}

  // Called when a group should be closed (all tabs closed).
  virtual void OnCloseGroup(const std::string& group_id) {}

  // Called when a tab should be closed.
  virtual void OnCloseTab(const std::string& group_id,
                          const std::string& tab_id) {}

  // Called to open a new tab in a group.
  virtual void OnNewTabInGroup(const std::string& group_id) {}

  // Called to move a tab between groups.
  virtual void OnMoveTab(const std::string& source_group_id,
                         const std::string& target_group_id,
                         const std::string& tab_id) {}

  // Called to toggle group collapse state.
  virtual void OnToggleGroupCollapsed(const std::string& group_id) {}
};

// =========================================================================
// AstraTabGroupCardView — a single tab group card
// =========================================================================
//
// Displays a tab group with colored header, title, tab count,
// action buttons (collapse, pin, more), and a list of tab items.
// =========================================================================
class AstraTabGroupCardView : public views::View {
 public:
  explicit AstraTabGroupCardView(const std::string& group_id);
  ~AstraTabGroupCardView() override;

  AstraTabGroupCardView(const AstraTabGroupCardView&) = delete;
  AstraTabGroupCardView& operator=(const AstraTabGroupCardView&) = delete;

  // Update the card from a model entry.
  void UpdateFromModel(const AstraTabGroupsPageModel* model);

  const std::string& group_id() const { return group_id_; }

  // Set expanded/collapsed state.
  void SetExpanded(bool expanded);
  bool expanded() const { return expanded_; }

  // views::View:
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // Accessors for testing.
  views::Label* title_label() { return title_label_; }
  views::Label* tab_count_label() { return tab_count_label_; }
  views::ImageButton* collapse_button() { return collapse_button_; }
  views::ImageButton* pin_button() { return pin_button_; }
  views::ImageButton* more_button() { return more_button_; }
  views::View* tabs_container() { return tabs_container_; }
  int GetTabItemCount() const;

 private:
  void BuildLayout();
  void UpdateColors();
  void OnCollapseClicked();
  void OnPinClicked();
  void OnMoreClicked();
  void OnCardClicked();

  std::string group_id_;
  bool expanded_ = true;
  bool is_hovered_ = false;

  // Header.
  raw_ptr<views::View> header_ = nullptr;
  raw_ptr<views::View> color_stripe_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::ImageButton> collapse_button_ = nullptr;
  raw_ptr<views::ImageButton> pin_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  // Tabs container.
  raw_ptr<views::View> tabs_container_ = nullptr;

  // Owned tab item views.
  std::vector<raw_ptr<views::View, VectorExperimental>> tab_items_;
};

// =========================================================================
// AstraTabGroupsPageView — full page tab groups manager
// =========================================================================
//
// Full-page view for managing tab groups. Layout:
//
//   +-----------------------------------------------------------+
//   |  [Search]  [Sort v]  [Filter v]  [+ New group]           |  Toolbar
//   +-----------+-----------------------------------------------+
//   | Categories|  Tab group cards (grid or list)              |
//   |  - All    |                                               |
//   |  - Work   |  [Group Card] [Group Card] [Group Card]     |
//   |  - Personal|                                            |
//   |  ...      |                                               |
//   +-----------+-----------------------------------------------+
//
// Chromium subsystems reused:
//   - TabStripModel + tab_groups (truth source)
//   - views framework
//
// TODO(astra): Wire to Chrome's tab groups system.
//   Reference: components/tab_groups/core/tab_group_id.h
//   Patch point: chrome/browser/ui/views/tabs/tab_group_header.cc
// =========================================================================
class AstraTabGroupsPageView : public views::View,
                               public views::TextfieldController,
                               public AstraTabGroupsPageObserver {
 public:
  AstraTabGroupsPageView();
  ~AstraTabGroupsPageView() override;

  AstraTabGroupsPageView(const AstraTabGroupsPageView&) = delete;
  AstraTabGroupsPageView& operator=(const AstraTabGroupsPageView&) = delete;

  // Set the model to observe. The view does not own the model.
  void SetModel(AstraTabGroupsPageModel* model);
  AstraTabGroupsPageModel* model() { return model_; }

  // Set the delegate for page actions.
  void SetDelegate(AstraTabGroupsPageDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraTabGroupsPageDelegate* delegate() { return delegate_; }

  // Refresh everything from the model.
  void RefreshFromModel();

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // -- views::TextfieldController ------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // -- AstraTabGroupsPageObserver ------------------------------------------

  void OnTabGroupsChanged(AstraTabGroupsPageModel* model) override;
  void OnTabGroupAdded(AstraTabGroupsPageModel* model,
                       const std::string& group_id) override;
  void OnTabGroupRemoved(AstraTabGroupsPageModel* model,
                         const std::string& group_id) override;
  void OnTabGroupUpdated(AstraTabGroupsPageModel* model,
                         const std::string& group_id) override;
  void OnFilterChanged(AstraTabGroupsPageModel* model) override;
  void OnSearchChanged(AstraTabGroupsPageModel* model,
                       const std::u16string& query) override;
  void OnTabGroupsPageModelShutdown(AstraTabGroupsPageModel* model) override;

  // Accessors for testing.
  views::Textfield* search_field() { return search_field_; }
  views::Combobox* sort_combobox() { return sort_combobox_; }
  views::Combobox* filter_combobox() { return filter_combobox_; }
  views::View* categories_sidebar() { return categories_sidebar_; }
  views::ScrollView* content_scroll_view() { return content_scroll_view_; }
  views::View* groups_container() { return groups_container_; }
  views::View* empty_state() { return empty_state_; }
  views::MdTextButton* new_group_button() { return new_group_button_; }
  int GetGroupCardCount() const;
  AstraTabGroupCardView* GetGroupCardAt(int index) const;

 private:
  void BuildUI();
  void BuildToolbar();
  void BuildCategoriesSidebar();
  void BuildContentArea();
  void BuildEmptyState();

  // Refresh group cards list.
  void RefreshGroupCards();

  // Refresh categories sidebar.
  void RefreshCategories();

  // Update empty state visibility.
  void UpdateEmptyState();

  // Handle sort change.
  void OnSortChanged();

  // Handle filter change.
  void OnFilterChanged();

  // Handle category selection.
  void OnCategorySelected(const std::string& category_id);

  // Handle new group button.
  void OnNewGroup();

  // Draw helper: colored tab group indicator.
  void DrawGroupIndicator(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          SkColor color);

  // Model (not owned).
  raw_ptr<AstraTabGroupsPageModel> model_ = nullptr;

  // Delegate (not owned).
  raw_ptr<AstraTabGroupsPageDelegate> delegate_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraTabGroupsPageModel,
                          AstraTabGroupsPageObserver>
      scoped_observation_{this};

  // Toolbar.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::Combobox> sort_combobox_ = nullptr;
  raw_ptr<views::Combobox> filter_combobox_ = nullptr;
  raw_ptr<views::MdTextButton> new_group_button_ = nullptr;

  // Sidebar.
  raw_ptr<views::View> categories_sidebar_ = nullptr;
  raw_ptr<views::View> categories_container_ = nullptr;
  raw_ptr<views::Label> categories_header_ = nullptr;

  // Content area.
  raw_ptr<views::ScrollView> content_scroll_view_ = nullptr;
  raw_ptr<views::View> groups_container_ = nullptr;
  raw_ptr<views::View> empty_state_ = nullptr;
  raw_ptr<views::Label> empty_state_title_ = nullptr;
  raw_ptr<views::Label> empty_state_desc_ = nullptr;

  // Stats bar at bottom.
  raw_ptr<views::View> stats_bar_ = nullptr;
  raw_ptr<views::Label> stats_label_ = nullptr;

  // Owned group card views.
  std::vector<raw_ptr<AstraTabGroupCardView, VectorExperimental>>
      group_cards_;

  // Selected category (empty = all).
  std::string selected_category_;

  // Layout constants.
  static constexpr int kToolbarHeight = 56;
  static constexpr int kSidebarWidth = 200;
  static constexpr int kCardMinWidth = 280;
  static constexpr int kCardSpacing = 16;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_VIEW_H_
