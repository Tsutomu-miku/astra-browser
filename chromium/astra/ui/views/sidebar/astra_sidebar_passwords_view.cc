#include "astra/ui/views/sidebar/astra_sidebar_passwords_view.h"

#include <memory>
#include <utility>

#include "astra/browser/astra_password_helper.h"
#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kPasswordsSectionHeaderHeight = 28;
constexpr int kPasswordsSectionHorizontalPadding = 12;
constexpr int kPasswordsSectionVerticalPadding = 8;
constexpr int kPasswordsSearchFieldHeight = 28;
constexpr int kPasswordsSearchFieldMarginBottom = 6;
constexpr int kPasswordsItemSpacing = 2;
constexpr int kPasswordsFooterLinkHeight = 28;
constexpr int kPasswordsEmptyStateHeight = 48;
constexpr int kPasswordsLoadingStateHeight = 40;
constexpr int kPasswordsToolbarHeight = 28;
constexpr int kPasswordsHealthSummaryHeight = 32;
constexpr int kPasswordsShowMoreHeight = 28;

// Section title.
const char16_t kPasswordsSectionTitle[] = u"Passwords";

// Search field placeholder.
const char16_t kSearchPlaceholder[] = u"Search passwords";

// Footer link text.
const char16_t kPasswordSettingsText[] = u"Password settings";

// State messages.
const char16_t kLoadingText[] = u"Loading passwords...";
const char16_t kEmptyStateText[] = u"No saved passwords";
const char16_t kShowMoreText[] = u"Show more";
const char16_t kHealthSummaryText[] = u"Password health";

// Sort order labels.
const char16_t kSortAlphabetical[] = u"A-Z";
const char16_t kSortLastUsed[] = u"Recently used";
const char16_t kSortDateCreated[] = u"Recently added";

// Filter labels.
const char16_t kFilterAll[] = u"All";
const char16_t kFilterCompromised[] = u"Compromised";
const char16_t kFilterWeak[] = u"Weak";
const char16_t kFilterReused[] = u"Reused";

// Astra color IDs for the passwords panel.
constexpr ui::ColorId kPasswordsHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kPasswordsCountBadgeTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kPasswordsFooterLinkColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kPasswordsSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kPasswordsToolbarTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kPasswordsHealthTextColorId =
    kColorAstraSidebarItemText;

// Helper to build sort order label list for the combobox.
std::vector<std::u16string> GetSortOrderLabels() {
  return {kSortAlphabetical, kSortLastUsed, kSortDateCreated};
}

// Helper to build filter label list for the combobox.
std::vector<std::u16string> GetFilterLabels() {
  return {kFilterAll, kFilterCompromised, kFilterWeak, kFilterReused};
}

}  // namespace

AstraSidebarPasswordsView::AstraSidebarPasswordsView(Profile* profile)
    : profile_(profile) {
  BuildLayout();

  // Obtain the password helper and start observing.
  password_helper_ = GetPasswordHelper();
  if (password_helper_) {
    password_helper_->AddObserver(this);
    // Sync presentation settings from the helper.
    all_revealed_ = !password_helper_->GetHidePasswordsByDefault();
  }

  // Kick off the initial password query.
  Refresh();
}

AstraSidebarPasswordsView::~AstraSidebarPasswordsView() {
  // Stop observing the password helper.
  if (password_helper_) {
    password_helper_->RemoveObserver(this);
  }
  // WeakPtrFactory automatically cancels pending callbacks on destruction.
}

