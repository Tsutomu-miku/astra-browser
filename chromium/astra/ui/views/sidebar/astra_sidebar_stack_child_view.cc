#include "astra/ui/views/sidebar/astra_sidebar_stack_child_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_tab_item_view.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Spacing between tab items within a stack child area.
constexpr int kTabItemSpacing = 1;

// Horizontal padding for the child area (matches the indent visual).
constexpr int kChildHorizontalPadding = 6;

}  // namespace

AstraSidebarStackChildView::AstraSidebarStackChildView() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Vertical box layout for tab items.
  layout_ = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(2, kChildHorizontalPadding),
      kTabItemSpacing));
  layout_->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
}

AstraSidebarStackChildView::~AstraSidebarStackChildView() = default;

// =========================================================================
// Tab data management
// =========================================================================

void AstraSidebarStackChildView::SetTabs(
    const std::vector<AstraStackTabInfo>& tabs) {
  tab_infos_ = tabs;
  active_tab_index_ = -1;

  // Find active tab.
  for (size_t i = 0; i < tab_infos_.size(); ++i) {
    if (tab_infos_[i].is_active) {
      active_tab_index_ = static_cast<int>(i);
      break;
    }
  }

  RebuildTabViews();
}

int AstraSidebarStackChildView::GetTabCount() const {
  return static_cast<int>(tab_infos_.size());
}

AstraStackTabInfo AstraSidebarStackChildView::GetTabAt(int index) const {
  if (index < 0 || index >= static_cast<int>(tab_infos_.size())) {
    return AstraStackTabInfo();
  }
  return tab_infos_[index];
}

void AstraSidebarStackChildView::AddTab(const AstraStackTabInfo& tab,
                                        int position) {
  int insert_pos = (position < 0 || position > static_cast<int>(tab_infos_.size()))
                       ? static_cast<int>(tab_infos_.size())
                       : position;

  tab_infos_.insert(tab_infos_.begin() + insert_pos, tab);

  // Update active tab index if needed.
  if (tab.is_active) {
    active_tab_index_ = insert_pos;
  } else if (active_tab_index_ >= insert_pos) {
    active_tab_index_++;
  }

  // Update indices for all tabs after insertion point.
  for (size_t i = insert_pos; i < tab_infos_.size(); ++i) {
    tab_infos_[i].index_in_stack = static_cast<int>(i);
  }

  // Create and insert the view.
  auto tab_view = CreateTabItemView(tab);
  if (insert_pos >= static_cast<int>(children().size())) {
    AddChildView(std::move(tab_view));
  } else {
    AddChildViewAt(std::move(tab_view), insert_pos);
  }

  // Update first/last indicators.
  if (insert_pos == 0) {
    auto* first_view = GetTabViewAt(0);
    if (first_view) {
      first_view->SetIsFirst(true);
    }
    auto* second_view = GetTabViewAt(1);
    if (second_view) {
      second_view->SetIsFirst(false);
    }
  }
  if (insert_pos == static_cast<int>(tab_infos_.size()) - 1) {
    auto* last_view = GetTabViewAt(static_cast<int>(tab_infos_.size()) - 1);
    if (last_view) {
      last_view->SetIsLast(true);
    }
    auto* prev_view = GetTabViewAt(static_cast<int>(tab_infos_.size()) - 2);
    if (prev_view) {
      prev_view->SetIsLast(false);
    }
  }

  InvalidateLayout();
}

void AstraSidebarStackChildView::RemoveTab(int index) {
  if (index < 0 || index >= static_cast<int>(tab_infos_.size())) {
    return;
  }

  tab_infos_.erase(tab_infos_.begin() + index);

  // Update active tab index.
  if (active_tab_index_ == index) {
    active_tab_index_ = -1;
  } else if (active_tab_index_ > index) {
    active_tab_index_--;
  }

  // Update indices for tabs after removal point.
  for (size_t i = index; i < tab_infos_.size(); ++i) {
    tab_infos_[i].index_in_stack = static_cast<int>(i);
  }

  // Remove the view.
  if (index < static_cast<int>(children().size())) {
    RemoveChildView(children()[index]);
  }

  // Update first/last indicators.
  if (index == 0 && !tab_infos_.empty()) {
    auto* first_view = GetTabViewAt(0);
    if (first_view) {
      first_view->SetIsFirst(true);
    }
  }
  if (index == static_cast<int>(tab_infos_.size()) && !tab_infos_.empty()) {
    auto* last_view = GetTabViewAt(static_cast<int>(tab_infos_.size()) - 1);
    if (last_view) {
      last_view->SetIsLast(true);
    }
  }

  InvalidateLayout();
}

void AstraSidebarStackChildView::UpdateTab(int index,
                                           const AstraStackTabInfo& tab) {
  if (index < 0 || index >= static_cast<int>(tab_infos_.size())) {
    return;
  }

  bool was_active = tab_infos_[index].is_active;
  tab_infos_[index] = tab;
  tab_infos_[index].index_in_stack = index;

  // Update active tab tracking.
  if (tab.is_active && !was_active) {
    // Clear previous active.
    if (active_tab_index_ >= 0 &&
        active_tab_index_ < static_cast<int>(tab_infos_.size()) &&
        active_tab_index_ != index) {
      tab_infos_[active_tab_index_].is_active = false;
      auto* prev_view = GetTabViewAt(active_tab_index_);
      if (prev_view) {
        prev_view->SetActive(false);
      }
    }
    active_tab_index_ = index;
  } else if (!tab.is_active && was_active && active_tab_index_ == index) {
    active_tab_index_ = -1;
  }

  // Update the view.
  auto* view = GetTabViewAt(index);
  if (view) {
    view->SetTabInfo(tab);
  }
}

