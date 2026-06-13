#include "astra/ui/views/sidebar/astra_sidebar_extensions_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "astra/ui/color/astra_color_ids.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_extension_helper.h"
#include "astra/ui/views/sidebar/astra_extension_icon_view.h"
#include "astra/ui/views/sidebar/astra_extension_popup_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSectionHeaderHeight = 28;
constexpr int kSectionHorizontalPadding = 12;
constexpr int kSectionVerticalPadding = 8;
constexpr int kSectionHeaderFontSizeDelta = 1;

// Section title.
const char16_t kExtensionsTitle[] = u"Extensions";
const char16_t kPinnedTitle[] = u"Pinned";
const char16_t kManageExtensionsText[] = u"Manage extensions";

// Astra color ID for the extensions panel header.
constexpr ui::ColorId kSectionHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;

}  // namespace

AstraSidebarExtensionsView::AstraSidebarExtensionsView(Profile* profile)
    : profile_(profile) {
  // Look up the extension helper from the profile.
  // TODO(astra): Get AstraExtensionHelper from a factory once the factory
  //   is implemented. For now, we create one directly as a placeholder.
  //   The proper pattern is to use a ProfileKeyedServiceFactory.
  //
  // Chromium pattern: BrowserContextKeyedServiceFactory or
  //   ProfileKeyedServiceFactory for profile-scoped services.
  //   See AstraWorkspaceServiceFactory for an example.

  // For now, we can't easily create the helper without a factory.
  extension_helper_ = nullptr;

  BuildLayout();

  // Subscribe to extension helper changes.
  if (extension_helper_) {
    extension_helper_observation_.Observe(extension_helper_);
  }
}

AstraSidebarExtensionsView::~AstraSidebarExtensionsView() {
  // ScopedObservation cleans up automatically.
  // Popup widget will clean itself up when closed.
}

// =========================================================================
// Delegate
// =========================================================================

void AstraSidebarExtensionsView::SetDelegate(
    AstraSidebarExtensionsDelegate* delegate) {
  delegate_ = delegate;
}

AstraSidebarExtensionsDelegate* AstraSidebarExtensionsView::GetDelegate()
    const {
  return delegate_;
}

// =========================================================================
// Extension list management
// =========================================================================

void AstraSidebarExtensionsView::SetExtensions(
    const std::vector<AstraExtensionInfo>& extensions) {
  extensions_ = extensions;
  RebuildIcons();
  UpdateSectionVisibility();
}

int AstraSidebarExtensionsView::GetExtensionCount() const {
  return static_cast<int>(extensions_.size());
}

AstraExtensionInfo AstraSidebarExtensionsView::GetExtensionAt(int index) const {
  if (index < 0 || index >= static_cast<int>(extensions_.size())) {
    return AstraExtensionInfo();
  }
  return extensions_[index];
}

void AstraSidebarExtensionsView::AddExtension(const AstraExtensionInfo& info) {
  // Check if already exists.
  auto it = base::ranges::find_if(
      extensions_,
      [&info](const AstraExtensionInfo& e) {
        return e.extension_id == info.extension_id;
      });
  if (it != extensions_.end()) {
    // Update instead of adding.
    *it = info;
  } else {
    extensions_.push_back(info);
  }
  SortExtensionList();
  RebuildIcons();
}

void AstraSidebarExtensionsView::RemoveExtension(
    const std::string& extension_id) {
  std::erase_if(extensions_,
                [&extension_id](const AstraExtensionInfo& info) {
                  return info.extension_id == extension_id;
                });
  // Also remove from pinned list.
  std::erase(pinned_ids_, extension_id);
  RebuildIcons();
  UpdateSectionVisibility();
}

void AstraSidebarExtensionsView::UpdateExtension(
    const AstraExtensionInfo& info) {
  auto it = base::ranges::find_if(
      extensions_,
      [&info](const AstraExtensionInfo& e) {
        return e.extension_id == info.extension_id;
      });
  if (it != extensions_.end()) {
    *it = info;
    SortExtensionList();
    RebuildIcons();
  }
}

