#include "astra/ui/views/sidebar/astra_sidebar_history_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_history_item_view.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
// TODO(astra): The above includes reference Chromium headers that are only
// available in a full Chromium checkout.  In this overlay repo, the types
// are forward-declared in the header (astra_sidebar_history_view.h) and
// the real definitions come from Chromium at build time.
// Chromium owner: HistoryService (components/history/core/browser/).

namespace astra {

namespace {

// Layout constants.
constexpr int kHistorySectionHeaderHeight = 28;
constexpr int kHistorySectionHorizontalPadding = 12;
constexpr int kHistorySectionVerticalPadding = 8;
constexpr int kHistoryGroupHeaderHeight = 24;
constexpr int kHistoryGroupHeaderFontSizeDelta = 0;
constexpr int kHistoryItemSpacing = 2;
constexpr int kHistoryFooterLinkHeight = 28;
constexpr int kHistoryFooterLinkFontSizeDelta = 0;
constexpr int kHistoryLoadingStateHeight = 40;
constexpr int kHistoryEmptyStateHeight = 48;

// Group titles.
const char16_t kTodayTitle[] = u"Today";
const char16_t kYesterdayTitle[] = u"Yesterday";
const char16_t kLast7DaysTitle[] = u"Last 7 days";
const char16_t kShowFullHistoryText[] = u"Show full history";
const char16_t kHistorySectionTitle[] = u"History";
const char16_t kLoadingText[] = u"Loading history...";
const char16_t kEmptyStateText[] = u"No browsing history";

// Astra color IDs for the history panel.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra color system: astra/ui/color/astra_color_ids.h
constexpr ui::ColorId kHistoryHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kHistoryGroupHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kHistoryFooterLinkColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kHistorySecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kHistoryBackgroundId = kColorAstraSidebarBackground;

}  // namespace

AstraSidebarHistoryView::AstraSidebarHistoryView(Profile* profile)
    : profile_(profile) {
  BuildLayout();

  // Kick off the initial history query.
  // TODO(astra): Use HistoryServiceObserver for reactive updates instead
  // of just a one-time query on construction.
  // Chromium observer: history::HistoryServiceObserver
  Refresh();
}

AstraSidebarHistoryView::~AstraSidebarHistoryView() {
  // WeakPtrFactory automatically cancels pending callbacks on destruction.
  // TODO(astra): Also explicitly cancel any pending HistoryService task
  // via CancelableTaskTracker when that pattern is adopted.
}

void AstraSidebarHistoryView::BuildLayout() {
  // Vertical box layout for the entire section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(0);

  // Section header.
  header_label_ = AddChildView(std::make_unique<views::Label>(kHistorySectionTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kHistorySectionVerticalPadding, kHistorySectionHorizontalPadding)));
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetAccessibleName(u"History section");

  // Loading state indicator — initially hidden.
  loading_label_ = AddChildView(std::make_unique<views::Label>(kLoadingText));
  loading_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  loading_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kHistoryLoadingStateHeight / 2 - 8,
                      kHistorySectionHorizontalPadding)));
  loading_label_->SetAutoColorReadabilityEnabled(false);
  loading_label_->SetVisible(false);
  loading_label_->SetAccessibleName(u"Loading history");

  // Empty state message — initially hidden.
  empty_state_label_ = AddChildView(std::make_unique<views::Label>(kEmptyStateText));
  empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  empty_state_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kHistoryEmptyStateHeight / 2 - 8,
                      kHistorySectionHorizontalPadding)));
  empty_state_label_->SetAutoColorReadabilityEnabled(false);
  empty_state_label_->SetVisible(false);
  empty_state_label_->SetAccessibleName(u"No browsing history");

  // Today group.
  today_group_ = AddChildView(std::make_unique<views::View>());
  auto* today_layout = today_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  today_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  today_header_ = today_group_->AddChildView(
      std::make_unique<views::Label>(kTodayTitle));
  today_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  today_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHorizontalPadding)));
  today_header_->SetAutoColorReadabilityEnabled(false);

  today_items_ = today_group_->AddChildView(std::make_unique<views::View>());
  auto* today_items_layout = today_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  today_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Yesterday group.
  yesterday_group_ = AddChildView(std::make_unique<views::View>());
  auto* yesterday_layout = yesterday_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  yesterday_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  yesterday_header_ = yesterday_group_->AddChildView(
      std::make_unique<views::Label>(kYesterdayTitle));
  yesterday_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  yesterday_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHorizontalPadding)));
  yesterday_header_->SetAutoColorReadabilityEnabled(false);

  yesterday_items_ = yesterday_group_->AddChildView(
      std::make_unique<views::View>());
  auto* yesterday_items_layout = yesterday_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  yesterday_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Last 7 days group.
  last7days_group_ = AddChildView(std::make_unique<views::View>());
  auto* last7days_layout = last7days_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  last7days_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  last7days_header_ = last7days_group_->AddChildView(
      std::make_unique<views::Label>(kLast7DaysTitle));
  last7days_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  last7days_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHorizontalPadding)));
  last7days_header_->SetAutoColorReadabilityEnabled(false);

  last7days_items_ = last7days_group_->AddChildView(
      std::make_unique<views::View>());
  auto* last7days_items_layout = last7days_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  last7days_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // "Show full history" footer link.
  show_full_history_link_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarHistoryView::OnShowFullHistoryClicked,
              base::Unretained(this)),
          kShowFullHistoryText));
  show_full_history_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  show_full_history_link_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kHistorySectionHorizontalPadding)));
  show_full_history_link_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  show_full_history_link_->SetAccessibleName(u"Show full history");

  // Initially hide groups — they become visible when items are added.
  today_group_->SetVisible(false);
  yesterday_group_->SetVisible(false);
  last7days_group_->SetVisible(false);

  // Set accessibility for the whole view.
  SetAccessibleName(u"History");
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

