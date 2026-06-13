#include "astra/ui/views/sidebar/astra_sidebar_recently_closed_view.h"

#include <memory>
#include <utility>

#include "astra/browser/astra_recent_tabs_helper.h"
#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_recently_closed_item_view.h"
#include "base/strings/utf_string_conversions.h"
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

namespace astra {

namespace {

// Layout constants.
constexpr int kRecentlyClosedSectionHeaderHeight = 28;
constexpr int kRecentlyClosedSectionHorizontalPadding = 12;
constexpr int kRecentlyClosedSectionVerticalPadding = 8;
constexpr int kRecentlyClosedItemSpacing = 2;
constexpr int kRecentlyClosedFooterLinkHeight = 28;
constexpr int kRecentlyClosedFooterLinkFontSizeDelta = 0;
constexpr int kRecentlyClosedEmptyStateHeight = 40;

// Section title.
const char16_t kRecentlyClosedSectionTitle[] = u"Recently closed";

// Footer link text.
const char16_t kRestoreAllText[] = u"Restore all";

// Empty state text.
const char16_t kEmptyStateText[] = u"No recently closed tabs";

// Collapsed/expanded indicator text.
const char16_t kChevronDown[] = u"▾";
const char16_t kChevronRight[] = u"▸";

// Astra color IDs for the recently closed panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kRecentlyClosedHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kRecentlyClosedFooterLinkColorId =
    kColorAstraSidebarItemText;
constexpr ui::ColorId kRecentlyClosedSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;

}  // namespace

AstraSidebarRecentlyClosedView::AstraSidebarRecentlyClosedView(
    Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  BuildLayout();

  // Kick off the initial refresh.
  // TODO(astra): Use TabRestoreServiceObserver for reactive updates instead
  // of just a one-time query on construction.
  // Chromium observer: sessions::TabRestoreServiceObserver
  Refresh();
}

AstraSidebarRecentlyClosedView::~AstraSidebarRecentlyClosedView() {
  // WeakPtrFactory automatically cancels pending callbacks on destruction.
  // TODO(astra): Also remove TabRestoreServiceObserver when that pattern
  // is adopted.
}

void AstraSidebarRecentlyClosedView::BuildLayout() {
  // Vertical box layout for the entire section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(0);

  // Section header button — click to collapse/expand.
  header_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraSidebarRecentlyClosedView::OnHeaderClicked,
          base::Unretained(this)),
      kRecentlyClosedSectionTitle));
  header_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kRecentlyClosedSectionVerticalPadding,
      kRecentlyClosedSectionHorizontalPadding)));
  header_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  // TODO(astra): Replace text-based chevron with a proper image icon
  // from Chromium's vector icon system (ui/gfx/vector_icon_types.h).
  // For now, use a simple text chevron as a placeholder.
  // Chromium owner: views::ImageView + vector icons (ui/gfx/vector_icon_utils.h)
  header_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::ImageSkia());  // Placeholder — text chevron in label instead

  // Items container.
  items_container_ = AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kRecentlyClosedItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Empty state label — shown when there are no recently closed tabs.
  empty_state_label_ = AddChildView(std::make_unique<views::Label>(kEmptyStateText));
  empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  empty_state_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kRecentlyClosedEmptyStateHeight / 2 - 8,
                      kRecentlyClosedSectionHorizontalPadding)));
  empty_state_label_->SetAutoColorReadabilityEnabled(false);
  empty_state_label_->SetVisible(false);
  empty_state_label_->SetAccessibleName(u"No recently closed tabs");

  // "Restore all" footer link.
  restore_all_link_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraSidebarRecentlyClosedView::OnRestoreAllClicked,
          base::Unretained(this)),
      kRestoreAllText));
  restore_all_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  restore_all_link_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kRecentlyClosedSectionHorizontalPadding)));
  restore_all_link_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  restore_all_link_->SetAccessibleName(u"Restore all recently closed tabs");

  // Accessibility for the whole section.
  SetAccessibleName(u"Recently closed tabs");
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

