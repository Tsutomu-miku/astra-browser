#include "astra/ui/views/sidebar/astra_sidebar_extensions_view.h"

#include <memory>
#include <string>
#include <vector>

#include "astra/ui/color/astra_color_ids.h"
#include "base/ranges/algorithm.h"
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

// Grid spacing.
constexpr int kIconGridSpacing = 4;

// Section title.
const char16_t kExtensionsTitle[] = u"Extensions";

// Astra color ID for the extensions panel header.
// Uses the Astra sidebar section header color from the Astra color system.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
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
  // The real implementation would use:
  //   extension_helper_ =
  //       AstraExtensionHelperFactory::GetForProfile(profile_);
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
// Build layout
// =========================================================================

void AstraSidebarExtensionsView::BuildLayout() {
  // Vertical box layout: header + icons grid.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(0);

  // Header row: label + collapse arrow (placeholder).
  // TODO(astra): Add a collapse/expand button to the header, similar to
  //   how other sidebar sections work. For now, just the label.
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
      header_label_->font_list().DeriveWithSizeDelta(kSectionHeaderFontSizeDelta));
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_layout->SetFlexForView(header_label_, 1);

  // Icons container — grid layout of extension icons.
  // Using FlexLayout with kRow direction and wrapping for a grid effect.
  icons_container_ = AddChildView(std::make_unique<views::View>());
  auto* icons_layout = icons_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  icons_layout->SetOrientation(views::LayoutOrientation::kRow);
  icons_layout->SetWrapDirection(views::FlexLayout::WrapDirection::kWrap);
  icons_layout->SetDefault(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithWeight(1.0f));
  icons_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kSectionHorizontalPadding));
  icons_layout->SetColumnGap(kIconGridSpacing);
  icons_layout->SetRowGap(kIconGridSpacing);

  // Initial populate.
  if (extension_helper_) {
    RefreshFromHelper();
  } else {
    // No extension helper available — show empty state.
    // The section will be hidden by UpdateSectionVisibility.
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
  RebuildIcons();
  UpdateSectionVisibility();
}

void AstraSidebarExtensionsView::RebuildIcons() {
  DCHECK(extension_helper_);

  icons_container_->RemoveAllChildViews();

  auto extensions = extension_helper_->GetExtensionsWithBrowserActions();
  for (const auto& info : extensions) {
    icons_container_->AddChildView(CreateIconView(info));
  }

  InvalidateLayout();
}

void AstraSidebarExtensionsView::UpdateSectionVisibility() {
  bool has_extensions = extension_helper_ &&
      !extension_helper_->GetExtensionsWithBrowserActions().empty();

  // Hide the section if there are no extensions with browser actions.
  if (!has_extensions) {
    SetVisible(false);
    return;
  }

  // Show the section, respecting the collapsed state.
  SetVisible(true);
  icons_container_->SetVisible(expanded_);
}

std::unique_ptr<AstraExtensionIconView>
AstraSidebarExtensionsView::CreateIconView(const AstraExtensionInfo& info) {
  auto icon_view = std::make_unique<AstraExtensionIconView>(info.id, this);

  // Set tooltip.
  icon_view->SetTooltipText(info.name);

  // Set enabled state.
  icon_view->SetExtensionEnabled(info.enabled);

  // Set icon (placeholder for now — real icon comes from async load).
  // TODO(astra): Load real extension icon via AstraExtensionHelper.
  //   The helper should provide a way to get or request the icon,
  //   and notify us via OnExtensionIconChanged() when it's ready.
  gfx::ImageSkia default_icon =
      extension_helper_->GetDefaultExtensionIcon(16);
  icon_view->SetExtensionIcon(default_icon);

  return icon_view;
}