// =========================================================================
// Active tab
// =========================================================================

void AstraSidebarStackChildView::SetActiveTab(int index) {
  if (active_tab_index_ == index) {
    return;
  }

  // Clear old active.
  if (active_tab_index_ >= 0 &&
      active_tab_index_ < static_cast<int>(tab_infos_.size())) {
    tab_infos_[active_tab_index_].is_active = false;
    auto* old_view = GetTabViewAt(active_tab_index_);
    if (old_view) {
      old_view->SetActive(false);
    }
  }

  active_tab_index_ = index;

  // Set new active.
  if (active_tab_index_ >= 0 &&
      active_tab_index_ < static_cast<int>(tab_infos_.size())) {
    tab_infos_[active_tab_index_].is_active = true;
    auto* new_view = GetTabViewAt(active_tab_index_);
    if (new_view) {
      new_view->SetActive(true);
    }
  }
}

int AstraSidebarStackChildView::GetActiveTabIndex() const {
  return active_tab_index_;
}

// =========================================================================
// Reordering
// =========================================================================

void AstraSidebarStackChildView::MoveTab(int from_index, int to_index) {
  int count = static_cast<int>(tab_infos_.size());
  if (from_index < 0 || from_index >= count ||
      to_index < 0 || to_index >= count ||
      from_index == to_index) {
    return;
  }

  // Move in data.
  AstraStackTabInfo tab = tab_infos_[from_index];
  tab_infos_.erase(tab_infos_.begin() + from_index);
  tab_infos_.insert(tab_infos_.begin() + to_index, tab);

  // Update indices.
  for (size_t i = 0; i < tab_infos_.size(); ++i) {
    tab_infos_[i].index_in_stack = static_cast<int>(i);
  }

  // Update active index.
  if (active_tab_index_ == from_index) {
    active_tab_index_ = to_index;
  } else if (from_index < active_tab_index_ &&
             to_index >= active_tab_index_) {
    active_tab_index_--;
  } else if (from_index > active_tab_index_ &&
             to_index <= active_tab_index_) {
    active_tab_index_++;
  }

  // Move views.
  // TODO(astra): Use a more efficient view reordering approach.
  //   For now, rebuild all views since it's simpler.
  RebuildTabViews();
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarStackChildView::SetTabHeight(int height) {
  if (tab_height_ == height || height <= 0) {
    return;
  }
  tab_height_ = height;
  // TODO(astra): Propagate to tab item views.
  InvalidateLayout();
}

void AstraSidebarStackChildView::SetShowFavicons(bool show) {
  if (show_favicons_ == show) {
    return;
  }
  show_favicons_ = show;
  UpdateAllTabItemOptions();
  InvalidateLayout();
}

void AstraSidebarStackChildView::SetShowCloseButtons(bool show) {
  if (show_close_buttons_ == show) {
    return;
  }
  show_close_buttons_ = show;
  UpdateAllTabItemOptions();
  InvalidateLayout();
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarStackChildView::SetDragDropEnabled(bool enabled) {
  if (drag_drop_enabled_ == enabled) {
    return;
  }
  drag_drop_enabled_ = enabled;

  // Propagate to tab item views.
  for (auto* child : children()) {
    auto* tab_view = static_cast<AstraSidebarStackTabItemView*>(child);
    if (tab_view) {
      tab_view->SetDraggable(enabled);
    }
  }
}

// =========================================================================
// View access
// =========================================================================

AstraSidebarStackTabItemView* AstraSidebarStackChildView::GetTabViewAt(
    int index) {
  if (index < 0 || index >= static_cast<int>(children().size())) {
    return nullptr;
  }
  return static_cast<AstraSidebarStackTabItemView*>(children()[index]);
}

void AstraSidebarStackChildView::ClearAllTabs() {
  tab_infos_.clear();
  active_tab_index_ = -1;
  RemoveAllChildViews();
}

// =========================================================================
// Internal helpers
// =========================================================================

std::unique_ptr<AstraSidebarStackTabItemView>
AstraSidebarStackChildView::CreateTabItemView(const AstraStackTabInfo& info) {
  auto view = std::make_unique<AstraSidebarStackTabItemView>(info.title);
  view->SetTabInfo(info);
  view->SetShowFavicon(show_favicons_);
  view->SetShowCloseButton(show_close_buttons_);
  view->SetDraggable(drag_drop_enabled_);
  view->SetStackId(stack_id_);
  return view;
}

void AstraSidebarStackChildView::UpdateAllTabItemOptions() {
  for (auto* child : children()) {
    auto* tab_view = static_cast<AstraSidebarStackTabItemView*>(child);
    if (tab_view) {
      tab_view->SetShowFavicon(show_favicons_);
      tab_view->SetShowCloseButton(show_close_buttons_);
    }
  }
}

void AstraSidebarStackChildView::RebuildTabViews() {
  RemoveAllChildViews();

  for (size_t i = 0; i < tab_infos_.size(); ++i) {
    auto tab_view = CreateTabItemView(tab_infos_[i]);
    auto* raw = tab_view.get();
    AddChildView(std::move(tab_view));

    // Set first/last indicators.
    if (i == 0) {
      raw->SetIsFirst(true);
    }
    if (i == tab_infos_.size() - 1) {
      raw->SetIsLast(true);
    }
  }

  InvalidateLayout();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarStackChildView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarStackChildView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // Theme change propagates to child views automatically.
}

void AstraSidebarStackChildView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Stack tabs");
}

}  // namespace astra