void AstraSidebarPasswordsView::BuildLayout() {
  // Vertical box layout for the entire section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(0);

  // ---- Section header row (title + count badge) ----
  auto* header_row = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kPasswordsSectionVerticalPadding,
                          kPasswordsSectionHorizontalPadding)));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Section title.
  header_label_ = header_row->AddChildView(
      std::make_unique<views::Label>(kPasswordsSectionTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_layout->SetFlexForView(header_label_, 1);

  // Count badge (shows number of saved passwords).
  count_badge_ = header_row->AddChildView(std::make_unique<views::Label>());
  count_badge_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_badge_->SetAutoColorReadabilityEnabled(false);

  // ---- Search field ----
  auto* search_container = AddChildView(std::make_unique<views::View>());
  auto* search_layout = search_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kPasswordsSectionHorizontalPadding),
          kPasswordsSearchFieldMarginBottom));
  search_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  search_field_ = search_container->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(kSearchPlaceholder);
  search_field_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_field_->SetAccessibleName(kSearchPlaceholder);
  search_field_->SetTextChangedCallback(base::BindRepeating(
      &AstraSidebarPasswordsView::OnSearchTextChanged,
      base::Unretained(this)));

  // ---- Toolbar row (sort + filter + group + reveal all) ----
  toolbar_row_ = AddChildView(std::make_unique<views::View>());
  auto* toolbar_layout = toolbar_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(4, kPasswordsSectionHorizontalPadding)));
  toolbar_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  toolbar_layout->set_between_child_spacing(8);
  toolbar_row_->SetPreferredSize(gfx::Size(0, kPasswordsToolbarHeight));

  // Sort combobox.
  sort_combobox_ = toolbar_row_->AddChildView(
      std::make_unique<views::Combobox>(GetSortOrderLabels()));
  sort_combobox_->SetAccessibleName(u"Sort passwords");
  sort_combobox_->SetTooltipText(u"Sort passwords");
  sort_combobox_->SetCallback(base::BindRepeating(
      &AstraSidebarPasswordsView::OnSortOrderChanged,
      base::Unretained(this)));

  // Filter combobox.
  filter_combobox_ = toolbar_row_->AddChildView(
      std::make_unique<views::Combobox>(GetFilterLabels()));
  filter_combobox_->SetAccessibleName(u"Filter passwords");
  filter_combobox_->SetTooltipText(u"Filter passwords");
  filter_combobox_->SetCallback(base::BindRepeating(
      &AstraSidebarPasswordsView::OnFilterChanged,
      base::Unretained(this)));

  // Spacer between filters and toggles.
  auto* spacer = toolbar_row_->AddChildView(std::make_unique<views::View>());
  toolbar_layout->SetFlexForView(spacer, 1);

  // Group by toggle.
  group_toggle_ = toolbar_row_->AddChildView(
      std::make_unique<views::ToggleButton>(base::BindRepeating(
          &AstraSidebarPasswordsView::OnGroupByToggled,
          base::Unretained(this))));
  group_toggle_->SetAccessibleName(u"Group by site");
  group_toggle_->SetTooltipText(u"Group passwords by site");
  group_toggle_->SetPreferredSize(gfx::Size(28, 16));

  // Reveal all toggle.
  reveal_all_toggle_ = toolbar_row_->AddChildView(
      std::make_unique<views::ToggleButton>(base::BindRepeating(
          &AstraSidebarPasswordsView::OnRevealAllToggled,
          base::Unretained(this))));
  reveal_all_toggle_->SetAccessibleName(u"Show all passwords");
  reveal_all_toggle_->SetTooltipText(u"Show all passwords");
  reveal_all_toggle_->SetPreferredSize(gfx::Size(28, 16));

  // ---- Health summary row ----
  health_summary_row_ = AddChildView(std::make_unique<views::View>());
  auto* health_layout = health_summary_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(4, kPasswordsSectionHorizontalPadding)));
  health_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  health_summary_row_->SetPreferredSize(
      gfx::Size(0, kPasswordsHealthSummaryHeight));
  health_summary_row_->SetVisible(false);  // Hidden by default

  health_summary_label_ = health_summary_row_->AddChildView(
      std::make_unique<views::Label>(kHealthSummaryText));
  health_summary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  health_summary_label_->SetAutoColorReadabilityEnabled(false);
  health_layout->SetFlexForView(health_summary_label_, 1);

  // ---- Loading state indicator ----
  loading_label_ = AddChildView(std::make_unique<views::Label>(kLoadingText));
  loading_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  loading_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kPasswordsLoadingStateHeight / 2 - 8,
                      kPasswordsSectionHorizontalPadding)));
  loading_label_->SetAutoColorReadabilityEnabled(false);
  loading_label_->SetVisible(false);
  loading_label_->SetAccessibleName(u"Loading passwords");

  // ---- Empty state message ----
  empty_state_label_ = AddChildView(std::make_unique<views::Label>(kEmptyStateText));
  empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  empty_state_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kPasswordsEmptyStateHeight / 2 - 8,
                      kPasswordsSectionHorizontalPadding)));
  empty_state_label_->SetAutoColorReadabilityEnabled(false);
  empty_state_label_->SetVisible(false);
  empty_state_label_->SetAccessibleName(u"No saved passwords");

  // ---- Items container ----
  items_container_ = AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kPasswordsItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // ---- "Show more" button ----
  show_more_button_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarPasswordsView::OnShowMoreClicked,
              base::Unretained(this)),
          kShowMoreText));
  show_more_button_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  show_more_button_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kPasswordsSectionHorizontalPadding)));
  show_more_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  show_more_button_->SetVisible(false);
  show_more_button_->SetAccessibleName(u"Show more passwords");

  // ---- Footer: "Password settings" link ----
  settings_link_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarPasswordsView::OnPasswordSettingsClicked,
              base::Unretained(this)),
          kPasswordSettingsText));
  settings_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  settings_link_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kPasswordsSectionHorizontalPadding)));
  settings_link_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  settings_link_->SetAccessibleName(u"Password settings");

  // Accessibility for the whole section.
  SetAccessibleName(u"Passwords");
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

