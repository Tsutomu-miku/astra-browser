#include "astra/ui/views/sidebar/astra_sidebar_tab_groups_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_tab_group_header_view.h"
#include "astra/ui/views/sidebar/astra_tab_group_tab_item_view.h"
#include "base/containers/flat_map.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kTabGroupsSectionHeaderHeight = 28;
constexpr int kTabGroupsSectionHorizontalPadding = 12;
constexpr int kTabGroupsSectionVerticalPadding = 8;
constexpr int kTabGroupsSectionHeaderFontSizeDelta = 1;
constexpr int kTabGroupsGroupSpacing = 4;
constexpr int kTabGroupsAddButtonHeight = 28;

// Astra color ID for the tab groups panel.
constexpr ui::ColorId kTabGroupsSectionTitleTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kTabGroupsAddButtonTextColorId =
    kColorAstraSidebarItemText;

// Default section title.
const char16_t kTabGroupsTitle[] = u"Tab Groups";

// Default "add group" button text.
const char16_t kAddGroupText[] = u"+ New group";

// Comparator for sorting groups by name (ascending).
bool CompareGroupsByName(const AstraTabGroupInfo& a,
                         const AstraTabGroupInfo& b) {
  return base::i18n::CompareString16WithCollator(a.name, b.name) < 0;
}

// Comparator for sorting groups by tab count (descending).
bool CompareGroupsByTabCount(const AstraTabGroupInfo& a,
                             const AstraTabGroupInfo& b) {
  if (a.tab_count != b.tab_count) {
    return a.tab_count > b.tab_count;
  }
  return CompareGroupsByName(a, b);
}

// Comparator for sorting groups by last accessed (descending).
bool CompareGroupsByLastAccessed(const AstraTabGroupInfo& a,
                                 const AstraTabGroupInfo& b) {
  if (a.last_accessed != b.last_accessed) {
    return a.last_accessed > b.last_accessed;
  }
  return CompareGroupsByName(a, b);
}

// Comparator for sorting groups by color ID (ascending).
bool CompareGroupsByColor(const AstraTabGroupInfo& a,
                          const AstraTabGroupInfo& b) {
  if (a.color_id != b.color_id) {
    return a.color_id < b.color_id;
  }
  return CompareGroupsByName(a, b);
}

// Comparator for sorting groups by manual order (ascending).
bool CompareGroupsByOrderIndex(const AstraTabGroupInfo& a,
                               const AstraTabGroupInfo& b) {
  if (a.order_index != b.order_index) {
    return a.order_index < b.order_index;
  }
  return CompareGroupsByName(a, b);
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSidebarTabGroupsView::AstraSidebarTabGroupsView() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  BuildLayout();
}

AstraSidebarTabGroupsView::AstraSidebarTabGroupsView(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  BuildLayout();

  // Initial model sync.
  UpdateFromModel();
}

AstraSidebarTabGroupsView::~AstraSidebarTabGroupsView() = default;

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarTabGroupsView::BuildLayout() {
  // Vertical box layout: section title + groups container + add button.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Section title label.
  section_title_ = AddChildView(std::make_unique<views::Label>(kTabGroupsTitle));
  section_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_title_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kTabGroupsSectionVerticalPadding, kTabGroupsSectionHorizontalPadding)));
  section_title_->SetFontList(
      section_title_->font_list().DeriveWithSizeDelta(
          kTabGroupsSectionHeaderFontSizeDelta));
  section_title_->SetAutoColorReadabilityEnabled(false);

  // Groups container — holds all group headers and their tab items.
  groups_container_ = AddChildView(std::make_unique<views::View>());
  auto* groups_layout = groups_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kTabGroupsSectionHorizontalPadding / 2),
          kTabGroupsGroupSpacing));
  groups_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  layout->SetFlexForView(groups_container_, 1);

  // "Add group" button (hidden by default).
  add_group_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraSidebarTabGroupsView::HandleAddGroupClicked,
                          base::Unretained(this)),
      kAddGroupText));
  add_group_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  add_group_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      4, kTabGroupsSectionHorizontalPadding)));
  add_group_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  add_group_button_->SetVisible(false);
}