void AstraSidebarHistoryView::Refresh() {
  history::HistoryService* history_service = GetHistoryService();
  if (!history_service) {
    // History service not available — show empty state.
    ClearItems();
    ShowEmptyState();
    return;
  }

  is_loading_ = true;
  ShowLoadingState();
  HideEmptyState();

  // TODO(astra): Actually call HistoryService::QueryHistory() with proper
  // QueryOptions and a callback. In a full Chromium build:
  //
  //   history::QueryOptions options;
  //   options.max_count = max_items_;
  //   options.begin_time = base::Time::Now() - base::Days(7);
  //   history_service->QueryHistory(
  //       std::u16string(),  // empty text = get all history
  //       options,
  //       &history_query_tracker_,
  //       base::BindOnce(&AstraSidebarHistoryView::OnHistoryQueryComplete,
  //                      weak_ptr_factory_.GetWeakPtr()));
  //
  // For the overlay, we simulate the async pattern with a no-op and
  // clear the list (empty projection). The real query happens when
  // wired into Chromium's HistoryService.
  //
  // Chromium API: history::HistoryService::QueryHistory
  //   (components/history/core/browser/history_service.h)
  // Chromium query options: history::QueryOptions
  //   (components/history/core/browser/history_types.h)

  // For now, clear items to show an empty history section.
  // TODO(astra): Remove this placeholder when real QueryHistory is wired up.
  ClearItems();
  is_loading_ = false;
  HideLoadingState();
  UpdateStateVisibility();
}

history::HistoryService* AstraSidebarHistoryView::GetHistoryService() {
  // TODO(astra): Use HistoryServiceFactory::GetForProfile(profile_) when
  // building against the full Chromium source tree. In the overlay, we
  // return nullptr as a placeholder since the real service isn't linked.
  //
  // Chromium factory: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  // The service is a BrowserContextKeyedService, one per profile.
  //
  // Patch point: None needed — we just call the existing factory.
  return nullptr;
}

void AstraSidebarHistoryView::OnHistoryQueryComplete(
    const history::QueryResults& results) {
  is_loading_ = false;
  HideLoadingState();

  ClearItems();

  // Populate items from query results, up to max_items_.
  size_t count = std::min(results.results.size(), max_items_);
  for (size_t i = 0; i < count; ++i) {
    const auto& result = results.results[i];
    AddHistoryItem(result.title, result.url, result.last_visit);
  }

  // Update group and state visibility.
  UpdateStateVisibility();

  InvalidateLayout();
}

AstraSidebarHistoryView::TimeGroup AstraSidebarHistoryView::GroupTime(
    base::Time visit_time) const {
  base::Time now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.LocalExplode(&now_exploded);

  // Compute start of today (midnight).
  base::Time::Exploded today_start_exploded = now_exploded;
  today_start_exploded.hour = 0;
  today_start_exploded.minute = 0;
  today_start_exploded.second = 0;
  today_start_exploded.millisecond = 0;
  base::Time today_start;
  if (!base::Time::FromLocalExploded(today_start_exploded, &today_start)) {
    // Fallback: just use the day-based approximation.
    base::TimeDelta delta = now - visit_time;
    if (delta < base::Days(1)) {
      return TimeGroup::kToday;
    }
    if (delta < base::Days(2)) {
      return TimeGroup::kYesterday;
    }
    return TimeGroup::kLast7Days;
  }

  if (visit_time >= today_start) {
    return TimeGroup::kToday;
  }

  base::Time yesterday_start = today_start - base::Days(1);
  if (visit_time >= yesterday_start) {
    return TimeGroup::kYesterday;
  }

  base::Time week_ago_start = today_start - base::Days(7);
  if (visit_time >= week_ago_start) {
    return TimeGroup::kLast7Days;
  }

  // Older than 7 days — still put in last 7 days for now since we query
  // only 7 days of history. If we ever show older items, add an "Older" group.
  return TimeGroup::kLast7Days;
}

