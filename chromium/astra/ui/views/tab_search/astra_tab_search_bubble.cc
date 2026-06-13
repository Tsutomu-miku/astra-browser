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
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
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
constexpr int kSearchRowPaddingHorizontal = 12;
constexpr int kSearchIconSize = 16;
constexpr int kSearchIconSpacing = 8;
constexpr int kCategoryRowHeight = 32;
constexpr int kCategorySpacing = 8;
constexpr int kCategoryPaddingHorizontal = 12;
constexpr int kResultCountHeight = 28;
constexpr int kResultsMaxHeight = 380;
constexpr int kDividerThickness = 1;
constexpr int kGroupHeaderHeight = 32;
constexpr int kBubblePadding = 8;
constexpr int kEmptyStateVerticalPadding = 32;
constexpr int kNoResultsVerticalPadding = 32;

// Color IDs.
// TODO(astra): Define dedicated tab-search color IDs in astra_color_ids.h.
constexpr ui::ColorId kBubbleBackground = kColorAstraCommandPaletteBackground;
constexpr ui::ColorId kBubbleBorder = kColorAstraCommandPaletteBorder;
constexpr ui::ColorId kSearchFieldBackground = ui::kColorTextFieldBackground;
constexpr ui::ColorId kSearchFieldText = kColorAstraCommandPaletteSearchText;
constexpr ui::ColorId kSearchIconColor = ui::kColorIcon;
constexpr ui::ColorId kDividerColor = ui::kColorSeparator;
constexpr ui::ColorId kResultCountText =
    kColorAstraCommandPaletteDescriptionText;
constexpr ui::ColorId kGroupHeaderText =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kGroupHeaderBackground =
    kColorAstraCommandPaletteBackground;
constexpr ui::ColorId kCategoryTextColor =
    kColorAstraCommandPaletteDescriptionText;
constexpr ui::ColorId kCategoryActiveTextColor = kColorAstraCommandPaletteText;
constexpr ui::ColorId kEmptyStateTextColor =
    kColorAstraCommandPaletteDescriptionText;

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

  // Set accessible name for the bubble.
  SetAccessibleName(u"Tab search");
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
  } else {
    // If search field doesn't exist yet, set directly on model.
    model_.SetQuery(query);
  }
}