AstraPasswordHelper* AstraSidebarPasswordsView::GetPasswordHelper() {
  // TODO(astra): Use a proper AstraPasswordHelperFactory to obtain the
  //   profile-scoped password helper, following Chromium's
  //   ProfileKeyedServiceFactory pattern.
  //
  //   For now, we create a helper instance directly as a placeholder.
  //   In a full Chromium build, this would be:
  //     return AstraPasswordHelperFactory::GetForProfile(profile_);
  //
  //   Chromium pattern: ProfileKeyedServiceFactory
  //     (components/keyed_service/content/browser_context_keyed_service_factory.h)
  //
  //   Patch point: None needed — we just use the standard factory pattern.

  // Placeholder: return nullptr for now since the factory doesn't exist
  // in the overlay repo. The view will show an empty state.
  return nullptr;
}

void AstraSidebarPasswordsView::Refresh() {
  if (!password_helper_) {
    // Password helper not available — show empty state.
    ClearItems();
    UpdateCountBadge();
    is_loading_ = false;
    UpdateStateVisibility();
    return;
  }

  is_loading_ = true;
  ShowLoadingState();
  HideEmptyState();

  // TODO(astra): The real password store query is async. The helper
  //   should notify via OnPasswordsChanged when results arrive.
  //   For now, we do a synchronous read and update immediately.

  std::vector<AstraPasswordEntry> entries;
  if (!search_query_.empty()) {
    entries = password_helper_->SearchPasswords(search_query_, max_items_ + 1);
  } else {
    entries = password_helper_->GetDisplayPasswords(max_items_ + 1);
  }

  // Check if there are more items than max_items_ (for "Show more" button).
  bool has_more = entries.size() > max_items_;
  if (has_more) {
    entries.resize(max_items_);
  }
  SetShowMoreVisible(has_more);

  PopulateItems(entries);
  UpdateCountBadge();
  is_loading_ = false;
  UpdateStateVisibility();
}

void AstraSidebarPasswordsView::PopulateItems(
    const std::vector<AstraPasswordEntry>& entries) {
  ClearItems();

  for (const auto& entry : entries) {
    auto item = std::make_unique<AstraPasswordItemView>(entry);
    item->set_delegate(this);
    item->SetPasswordRevealed(all_revealed_);
    items_container_->AddChildView(std::move(item));
  }

  InvalidateLayout();
}

void AstraSidebarPasswordsView::ClearItems() {
  if (items_container_) {
    items_container_->RemoveAllChildViews();
  }
}

void AstraSidebarPasswordsView::UpdateCountBadge() {
  if (!count_badge_ || !password_helper_) {
    if (count_badge_) {
      count_badge_->SetText(std::u16string());
    }
    return;
  }

  // Show the total count of saved passwords in the badge.
  // When searching, show the number of search results.
  size_t count = 0;
  if (!search_query_.empty()) {
    // For search, count results from the current visible items.
    count = items_container_ ? items_container_->children().size() : 0;
  } else {
    count = password_helper_->GetPasswordCount();
  }

  count_badge_->SetText(base::NumberToString16(count));
}