bool AstraSidebarExtensionsView::HasExtension(
    const std::string& extension_id) const {
  return base::ranges::any_of(
      extensions_,
      [&extension_id](const AstraExtensionInfo& info) {
        return info.extension_id == extension_id;
      });
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarExtensionsView::SetSelectedExtension(
    const std::string& extension_id) {
  if (selected_extension_id_ == extension_id) {
    return;
  }

  // Clear previous selection.
  if (!selected_extension_id_.empty()) {
    AstraExtensionIconView* old_view = FindIconView(selected_extension_id_);
    if (old_view) {
      old_view->SetIsCurrent(false);
    }
  }

  selected_extension_id_ = extension_id;

  // Set new selection.
  if (!selected_extension_id_.empty()) {
    AstraExtensionIconView* new_view = FindIconView(selected_extension_id_);
    if (new_view) {
      new_view->SetIsCurrent(true);
    }
  }
}

std::string AstraSidebarExtensionsView::GetSelectedExtensionId() const {
  return selected_extension_id_;
}

void AstraSidebarExtensionsView::ClearSelection() {
  SetSelectedExtension(std::string());
}

// =========================================================================
// Pinned extensions
// =========================================================================

void AstraSidebarExtensionsView::SetPinnedExtensions(
    const std::vector<std::string>& extension_ids) {
  pinned_ids_ = extension_ids;
  // Update is_pinned on each extension info.
  for (auto& info : extensions_) {
    info.is_pinned = base::Contains(pinned_ids_, info.extension_id);
  }
  RebuildIcons();
}

std::vector<std::string> AstraSidebarExtensionsView::GetPinnedExtensions()
    const {
  return pinned_ids_;
}

bool AstraSidebarExtensionsView::IsExtensionPinned(
    const std::string& extension_id) const {
  return base::Contains(pinned_ids_, extension_id);
}

void AstraSidebarExtensionsView::PinExtension(
    const std::string& extension_id) {
  if (IsExtensionPinned(extension_id)) {
    return;
  }
  pinned_ids_.push_back(extension_id);

  // Update the extension info.
  auto it = base::ranges::find_if(
      extensions_,
      [&extension_id](const AstraExtensionInfo& e) {
        return e.extension_id == extension_id;
      });
  if (it != extensions_.end()) {
    it->is_pinned = true;
  }

  // Update the icon view.
  AstraExtensionIconView* view = FindIconView(extension_id);
  if (view) {
    view->SetPinned(true);
  }

  RebuildIcons();

  if (delegate_) {
    delegate_->OnExtensionPinned(extension_id, true);
  }
}

void AstraSidebarExtensionsView::UnpinExtension(
    const std::string& extension_id) {
  if (!IsExtensionPinned(extension_id)) {
    return;
  }
  std::erase(pinned_ids_, extension_id);

  // Update the extension info.
  auto it = base::ranges::find_if(
      extensions_,
      [&extension_id](const AstraExtensionInfo& e) {
        return e.extension_id == extension_id;
      });
  if (it != extensions_.end()) {
    it->is_pinned = false;
  }

  // Update the icon view.
  AstraExtensionIconView* view = FindIconView(extension_id);
  if (view) {
    view->SetPinned(false);
  }

  RebuildIcons();

  if (delegate_) {
    delegate_->OnExtensionPinned(extension_id, false);
  }
}

// =========================================================================
// Reordering
// =========================================================================

void AstraSidebarExtensionsView::MoveExtension(int from_index, int to_index) {
  if (from_index < 0 || from_index >= static_cast<int>(extensions_.size())) {
    return;
  }
  if (to_index < 0 || to_index >= static_cast<int>(extensions_.size())) {
    return;
  }
  if (from_index == to_index) {
    return;
  }

  // Move the extension in the list.
  AstraExtensionInfo info = extensions_[from_index];
  extensions_.erase(extensions_.begin() + from_index);
  extensions_.insert(extensions_.begin() + to_index, info);

  // If we're in manual sort mode, rebuild.
  if (sort_by_ == AstraExtensionSortBy::kManual) {
    RebuildIcons();
  }

  if (delegate_) {
    delegate_->OnExtensionReordered(from_index, to_index);
  }
}

// =========================================================================
// Grid layout
// =========================================================================

void AstraSidebarExtensionsView::SetExtensionsPerRow(int count) {
  if (extensions_per_row_ == count) {
    return;
  }
  extensions_per_row_ = count;
  // TODO(astra): Update grid layout with new column count.
  //   For now, we use FlexLayout wrapping which adapts automatically.
  InvalidateLayout();
}

int AstraSidebarExtensionsView::GetExtensionsPerRow() const {
  return extensions_per_row_;
}

void AstraSidebarExtensionsView::SetIconSize(int size_px) {
  if (icon_size_ == size_px) {
    return;
  }
  icon_size_ = size_px;
  // Update all icon views.
  for (views::View* child : all_icons_container_->children()) {
    static_cast<AstraExtensionIconView*>(child)->SetIconSize(size_px);
  }
  for (views::View* child : pinned_icons_container_->children()) {
    static_cast<AstraExtensionIconView*>(child)->SetIconSize(size_px);
  }
  InvalidateLayout();
}

int AstraSidebarExtensionsView::GetIconSize() const {
  return icon_size_;
}

void AstraSidebarExtensionsView::SetSpacing(int spacing_px) {
  if (icon_spacing_ == spacing_px) {
    return;
  }
  icon_spacing_ = spacing_px;
  // TODO(astra): Update FlexLayout gap on both containers.
  InvalidateLayout();
}

int AstraSidebarExtensionsView::GetSpacing() const {
  return icon_spacing_;
}

// =========================================================================
// Sections
// =========================================================================

void AstraSidebarExtensionsView::SetShowPinnedSection(bool show) {
  if (show_pinned_section_ == show) {
    return;
  }
  show_pinned_section_ = show;
  if (pinned_section_) {
    pinned_section_->SetVisible(show && !pinned_ids_.empty());
  }
  InvalidateLayout();
}

bool AstraSidebarExtensionsView::GetShowPinnedSection() const {
  return show_pinned_section_;
}

void AstraSidebarExtensionsView::SetShowAllExtensionsSection(bool show) {
  if (show_all_section_ == show) {
    return;
  }
  show_all_section_ = show;
  if (all_section_) {
    all_section_->SetVisible(show);
  }
  InvalidateLayout();
}

bool AstraSidebarExtensionsView::GetShowAllExtensionsSection() const {
  return show_all_section_;
}

void AstraSidebarExtensionsView::SetShowDisabledExtensions(bool show) {
  if (show_disabled_extensions_ == show) {
    return;
  }
  show_disabled_extensions_ = show;
  RebuildIcons();
}

bool AstraSidebarExtensionsView::GetShowDisabledExtensions() const {
  return show_disabled_extensions_;
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarExtensionsView::SetSortExtensionsBy(
    AstraExtensionSortBy sort_by) {
  if (sort_by_ == sort_by) {
    return;
  }
  sort_by_ = sort_by;
  SortExtensionList();
  RebuildIcons();
}

AstraExtensionSortBy AstraSidebarExtensionsView::GetSortExtensionsBy() const {
  return sort_by_;
}

void AstraSidebarExtensionsView::SortExtensionList() {
  auto comparator = [this](const AstraExtensionInfo& a,
                           const AstraExtensionInfo& b) {
    switch (sort_by_) {
      case AstraExtensionSortBy::kName:
        return base::CompareCaseInsensitiveASCII(a.name, b.name) < 0;
      case AstraExtensionSortBy::kInstallDate:
        return a.install_time > b.install_time;  // Most recent first
      case AstraExtensionSortBy::kLastUsed:
        // TODO(astra): Sort by last used time when we track it.
        return base::CompareCaseInsensitiveASCII(a.name, b.name) < 0;
      case AstraExtensionSortBy::kManual:
        // Keep original order — no sorting.
        return false;
    }
    return false;
  };

  std::stable_sort(extensions_.begin(), extensions_.end(), comparator);
}

// =========================================================================
// Counts
// =========================================================================

int AstraSidebarExtensionsView::GetPinnedExtensionCount() const {
  return static_cast<int>(pinned_ids_.size());
}

int AstraSidebarExtensionsView::GetEnabledExtensionCount() const {
  return static_cast<int>(base::ranges::count_if(
      extensions_,
      [](const AstraExtensionInfo& info) {
        return info.state == AstraExtensionState::kEnabled;
      }));
}

int AstraSidebarExtensionsView::GetDisabledExtensionCount() const {
  return static_cast<int>(base::ranges::count_if(
      extensions_,
      [](const AstraExtensionInfo& info) {
        return info.state == AstraExtensionState::kDisabled;
      }));
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarExtensionsView::SearchExtensions(
    const std::u16string& query) {
  search_query_ = query;
  RebuildIcons();
}

int AstraSidebarExtensionsView::GetSearchResultsCount() const {
  if (search_query_.empty()) {
    return GetExtensionCount();
  }
  return static_cast<int>(base::ranges::count_if(
      extensions_,
      [this](const AstraExtensionInfo& info) {
        return ShouldShowExtension(info);
      }));
}

// =========================================================================
// Icon view access
// =========================================================================

AstraExtensionIconView* AstraSidebarExtensionsView::GetExtensionIconView(
    const std::string& extension_id) const {
  return FindIconView(extension_id);
}

// =========================================================================
// Popup management
// =========================================================================

void AstraSidebarExtensionsView::ShowExtensionPopup(
    const std::string& extension_id,
    views::View* anchor) {
  // Close any existing popup first.
  CloseActivePopup();

  // Find extension info for the popup title.
  std::u16string extension_name = base::UTF8ToUTF16(extension_id);
  auto it = base::ranges::find_if(
      extensions_,
      [&extension_id](const AstraExtensionInfo& info) {
        return info.extension_id == extension_id;
      });
  if (it != extensions_.end()) {
    extension_name = it->name;
  }

  // Create and show the new popup.
  auto* popup = new AstraExtensionPopupView(
      extension_id, extension_name, anchor, this);
  active_popup_ = popup;
  active_popup_extension_id_ = extension_id;

  // Mark the icon as having an active popup.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view) {
    icon_view->SetPopupShowing(true);
  }

  popup->Show();

  // Notify delegate.
  if (delegate_) {
    delegate_->OnExtensionPopupShown(extension_id);
  }
}

void AstraSidebarExtensionsView::HideExtensionPopup() {
  CloseActivePopup();
}

bool AstraSidebarExtensionsView::IsPopupVisible() const {
  return active_popup_ != nullptr && active_popup_->IsVisible();
}

std::string AstraSidebarExtensionsView::GetCurrentPopupExtensionId() const {
  return active_popup_extension_id_;
}

// =========================================================================
// Badge / notifications
// =========================================================================

void AstraSidebarExtensionsView::SetShowExtensionsBadge(bool show) {
  show_extensions_badge_ = show;
  // TODO(astra): Show/hide badge on the section header.
  SchedulePaint();
}

bool AstraSidebarExtensionsView::GetShowExtensionsBadge() const {
  return show_extensions_badge_;
}

int AstraSidebarExtensionsView::GetExtensionNotificationCount() const {
  int total = 0;
  for (const auto& info : extensions_) {
    // TODO(astra): Track notification count per extension in AstraExtensionInfo.
    //   For now, we estimate from badge text or return 0.
    total += 0;
  }
  return total;
}

// =========================================================================
// Build layout
// =========================================================================

void AstraSidebarExtensionsView::BuildLayout() {
  // Vertical box layout: header + pinned section + search + all section + manage.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(0);

  // --------------------------------------------------------------------
  // Header row: label + collapse arrow (placeholder)
  // --------------------------------------------------------------------
  header_row_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout =
      header_row_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kSectionVerticalPadding, kSectionHorizontalPadding),
          0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  header_label_ =
      header_row_->AddChildView(std::make_unique<views::Label>(kExtensionsTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(
          kSectionHeaderFontSizeDelta));
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_layout->SetFlexForView(header_label_, 1);

  // --------------------------------------------------------------------
  // Pinned extensions section
  // --------------------------------------------------------------------
  pinned_section_ = AddChildView(std::make_unique<views::View>());
  auto* pinned_layout = pinned_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  pinned_layout->set_between_child_spacing(kSectionVerticalPadding);

  pinned_icons_container_ =
      pinned_section_->AddChildView(std::make_unique<views::View>());
  auto* pinned_icons_layout = pinned_icons_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  pinned_icons_layout->SetOrientation(views::LayoutOrientation::kRow);
  pinned_icons_layout->SetWrapDirection(
      views::FlexLayout::WrapDirection::kWrap);
  pinned_icons_layout->SetDefault(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithWeight(1.0f));
  pinned_icons_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kSectionHorizontalPadding));
  pinned_icons_layout->SetColumnGap(icon_spacing_);
  pinned_icons_layout->SetRowGap(icon_spacing_);

  pinned_section_->SetVisible(false);  // Hidden until there are pinned exts.

  // --------------------------------------------------------------------
  // All extensions section
  // --------------------------------------------------------------------
  all_section_ = AddChildView(std::make_unique<views::View>());
  auto* all_layout = all_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  all_layout->set_between_child_spacing(kSectionVerticalPadding);

  all_icons_container_ =
      all_section_->AddChildView(std::make_unique<views::View>());
  auto* all_icons_layout = all_icons_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  all_icons_layout->SetOrientation(views::LayoutOrientation::kRow);
  all_icons_layout->SetWrapDirection(
      views::FlexLayout::WrapDirection::kWrap);
  all_icons_layout->SetDefault(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithWeight(1.0f));
  all_icons_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kSectionHorizontalPadding));
  all_icons_layout->SetColumnGap(icon_spacing_);
  all_icons_layout->SetRowGap(icon_spacing_);

  // --------------------------------------------------------------------
  // Manage extensions link
  // --------------------------------------------------------------------
  manage_link_ = AddChildView(std::make_unique<views::Label>(
      kManageExtensionsText));
  manage_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  manage_link_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSectionVerticalPadding, kSectionHorizontalPadding)));
  manage_link_->SetVisible(show_manage_link_);
  // TODO(astra): Make this an actual clickable link button.

  // Initial populate.
  if (extension_helper_) {
    RefreshFromHelper();
  } else {
    // No extension helper available — show empty state.
    UpdateSectionVisibility();
  }
}