// =========================================================================
// Title
// =========================================================================

void AstraSidebarTabGroupsView::SetTitle(const std::u16string& title) {
  section_title_->SetText(title);
}

// =========================================================================
// Group management
// =========================================================================

void AstraSidebarTabGroupsView::SetTabGroups(
    const std::vector<AstraTabGroupInfo>& groups) {
  // Store new groups data.
  groups_.clear();
  groups_.reserve(groups.size());
  for (const auto& info : groups) {
    GroupEntry entry;
    entry.info = info;
    groups_.push_back(std::move(entry));
  }

  ApplySortOrder();
  RebuildAllViews();
}

int AstraSidebarTabGroupsView::GetGroupCount() const {
  return static_cast<int>(groups_.size());
}

AstraTabGroupInfo AstraSidebarTabGroupsView::GetGroupAt(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].info;
}

void AstraSidebarTabGroupsView::AddGroup(const AstraTabGroupInfo& group) {
  GroupEntry entry;
  entry.info = group;
  groups_.push_back(std::move(entry));
  ApplySortOrder();
  RebuildAllViews();
}

void AstraSidebarTabGroupsView::RemoveGroup(int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  // Adjust selection if needed.
  if (selected_group_index_ == index) {
    selected_group_index_ = -1;
  } else if (selected_group_index_ > index) {
    selected_group_index_--;
  }

  groups_.erase(groups_.begin() + index);
  RebuildAllViews();
}

void AstraSidebarTabGroupsView::UpdateGroup(int index,
                                            const AstraTabGroupInfo& group) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  groups_[index].info = group;
  ApplySortOrder();
  // TODO(astra): Optimize — only update the affected header + tabs
  // instead of rebuilding everything.
  RebuildAllViews();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarTabGroupsView::SetSelectedGroup(int index) {
  if (index < -1 || index >= static_cast<int>(groups_.size())) {
    return;
  }
  if (selected_group_index_ == index) {
    return;
  }

  // Clear old selection.
  if (selected_group_index_ >= 0 &&
      selected_group_index_ < static_cast<int>(groups_.size()) &&
      groups_[selected_group_index_].header) {
    groups_[selected_group_index_].header->SetSelected(false);
  }

  selected_group_index_ = index;

  // Set new selection.
  if (selected_group_index_ >= 0 &&
      groups_[selected_group_index_].header) {
    groups_[selected_group_index_].header->SetSelected(true);
  }
}

int AstraSidebarTabGroupsView::GetSelectedGroupIndex() const {
  return selected_group_index_;
}

void AstraSidebarTabGroupsView::ClearSelection() {
  SetSelectedGroup(-1);
}

// =========================================================================
// Expansion
// =========================================================================

void AstraSidebarTabGroupsView::SetGroupExpanded(int index, bool expanded) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  if (groups_[index].info.is_expanded == expanded) {
    return;
  }

  groups_[index].info.is_expanded = expanded;
  if (groups_[index].header) {
    groups_[index].header->SetExpanded(expanded);
  }

  // Rebuild to show/hide tab items.
  // TODO(astra): Optimize by toggling visibility of the tab container
  // instead of rebuilding all views.
  RebuildAllViews();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnGroupExpandedChanged(groups_[index].info.group_id, expanded);
  }
}

bool AstraSidebarTabGroupsView::IsGroupExpanded(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].info.is_expanded;
}

void AstraSidebarTabGroupsView::ToggleGroupExpanded(int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  SetGroupExpanded(index, !groups_[index].info.is_expanded);
}

void AstraSidebarTabGroupsView::ExpandAllGroups() {
  for (size_t i = 0; i < groups_.size(); ++i) {
    groups_[i].info.is_expanded = true;
  }
  RebuildAllViews();
}

void AstraSidebarTabGroupsView::CollapseAllGroups() {
  for (size_t i = 0; i < groups_.size(); ++i) {
    groups_[i].info.is_expanded = false;
  }
  RebuildAllViews();
}