void AstraSidebarRecentlyClosedView::Refresh() {
  // Get recently closed tabs from the helper (which reads from
  // TabRestoreService).
  std::vector<AstraRecentlyClosedTab> tabs =
      AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_, max_items_);

  // If the service returned empty (service not available in overlay build),
  // we still rebuild the items (which results in an empty list).
  PopulateItems(tabs);

  // Hide the "Restore all" link when there are no items.
  bool has_items = !tabs.empty();
  restore_all_link_->SetVisible(has_items && is_expanded_);
}

void AstraSidebarRecentlyClosedView::SetExpanded(bool expanded) {
  if (is_expanded_ == expanded) {
    return;
  }
  is_expanded_ = expanded;

  // Toggle visibility of items and footer link.
  bool has_items = items_container_->children().size() > 0;
  items_container_->SetVisible(is_expanded_ && has_items);
  restore_all_link_->SetVisible(is_expanded_ && has_items);

  // Show/hide empty state.
  if (empty_state_label_) {
    empty_state_label_->SetVisible(is_expanded_ && !has_items);
  }

  // Update header chevron.
  // TODO(astra): Use a proper icon instead of text.
  // For now, we update the image or text indicator.
  // Currently using text-only approach — update via tooltip or text.
  // Actually, let's use the label button text with a prefix.
  // For simplicity, we keep the title static and use a tooltip.
  std::u16string title = kRecentlyClosedSectionTitle;
  title += is_expanded_ ? u"  " + std::u16string(kChevronDown)
                        : u"  " + std::u16string(kChevronRight);
  // Re-setting the full text updates the button.
  header_button_->SetText(title);

  InvalidateLayout();
}

bool AstraSidebarRecentlyClosedView::HasItems() const {
  if (!items_container_) {
    return false;
  }
  return items_container_->children().size() > 0;
}

void AstraSidebarRecentlyClosedView::ClearItems() {
  if (items_container_) {
    items_container_->RemoveAllChildViews();
  }
}

void AstraSidebarRecentlyClosedView::PopulateItems(
    const std::vector<AstraRecentlyClosedTab>& tabs) {
  ClearItems();

  for (const auto& tab : tabs) {
    auto item = std::make_unique<AstraRecentlyClosedItemView>(
        tab.title, tab.url, tab.close_time, tab.entry_id);

    // Click handler: restore the tab.
    // We use LabelButton's SetCallback since the item inherits from it.
    // The item also has its own Delegate interface, but using the
    // LabelButton callback is simpler and follows the history pattern.
    int entry_id = tab.entry_id;
    item->SetCallback(base::BindRepeating(
        &AstraSidebarRecentlyClosedView::OnItemClicked,
        base::Unretained(this), entry_id));

    items_container_->AddChildView(std::move(item));
  }

  // Update visibility based on expanded state.
  bool has_items = !tabs.empty();
  items_container_->SetVisible(is_expanded_ && has_items);
  restore_all_link_->SetVisible(is_expanded_ && has_items);

  // Show/hide empty state.
  if (empty_state_label_) {
    empty_state_label_->SetVisible(!has_items && is_expanded_);
  }

  InvalidateLayout();
}

void AstraSidebarRecentlyClosedView::OnHeaderClicked() {
  SetExpanded(!is_expanded_);
}

void AstraSidebarRecentlyClosedView::OnRestoreAllClicked() {
  if (delegate_) {
    delegate_->RestoreAllRecentlyClosedTabs();
  }
}

void AstraSidebarRecentlyClosedView::OnItemClicked(int entry_id) {
  if (delegate_) {
    delegate_->RestoreRecentlyClosedTab(entry_id);
  }
}

gfx::Size AstraSidebarRecentlyClosedView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarRecentlyClosedView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Recently closed tabs");
}

void AstraSidebarRecentlyClosedView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Section header color.
  if (header_button_) {
    header_button_->SetEnabledTextColors(
        color_provider->GetColor(kRecentlyClosedHeaderTextColorId));
  }

  // Empty state label color.
  if (empty_state_label_) {
    empty_state_label_->SetEnabledColor(
        color_provider->GetColor(kRecentlyClosedSecondaryTextColorId));
  }

  // Footer link color.
  if (restore_all_link_) {
    restore_all_link_->SetEnabledTextColors(
        color_provider->GetColor(kRecentlyClosedFooterLinkColorId));
  }
}

}  // namespace astra