std::u16string AstraTabSearchBubble::GetQuery() const {
  if (search_field_) {
    return search_field_->GetText();
  }
  return model_.query();
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
  size_t count = GetResultCount();
  if (count == 0) {
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
// Search mode / filter
// =========================================================================

void AstraTabSearchBubble::SetSearchMode(AstraTabSearchMode mode) {
  model_.SetSearchMode(mode);
  // UpdateResults will be triggered via observer notification.
}

AstraTabSearchMode AstraTabSearchBubble::GetSearchMode() const {
  return model_.GetSearchMode();
}

void AstraTabSearchBubble::SetFilter(AstraTabSearchFilter filter) {
  model_.SetFilter(filter);
  // UpdateCategoryTogglesFromFilter will be called via observer.
}

AstraTabSearchFilter AstraTabSearchBubble::GetFilter() const {
  return model_.GetFilter();
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
                                      kCategoryRowHeight -
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

  // 1. Search row: search icon + text field.
  search_row_ = container->AddChildView(std::make_unique<views::View>());
  auto* search_row_layout = search_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kSearchRowPaddingHorizontal),
          kSearchIconSpacing));
  search_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  search_row_->SetPreferredSize(gfx::Size(bubble_width_, kSearchFieldHeight));

  // Search icon.
  // TODO(astra): Use a proper search vector icon from Chromium.
  //   Chromium component: ui/views/vector_icons/
  search_icon_ = search_row_->AddChildView(
      std::make_unique<views::ImageView>());
  search_icon_->SetPreferredSize(
      gfx::Size(kSearchIconSize, kSearchIconSize));
  search_icon_->SetImage(
      ui::ImageModel::FromVectorIcon(gfx::VectorIcon(), kSearchIconColor));
  // TODO(astra): Replace with actual search vector icon.

  // Search textfield.
  search_field_ =
      search_row_->AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search tabs, bookmarks, history…");
  search_field_->SetBackgroundColor(kSearchFieldBackground);
  search_field_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_field_->set_controller(this);
  search_field_->SetAccessibleName(u"Search tabs");
  search_field_->SetAccessibleDescription(
      u"Type to search open tabs, bookmarks, and browsing history. Use arrow keys to navigate results.");
  search_row_layout->SetFlexForView(search_field_, 1);

  // 2. Category toggle row.
  category_row_ = container->AddChildView(std::make_unique<views::View>());
  BuildCategoryToggles(category_row_);

  // 3. Result count label.
  result_count_label_ =
      container->AddChildView(std::make_unique<views::Label>(std::u16string()));
  result_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  result_count_label_->SetAutoColorReadabilityEnabled(false);
  result_count_label_->SetFontList(
      views::Label::GetDefaultFontList().Derive(
          -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  result_count_label_->SetPreferredSize(
      gfx::Size(bubble_width_, kResultCountHeight));
  result_count_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kCategoryPaddingHorizontal)));
  result_count_label_->SetAccessibleName(u"Result count");

  // Divider between search area and results.
  auto* divider = container->AddChildView(std::make_unique<views::View>());
  divider->SetPaintToLayer();
  divider->layer()->SetFillsBoundsOpaquely(true);
  divider->SetPreferredSize(gfx::Size(0, kDividerThickness));

  // 4. Scrollable results area.
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

  // Set accessible role for results container.
  results_container_->SetAccessibleRole(ax::mojom::Role::kListBox);
  results_container_->SetAccessibleName(u"Search results");

  // Populate with initial results.
  UpdateResults();
}

