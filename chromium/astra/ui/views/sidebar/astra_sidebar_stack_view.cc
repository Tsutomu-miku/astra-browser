#include "astra/ui/views/sidebar/astra_sidebar_stack_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_tab_stack_service.h"
#include "astra/browser/astra_tab_stack_service_factory.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_child_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_header_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_tab_item_view.h"
#include "base/containers/flat_map.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/web_contents.h"
#include "skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kStacksSectionHeaderHeight = 28;
constexpr int kStacksSectionHorizontalPadding = 12;
constexpr int kStacksSectionVerticalPadding = 8;
constexpr int kStacksSectionHeaderFontSizeDelta = 1;
constexpr int kStacksStackSpacing = 4;
constexpr int kStacksTabItemSpacing = 2;
constexpr int kNewStackButtonHeight = 28;

// Astra color IDs for the tab stacks panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kStacksSectionTitleTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kNewStackButtonTextColorId =
    kColorAstraSidebarSectionHeaderText;

// Default section title.
const char16_t kStacksTitle[] = u"Tab Stacks";

// Default name for newly created stacks.
const char kDefaultNewStackName[] = "New Stack";

// Default color for new stacks.
constexpr SkColor kDefaultNewStackColor = SK_ColorBLUE;

// Comparators for sorting stacks.
// TODO(astra): Consider extracting these to a separate sort utilities file
//   if they become more complex.
bool CompareStacksByName(const AstraStackInfo& a, const AstraStackInfo& b) {
  if (a.is_pinned != b.is_pinned)
    return a.is_pinned;  // Pinned stacks come first.
  return base::i18n::ToLower(a.name) < base::i18n::ToLower(b.name);
}

bool CompareStacksByTabCount(const AstraStackInfo& a, const AstraStackInfo& b) {
  if (a.is_pinned != b.is_pinned)
    return a.is_pinned;
  if (a.tab_count != b.tab_count)
    return a.tab_count > b.tab_count;  // More tabs first.
  return a.name < b.name;  // Stable tiebreaker.
}

bool CompareStacksByLastAccessed(const AstraStackInfo& a,
                                 const AstraStackInfo& b) {
  if (a.is_pinned != b.is_pinned)
    return a.is_pinned;
  if (a.last_accessed != b.last_accessed)
    return a.last_accessed > b.last_accessed;  // More recent first.
  return a.name < b.name;
}

bool CompareStacksByColor(const AstraStackInfo& a, const AstraStackInfo& b) {
  if (a.is_pinned != b.is_pinned)
    return a.is_pinned;
  // Compare by hue (simplified: compare RGB components).
  // TODO(astra): Use proper HSL color space comparison for better results.
  //   Chromium owner: ui::color_utils (ui/gfx/color_utils.h)
  if (SkColorGetR(a.color) != SkColorGetR(b.color))
    return SkColorGetR(a.color) < SkColorGetR(b.color);
  if (SkColorGetG(a.color) != SkColorGetG(b.color))
    return SkColorGetG(a.color) < SkColorGetG(b.color);
  return SkColorGetB(a.color) < SkColorGetB(b.color);
}

bool CompareStacksByOrderIndex(const AstraStackInfo& a,
                               const AstraStackInfo& b) {
  if (a.is_pinned != b.is_pinned)
    return a.is_pinned;
  return a.order_index < b.order_index;
}

}  // namespace

AstraSidebarStackView::AstraSidebarStackView(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  BuildLayout();

  // Observe tab strip model for tab changes.
  if (browser_->tab_strip_model()) {
    browser_->tab_strip_model()->AddObserver(this);
  }

  // Observe tab stack service for stack changes.
  if (AstraTabStackService* service = GetStackService()) {
    stack_service_observation_.Observe(service);
  }

  // Initial model sync.
  UpdateFromModel();
}

AstraSidebarStackView::~AstraSidebarStackView() {
  // Stop observing tab strip model.
  if (browser_ && browser_->tab_strip_model()) {
    browser_->tab_strip_model()->RemoveObserver(this);
  }
}

// =========================================================================
// Stack data management
// =========================================================================

void AstraSidebarStackView::SetStacks(const std::vector<AstraStackInfo>& stacks) {
  stacks_ = stacks;
  SortStacks();
  ClearStackViews();
  PopulateStacks();
  InvalidateLayout();
}

int AstraSidebarStackView::GetStackCount() const {
  return static_cast<int>(stacks_.size());
}

AstraStackInfo AstraSidebarStackView::GetStackAt(int index) const {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return AstraStackInfo();
  }
  return stacks_[index];
}

void AstraSidebarStackView::AddStack(const AstraStackInfo& stack) {
  stacks_.push_back(stack);
  stacks_.back().order_index = static_cast<int>(stacks_.size()) - 1;
  SortStacks();

  // Find the actual index after sorting.
  int new_index = FindStackIndexById(stack.stack_id);
  if (new_index >= 0) {
    // Rebuild since we need to insert at the right position.
    // TODO(astra): Optimize by inserting the view at the correct position
    //   instead of full rebuild.
    ClearStackViews();
    PopulateStacks();
    InvalidateLayout();
  }
}

