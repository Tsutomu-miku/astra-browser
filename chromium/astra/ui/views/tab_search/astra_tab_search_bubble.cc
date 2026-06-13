#include "astra/ui/views/tab_search/astra_tab_search_bubble.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/tab_search/astra_tab_search_item_view.h"

namespace astra {

namespace {

// Bubble sizing constants.
constexpr int kDefaultBubbleWidth = 360;
constexpr int kDefaultBubbleMaxHeight = 480;

// Layout constants.
constexpr int kSearchFieldHeight = 44;
constexpr int kResultCountHeight = 28;
constexpr int kResultsMaxHeight = 380;
constexpr int kDividerThickness = 1;
constexpr int kGroupHeaderHeight = 32;
constexpr int kBubblePadding = 8;

// Color IDs.
// TODO(astra): Define dedicated tab-search color IDs in astra_color_ids.h.
constexpr ui::ColorId kBubbleBackground = kColorAstraCommandPaletteBackground;
constexpr ui::ColorId kBubbleBorder = kColorAstraCommandPaletteBorder;
constexpr ui::ColorId kSearchFieldBackground = ui::kColorTextFieldBackground;
constexpr ui::ColorId kSearchFieldText = kColorAstraCommandPaletteSearchText;
constexpr ui::ColorId kDividerColor = ui::kColorSeparator;
constexpr ui::ColorId kResultCountText =
    kColorAstraCommandPaletteDescriptionText;
constexpr ui::ColorId kGroupHeaderText =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kGroupHeaderBackground =
    kColorAstraCommandPaletteBackground;

// Extract the display host from a WebContents (e.g. "example.com").
std::u16string GetDisplayHost(content::WebContents* web_contents) {
  if (!web_contents) {
    return std::u16string();
  }
  const GURL& url = web_contents->GetURL();
  if (!url.is_valid() || url.host().empty()) {
    return base::UTF8ToUTF16(url.spec());
  }
  return base::UTF8ToUTF16(url.host());
}

// Get the tab title from a WebContents.
std::u16string GetTabTitle(content::WebContents* web_contents) {
  if (!web_contents) {
    return std::u16string();
  }
  std::u16string title = web_contents->GetTitle();
  if (title.empty()) {
    title = base::UTF8ToUTF16(web_contents->GetURL().spec());
  }
  return title;
}

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraTabSearchBubble::ShowBubble(views::View* anchor_view,
                                                Browser* browser,
                                                Delegate* delegate) {
  DCHECK(anchor_view);
  DCHECK(browser);

  auto* bubble = new AstraTabSearchBubble(anchor_view, browser, delegate);

  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::TOP_LEFT);
  // TODO(astra): Adjust arrow position based on actual anchor point.
  //   Chromium owner: TabSearchButton (chrome/browser/ui/views/tab_search/)

  widget->Show();
  bubble->RequestSearchFocus();