AstraExtensionIconView* AstraSidebarExtensionsView::FindIconView(
    const std::string& extension_id) const {
  for (views::View* child : icons_container_->children()) {
    auto* icon_view = static_cast<AstraExtensionIconView*>(child);
    if (icon_view->extension_id() == extension_id) {
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
  icons_container_->SetVisible(expanded_);

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
  // TODO(astra): For better performance, do incremental updates instead
  //   of full rebuild. Compare the old list to the new list and only
  //   add/remove/modify changed extensions.
  RefreshFromHelper();
}

void AstraSidebarExtensionsView::OnExtensionIconChanged(
    const std::string& extension_id) {
  // Update just that one icon.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view && extension_helper_) {
    gfx::Image icon = extension_helper_->GetExtensionIcon(extension_id, 16);
    icon_view->SetExtensionIcon(icon.AsImageSkia());
  }
}

// =========================================================================
// AstraExtensionIconDelegate
// =========================================================================

void AstraSidebarExtensionsView::OnExtensionIconClicked(
    const std::string& extension_id,
    views::View* anchor_view) {
  // Left click: toggle the extension popup.
  ToggleExtensionPopup(extension_id, anchor_view);
}

void AstraSidebarExtensionsView::OnExtensionIconContextMenu(
    const std::string& extension_id,
    const gfx::Point& point) {
  // Right click: show extension context menu.
  ShowExtensionContextMenu(extension_id, point);
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

  // Close any other popup that's showing.
  CloseActivePopup();

  // Look up extension info for the popup title.
  std::u16string extension_name = base::UTF8ToUTF16(extension_id);
  if (extension_helper_) {
    auto extensions = extension_helper_->GetExtensionsWithBrowserActions();
    auto it = base::ranges::find_if(
        extensions,
        [&extension_id](const AstraExtensionInfo& info) {
          return info.id == extension_id;
        });
    if (it != extensions.end()) {
      extension_name = it->name;
    }
  }

  // Create and show the new popup.
  // The popup view is owned by its widget (BubbleDialogDelegate pattern).
  auto* popup = new AstraExtensionPopupView(
      extension_id, extension_name, anchor_view, this);
  active_popup_ = popup;
  active_popup_extension_id_ = extension_id;

  // Mark the icon as having an active popup.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view) {
    icon_view->SetPopupShowing(true);
  }

  popup->Show();

  // TODO(astra): Wire up the real extension popup WebContents.
  //   We need to:
  //     1. Get the popup URL from the extension action
  //     2. Create an ExtensionHost for the popup
  //     3. Attach it to the popup view
  //     4. Handle resize and close events
  //
  //   Chromium owner: ExtensionPopup
  //     (chrome/browser/ui/views/extensions/extension_popup.h)
  //   Chromium patch point: We may need to patch ExtensionPopup to
  //     support alternative anchor views (sidebar instead of toolbar),
  //     or we can reimplement the popup hosting logic in Astra.
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

  active_popup_->ClosePopup();
  // active_popup_ will be cleared by OnExtensionPopupClosed callback.
}

// =========================================================================
// AstraExtensionPopupDelegate
// =========================================================================

void AstraSidebarExtensionsView::OnExtensionPopupClosed(
    const std::string& extension_id) {
  // Clear the active popup reference.
  // The popup widget is being destroyed — don't access the view pointer.
  if (active_popup_extension_id_ == extension_id) {
    active_popup_ = nullptr;
    active_popup_extension_id_.clear();
  }

  // Update the icon's popup-showing state.
  AstraExtensionIconView* icon_view = FindIconView(extension_id);
  if (icon_view) {
    icon_view->SetPopupShowing(false);
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
  //   which has all the standard extension menu items:
  //     - Options
  //     - Remove from Chrome
  //     - Manage extensions
  //     - Pin to toolbar / Unpin
  //     - etc.
  //
  //   We should use that model with a views::MenuRunner to show the menu.
  //
  //   Chromium owner: ExtensionContextMenuModel
  //     (chrome/browser/extensions/extension_context_menu_model.h)
  //   Chromium owner: MenuRunner (ui/views/controls/menu/menu_runner.h)
  //
  //   Patch point: None needed — we can use these classes directly.
  //   We just need to create the model and run the menu.

  // Placeholder: no-op for now.
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

void AstraSidebarExtensionsView::SetSectionVisible(bool visible) {
  SetVisible(visible);
}

}  // namespace astra