void AstraSidebarStackView::RemoveStack(int index) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  std::string stack_id = stacks_[index].stack_id;
  stacks_.erase(stacks_.begin() + index);

  // Update selection.
  if (selected_stack_index_ == index) {
    selected_stack_index_ = -1;
  } else if (selected_stack_index_ > index) {
    selected_stack_index_--;
  }

  // Remove from maps.
  header_views_.erase(stack_id);
  child_views_.erase(stack_id);

  // Full rebuild for simplicity.
  // TODO(astra): Optimize by removing just this stack's views.
  ClearStackViews();
  PopulateStacks();
  InvalidateLayout();
}

void AstraSidebarStackView::UpdateStack(int index, const AstraStackInfo& stack) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  stacks_[index] = stack;

  // Update the header view.
  auto it = header_views_.find(stack.stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetName(stack.name);
    it->second->SetColor(stack.color);
    it->second->SetTabCount(stack.tab_count);
    it->second->SetExpanded(stack.is_expanded);
    it->second->SetPinned(stack.is_pinned);
    it->second->SetHasUnread(stack.has_unread);
    it->second->SetShowColorIndicator(show_stack_colors_);
    it->second->SetShowTabCount(show_stack_count_);
    it->second->SetCompact(compact_mode_);
  }

  // Update the child view if expanded.
  auto child_it = child_views_.find(stack.stack_id);
  if (child_it != child_views_.end() && child_it->second) {
    child_it->second->SetVisible(stack.is_expanded);
    // TODO(astra): Update tab items if they changed.
  }

  // If sort order may have changed, refresh.
  if (sort_by_ != AstraStackSortBy::kManual) {
    RefreshSortOrder();
  }

  InvalidateLayout();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarStackView::SetSelectedStack(int index) {
  if (selected_stack_index_ == index) {
    return;
  }

  // Clear old selection.
  if (selected_stack_index_ >= 0 &&
      selected_stack_index_ < static_cast<int>(stacks_.size())) {
    auto it = header_views_.find(stacks_[selected_stack_index_].stack_id);
    if (it != header_views_.end() && it->second) {
      it->second->SetSelected(false);
    }
  }

  selected_stack_index_ = index;

  // Set new selection.
  if (selected_stack_index_ >= 0 &&
      selected_stack_index_ < static_cast<int>(stacks_.size())) {
    auto it = header_views_.find(stacks_[selected_stack_index_].stack_id);
    if (it != header_views_.end() && it->second) {
      it->second->SetSelected(true);
    }
  }
}

int AstraSidebarStackView::GetSelectedStackIndex() const {
  return selected_stack_index_;
}

void AstraSidebarStackView::ClearSelection() {
  SetSelectedStack(-1);
}

// =========================================================================
// Expansion
// =========================================================================

void AstraSidebarStackView::SetExpandedStack(int index, bool expanded) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  if (stacks_[index].is_expanded == expanded) {
    return;
  }

  stacks_[index].is_expanded = expanded;

  auto it = header_views_.find(stacks_[index].stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetExpanded(expanded);
  }

  auto child_it = child_views_.find(stacks_[index].stack_id);
  if (child_it != child_views_.end() && child_it->second) {
    child_it->second->SetVisible(expanded);
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnStackExpandedChanged(stacks_[index].stack_id, expanded);
  }

  InvalidateLayout();
}

bool AstraSidebarStackView::IsStackExpanded(int index) const {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return false;
  }
  return stacks_[index].is_expanded;
}

void AstraSidebarStackView::ToggleStackExpanded(int index) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }
  SetExpandedStack(index, !stacks_[index].is_expanded);
}

void AstraSidebarStackView::ExpandAllStacks() {
  bool changed = false;
  for (size_t i = 0; i < stacks_.size(); ++i) {
    if (!stacks_[i].is_expanded) {
      stacks_[i].is_expanded = true;
      changed = true;
      auto it = header_views_.find(stacks_[i].stack_id);
      if (it != header_views_.end() && it->second) {
        it->second->SetExpanded(true);
      }
      auto child_it = child_views_.find(stacks_[i].stack_id);
      if (child_it != child_views_.end() && child_it->second) {
        child_it->second->SetVisible(true);
      }
    }
  }
  if (changed) {
    InvalidateLayout();
  }
}