  if (delegate) {
    delegate->OnBubbleOpened();
  }

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabSearchBubble::AstraTabSearchBubble(views::View* anchor_view,
                                           Browser* browser,
                                           Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_LEFT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser),
      delegate_(delegate),
      bubble_width_(kDefaultBubbleWidth),
      bubble_max_height_(kDefaultBubbleMaxHeight) {
  // Set bubble properties.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(false);
  SetShowCloseButton(false);
  set_fixed_width(bubble_width_);

  // Auto-dismiss when the widget loses activation.
  set_close_on_deactivate(true);

  // Register as a model observer.
  model_.AddObserver(this);

  // Populate model from TabStripModel if available.
  // TODO(astra): Set up TabStripModelObserver for live updates.
  //   Chromium component: TabStripModelObserver
  if (browser_) {
    RefreshModelFromTabStrip();
  }
}

AstraTabSearchBubble::~AstraTabSearchBubble() {
  // Unregister as model observer.
  model_.RemoveObserver(this);

  // Notify the delegate that the bubble is closing.
  if (delegate_) {
    delegate_->OnBubbleClosed();
  }
}

// =========================================================================
// Bubble visibility
// =========================================================================

void AstraTabSearchBubble::Show(gfx::NativeView parent,
                                const gfx::Rect& anchor_rect) {
  views::Widget* widget = GetWidget();
  if (!widget) {
    // TODO(astra): Create widget from native view parent if needed.
    //   For now, the widget is created by the static ShowBubble factory.
    return;
  }
  widget->Show();
  RequestSearchFocus();

  if (delegate_) {
    delegate_->OnBubbleOpened();
  }
}

void AstraTabSearchBubble::Hide() {
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

bool AstraTabSearchBubble::IsVisible() const {
  const views::Widget* widget = GetWidget();
  return widget && widget->IsVisible();
}

views::View* AstraTabSearchBubble::GetBubbleView() {
  return this;
}

// =========================================================================
// Query management
// =========================================================================

void AstraTabSearchBubble::SetQuery(const std::u16string& query) {
  if (search_field_) {
    search_field_->SetText(query);
    // ContentsChanged will be called automatically by Textfield.
  }
}

std::u16string AstraTabSearchBubble::GetQuery() const {
  if (search_field_) {
    return search_field_->GetText();
  }
  return std::u16string();
}

// =========================================================================
// Selection navigation
// =========================================================================

void AstraTabSearchBubble::SelectNext() {
  MoveSelection(1);
}

void AstraTabSearchBubble::SelectPrevious() {
  MoveSelection(-1);
}

void AstraTabSearchBubble::SelectFirst() {
  if (GetResultCount() == 0) {
    return;
  }
  selected_index_ = 0;
  UpdateSelectionVisual();
  ScrollSelectedIntoView();
  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::SelectLast() {
  size_t count = GetResultCount();
  if (count == 0) {
    return;
  }
  selected_index_ = count - 1;
  UpdateSelectionVisual();
  ScrollSelectedIntoView();
  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::ActivateSelected() {
  ActivateSelectedResult();
}

// =========================================================================
// Search mode
// =========================================================================

void AstraTabSearchBubble::SetSearchMode(AstraTabSearchMode mode) {
  model_.SetSearchMode(mode);
  // UpdateResults will be triggered via observer notification.
}

AstraTabSearchMode AstraTabSearchBubble::GetSearchMode() const {
  return model_.GetSearchMode();
}

// =========================================================================
// Bubble sizing
// =========================================================================

void AstraTabSearchBubble::SetBubbleWidth(int width) {
  bubble_width_ = width;
  set_fixed_width(width);
  if (GetWidget()) {
    GetWidget()->SetSize(gfx::Size(width, height()));
  }
  // Re-layout children.
  if (search_field_) {
    search_field_->SetPreferredSize(gfx::Size(width, kSearchFieldHeight));
  }
  InvalidateLayout();
}

void AstraTabSearchBubble::SetBubbleHeight(int height) {
  bubble_max_height_ = height;
  if (scroll_view_) {
    scroll_view_->ClipHeightTo(0, height - kSearchFieldHeight -
                                      kResultCountHeight - kDividerThickness -
                                      kBubblePadding * 2);
  }
  if (GetWidget()) {
    gfx::Size size = GetWidget()->GetWindowBoundsInScreen().size();
    size.set_height(height);
    GetWidget()->SetSize(size);
  }
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraTabSearchBubble::Init() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);

  SetLayoutManager(std::make_unique<views::FillLayout>());

  BuildLayout();
}

void AstraTabSearchBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
}

void AstraTabSearchBubble::OnWidgetActivationChanged(views::Widget* widget,
                                                     bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);
  // Auto-closes due to set_close_on_deactivate(true).
}

// =========================================================================
// Focus management
// =========================================================================

void AstraTabSearchBubble::RequestSearchFocus() {
  if (search_field_) {
    search_field_->RequestFocus();
    search_field_->SelectAll(false);
  }
}

// =========================================================================
// Layout construction
// =========================================================================

void AstraTabSearchBubble::BuildLayout() {
  // Container view with vertical box layout.
  auto* container = AddChildView(std::make_unique<views::View>());
  auto* layout = container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(kBubblePadding, kBubblePadding), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Search textfield.
  search_field_ = container->AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search tabs, bookmarks, history…");
  search_field_->SetBackgroundColor(kSearchFieldBackground);
  search_field_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_field_->set_controller(this);
  search_field_->SetPreferredSize(
      gfx::Size(bubble_width_, kSearchFieldHeight));

  // 2. Result count label.
  result_count_label_ =
      container->AddChildView(std::make_unique<views::Label>(std::u16string()));
  result_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  result_count_label_->SetAutoColorReadabilityEnabled(false);
  result_count_label_->SetFontList(
      views::Label::GetDefaultFontList().Derive(
          -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  result_count_label_->SetPreferredSize(
      gfx::Size(bubble_width_, kResultCountHeight));

  // Divider between search area and results.
  auto* divider = container->AddChildView(std::make_unique<views::View>());
  divider->SetPaintToLayer();
  divider->layer()->SetFillsBoundsOpaquely(true);
  divider->SetPreferredSize(gfx::Size(0, kDividerThickness));

  // 3. Scrollable results area.
  scroll_view_ = container->AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(kBubbleBackground);
  scroll_view_->ClipHeightTo(0, kResultsMaxHeight);
  layout->SetFlexForView(scroll_view_, 1);

  // Results container inside the scroll view.
  results_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  results_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  results_container_->SetPaintToLayer();
  results_container_->layer()->SetFillsBoundsOpaquely(false);

  // Populate with initial results.
  UpdateResults();
}

// =========================================================================
// Results UI — update
// =========================================================================

void AstraTabSearchBubble::UpdateResults() {
  // Get current query.
  std::u16string query;
  if (search_field_) {
    query = search_field_->GetText();
  }

  // Run search through the model.
  results_ = model_.SearchTabs(query);

  // Clear existing items.
  if (results_container_) {
    results_container_->RemoveAllChildViews();
  }

  if (results_.empty()) {
    selected_index_ = 0;
    UpdateResultCountLabel();
    InvalidateLayout();
    return;
  }

  // Build the results list with group headers.
  // TODO(astra): Add proper group categorization (open tabs, recently closed,
  //   bookmarks, workspace groups).  For now, all results are "open tabs".
  AstraTabSearchItemView::Group current_group =
      static_cast<AstraTabSearchItemView::Group>(-1);
  int display_index = 0;

  // For now, treat all results as "Open Tabs" group.
  // TODO(astra): Properly categorize results by source group.
  size_t open_tabs_count = results_.size();
  size_t recently_closed_count = 0;
  size_t bookmarks_count = 0;

  // Add group header and items.
  for (size_t i = 0; i < results_.size(); ++i) {
    const auto& item = results_[i];

    // Determine the group.
    // TODO(astra): Use proper group categorization from the model.
    AstraTabSearchItemView::Group group =
        AstraTabSearchItemView::Group::kOpenTabs;

    // Add group header when group changes.
    if (group != current_group) {
      current_group = group;
      size_t count = 0;
      switch (group) {
        case AstraTabSearchItemView::Group::kOpenTabs:
          count = open_tabs_count;
          break;
        case AstraTabSearchItemView::Group::kRecentlyClosed:
          count = recently_closed_count;
          break;
        case AstraTabSearchItemView::Group::kBookmarks:
          count = bookmarks_count;
          break;
      }
      AddGroupHeader(group, count);
    }

    ++display_index;
    AddResultItem(item, display_index);
  }

  // Reset selection to first item.
  selected_index_ = 0;
  UpdateSelectionVisual();
  UpdateResultCountLabel();
  InvalidateLayout();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnResultCountChanged(results_.size());
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::AddGroupHeader(
    AstraTabSearchItemView::Group group,
    size_t count) {
  if (!results_container_) {
    return;
  }

  auto* header = results_container_->AddChildView(
      std::make_unique<views::Label>(GetGroupLabel(group, count)));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(views::Label::GetDefaultFontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));
  header->SetPreferredSize(gfx::Size(bubble_width_, kGroupHeaderHeight));
  header->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, 12)));