void AstraTabSearchBubble::BuildCategoryToggles(views::View* container) {
  auto* layout = container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kCategoryPaddingHorizontal),
          kCategorySpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  container->SetPreferredSize(
      gfx::Size(bubble_width_, kCategoryRowHeight));

  // Tabs toggle.
  tabs_toggle_ = container->AddChildView(
      std::make_unique<views::ToggleButton>());
  tabs_toggle_->SetText(u"Tabs");
  tabs_toggle_->SetIsOn(true);
  tabs_toggle_->SetAccessibleName(u"Filter tabs");
  tabs_toggle_->SetCallback(base::BindRepeating(
      &AstraTabSearchBubble::OnCategoryToggleClicked,
      weak_ptr_factory_.GetWeakPtr(),
      AstraTabSearchFilter::kTabs));

  // Bookmarks toggle.
  bookmarks_toggle_ = container->AddChildView(
      std::make_unique<views::ToggleButton>());
  bookmarks_toggle_->SetText(u"Bookmarks");
  bookmarks_toggle_->SetIsOn(true);
  bookmarks_toggle_->SetAccessibleName(u"Filter bookmarks");
  bookmarks_toggle_->SetCallback(base::BindRepeating(
      &AstraTabSearchBubble::OnCategoryToggleClicked,
      weak_ptr_factory_.GetWeakPtr(),
      AstraTabSearchFilter::kBookmarks));

  // History toggle.
  history_toggle_ = container->AddChildView(
      std::make_unique<views::ToggleButton>());
  history_toggle_->SetText(u"History");
  history_toggle_->SetIsOn(true);
  history_toggle_->SetAccessibleName(u"Filter history");
  history_toggle_->SetCallback(base::BindRepeating(
      &AstraTabSearchBubble::OnCategoryToggleClicked,
      weak_ptr_factory_.GetWeakPtr(),
      AstraTabSearchFilter::kHistory));

  // Recently closed toggle.
  recent_toggle_ = container->AddChildView(
      std::make_unique<views::ToggleButton>());
  recent_toggle_->SetText(u"Recent");
  recent_toggle_->SetIsOn(true);
  recent_toggle_->SetAccessibleName(u"Filter recently closed tabs");
  recent_toggle_->SetCallback(base::BindRepeating(
      &AstraTabSearchBubble::OnCategoryToggleClicked,
      weak_ptr_factory_.GetWeakPtr(),
      AstraTabSearchFilter::kRecentlyClosed));

  UpdateCategoryTogglesFromFilter();
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

  // Reset state flags.
  showing_empty_state_ = false;
  showing_no_results_ = false;

  // Handle empty state (no query, no recent searches, no results).
  if (query.empty() && results_.empty() &&
      model_.GetRecentSearches().empty()) {
    ShowEmptyState();
    selected_index_ = 0;
    UpdateResultCountLabel();
    InvalidateLayout();
    return;
  }

  // Handle no results state (query present but no matches).
  if (!query.empty() && results_.empty()) {
    ShowNoResultsState();
    selected_index_ = 0;
    UpdateResultCountLabel();
    InvalidateLayout();
    return;
  }

  // Build the results list with group headers.
  size_t display_index = 0;
  AstraTabSearchResultType current_type =
      static_cast<AstraTabSearchResultType>(-1);

  // Group results by type.
  std::vector<AstraTabSearchItem> open_tabs;
  std::vector<AstraTabSearchItem> recently_closed;
  std::vector<AstraTabSearchItem> bookmarks;
  std::vector<AstraTabSearchItem> history;

  for (const auto& item : results_) {
    switch (item.result_type) {
      case AstraTabSearchResultType::kOpenTab:
        open_tabs.push_back(item);
        break;
      case AstraTabSearchResultType::kRecentlyClosed:
        recently_closed.push_back(item);
        break;
      case AstraTabSearchResultType::kBookmark:
        bookmarks.push_back(item);
        break;
      case AstraTabSearchResultType::kHistory:
        history.push_back(item);
        break;
      default:
        break;
    }
  }

  // Helper lambda to add a group with items.
  auto add_group = [&](AstraTabSearchResultType type,
                       const std::vector<AstraTabSearchItem>& items) {
    if (items.empty()) {
      return;
    }
    std::u16string label = GetResultTypeLabel(type);
    AddGroupHeader(type, label, items.size());
    for (const auto& item : items) {
      ++display_index;
      AddResultItem(item, static_cast<int>(display_index),
                    display_index - 1);
    }
  };

  // Add groups in priority order.
  add_group(AstraTabSearchResultType::kOpenTab, open_tabs);
  add_group(AstraTabSearchResultType::kRecentlyClosed, recently_closed);
  add_group(AstraTabSearchResultType::kBookmark, bookmarks);
  add_group(AstraTabSearchResultType::kHistory, history);

  // Add recent searches section when query is empty.
  if (query.empty() && model_.show_recent_searches()) {
    const auto& recent = model_.GetRecentSearches();
    if (!recent.empty()) {
      AddGroupHeader(AstraTabSearchResultType::kSearchHistory,
                     u"Recent Searches", recent.size());
      int recent_index = 0;
      for (const auto& entry : recent) {
        AddRecentSearchItem(entry, recent_index + 1);
        ++recent_index;
      }
    }
  }

  // Reset selection to first item.
  size_t total = GetResultCount();
  if (total > 0) {
    selected_index_ = 0;
  } else {
    selected_index_ = 0;
  }

  UpdateSelectionVisual();
  UpdateResultCountLabel();
  InvalidateLayout();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnResultCountChanged(results_.size());
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::AddGroupHeader(AstraTabSearchResultType type,
                                          const std::u16string& title,
                                          size_t count) {
  if (!results_container_) {
    return;
  }

  auto* header = results_container_->AddChildView(
      std::make_unique<AstraTabSearchGroupHeaderView>(title, count));
  header->SetGroupType(type);
}