int AstraSidebarTabGroupsView::GetExpandedGroupCount() const {
  int count = 0;
  for (const auto& entry : groups_) {
    if (entry.info.is_expanded) {
      ++count;
    }
  }
  return count;
}

// =========================================================================
// Reordering
// =========================================================================

void AstraSidebarTabGroupsView::MoveGroup(int from_index, int to_index) {
  int count = static_cast<int>(groups_.size());
  if (from_index < 0 || from_index >= count || to_index < 0 ||
      to_index >= count || from_index == to_index) {
    return;
  }

  // Perform the move.
  AstraTabGroupInfo moved = groups_[from_index].info;
  groups_.erase(groups_.begin() + from_index);
  groups_.insert(groups_.begin() + to_index, GroupEntry());
  groups_[to_index].info = moved;

  // Update order indices for kManual sort mode.
  if (sort_by_ == AstraTabGroupSortBy::kManual) {
    for (size_t i = 0; i < groups_.size(); ++i) {
      groups_[i].info.order_index = static_cast<int>(i);
    }
  }

  // Adjust selection if needed.
  if (selected_group_index_ == from_index) {
    selected_group_index_ = to_index;
  } else if (from_index < selected_group_index_ &&
             to_index >= selected_group_index_) {
    selected_group_index_--;
  } else if (from_index > selected_group_index_ &&
             to_index <= selected_group_index_) {
    selected_group_index_++;
  }

  RebuildAllViews();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnGroupReordered(from_index, to_index);
  }
}

// =========================================================================
// Color
// =========================================================================

void AstraSidebarTabGroupsView::SetGroupColor(int index, SkColor color) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  groups_[index].info.color = color;
  if (groups_[index].header) {
    groups_[index].header->SetColor(color);
  }

  // Notify delegate.
  if (delegate_) {
    delegate_->OnGroupColorChanged(groups_[index].info.group_id, color);
  }
}

SkColor AstraSidebarTabGroupsView::GetGroupColor(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].info.color;
}

// =========================================================================
// Naming
// =========================================================================

void AstraSidebarTabGroupsView::RenameGroup(int index,
                                            const std::u16string& new_name) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  groups_[index].info.name = new_name;
  if (groups_[index].header) {
    groups_[index].header->SetName(new_name);
  }

  // Re-sort if sorting by name.
  if (sort_by_ == AstraTabGroupSortBy::kName) {
    ApplySortOrder();
    RebuildAllViews();
  }

  // Notify delegate.
  if (delegate_) {
    delegate_->OnGroupRenamed(groups_[index].info.group_id, new_name);
  }
}

std::u16string AstraSidebarTabGroupsView::GetGroupName(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].info.name;
}

// =========================================================================
// Tab counts
// =========================================================================

int AstraSidebarTabGroupsView::GetTabCountInGroup(int index) const {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].info.tab_count;
}