// =========================================================================
// Refresh / rebuild
// =========================================================================

void AstraSidebarExtensionsView::RefreshFromHelper() {
  if (!extension_helper_) {
    return;
  }
  // TODO(astra): Read from extension helper and populate extensions_ list.
  //   For now, the list is set directly via SetExtensions() in tests.
  RebuildIcons();
  UpdateSectionVisibility();
}

void AstraSidebarExtensionsView::RebuildIcons() {
  // Clear both containers.
  pinned_icons_container_->RemoveAllChildViews();
  all_icons_container_->RemoveAllChildViews();

  bool has_pinned = false;
  bool has_all = false;

  for (const auto& info : extensions_) {
    if (!ShouldShowExtension(info)) {
      continue;
    }

    std::unique_ptr<AstraExtensionIconView> icon_view = CreateIconView(info);

    if (info.is_pinned && show_pinned_section_) {
      pinned_icons_container_->AddChildView(std::move(icon_view));
      has_pinned = true;
    } else if (show_all_section_) {
      all_icons_container_->AddChildView(std::move(icon_view));
      has_all = true;
    }
  }

  // Update section visibility based on content.
  pinned_section_->SetVisible(show_pinned_section_ && has_pinned);
  all_section_->SetVisible(show_all_section_ && has_all);

  // Update selected state on the newly created icon.
  if (!selected_extension_id_.empty()) {
    AstraExtensionIconView* selected = FindIconView(selected_extension_id_);
    if (selected) {
      selected->SetIsCurrent(true);
    }
  }

  InvalidateLayout();
}