AstraTabSearchItemView* AstraTabSearchBubble::AddResultItem(
    const AstraTabSearchItem& item,
    int display_index,
    size_t result_index) {
  if (!results_container_) {
    return nullptr;
  }

  auto* item_view = results_container_->AddChildView(
      std::make_unique<AstraTabSearchItemView>(item));
  item_view->SetDisplayIndex(display_index);

  // Set matches for highlighting.
  if (search_field_) {
    auto matches = model_.ComputeMatches(item, search_field_->GetText());
    item_view->SetMatches(matches);
  }

  // Click handler — activate the result and close the bubble.
  item_view->SetActivatedCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, size_t idx) {
        if (!weak_this) {
          return;
        }
        weak_this->ActivateResultAt(idx);
      },
      weak_ptr_factory_.GetWeakPtr(), result_index));

  // Close button handler.
  item_view->SetCloseCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, size_t idx) {
        if (!weak_this) {
          return;
        }
        weak_this->CloseTabAtResultIndex(idx);
      },
      weak_ptr_factory_.GetWeakPtr(), result_index));

  // Middle click handler = close.
  item_view->SetMiddleClickCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this, size_t idx) {
        if (!weak_this) {
          return;
        }
        weak_this->CloseTabAtResultIndex(idx);
      },
      weak_ptr_factory_.GetWeakPtr(), result_index));

  return item_view;
}

void AstraTabSearchBubble::AddRecentSearchItem(
    const AstraTabSearchRecentSearch& entry,
    int display_index) {
  if (!results_container_) {
    return;
  }

  // Create an item view that represents a recent search.
  AstraTabSearchItem item;
  item.result_type = AstraTabSearchResultType::kSearchHistory;
  item.item_id = display_index;
  item.title = entry.query;
  item.hostname = base::NumberToString16(entry.visit_count) + u" visits";
  item.last_visited_time = entry.timestamp;
  item.visit_count = entry.visit_count;

  auto* item_view = results_container_->AddChildView(
      std::make_unique<AstraTabSearchItemView>(item));
  item_view->SetDisplayIndex(0);  // No shortcut hint for recent searches.
  item_view->ShowShortcutHint(false);
  item_view->ShowCloseButton(true);  // Always show close to remove from history.
  item_view->ShowFavicon(false);    // No favicon for search history.

  // Click handler — fill the query with this recent search.
  item_view->SetActivatedCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this,
         const std::u16string& query_text) {
        if (!weak_this) {
          return;
        }
        weak_this->SetQuery(query_text);
        if (weak_this->search_field_) {
          weak_this->search_field_->SelectAll(false);
        }
        if (weak_this->delegate_) {
          weak_this->delegate_->OnRecentSearchSelected(query_text);
        }
      },
      weak_ptr_factory_.GetWeakPtr(), entry.query));

  // Close handler — remove this recent search.
  item_view->SetCloseCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchBubble> weak_this,
         const std::u16string& query_text) {
        if (!weak_this) {
          return;
        }
        weak_this->model_.RemoveRecentSearch(query_text);
      },
      weak_ptr_factory_.GetWeakPtr(), entry.query));
}

