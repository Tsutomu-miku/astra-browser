#include "astra/ui/views/sidebar/astra_sidebar_history_view.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_history_item_view.h"
#include "base/strings/string_util.h"
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
// are forward-declared and the real definitions come from Chromium at build time.

namespace astra {

namespace {

// Layout constants.
constexpr int kHistoryGroupHeaderHeight = 24;
constexpr int kHistoryGroupHeaderFontSizeDelta = 0;
constexpr int kHistoryItemSpacing = 2;
constexpr int kHistoryFooterLinkHeight = 28;
constexpr int kHistoryLoadingStateHeight = 40;
constexpr int kHistoryEmptyStateHeight = 48;

// Group titles.
const char16_t kTodayTitle[] = u"Today";
const char16_t kYesterdayTitle[] = u"Yesterday";
const char16_t kLastWeekTitle[] = u"Last week";
const char16_t kOlderTitle[] = u"Older";
const char16_t kShowFullHistoryText[] = u"Show full history";
const char16_t kHistorySectionTitle[] = u"History";
const char16_t kLoadingText[] = u"Loading history...";
const char16_t kEmptyStateText[] = u"No browsing history";
const char16_t kSearchPlaceholder[] = u"Search history...";

// Astra color IDs.
constexpr ui::ColorId kHistoryGroupHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kHistoryFooterLinkColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kHistorySecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kHistoryBackgroundId = kColorAstraSidebarBackground;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraSidebarHistoryView::AstraSidebarHistoryView(Profile* profile)
    : AstraSidebarSectionView(kHistorySectionTitle,
                              AstraSidebarSectionType::kHistory),
      profile_(profile) {
  BuildHistoryLayout();

  // Configure base section appearance.
  SetShowChevron(true);
  SetShowItemCount(true);
  SetShowSearch(true);
  SetShowMoreButton(true);
  SetEmptyStateText(kEmptyStateText);

  // Kick off the initial history query.
  Refresh();
}

AstraSidebarHistoryView::~AstraSidebarHistoryView() {
  // WeakPtrFactory automatically cancels pending callbacks on destruction.
}

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarHistoryView::BuildHistoryLayout() {
  // The base class creates header, content (scroll), and footer areas.
  // We build the group containers inside the content view's items container.

  // Clear the default items container and replace with group structure.
  // TODO(astra): Better integration with base class content area.
  // For now, we build groups inside the items container.

  if (!items_container()) {
    return;
  }

  // Ensure vertical box layout on items container.
  auto* layout = items_container()->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Today group.
  today_group_ = items_container()->AddChildView(std::make_unique<views::View>());
  auto* today_layout = today_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  today_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  today_header_ = today_group_->AddChildView(
      std::make_unique<views::Label>(kTodayTitle));
  today_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  today_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHeaderHeight)));
  today_header_->SetAutoColorReadabilityEnabled(false);
  today_header_->SetFontList(
      today_header_->font_list().DeriveWithSizeDelta(
          kHistoryGroupHeaderFontSizeDelta));

  today_items_ = today_group_->AddChildView(std::make_unique<views::View>());
  auto* today_items_layout = today_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  today_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Yesterday group.
  yesterday_group_ = items_container()->AddChildView(std::make_unique<views::View>());
  auto* yesterday_layout = yesterday_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  yesterday_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  yesterday_header_ = yesterday_group_->AddChildView(
      std::make_unique<views::Label>(kYesterdayTitle));
  yesterday_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  yesterday_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHeaderHeight)));
  yesterday_header_->SetAutoColorReadabilityEnabled(false);

  yesterday_items_ = yesterday_group_->AddChildView(std::make_unique<views::View>());
  auto* yesterday_items_layout = yesterday_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  yesterday_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Last week group.
  last_week_group_ = items_container()->AddChildView(std::make_unique<views::View>());
  auto* last_week_layout = last_week_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  last_week_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  last_week_header_ = last_week_group_->AddChildView(
      std::make_unique<views::Label>(kLastWeekTitle));
  last_week_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  last_week_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHeaderHeight)));
  last_week_header_->SetAutoColorReadabilityEnabled(false);

  last_week_items_ = last_week_group_->AddChildView(std::make_unique<views::View>());
  auto* last_week_items_layout = last_week_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  last_week_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Older group.
  older_group_ = items_container()->AddChildView(std::make_unique<views::View>());
  auto* older_layout = older_group_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  older_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  older_header_ = older_group_->AddChildView(
      std::make_unique<views::Label>(kOlderTitle));
  older_header_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  older_header_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kHistorySectionHeaderHeight)));
  older_header_->SetAutoColorReadabilityEnabled(false);

  older_items_ = older_group_->AddChildView(std::make_unique<views::View>());
  auto* older_items_layout = older_items_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kHistoryItemSpacing));
  older_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Initially hide all groups — they become visible when items are added.
  today_group_->SetVisible(false);
  yesterday_group_->SetVisible(false);
  last_week_group_->SetVisible(false);
  older_group_->SetVisible(false);

  // Footer: "Show full history" link.
  // The base class provides a footer area; we configure it here.
  if (GetFooterView()) {
    GetFooterView()->SetVisible(true);
    // TODO(astra): Add the "Show full history" link to footer.
  }

  // Set accessibility for the whole view.
  SetAccessibleName(u"History");
}