void AstraSidebarHistoryView::ClearItems() {
  if (today_items_) {
    today_items_->RemoveAllChildViews();
  }
  if (yesterday_items_) {
    yesterday_items_->RemoveAllChildViews();
  }
  if (last7days_items_) {
    last7days_items_->RemoveAllChildViews();
  }
}

void AstraSidebarHistoryView::AddHistoryItem(const std::u16string& title,
                                             const GURL& url,
                                             base::Time visit_time) {
  TimeGroup group = GroupTime(visit_time);
  views::View* container = GetGroupContainer(group);
  if (!container) {
    return;
  }

  // Ensure the group header is visible.
  EnsureGroupHeader(group);

  auto item = std::make_unique<AstraHistoryItemView>(title, url, visit_time);
  // We implement AstraHistoryItemView::Delegate — set ourselves as the delegate
  // to receive click and remove actions from individual items.
  item->set_delegate(this);

  container->AddChildView(std::move(item));
}

views::View* AstraSidebarHistoryView::GetGroupContainer(TimeGroup group) {
  switch (group) {
    case TimeGroup::kToday:
      return today_items_;
    case TimeGroup::kYesterday:
      return yesterday_items_;
    case TimeGroup::kLast7Days:
      return last7days_items_;
  }
  return nullptr;
}

void AstraSidebarHistoryView::EnsureGroupHeader(TimeGroup group) {
  views::View* group_view = nullptr;
  switch (group) {
    case TimeGroup::kToday:
      group_view = today_group_;
      break;
    case TimeGroup::kYesterday:
      group_view = yesterday_group_;
      break;
    case TimeGroup::kLast7Days:
      group_view = last7days_group_;
      break;
  }
  if (group_view && !group_view->GetVisible()) {
    group_view->SetVisible(true);
  }
}

void AstraSidebarHistoryView::OnShowFullHistoryClicked() {
  if (delegate_) {
    delegate_->OpenFullHistory();
  }
}

void AstraSidebarHistoryView::OnHistoryItemClicked(const GURL& url) {
  if (delegate_) {
    // Open in the active tab by default.
    // TODO(astra): Add modifier key support: Ctrl/Cmd+click opens in new tab.
    // Chromium pattern: ui::Event::IsCtrlDown() / IsCommandDown() check
    // in the button press handler.
    delegate_->OpenHistoryURL(url, /*in_new_tab=*/false);
  }
}

void AstraSidebarHistoryView::OnHistoryItemRemoved(const GURL& url) {
  // Forward to the delegate / service layer.
  // TODO(astra): Call HistoryService::RemoveURLs when properly wired.
  // Chromium owner: history::HistoryService::RemoveURLs
  //   (components/history/core/browser/history_service.h)
  if (delegate_) {
    // For now, just refresh the view to reflect the change.
    Refresh();
  }
}

// =========================================================================
// Loading and empty states
// =========================================================================

void AstraSidebarHistoryView::ShowLoadingState() {
  if (loading_label_) {
    loading_label_->SetVisible(true);
  }
  // Hide groups and empty state while loading.
  if (today_group_) {
    today_group_->SetVisible(false);
  }
  if (yesterday_group_) {
    yesterday_group_->SetVisible(false);
  }
  if (last7days_group_) {
    last7days_group_->SetVisible(false);
  }
  HideEmptyState();
}

void AstraSidebarHistoryView::HideLoadingState() {
  if (loading_label_) {
    loading_label_->SetVisible(false);
  }
}

void AstraSidebarHistoryView::ShowEmptyState() {
  if (empty_state_label_) {
    empty_state_label_->SetVisible(true);
  }
}

void AstraSidebarHistoryView::HideEmptyState() {
  if (empty_state_label_) {
    empty_state_label_->SetVisible(false);
  }
}

void AstraSidebarHistoryView::UpdateStateVisibility() {
  size_t item_count = GetVisibleItemCount();

  if (is_loading_) {
    // Loading state handles its own visibility.
    return;
  }

  if (item_count == 0) {
    ShowEmptyState();
    // Hide all groups when empty.
    if (today_group_) {
      today_group_->SetVisible(false);
    }
    if (yesterday_group_) {
      yesterday_group_->SetVisible(false);
    }
    if (last7days_group_) {
      last7days_group_->SetVisible(false);
    }
  } else {
    HideEmptyState();
    // Show groups that have items.
    if (today_group_ && today_items_) {
      today_group_->SetVisible(today_items_->children().size() > 0);
    }
    if (yesterday_group_ && yesterday_items_) {
      yesterday_group_->SetVisible(yesterday_items_->children().size() > 0);
    }
    if (last7days_group_ && last7days_items_) {
      last7days_group_->SetVisible(last7days_items_->children().size() > 0);
    }
  }
}