int AstraSidebarTabGroupsView::GetTotalTabCount() const {
  int total = 0;
  for (const auto& entry : groups_) {
    total += entry.info.tab_count;
  }
  return total;
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarTabGroupsView::SetShowTabCount(bool show) {
  if (show_tab_count_ == show) {
    return;
  }
  show_tab_count_ = show;
  ApplyDisplayOptions();
}

bool AstraSidebarTabGroupsView::GetShowTabCount() const {
  return show_tab_count_;
}

void AstraSidebarTabGroupsView::SetShowAddGroupButton(bool show) {
  if (show_add_group_button_ == show) {
    return;
  }
  show_add_group_button_ = show;
  if (add_group_button_) {
    add_group_button_->SetVisible(show);
  }
}

bool AstraSidebarTabGroupsView::GetShowAddGroupButton() const {
  return show_add_group_button_;
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarTabGroupsView::SetSortGroupsBy(AstraTabGroupSortBy sort_by) {
  if (sort_by_ == sort_by) {
    return;
  }
  sort_by_ = sort_by;
  ApplySortOrder();
  RebuildAllViews();
}

AstraTabGroupSortBy AstraSidebarTabGroupsView::GetSortGroupsBy() const {
  return sort_by_;
}

void AstraSidebarTabGroupsView::ApplySortOrder() {
  // Remember selected group ID to restore selection after sort.
  std::string selected_id;
  if (selected_group_index_ >= 0 &&
      selected_group_index_ < static_cast<int>(groups_.size())) {
    selected_id = groups_[selected_group_index_].info.group_id;
  }

  switch (sort_by_) {
    case AstraTabGroupSortBy::kManual:
      base::ranges::sort(groups_, [](const GroupEntry& a, const GroupEntry& b) {
        return CompareGroupsByOrderIndex(a.info, b.info);
      });
      break;
    case AstraTabGroupSortBy::kName:
      base::ranges::sort(groups_, [](const GroupEntry& a, const GroupEntry& b) {
        return CompareGroupsByName(a.info, b.info);
      });
      break;
    case AstraTabGroupSortBy::kColor:
      base::ranges::sort(groups_, [](const GroupEntry& a, const GroupEntry& b) {
        return CompareGroupsByColor(a.info, b.info);
      });
      break;
    case AstraTabGroupSortBy::kTabCount:
      base::ranges::sort(groups_, [](const GroupEntry& a, const GroupEntry& b) {
        return CompareGroupsByTabCount(a.info, b.info);
      });
      break;
    case AstraTabGroupSortBy::kLastAccessed:
      base::ranges::sort(groups_, [](const GroupEntry& a, const GroupEntry& b) {
        return CompareGroupsByLastAccessed(a.info, b.info);
      });
      break;
  }

  // Restore selection by ID.
  if (!selected_id.empty()) {
    selected_group_index_ = FindGroupIndexById(selected_id);
  }
}

// =========================================================================
// Group operations
// =========================================================================

void AstraSidebarTabGroupsView::NewGroup(const std::u16string& name,
                                         SkColor color) {
  // In presentation mode, we forward to delegate.
  // The actual group creation happens in Chromium's TabStripModel.
  if (delegate_) {
    delegate_->OnNewGroupRequested();
  }
}

void AstraSidebarTabGroupsView::DeleteGroup(int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  std::string group_id = groups_[index].info.group_id;

  // Remove from our projection.
  RemoveGroup(index);

  // Notify delegate.
  if (delegate_) {
    delegate_->OnDeleteGroupRequested(group_id);
  }
}

void AstraSidebarTabGroupsView::CloseAllTabsInGroup(int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));

  std::string group_id = groups_[index].info.group_id;

  // In presentation mode, forward to delegate.
  if (delegate_) {
    delegate_->OnCloseAllTabsInGroupRequested(group_id);
  }
}

// =========================================================================
// Tab operations between groups
// =========================================================================

void AstraSidebarTabGroupsView::MoveTabToGroup(int from_group, int from_tab,
                                               int to_group, int to_tab) {
  DCHECK_GE(from_group, 0);
  DCHECK_LT(from_group, static_cast<int>(groups_.size()));
  DCHECK_GE(to_group, 0);
  DCHECK_LT(to_group, static_cast<int>(groups_.size()));
  DCHECK_GE(from_tab, 0);
  DCHECK_GE(to_tab, 0);

  // Adjust tab counts.
  groups_[from_group].info.tab_count--;
  groups_[to_group].info.tab_count++;

  // Update headers.
  if (groups_[from_group].header) {
    groups_[from_group].header->SetTabCount(groups_[from_group].info.tab_count);
  }
  if (groups_[to_group].header) {
    groups_[to_group].header->SetTabCount(groups_[to_group].info.tab_count);
  }

  // Re-sort if needed.
  if (sort_by_ == AstraTabGroupSortBy::kTabCount) {
    ApplySortOrder();
  }

  RebuildAllViews();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnTabDropped(
        groups_[to_group].info.group_id, to_tab,
        groups_[from_group].info.group_id, from_tab);
  }
}