  const auto* color_provider = GetColorProvider();
  if (color_provider) {
    header->SetEnabledColor(color_provider->GetColor(kGroupHeaderText));
  }

  header->SetPaintToLayer();
  header->layer()->SetFillsBoundsOpaquely(false);
}

AstraTabSearchItemView* AstraTabSearchBubble::AddResultItem(
    const AstraTabSearchItem& item,
    int display_index) {
  if (!results_container_) {
    return nullptr;
  }

  auto* item_view = results_container_->AddChildView(
      std::make_unique<AstraTabSearchItemView>(item));
  item_view->SetDisplayIndex(display_index);

  // Click handler — activate the result and close the bubble.
  item_view->SetActivatedCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, size_t index) {
        if (!weak_this) {
          return;
        }
        weak_this->ActivateResultAt(index);
      },
      weak_ptr_factory_.GetWeakPtr(),
      results_.size()));  // Will be set below

  // We can't easily know the index during construction — fix this later.
  // For now, use tab_index from the item for activation.
  item_view->SetActivatedCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, int tab_idx) {
        if (!weak_this) {
          return;
        }
        // Find the result index for this tab.
        for (size_t i = 0; i < weak_this->results_.size(); ++i) {
          if (weak_this->results_[i].tab_index == tab_idx) {
            weak_this->ActivateResultAt(i);
            break;
          }
        }
      },
      weak_ptr_factory_.GetWeakPtr(), item.tab_index));

  // Close button handler.
  item_view->SetCloseCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, int tab_idx) {
        if (!weak_this) {
          return;
        }
        // Find the result index.
        for (size_t i = 0; i < weak_this->results_.size(); ++i) {
          if (weak_this->results_[i].tab_index == tab_idx) {
            weak_this->CloseTabAtResultIndex(i);
            break;
          }
        }
      },
      weak_ptr_factory_.GetWeakPtr(), item.tab_index));

  return item_view;
}

