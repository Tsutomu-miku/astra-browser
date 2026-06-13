#include "astra/ui/views/sidebar/astra_sidebar_stack_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_tab_stack_service.h"
#include "astra/browser/astra_tab_stack_service_factory.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_header_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_tab_item_view.h"
#include "base/containers/flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/web_contents.h"
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
const char kDefaultNewStackColor[] = "#5B8FF9";

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

void AstraSidebarStackView::BuildLayout() {
  // Vertical box layout: section title + stacks container + new stack button.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Section title label.
  section_title_ = AddChildView(std::make_unique<views::Label>(kStacksTitle));
  section_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_title_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kStacksSectionVerticalPadding, kStacksSectionHorizontalPadding)));
  section_title_->SetFontList(
      section_title_->font_list().DeriveWithSizeDelta(
          kStacksSectionHeaderFontSizeDelta));
  section_title_->SetAutoColorReadabilityEnabled(false);

  // Stacks container — holds all stack headers and their tab items.
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

  ClearStacks();
  PopulateStacks();

  InvalidateLayout();
}

void AstraSidebarStackView::ClearStacks() {
  stacks_container_->RemoveAllChildViews();
  header_views_.clear();
}

void AstraSidebarStackView::PopulateStacks() {
  AstraTabStackService* service = GetStackService();
  if (!service) {
    return;
  }

  std::vector<AstraTabStack> stacks = service->GetAllStacks();
  if (stacks.empty()) {
    // No stacks — the section will show just the title and "New stack" button.
    return;
  }

  for (const auto& stack : stacks) {
    // Create and add the stack header.
    auto header = CreateStackHeader(stack);
    AstraSidebarStackHeaderView* header_raw = header.get();
    stacks_container_->AddChildView(std::move(header));
    header_views_[stack.id] = header_raw;

    // If expanded, create and add tab items for each tab in the stack.
    if (!stack.collapsed) {
      std::vector<content::WebContents*> tabs =
          service->GetTabsInStack(stack.id);
      for (content::WebContents* web_contents : tabs) {
        auto tab_item = CreateStackTabItem(web_contents, stack.id);
        stacks_container_->AddChildView(std::move(tab_item));
      }
    }
  }
}

std::unique_ptr<AstraSidebarStackHeaderView>
AstraSidebarStackView::CreateStackHeader(const AstraTabStack& stack) {
  auto header = std::make_unique<AstraSidebarStackHeaderView>(
      base::UTF8ToUTF16(stack.name));

  header->set_stack_id(stack.id);
  header->SetAccentColor(stack.color);
  header->SetChildCount(stack.tab_count);
  header->SetExpanded(!stack.collapsed);
  header->set_delegate(this);

  // Check if any tab in this stack is the active tab.
  bool is_active = false;
  if (browser_ && browser_->tab_strip_model()) {
    content::WebContents* active_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    if (active_contents) {
      AstraTabFeatures* features =
          AstraTabFeatures::FromWebContents(active_contents);
      if (features && features->stack_id() == stack.id) {
        is_active = true;
      }
    }
  }
  header->SetActive(is_active);

  return header;
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
  item->set_stack_id(stack_id);
  item->set_delegate(this);
  item->SetDraggable(true);

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
  CreateNewStack();
}

std::string AstraSidebarStackView::CreateNewStack() {
  AstraTabStackService* service = GetStackService();
  if (!service) {
    return std::string();
  }

  // Generate a unique name.
  std::string name = kDefaultNewStackName;
  size_t count = service->GetStackCount();
  if (count > 0) {
    name += " " + base::NumberToString(count + 1);
  }

  return service->CreateStack(name, kDefaultNewStackColor);
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
    it->second->SetTitle(base::UTF8ToUTF16(new_name));
  } else {
    UpdateFromModel();
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
        it->second->SetChildCount(stack->tab_count);
      }
    }
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
        it->second->SetChildCount(stack->tab_count);
      }
    }
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
  // Full rebuild to remove tab item views.
  // TODO(astra): Optimize by just hiding the tab item views.
  UpdateFromModel();
}

void AstraSidebarStackView::OnStackExpanded(
    const AstraTabStackId& stack_id) {
  auto it = header_views_.find(stack_id);
  if (it != header_views_.end() && it->second) {
    it->second->SetExpanded(true);
  }
  // Full rebuild to show tab item views.
  // TODO(astra): Optimize by inserting the tab item views.
  UpdateFromModel();
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
  AstraTabStackService* service = GetStackService();
  if (!service) {
    return;
  }

  service->ToggleStack(stack_id);
}

void AstraSidebarStackView::OnStackHeaderClicked(
    const std::string& stack_id) {
  // Clicking the stack header activates the first tab in the stack
  // and also toggles the expanded state.
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

  // Toggle expanded state.
  AstraTabStackService* service = GetStackService();
  if (service) {
    service->ToggleStack(stack_id);
  }

  // Activate the first tab in the stack (if expanded now).
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
}

}  // namespace astra