void AstraSidebarStackView::CollapseAllStacks() {
  bool changed = false;
  for (size_t i = 0; i < stacks_.size(); ++i) {
    if (stacks_[i].is_expanded) {
      stacks_[i].is_expanded = false;
      changed = true;
      auto it = header_views_.find(stacks_[i].stack_id);
      if (it != header_views_.end() && it->second) {
        it->second->SetExpanded(false);
      }
      auto child_it = child_views_.find(stacks_[i].stack_id);
      if (child_it != child_views_.end() && child_it->second) {
        child_it->second->SetVisible(false);
      }
    }
  }
  if (changed) {
    InvalidateLayout();
  }
}

int AstraSidebarStackView::GetExpandedStackCount() const {
  int count = 0;
  for (const auto& stack : stacks_) {
    if (stack.is_expanded) {
      count++;
    }
  }
  return count;
}

// =========================================================================
// Reordering
// =========================================================================

void AstraSidebarStackView::MoveStack(int from_index, int to_index) {
  int count = static_cast<int>(stacks_.size());
  if (from_index < 0 || from_index >= count ||
      to_index < 0 || to_index >= count ||
      from_index == to_index) {
    return;
  }

  // Move in the data vector.
  AstraStackInfo stack = stacks_[from_index];
  stacks_.erase(stacks_.begin() + from_index);
  stacks_.insert(stacks_.begin() + to_index, stack);

  // Update order indices for manual sort.
  for (size_t i = 0; i < stacks_.size(); ++i) {
    stacks_[i].order_index = static_cast<int>(i);
  }

  // Update selection index.
  if (selected_stack_index_ == from_index) {
    selected_stack_index_ = to_index;
  } else if (from_index < selected_stack_index_ &&
             to_index >= selected_stack_index_) {
    selected_stack_index_--;
  } else if (from_index > selected_stack_index_ &&
             to_index <= selected_stack_index_) {
    selected_stack_index_++;
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnStackReordered(from_index, to_index);
  }

  // Rebuild views.
  // TODO(astra): Animate the reordering instead of full rebuild.
  ClearStackViews();
  PopulateStacks();
  InvalidateLayout();
}

// =========================================================================
// Stack properties
// =========================================================================

void AstraSidebarStackView::SetStackColor(int index, SkColor color) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  if (stacks_[index].color == color) {
    return;
  }

  stacks_[index].color = color;

  auto it = header_views_.find(stacks_[index].stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetColor(color);
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnStackColorChanged(stacks_[index].stack_id, color);
  }

  // Refresh sort if sorted by color.
  if (sort_by_ == AstraStackSortBy::kColor) {
    RefreshSortOrder();
  }
}

SkColor AstraSidebarStackView::GetStackColor(int index) const {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return SK_ColorGRAY;
  }
  return stacks_[index].color;
}

void AstraSidebarStackView::RenameStack(int index,
                                        const std::u16string& new_name) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  if (stacks_[index].name == new_name) {
    return;
  }

  stacks_[index].name = new_name;

  auto it = header_views_.find(stacks_[index].stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetName(new_name);
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnStackRenamed(stacks_[index].stack_id, new_name);
  }

  // Refresh sort if sorted by name.
  if (sort_by_ == AstraStackSortBy::kName) {
    RefreshSortOrder();
  }
}

std::u16string AstraSidebarStackView::GetStackName(int index) const {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return std::u16string();
  }
  return stacks_[index].name;
}

int AstraSidebarStackView::GetTabCountInStack(int index) const {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return 0;
  }
  return stacks_[index].tab_count;
}