void AstraSidebarTabGroupsView::UngroupTab(int group_index, int tab_index) {
  DCHECK_GE(group_index, 0);
  DCHECK_LT(group_index, static_cast<int>(groups_.size()));
  DCHECK_GE(tab_index, 0);

  std::string group_id = groups_[group_index].info.group_id;

  // Adjust tab count.
  groups_[group_index].info.tab_count--;
  if (groups_[group_index].header) {
    groups_[group_index].header->SetTabCount(
        groups_[group_index].info.tab_count);
  }

  // Re-sort if needed.
  if (sort_by_ == AstraTabGroupSortBy::kTabCount) {
    ApplySortOrder();
  }

  RebuildAllViews();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnUngroupTabRequested(group_id, tab_index);
  }
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarTabGroupsView::SetDragDropEnabled(bool enabled) {
  drag_drop_enabled_ = enabled;
}

bool AstraSidebarTabGroupsView::GetDragDropEnabled() const {
  return drag_drop_enabled_;
}

// =========================================================================
// Color display
// =========================================================================

void AstraSidebarTabGroupsView::SetShowGroupColors(bool show) {
  if (show_group_colors_ == show) {
    return;
  }
  show_group_colors_ = show;
  ApplyDisplayOptions();
}

bool AstraSidebarTabGroupsView::GetShowGroupColors() const {
  return show_group_colors_;
}

// =========================================================================
// Compact mode
// =========================================================================

void AstraSidebarTabGroupsView::SetCompactMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;
  ApplyDisplayOptions();
}

bool AstraSidebarTabGroupsView::GetCompactMode() const {
  return compact_mode_;
}

// =========================================================================
// View access
// =========================================================================

AstraTabGroupHeaderView* AstraSidebarTabGroupsView::GetGroupHeaderViewAt(
    int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(groups_.size()));
  return groups_[index].header;
}

AstraTabGroupTabItemView* AstraSidebarTabGroupsView::GetGroupTabItemAt(
    int group_index, int tab_index) {
  DCHECK_GE(group_index, 0);
  DCHECK_LT(group_index, static_cast<int>(groups_.size()));
  DCHECK_GE(tab_index, 0);
  DCHECK_LT(tab_index, static_cast<int>(groups_[group_index].tabs.size()));
  return groups_[group_index].tabs[tab_index];
}

int AstraSidebarTabGroupsView::GetGroupTabCountAt(int group_index) const {
  DCHECK_GE(group_index, 0);
  DCHECK_LT(group_index, static_cast<int>(groups_.size()));
  return static_cast<int>(groups_[group_index].tabs.size());
}

// =========================================================================
// Display options application
// =========================================================================

void AstraSidebarTabGroupsView::ApplyDisplayOptions() {
  for (auto& entry : groups_) {
    if (entry.header) {
      entry.header->SetShowTabCount(show_tab_count_);
      entry.header->SetShowColorDot(show_group_colors_);
      entry.header->SetCompact(compact_mode_);
    }
  }
}

// =========================================================================
// View rebuilding
// =========================================================================

void AstraSidebarTabGroupsView::RebuildAllViews() {
  ClearGroups();

  for (size_t i = 0; i < groups_.size(); ++i) {
    auto& entry = groups_[i];

    // Create header.
    auto header = CreateGroupHeader(entry.info);
    entry.header = header.get();
    groups_container_->AddChildView(std::move(header));

    // Set selection state.
    if (static_cast<int>(i) == selected_group_index_) {
      entry.header->SetSelected(true);
    }

    // Create tab items if expanded.
    if (entry.info.is_expanded) {
      RebuildGroupTabs(entry);
    } else {
      entry.tabs.clear();
      entry.tab_container = nullptr;
    }
  }

  InvalidateLayout();
}