bool AstraSidebarExtensionsView::ShouldShowExtension(
    const AstraExtensionInfo& info) const {
  // Filter by state (disabled extensions).
  if (!show_disabled_extensions_ &&
      info.state == AstraExtensionState::kDisabled) {
    return false;
  }

  // Filter by search query.
  if (!search_query_.empty()) {
    if (info.name.find(search_query_) == std::u16string::npos) {
      // Also try case-insensitive match.
      std::u16string lower_name = base::ToLowerASCII(info.name);
      std::u16string lower_query = base::ToLowerASCII(search_query_);
      if (lower_name.find(lower_query) == std::u16string::npos) {
        return false;
      }
    }
  }

  return true;
}

void AstraSidebarExtensionsView::UpdateSectionVisibility() {
  bool has_extensions = !extensions_.empty();

  // Hide the section if there are no extensions.
  if (!has_extensions) {
    SetVisible(false);
    return;
  }

  // Show the section, respecting the collapsed state.
  SetVisible(true);
  pinned_section_->SetVisible(expanded_ && show_pinned_section_ &&
                              !pinned_ids_.empty());
  all_section_->SetVisible(expanded_ && show_all_section_);
  if (search_field_) {
    search_field_->SetVisible(expanded_ && show_search_);
  }
  if (manage_link_) {
    manage_link_->SetVisible(expanded_ && show_manage_link_);
  }
}