int AstraSidebarStackView::GetTotalTabCount() const {
  int total = 0;
  for (const auto& stack : stacks_) {
    total += stack.tab_count;
  }
  return total;
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarStackView::SetShowStackCount(bool show) {
  if (show_stack_count_ == show) {
    return;
  }
  show_stack_count_ = show;

  for (auto& [stack_id, header_view] : header_views_) {
    if (header_view) {
      header_view->SetShowTabCount(show);
    }
  }

  InvalidateLayout();
}

bool AstraSidebarStackView::GetShowStackCount() const {
  return show_stack_count_;
}

void AstraSidebarStackView::SetShowAddStackButton(bool show) {
  if (show_add_stack_button_ == show) {
    return;
  }
  show_add_stack_button_ = show;
  if (new_stack_button_) {
    new_stack_button_->SetVisible(show);
  }
  InvalidateLayout();
}

bool AstraSidebarStackView::GetShowAddStackButton() const {
  return show_add_stack_button_;
}

void AstraSidebarStackView::SetShowCollapseAllButton(bool show) {
  if (show_collapse_all_button_ == show) {
    return;
  }
  show_collapse_all_button_ = show;
  if (collapse_all_button_) {
    collapse_all_button_->SetVisible(show);
  }
  InvalidateLayout();
}

bool AstraSidebarStackView::GetShowCollapseAllButton() const {
  return show_collapse_all_button_;
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarStackView::SetSortStacksBy(AstraStackSortBy sort_by) {
  if (sort_by_ == sort_by) {
    return;
  }
  sort_by_ = sort_by;
  SortStacks();
  ClearStackViews();
  PopulateStacks();
  InvalidateLayout();
}

AstraStackSortBy AstraSidebarStackView::GetSortStacksBy() const {
  return sort_by_;
}

void AstraSidebarStackView::SortStacks() {
  switch (sort_by_) {
    case AstraStackSortBy::kManual:
      base::ranges::sort(stacks_, CompareStacksByOrderIndex);
      break;
    case AstraStackSortBy::kName:
      base::ranges::sort(stacks_, CompareStacksByName);
      break;
    case AstraStackSortBy::kTabCount:
      base::ranges::sort(stacks_, CompareStacksByTabCount);
      break;
    case AstraStackSortBy::kLastAccessed:
      base::ranges::sort(stacks_, CompareStacksByLastAccessed);
      break;
    case AstraStackSortBy::kColor:
      base::ranges::sort(stacks_, CompareStacksByColor);
      break;
  }
}

void AstraSidebarStackView::RefreshSortOrder() {
  // Save current selection ID.
  std::string selected_id;
  if (selected_stack_index_ >= 0 &&
      selected_stack_index_ < static_cast<int>(stacks_.size())) {
    selected_id = stacks_[selected_stack_index_].stack_id;
  }

  SortStacks();

  // Restore selection index.
  selected_stack_index_ = FindStackIndexById(selected_id);

  ClearStackViews();
  PopulateStacks();
  InvalidateLayout();
}

// =========================================================================
// Stack operations
// =========================================================================

int AstraSidebarStackView::NewStack(const std::u16string& name, SkColor color) {
  // Generate a unique stack ID.
  static int next_id = 0;
  std::string stack_id = "stack_" + base::NumberToString(next_id++);

  AstraStackInfo info;
  info.stack_id = stack_id;
  info.name = name;
  info.color = color;
  info.tab_count = 0;
  info.is_expanded = true;
  info.is_pinned = false;
  info.order_index = static_cast<int>(stacks_.size());
  info.created_time = base::Time::Now();
  info.last_accessed = base::Time::Now();
  info.has_unread = false;

  AddStack(info);

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnNewStackRequested();
  }

  return FindStackIndexById(stack_id);
}

void AstraSidebarStackView::DeleteStack(int index) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  std::string stack_id = stacks_[index].stack_id;

  // Notify delegate before removal.
  if (stack_delegate_) {
    stack_delegate_->OnDeleteStackRequested(stack_id);
  }

  RemoveStack(index);
}

void AstraSidebarStackView::CloseAllTabsInStack(int index) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return;
  }

  // TODO(astra): Implement actual tab closing when wired to TabStripModel.
  //   Chromium owner: TabStripModel::CloseWebContentsAt()
  //   For now, just update the count.
  stacks_[index].tab_count = 0;

  auto it = header_views_.find(stacks_[index].stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetTabCount(0);
  }

  auto child_it = child_views_.find(stacks_[index].stack_id);
  if (child_it != child_views_.end() && child_it->second) {
    child_it->second->ClearAllTabs();
  }
}

void AstraSidebarStackView::MoveTabToStack(int from_stack, int from_tab,
                                           int to_stack, int to_tab) {
  if (from_stack < 0 || from_stack >= static_cast<int>(stacks_.size()) ||
      to_stack < 0 || to_stack >= static_cast<int>(stacks_.size())) {
    return;
  }

  // Validate tab indices.
  if (from_tab < 0 || from_tab >= stacks_[from_stack].tab_count) {
    return;
  }
  if (to_tab < -1 || to_tab > stacks_[to_stack].tab_count) {
    return;
  }

  // Update counts.
  stacks_[from_stack].tab_count--;
  stacks_[to_stack].tab_count++;

  // Update header views.
  auto from_it = header_views_.find(stacks_[from_stack].stack_id);
  if (from_it != header_views_.end() && from_it->second) {
    from_it->second->SetTabCount(stacks_[from_stack].tab_count);
  }

  auto to_it = header_views_.find(stacks_[to_stack].stack_id);
  if (to_it != header_views_.end() && to_it->second) {
    to_it->second->SetTabCount(stacks_[to_stack].tab_count);
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnMoveTabToStackRequested(
        stacks_[from_stack].stack_id, from_tab,
        stacks_[to_stack].stack_id);
  }
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarStackView::SetDragDropEnabled(bool enabled) {
  if (drag_drop_enabled_ == enabled) {
    return;
  }
  drag_drop_enabled_ = enabled;

  // TODO(astra): Propagate to child views when drag-drop is fully implemented.
  //   For now, just update the state.
}

bool AstraSidebarStackView::GetDragDropEnabled() const {
  return drag_drop_enabled_;
}

// =========================================================================
// Visual options
// =========================================================================

void AstraSidebarStackView::SetShowStackColors(bool show) {
  if (show_stack_colors_ == show) {
    return;
  }
  show_stack_colors_ = show;

  for (auto& [stack_id, header_view] : header_views_) {
    if (header_view) {
      header_view->SetShowColorIndicator(show);
    }
  }

  InvalidateLayout();
}

bool AstraSidebarStackView::GetShowStackColors() const {
  return show_stack_colors_;
}

void AstraSidebarStackView::SetCompactMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;

  for (auto& [stack_id, header_view] : header_views_) {
    if (header_view) {
      header_view->SetCompact(compact);
    }
  }

  // TODO(astra): Also update tab item views for compact mode.

  InvalidateLayout();
}

