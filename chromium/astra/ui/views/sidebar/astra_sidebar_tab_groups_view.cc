#include "astra/ui/views/sidebar/astra_sidebar_tab_groups_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_tab_group_header_view.h"
#include "astra/ui/views/sidebar/astra_tab_group_tab_item_view.h"
#include "base/containers/flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "content/public/browser/web_contents.h"
#include "tab_groups/tab_group_color.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kTabGroupsSectionHeaderHeight = 28;
constexpr int kTabGroupsSectionHorizontalPadding = 12;
constexpr int kTabGroupsSectionVerticalPadding = 8;
constexpr int kTabGroupsSectionHeaderFontSizeDelta = 1;
constexpr int kTabGroupsSectionItemSpacing = 2;
constexpr int kTabGroupsGroupSpacing = 4;

// Astra color ID for the tab groups panel.
// Uses the Astra sidebar section header color from the Astra color system.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kTabGroupsSectionTitleTextColorId =
    kColorAstraSidebarSectionHeaderText;

// Default section title.
const char16_t kTabGroupsTitle[] = u"Tab Groups";

}  // namespace

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

void AstraSidebarTabGroupsView::BuildLayout() {
  // Vertical box layout: section title + groups container.
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
}

void AstraSidebarTabGroupsView::SetTitle(const std::u16string& title) {
  section_title_->SetText(title);
}

void AstraSidebarTabGroupsView::UpdateFromModel() {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }

  TabStripModel* tab_strip = browser_->tab_strip_model();
  ClearGroups();
  PopulateGroups(tab_strip);

  InvalidateLayout();
}

void AstraSidebarTabGroupsView::ClearGroups() {
  // Remove all group views. Each "group" in the container is actually a
  // sub-view containing the header + tab items, or we interleave headers
  // and items. For simplicity, we use a flat list where headers and items
  // are siblings in the container, with items indented under their header.
  //
  // TODO(astra): Consider a tree structure where each group has its own
  // container view (header + items), making expand/collapse a simple
  // visibility toggle on the items container. This is the cleaner approach
  // but requires one extra view per group.
  groups_container_->RemoveAllChildViews();
}

void AstraSidebarTabGroupsView::PopulateGroups(TabStripModel* tab_strip) {
  TabGroupModel* group_model = tab_strip->group_model();
  if (!group_model) {
    // TODO(astra): Handle case where TabStripModel doesn't expose a
    // group_model(). In some Chromium configurations, groups may not be
    // available. We should gracefully hide the tab groups section.
    // Chromium owner: TabStripModel::group_model()
    return;
  }

  const std::vector<tab_groups::TabGroupId>& group_ids = group_model->ListTabGroups();
  if (group_ids.empty()) {
    // No groups — section will be empty. The parent sidebar may choose
    // to hide the entire section when there are no groups.
    return;
  }

  for (const auto& group_id : group_ids) {
    const TabGroup* group = group_model->GetTabGroup(group_id);
    if (!group) {
      continue;
    }

    // Create and add the group header.
    auto header = CreateGroupHeader(group_id, group);
    groups_container_->AddChildView(std::move(header));

    // Determine if this group is expanded in the sidebar.
    bool expanded = true;
    auto it = expanded_state_.find(group_id);
    if (it != expanded_state_.end()) {
      expanded = it->second;
    } else {
      // Default to expanded for new groups.
      expanded_state_[group_id] = true;
    }

    if (expanded) {
      // Create and add tab items for each tab in the group.
      gfx::Range tab_range = group->ListTabs();
      for (size_t i = tab_range.start(); i < tab_range.end(); ++i) {
        auto tab_item = CreateGroupTabItem(static_cast<int>(i), group_id);
        groups_container_->AddChildView(std::move(tab_item));
      }
    }
  }
}

std::unique_ptr<views::View> AstraSidebarTabGroupsView::CreateGroupHeader(
    const tab_groups::TabGroupId& group_id,
    const TabGroup* group) {
  // Get group display name.
  std::u16string title;
  if (group->visual_data()) {
    title = group->visual_data()->title();
  }
  if (title.empty()) {
    // Fallback: use group id or a default name.
    // TODO(astra): Use the same default naming as Chromium's tab strip.
    // Chromium shows "Group" or a color-based name for unnamed groups.
    title = u"Untitled Group";
  }

  // Get tab count.
  int tab_count = 0;
  gfx::Range tab_range = group->ListTabs();
  tab_count = static_cast<int>(tab_range.length());

  // Get group color.
  tab_groups::TabGroupColorId color = tab_groups::TabGroupColorId::kGrey;
  if (group->visual_data()) {
    color = group->visual_data()->color();
  }

  auto header = std::make_unique<AstraTabGroupHeaderView>(
      title, color,
      base::BindRepeating(&AstraSidebarTabGroupsView::ToggleGroupExpanded,
                          base::Unretained(this), group_id),
      base::BindRepeating(&AstraSidebarTabGroupsView::NewTabInGroup,
                          base::Unretained(this), group_id));

  header->SetTabCount(tab_count);

  // Set initial expanded state.
  auto it = expanded_state_.find(group_id);
  if (it != expanded_state_.end()) {
    header->SetExpanded(it->second);
  }

  return header;
}