void AstraSidebarPasswordsView::UpdateHeaderForSearch() {
  // When searching, the header stays as "Passwords" but the count badge
  // updates to show search result count. UpdateCountBadge handles that.
  UpdateCountBadge();
}

// =========================================================================
// Sort / filter / group controls
// =========================================================================

void AstraSidebarPasswordsView::SetSortOrder(AstraPasswordSortOrder order) {
  if (password_helper_) {
    password_helper_->SetSortOrder(order);
  }
  if (sort_combobox_) {
    int index = 0;
    switch (order) {
      case AstraPasswordSortOrder::kAlphabetical: index = 0; break;
      case AstraPasswordSortOrder::kLastUsed: index = 1; break;
      case AstraPasswordSortOrder::kDateCreated: index = 2; break;
    }
    sort_combobox_->SetSelectedRow(index);
  }
  Refresh();
}

AstraPasswordSortOrder AstraSidebarPasswordsView::GetSortOrder() const {
  if (password_helper_) {
    return password_helper_->GetSortOrder();
  }
  return AstraPasswordSortOrder::kAlphabetical;
}

void AstraSidebarPasswordsView::SetFilter(AstraPasswordFilter filter) {
  if (password_helper_) {
    password_helper_->SetFilter(filter);
  }
  if (filter_combobox_) {
    int index = 0;
    switch (filter) {
      case AstraPasswordFilter::kAll: index = 0; break;
      case AstraPasswordFilter::kCompromised: index = 1; break;
      case AstraPasswordFilter::kWeak: index = 2; break;
      case AstraPasswordFilter::kReused: index = 3; break;
    }
    filter_combobox_->SetSelectedRow(index);
  }
  Refresh();
}

AstraPasswordFilter AstraSidebarPasswordsView::GetFilter() const {
  if (password_helper_) {
    return password_helper_->GetFilter();
  }
  return AstraPasswordFilter::kAll;
}

void AstraSidebarPasswordsView::SetGroupBy(AstraPasswordGroupBy group_by) {
  if (password_helper_) {
    password_helper_->SetGroupBy(group_by);
  }
  if (group_toggle_) {
    group_toggle_->SetIsOn(group_by != AstraPasswordGroupBy::kNone);
  }
  Refresh();
}

AstraPasswordGroupBy AstraSidebarPasswordsView::GetGroupBy() const {
  if (password_helper_) {
    return password_helper_->GetGroupBy();
  }
  return AstraPasswordGroupBy::kNone;
}

void AstraSidebarPasswordsView::SetAllRevealed(bool revealed) {
  all_revealed_ = revealed;
  if (reveal_all_toggle_) {
    reveal_all_toggle_->SetIsOn(revealed);
  }
  // Update all items.
  if (items_container_) {
    for (auto* child : items_container_->children()) {
      auto* item = static_cast<AstraPasswordItemView*>(child);
      item->SetPasswordRevealed(revealed);
    }
  }
}

bool AstraSidebarPasswordsView::AreAllRevealed() const {
  return all_revealed_;
}

// =========================================================================
// Show more
// =========================================================================

void AstraSidebarPasswordsView::SetShowMoreVisible(bool visible) {
  if (show_more_button_) {
    show_more_button_->SetVisible(visible);
  }
}

bool AstraSidebarPasswordsView::IsShowMoreVisible() const {
  return show_more_button_ && show_more_button_->GetVisible();
}

// =========================================================================
// Health summary
// =========================================================================

void AstraSidebarPasswordsView::SetHealthSummaryVisible(bool visible) {
  if (health_summary_row_) {
    health_summary_row_->SetVisible(visible);
  }
}

bool AstraSidebarPasswordsView::IsHealthSummaryVisible() const {
  return health_summary_row_ && health_summary_row_->GetVisible();
}

void AstraSidebarPasswordsView::UpdateHealthSummary() {
  if (!health_summary_label_ || !password_helper_) {
    return;
  }
  auto stats = password_helper_->GetPasswordHealthStats();
  std::u16string text = kHealthSummaryText +
      u" (" + base::NumberToString16(stats.problem_count()) + u" issues)";
  health_summary_label_->SetText(text);
}