bool AstraSidebarStackView::GetCompactMode() const {
  return compact_mode_;
}

void AstraSidebarStackView::SetStackHeight(int height_px) {
  if (stack_height_px_ == height_px || height_px <= 0) {
    return;
  }
  stack_height_px_ = height_px;
  // TODO(astra): Propagate to header views and trigger relayout.
  InvalidateLayout();
}

int AstraSidebarStackView::GetStackHeight() const {
  return stack_height_px_;
}

// =========================================================================
// View accessors
// =========================================================================

views::View* AstraSidebarStackView::GetHeaderView() {
  return section_header_;
}

AstraSidebarStackHeaderView* AstraSidebarStackView::GetStackViewAt(
    int index) {
  if (index < 0 || index >= static_cast<int>(stacks_.size())) {
    return nullptr;
  }
  auto it = header_views_.find(stacks_[index].stack_id);
  if (it != header_views_.end()) {
    return it->second;
  }
  return nullptr;
}

AstraSidebarStackTabItemView* AstraSidebarStackView::GetStackTabItemAt(
    int stack_index, int tab_index) {
  if (stack_index < 0 || stack_index >= static_cast<int>(stacks_.size())) {
    return nullptr;
  }
  auto child_it = child_views_.find(stacks_[stack_index].stack_id);
  if (child_it == child_views_.end() || !child_it->second) {
    return nullptr;
  }
  return child_it->second->GetTabViewAt(tab_index);
}

// =========================================================================
// Layout and building
// =========================================================================

void AstraSidebarStackView::BuildLayout() {
  // Vertical box layout: section header + stacks container + buttons.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Section header row (title + buttons).
  section_header_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout = section_header_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  section_header_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kStacksSectionVerticalPadding, kStacksSectionHorizontalPadding)));

  // Section title label.
  section_title_ = section_header_->AddChildView(
      std::make_unique<views::Label>(kStacksTitle));
  section_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_title_->SetAutoColorReadabilityEnabled(false);
  section_title_->SetFontList(
      section_title_->font_list().DeriveWithSizeDelta(
          kStacksSectionHeaderFontSizeDelta));
  header_layout->SetFlexForView(section_title_, 1);

  // Collapse all button.
  collapse_all_button_ = section_header_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarStackView::OnCollapseAllButtonClicked,
              base::Unretained(this)),
          u"Collapse all"));
  collapse_all_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  collapse_all_button_->SetVisible(show_collapse_all_button_);
  collapse_all_button_->SetTooltipText(u"Collapse all stacks");

  // Stacks container — holds all stack headers and their child views.
  stacks_container_ = AddChildView(std::make_unique<views::View>());
  auto* stacks_layout = stacks_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kStacksSectionHorizontalPadding / 2),
          kStacksStackSpacing));
  stacks_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  layout->SetFlexForView(stacks_container_, 1);

  // "New stack" button at the bottom.
  new_stack_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraSidebarStackView::OnNewStackButtonClicked,
                          base::Unretained(this)),
      u"+ New stack"));
  new_stack_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  new_stack_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      4, kStacksSectionHorizontalPadding)));
  new_stack_button_->SetFontList(
      new_stack_button_->font_list().DeriveWithSizeDelta(-1));
  new_stack_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  new_stack_button_->SetTooltipText(u"Create a new tab stack");
  new_stack_button_->SetVisible(show_add_stack_button_);
}

void AstraSidebarStackView::PopulateStacks() {
  if (!stacks_container_) {
    return;
  }

  for (size_t i = 0; i < stacks_.size(); ++i) {
    const auto& stack = stacks_[i];

    // Create and add the stack header.
    auto header = CreateStackHeader(stack);
    AstraSidebarStackHeaderView* header_raw = header.get();
    stacks_container_->AddChildView(std::move(header));
    header_views_[stack.stack_id] = header_raw;

    // Create the child view (tab items container).
    auto child_view = CreateStackChildView(stack);
    AstraSidebarStackChildView* child_raw = child_view.get();
    stacks_container_->AddChildView(std::move(child_view));
    child_views_[stack.stack_id] = child_raw;
    child_raw->SetVisible(stack.is_expanded);
  }
}

void AstraSidebarStackView::ClearStackViews() {
  if (stacks_container_) {
    stacks_container_->RemoveAllChildViews();
  }
  header_views_.clear();
  child_views_.clear();
}