std::unique_ptr<AstraTabGroupTabItemView>
AstraSidebarTabGroupsView::CreateGroupTabItem(
    int tab_index,
    const tab_groups::TabGroupId& /*group_id*/) {
  content::WebContents* web_contents =
      browser_->tab_strip_model()->GetWebContentsAt(tab_index);

  // Determine the tab title.
  std::u16string title;
  if (web_contents && !web_contents->GetTitle().empty()) {
    title = web_contents->GetTitle();
  } else {
    title = u"Tab " + base::NumberToString16(tab_index + 1);
  }

  auto item = std::make_unique<AstraTabGroupTabItemView>(
      title, tab_index,
      base::BindRepeating(
          [](AstraSidebarTabGroupsView* view, int idx,
             const ui::Event& /*event*/) {
            view->ActivateTab(idx);
          },
          base::Unretained(this), tab_index),
      base::BindRepeating(&AstraSidebarTabGroupsView::CloseTab,
                          base::Unretained(this), tab_index));

  // Mark as active if this is the currently selected tab.
  if (browser_->tab_strip_model()->active_index() == tab_index) {
    item->SetActive(true);
  }

  return item;
}

void AstraSidebarTabGroupsView::ToggleGroupExpanded(
    const tab_groups::TabGroupId& group_id) {
  // Toggle sidebar-local expanded state.
  bool new_state = true;
  auto it = expanded_state_.find(group_id);
  if (it != expanded_state_.end()) {
    new_state = !it->second;
  }
  expanded_state_[group_id] = new_state;

  // Rebuild from model to reflect the new expanded state.
  // TODO(astra): Optimize by only toggling visibility of the tab items
  // for this group, instead of rebuilding everything. Each group's tab
  // items could be in a dedicated container view that we just show/hide.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::NewTabInGroup(
    const tab_groups::TabGroupId& group_id) {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }

  TabStripModel* tab_strip = browser_->tab_strip_model();

  // Add a new tab and add it to the group.
  //
  // TODO(astra): Use Chromium's proper "new tab in group" flow.
  // The correct approach is to:
  //   1. Create a new tab (Browser::AddNewTab or similar)
  //   2. Add it to the target group via TabStripModel::AddToGroup or
  //      TabGroupController::AddTab
  //
  // Chromium owner:
  //   - TabGroupController (chrome/browser/ui/tabs/tab_group_controller.h)
  //   - TabStripModel::AddToGroup() or similar method
  //   - Browser::AddNewTabWithWebContents() or AddSelectedTab()
  //
  // For now, we add a new tab at the end of the group as a proof of concept.
  // The sidebar will update via TabStripModelObserver.
  //
  // TODO(astra): This creates a tab but doesn't add it to the group.
  // We need to use TabGroupController or TabStripModel::SetTabGroup()
  // to actually add the new tab to the target group.
  // Chromium owner: TabGroupController::AddNewTabInGroup()

  // For now, use the standard "new tab" action.
  // TODO(astra): Actually route through Chromium's command system (IDC_NEW_TAB)
  // and then move the new tab into the group, or find the proper
  // "add tab to group" API.
  //
  // The proper Chromium API is:
  //   TabStripModel::AddToGroup(int index, const TabGroupId& group)
  //
  // But creating a new tab goes through Browser::AddNewTab(). We need to
  // chain these: create tab, then add to group.
  //
  // For this proof of concept, we'll dispatch to Chromium's new tab command.
  // The tab will be created but not automatically added to the group —
  // that's a TODO(astra) item for when we have the full TabGroupController integration.
  browser_->ExecuteCommand(IDC_NEW_TAB);
}

void AstraSidebarTabGroupsView::ActivateTab(int tab_index) {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }
  if (tab_index < 0 || tab_index >= browser_->tab_strip_model()->GetTabCount()) {
    return;
  }
  browser_->tab_strip_model()->ActivateTabAt(tab_index);
}