// =========================================================================
// AstraPasswordHelperObserver
// =========================================================================

void AstraSidebarPasswordsView::OnPasswordsChanged() {
  // Password store state changed — refresh our projection.
  Refresh();
}

void AstraSidebarPasswordsView::OnPasswordSettingsChanged() {
  // Presentation settings changed — refresh UI accordingly.
  Refresh();
}

void AstraSidebarPasswordsView::OnPasswordHealthChanged() {
  // Health stats changed — update the summary.
  UpdateHealthSummary();
}

// =========================================================================
// AstraPasswordItemDelegate
// =========================================================================

void AstraSidebarPasswordsView::OnPasswordItemClicked(
    const AstraPasswordEntry& entry) {
  if (delegate_) {
    delegate_->OnPasswordSelected(entry);
    // Open the site in a new tab by default.
    delegate_->OpenPasswordURL(entry.url, /*in_new_tab=*/true);
  }
}

void AstraSidebarPasswordsView::OnPasswordCopyRequested(
    const AstraPasswordEntry& entry) {
  if (!password_helper_) {
    return;
  }
  // Delegate copy to the password helper.
  password_helper_->CopyPasswordToClipboard(entry);
  // TODO(astra): Show "Copied!" visual feedback.
}

void AstraSidebarPasswordsView::OnUsernameCopyRequested(
    const AstraPasswordEntry& entry) {
  if (!password_helper_) {
    return;
  }
  password_helper_->CopyUsernameToClipboard(entry);
  // TODO(astra): Show "Copied!" visual feedback.
}

void AstraSidebarPasswordsView::OnPasswordRevealToggled(
    const AstraPasswordEntry& entry,
    bool revealed) {
  // Per-item reveal state is handled by the item view itself.
  // We could sync this back to the helper if we wanted to persist it.
  // For now, it's just a per-session UI state.
}

void AstraSidebarPasswordsView::OnPasswordOpenInNewTab(
    const AstraPasswordEntry& entry) {
  if (delegate_ && entry.url.is_valid()) {
    delegate_->OpenPasswordURL(entry.url, /*in_new_tab=*/true);
  }
}

// =========================================================================
// Footer link callback
// =========================================================================

void AstraSidebarPasswordsView::OnPasswordSettingsClicked() {
  if (delegate_) {
    delegate_->OpenPasswordSettings();
  }
}

// =========================================================================
// Show more callback
// =========================================================================

void AstraSidebarPasswordsView::OnShowMoreClicked() {
  if (delegate_) {
    delegate_->OnShowMorePasswords();
  }
  // Increase the max items and refresh.
  max_items_ += 20;
  Refresh();
}

// =========================================================================
// Search text changes
// =========================================================================

void AstraSidebarPasswordsView::OnSearchTextChanged() {
  if (!search_field_) {
    return;
  }

  std::u16string new_query = search_field_->GetText();
  if (new_query == search_query_) {
    return;
  }

  search_query_ = new_query;
  Refresh();
  UpdateHeaderForSearch();
}

// =========================================================================
// Sort / filter / group callbacks
// =========================================================================

void AstraSidebarPasswordsView::OnSortOrderChanged() {
  if (!sort_combobox_ || !password_helper_) {
    return;
  }
  int selected = sort_combobox_->GetSelectedRow().value_or(0);
  AstraPasswordSortOrder order;
  switch (selected) {
    case 0: order = AstraPasswordSortOrder::kAlphabetical; break;
    case 1: order = AstraPasswordSortOrder::kLastUsed; break;
    case 2: order = AstraPasswordSortOrder::kDateCreated; break;
    default: order = AstraPasswordSortOrder::kAlphabetical; break;
  }
  password_helper_->SetSortOrder(order);
  Refresh();
}

void AstraSidebarPasswordsView::OnFilterChanged() {
  if (!filter_combobox_ || !password_helper_) {
    return;
  }
  int selected = filter_combobox_->GetSelectedRow().value_or(0);
  AstraPasswordFilter filter;
  switch (selected) {
    case 0: filter = AstraPasswordFilter::kAll; break;
    case 1: filter = AstraPasswordFilter::kCompromised; break;
    case 2: filter = AstraPasswordFilter::kWeak; break;
    case 3: filter = AstraPasswordFilter::kReused; break;
    default: filter = AstraPasswordFilter::kAll; break;
  }
  password_helper_->SetFilter(filter);
  Refresh();
}