std::unique_ptr<AstraSidebarStackHeaderView>
AstraSidebarStackView::CreateStackHeader(const AstraStackInfo& info) {
  auto header = std::make_unique<AstraSidebarStackHeaderView>(info.name);

  header->SetStackInfo(info);
  header->SetShowTabCount(show_stack_count_);
  header->SetShowColorIndicator(show_stack_colors_);
  header->SetCompact(compact_mode_);
  header->set_delegate(this);

  // Set selected state.
  int index = FindStackIndexById(info.stack_id);
  if (index >= 0 && index == selected_stack_index_) {
    header->SetSelected(true);
  }

  return header;
}

std::unique_ptr<AstraSidebarStackChildView>
AstraSidebarStackView::CreateStackChildView(const AstraStackInfo& info) {
  auto child_view = std::make_unique<AstraSidebarStackChildView>();
  child_view->set_stack_id(info.stack_id);
  child_view->SetShowFavicons(true);
  child_view->SetShowCloseButtons(true);
  child_view->SetDragDropEnabled(drag_drop_enabled_);
  return child_view;
}

std::unique_ptr<AstraSidebarStackTabItemView>
AstraSidebarStackView::CreateStackTabItem(
    content::WebContents* web_contents,
    const std::string& stack_id) {
  // Determine the tab title.
  std::u16string title;
  if (web_contents && !web_contents->GetTitle().empty()) {
    title = web_contents->GetTitle();
  } else {
    title = u"Untitled";
  }

  auto item = std::make_unique<AstraSidebarStackTabItemView>(title);
  item->set_web_contents(web_contents);
  item->SetStackId(stack_id);
  item->set_delegate(this);
  item->SetDraggable(drag_drop_enabled_);

  // Mark as active if this is the currently selected tab.
  if (browser_ && browser_->tab_strip_model()) {
    content::WebContents* active_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    if (web_contents == active_contents) {
      item->SetActive(true);
    }
  }

  // TODO(astra): Set audio state based on WebContents audio state.
  //   Chromium owner: content::WebContents::IsCurrentlyAudible()
  //   For now, audio state is not projected.

  return item;
}

void AstraSidebarStackView::OnNewStackButtonClicked() {
  NewStack(base::UTF8ToUTF16(kDefaultNewStackName), kDefaultNewStackColor);
}

void AstraSidebarStackView::OnCollapseAllButtonClicked() {
  // Toggle: if all are collapsed, expand all; otherwise collapse all.
  if (GetExpandedStackCount() == 0) {
    ExpandAllStacks();
  } else {
    CollapseAllStacks();
  }
}

std::string AstraSidebarStackView::CreateNewStack() {
  // Legacy method — delegates to NewStack.
  // TODO(astra): Remove this method once all callers use NewStack().
  int index = NewStack(base::UTF8ToUTF16(kDefaultNewStackName),
                       kDefaultNewStackColor);
  if (index >= 0 && index < static_cast<int>(stacks_.size())) {
    return stacks_[index].stack_id;
  }
  return std::string();
}

void AstraSidebarStackView::ActivateFirstTabInStack(
    const std::string& stack_id) {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }

  AstraTabStackService* service = GetStackService();
  if (!service) {
    return;
  }

  std::vector<content::WebContents*> tabs = service->GetTabsInStack(stack_id);
  if (tabs.empty()) {
    return;
  }

  // Find the first tab in the stack and activate it.
  TabStripModel* tab_strip = browser_->tab_strip_model();
  for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
    if (tab_strip->GetWebContentsAt(i) == tabs[0]) {
      tab_strip->ActivateTabAt(i);
      break;
    }
  }
}

AstraTabStackService* AstraSidebarStackView::GetStackService() {
  if (!browser_) {
    return nullptr;
  }
  return AstraTabStackServiceFactory::GetForProfile(browser_->profile());
}