void AstraTabSearchBubble::ShowEmptyState() {
  showing_empty_state_ = true;

  if (!results_container_) {
    return;
  }

  auto* empty_state = results_container_->AddChildView(
      std::make_unique<views::View>());
  auto* layout = empty_state->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(kEmptyStateVerticalPadding, 0),
          8));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  empty_state->SetPaintToLayer();
  empty_state->layer()->SetFillsBoundsOpaquely(false);

  // Empty state icon placeholder.
  // TODO(astra): Use a proper empty state icon.
  auto* icon_view = empty_state->AddChildView(
      std::make_unique<views::View>());
  icon_view->SetPreferredSize(gfx::Size(32, 32));
  icon_view->SetPaintToLayer();
  icon_view->layer()->SetFillsBoundsOpaquely(true);
  icon_view->layer()->SetColor(SK_ColorTRANSPARENT);

  // Empty state text.
  auto* text_label = empty_state->AddChildView(
      std::make_unique<views::Label>(u"Start typing to search"));
  text_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  text_label->SetAutoColorReadabilityEnabled(false);
  text_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  auto* hint_label = empty_state->AddChildView(
      std::make_unique<views::Label>(
          u"Search open tabs, bookmarks, and history"));
  hint_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  hint_label->SetAutoColorReadabilityEnabled(false);
  hint_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      -2, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  empty_state_view_ = empty_state;

  // Update colors.
  const auto* color_provider = GetColorProvider();
  if (color_provider) {
    text_label->SetEnabledColor(
        color_provider->GetColor(kEmptyStateTextColor));
    hint_label->SetEnabledColor(
        color_provider->GetColor(kEmptyStateTextColor));
  }

  // Accessibility.
  empty_state->SetAccessibleName(u"No search results yet");
  empty_state->SetAccessibleDescription(
      u"Start typing to search open tabs, bookmarks, and browsing history.");
}

void AstraTabSearchBubble::ShowNoResultsState() {
  showing_no_results_ = true;

  if (!results_container_) {
    return;
  }

  auto* no_results = results_container_->AddChildView(
      std::make_unique<views::View>());
  auto* layout = no_results->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(kNoResultsVerticalPadding, 0),
          8));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  no_results->SetPaintToLayer();
  no_results->layer()->SetFillsBoundsOpaquely(false);

  // "No results" icon placeholder.
  auto* icon_view = no_results->AddChildView(
      std::make_unique<views::View>());
  icon_view->SetPreferredSize(gfx::Size(32, 32));
  icon_view->SetPaintToLayer();
  icon_view->layer()->SetFillsBoundsOpaquely(true);
  icon_view->layer()->SetColor(SK_ColorTRANSPARENT);

  // "No results" text.
  auto* text_label = no_results->AddChildView(
      std::make_unique<views::Label>(u"No matching results"));
  text_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  text_label->SetAutoColorReadabilityEnabled(false);
  text_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));

  auto* hint_label = no_results->AddChildView(
      std::make_unique<views::Label>(
          u"Try different keywords or check your filters"));
  hint_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  hint_label->SetAutoColorReadabilityEnabled(false);
  hint_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      -2, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  no_results_view_ = no_results;

  // Update colors.
  const auto* color_provider = GetColorProvider();
  if (color_provider) {
    text_label->SetEnabledColor(
        color_provider->GetColor(kEmptyStateTextColor));
    hint_label->SetEnabledColor(
        color_provider->GetColor(kEmptyStateTextColor));
  }

  // Accessibility.
  no_results->SetAccessibleName(u"No matching results found");
  no_results->SetAccessibleDescription(
      u"Your search query did not match any tabs, bookmarks, or history items.");
}

// =========================================================================
// Selection handling
// =========================================================================

