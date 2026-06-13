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

// Section title.
const char16_t kPasswordsSectionTitle[] = u"Passwords";

// Search field placeholder.
const char16_t kSearchPlaceholder[] = u"Search passwords";

// Footer link text.
const char16_t kPasswordSettingsText[] = u"Password settings";

// State messages.
const char16_t kLoadingText[] = u"Loading passwords...";
const char16_t kEmptyStateText[] = u"No saved passwords";

// Astra color IDs for the passwords panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kPasswordsHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kPasswordsCountBadgeTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kPasswordsFooterLinkColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kPasswordsSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;

}  // namespace

AstraSidebarPasswordsView::AstraSidebarPasswordsView(Profile* profile)
    : profile_(profile) {
  BuildLayout();

  // Obtain the password helper and start observing.
  password_helper_ = GetPasswordHelper();
  if (password_helper_) {
    password_helper_->AddObserver(this);
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
  // Container for the search field with padding.
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
  //
  //   Proper flow:
  //     1. password_helper_->StartQuery() — triggers async PasswordStore query
  //     2. Helper receives results via PasswordStoreConsumer callback
  //     3. Helper calls OnPasswordsChanged() on observers
  //     4. This view reads updated data via GetSavedPasswords()
  //
  //   Chromium pattern: PasswordStoreConsumer
  //     (components/password_manager/core/browser/password_store_consumer.h)

  std::vector<AstraPasswordEntry> entries;
  if (!search_query_.empty()) {
    entries = password_helper_->SearchPasswords(search_query_, max_items_);
  } else {
    entries = password_helper_->GetSavedPasswords(max_items_);
  }

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
// AstraPasswordHelperObserver
// =========================================================================

void AstraSidebarPasswordsView::OnPasswordsChanged() {
  // Password store state changed — refresh our projection.
  Refresh();
}

// =========================================================================
// AstraPasswordItemDelegate
// =========================================================================

void AstraSidebarPasswordsView::OnPasswordItemClicked(
    const AstraPasswordEntry& entry) {
  if (delegate_) {
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
  // The helper handles all security (reauth, etc.) via Chromium's
  // password manager. The view just shows visual feedback.
  bool success = password_helper_->CopyPasswordToClipboard(entry);

  // TODO(astra): Show a "Copied!" toast or tooltip on success.
  //   For example, show a brief confirmation label near the copy button
  //   or use the system notification center.
  //   Chromium pattern: PasswordManagerUI shows "Copied" text feedback.
  if (success) {
    // Visual feedback could go here.
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
// Loading and empty states
// =========================================================================

void AstraSidebarPasswordsView::ShowLoadingState() {
  if (loading_label_) {
    loading_label_->SetVisible(true);
  }
  if (items_container_) {
    items_container_->SetVisible(false);
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
// Accessibility
// =========================================================================

void AstraSidebarPasswordsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Passwords");
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

  // Search field styling.
  // TODO(astra): Style the search field properly with Astra colors.
  // For now, it uses default Chromium textfield styling.
}

}  // namespace astra