int AstraSidebarStackView::FindStackIndexById(const std::string& stack_id) const {
  for (size_t i = 0; i < stacks_.size(); ++i) {
    if (stacks_[i].stack_id == stack_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// =========================================================================
// AstraTabStackServiceObserver
// =========================================================================

void AstraSidebarStackView::OnStackCreated(const AstraTabStack& /*stack*/) {
  // TODO(astra): Incremental update — insert the new stack header at
  //   the right position instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarStackView::OnStackDeleted(
    const AstraTabStackId& /*stack_id*/) {
  // TODO(astra): Incremental update — remove the stack header and its
  //   tab items instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarStackView::OnStackRenamed(
    const AstraTabStackId& stack_id,
    const std::string& new_name) {
  // Incremental update: find the header view and update its title.
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetName(base::UTF8ToUTF16(new_name));
  } else {
    UpdateFromModel();
  }

  // Also update the data model.
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    stacks_[index].name = base::UTF8ToUTF16(new_name);
  }
}

void AstraSidebarStackView::OnStacksReordered() {
  // Reordering requires full rebuild because the visual order changes.
  // TODO(astra): Animate the reordering instead of instant rebuild.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabAddedToStack(
    content::WebContents* /*web_contents*/,
    const AstraTabStackId& stack_id) {
  // Update tab count on the header and potentially add the tab item.
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    AstraTabStackService* service = GetStackService();
    if (service) {
      const AstraTabStack* stack = service->GetStack(stack_id);
      if (stack) {
        it->second->SetTabCount(stack->tab_count);
      }
    }
  }

  // Update data model.
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    stacks_[index].tab_count++;
  }

  // TODO(astra): Incrementally add the tab item view if the stack
  //   is expanded, instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabRemovedFromStack(
    content::WebContents* /*web_contents*/,
    const AstraTabStackId& stack_id) {
  // Update tab count on the header.
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    AstraTabStackService* service = GetStackService();
    if (service) {
      const AstraTabStack* stack = service->GetStack(stack_id);
      if (stack) {
        it->second->SetTabCount(stack->tab_count);
      }
    }
  }

  // Update data model.
  int index = FindStackIndexById(stack_id);
  if (index >= 0 && stacks_[index].tab_count > 0) {
    stacks_[index].tab_count--;
  }

  // TODO(astra): Incrementally remove the tab item view if the stack
  //   is expanded, instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarStackView::OnStackCollapsed(
    const AstraTabStackId& stack_id) {
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetExpanded(false);
  }

  auto child_it = child_views_.find(stack_id);
  if (child_it != child_views_.end() && child_it->second) {
    child_it->second->SetVisible(false);
  }

  // Update data model.
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    stacks_[index].is_expanded = false;
  }

  InvalidateLayout();
}

void AstraSidebarStackView::OnStackExpanded(
    const AstraTabStackId& stack_id) {
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetExpanded(true);
  }

  auto child_it = child_views_.find(stack_id);
  if (child_it != child_views_.end() && child_it->second) {
    child_it->second->SetVisible(true);
  }

  // Update data model.
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    stacks_[index].is_expanded = true;
  }

  InvalidateLayout();
}

// =========================================================================
// TabStripModelObserver
// =========================================================================

void AstraSidebarStackView::OnTabStripModelChanged(
    TabStripModel* /*tab_strip_model*/,
    const TabStripModelChange& /*change*/,
    const TabStripSelectionChange& /*selection*/) {
  // Generic change handler — catches any change not handled by more
  // specific methods. Full rebuild is the safe default.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabInsertedAt(
    TabStripModel* /*tab_strip_model*/,
    int /*index*/,
    bool /*foreground*/) {
  // A new tab was inserted. It might belong to a stack.
  // TODO(astra): Incremental update — check if the new tab is in a stack
  //   and insert the tab item view accordingly.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabRemovedAt(
    TabStripModel* /*tab_strip_model*/,
    int /*index*/,
    bool /*was_active*/) {
  // A tab was removed. It might have been in a stack.
  // TODO(astra): Incremental update — remove the tab item from its stack.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabMoved(TabStripModel* /*tab_strip_model*/,
                                        int /*from_index*/,
                                        int /*to_index*/) {
  // A tab was moved. It may have moved into, out of, or within a stack.
  // Since the stack tab order follows TabStripModel order, we need to
  // update the presentation.
  // TODO(astra): Incremental update — reorder tab items within the stack.
  UpdateFromModel();
}

void AstraSidebarStackView::OnActiveTabChanged(
    TabStripModel* /*tab_strip_model*/,
    int /*old_index*/,
    int /*new_index*/,
    const TabStripSelectionChange& /*selection*/) {
  // The active tab changed. Update the active highlight.
  // TODO(astra): Incrementally update the active highlight by finding
  //   the old and new tab items and toggling their active state.
  UpdateFromModel();
}

void AstraSidebarStackView::OnTabChanged(TabStripModel* /*tab_strip_model*/,
                                          int /*index*/,
                                          TabChangeType /*change_type*/) {
  // A tab's display state changed (title, favicon, etc.).
  // TODO(astra): Incrementally update just that tab item's title/icon.
  UpdateFromModel();
}

// =========================================================================
// AstraSidebarStackHeaderDelegate
// =========================================================================

void AstraSidebarStackView::OnStackToggleExpanded(
    const std::string& stack_id) {
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    ToggleStackExpanded(index);
  }
}