void AstraTabSearchBubble::MoveSelection(int delta) {
  size_t count = GetResultCount();
  if (count == 0) {
    return;
  }

  // Use wrap-around selection.
  int new_index = static_cast<int>(selected_index_) + delta;
  if (new_index < 0) {
    new_index = static_cast<int>(count) - 1;
  } else if (new_index >= static_cast<int>(count)) {
    new_index = 0;
  }

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

  if (selected_index_ >= results_.size() - 1) {
    // Already at end — wrap to first.
    selected_index_ = 0;
    UpdateSelectionVisual();
    ScrollSelectedIntoView();
    if (delegate_) {
      delegate_->OnSelectionChanged(selected_index_);
    }
    return;
  }

  AstraTabSearchResultType current_type =
      results_[selected_index_].result_type;

  // Find the first result of the next type.
  for (size_t i = selected_index_ + 1; i < results_.size(); ++i) {
    if (results_[i].result_type != current_type) {
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

  if (selected_index_ == 0) {
    // Already at start — wrap to last.
    selected_index_ = results_.size() - 1;
    UpdateSelectionVisual();
    ScrollSelectedIntoView();
    if (delegate_) {
      delegate_->OnSelectionChanged(selected_index_);
    }
    return;
  }

  AstraTabSearchResultType current_type =
      results_[selected_index_].result_type;

  // Find the last result of the previous group.
  for (int i = static_cast<int>(selected_index_) - 1; i >= 0; --i) {
    if (results_[static_cast<size_t>(i)].result_type != current_type) {
      selected_index_ = static_cast<size_t>(i);
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

  switch (item.result_type) {
    case AstraTabSearchResultType::kOpenTab:
      // Switch to the tab via the model.
      model_.SwitchToTab(item.tab_index);

      // Notify delegate.
      {
        TabStripModel* tab_strip = GetTabStripModel();
        if (tab_strip && item.tab_index >= 0 &&
            item.tab_index < tab_strip->count()) {
          content::WebContents* contents =
              tab_strip->GetWebContentsAt(item.tab_index);
          if (contents && delegate_) {
            delegate_->OnTabActivated(contents);
          }
        }
      }
      break;

    case AstraTabSearchResultType::kBookmark:
      if (delegate_ && item.url.is_valid()) {
        delegate_->OnBookmarkActivated(item.url);
      }
      break;

    case AstraTabSearchResultType::kHistory:
      if (delegate_ && item.url.is_valid()) {
        delegate_->OnHistoryActivated(item.url);
      }
      break;

    default:
      break;
  }

  // Add query to recent searches.
  if (search_field_ && !search_field_->GetText().empty()) {
    model_.AddRecentSearch(search_field_->GetText());
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

  // Only open tabs can be closed.
  if (item.result_type != AstraTabSearchResultType::kOpenTab) {
    // For bookmarks/history, remove from the model.
    // TODO(astra): Handle bookmark/history removal.
    return;
  }

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
  size_t result_idx = 0;
  for (views::View* child : results_container_->children()) {
    auto* item_view = dynamic_cast<AstraTabSearchItemView*>(child);
    if (!item_view) {
      continue;  // Skip group headers and other non-item views.
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
  if (showing_empty_state_) {
    result_count_label_->SetText(std::u16string());
  } else if (showing_no_results_) {
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

AstraTabSearchGroupHeaderView* AstraTabSearchBubble::GetGroupHeaderForType(
    AstraTabSearchResultType type) const {
  if (!results_container_) {
    return nullptr;
  }

  for (views::View* child : results_container_->children()) {
    auto* header = dynamic_cast<AstraTabSearchGroupHeaderView*>(child);
    if (header && header->group_type() == type) {
      return header;
    }
  }

  return nullptr;
}

size_t AstraTabSearchBubble::GetTotalSelectableCount() const {
  return results_.size();
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
    item.result_type = AstraTabSearchResultType::kOpenTab;
    item.item_id = i;
    item.tab_id = i;  // Legacy alias.
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
    item.last_visited_time = base::Time::Now() -
                             base::Minutes(i);  // Approximate recency.
    item.relevance_score = 0.0;

    tabs.push_back(std::move(item));
  }

  model_.SetTabList(std::move(tabs));
}

// =========================================================================
// Category toggles
// =========================================================================

void AstraTabSearchBubble::UpdateCategoryTogglesFromFilter() {
  if (!tabs_toggle_ || !bookmarks_toggle_ || !history_toggle_ ||
      !recent_toggle_) {
    return;
  }

  AstraTabSearchFilter filter = model_.GetFilter();

  tabs_toggle_->SetIsOn(
      (static_cast<int>(filter) &
       static_cast<int>(AstraTabSearchFilter::kTabs)) != 0);
  bookmarks_toggle_->SetIsOn(
      (static_cast<int>(filter) &
       static_cast<int>(AstraTabSearchFilter::kBookmarks)) != 0);
  history_toggle_->SetIsOn(
      (static_cast<int>(filter) &
       static_cast<int>(AstraTabSearchFilter::kHistory)) != 0);
  recent_toggle_->SetIsOn(
      (static_cast<int>(filter) &
       static_cast<int>(AstraTabSearchFilter::kRecentlyClosed)) != 0);
}

void AstraTabSearchBubble::OnCategoryToggleClicked(
    AstraTabSearchFilter filter) {
  AstraTabSearchFilter current = model_.GetFilter();
  int current_int = static_cast<int>(current);
  int filter_int = static_cast<int>(filter);

  // Toggle the specific filter bit.
  int new_int = current_int ^ filter_int;

  // Don't allow all filters to be off — keep at least one.
  if (new_int == 0) {
    return;
  }

  model_.SetFilter(static_cast<AstraTabSearchFilter>(new_int));
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

std::u16string AstraTabSearchBubble::GetResultTypeLabel(
    AstraTabSearchResultType type) {
  switch (type) {
    case AstraTabSearchResultType::kOpenTab:
      return u"Open Tabs";
    case AstraTabSearchResultType::kRecentlyClosed:
      return u"Recently Closed";
    case AstraTabSearchResultType::kBookmark:
      return u"Bookmarks";
    case AstraTabSearchResultType::kHistory:
      return u"History";
    case AstraTabSearchResultType::kSearchHistory:
      return u"Recent Searches";
    case AstraTabSearchResultType::kAction:
      return u"Actions";
  }
  return std::u16string();
}

// =========================================================================
// TextfieldController
// =========================================================================

void AstraTabSearchBubble::ContentsChanged(views::Textfield* sender,
                                           const std::u16string& new_contents) {
  DCHECK_EQ(sender, search_field_);
  model_.SetQuery(new_contents);
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

    case ui::VKEY_HOME:
      SelectFirst();
      return true;

    case ui::VKEY_END:
      SelectLast();
      return true;

    case ui::VKEY_RETURN:
      ActivateSelectedResult();
      return true;

    case ui::VKEY_ESCAPE:
      if (GetWidget()) {
        GetWidget()->Close();
      }
      return true;

    case ui::VKEY_PRIOR:  // Page Up
      MoveSelection(-5);
      return true;

    case ui::VKEY_NEXT:  // Page Down
      MoveSelection(5);
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

void AstraTabSearchBubble::OnSelectedIndexChanged(
    AstraTabSearchModel* /*model*/,
    size_t /*old_index*/,
    size_t new_index) {
  selected_index_ = new_index;
  UpdateSelectionVisual();
  ScrollSelectedIntoView();
  if (delegate_) {
    delegate_->OnSelectionChanged(selected_index_);
  }
}

void AstraTabSearchBubble::OnSearchModeChanged(AstraTabSearchModel* /*model*/,
                                              AstraTabSearchMode mode) {
  if (delegate_) {
    delegate_->OnSearchModeChanged(mode);
  }
}

void AstraTabSearchBubble::OnFilterChanged(AstraTabSearchModel* /*model*/,
                                           AstraTabSearchFilter filter) {
  UpdateCategoryTogglesFromFilter();
  UpdateResults();
  if (delegate_) {
    delegate_->OnFilterChanged(filter);
  }
}

void AstraTabSearchBubble::OnQueryChanged(AstraTabSearchModel* /*model*/,
                                          const std::u16string& /*query*/) {
  // Query changes flow from textfield -> model, not the other way around.
  // But if model changes query from somewhere else, sync the textfield.
  if (search_field_ && search_field_->GetText() != model_.query()) {
    search_field_->SetText(model_.query());
  }
}

void AstraTabSearchBubble::OnRecentSearchesChanged(
    AstraTabSearchModel* /*model*/) {
  // Only update if we're showing recent searches (empty query).
  if (search_field_ && search_field_->GetText().empty()) {
    UpdateResults();
  }
}

void AstraTabSearchBubble::OnTabSearchModelShutdown(
    AstraTabSearchModel* /*model*/) {
  // Model is shutting down — clean up references.
}

}  // namespace astra