// =========================================================================
// Context menu
// =========================================================================

void AstraSidebarHistoryView::ShowItemContextMenu(
    AstraHistoryItemView* item,
    const gfx::Point& screen_point) {
  // TODO(astra): Implement full context menu using views::MenuRunner and
  //   MenuModelAdapter with actions like:
  //   - Open in new tab
  //   - Open in new window
  //   - Open in incognito window
  //   - Remove from history
  //   - Copy link address
  // Chromium owner: views::MenuRunner (ui/views/controls/menu/menu_runner.h)
  // Chromium owner: MenuModel (ui/base/models/simple_menu_model.h)
  //
  // For now, we provide a minimal "remove" action through the delegate.
  if (!item) {
    return;
  }
  OnHistoryItemRemoved(item->url());
}

void AstraSidebarHistoryView::OnRemoveFromHistory(const GURL& url) {
  OnHistoryItemRemoved(url);
}

// =========================================================================
// Accessibility
// =========================================================================

void AstraSidebarHistoryView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("History");
}

// =========================================================================
// Keyboard navigation
// =========================================================================

bool AstraSidebarHistoryView::MoveFocusToNextItem() {
  // TODO(astra): Implement full arrow-key navigation between history items.
  //   This would work like a list view, moving focus between items in the
  //   flat order (today items, then yesterday items, then last 7 days items).
  // Chromium pattern: views::FocusManager + views::FocusTraversable
  return false;
}

bool AstraSidebarHistoryView::MoveFocusToPreviousItem() {
  // TODO(astra): Implement full arrow-key navigation (see MoveFocusToNextItem).
  return false;
}

size_t AstraSidebarHistoryView::GetVisibleItemCount() const {
  size_t count = 0;
  if (today_items_ && today_group_ && today_group_->GetVisible()) {
    count += today_items_->children().size();
  }
  if (yesterday_items_ && yesterday_group_ && yesterday_group_->GetVisible()) {
    count += yesterday_items_->children().size();
  }
  if (last7days_items_ && last7days_group_ && last7days_group_->GetVisible()) {
    count += last7days_items_->children().size();
  }
  return count;
}

AstraHistoryItemView* AstraSidebarHistoryView::GetItemAtFlatIndex(
    size_t index) const {
  size_t flat_index = 0;

  if (today_items_ && today_group_ && today_group_->GetVisible()) {
    size_t today_count = today_items_->children().size();
    if (index < flat_index + today_count) {
      return static_cast<AstraHistoryItemView*>(
          today_items_->children()[index - flat_index]);
    }
    flat_index += today_count;
  }

  if (yesterday_items_ && yesterday_group_ && yesterday_group_->GetVisible()) {
    size_t yesterday_count = yesterday_items_->children().size();
    if (index < flat_index + yesterday_count) {
      return static_cast<AstraHistoryItemView*>(
          yesterday_items_->children()[index - flat_index]);
    }
    flat_index += yesterday_count;
  }

  if (last7days_items_ && last7days_group_ && last7days_group_->GetVisible()) {
    size_t last7_count = last7days_items_->children().size();
    if (index < flat_index + last7_count) {
      return static_cast<AstraHistoryItemView*>(
          last7days_items_->children()[index - flat_index]);
    }
  }

  return nullptr;
}

gfx::Size AstraSidebarHistoryView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarHistoryView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Section header color.
  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kHistoryHeaderTextColorId));
  }

  // Loading state label color.
  if (loading_label_) {
    loading_label_->SetEnabledColor(
        color_provider->GetColor(kHistorySecondaryTextColorId));
  }

  // Empty state label color.
  if (empty_state_label_) {
    empty_state_label_->SetEnabledColor(
        color_provider->GetColor(kHistorySecondaryTextColorId));
  }

  // Group header colors.
  SkColor group_header_color =
      color_provider->GetColor(kHistoryGroupHeaderTextColorId);
  if (today_header_) {
    today_header_->SetEnabledColor(group_header_color);
  }
  if (yesterday_header_) {
    yesterday_header_->SetEnabledColor(group_header_color);
  }
  if (last7days_header_) {
    last7days_header_->SetEnabledColor(group_header_color);
  }

  // Footer link color.
  if (show_full_history_link_) {
    show_full_history_link_->SetEnabledTextColors(
        color_provider->GetColor(kHistoryFooterLinkColorId));
  }

  // Background color for the section.
  SetBackground(views::CreateSolidBackground(
      color_provider->GetColor(kHistoryBackgroundId)));
}

}  // namespace astra