void AstraSidebarTabGroupsView::RebuildGroupTabs(GroupEntry& entry) {
  entry.tabs.clear();

  // Create dummy tab items based on tab_count for presentation purposes.
  // In a real Chromium build, these would be populated from TabStripModel.
  // TODO(astra): Replace with real tab data when fully integrated with
  //   TabStripModel and WebContents.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  for (int i = 0; i < entry.info.tab_count; ++i) {
    AstraTabGroupTabInfo tab_info;
    tab_info.tab_id = entry.info.group_id + "_tab_" + base::NumberToString(i);
    tab_info.title = u"Tab " + base::NumberToString16(i + 1);
    tab_info.index_in_group = i;
    tab_info.group_id = entry.info.group_id;
    tab_info.is_active = i == 0;  // First tab is "active" in dummy data.

    auto tab_item = CreateGroupTabItem(tab_info);
    raw_ptr<AstraTabGroupTabItemView> raw = tab_item.get();
    entry.tabs.push_back(raw);
    groups_container_->AddChildView(std::move(tab_item));
  }
}

void AstraSidebarTabGroupsView::ClearGroups() {
  groups_container_->RemoveAllChildViews();
  for (auto& entry : groups_) {
    entry.header = nullptr;
    entry.tabs.clear();
    entry.tab_container = nullptr;
  }
}

std::unique_ptr<AstraTabGroupHeaderView>
AstraSidebarTabGroupsView::CreateGroupHeader(const AstraTabGroupInfo& info) {
  int group_index = FindGroupIndexById(info.group_id);

  auto header = std::make_unique<AstraTabGroupHeaderView>(
      info.name, info.color,
      base::BindRepeating(&AstraSidebarTabGroupsView::HandleGroupHeaderClicked,
                          base::Unretained(this), group_index),
      base::BindRepeating(
          [](AstraSidebarTabGroupsView* view, int idx) {
            // New tab in group.
            // TODO(astra): Implement proper "new tab in group" flow.
          },
          base::Unretained(this), group_index));

  header->SetGroupInfo(info);
  header->SetShowTabCount(show_tab_count_);
  header->SetShowColorDot(show_group_colors_);
  header->SetCompact(compact_mode_);

  return header;
}

std::unique_ptr<AstraTabGroupTabItemView>
AstraSidebarTabGroupsView::CreateGroupTabItem(
    const AstraTabGroupTabInfo& info) {
  int group_index = FindGroupIndexById(info.group_id);

  auto item = std::make_unique<AstraTabGroupTabItemView>(
      info.title, info.index_in_group,
      base::BindRepeating(
          [](AstraSidebarTabGroupsView* view, int g_idx, int t_idx,
             const ui::Event& event) {
            view->HandleTabClicked(g_idx, t_idx);
          },
          base::Unretained(this), group_index, info.index_in_group),
      base::BindRepeating(
          &AstraSidebarTabGroupsView::HandleTabClosed, base::Unretained(this),
          group_index, info.index_in_group));

  item->SetTabInfo(info);
  item->SetShowFavicon(show_group_colors_);  // Reuse flag for favicons

  return item;
}