std::unique_ptr<AstraExtensionIconView>
AstraSidebarExtensionsView::CreateIconView(const AstraExtensionInfo& info) {
  auto icon_view =
      std::make_unique<AstraExtensionIconView>(info.extension_id, this);

  // Set all info at once.
  icon_view->SetExtensionInfo(info);

  // Set icon size.
  icon_view->SetIconSize(icon_size_);

  // Set popup showing state if this is the active popup.
  if (info.extension_id == active_popup_extension_id_) {
    icon_view->SetPopupShowing(true);
  }

  // Set current/selected state.
  if (info.extension_id == selected_extension_id_) {
    icon_view->SetIsCurrent(true);
  }

  return icon_view;
}

AstraExtensionIconView* AstraSidebarExtensionsView::FindIconView(
    const std::string& extension_id) const {
  // Search pinned section first.
  for (views::View* child : pinned_icons_container_->children()) {
    auto* icon_view = static_cast<AstraExtensionIconView*>(child);
    if (icon_view->GetExtensionId() == extension_id) {
      return icon_view;
    }
  }

  // Search all extensions section.
  for (views::View* child : all_icons_container_->children()) {
    auto* icon_view = static_cast<AstraExtensionIconView*>(child);
    if (icon_view->GetExtensionId() == extension_id) {
      return icon_view;
    }
  }

  return nullptr;
}