void AstraSidebarTabGroupsView::CloseTab(int tab_index) {
  if (!browser_ || !browser_->tab_strip_model()) {
    return;
  }
  if (tab_index < 0 || tab_index >= browser_->tab_strip_model()->GetTabCount()) {
    return;
  }
  // Dispatch close through Chromium's tab strip model.
  // TODO(astra): Use the proper tab close API. Options:
  //   - TabStripModel::CloseWebContentsAt()
  //   - Browser::CloseTabAt()
  //   - BrowserCommandController::ExecuteCommand(IDC_CLOSE_TAB)
  //
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  browser_->tab_strip_model()->CloseWebContentsAt(
      tab_index, TabStripModel::CLOSE_NONE);
}

tab_groups::TabGroupColorId AstraSidebarTabGroupsView::GetGroupColor(
    const tab_groups::TabGroupId& group_id) const {
  if (!browser_ || !browser_->tab_strip_model()) {
    return tab_groups::TabGroupColorId::kGrey;
  }
  TabGroupModel* group_model = browser_->tab_strip_model()->group_model();
  if (!group_model) {
    return tab_groups::TabGroupColorId::kGrey;
  }
  const TabGroup* group = group_model->GetTabGroup(group_id);
  if (!group || !group->visual_data()) {
    return tab_groups::TabGroupColorId::kGrey;
  }
  return group->visual_data()->color();
}

// =========================================================================
// TabStripModelObserver
// =========================================================================
//
// Primary reactive update path. All group and tab state changes originate
// from Chromium's TabStripModel and flow through these observer methods.
// The tab groups view is a pure projection — it never mutates TabStripModel
// directly except in response to explicit user actions (click to activate,
// close button, etc.).
//
// TODO(astra): Implement incremental updates for each change type instead
// of calling UpdateFromModel() (full rebuild) on every change. Full rebuilds
// are correct but inefficient for frequent operations. The main challenge
// is mapping TabStripModel indices to sidebar view indices when groups
// are interleaved in a flat list.
//
// Performance note: With many groups and tabs, full rebuilds cause
// noticeable jank on every tab change. Incremental updates would be O(1)
// for most operations.

void AstraSidebarTabGroupsView::OnTabStripModelChanged(
    TabStripModel* /*tab_strip_model*/,
    const TabStripModelChange& /*change*/,
    const TabStripSelectionChange& /*selection*/) {
  // Generic change handler — catches any change not handled by more
  // specific methods. Full rebuild is the safe default.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabGroupChanged(const TabGroupChange& change) {
  // A tab group was added, removed, or had tabs moved into/out of it.
  // TODO(astra): Handle each case incrementally:
  //   - kCreated: insert a new group header at the right position
  //   - kDeleted: remove the group header and its tab items
  //   - kMoved: reorder the group in the list
  //   - kContentsChanged: update tab items for this group
  //
  // Chromium owner: TabGroupChange (chrome/browser/ui/tabs/tab_strip_model_observer.h)
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabGroupVisualsChanged(
    TabStripModel* /*tab_strip_model*/,
    const tab_groups::TabGroupId& /*group*/) {
  // Group visual state changed (name, color, etc.).
  // TODO(astra): Incrementally update just the header view for this group
  // instead of rebuilding everything.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabInsertedAt(TabStripModel* /*tab_strip_model*/,
                                                int /*index*/,
                                                bool /*foreground*/) {
  // A new tab was inserted. It might belong to a group.
  // TODO(astra): Incrementally insert a tab item into the appropriate group.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabRemovedAt(TabStripModel* /*tab_strip_model*/,
                                               int /*index*/,
                                               bool /*was_active*/) {
  // A tab was removed. It might have been in a group.
  // TODO(astra): Incrementally remove the tab item from its group.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabMoved(TabStripModel* /*tab_strip_model*/,
                                           int /*from_index*/,
                                           int /*to_index*/) {
  // A tab was moved. It may have moved into, out of, or within a group.
  // TODO(astra): Handle group membership changes incrementally.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnActiveTabChanged(
    TabStripModel* /*tab_strip_model*/,
    int /*old_index*/,
    int /*new_index*/,
    const TabStripSelectionChange& /*selection*/) {
  // The active tab changed. Update the active highlight.
  // TODO(astra): Incrementally update the active highlight by finding
  // the old and new tab items and toggling their active state.
  UpdateFromModel();
}

void AstraSidebarTabGroupsView::OnTabChanged(TabStripModel* /*tab_strip_model*/,
                                             int /*index*/,
                                             TabChangeType /*change_type*/) {
  // A tab's display state changed (title, favicon, etc.).
  // TODO(astra): Incrementally update just that tab item's title/icon.
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
}

}  // namespace astra