void AstraSidebarStackView::OnStackHeaderClicked(
    const std::string& stack_id) {
  int index = FindStackIndexById(stack_id);
  if (index >= 0) {
    SetSelectedStack(index);
  }

  // Notify delegate.
  if (stack_delegate_) {
    stack_delegate_->OnStackClicked(stack_id);
  }

  // Clicking the stack header also activates the first tab in the stack.
  //
  // TODO(astra): Finalize the interaction model for stack header clicks.
  //   Options:
  //     - Vivaldi: click = toggle expand, double-click = activate tab
  //     - Arc: click = activate tab + toggle expand
  //     - Tree-style: arrow = expand/collapse, title = activate
  //
  //   For now, we toggle expand and activate the first tab.
  //
  // Chromium owner: TabGroupHeader
  //   (chrome/browser/ui/views/tabs/tab_group_header.h)

  // Activate the first tab in the stack (if expanded).
  AstraTabStackService* service = GetStackService();
  if (service && !service->IsStackCollapsed(stack_id)) {
    ActivateFirstTabInStack(stack_id);
  }
}

void AstraSidebarStackView::OnStackMenuClicked(
    const std::string& /*stack_id*/,
    const gfx::Point& /*anchor_point*/) {
  // TODO(astra): Show a context menu with stack actions:
  //   - Rename stack
  //   - Change color
  //   - Delete stack
  //   - Collapse/expand all
  //   - etc.
  //
  // This requires building a menu with views::MenuRunner or
  // views::MenuItemView.
  //
  // Chromium owner: views::MenuRunner (ui/views/controls/menu/menu_runner.h)
  // Chromium owner: TabGroupContextMenu
  //   (chrome/browser/ui/views/tabs/tab_group_context_menu.h)
}

// =========================================================================
// AstraSidebarStackTabItemDelegate
// =========================================================================

void AstraSidebarStackView::OnStackTabClicked(
    content::WebContents* web_contents) {
  if (!browser_ || !browser_->tab_strip_model() || !web_contents) {
    return;
  }

  // Find the tab index and activate it.
  TabStripModel* tab_strip = browser_->tab_strip_model();
  for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
    if (tab_strip->GetWebContentsAt(i) == web_contents) {
      tab_strip->ActivateTabAt(i);
      break;
    }
  }
}

void AstraSidebarStackView::OnStackTabClosed(
    content::WebContents* web_contents) {
  if (!browser_ || !browser_->tab_strip_model() || !web_contents) {
    return;
  }

  // Find the tab index and close it.
  TabStripModel* tab_strip = browser_->tab_strip_model();
  for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
    if (tab_strip->GetWebContentsAt(i) == web_contents) {
      tab_strip->CloseWebContentsAt(i, TabStripModel::CLOSE_NONE);
      break;
    }
  }
}

void AstraSidebarStackView::OnStackTabDragStarted(
    content::WebContents* /*web_contents*/,
    const gfx::Point& /*mouse_location*/) {
  // TODO(astra): Implement drag-and-drop for stack tab items.
  //   Dragging a tab out of a stack should remove it from the stack.
  //   Dragging a tab onto a stack should add it to the stack.
  //   Dragging within a stack should reorder tabs.
  //
  // This requires integration with the sidebar drag controller.
  //
  // Chromium owner: TabDragController
  //   (chrome/browser/ui/views/tabs/tab_drag_controller.h)
  // Astra drag types: AstraSidebarDragData, AstraSidebarDropResult
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarStackView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarStackView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Tab stacks");
}

void AstraSidebarStackView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (section_title_) {
    section_title_->SetEnabledColor(
        color_provider->GetColor(kStacksSectionTitleTextColorId));
  }

  if (new_stack_button_) {
    // TODO(astra): Use proper LabelButton text color API.
    //   Chromium owner: views::LabelButton::SetTextColor()
    //   (ui/views/controls/button/label_button.h)
    // For now, rely on the default button text color from the theme.
  }

  if (collapse_all_button_) {
    // Same as above — relies on default theme colors.
  }
}

void AstraSidebarStackView::SetTitle(const std::u16string& title) {
  if (section_title_) {
    section_title_->SetText(title);
  }
}

void AstraSidebarStackView::UpdateFromModel() {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }

  ClearStackViews();

  // Rebuild stacks_ data from the service.
  stacks_.clear();

  AstraTabStackService* service = GetStackService();
  if (service) {
    std::vector<AstraTabStack> service_stacks = service->GetAllStacks();
    for (const auto& service_stack : service_stacks) {
      AstraStackInfo info;
      info.stack_id = service_stack.id;
      info.name = base::UTF8ToUTF16(service_stack.name);
      // Parse color from hex string.
      // TODO(astra): Use proper SkColor parsing utility.
      info.color = AstraSidebarStackHeaderView::ParseHexColor(
          service_stack.color);
      info.tab_count = service_stack.tab_count;
      info.is_expanded = !service_stack.collapsed;
      info.is_pinned = false;  // TODO(astra): Add pinned to AstraTabStack.
      info.order_index = service_stack.order_index;
      info.created_time = service_stack.created_time;
      info.last_accessed = base::Time::Now();  // TODO(astra): Track last access.
      info.has_unread = false;
      stacks_.push_back(info);
    }
  }

  SortStacks();
  PopulateStacks();
  InvalidateLayout();
}

}  // namespace astra