// =========================================================================
// History data projection
// =========================================================================

void AstraSidebarHistoryView::SetHistoryItems(
    const std::vector<AstraHistoryItemInfo>& items) {
  history_items_ = items;
  RebuildGroupsFromItems();
  SetItemCount(static_cast<int>(history_items_.size()));
  SetEmpty(history_items_.empty());
  UpdateStateVisibility();
}

int AstraSidebarHistoryView::GetHistoryItemCount() const {
  return static_cast<int>(history_items_.size());
}

AstraHistoryItemInfo AstraSidebarHistoryView::GetHistoryItemAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(history_items_.size())) {
    return AstraHistoryItemInfo();
  }
  return history_items_[index];
}

void AstraSidebarHistoryView::AddHistoryItem(
    const AstraHistoryItemInfo& item) {
  history_items_.push_back(item);
  AddHistoryItemView(item);
  SetItemCount(static_cast<int>(history_items_.size()));
  SetEmpty(false);
  UpdateStateVisibility();
}

void AstraSidebarHistoryView::RemoveHistoryItem(int index) {
  if (index < 0 || index >= static_cast<int>(history_items_.size())) {
    return;
  }
  history_items_.erase(history_items_.begin() + index);
  RebuildGroupsFromItems();
  SetItemCount(static_cast<int>(history_items_.size()));
  SetEmpty(history_items_.empty());
  UpdateStateVisibility();
}