void AstraSidebarPasswordsView::OnGroupByToggled() {
  if (!group_toggle_ || !password_helper_) {
    return;
  }
  bool is_on = group_toggle_->GetIsOn();
  password_helper_->SetGroupBy(is_on ? AstraPasswordGroupBy::kSite
                                     : AstraPasswordGroupBy::kNone);
  Refresh();
}

void AstraSidebarPasswordsView::OnRevealAllToggled() {
  if (!reveal_all_toggle_) {
    return;
  }
  SetAllRevealed(reveal_all_toggle_->GetIsOn());
}

// =========================================================================
// Loading and empty states
// =========================================================================

void AstraSidebarPasswordsView::ShowLoadingState() {
  if (loading_label_) {
    loading_label_->SetVisible(true);
  }
  if (items_container_) {
    items_container_->SetVisible(false);
  }
  if (show_more_button_) {
    show_more_button_->SetVisible(false);
  }
}

void AstraSidebarPasswordsView::HideLoadingState() {
  if (loading_label_) {
    loading_label_->SetVisible(false);
  }
  if (items_container_) {
    items_container_->SetVisible(true);
  }
}

void AstraSidebarPasswordsView::ShowEmptyState() {
  if (empty_state_label_) {
    empty_state_label_->SetVisible(true);
  }
  if (items_container_) {
    items_container_->SetVisible(false);
  }
  if (show_more_button_) {
    show_more_button_->SetVisible(false);
  }
}

void AstraSidebarPasswordsView::HideEmptyState() {
  if (empty_state_label_) {
    empty_state_label_->SetVisible(false);
  }
}

void AstraSidebarPasswordsView::UpdateStateVisibility() {
  if (is_loading_) {
    ShowLoadingState();
    HideEmptyState();
    return;
  }

  HideLoadingState();

  size_t item_count = items_container_ ? items_container_->children().size() : 0;
  if (item_count == 0) {
    ShowEmptyState();
  } else {
    HideEmptyState();
    if (items_container_) {
      items_container_->SetVisible(true);
    }
  }
}

// =========================================================================
// Item access (for testing)
// =========================================================================

size_t AstraSidebarPasswordsView::GetItemCount() const {
  if (!items_container_) {
    return 0;
  }
  return items_container_->children().size();
}

AstraPasswordItemView* AstraSidebarPasswordsView::GetItemAt(
    size_t index) const {
  if (!items_container_ || index >= items_container_->children().size()) {
    return nullptr;
  }
  return static_cast<AstraPasswordItemView*>(
      items_container_->children()[index]);
}

// =========================================================================
// Accessibility
// =========================================================================

void AstraSidebarPasswordsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Passwords");
  size_t count = GetItemCount();
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kSetSize,
                             static_cast<int>(count));
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarPasswordsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarPasswordsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Header colors.
  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kPasswordsHeaderTextColorId));
  }
  if (count_badge_) {
    count_badge_->SetEnabledColor(
        color_provider->GetColor(kPasswordsCountBadgeTextColorId));
  }

  // Loading state label color.
  if (loading_label_) {
    loading_label_->SetEnabledColor(
        color_provider->GetColor(kPasswordsSecondaryTextColorId));
  }

  // Empty state label color.
  if (empty_state_label_) {
    empty_state_label_->SetEnabledColor(
        color_provider->GetColor(kPasswordsSecondaryTextColorId));
  }

  // Footer link color.
  if (settings_link_) {
    settings_link_->SetEnabledTextColors(
        color_provider->GetColor(kPasswordsFooterLinkColorId));
  }

  // Health summary label color.
  if (health_summary_label_) {
    health_summary_label_->SetEnabledColor(
        color_provider->GetColor(kPasswordsHealthTextColorId));
  }

  // Show more button color.
  if (show_more_button_) {
    show_more_button_->SetEnabledTextColors(
        color_provider->GetColor(kPasswordsSecondaryTextColorId));
  }

  // Toolbar text colors.
  // TODO(astra): Style toolbar comboboxes properly with Astra colors.
}

}  // namespace astra