// =========================================================================
// Collapse / expand
// =========================================================================

void AstraSidebarExtensionsView::ToggleExpanded() {
  SetExpanded(!expanded_);
}

void AstraSidebarExtensionsView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;

  pinned_section_->SetVisible(expanded && show_pinned_section_ &&
                              !pinned_ids_.empty());
  all_section_->SetVisible(expanded && show_all_section_);
  if (search_field_) {
    search_field_->SetVisible(expanded && show_search_);
  }
  if (manage_link_) {
    manage_link_->SetVisible(expanded && show_manage_link_);
  }

  // TODO(astra): Animate the expand/collapse transition.
  //   Chromium pattern: Use views::Animation or gfx::SlideAnimation
  //   for smooth section expand/collapse.

  InvalidateLayout();
}

// =========================================================================
// AstraExtensionHelperObserver
// =========================================================================

void AstraSidebarExtensionsView::OnExtensionsChanged() {
  // Extension list changed — rebuild everything.
  RefreshFromHelper();
}

void AstraSidebarExtensionsView::OnExtensionIconChanged(
    const std::string& extension_id) {
  // Update just that one icon.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view && extension_helper_) {
    gfx::Image icon = extension_helper_->GetExtensionIcon(extension_id, 16);
    icon_view->SetIcon(icon.AsImageSkia());
  }
}

// =========================================================================
// AstraExtensionIconDelegate
// =========================================================================