// =========================================================================
// Selection handling
// =========================================================================

void AstraTabSearchBubble::MoveSelection(int delta) {
  size_t count = GetResultCount();
  if (count == 0) {
    return;
  }

  int new_index = static_cast<int>(selected_index_) + delta;
  new_index = std::max(0, std::min(static_cast<int>(count) - 1, new_index));
  selected_index_ = static_cast<size_t>(new_index);

  UpdateSelectionVisual();
  ScrollSelectedIntoView();

  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::MoveToNextGroup() {
  if (results_.empty()) {
    return;
  }

  AstraTabSearchItemView::Group current_group = GetGroupAt(selected_index_);

  // Find the first result in the next group.
  for (size_t i = selected_index_ + 1; i < results_.size(); ++i) {
    // TODO(astra): Proper group detection once we have mixed result groups.
    if (GetGroupAt(i) != current_group) {
      selected_index_ = i;
      UpdateSelectionVisual();
      ScrollSelectedIntoView();
      if (delegate_) {
        delegate_->OnSelectionChanged(selected_index_);
      }
      return;
    }
  }

  // Already in the last group — go to last item.
  selected_index_ = results_.size() - 1;
  UpdateSelectionVisual();
  ScrollSelectedIntoView();
  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::MoveToPreviousGroup() {
  if (results_.empty()) {
    return;
  }

  AstraTabSearchItemView::Group current_group = GetGroupAt(selected_index_);

  // Find the last result in the previous group.
  for (int i = static_cast<int>(selected_index_) - 1; i >= 0; --i) {
    if (GetGroupAt(static_cast<size_t>(i)) != current_group) {
      // Go to the first item of the previous group.
      size_t group_start = static_cast<size_t>(i);
      while (group_start > 0 &&
             GetGroupAt(group_start - 1) ==
                 GetGroupAt(static_cast<size_t>(i))) {
        --group_start;
      }
      selected_index_ = group_start;
      UpdateSelectionVisual();
      ScrollSelectedIntoView();
      if (delegate_) {
        delegate_->OnSelectionChanged(selected_index_);
      }
      return;
    }
  }

  // Already in the first group — go to first item.
  selected_index_ = 0;
  UpdateSelectionVisual();
  ScrollSelectedIntoView();
  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::ActivateSelectedResult() {
  if (selected_index_ >= results_.size()) {
    return;
  }
  ActivateResultAt(selected_index_);
}

void AstraTabSearchBubble::ActivateResultAt(size_t index) {
  if (index >= results_.size()) {
    return;
  }

  const AstraTabSearchItem& item = results_[index];

  // Switch to the tab via the model (which delegates to Chromium).
  model_.SwitchToTab(item.tab_index);

  // Notify delegate.
  TabStripModel* tab_strip = GetTabStripModel();
  if (tab_strip && item.tab_index >= 0 &&
      item.tab_index < tab_strip->count()) {
    content::WebContents* contents =
        tab_strip->GetWebContentsAt(item.tab_index);
    if (contents && delegate_) {
      delegate_->OnTabActivated(contents);
    }
  }

  // Close bubble on activate (if setting is enabled).
  if (model_.close_tab_on_activate() && GetWidget()) {
    GetWidget()->Close();
  }
}

void AstraTabSearchBubble::CloseTabAtResultIndex(size_t result_index) {
  if (result_index >= results_.size()) {
    return;
  }

  const AstraTabSearchItem& item = results_[result_index];

  // Close via model.
  model_.CloseTab(item.tab_index);

  // Notify delegate.
  TabStripModel* tab_strip = GetTabStripModel();
  if (tab_strip && item.tab_index >= 0 &&
      item.tab_index < tab_strip->count()) {
    content::WebContents* contents =
        tab_strip->GetWebContentsAt(item.tab_index);
    if (contents && delegate_) {
      delegate_->OnTabClosed(contents);
      delegate_->OnTabCloseRequested(contents);
    }
  }

  // Refresh results.
  UpdateResults();
}

size_t AstraTabSearchBubble::GetResultCount() const {
  return results_.size();
}

void AstraTabSearchBubble::UpdateSelectionVisual() {
  if (!results_container_) {
    return;
  }

  // Iterate through item views and update selection state.
  // We need to skip group header labels.
  size_t result_idx = 0;
  for (views::View* child : results_container_->children()) {
    auto* item_view = dynamic_cast<AstraTabSearchItemView*>(child);
    if (!item_view) {
      continue;  // Skip group headers.
    }
    item_view->SetSelected(result_idx == selected_index_);
    ++result_idx;
  }
}

void AstraTabSearchBubble::UpdateResultCountLabel() {
  if (!result_count_label_) {
    return;
  }

  size_t total = results_.size();
  if (total == 0) {
    result_count_label_->SetText(u"No results");
  } else if (total == 1) {
    result_count_label_->SetText(u"1 result");
  } else {
    result_count_label_->SetText(
        base::ASCIIToUTF16(base::NumberToString(total)) + u" results");
  }

  const auto* color_provider = GetColorProvider();
  if (color_provider) {
    result_count_label_->SetEnabledColor(
        color_provider->GetColor(kResultCountText));
  }
}

void AstraTabSearchBubble::ScrollSelectedIntoView() {
  if (!scroll_view_ || !results_container_) {
    return;
  }

  AstraTabSearchItemView* selected_item = GetItemViewAt(selected_index_);
  if (!selected_item) {
    return;
  }

  scroll_view_->ScrollViewToVisible(selected_item);
}

AstraTabSearchItemView* AstraTabSearchBubble::GetItemViewAt(
    size_t index) const {
  if (!results_container_ || index >= results_.size()) {
    return nullptr;
  }

  // Skip group headers to find the item view at the given result index.
  size_t result_idx = 0;
  for (views::View* child : results_container_->children()) {
    auto* item_view = dynamic_cast<AstraTabSearchItemView*>(child);
    if (!item_view) {
      continue;
    }
    if (result_idx == index) {
      return item_view;
    }
    ++result_idx;
  }

  return nullptr;
}

AstraTabSearchItemView::Group AstraTabSearchBubble::GetGroupAt(
    size_t index) const {
  if (index >= results_.size()) {
    return AstraTabSearchItemView::Group::kOpenTabs;
  }
  // TODO(astra): Return actual group once results have mixed groups.
  return AstraTabSearchItemView::Group::kOpenTabs;
}

// =========================================================================
// TabStripModel access
// =========================================================================

TabStripModel* AstraTabSearchBubble::GetTabStripModel() const {
  if (!browser_) {
    return nullptr;
  }
  return browser_->tab_strip_model();
}

// =========================================================================
// Model refresh from TabStripModel
// =========================================================================

void AstraTabSearchBubble::RefreshModelFromTabStrip() {
  TabStripModel* tab_strip = GetTabStripModel();
  if (!tab_strip) {
    return;
  }

  std::vector<AstraTabSearchItem> tabs;
  int count = tab_strip->count();
  int active_index = tab_strip->active_index();

  for (int i = 0; i < count; ++i) {
    content::WebContents* contents = tab_strip->GetWebContentsAt(i);
    if (!contents) {
      continue;
    }

    AstraTabSearchItem item;
    item.tab_id = i;  // Use index as ID for now.
    item.title = GetTabTitle(contents);
    item.url = contents->GetURL();
    item.hostname = GetDisplayHost(contents);
    item.is_active = (i == active_index);
    item.is_pinned = tab_strip->IsTabPinned(i);
    item.tab_index = i;
    item.window_id = 0;  // Single window for now.
    item.is_audible = contents->IsAudioMuted() || contents->WasRecentlyAudible();
    item.is_muted = contents->IsAudioMuted();
    item.is_loading = contents->IsLoading();
    item.has_crashed = contents->IsCrashed();
    item.last_active_time = base::Time::Now() -
                            base::Minutes(i);  // Approximate recency.
    item.relevance_score = 0.0;

    tabs.push_back(std::move(item));
  }

  model_.SetTabList(std::move(tabs));
}

// =========================================================================
// Helpers
// =========================================================================

std::u16string AstraTabSearchBubble::GetGroupLabel(
    AstraTabSearchItemView::Group group,
    size_t count) {
  switch (group) {
    case AstraTabSearchItemView::Group::kOpenTabs:
      return count == 1 ? u"Open Tab" : u"Open Tabs";
    case AstraTabSearchItemView::Group::kRecentlyClosed:
      return u"Recently Closed";
    case AstraTabSearchItemView::Group::kBookmarks:
      return u"Bookmarks";
  }
  return std::u16string();
}

// =========================================================================
// TextfieldController
// =========================================================================

void AstraTabSearchBubble::ContentsChanged(views::Textfield* sender,
                                           const std::u16string& new_contents) {
  DCHECK_EQ(sender, search_field_);
  selected_index_ = 0;
  UpdateResults();

  if (delegate_) {
    delegate_->OnSearchTextChanged(new_contents);
  }
}

bool AstraTabSearchBubble::HandleKeyEvent(views::Textfield* sender,
                                          const ui::KeyEvent& key_event) {
  DCHECK_EQ(sender, search_field_);

  // Only handle key press events.
  if (key_event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  // Ctrl+1..9 shortcuts.
  if (key_event.IsControlDown() && !key_event.IsShiftDown() &&
      !key_event.IsAltDown() && !key_event.IsCommandDown()) {
    ui::KeyboardCode code = key_event.key_code();
    if (code >= ui::VKEY_1 && code <= ui::VKEY_9) {
      int digit = code - ui::VKEY_1 + 1;
      size_t index = static_cast<size_t>(digit - 1);
      if (index < GetResultCount()) {
        ActivateResultAt(index);
        return true;
      }
    }
  }

  // Tab / Shift+Tab to navigate between groups.
  if (key_event.key_code() == ui::VKEY_TAB) {
    if (key_event.IsShiftDown()) {
      MoveToPreviousGroup();
    } else {
      MoveToNextGroup();
    }
    return true;
  }

  switch (key_event.key_code()) {
    case ui::VKEY_UP:
      MoveSelection(-1);
      return true;

    case ui::VKEY_DOWN:
      MoveSelection(1);
      return true;

    case ui::VKEY_RETURN:
      ActivateSelectedResult();
      return true;

    case ui::VKEY_ESCAPE:
      if (GetWidget()) {
        GetWidget()->Close();
      }
      return true;

    default:
      return false;
  }
}

// =========================================================================
// AstraTabSearchObserver
// =========================================================================

void AstraTabSearchBubble::OnTabListChanged(AstraTabSearchModel* /*model*/) {
  UpdateResults();
}

void AstraTabSearchBubble::OnSearchResultsChanged(
    AstraTabSearchModel* /*model*/) {
  UpdateResults();
}

void AstraTabSearchBubble::OnSearchModeChanged(AstraTabSearchModel* /*model*/,
                                              AstraTabSearchMode mode) {
  if (delegate_) {
    delegate_->OnSearchModeChanged(mode);
  }
}

void AstraTabSearchBubble::OnTabSearchModelShutdown(
    AstraTabSearchModel* /*model*/) {
  // Model is shutting down — clean up references.
}

}  // namespace astra