void AstraSidebarHistoryView::ClearAllHistory() {
  history_items_.clear();
  ClearItemsViews();
  SetItemCount(0);
  SetEmpty(true);
  UpdateStateVisibility();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarHistoryView::SetSelectedItem(int index) {
  selected_index_ = index;
  // TODO(astra): Update visual selection state of item views.
}

void AstraSidebarHistoryView::ClearSelection() {
  selected_index_ = -1;
  // TODO(astra): Clear visual selection on all items.
}

// =========================================================================
// Date grouping
// =========================================================================

void AstraSidebarHistoryView::SetGroupByDate(bool group) {
  if (group_by_date_ == group) {
    return;
  }
  group_by_date_ = group;
  RebuildGroupsFromItems();
}

void AstraSidebarHistoryView::SetShowToday(bool show) {
  if (show_today_ == show) {
    return;
  }
  show_today_ = show;
  if (today_group_) {
    today_group_->SetVisible(show && today_expanded_ &&
                             GetTodayCount() > 0);
  }
}

void AstraSidebarHistoryView::SetShowYesterday(bool show) {
  if (show_yesterday_ == show) {
    return;
  }
  show_yesterday_ = show;
  if (yesterday_group_) {
    yesterday_group_->SetVisible(show && yesterday_expanded_ &&
                                  GetYesterdayCount() > 0);
  }
}

void AstraSidebarHistoryView::SetShowLastWeek(bool show) {
  if (show_last_week_ == show) {
    return;
  }
  show_last_week_ = show;
  if (last_week_group_) {
    last_week_group_->SetVisible(show && last_week_expanded_ &&
                                  GetLastWeekCount() > 0);
  }
}

void AstraSidebarHistoryView::SetShowOlder(bool show) {
  if (show_older_ == show) {
    return;
  }
  show_older_ = show;
  if (older_group_) {
    older_group_->SetVisible(show && older_expanded_ &&
                              GetOlderCount() > 0);
  }
}

int AstraSidebarHistoryView::GetTodayCount() const {
  int count = 0;
  for (const auto& item : history_items_) {
    if (item.time_group == AstraHistoryItemInfo::TimeGroup::kToday) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarHistoryView::GetYesterdayCount() const {
  int count = 0;
  for (const auto& item : history_items_) {
    if (item.time_group == AstraHistoryItemInfo::TimeGroup::kYesterday) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarHistoryView::GetLastWeekCount() const {
  int count = 0;
  for (const auto& item : history_items_) {
    if (item.time_group == AstraHistoryItemInfo::TimeGroup::kLastWeek) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarHistoryView::GetOlderCount() const {
  int count = 0;
  for (const auto& item : history_items_) {
    if (item.time_group == AstraHistoryItemInfo::TimeGroup::kOlder) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarHistoryView::GetGroupCount() const {
  int count = 0;
  if (show_today_ && GetTodayCount() > 0) ++count;
  if (show_yesterday_ && GetYesterdayCount() > 0) ++count;
  if (show_last_week_ && GetLastWeekCount() > 0) ++count;
  if (show_older_ && GetOlderCount() > 0) ++count;
  return count;
}

AstraSidebarHistoryView::GroupInfo AstraSidebarHistoryView::GetGroupAt(
    int index) const {
  GroupInfo info;
  int visible_index = 0;

  auto check_group = [&](TimeGroup group, const std::u16string& title,
                          int count, bool expanded) {
    if (count > 0 && show_today_) {
      if (visible_index == index) {
        info.group = group;
        info.title = title;
        info.item_count = count;
        info.is_expanded = expanded;
        return true;
      }
      ++visible_index;
    }
    return false;
  };

  if (check_group(TimeGroup::kToday, kTodayTitle, GetTodayCount(),
                   today_expanded_)) {
    return info;
  }
  if (check_group(TimeGroup::kYesterday, kYesterdayTitle, GetYesterdayCount(),
                   yesterday_expanded_)) {
    return info;
  }
  if (check_group(TimeGroup::kLastWeek, kLastWeekTitle, GetLastWeekCount(),
                   last_week_expanded_)) {
    return info;
  }
  if (check_group(TimeGroup::kOlder, kOlderTitle, GetOlderCount(),
                   older_expanded_)) {
    return info;
  }

  return info;
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarHistoryView::SearchHistory(const std::u16string& query) {
  SetSearchQuery(query);
  show_search_results_ = !query.empty();

  if (delegate_) {
    delegate_->OnSearchHistory(query);
  }
}

int AstraSidebarHistoryView::GetSearchResultsCount() const {
  if (GetSearchQuery().empty()) {
    return static_cast<int>(history_items_.size());
  }
  // TODO(astra): Count actual search results.
  // For now, return total count.
  return static_cast<int>(history_items_.size());
}

void AstraSidebarHistoryView::SetShowSearchResults(bool show) {
  show_search_results_ = show;
}

// =========================================================================
// Domain operations
// =========================================================================

void AstraSidebarHistoryView::RemoveItemsForDomain(const std::string& domain) {
  std::vector<AstraHistoryItemInfo> filtered;
  for (const auto& item : history_items_) {
    if (item.url.host() != domain) {
      filtered.push_back(item);
    }
  }
  history_items_ = std::move(filtered);
  RebuildGroupsFromItems();
  SetItemCount(static_cast<int>(history_items_.size()));
  SetEmpty(history_items_.empty());
}

int AstraSidebarHistoryView::GetDomainCount() const {
  std::set<std::string> domains;
  for (const auto& item : history_items_) {
    domains.insert(item.url.host());
  }
  return static_cast<int>(domains.size());
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarHistoryView::SetShowFavicons(bool show) {
  if (show_favicons_ == show) {
    return;
  }
  show_favicons_ = show;
  // TODO(astra): Update all item views to show/hide favicons.
}

void AstraSidebarHistoryView::SetShowTime(bool show) {
  if (show_time_ == show) {
    return;
  }
  show_time_ = show;
  // TODO(astra): Update all item views to show/hide time.
}

void AstraSidebarHistoryView::SetMaxItemsPerGroup(int max) {
  if (max_items_per_group_ == max) {
    return;
  }
  max_items_per_group_ = max;
  RebuildGroupsFromItems();
}

// =========================================================================
// Group expand/collapse
// =========================================================================

void AstraSidebarHistoryView::ExpandGroup(int group_index) {
  GroupInfo info = GetGroupAt(group_index);
  if (info.item_count == 0) return;

  switch (info.group) {
    case TimeGroup::kToday:
      today_expanded_ = true;
      if (today_group_) today_group_->SetVisible(show_today_);
      break;
    case TimeGroup::kYesterday:
      yesterday_expanded_ = true;
      if (yesterday_group_) yesterday_group_->SetVisible(show_yesterday_);
      break;
    case TimeGroup::kLastWeek:
      last_week_expanded_ = true;
      if (last_week_group_) last_week_group_->SetVisible(show_last_week_);
      break;
    case TimeGroup::kOlder:
      older_expanded_ = true;
      if (older_group_) older_group_->SetVisible(show_older_);
      break;
  }
}

void AstraSidebarHistoryView::CollapseGroup(int group_index) {
  GroupInfo info = GetGroupAt(group_index);
  if (info.item_count == 0) return;

  switch (info.group) {
    case TimeGroup::kToday:
      today_expanded_ = false;
      if (today_group_) today_group_->SetVisible(false);
      break;
    case TimeGroup::kYesterday:
      yesterday_expanded_ = false;
      if (yesterday_group_) yesterday_group_->SetVisible(false);
      break;
    case TimeGroup::kLastWeek:
      last_week_expanded_ = false;
      if (last_week_group_) last_week_group_->SetVisible(false);
      break;
    case TimeGroup::kOlder:
      older_expanded_ = false;
      if (older_group_) older_group_->SetVisible(false);
      break;
  }
}

bool AstraSidebarHistoryView::IsGroupExpanded(int group_index) const {
  GroupInfo info = GetGroupAt(group_index);
  return info.is_expanded;
}

void AstraSidebarHistoryView::ToggleGroup(int group_index) {
  if (IsGroupExpanded(group_index)) {
    CollapseGroup(group_index);
  } else {
    ExpandGroup(group_index);
  }
}

// =========================================================================
// Refresh from service
// =========================================================================

void AstraSidebarHistoryView::Refresh() {
  history::HistoryService* history_service = GetHistoryService();
  if (!history_service) {
    ClearItemsViews();
    ShowEmptyState();
    return;
  }

  is_loading_ = true;
  ShowLoadingState();
  HideEmptyState();

  // TODO(astra): Actually call HistoryService::QueryHistory().
  // For now, clear items to show empty state.
  ClearItemsViews();
  is_loading_ = false;
  HideLoadingState();
  UpdateStateVisibility();
}

history::HistoryService* AstraSidebarHistoryView::GetHistoryService() {
  // TODO(astra): Use HistoryServiceFactory::GetForProfile(profile_).
  // Chromium factory: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  return nullptr;
}

void AstraSidebarHistoryView::OnHistoryQueryComplete(
    const history::QueryResults& results) {
  is_loading_ = false;
  HideLoadingState();

  ClearItemsViews();
  history_items_.clear();

  size_t count = std::min(results.results.size(), max_items_);
  for (size_t i = 0; i < count; ++i) {
    const auto& result = results.results[i];
    AstraHistoryItemInfo info;
    info.id = base::NumberToString(result.id());
    info.title = result.title();
    info.url = result.url();
    info.hostname = base::UTF8ToUTF16(result.url().host());
    info.visit_time = result.last_visit();
    info.visit_count = result.visit_count();
    info.time_group = GroupTime(result.last_visit());
    history_items_.push_back(info);
    AddHistoryItemView(info);
  }

  SetItemCount(static_cast<int>(history_items_.size()));
  SetEmpty(history_items_.empty());
  UpdateStateVisibility();
  InvalidateLayout();
}

// =========================================================================
// Time grouping helpers
// =========================================================================

// static
AstraHistoryItemInfo::TimeGroup AstraSidebarHistoryView::GroupTime(
    base::Time visit_time) {
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
    // Fallback: day-based approximation.
    base::TimeDelta delta = now - visit_time;
    if (delta < base::Days(1)) return AstraHistoryItemInfo::TimeGroup::kToday;
    if (delta < base::Days(2)) return AstraHistoryItemInfo::TimeGroup::kYesterday;
    if (delta < base::Days(7)) return AstraHistoryItemInfo::TimeGroup::kLastWeek;
    return AstraHistoryItemInfo::TimeGroup::kOlder;
  }

  if (visit_time >= today_start) {
    return AstraHistoryItemInfo::TimeGroup::kToday;
  }

  base::Time yesterday_start = today_start - base::Days(1);
  if (visit_time >= yesterday_start) {
    return AstraHistoryItemInfo::TimeGroup::kYesterday;
  }

  base::Time week_ago_start = today_start - base::Days(7);
  if (visit_time >= week_ago_start) {
    return AstraHistoryItemInfo::TimeGroup::kLastWeek;
  }

  return AstraHistoryItemInfo::TimeGroup::kOlder;
}

// =========================================================================
// Item view management
// =========================================================================

void AstraSidebarHistoryView::ClearItemsViews() {
  if (today_items_) today_items_->RemoveAllChildViews();
  if (yesterday_items_) yesterday_items_->RemoveAllChildViews();
  if (last_week_items_) last_week_items_->RemoveAllChildViews();
  if (older_items_) older_items_->RemoveAllChildViews();
}

void AstraSidebarHistoryView::AddHistoryItemView(
    const AstraHistoryItemInfo& info) {
  TimeGroup group = info.time_group;
  views::View* container = GetGroupContainer(group);
  if (!container) {
    return;
  }

  EnsureGroupHeader(group);

  auto item = std::make_unique<AstraHistoryItemView>(
      info.title, info.url, info.visit_time);
  item->SetVisitCount(info.visit_count);
  item->SetTypedVisit(info.is_typed_visit);
  item->SetTimeGroup(GetGroupTitle(group));

  // Delegate for item actions.
  // TODO(astra): Implement AstraHistoryItemDelegate on this view.
  // Currently the view's internal handlers manage this.

  container->AddChildView(std::move(item));
}

views::View* AstraSidebarHistoryView::GetGroupContainer(TimeGroup group) {
  switch (group) {
    case TimeGroup::kToday:
      return today_items_;
    case TimeGroup::kYesterday:
      return yesterday_items_;
    case TimeGroup::kLastWeek:
      return last_week_items_;
    case TimeGroup::kOlder:
      return older_items_;
  }
  return nullptr;
}

void AstraSidebarHistoryView::EnsureGroupHeader(TimeGroup group) {
  views::View* group_view = nullptr;
  bool show_group = false;
  bool expanded = true;

  switch (group) {
    case TimeGroup::kToday:
      group_view = today_group_;
      show_group = show_today_;
      expanded = today_expanded_;
      break;
    case TimeGroup::kYesterday:
      group_view = yesterday_group_;
      show_group = show_yesterday_;
      expanded = yesterday_expanded_;
      break;
    case TimeGroup::kLastWeek:
      group_view = last_week_group_;
      show_group = show_last_week_;
      expanded = last_week_expanded_;
      break;
    case TimeGroup::kOlder:
      group_view = older_group_;
      show_group = show_older_;
      expanded = older_expanded_;
      break;
  }
  if (group_view && show_group && expanded) {
    group_view->SetVisible(true);
  }
}

void AstraSidebarHistoryView::RebuildGroupsFromItems() {
  ClearItemsViews();

  // Hide all groups initially.
  if (today_group_) today_group_->SetVisible(false);
  if (yesterday_group_) yesterday_group_->SetVisible(false);
  if (last_week_group_) last_week_group_->SetVisible(false);
  if (older_group_) older_group_->SetVisible(false);

  // Add items to their respective groups.
  int counts[4] = {0, 0, 0, 0};  // today, yesterday, last_week, older

  for (const auto& item : history_items_) {
    int idx = static_cast<int>(item.time_group);
    if (idx < 0 || idx >= 4) continue;

    // Enforce max items per group.
    if (counts[idx] >= max_items_per_group_) {
      continue;
    }
    ++counts[idx];

    AddHistoryItemView(item);
  }

  UpdateGroupsVisibility();
}

void AstraSidebarHistoryView::UpdateGroupsVisibility() {
  if (today_group_) {
    today_group_->SetVisible(show_today_ && today_expanded_ &&
                              GetTodayCount() > 0);
  }
  if (yesterday_group_) {
    yesterday_group_->SetVisible(show_yesterday_ && yesterday_expanded_ &&
                                  GetYesterdayCount() > 0);
  }
  if (last_week_group_) {
    last_week_group_->SetVisible(show_last_week_ && last_week_expanded_ &&
                                  GetLastWeekCount() > 0);
  }
  if (older_group_) {
    older_group_->SetVisible(show_older_ && older_expanded_ &&
                              GetOlderCount() > 0);
  }
}

// static
std::u16string AstraSidebarHistoryView::GetGroupTitle(TimeGroup group) {
  switch (group) {
    case TimeGroup::kToday:
      return kTodayTitle;
    case TimeGroup::kYesterday:
      return kYesterdayTitle;
    case TimeGroup::kLastWeek:
      return kLastWeekTitle;
    case TimeGroup::kOlder:
      return kOlderTitle;
  }
  return std::u16string();
}

// =========================================================================
// Loading and empty states
// =========================================================================

void AstraSidebarHistoryView::ShowLoadingState() {
  SetLoading(true);
}

void AstraSidebarHistoryView::HideLoadingState() {
  SetLoading(false);
}

void AstraSidebarHistoryView::ShowEmptyState() {
  SetEmpty(true);
}

void AstraSidebarHistoryView::HideEmptyState() {
  SetEmpty(false);
}

void AstraSidebarHistoryView::UpdateStateVisibility() {
  if (is_loading_) {
    return;
  }

  bool has_items = !history_items_.empty();
  SetEmpty(!has_items);

  if (has_items) {
    UpdateGroupsVisibility();
  }
}

// =========================================================================
// Context menu
// =========================================================================

void AstraSidebarHistoryView::ShowItemContextMenu(
    AstraHistoryItemView* item,
    const gfx::Point& screen_point) {
  // TODO(astra): Implement full context menu using views::MenuRunner.
  // Chromium owner: views::MenuRunner (ui/views/controls/menu/menu_runner.h)
  if (!item) {
    return;
  }
  OnHistoryItemRemoved(item->GetUrl());
}

void AstraSidebarHistoryView::OnRemoveFromHistory(const GURL& url) {
  OnHistoryItemRemoved(url);
}

// =========================================================================
// Callbacks
// =========================================================================

void AstraSidebarHistoryView::OnShowFullHistoryClicked() {
  if (delegate_) {
    // TODO(astra): Notify delegate to open full history.
  }
}

void AstraSidebarHistoryView::OnHistoryItemClicked(const GURL& url) {
  if (delegate_) {
    // Find the item ID for this URL.
    for (const auto& item : history_items_) {
      if (item.url == url) {
        delegate_->OnHistoryItemClicked(item.id);
        break;
      }
    }
  }
}

void AstraSidebarHistoryView::OnHistoryItemRemoved(const GURL& url) {
  // TODO(astra): Forward to the delegate / service layer.
  if (delegate_) {
    for (const auto& item : history_items_) {
      if (item.url == url) {
        delegate_->OnRemoveHistoryItem(item.id);
        break;
      }
    }
    Refresh();
  }
}

// =========================================================================
// AstraSidebarSectionView overrides
// =========================================================================

void AstraSidebarHistoryView::OnSearchQueryChanged(
    const std::u16string& query) {
  AstraSidebarSectionView::OnSearchQueryChanged(query);
  if (delegate_) {
    delegate_->OnSearchHistory(query);
  }
}

void AstraSidebarHistoryView::OnShowMoreClicked() {
  OnShowFullHistoryClicked();
}

void AstraSidebarHistoryView::OnMoreButtonClicked() {
  // TODO(astra): Show section options menu (clear history, etc.).
  AstraSidebarSectionView::OnMoreButtonClicked();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarHistoryView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return AstraSidebarSectionView::CalculatePreferredSize(available_size);
}

void AstraSidebarHistoryView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  AstraSidebarSectionView::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("History");
}

void AstraSidebarHistoryView::OnThemeChanged() {
  AstraSidebarSectionView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Group header colors.
  SkColor group_header_color =
      color_provider->GetColor(kHistoryGroupHeaderTextColorId);
  if (today_header_) today_header_->SetEnabledColor(group_header_color);
  if (yesterday_header_) yesterday_header_->SetEnabledColor(group_header_color);
  if (last_week_header_) last_week_header_->SetEnabledColor(group_header_color);
  if (older_header_) older_header_->SetEnabledColor(group_header_color);

  SetBackground(views::CreateSolidBackground(
      color_provider->GetColor(kHistoryBackgroundId)));
}

// =========================================================================
// Keyboard navigation
// =========================================================================

bool AstraSidebarHistoryView::MoveFocusToNextItem() {
  // TODO(astra): Implement full arrow-key navigation between history items.
  return false;
}

bool AstraSidebarHistoryView::MoveFocusToPreviousItem() {
  // TODO(astra): Implement full arrow-key navigation.
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
  if (last_week_items_ && last_week_group_ && last_week_group_->GetVisible()) {
    count += last_week_items_->children().size();
  }
  if (older_items_ && older_group_ && older_group_->GetVisible()) {
    count += older_items_->children().size();
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

  if (last_week_items_ && last_week_group_ && last_week_group_->GetVisible()) {
    size_t last7_count = last_week_items_->children().size();
    if (index < flat_index + last7_count) {
      return static_cast<AstraHistoryItemView*>(
          last_week_items_->children()[index - flat_index]);
    }
    flat_index += last7_count;
  }

  if (older_items_ && older_group_ && older_group_->GetVisible()) {
    size_t older_count = older_items_->children().size();
    if (index < flat_index + older_count) {
      return static_cast<AstraHistoryItemView*>(
          older_items_->children()[index - flat_index]);
    }
  }

  return nullptr;
}

}  // namespace astra