int AstraSidebarTabGroupsView::FindGroupIndexById(
    const std::string& group_id) const {
  for (size_t i = 0; i < groups_.size(); ++i) {
    if (groups_[i].info.group_id == group_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// =========================================================================
// Handlers for user actions
// =========================================================================

void AstraSidebarTabGroupsView::HandleGroupHeaderClicked(int group_index) {
  if (group_index < 0 || group_index >= static_cast<int>(groups_.size())) {
    return;
  }

  // Set selection.
  SetSelectedGroup(group_index);

  // Toggle expanded state.
  ToggleGroupExpanded(group_index);

  // Notify delegate of click.
  if (delegate_) {
    delegate_->OnGroupClicked(groups_[group_index].info.group_id);
  }
}

void AstraSidebarTabGroupsView::HandleTabClicked(int group_index,
                                                 int tab_index) {
  if (delegate_ && group_index >= 0 &&
      group_index < static_cast<int>(groups_.size())) {
    delegate_->OnTabClicked(groups_[group_index].info.group_id, tab_index);
  }
}

void AstraSidebarTabGroupsView::HandleTabClosed(int group_index,
                                                int tab_index) {
  if (delegate_ && group_index >= 0 &&
      group_index < static_cast<int>(groups_.size())) {
    delegate_->OnTabClosed(groups_[group_index].info.group_id, tab_index);
  }
}

void AstraSidebarTabGroupsView::HandleAddGroupClicked() {
  if (delegate_) {
    delegate_->OnNewGroupRequested();
  }
}

// =========================================================================
// TabStripModel integration
// =========================================================================

void AstraSidebarTabGroupsView::UpdateFromModel() {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }

  TabStripModel* tab_strip = browser_->tab_strip_model();
  PopulateGroupsFromModel(tab_strip);
  RebuildAllViews();
}

void AstraSidebarTabGroupsView::PopulateGroupsFromModel(
    TabStripModel* tab_strip) {
  TabGroupModel* group_model = tab_strip->group_model();
  if (!group_model) {
    groups_.clear();
    return;
  }

  const std::vector<tab_groups::TabGroupId>& group_ids =
      group_model->ListTabGroups();

  groups_.clear();
  groups_.reserve(group_ids.size());

  int order_index = 0;
  for (const auto& group_id : group_ids) {
    const TabGroup* group = group_model->GetTabGroup(group_id);
    if (!group) {
      continue;
    }

    GroupEntry entry;
    entry.info = TabGroupToInfo(group_id, group);
    entry.info.order_index = order_index++;
    groups_.push_back(std::move(entry));
  }

  ApplySortOrder();
}

AstraTabGroupInfo AstraSidebarTabGroupsView::TabGroupToInfo(
    const tab_groups::TabGroupId& group_id, const TabGroup* group) const {
  AstraTabGroupInfo info;

  // Convert TabGroupId to string for our use.
  // TODO(astra): Use TabGroupId directly instead of string conversion.
  // Chromium's TabGroupId is a token type; we convert to string for
  // simplicity in the Astra layer.
  info.group_id = base::NumberToString(group_id.ToString());

  // Get display name.
  if (group->visual_data()) {
    info.name = group->visual_data()->title();
    info.color_id = static_cast<int>(group->visual_data()->color());
    // TODO(astra): Resolve actual SkColor from color ID via ColorProvider.
    info.color = SK_ColorGRAY;
  }
  if (info.name.empty()) {
    info.name = u"Untitled Group";
  }

  // Get tab count.
  gfx::Range tab_range = group->ListTabs();
  info.tab_count = static_cast<int>(tab_range.length());

  // Collapsed state.
  info.is_collapsed_in_tabstrip = group->IsCollapsed();

  // Default expanded state for sidebar.
  info.is_expanded = true;

  return info;
}

// =========================================================================
// TabStripModelObserver implementations
// =========================================================================
// All of these currently call UpdateFromModel() (full rebuild).
// TODO(astra): Implement incremental updates for better performance.

void AstraSidebarTabGroupsView::OnTabStripModelChanged(
    TabStripModel* /*tab_strip_model*/,
    const TabStripModelChange& /*change*/,
    const TabStripSelectionChange& /*selection*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabGroupChanged(const TabGroupChange& /*change*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabGroupVisualsChanged(
    TabStripModel* /*tab_strip_model*/,
    const tab_groups::TabGroupId& /*group*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabInsertedAt(TabStripModel* /*tab_strip_model*/,
                                                int /*index*/,
                                                bool /*foreground*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabRemovedAt(TabStripModel* /*tab_strip_model*/,
                                               int /*index*/,
                                               bool /*was_active*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabMoved(TabStripModel* /*tab_strip_model*/,
                                           int /*from_index*/,
                                           int /*to_index*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnActiveTabChanged(
    TabStripModel* /*tab_strip_model*/,
    int /*old_index*/,
    int /*new_index*/,
    const TabStripSelectionChange& /*selection*/) {
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabChanged(TabStripModel* /*tab_strip_model*/,
                                             int /*index*/,
                                             TabChangeType /*change_type*/) {
  UpdateFromModel();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarTabGroupsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarTabGroupsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Tab groups");
}

void AstraSidebarTabGroupsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  section_title_->SetEnabledColor(
      color_provider->GetColor(kTabGroupsSectionTitleTextColorId));

  if (add_group_button_) {
    add_group_button_->SetEnabledTextColors(
        color_provider->GetColor(kTabGroupsAddButtonTextColorId));
  }
}

}  // namespace astra