void AstraSidebarExtensionsView::OnExtensionClicked(
    const std::string& extension_id) {
  // Left click: toggle the extension popup.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view) {
    ToggleExtensionPopup(extension_id, icon_view);
  }

  if (delegate_) {
    delegate_->OnExtensionClicked(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionMiddleClicked(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnExtensionMiddleClicked(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionRightClicked(
    const std::string& extension_id,
    const gfx::Point& point) {
  ShowExtensionContextMenu(extension_id, point);

  if (delegate_) {
    delegate_->OnExtensionRightClicked(extension_id, point);
  }
}

void AstraSidebarExtensionsView::OnExtensionPopupShown(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnExtensionPopupShown(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionPopupClosed(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnExtensionPopupClosed(extension_id);
  }
}

void AstraSidebarExtensionsView::OnPinExtension(
    const std::string& extension_id, bool pinned) {
  if (pinned) {
    PinExtension(extension_id);
  } else {
    UnpinExtension(extension_id);
  }
}

void AstraSidebarExtensionsView::OnManageExtensionRequested(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnManageExtensionsRequested();
  }
}

void AstraSidebarExtensionsView::OnRemoveExtensionRequested(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnRemoveExtensionRequested(extension_id);
  }
}

void AstraSidebarExtensionsView::OnDisableExtensionRequested(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnDisableExtensionRequested(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionOptionsRequested(
    const std::string& extension_id) {
  if (delegate_) {
    delegate_->OnExtensionOptionsRequested(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionIconClicked(
    const std::string& extension_id,
    views::View* anchor_view) {
  // Legacy callback — handled by OnExtensionClicked.
}

void AstraSidebarExtensionsView::OnExtensionIconContextMenu(
    const std::string& extension_id,
    const gfx::Point& point) {
  // Legacy callback — handled by OnExtensionRightClicked.
}

// =========================================================================
// Popup management
// =========================================================================

void AstraSidebarExtensionsView::ToggleExtensionPopup(
    const std::string& extension_id,
    views::View* anchor_view) {
  // If this extension's popup is already showing, close it.
  if (active_popup_ && active_popup_extension_id_ == extension_id) {
    CloseActivePopup();
    return;
  }

  ShowExtensionPopup(extension_id, anchor_view);
}

void AstraSidebarExtensionsView::CloseActivePopup() {
  if (!active_popup_) {
    return;
  }

  // Mark the icon as no longer having an active popup.
  AstraExtensionIconView* icon_view =
      FindIconView(active_popup_extension_id_);
  if (icon_view) {
    icon_view->SetPopupShowing(false);
  }

  active_popup_->Hide();
  // active_popup_ will be cleared by OnExtensionPopupClosed callback.
}

// =========================================================================
// AstraExtensionPopupDelegate
// =========================================================================

void AstraSidebarExtensionsView::OnExtensionPopupClosed(
    const std::string& extension_id) {
  // Clear the active popup reference.
  if (active_popup_extension_id_ == extension_id) {
    active_popup_ = nullptr;
    active_popup_extension_id_.clear();
  }

  // Update the icon's popup-showing state.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view) {
    icon_view->SetPopupShowing(false);
  }

  // Notify parent delegate.
  if (delegate_) {
    delegate_->OnExtensionPopupClosed(extension_id);
  }
}

void AstraSidebarExtensionsView::OnExtensionPopupShown(
    const std::string& extension_id) {
  // Notify parent delegate.
  if (delegate_) {
    delegate_->OnExtensionPopupShown(extension_id);
  }
}

// =========================================================================
// Context menu
// =========================================================================

void AstraSidebarExtensionsView::ShowExtensionContextMenu(
    const std::string& extension_id,
    const gfx::Point& screen_point) {
  // TODO(astra): Show real extension context menu from Chromium.
  //
  //   Chromium provides ExtensionContextMenuModel
  //   (chrome/browser/extensions/extension_context_menu_model.h)
  //   which has all the standard extension menu items.
  //
  //   Chromium owner: ExtensionContextMenuModel
  //     (chrome/browser/extensions/extension_context_menu_model.h)
  //   Chromium owner: MenuRunner (ui/views/controls/menu/menu_runner.h)
  //
  //   Patch point: None needed — we can use these classes directly.

  // Placeholder: no-op for now.
}

// =========================================================================
// Section visibility
// =========================================================================

void AstraSidebarExtensionsView::SetSectionVisible(bool visible) {
  SetVisible(visible);
}

// =========================================================================
// Displayed extensions
// =========================================================================

std::vector<AstraExtensionInfo>
AstraSidebarExtensionsView::GetDisplayedExtensions() const {
  std::vector<AstraExtensionInfo> result;
  for (const auto& info : extensions_) {
    if (ShouldShowExtension(info)) {
      result.push_back(info);
    }
  }
  return result;
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarExtensionsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarExtensionsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Extensions");
}

void AstraSidebarExtensionsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  header_label_->SetEnabledColor(
      color_provider->GetColor(kSectionHeaderTextColorId));
}

}  // namespace astra
